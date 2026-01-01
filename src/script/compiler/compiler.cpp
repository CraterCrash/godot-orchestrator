// This file is part of the Godot Orchestrator project.
//
// Copyright (c) 2023-present Crater Crash Studios LLC and its contributors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//		http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "script/compiler/compiler.h"

#include "common/dictionary_utils.h"
#include "common/error_list.h"
#include "core/godot/config/project_settings.h"
#include "core/godot/object/class_db.h"
#include "core/godot/variant/variant.h"
#include "script/compiler/analyzer.h"
#include "script/compiler/bytecode_generator.h"
#include "script/script.h"
#include "script/script_cache.h"
#include "script/script_instance.h"
#include "script/script_server.h"
#include "script/utility_functions.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/engine_debugger.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

// todo:
//  Currently GodotCPP does not allow GDExtension code to lookup MethodBind pointers from the engine.
//  This causes errors during validating method call lookup, so this define excludes making sure calls
//  until such time that GodotCPP allows this.
// #define ALLOW_VALIDATED_METHOD_CALL

static bool is_exact_type(const PropertyInfo &p_par_type, const OScriptDataType &p_arg_type) {
    if (!p_arg_type.has_type()) {
        return false;
    }

    if (p_par_type.type == Variant::NIL) {
        return false;
    }

    if (p_par_type.type == Variant::OBJECT) {
        if (p_arg_type.kind == OScriptDataType::BUILTIN) {
            return false;
        }
        StringName class_name;
        if (p_arg_type.kind == OScriptDataType::NATIVE) {
            class_name = p_arg_type.native_type;
        } else {
            class_name = p_arg_type.native_type == StringName() ? p_arg_type.script_type->get_instance_base_type() : p_arg_type.native_type;
        }
        return p_par_type.class_name == class_name || ClassDB::is_parent_class(class_name, p_par_type.class_name);
    } else {
        if (p_arg_type.kind != OScriptDataType::BUILTIN) {
            return false;
        }
        return p_par_type.type == p_arg_type.builtin_type;
    }
}

#ifdef ALLOW_VALIDATED_METHOD_CALL
static bool can_use_validate_call(const MethodBind *p_method, const Vector<OScriptCodeGenerator::Address> &p_arguments) {
    ERR_FAIL_NULL_V_MSG(p_method, false, "Cannot use validated method call, method lookup failed");

    if (p_method->is_vararg()) {
        // Validated call won't work with vararg methods.
        return false;
    }
    if (p_method->get_argument_count() != p_arguments.size()) {
        // Validated call won't work with default arguments.
        return false;
    }

    MethodInfo info;
    GDE::ClassDB::get_method_info(p_method->get_instance_class(), p_method->get_name(), info);
    for (uint64_t i = 0; i < info.arguments.size(); i++) {
        if (!is_exact_type(info.arguments[i], p_arguments[i].type)) {
            return false;
        }
    }
    return true;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OScriptCompiler::CompilerContext

OScriptCodeGenerator::Address OScriptCompiler::CompilerContext::add_local(const StringName& p_name, const OScriptDataType& p_type) {
    uint32_t addr = generator->add_local(p_name, p_type);
    locals[p_name] = OScriptCodeGenerator::Address(OScriptCodeGenerator::Address::LOCAL_VARIABLE, addr, p_type);
    return locals[p_name];
}

OScriptCodeGenerator::Address OScriptCompiler::CompilerContext::add_local_constant(const StringName& p_name, const Variant& p_value) {
    uint32_t addr = generator->add_local_constant(p_name, p_value);
    locals[p_name] = OScriptCodeGenerator::Address(OScriptCodeGenerator::Address::CONSTANT, addr);
    return locals[p_name];
}

OScriptCodeGenerator::Address OScriptCompiler::CompilerContext::add_temporary(const OScriptDataType& p_type) {
    uint32_t addr = generator->add_temporary(p_type);
    return OScriptCodeGenerator::Address(OScriptCodeGenerator::Address::TEMPORARY, addr, p_type);
}

OScriptCodeGenerator::Address OScriptCompiler::CompilerContext::add_constant(const Variant& p_value) {
    OScriptDataType type;
    type.kind = OScriptDataType::BUILTIN;
    type.builtin_type = p_value.get_type();

    if (type.builtin_type == Variant::OBJECT) {
        Object* object = p_value;
        if (object) {
            type.kind = OScriptDataType::NATIVE;
            type.native_type = object->get_class();

            Ref<Script> scr = object->get_script();
            if (scr.is_valid()) {
                type.script_type = scr.ptr();
                Ref<OScript> oscript = scr;
                if (oscript.is_valid()) {
                    type.kind = OScriptDataType::OSCRIPT;
                } else {
                    type.kind = OScriptDataType::SCRIPT;
                }
            }
        } else {
            type.builtin_type = Variant::NIL;
        }
    }

    uint32_t addr = generator->add_or_get_constant(p_value);
    return OScriptCodeGenerator::Address(OScriptCodeGenerator::Address::CONSTANT, addr, type);
}

void OScriptCompiler::CompilerContext::start_block() {
    HashMap<StringName, OScriptCodeGenerator::Address> old_locals = locals;
    locals_stack.push_back(old_locals);
    generator->start_block();
}

void OScriptCompiler::CompilerContext::end_block() {
    locals = locals_stack.back()->get();
    locals_stack.pop_back();
    generator->end_block();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OScriptCompiler

bool OScriptCompiler::is_class_member_property(CompilerContext& p_context, const StringName& p_name) {
    if (p_context.function_node && p_context.function_node->is_static) {
        return false;
    }
    if (is_local_or_parameter(p_context, p_name)) {
        return false; // shadowed
    }
    return is_class_member_property(p_context.script, p_name);
}

bool OScriptCompiler::is_class_member_property(OScript* p_owner, const StringName& p_name) {
    OScript* script = p_owner;
    OScriptNativeClass* nc = nullptr;
    while (script) {
        if (script->native.is_valid()) {
            nc = script->native.ptr();
        }
        script = script->base.ptr();
    }

    ERR_FAIL_NULL_V(nc, false);

    return GDE::ClassDB::has_property(nc->get_name(), p_name);
}

bool OScriptCompiler::is_local_or_parameter(CompilerContext& p_context, const StringName& p_name) {
    return p_context.parameters.has(p_name) || p_context.locals.has(p_name);
}

bool OScriptCompiler::has_utility_function(const StringName& p_name) {
    return ExtensionDB::get_function_names().has(p_name);
}

void OScriptCompiler::set_error(const String& p_error, const OScriptParser::Node* p_node) {
    if (!error.is_empty()) {
        return;
    }

    error = p_error;
    err_node_id = p_node ? p_node->script_node_id : -1;
}

OScriptDataType OScriptCompiler::resolve_type(const OScriptParser::DataType& p_type, OScript* p_owner, bool p_handle_metatype) {
    if (!p_type.is_set() || !p_type.is_hard_type() || p_type.is_coroutine) {
		return OScriptDataType();
	}

	OScriptDataType result;

	switch (p_type.kind) {
		case OScriptParser::DataType::VARIANT: {
			result.kind = OScriptDataType::VARIANT;
		    break;
		}
		case OScriptParser::DataType::BUILTIN: {
			result.kind = OScriptDataType::BUILTIN;
			result.builtin_type = p_type.builtin_type;
		    break;
		}
		case OScriptParser::DataType::NATIVE: {
			if (p_handle_metatype && p_type.is_meta_type) {
				result.kind = OScriptDataType::NATIVE;
				result.builtin_type = Variant::OBJECT;
				// Fixes GH-82255. `GDScriptNativeClass` is obtainable in GDScript,
				// but is not a registered and exposed class, so `GDScriptNativeClass`
				// is missing from `GDScriptLanguage::get_singleton()->get_global_map()`.
				//result.native_type = GDScriptNativeClass::get_class_static();
				result.native_type = Object::get_class_static();
				break;
			}

			result.kind = OScriptDataType::NATIVE;
			result.builtin_type = p_type.builtin_type;
			result.native_type = p_type.native_type;

            #ifdef DEBUG_ENABLED
			if (unlikely(!OScriptLanguage::get_singleton()->get_global_map().has(result.native_type))) {
				set_error(vformat(R"(OScript bug (please report): Native class "%s" not found.)", result.native_type), nullptr);
				return OScriptDataType();
			}
            #endif
		    break;
		}
		case OScriptParser::DataType::SCRIPT: {
			if (p_handle_metatype && p_type.is_meta_type) {
				result.kind = OScriptDataType::NATIVE;
				result.builtin_type = Variant::OBJECT;
				result.native_type = p_type.script_type.is_valid() ? StringName(p_type.script_type->get_class()) : Script::get_class_static();
				break;
			}

			result.kind = OScriptDataType::SCRIPT;
			result.builtin_type = p_type.builtin_type;
			result.script_type_ref = p_type.script_type;
			result.script_type = result.script_type_ref.ptr();
			result.native_type = p_type.native_type;
		    break;
		}
		case OScriptParser::DataType::CLASS: {
			if (p_handle_metatype && p_type.is_meta_type) {
				result.kind = OScriptDataType::NATIVE;
				result.builtin_type = Variant::OBJECT;
				result.native_type = OScript::get_class_static();
				break;
			}

			result.kind = OScriptDataType::OSCRIPT;
			result.builtin_type = p_type.builtin_type;
			result.native_type = p_type.native_type;

			bool is_local_class = parser->has_class(p_type.class_type);

			Ref<OScript> script;
			if (is_local_class) {
				script = Ref<OScript>(main_script);
			} else {
				Error err = OK;
				script = OScriptCache::get_shallow_script(p_type.script_path, err, p_owner->path);
				if (err) {
					set_error(vformat(R"(Could not find script "%s": %s)", p_type.script_path, error_names[err]), nullptr);
					return OScriptDataType();
				}
			}

			if (script.is_valid()) {
				script = Ref<OScript>(script->find_class(p_type.class_type->fqcn));
			}

			if (script.is_null()) {
				set_error(vformat(R"(Could not find class "%s" in "%s".)", p_type.class_type->fqcn, p_type.script_path), nullptr);
				return OScriptDataType();
			} else {
				// Only hold a strong reference if the owner of the element qualified with this type is not local, to avoid cyclic references (leaks).
				// TODO: Might lead to use after free if script_type is a subclass and is used after its parent is freed.
				if (!is_local_class) {
					result.script_type_ref = script;
				}
				result.script_type = script.ptr();
				result.native_type = p_type.native_type;
			}
		    break;
		}
		case OScriptParser::DataType::ENUM: {
		    if (p_handle_metatype && p_type.is_meta_type) {
		        result.kind = OScriptDataType::BUILTIN;
		        result.builtin_type = Variant::DICTIONARY;
		        break;
		    }

		    result.kind = OScriptDataType::BUILTIN;
		    result.builtin_type = p_type.builtin_type;
		    break;
		}
		case OScriptParser::DataType::RESOLVING:
		case OScriptParser::DataType::UNRESOLVED: {
			set_error("Parser bug (please report): converting unresolved type.", nullptr);
			return OScriptDataType();
		}
	}

	for (int i = 0; i < p_type.container_element_types.size(); i++) {
		result.set_container_element_type(i, resolve_type(p_type.get_container_element_type_or_variant(i), p_owner, false));
	}

	return result;
}

List<OScriptCodeGenerator::Address> OScriptCompiler::add_block_locals(CompilerContext& p_context, const OScriptParser::SuiteNode* p_block) {
    List<OScriptCodeGenerator::Address> addresses;
    for (int i = 0; i < p_block->locals.size(); i++) {
        if (p_block->locals[i].type == OScriptParser::SuiteNode::Local::PARAMETER ||
            p_block->locals[i].type == OScriptParser::SuiteNode::Local::FOR_VARIABLE) {
            // Parameters are added directly from function and loop variables are declared explicitly.
            continue;
        }
        addresses.push_back(p_context.add_local(p_block->locals[i].name, resolve_type(p_block->locals[i].get_data_type(), p_context.script)));
    }
    return addresses;
}

// Avoid keeping in the stack long-lived references to objects, which may prevent `RefCounted` objects from being freed.
void OScriptCompiler::clear_block_locals(CompilerContext& p_context, const List<OScriptCodeGenerator::Address>& p_locals) {
    for (const OScriptCodeGenerator::Address &local : p_locals) {
        if (local.type.can_contain_object()) {
            p_context.generator->clear_address(local);
        }
    }
}

Error OScriptCompiler::parse_setter_getter(OScript* p_script, const OScriptParser::ClassNode* p_class, const OScriptParser::VariableNode* p_variable, bool p_is_setter) {
    OScriptParser::FunctionNode* function = p_is_setter ? p_variable->setter : p_variable->getter;

    Error err;
    parse_function(err, p_script, p_class, function);
    return err;
}

OScriptCompiledFunction* OScriptCompiler::parse_function(Error& r_error, OScript* p_script, const OScriptParser::ClassNode* p_class, const OScriptParser::FunctionNode* p_func, bool p_for_ready, bool p_for_lambda) {
    r_error = OK;

    CompilerContext context;
    context.generator = memnew(OScriptBytecodeGenerator);
    context.class_node = p_class;
    context.script = p_script;
    context.function_node = p_func;

    StringName func_name;
    bool is_abstract = false;
    bool is_static = false;
    Variant rpc_config;

    // Start with no return
    OScriptDataType return_type;
    return_type.kind = OScriptDataType::BUILTIN;
    return_type.builtin_type = Variant::NIL;

    if (p_func) {
        if (p_func->identifier) {
            func_name = p_func->identifier->name;
        } else {
            func_name = "<anonymous lambda>";
        }

        is_abstract = p_func->is_abstract;
        is_static = p_func->is_static;
        rpc_config = p_func->rpc_config;
        return_type = resolve_type(p_func->get_datatype(), p_script);
    } else {
        if (p_for_ready) {
            func_name = "@implicit_ready";
        } else {
            func_name = "@implicit_new";
        }
    }

    context.function_name = func_name;
    context.is_static = is_static;

    MethodInfo method_info;
    method_info.name = func_name;

    if (is_abstract) {
        method_info.flags |= METHOD_FLAG_VIRTUAL_REQUIRED;
    }

    if (is_static) {
        method_info.flags |= METHOD_FLAG_STATIC;
    }

    context.generator->write_start(p_script, func_name, is_static, rpc_config, return_type);

    int optional_parameters = 0;
    OScriptCodeGenerator::Address vararg_addr;

    if (p_func) {
        context.generator->write_newline(p_func->script_node_id);

        for (int i = 0; i < p_func->parameters.size(); i++) {
            const OScriptParser::ParameterNode* parameter = p_func->parameters[i];
            OScriptDataType type = resolve_type(parameter->get_datatype(), p_script);

            uint32_t addr = context.generator->add_parameter(parameter->identifier->name, parameter->initializer != nullptr, type);
            context.parameters[parameter->identifier->name] = OScriptCodeGenerator::Address(OScriptCodeGenerator::Address::FUNCTION_PARAMETER, addr, type);

            method_info.arguments.push_back(parameter->get_datatype().to_property_info(parameter->identifier->name));
            if (parameter->initializer != nullptr) {
                optional_parameters++;
            }
        }

        if (p_func->is_vararg()) {
            vararg_addr = context.add_local(p_func->rest_parameter->identifier->name, resolve_type(p_func->rest_parameter->get_datatype(), context.script));
            method_info.flags |= METHOD_FLAG_VARARG;
        }

        for (const Variant& item : p_func->default_arg_values) {
            method_info.default_arguments.push_back(item);
        }
    }

    // Parse initializer if applies.
    bool is_implicit_initializer = !p_for_ready && !p_func && !p_for_lambda;
    bool is_initializer = p_func && !p_for_lambda && p_func->identifier->name == OScriptLanguage::get_singleton()->strings._init;
    bool is_implicit_ready = !p_func && p_for_ready;

    if (!p_for_lambda && is_implicit_initializer) {
        // Initialize the default values for typed variables before anything.
        // This avoids crashes if they are accessed with validated calls before being properly initialized.
        // It may happen with out-of-order access or with `@onready` variables.
        for (const OScriptParser::ClassNode::Member& member : p_class->members) {
            if (member.type != OScriptParser::ClassNode::Member::VARIABLE) {
                continue;
            }

            const OScriptParser::VariableNode* field = member.variable;
            if (field->is_static) {
                continue;
            }

            OScriptDataType type = resolve_type(field->get_datatype(), context.script);
            if (type.has_type()) {
                context.generator->write_newline(field->script_node_id);
                OScriptCodeGenerator::Address dst_addr(OScriptCodeGenerator::Address::MEMBER, context.script->member_indices[field->identifier->name].index, type);

                if (type.builtin_type == Variant::ARRAY && type.has_container_element_types()) {
                    context.generator->write_construct_typed_array(dst_addr, type.get_container_element_type(0), Vector<OScriptCodeGenerator::Address>());
                } else if (type.builtin_type == Variant::DICTIONARY && type.has_container_element_types()) {
                    context.generator->write_construct_typed_dictionary(dst_addr, type.get_container_element_type_or_variant(0), type.get_container_element_type_or_variant(1), Vector<OScriptCodeGenerator::Address>());
                } else if (type.kind == OScriptDataType::BUILTIN) {
                    context.generator->write_construct(dst_addr, type.builtin_type, Vector<OScriptCodeGenerator::Address>());
                } else {
                    // Objects and such, left as null.
                }
            }
        }
    }

    if (!p_for_lambda && (is_implicit_initializer || is_implicit_ready)) {
        // Initialize class fields
        for (int i = 0; i < p_class->members.size(); i++) {
            if (p_class->members[i].type != OScriptParser::ClassNode::Member::VARIABLE) {
                continue;
            }

            const OScriptParser::VariableNode* field = p_class->members[i].variable;
            if (field->is_static) {
                continue;
            }

            if (field->onready != is_implicit_ready) {
                // Only initialize in `@implicit_ready`
                continue;
            }

            if (field->initializer) {
                context.generator->write_newline(field->initializer->script_node_id);

                OScriptCodeGenerator::Address src = parse_expression(context, r_error, field->initializer, false, true);
                if (r_error) {
                    memdelete(context.generator);
                    return nullptr;
                }

                OScriptDataType type = resolve_type(field->get_datatype(), context.script);
                OScriptCodeGenerator::Address dst(OScriptCodeGenerator::Address::MEMBER, context.script->member_indices[field->identifier->name].index, type);

                if (field->use_conversion_assign) {
                    context.generator->write_assign_with_conversion(dst, src);
                } else {
                    context.generator->write_assign(dst, src);
                }

                if (src.mode == OScriptCodeGenerator::Address::TEMPORARY) {
                    context.generator->pop_temporary();
                }
            }
        }
    }

    if (p_func) {
        if (optional_parameters > 0) {
            context.generator->start_parameters();
            for (int i = p_func->parameters.size() - optional_parameters; i < p_func->parameters.size(); i++) {
                const OScriptParser::ParameterNode* parameter = p_func->parameters[i];
                OScriptCodeGenerator::Address src = parse_expression(context, r_error, parameter->initializer);
                if (r_error) {
                    memdelete(context.generator);
                    return nullptr;
                }

                OScriptCodeGenerator::Address dst = context.parameters[parameter->identifier->name];
                context.generator->write_assign_default_parameter(dst, src, parameter->use_conversion_assign);
                if (src.mode == OScriptCodeGenerator::Address::TEMPORARY) {
                    context.generator->pop_temporary();
                }
            }
            context.generator->end_parameters();
        }

        // No need to reset locals at the end of the function, the stack will be cleared
        r_error = parse_block(context, p_func->body, true, false);
        if (r_error) {
            memdelete(context.generator);
            return nullptr;
        }
    }

    #ifdef DEBUG_ENABLED
    if (EngineDebugger::get_singleton()->is_active()) {
        String signature;
        // Path
        if (!p_script->get_script_path().is_empty()) {
            signature += p_script->get_script_path();
        }

        // Location
        if (p_func) {
            signature += "::" + itos(p_func->body->script_node_id);
        } else {
            signature += "::0";
        }

        if (p_class->identifier) {
            signature += "::" + String(p_class->identifier->name) + "." + String(func_name);
        } else {
            signature += "::" + String(func_name);
        }

        if (p_for_lambda) {
            signature += "(lambda)";
        }

        context.generator->set_signature(signature);
    }
    #endif

    if (p_func) {
        context.generator->set_initial_node_id(p_func->script_node_id);
    } else {
        context.generator->set_initial_node_id(0);
    }

    OScriptCompiledFunction* func = context.generator->write_end();
    if (is_initializer) {
        p_script->initializer = func;
    } else if (is_implicit_initializer) {
        p_script->implicit_initializer = func;
    } else if (is_implicit_ready) {
        p_script->implicit_ready = func;
    }

    if (p_func) {
        // If no 'return' statement, then return type is always 'void', not 'Variant'.
        if (p_func->body->has_return) {
            func->return_type = resolve_type(p_func->get_datatype(), p_script);
            method_info.return_val = p_func->get_datatype().to_property_info(String());
        } else {
            func->return_type = OScriptDataType();
            func->return_type.kind = OScriptDataType::BUILTIN;
            func->return_type.builtin_type = Variant::NIL;
        }

        if (p_func->is_vararg()) {
            func->vararg_index = vararg_addr.address;
        }
    }

    func->method_info = method_info;
    if (!is_implicit_initializer && !is_implicit_ready && !p_for_lambda) {
        p_script->member_functions[func_name] = func;
    }

    memdelete(context.generator);
    return func;
}

OScriptCompiledFunction* OScriptCompiler::make_static_initializer(Error& r_error, OScript* p_script, const OScriptParser::ClassNode* p_class) {
    r_error = OK;

    StringName func_name = StringName("@static_initializer");
    bool is_static = true;
    Variant rpc_config;

    OScriptDataType return_type;
    return_type.kind = OScriptDataType::BUILTIN;
    return_type.builtin_type = Variant::NIL;

    CompilerContext context;
    context.generator = memnew(OScriptBytecodeGenerator);
    context.class_node = p_class;
    context.script = p_script;
    context.function_name = func_name;
    context.is_static = is_static;
    context.generator->write_start(p_script, func_name, is_static, rpc_config, return_type);

    // The static initializer is always called on the same class where the static variables are defined.
    // So the CLASS address (current class) can be used instead of adding a constant.
    OScriptCodeGenerator::Address class_addr(OScriptCodeGenerator::Address::CLASS);

    // Initialize the default values for typed variables before anything.
    // This avoids crashes if they are accessed with validated calls before being initialized.
    // It could happen with out-of-order access or with `@onready` variables.
    for (const OScriptParser::ClassNode::Member& member : p_class->members) {
        if (member.type != OScriptParser::ClassNode::Member::VARIABLE) {
            continue;
        }

        const OScriptParser::VariableNode* field = member.variable;
        if (!field->is_static) {
            continue;
        }

        OScriptDataType type = resolve_type(field->get_datatype(), context.script);
        if (type.has_type()) {
            context.generator->write_newline(field->script_node_id);
            if (type.builtin_type == Variant::ARRAY && type.has_container_element_type(0)) {
                OScriptCodeGenerator::Address temp = context.add_temporary(type);
                context.generator->write_construct_typed_array(temp, type.get_container_element_type(0), Vector<OScriptCodeGenerator::Address>());
                context.generator->write_set_static_variable(temp, class_addr, p_script->static_variables_indices[field->identifier->name].index);
                context.generator->pop_temporary();
            } else if (type.builtin_type == Variant::DICTIONARY && type.has_container_element_types()) {
                OScriptCodeGenerator::Address temp = context.add_temporary(type);
                context.generator->write_construct_typed_dictionary(temp, type.get_container_element_type_or_variant(0), type.get_container_element_type_or_variant(1), Vector<OScriptCodeGenerator::Address>());
                context.generator->write_set_static_variable(temp, class_addr, p_script->static_variables_indices[field->identifier->name].index);
                context.generator->pop_temporary();
            } else if (type.kind == OScriptDataType::BUILTIN) {
                OScriptCodeGenerator::Address temp = context.add_temporary(type);
                context.generator->write_construct(temp, type.builtin_type, Vector<OScriptCodeGenerator::Address>());
                context.generator->write_set_static_variable(temp, class_addr, p_script->static_variables_indices[field->identifier->name].index);
                context.generator->pop_temporary();
            }
        }
    }

    for (int i = 0; i < p_class->members.size(); i++) {
        // Initialize static fields.
        if (p_class->members[i].type != OScriptParser::ClassNode::Member::VARIABLE) {
            continue;
        }
        const OScriptParser::VariableNode *field = p_class->members[i].variable;
        if (!field->is_static) {
            continue;
        }

        if (field->initializer) {
            context.generator->write_newline(field->initializer->script_node_id);

            OScriptCodeGenerator::Address src_address = parse_expression(context, r_error, field->initializer, false, true);
            if (r_error) {
                memdelete(context.generator);
                return nullptr;
            }

            OScriptDataType field_type = resolve_type(field->get_datatype(), context.script);
            OScriptCodeGenerator::Address temp = context.add_temporary(field_type);

            if (field->use_conversion_assign) {
                context.generator->write_assign_with_conversion(temp, src_address);
            } else {
                context.generator->write_assign(temp, src_address);
            }
            if (src_address.mode == OScriptCodeGenerator::Address::TEMPORARY) {
                context.generator->pop_temporary();
            }

            context.generator->write_set_static_variable(temp, class_addr, p_script->static_variables_indices[field->identifier->name].index);
            context.generator->pop_temporary();
        }
    }

    if (p_script->has_method(OScriptLanguage::get_singleton()->strings._static_init)) {
        context.generator->write_newline(p_class->script_node_id);
        context.generator->write_call(OScriptCodeGenerator::Address(), class_addr, OScriptLanguage::get_singleton()->strings._static_init, Vector<OScriptCodeGenerator::Address>());
    }

    #ifdef DEBUG_ENABLED
    if (EngineDebugger::get_singleton()->is_active()) {
        String signature;

        // Path.
        if (!p_script->get_script_path().is_empty()) {
            signature += p_script->get_script_path();
        }

        // Location.
        signature += "::0";

        // Function and class.
        if (p_class->identifier) {
            signature += "::" + String(p_class->identifier->name) + "." + String(func_name);
        } else {
            signature += "::" + String(func_name);
        }

        context.generator->set_signature(signature);
    }
    #endif

    context.generator->set_initial_node_id(p_class->script_node_id);

    OScriptCompiledFunction* func = context.generator->write_end();
    memdelete(context.generator);

    return func;
}

Error OScriptCompiler::parse_block(CompilerContext& p_context, const OScriptParser::SuiteNode* p_block, bool p_add_locals, bool p_clear_locals) {
    Error err = OK;
    OScriptCodeGenerator* generator = p_context.generator;
    List<OScriptCodeGenerator::Address> block_locals;

    generator->clear_temporaries();
    p_context.start_block();

    if (p_add_locals) {
        block_locals = add_block_locals(p_context, p_block);
    }

    for (int i = 0; i < p_block->statements.size(); i++) {
        const OScriptParser::Node* s = p_block->statements[i];

        generator->write_newline(s->script_node_id);

        switch (s->type) {
            case OScriptParser::Node::MATCH: {
				const OScriptParser::MatchNode *match = static_cast<const OScriptParser::MatchNode *>(s);

				p_context.start_block(); // Add an extra block, since @special locals belong to the match scope.

				// Evaluate the match expression.
				OScriptCodeGenerator::Address value = p_context.add_local("@match_value", resolve_type(match->test->get_datatype(), p_context.script));
				OScriptCodeGenerator::Address value_expr = parse_expression(p_context, err, match->test);
				if (err) {
					return err;
				}

				// Assign to local.
				// TODO: This can be improved by passing the target to parse_expression().
				generator->write_assign(value, value_expr);

				if (value_expr.mode == OScriptCodeGenerator::Address::TEMPORARY) {
					p_context.generator->pop_temporary();
				}

				// Then, let's save the type of the value in the stack too, so we can reuse for later comparisons.
				OScriptDataType typeof_type;
				typeof_type.kind = OScriptDataType::BUILTIN;
				typeof_type.builtin_type = Variant::INT;
				OScriptCodeGenerator::Address type = p_context.add_local("@match_type", typeof_type);

				Vector<OScriptCodeGenerator::Address> typeof_args;
				typeof_args.push_back(value);
				generator->write_call_utility(type, "typeof", typeof_args);

				// Now we can actually start testing.
				// For each branch.
				for (int j = 0; j < match->branches.size(); j++) {
					if (j > 0) {
						// Use `else` to not check the next branch after matching.
						generator->write_else();
					}

					const OScriptParser::MatchBranchNode *branch = match->branches[j];

					p_context.start_block(); // Add an extra block, since binds belong to the match branch scope.

					// Add locals in block before patterns, so temporaries don't use the stack address for binds.
					List<OScriptCodeGenerator::Address> branch_locals = add_block_locals(p_context, branch->block);

					generator->write_newline(branch->script_node_id);

					// For each pattern in branch.
					OScriptCodeGenerator::Address pattern_result = p_context.add_temporary();
					for (int k = 0; k < branch->patterns.size(); k++) {
						pattern_result = parse_match_pattern(p_context, err, branch->patterns[k], value, type, pattern_result, k == 0, false);
						if (err != OK) {
							return err;
						}
					}

					// If there's a guard, check its condition too.
					if (branch->guard_body != nullptr) {
						// Do this first so the guard does not run unless the pattern matched.
						generator->write_and_left_operand(pattern_result);

						// Don't actually use the block for the guard.
						// The binds are already in the locals and we don't want to clear the result of the guard condition before we check the actual match.
						OScriptCodeGenerator::Address guard_result = parse_expression(p_context, err, static_cast<OScriptParser::ExpressionNode *>(branch->guard_body->statements[0]));
						if (err) {
							return err;
						}

						generator->write_and_right_operand(guard_result);
						generator->write_end_and(pattern_result);

						if (guard_result.mode == OScriptCodeGenerator::Address::TEMPORARY) {
							p_context.generator->pop_temporary();
						}
					}

					// Check if pattern did match.
					generator->write_if(pattern_result);

					// Remove the result from stack.
					generator->pop_temporary();

					// Parse the branch block.
					err = parse_block(p_context, branch->block, false); // Don't add locals again.
					if (err) {
						return err;
					}

					clear_block_locals(p_context, branch_locals);

					p_context.end_block(); // Get out of extra block for binds.
				}

				// End all nested `if`s.
				for (int j = 0; j < match->branches.size(); j++) {
					generator->write_endif();
				}

				p_context.end_block(); // Get out of extra block for match's @special locals.
                break;
			}
            case OScriptParser::Node::IF: {
                const OScriptParser::IfNode* node = static_cast<const OScriptParser::IfNode*>(s);
                OScriptCodeGenerator::Address cond = parse_expression(p_context, err, node->condition);
                if (err) {
                    return err;
                }

                generator->write_if(cond);
                if (cond.mode == OScriptBytecodeGenerator::Address::TEMPORARY) {
                    p_context.generator->pop_temporary();
                }

                err = parse_block(p_context, node->true_block);
                if (err) {
                    return err;
                }

                if (node->false_block) {
                    generator->write_else();

                    err = parse_block(p_context, node->false_block);
                    if (err) {
                        return err;
                    }
                }

                generator->write_endif();
                break;
            }
            case OScriptParser::Node::FOR: {
				const OScriptParser::ForNode *for_n = static_cast<const OScriptParser::ForNode *>(s);

				// Add an extra block, since the iterator and @special locals belong to the loop scope.
				// Also we use custom logic to clear block locals.
				p_context.start_block();

				OScriptCodeGenerator::Address iterator = p_context.add_local(for_n->variable->name, resolve_type(for_n->variable->get_datatype(), p_context.script));

				// Optimize `range()` call to not allocate an array.
				OScriptParser::CallNode *range_call = nullptr;
				if (for_n->list && for_n->list->type == OScriptParser::Node::CALL) {
					OScriptParser::CallNode *call = static_cast<OScriptParser::CallNode *>(for_n->list);
					if (call->get_callee_type() == OScriptParser::Node::IDENTIFIER) {
						if (static_cast<OScriptParser::IdentifierNode *>(call->callee)->name == StringName("range")) {
							range_call = call;
						}
					}
				}

				generator->start_for(iterator.type, resolve_type(for_n->list->get_datatype(), p_context.script), range_call != nullptr);

				if (range_call != nullptr) {
					Vector<OScriptCodeGenerator::Address> args;
					args.resize(range_call->arguments.size());

					for (int j = 0; j < args.size(); j++) {
						args.write[j] = parse_expression(p_context, err, range_call->arguments[j]);
						if (err) {
							return err;
						}
					}

					switch (args.size()) {
						case 1:
							generator->write_for_range_assignment(p_context.add_constant(0), args[0], p_context.add_constant(1));
							break;
						case 2:
							generator->write_for_range_assignment(args[0], args[1], p_context.add_constant(1));
							break;
						case 3:
							generator->write_for_range_assignment(args[0], args[1], args[2]);
							break;
						default:
							set_error(R"*(Analyzer bug: Wrong "range()" argument count.)*", range_call);
							return ERR_BUG;
					}

					for (int j = 0; j < args.size(); j++) {
						if (args[j].mode == OScriptCodeGenerator::Address::TEMPORARY) {
							p_context.generator->pop_temporary();
						}
					}
				} else {
					OScriptCodeGenerator::Address list = parse_expression(p_context, err, for_n->list);
					if (err) {
						return err;
					}

					generator->write_for_list_assignment(list);

					if (list.mode == OScriptCodeGenerator::Address::TEMPORARY) {
						p_context.generator->pop_temporary();
					}
				}

				generator->write_for(iterator, for_n->use_conversion_assign, range_call != nullptr);

				// Loop variables must be cleared even when `break`/`continue` is used.
				List<OScriptCodeGenerator::Address> loop_locals = add_block_locals(p_context, for_n->loop);

				//_clear_block_locals(codegen, loop_locals); // Inside loop, before block - for `continue`. // TODO

				err = parse_block(p_context, for_n->loop, false); // Don't add locals again.
				if (err) {
					return err;
				}

				generator->write_endfor(range_call != nullptr);

				clear_block_locals(p_context, loop_locals); // Outside loop, after block - for `break` and normal exit.

				p_context.end_block(); // Get out of extra block for loop iterator, @special locals, and custom locals clearing.
                break;
			}
            case OScriptParser::Node::WHILE: {
                const OScriptParser::WhileNode *while_n = static_cast<const OScriptParser::WhileNode *>(s);

                p_context.start_block(); // Add an extra block, since we use custom logic to clear block locals.

                generator->start_while_condition();

                OScriptCodeGenerator::Address condition = parse_expression(p_context, err, while_n->condition);
                if (err) {
                    return err;
                }

                generator->write_while(condition);

                if (condition.mode == OScriptCodeGenerator::Address::TEMPORARY) {
                    p_context.generator->pop_temporary();
                }

                // Loop variables must be cleared even when `break`/`continue` is used.
                List<OScriptCodeGenerator::Address> loop_locals = add_block_locals(p_context, while_n->loop);

                //_clear_block_locals(codegen, loop_locals); // Inside loop, before block - for `continue`. // TODO

                err = parse_block(p_context, while_n->loop, false); // Don't add locals again.
                if (err) {
                    return err;
                }

                generator->write_endwhile();

                clear_block_locals(p_context, loop_locals); // Outside loop, after block - for `break` and normal exit.

                p_context.end_block(); // Get out of extra block for custom locals clearing.
                break;
            }
            case OScriptParser::Node::BREAK: {
                generator->write_break();
                break;
            }
            case OScriptParser::Node::CONTINUE: {
                generator->write_continue();
                break;
            }
            case OScriptParser::Node::RETURN: {
                const OScriptParser::ReturnNode* node = static_cast<const OScriptParser::ReturnNode*>(s);
                OScriptCodeGenerator::Address value;
                if (node->return_value != nullptr) {
                    value = parse_expression(p_context, err, node->return_value);
                    if (err) {
                        return err;
                    }
                }

                if (node->void_return) {
                    // Always return 'nil'
                    generator->write_return(p_context.add_constant(Variant()));
                } else {
                    generator->write_return(value);
                }

                if (value.mode == OScriptCodeGenerator::Address::TEMPORARY) {
                    p_context.generator->pop_temporary();
                }
                break;
            }
            case OScriptParser::Node::ASSERT: {
                #ifdef DEBUG_ENABLED
                const OScriptParser::AssertNode *as = static_cast<const OScriptParser::AssertNode *>(s);

                OScriptCodeGenerator::Address condition = parse_expression(p_context, err, as->condition);
                if (err) {
                    return err;
                }

                OScriptCodeGenerator::Address message;

                if (as->message) {
                    message = parse_expression(p_context, err, as->message);
                    if (err) {
                        return err;
                    }
                }
                generator->write_assert(condition, message);

                if (condition.mode == OScriptCodeGenerator::Address::TEMPORARY) {
                    p_context.generator->pop_temporary();
                }
                if (message.mode == OScriptCodeGenerator::Address::TEMPORARY) {
                    p_context.generator->pop_temporary();
                }
                #endif
                break;
            }
            case OScriptParser::Node::BREAKPOINT: {
                #ifdef DEBUG_ENABLED
                generator->write_breakpoint();
                #endif
                break;
            }
            case OScriptParser::Node::VARIABLE: {
                const OScriptParser::VariableNode *lv = static_cast<const OScriptParser::VariableNode *>(s);

                // Should be already in stack when the block began.
                OScriptCodeGenerator::Address local = p_context.locals[lv->identifier->name];
                OScriptDataType local_type = resolve_type(lv->get_datatype(), p_context.script);

                bool initialized = false;
                if (lv->initializer != nullptr) {
                    OScriptCodeGenerator::Address src_address = parse_expression(p_context, err, lv->initializer);
                    if (err) {
                        return err;
                    }

                    if (lv->use_conversion_assign) {
                        generator->write_assign_with_conversion(local, src_address);
                    } else {
                        generator->write_assign(local, src_address);
                    }

                    if (src_address.mode == OScriptCodeGenerator::Address::TEMPORARY) {
                        p_context.generator->pop_temporary();
                    }
                    initialized = true;
                } else if (local_type.kind == OScriptDataType::BUILTIN || p_context.generator->is_local_dirty(local)) {
                    // Initialize with default for the type. Built-in types must always be cleared (they cannot be `null`).
                    // Objects and untyped variables are assigned to `null` only if the stack address has been reused and not cleared.
                    p_context.generator->clear_address(local);
                    initialized = true;
                }

                // Don't check `is_local_dirty()` since the variable must be assigned to `null` **on each iteration**.
                if (!initialized && p_block->is_in_loop) {
                    p_context.generator->clear_address(local);
                }
                break;
            }
            case OScriptParser::Node::CONSTANT: {
                // Local constants.
                const OScriptParser::ConstantNode *lc = static_cast<const OScriptParser::ConstantNode *>(s);
                if (!lc->initializer->is_constant) {
                    set_error("Local constant must have a constant value as initializer.", lc->initializer);
                    return ERR_PARSE_ERROR;
                }
                p_context.add_local_constant(lc->identifier->name, lc->initializer->reduced_value);
                break;
            }
            case OScriptParser::Node::PASS: {
                // Nothing to do.
                break;
            }
            default: {
                // Expression.
                if (s->is_expression()) {
                    OScriptCodeGenerator::Address expr = parse_expression(p_context, err, static_cast<const OScriptParser::ExpressionNode *>(s), true);
                    if (err) {
                        return err;
                    }
                    if (expr.mode == OScriptCodeGenerator::Address::TEMPORARY) {
                        p_context.generator->pop_temporary();
                    }
                } else {
                    set_error(vformat(R"(Compiler bug (please report): unexpected node type %d in parse tree while parsing statement.)", s->type), s); // Unreachable code.
                    return ERR_INVALID_DATA;
                }
                break;
            }
        }

        generator->clear_temporaries();
    }

    if (p_add_locals && p_clear_locals) {
        clear_block_locals(p_context, block_locals);
    }

    p_context.end_block();
    return OK;
}

OScriptCodeGenerator::Address OScriptCompiler::parse_expression(CompilerContext& p_context, Error& r_error, const OScriptParser::ExpressionNode* p_expression, bool p_root, bool p_initializer) {
    if (p_expression->is_constant && !(p_expression->get_datatype().is_meta_type && p_expression->get_datatype().kind == OScriptParser::DataType::CLASS)) {
        return p_context.add_constant(p_expression->reduced_value);
    }

    OScriptCodeGenerator* generator = p_context.generator;
    generator->write_newline(p_expression->script_node_id);

    switch (p_expression->type) {
        case OScriptParser::Node::IDENTIFIER: {
            // Look for identifiers in current scope.
			const OScriptParser::IdentifierNode *in = static_cast<const OScriptParser::IdentifierNode *>(p_expression);
			const StringName identifier = in->name;
			switch (in->source) {
			    // LOCALS.
			    case OScriptParser::IdentifierNode::FUNCTION_PARAMETER:
			    case OScriptParser::IdentifierNode::LOCAL_VARIABLE:
			    case OScriptParser::IdentifierNode::LOCAL_CONSTANT:
			    case OScriptParser::IdentifierNode::LOCAL_ITERATOR:
			    case OScriptParser::IdentifierNode::LOCAL_BIND: {
			        // Try function parameters.
			        if (p_context.parameters.has(identifier)) {
			            return p_context.parameters[identifier];
			        }

			        // Try local variables and constants.
			        if (!p_initializer && p_context.locals.has(identifier)) {
			            return p_context.locals[identifier];
			        }
			        break;
			    }
			    // MEMBERS.
			    case OScriptParser::IdentifierNode::MEMBER_VARIABLE:
			    case OScriptParser::IdentifierNode::MEMBER_FUNCTION:
			    case OScriptParser::IdentifierNode::MEMBER_SIGNAL:
			    case OScriptParser::IdentifierNode::INHERITED_VARIABLE: {
			        // Try class members.
			        if (is_class_member_property(p_context, identifier)) {
			            // Get property.
			            OScriptCodeGenerator::Address temp = p_context.add_temporary(resolve_type(p_expression->get_datatype(), p_context.script));
			            generator->write_get_member(temp, identifier);
			            return temp;
			        }

			        // Try members.
			        if (!p_context.function_node || !p_context.function_node->is_static) {
			            // Try member variables.
			            if (p_context.script->member_indices.has(identifier)) {
			                if (p_context.script->member_indices[identifier].getter != StringName() && p_context.script->member_indices[identifier].getter != p_context.function_name) {
			                    // Perform getter.
			                    OScriptCodeGenerator::Address temp = p_context.add_temporary(p_context.script->member_indices[identifier].data_type);
			                    Vector<OScriptCodeGenerator::Address> args; // No argument needed.
			                    generator->write_call_self(temp, p_context.script->member_indices[identifier].getter, args);
			                    return temp;
			                } else {
			                    // No getter or inside getter: direct member access.
			                    int idx = p_context.script->member_indices[identifier].index;
			                    return OScriptCodeGenerator::Address(OScriptCodeGenerator::Address::MEMBER, idx, p_context.script->get_member_type(identifier));
			                }
			            }
			        }

			        // Try methods and signals (can be Callable and Signal).
			        {
			            // Search upwards through parent classes:
			            const OScriptParser::ClassNode *base_class = p_context.class_node;
			            while (base_class != nullptr) {
			                if (base_class->has_member(identifier)) {
			                    const OScriptParser::ClassNode::Member &member = base_class->get_member(identifier);
			                    if (member.type == OScriptParser::ClassNode::Member::FUNCTION || member.type == OScriptParser::ClassNode::Member::SIGNAL) {
			                        // Get like it was a property.
			                        OScriptCodeGenerator::Address temp = p_context.add_temporary(); // TODO: Get type here.

			                        OScriptCodeGenerator::Address base(OScriptCodeGenerator::Address::SELF);
			                        if (member.type == OScriptParser::ClassNode::Member::FUNCTION && member.function->is_static) {
			                            base = OScriptCodeGenerator::Address(OScriptCodeGenerator::Address::CLASS);
			                        }

			                        generator->write_get_named(temp, identifier, base);
			                        return temp;
			                    }
			                }
			                base_class = base_class->base_type.class_type;
			            }

			            // Try in native base.
			            OScript *scr = p_context.script;
			            OScriptNativeClass *nc = nullptr;
			            while (scr) {
			                if (scr->native.is_valid()) {
			                    nc = scr->native.ptr();
			                }
			                scr = scr->base.ptr();
			            }

			            if (nc && (identifier == StringName("free")
                            || ClassDB::class_has_signal(nc->get_name(), identifier)
                            || ClassDB::class_has_method(nc->get_name(), identifier))) {
			                // Get like it was a property.
			                OScriptCodeGenerator::Address temp = p_context.add_temporary(); // TODO: Get type here.
			                OScriptCodeGenerator::Address self(OScriptCodeGenerator::Address::SELF);

			                generator->write_get_named(temp, identifier, self);
			                return temp;
                            }
			        }
			        break;
			    }
			    case OScriptParser::IdentifierNode::MEMBER_CONSTANT:
			    case OScriptParser::IdentifierNode::MEMBER_CLASS: {
			        // Try class constants.
			        OScript *owner = p_context.script;
			        while (owner) {
			            OScript *scr = owner;
			            OScriptNativeClass *nc = nullptr;

			            while (scr) {
			                if (scr->constants.has(identifier)) {
			                    return p_context.add_constant(scr->constants[identifier]); // TODO: Get type here.
			                }
			                if (scr->native.is_valid()) {
			                    nc = scr->native.ptr();
			                }
			                scr = scr->base.ptr();
			            }

			            // Class C++ integer constant.
			            if (nc) {
			                bool success = false;
			                int64_t constant = GDE::ClassDB::get_integer_constant(nc->get_name(), identifier, success);
			                if (success) {
			                    p_context.add_constant(constant);
			                }
			            }

			            owner = owner->subclass_owner;
			        }
			        break;
			    }
			    case OScriptParser::IdentifierNode::STATIC_VARIABLE: {
			        // Try static variables.
			        OScript *scr = p_context.script;
			        while (scr) {
			            if (scr->static_variables_indices.has(identifier)) {
			                if (scr->static_variables_indices[identifier].getter != StringName() && scr->static_variables_indices[identifier].getter != p_context.function_name) {
			                    // Perform getter.
			                    OScriptCodeGenerator::Address temp = p_context.add_temporary(scr->static_variables_indices[identifier].data_type);
			                    OScriptCodeGenerator::Address class_addr(OScriptCodeGenerator::Address::CLASS);
			                    Vector<OScriptCodeGenerator::Address> args; // No argument needed.
			                    generator->write_call(temp, class_addr, scr->static_variables_indices[identifier].getter, args);
			                    return temp;
			                } else {
			                    // No getter or inside getter: direct variable access.
			                    OScriptCodeGenerator::Address temp = p_context.add_temporary(scr->static_variables_indices[identifier].data_type);
			                    OScriptCodeGenerator::Address _class = p_context.add_constant(scr);
			                    int index = scr->static_variables_indices[identifier].index;
			                    generator->write_get_static_variable(temp, _class, index);
			                    return temp;
			                }
			            }
			            scr = scr->base.ptr();
			        }
			        break;
			    }
			    // GLOBALS.
			    case OScriptParser::IdentifierNode::NATIVE_CLASS:
			    case OScriptParser::IdentifierNode::UNDEFINED_SOURCE: {
			        // Try globals.
			        if (OScriptLanguage::get_singleton()->get_global_map().has(identifier)) {
			            if (GDE::ProjectSettings::has_singleton_autoload(identifier)) {
			                OScriptBytecodeGenerator::Address global = p_context.add_temporary(resolve_type(in->get_datatype(), p_context.script));
			                int idx = OScriptLanguage::get_singleton()->get_global_map()[identifier];
			                generator->write_store_global(global, idx);
			                return global;
			            } else {
			                int idx = OScriptLanguage::get_singleton()->get_global_map()[identifier];
			                Variant global = OScriptLanguage::get_singleton()->get_global_array()[idx];
			                return p_context.add_constant(global);
			            }
			        }

			        // Try global classes.
			        if (ScriptServer::is_global_class(identifier)) {
			            const OScriptParser::ClassNode *class_node = p_context.class_node;
			            while (class_node->outer) {
			                class_node = class_node->outer;
			            }

			            Ref<Resource> res;

			            if (class_node->identifier && class_node->identifier->name == identifier) {
			                res = Ref<OScript>(main_script);
			            } else {
			                ScriptServer::GlobalClass global_class = ScriptServer::get_global_class(identifier);
			                String global_class_path = global_class.path;
			                if (global_class.language == "OScript") {
			                    Error err = OK;
			                    // Should not need to pass p_owner since analyzer will already have done it.
			                    res = OScriptCache::get_shallow_script(global_class_path, err);
			                    if (err != OK) {
			                        set_error("Can't load global class " + String(identifier), p_expression);
			                        r_error = ERR_COMPILATION_FAILED;
			                        return OScriptCodeGenerator::Address();
			                    }
			                } else {
			                    res = ResourceLoader::get_singleton()->load(global_class_path);
			                    if (res.is_null()) {
			                        set_error("Can't load global class " + String(identifier) + ", cyclic reference?", p_expression);
			                        r_error = ERR_COMPILATION_FAILED;
			                        return OScriptCodeGenerator::Address();
			                    }
			                }
			            }

			            return p_context.add_constant(res);
			        }

			        #ifdef TOOLS_ENABLED
			        if (OScriptLanguage::get_singleton()->get_named_globals_map().has(identifier)) {
			            OScriptCodeGenerator::Address global = p_context.add_temporary(); // TODO: Get type.
			            generator->write_store_named_global(global, identifier);
			            return global;
			        }
			        #endif
			        break;
			    }
			}
            // Not found, error.
            set_error("Identifier not found: " + String(identifier), p_expression);
            r_error = ERR_COMPILATION_FAILED;
            return OScriptCodeGenerator::Address();
        }
        case OScriptParser::Node::LITERAL: {
            // Return constant.
            const OScriptParser::LiteralNode *cn = static_cast<const OScriptParser::LiteralNode *>(p_expression);
            return p_context.add_constant(cn->value);
        }
        case OScriptParser::Node::SELF: {
            //return constant
            if (p_context.function_node && p_context.function_node->is_static) {
                set_error("'self' not present in static function.", p_expression);
                r_error = ERR_COMPILATION_FAILED;
                return OScriptCodeGenerator::Address();
            }
            return OScriptCodeGenerator::Address(OScriptCodeGenerator::Address::SELF);
        }
        case OScriptParser::Node::ARRAY: {
            const OScriptParser::ArrayNode *an = static_cast<const OScriptParser::ArrayNode *>(p_expression);
            Vector<OScriptCodeGenerator::Address> values;

            // Create the result temporary first since it's the last to be killed.
            OScriptDataType array_type = resolve_type(an->get_datatype(), p_context.script);
            OScriptCodeGenerator::Address result = p_context.add_temporary(array_type);

            for (int i = 0; i < an->elements.size(); i++) {
                OScriptCodeGenerator::Address val = parse_expression(p_context, r_error, an->elements[i]);
                if (r_error) {
                    return OScriptCodeGenerator::Address();
                }
                values.push_back(val);
            }

            if (array_type.has_container_element_type(0)) {
                generator->write_construct_typed_array(result, array_type.get_container_element_type(0), values);
            } else {
                generator->write_construct_array(result, values);
            }

            for (int i = 0; i < values.size(); i++) {
                if (values[i].mode == OScriptCodeGenerator::Address::TEMPORARY) {
                    generator->pop_temporary();
                }
            }

            return result;
        }
        case OScriptParser::Node::DICTIONARY: {
            const OScriptParser::DictionaryNode *dn = static_cast<const OScriptParser::DictionaryNode *>(p_expression);
            Vector<OScriptCodeGenerator::Address> elements;

            // Create the result temporary first since it's the last to be killed.
            OScriptDataType dict_type = resolve_type(dn->get_datatype(), p_context.script);
            OScriptCodeGenerator::Address result = p_context.add_temporary(dict_type);

            for (int i = 0; i < dn->elements.size(); i++) {
                // Key.
                OScriptCodeGenerator::Address element;
                switch (dn->style) {
                    case OScriptParser::DictionaryNode::PYTHON_DICT: {
                        // Python-style: key is any expression.
                        element = parse_expression(p_context, r_error, dn->elements[i].key);
                        if (r_error) {
                            return OScriptCodeGenerator::Address();
                        }
                        break;
                    }
                    case OScriptParser::DictionaryNode::LUA_TABLE: {
                        // Lua-style: key is an identifier interpreted as StringName.
                        StringName key = dn->elements[i].key->reduced_value.operator StringName();
                        element = p_context.add_constant(key);
                        break;
                    }
                }

                elements.push_back(element);

                element = parse_expression(p_context, r_error, dn->elements[i].value);
                if (r_error) {
                    return OScriptCodeGenerator::Address();
                }

                elements.push_back(element);
            }

            if (dict_type.has_container_element_types()) {
                generator->write_construct_typed_dictionary(result, dict_type.get_container_element_type_or_variant(0), dict_type.get_container_element_type_or_variant(1), elements);
            } else {
                generator->write_construct_dictionary(result, elements);
            }

            for (int i = 0; i < elements.size(); i++) {
                if (elements[i].mode == OScriptCodeGenerator::Address::TEMPORARY) {
                    generator->pop_temporary();
                }
            }

            return result;
        }
        case OScriptParser::Node::CAST: {
            const OScriptParser::CastNode *cn = static_cast<const OScriptParser::CastNode *>(p_expression);
            OScriptDataType cast_type = resolve_type(cn->get_datatype(), p_context.script, false);

            OScriptCodeGenerator::Address result;
            if (cast_type.has_type()) {
                // Create temporary for result first since it will be deleted last.
                result = p_context.add_temporary(cast_type);

                OScriptCodeGenerator::Address src = parse_expression(p_context, r_error, cn->operand);

                generator->write_cast(result, src, cast_type);

                if (src.mode == OScriptCodeGenerator::Address::TEMPORARY) {
                    generator->pop_temporary();
                }
            } else {
                result = parse_expression(p_context, r_error, cn->operand);
            }

            return result;
        }
        case OScriptParser::Node::CALL: {
			const OScriptParser::CallNode *call = static_cast<const OScriptParser::CallNode *>(p_expression);
			bool is_awaited = p_expression == awaited_node;
			OScriptDataType type = resolve_type(call->get_datatype(), p_context.script);
			OScriptCodeGenerator::Address result;
			if (p_root) {
				result = OScriptCodeGenerator::Address(OScriptCodeGenerator::Address::NIL);
			} else {
				result = p_context.add_temporary(type);
			}

			Vector<OScriptCodeGenerator::Address> arguments;
			for (int i = 0; i < call->arguments.size(); i++) {
				OScriptCodeGenerator::Address arg = parse_expression(p_context, r_error, call->arguments[i]);
				if (r_error) {
					return OScriptCodeGenerator::Address();
				}
				arguments.push_back(arg);
			}

			if (!call->is_super && call->callee->type == OScriptParser::Node::IDENTIFIER && OScriptParser::get_builtin_type(call->function_name) < Variant::VARIANT_MAX) {
				generator->write_construct(result, OScriptParser::get_builtin_type(call->function_name), arguments);
			} else if (!call->is_super && call->callee->type == OScriptParser::Node::IDENTIFIER && has_utility_function(call->function_name)) {
				// Variant utility function.
				generator->write_call_utility(result, call->function_name, arguments);
			} else if (!call->is_super && call->callee->type == OScriptParser::Node::IDENTIFIER && OScriptUtilityFunctions::function_exists(call->function_name)) {
				// GDScript utility function.
				generator->write_call_oscript_utility(result, call->function_name, arguments);
			} else {
				// Regular function.
				const OScriptParser::ExpressionNode *callee = call->callee;

				if (call->is_super) {
					// Super call.
					generator->write_super_call(result, call->function_name, arguments);
				} else {
					if (callee->type == OScriptParser::Node::IDENTIFIER) {
						// Self function call.
						if (ClassDB::class_has_method(p_context.script->native->get_name(), call->function_name)) {
							// Native method, use faster path.
							OScriptCodeGenerator::Address self;
							self.mode = OScriptCodeGenerator::Address::SELF;
						    #ifdef ALLOW_VALIDATED_METHOD_CALL
						    MethodBind* method = ClassDB::get_method(p_context.script->native->get_name(), call->function_name);
							if (can_use_validate_call(method, arguments)) {
								// Exact arguments, use validated call.
								generator->write_call_method_bind_validated(result, self, method, arguments);
							} else {
								// Not exact arguments, but still can use method bind call.
								generator->write_call_method_bind(result, self, method, arguments);
							}
						    #else
						    generator->write_call(result, self, call->function_name, arguments);
						    #endif
						} else if (call->is_static || p_context.is_static || (p_context.function_node && p_context.function_node->is_static) || call->function_name == StringName("new")) {
							OScriptCodeGenerator::Address self;
							self.mode = OScriptCodeGenerator::Address::CLASS;
							if (is_awaited) {
								generator->write_call_async(result, self, call->function_name, arguments);
							} else {
								generator->write_call(result, self, call->function_name, arguments);
							}
						} else {
							if (is_awaited) {
								generator->write_call_self_async(result, call->function_name, arguments);
							} else {
								generator->write_call_self(result, call->function_name, arguments);
							}
						}
					} else if (callee->type == OScriptParser::Node::SUBSCRIPT) {
						const OScriptParser::SubscriptNode *subscript = static_cast<const OScriptParser::SubscriptNode *>(call->callee);

						if (subscript->is_attribute) {
							// May be static built-in method call.
							if (!call->is_super && subscript->base->type == OScriptParser::Node::IDENTIFIER && OScriptParser::get_builtin_type(static_cast<OScriptParser::IdentifierNode *>(subscript->base)->name) < Variant::VARIANT_MAX) {
								generator->write_call_builtin_type_static(result, OScriptParser::get_builtin_type(static_cast<OScriptParser::IdentifierNode *>(subscript->base)->name), subscript->attribute->name, arguments);
							} else if (!call->is_super && subscript->base->type == OScriptParser::Node::IDENTIFIER && call->function_name != StringName("new") &&
									static_cast<OScriptParser::IdentifierNode *>(subscript->base)->source == OScriptParser::IdentifierNode::NATIVE_CLASS && !Engine::get_singleton()->has_singleton(static_cast<OScriptParser::IdentifierNode *>(subscript->base)->name)) {
								// It's a static native method call.
							    StringName class_name = static_cast<OScriptParser::IdentifierNode *>(subscript->base)->name;
							    #ifdef ALLOW_VALIDATED_METHOD_CALL
								MethodBind *method = ClassDB::get_method(class_name, subscript->attribute->name);
								if (can_use_validate_call(method, arguments)) {
									// Exact arguments, use validated call.
									generator->write_call_native_static_validated(result, method, arguments);
								} else {
									// Not exact arguments, use regular static call
									generator->write_call_native_static(result, class_name, subscript->attribute->name, arguments);
								}
							    #else
							    generator->write_call_native_static(result, class_name, subscript->attribute->name, arguments);
							    #endif
							} else {
								OScriptCodeGenerator::Address base = parse_expression(p_context, r_error, subscript->base);
								if (r_error) {
									return OScriptCodeGenerator::Address();
								}
								if (is_awaited) {
									generator->write_call_async(result, base, call->function_name, arguments);
								} else if (base.type.kind != OScriptDataType::VARIANT && base.type.kind != OScriptDataType::BUILTIN) {
									// Native method, use faster path.
									StringName class_name;
									if (base.type.kind == OScriptDataType::NATIVE) {
										class_name = base.type.native_type;
									} else {
										class_name = base.type.native_type == StringName() ? base.type.script_type->get_instance_base_type() : base.type.native_type;
									}
								    #ifdef ALLOW_VALIDATED_METHOD_CALL
									if (OScriptAnalyzer::class_exists(class_name) && ClassDB::class_has_method(class_name, call->function_name)) {
										MethodBind *method = ClassDB::get_method(class_name, call->function_name);
										if (can_use_validate_call(method, arguments)) {
											// Exact arguments, use validated call.
											generator->write_call_method_bind_validated(result, base, method, arguments);
										} else {
											// Not exact arguments, but still can use method bind call.
											generator->write_call_method_bind(result, base, method, arguments);
										}
									} else {
										generator->write_call(result, base, call->function_name, arguments);
									}
								    #else
								    generator->write_call(result, base, call->function_name, arguments);
								    #endif
								} else if (base.type.kind == OScriptDataType::BUILTIN) {
									generator->write_call_builtin_type(result, base, base.type.builtin_type, call->function_name, arguments);
								} else {
									generator->write_call(result, base, call->function_name, arguments);
								}
								if (base.mode == OScriptCodeGenerator::Address::TEMPORARY) {
									generator->pop_temporary();
								}
							}
						} else {
							set_error("Cannot call something that isn't a function.", call->callee);
							r_error = ERR_COMPILATION_FAILED;
							return OScriptCodeGenerator::Address();
						}
					} else {
						set_error("Compiler bug (please report): incorrect callee type in call node.", call->callee);
						r_error = ERR_COMPILATION_FAILED;
						return OScriptCodeGenerator::Address();
					}
				}
			}

			for (int i = 0; i < arguments.size(); i++) {
				if (arguments[i].mode == OScriptCodeGenerator::Address::TEMPORARY) {
					generator->pop_temporary();
				}
			}
			return result;
		}
        case OScriptParser::Node::GET_NODE: {
			const OScriptParser::GetNodeNode *get_node = static_cast<const OScriptParser::GetNodeNode *>(p_expression);

			Vector<OScriptCodeGenerator::Address> args;
			args.push_back(p_context.add_constant(NodePath(get_node->full_path)));

			OScriptCodeGenerator::Address result = p_context.add_temporary(resolve_type(get_node->get_datatype(), p_context.script));

			MethodBind *get_node_method = ClassDB::get_method("Node", "get_node");
			generator->write_call_method_bind_validated(result, OScriptCodeGenerator::Address(OScriptCodeGenerator::Address::SELF), get_node_method, args);

			return result;
		}
		case OScriptParser::Node::PRELOAD: {
			const OScriptParser::PreloadNode *preload = static_cast<const OScriptParser::PreloadNode *>(p_expression);

			// Add resource as constant.
			return p_context.add_constant(preload->resource);
		}
		case OScriptParser::Node::AWAIT: {
			const OScriptParser::AwaitNode *await = static_cast<const OScriptParser::AwaitNode *>(p_expression);

			OScriptCodeGenerator::Address result = p_context.add_temporary(resolve_type(p_expression->get_datatype(), p_context.script));
			OScriptParser::ExpressionNode *previous_awaited_node = awaited_node;
			awaited_node = await->to_await;
			OScriptCodeGenerator::Address argument = parse_expression(p_context, r_error, await->to_await);
			awaited_node = previous_awaited_node;
			if (r_error) {
				return OScriptCodeGenerator::Address();
			}

			generator->write_await(result, argument);

			if (argument.mode == OScriptCodeGenerator::Address::TEMPORARY) {
				generator->pop_temporary();
			}

			return result;
		}
		// Indexing operator.
		case OScriptParser::Node::SUBSCRIPT: {
			const OScriptParser::SubscriptNode *subscript = static_cast<const OScriptParser::SubscriptNode *>(p_expression);
			OScriptCodeGenerator::Address result = p_context.add_temporary(resolve_type(subscript->get_datatype(), p_context.script));

			OScriptCodeGenerator::Address base = parse_expression(p_context, r_error, subscript->base);
			if (r_error) {
				return OScriptCodeGenerator::Address();
			}

			bool named = subscript->is_attribute;
			StringName name;
			OScriptCodeGenerator::Address index;
			if (subscript->is_attribute) {
				if (subscript->base->type == OScriptParser::Node::SELF && p_context.script) {
					OScriptParser::IdentifierNode *identifier = subscript->attribute;
					HashMap<StringName, OScript::MemberInfo>::Iterator MI = p_context.script->member_indices.find(identifier->name);

                    #ifdef DEBUG_ENABLED
					if (MI && MI->value.getter == p_context.function_name) {
						String n = identifier->name;
						set_error("Must use '" + n + "' instead of 'self." + n + "' in getter.", identifier);
						r_error = ERR_COMPILATION_FAILED;
						return OScriptCodeGenerator::Address();
					}
                    #endif

					if (MI && MI->value.getter == StringName("")) {
						// Remove result temp as we don't need it.
						generator->pop_temporary();
						// Faster than indexing self (as if no self. had been used).
						return OScriptCodeGenerator::Address(OScriptCodeGenerator::Address::MEMBER, MI->value.index, resolve_type(subscript->get_datatype(), p_context.script));
					}
				}

				name = subscript->attribute->name;
				named = true;
			} else {
				if (subscript->index->is_constant && subscript->index->reduced_value.get_type() == Variant::STRING_NAME) {
					// Also, somehow, named (speed up anyway).
					name = subscript->index->reduced_value;
					named = true;
				} else {
					// Regular indexing.
					index = parse_expression(p_context, r_error, subscript->index);
					if (r_error) {
						return OScriptCodeGenerator::Address();
					}
				}
			}

			if (named) {
				generator->write_get_named(result, name, base);
			} else {
				generator->write_get(result, index, base);
			}

			if (index.mode == OScriptCodeGenerator::Address::TEMPORARY) {
				generator->pop_temporary();
			}
			if (base.mode == OScriptCodeGenerator::Address::TEMPORARY) {
				generator->pop_temporary();
			}

			return result;
		}
		case OScriptParser::Node::UNARY_OPERATOR: {
			const OScriptParser::UnaryOpNode *unary = static_cast<const OScriptParser::UnaryOpNode *>(p_expression);

			OScriptCodeGenerator::Address result = p_context.add_temporary(resolve_type(unary->get_datatype(), p_context.script));

			OScriptCodeGenerator::Address operand = parse_expression(p_context, r_error, unary->operand);
			if (r_error) {
				return OScriptCodeGenerator::Address();
			}

			generator->write_unary_operator(result, unary->variant_op, operand);

			if (operand.mode == OScriptCodeGenerator::Address::TEMPORARY) {
				generator->pop_temporary();
			}

			return result;
		}
		case OScriptParser::Node::BINARY_OPERATOR: {
			const OScriptParser::BinaryOpNode *binary = static_cast<const OScriptParser::BinaryOpNode *>(p_expression);

			OScriptCodeGenerator::Address result = p_context.add_temporary(resolve_type(binary->get_datatype(), p_context.script));

			switch (binary->operation) {
				case OScriptParser::BinaryOpNode::OP_LOGIC_AND: {
					// AND operator with early out on failure.
					OScriptCodeGenerator::Address left_operand = parse_expression(p_context, r_error, binary->left_operand);
					generator->write_and_left_operand(left_operand);
					OScriptCodeGenerator::Address right_operand = parse_expression(p_context, r_error, binary->right_operand);
					generator->write_and_right_operand(right_operand);

					generator->write_end_and(result);

					if (right_operand.mode == OScriptCodeGenerator::Address::TEMPORARY) {
						generator->pop_temporary();
					}
					if (left_operand.mode == OScriptCodeGenerator::Address::TEMPORARY) {
						generator->pop_temporary();
					}
				    break;
				}
				case OScriptParser::BinaryOpNode::OP_LOGIC_OR: {
					// OR operator with early out on success.
					OScriptCodeGenerator::Address left_operand = parse_expression(p_context, r_error, binary->left_operand);
					generator->write_or_left_operand(left_operand);
					OScriptCodeGenerator::Address right_operand = parse_expression(p_context, r_error, binary->right_operand);
					generator->write_or_right_operand(right_operand);

					generator->write_end_or(result);

					if (right_operand.mode == OScriptCodeGenerator::Address::TEMPORARY) {
						generator->pop_temporary();
					}
					if (left_operand.mode == OScriptCodeGenerator::Address::TEMPORARY) {
						generator->pop_temporary();
					}
				    break;
				}
				default: {
					OScriptCodeGenerator::Address left_operand = parse_expression(p_context, r_error, binary->left_operand);
					OScriptCodeGenerator::Address right_operand = parse_expression(p_context, r_error, binary->right_operand);

					generator->write_binary_operator(result, binary->variant_op, left_operand, right_operand);

					if (right_operand.mode == OScriptCodeGenerator::Address::TEMPORARY) {
						generator->pop_temporary();
					}
					if (left_operand.mode == OScriptCodeGenerator::Address::TEMPORARY) {
						generator->pop_temporary();
					}
				    break;
				}
			}
			return result;
		}
		case OScriptParser::Node::TERNARY_OPERATOR: {
			// x IF a ELSE y operator with early out on failure.
			const OScriptParser::TernaryOpNode *ternary = static_cast<const OScriptParser::TernaryOpNode *>(p_expression);
			OScriptCodeGenerator::Address result = p_context.add_temporary(resolve_type(ternary->get_datatype(), p_context.script));

			generator->write_start_ternary(result);

			OScriptCodeGenerator::Address condition = parse_expression(p_context, r_error, ternary->condition);
			if (r_error) {
				return OScriptCodeGenerator::Address();
			}
			generator->write_ternary_condition(condition);

			if (condition.mode == OScriptCodeGenerator::Address::TEMPORARY) {
				generator->pop_temporary();
			}

			OScriptCodeGenerator::Address true_expr = parse_expression(p_context, r_error, ternary->true_expr);
			if (r_error) {
				return OScriptCodeGenerator::Address();
			}
			generator->write_ternary_true_expr(true_expr);
			if (true_expr.mode == OScriptCodeGenerator::Address::TEMPORARY) {
				generator->pop_temporary();
			}

			OScriptCodeGenerator::Address false_expr = parse_expression(p_context, r_error, ternary->false_expr);
			if (r_error) {
				return OScriptCodeGenerator::Address();
			}
			generator->write_ternary_false_expr(false_expr);
			if (false_expr.mode == OScriptCodeGenerator::Address::TEMPORARY) {
				generator->pop_temporary();
			}

			generator->write_end_ternary();

			return result;
		}
		case OScriptParser::Node::TYPE_TEST: {
			const OScriptParser::TypeTestNode *type_test = static_cast<const OScriptParser::TypeTestNode *>(p_expression);
			OScriptCodeGenerator::Address result = p_context.add_temporary(resolve_type(type_test->get_datatype(), p_context.script));

			OScriptCodeGenerator::Address operand = parse_expression(p_context, r_error, type_test->operand);
			OScriptDataType test_type = resolve_type(type_test->test_datatype, p_context.script, false);
			if (r_error) {
				return OScriptCodeGenerator::Address();
			}

			if (test_type.has_type()) {
				generator->write_type_test(result, operand, test_type);
			} else {
				generator->write_assign_true(result);
			}

			if (operand.mode == OScriptCodeGenerator::Address::TEMPORARY) {
				generator->pop_temporary();
			}

			return result;
		}
		case OScriptParser::Node::ASSIGNMENT: {
			const OScriptParser::AssignmentNode *assignment = static_cast<const OScriptParser::AssignmentNode *>(p_expression);

			if (assignment->assignee->type == OScriptParser::Node::SUBSCRIPT) {
				// SET (chained) MODE!
				const OScriptParser::SubscriptNode *subscript = static_cast<OScriptParser::SubscriptNode *>(assignment->assignee);
                #ifdef DEBUG_ENABLED
				if (subscript->is_attribute && subscript->base->type == OScriptParser::Node::SELF && p_context.script) {
					HashMap<StringName, OScript::MemberInfo>::Iterator MI = p_context.script->member_indices.find(subscript->attribute->name);
					if (MI && MI->value.setter == p_context.function_name) {
						String n = subscript->attribute->name;
						set_error("Must use '" + n + "' instead of 'self." + n + "' in setter.", subscript);
						r_error = ERR_COMPILATION_FAILED;
						return OScriptCodeGenerator::Address();
					}
				}
                #endif
				/* Find chain of sets */

				StringName assign_class_member_property;

				OScriptCodeGenerator::Address target_member_property;
				bool is_member_property = false;
				bool member_property_has_setter = false;
				bool member_property_is_in_setter = false;
				bool is_static = false;
				OScriptCodeGenerator::Address static_var_class;
				int static_var_index = 0;
				OScriptDataType static_var_data_type;
				StringName var_name;
				StringName member_property_setter_function;

				List<const OScriptParser::SubscriptNode *> chain;

				{
					// Create get/set chain.
					const OScriptParser::SubscriptNode *n = subscript;
					while (true) {
						chain.push_back(n);
						if (n->base->type != OScriptParser::Node::SUBSCRIPT) {
							// Check for a property.
							if (n->base->type == OScriptParser::Node::IDENTIFIER) {
								OScriptParser::IdentifierNode *identifier = static_cast<OScriptParser::IdentifierNode *>(n->base);
								var_name = identifier->name;
								if (is_class_member_property(p_context, var_name)) {
									assign_class_member_property = var_name;
								} else if (!is_local_or_parameter(p_context, var_name)) {
									if (p_context.script->member_indices.has(var_name)) {
										is_member_property = true;
										is_static = false;
										const OScript::MemberInfo &minfo = p_context.script->member_indices[var_name];
										member_property_setter_function = minfo.setter;
										member_property_has_setter = member_property_setter_function != StringName();
										member_property_is_in_setter = member_property_has_setter && member_property_setter_function == p_context.function_name;
										target_member_property.mode = OScriptCodeGenerator::Address::MEMBER;
										target_member_property.address = minfo.index;
										target_member_property.type = minfo.data_type;
									} else {
										// Try static variables.
										OScript *scr = p_context.script;
										while (scr) {
											if (scr->static_variables_indices.has(var_name)) {
												is_member_property = true;
												is_static = true;
												const OScript::MemberInfo &minfo = scr->static_variables_indices[var_name];
												member_property_setter_function = minfo.setter;
												member_property_has_setter = member_property_setter_function != StringName();
												member_property_is_in_setter = member_property_has_setter && member_property_setter_function == p_context.function_name;
												static_var_class = p_context.add_constant(scr);
												static_var_index = minfo.index;
												static_var_data_type = minfo.data_type;
												break;
											}
											scr = scr->base.ptr();
										}
									}
								}
							}
							break;
						}
						n = static_cast<const OScriptParser::SubscriptNode *>(n->base);
					}
				}

				/* Chain of gets */

				// Get at (potential) root stack pos, so it can be returned.
				OScriptCodeGenerator::Address base = parse_expression(p_context, r_error, chain.back()->get()->base);
				const bool base_known_type = base.type.has_type();
				const bool base_is_shared = GDE::Variant::is_type_shared(base.type.builtin_type);

				if (r_error) {
					return OScriptCodeGenerator::Address();
				}

				OScriptCodeGenerator::Address prev_base = base;

				// In case the base has a setter, don't use the address directly, as we want to call that setter.
				// So use a temp value instead and call the setter at the end.
				OScriptCodeGenerator::Address base_temp;
				if ((!base_known_type || !base_is_shared) && base.mode == OScriptCodeGenerator::Address::MEMBER && member_property_has_setter && !member_property_is_in_setter) {
					base_temp = p_context.add_temporary(base.type);
					generator->write_assign(base_temp, base);
					prev_base = base_temp;
				}

				struct ChainInfo {
					bool is_named = false;
					OScriptCodeGenerator::Address base;
					OScriptCodeGenerator::Address key;
					StringName name;
				};

				List<ChainInfo> set_chain;

				for (List<const OScriptParser::SubscriptNode *>::Element *E = chain.back(); E; E = E->prev()) {
					if (E == chain.front()) {
						// Skip the main subscript, since we'll assign to that.
						break;
					}
					const OScriptParser::SubscriptNode *subscript_elem = E->get();
					OScriptCodeGenerator::Address value = p_context.add_temporary(resolve_type(subscript_elem->get_datatype(), p_context.script));
					OScriptCodeGenerator::Address key;
					StringName name;

					if (subscript_elem->is_attribute) {
						name = subscript_elem->attribute->name;
						generator->write_get_named(value, name, prev_base);
					} else {
						key = parse_expression(p_context, r_error, subscript_elem->index);
						if (r_error) {
							return OScriptCodeGenerator::Address();
						}
						generator->write_get(value, key, prev_base);
					}

					// Store base and key for setting it back later.
					set_chain.push_front({ subscript_elem->is_attribute, prev_base, key, name }); // Push to front to invert the list.
					prev_base = value;
				}

				// Get value to assign.
				OScriptCodeGenerator::Address assigned = parse_expression(p_context, r_error, assignment->assigned_value);
				if (r_error) {
					return OScriptCodeGenerator::Address();
				}
				// Get the key if needed.
				OScriptCodeGenerator::Address key;
				StringName name;
				if (subscript->is_attribute) {
					name = subscript->attribute->name;
				} else {
					key = parse_expression(p_context, r_error, subscript->index);
					if (r_error) {
						return OScriptCodeGenerator::Address();
					}
				}

				// Perform operator if any.
				if (assignment->operation != OScriptParser::AssignmentNode::OP_NONE) {
					OScriptCodeGenerator::Address op_result = p_context.add_temporary(resolve_type(assignment->get_datatype(), p_context.script));
					OScriptCodeGenerator::Address value = p_context.add_temporary(resolve_type(subscript->get_datatype(), p_context.script));
					if (subscript->is_attribute) {
						generator->write_get_named(value, name, prev_base);
					} else {
						generator->write_get(value, key, prev_base);
					}
					generator->write_binary_operator(op_result, assignment->variant_op, value, assigned);
					generator->pop_temporary();
					if (assigned.mode == OScriptCodeGenerator::Address::TEMPORARY) {
						generator->pop_temporary();
					}
					assigned = op_result;
				}

				// Perform assignment.
				if (subscript->is_attribute) {
					generator->write_set_named(prev_base, name, assigned);
				} else {
					generator->write_set(prev_base, key, assigned);
				}
				if (key.mode == OScriptCodeGenerator::Address::TEMPORARY) {
					generator->pop_temporary();
				}
				if (assigned.mode == OScriptCodeGenerator::Address::TEMPORARY) {
					generator->pop_temporary();
				}

				assigned = prev_base;

				// Set back the values into their bases.
				for (const ChainInfo &info : set_chain) {
					bool known_type = assigned.type.has_type();
					bool is_shared = GDE::Variant::is_type_shared(assigned.type.builtin_type);

					if (!known_type || !is_shared) {
						if (!known_type) {
							// Jump shared values since they are already updated in-place.
							generator->write_jump_if_shared(assigned);
						}
						if (!info.is_named) {
							generator->write_set(info.base, info.key, assigned);
						} else {
							generator->write_set_named(info.base, info.name, assigned);
						}
						if (!known_type) {
							generator->write_end_jump_if_shared();
						}
					}
					if (!info.is_named && info.key.mode == OScriptCodeGenerator::Address::TEMPORARY) {
						generator->pop_temporary();
					}
					if (assigned.mode == OScriptCodeGenerator::Address::TEMPORARY) {
						generator->pop_temporary();
					}
					assigned = info.base;
				}

				bool known_type = assigned.type.has_type();
				bool is_shared = GDE::Variant::is_type_shared(assigned.type.builtin_type);

				if (!known_type || !is_shared) {
					// If this is a class member property, also assign to it.
					// This allow things like: position.x += 2.0
					if (assign_class_member_property != StringName()) {
						if (!known_type) {
							generator->write_jump_if_shared(assigned);
						}
						generator->write_set_member(assigned, assign_class_member_property);
						if (!known_type) {
							generator->write_end_jump_if_shared();
						}
					} else if (is_member_property) {
						// Same as above but for script members.
						if (!known_type) {
							generator->write_jump_if_shared(assigned);
						}
						if (member_property_has_setter && !member_property_is_in_setter) {
							Vector<OScriptCodeGenerator::Address> args;
							args.push_back(assigned);
							OScriptCodeGenerator::Address call_base = is_static ? OScriptCodeGenerator::Address(OScriptCodeGenerator::Address::CLASS) : OScriptCodeGenerator::Address(OScriptCodeGenerator::Address::SELF);
							generator->write_call(OScriptCodeGenerator::Address(), call_base, member_property_setter_function, args);
						} else if (is_static) {
							OScriptCodeGenerator::Address temp = p_context.add_temporary(static_var_data_type);
							generator->write_assign(temp, assigned);
							generator->write_set_static_variable(temp, static_var_class, static_var_index);
							generator->pop_temporary();
						} else {
							generator->write_assign(target_member_property, assigned);
						}
						if (!known_type) {
							generator->write_end_jump_if_shared();
						}
					}
				} else if (base_temp.mode == OScriptCodeGenerator::Address::TEMPORARY) {
					if (!base_known_type) {
						generator->write_jump_if_shared(base);
					}
					// Save the temp value back to the base by calling its setter.
					generator->write_call(OScriptCodeGenerator::Address(), base, member_property_setter_function, { assigned });
					if (!base_known_type) {
						generator->write_end_jump_if_shared();
					}
				}

				if (assigned.mode == OScriptCodeGenerator::Address::TEMPORARY) {
					generator->pop_temporary();
				}
			} else if (assignment->assignee->type == OScriptParser::Node::IDENTIFIER && is_class_member_property(p_context, static_cast<OScriptParser::IdentifierNode *>(assignment->assignee)->name)) {
				// Assignment to member property.
				OScriptCodeGenerator::Address assigned_value = parse_expression(p_context, r_error, assignment->assigned_value);
				if (r_error) {
					return OScriptCodeGenerator::Address();
				}

				OScriptCodeGenerator::Address to_assign = assigned_value;
				bool has_operation = assignment->operation != OScriptParser::AssignmentNode::OP_NONE;

				StringName name = static_cast<OScriptParser::IdentifierNode *>(assignment->assignee)->name;

				if (has_operation) {
					OScriptCodeGenerator::Address op_result = p_context.add_temporary(resolve_type(assignment->get_datatype(), p_context.script));
					OScriptCodeGenerator::Address member = p_context.add_temporary(resolve_type(assignment->assignee->get_datatype(), p_context.script));
					generator->write_get_member(member, name);
					generator->write_binary_operator(op_result, assignment->variant_op, member, assigned_value);
					generator->pop_temporary(); // Pop member temp.
					to_assign = op_result;
				}

				generator->write_set_member(to_assign, name);

				if (to_assign.mode == OScriptCodeGenerator::Address::TEMPORARY) {
					generator->pop_temporary(); // Pop the assigned expression or the temp result if it has operation.
				}
				if (has_operation && assigned_value.mode == OScriptCodeGenerator::Address::TEMPORARY) {
					generator->pop_temporary(); // Pop the assigned expression if not done before.
				}
			} else {
				// Regular assignment.
				if (assignment->assignee->type != OScriptParser::Node::IDENTIFIER) {
					set_error("Compiler bug (please report): Expected the assignee to be an identifier here.", assignment->assignee);
					r_error = ERR_COMPILATION_FAILED;
					return OScriptCodeGenerator::Address();
				}
				OScriptCodeGenerator::Address member;
				bool is_member = false;
				bool has_setter = false;
				bool is_in_setter = false;
				bool is_static = false;
				OScriptCodeGenerator::Address static_var_class;
				int static_var_index = 0;
				OScriptDataType static_var_data_type;
				StringName var_name;
				StringName setter_function;
				var_name = static_cast<const OScriptParser::IdentifierNode *>(assignment->assignee)->name;
				if (!is_local_or_parameter(p_context, var_name)) {
					if (p_context.script->member_indices.has(var_name)) {
						is_member = true;
						is_static = false;
						OScript::MemberInfo &minfo = p_context.script->member_indices[var_name];
						setter_function = minfo.setter;
						has_setter = setter_function != StringName();
						is_in_setter = has_setter && setter_function == p_context.function_name;
						member.mode = OScriptCodeGenerator::Address::MEMBER;
						member.address = minfo.index;
						member.type = minfo.data_type;
					} else {
						// Try static variables.
						OScript *scr = p_context.script;
						while (scr) {
							if (scr->static_variables_indices.has(var_name)) {
								is_member = true;
								is_static = true;
								OScript::MemberInfo &minfo = scr->static_variables_indices[var_name];
								setter_function = minfo.setter;
								has_setter = setter_function != StringName();
								is_in_setter = has_setter && setter_function == p_context.function_name;
								static_var_class = p_context.add_constant(scr);
								static_var_index = minfo.index;
								static_var_data_type = minfo.data_type;
								break;
							}
							scr = scr->base.ptr();
						}
					}
				}

				OScriptCodeGenerator::Address target;
				if (is_member) {
					target = member; // parse_expression could call its getter, but we want to know the actual address
				} else {
					target = parse_expression(p_context, r_error, assignment->assignee);
					if (r_error) {
						return OScriptCodeGenerator::Address();
					}
				}

				OScriptCodeGenerator::Address assigned_value = parse_expression(p_context, r_error, assignment->assigned_value);
				if (r_error) {
					return OScriptCodeGenerator::Address();
				}

				OScriptCodeGenerator::Address to_assign;
				bool has_operation = assignment->operation != OScriptParser::AssignmentNode::OP_NONE;
				if (has_operation) {
					// Perform operation.
					OScriptCodeGenerator::Address op_result = p_context.add_temporary(resolve_type(assignment->get_datatype(), p_context.script));
					OScriptCodeGenerator::Address og_value = parse_expression(p_context, r_error, assignment->assignee);
					generator->write_binary_operator(op_result, assignment->variant_op, og_value, assigned_value);
					to_assign = op_result;

					if (og_value.mode == OScriptCodeGenerator::Address::TEMPORARY) {
						generator->pop_temporary();
					}
				} else {
					to_assign = assigned_value;
				}

				if (has_setter && !is_in_setter) {
					// Call setter.
					Vector<OScriptCodeGenerator::Address> args;
					args.push_back(to_assign);
					OScriptCodeGenerator::Address call_base = is_static ? OScriptCodeGenerator::Address(OScriptCodeGenerator::Address::CLASS) : OScriptCodeGenerator::Address(OScriptCodeGenerator::Address::SELF);
					generator->write_call(OScriptCodeGenerator::Address(), call_base, setter_function, args);
				} else if (is_static) {
					OScriptCodeGenerator::Address temp = p_context.add_temporary(static_var_data_type);
					if (assignment->use_conversion_assign) {
						generator->write_assign_with_conversion(temp, to_assign);
					} else {
						generator->write_assign(temp, to_assign);
					}
					generator->write_set_static_variable(temp, static_var_class, static_var_index);
					generator->pop_temporary();
				} else {
					// Just assign.
					if (assignment->use_conversion_assign) {
						generator->write_assign_with_conversion(target, to_assign);
					} else {
						generator->write_assign(target, to_assign);
					}
				}

				if (to_assign.mode == OScriptCodeGenerator::Address::TEMPORARY) {
					generator->pop_temporary(); // Pop assigned value or temp operation result.
				}
				if (has_operation && assigned_value.mode == OScriptCodeGenerator::Address::TEMPORARY) {
					generator->pop_temporary(); // Pop assigned value if not done before.
				}
				if (target.mode == OScriptCodeGenerator::Address::TEMPORARY) {
					generator->pop_temporary(); // Pop the target to assignment.
				}
			}
			return OScriptCodeGenerator::Address(); // Assignment does not return a value.
		}
        case OScriptParser::Node::LAMBDA: {
            const OScriptParser::LambdaNode *lambda = static_cast<const OScriptParser::LambdaNode *>(p_expression);
            OScriptCodeGenerator::Address result = p_context.add_temporary(resolve_type(lambda->get_datatype(), p_context.script));

            Vector<OScriptCodeGenerator::Address> captures;
            captures.resize(lambda->captures.size());
            for (int i = 0; i < lambda->captures.size(); i++) {
                captures.write[i] = parse_expression(p_context, r_error, lambda->captures[i]);
                if (r_error) {
                    return OScriptCodeGenerator::Address();
                }
            }

            OScriptCompiledFunction *function = parse_function(r_error, p_context.script, p_context.class_node, lambda->function, false, true);
            if (r_error) {
                return OScriptCodeGenerator::Address();
            }

            p_context.script->lambda_info.insert(function, { (int)lambda->captures.size(), lambda->use_self });
            generator->write_lambda(result, function, captures, lambda->use_self);

            for (int i = 0; i < captures.size(); i++) {
                if (captures[i].mode == OScriptCodeGenerator::Address::TEMPORARY) {
                    generator->pop_temporary();
                }
            }

            return result;
        }
        default: {
            set_error("Compiler bug (please report): Unexpected node in parse tree while parsing expression.", p_expression); // Unreachable code.
            r_error = ERR_COMPILATION_FAILED;
            return OScriptCodeGenerator::Address();
        }
    }
}

OScriptCodeGenerator::Address OScriptCompiler::parse_match_pattern(CompilerContext& p_context, Error& r_error, const OScriptParser::PatternNode* p_pattern, const OScriptCodeGenerator::Address& p_value_addr, const OScriptCodeGenerator::Address& p_type_addr, const OScriptCodeGenerator::Address& p_prev_test, bool p_is_first, bool p_is_nested) {
    switch (p_pattern->pattern_type) {
        case OScriptParser::PatternNode::PT_LITERAL: {
            if (p_is_nested) {
                p_context.generator->write_and_left_operand(p_prev_test);
            } else if (!p_is_first) {
                p_context.generator->write_or_left_operand(p_prev_test);
            }

            // Get literal type into constant map.
            Variant::Type literal_type = p_pattern->literal->value.get_type();
            OScriptCodeGenerator::Address literal_type_addr = p_context.add_constant(literal_type);

            // Equality is always a boolean.
            OScriptDataType equality_type;
            equality_type.kind = OScriptDataType::BUILTIN;
            equality_type.builtin_type = Variant::BOOL;

            // Check type equality.
            OScriptCodeGenerator::Address type_equality_addr = p_context.add_temporary(equality_type);
            p_context.generator->write_binary_operator(type_equality_addr, Variant::OP_EQUAL, p_type_addr, literal_type_addr);

            if (literal_type == Variant::STRING) {
                OScriptCodeGenerator::Address type_stringname_addr = p_context.add_constant(Variant::STRING_NAME);

                // Check StringName <-> String type equality.
                OScriptCodeGenerator::Address tmp_comp_addr = p_context.add_temporary(equality_type);

                p_context.generator->write_binary_operator(tmp_comp_addr, Variant::OP_EQUAL, p_type_addr, type_stringname_addr);
                p_context.generator->write_binary_operator(type_equality_addr, Variant::OP_OR, type_equality_addr, tmp_comp_addr);

                p_context.generator->pop_temporary(); // Remove tmp_comp_addr from stack.
            } else if (literal_type == Variant::STRING_NAME) {
                OScriptCodeGenerator::Address type_string_addr = p_context.add_constant(Variant::STRING);

                // Check String <-> StringName type equality.
                OScriptCodeGenerator::Address tmp_comp_addr = p_context.add_temporary(equality_type);

                p_context.generator->write_binary_operator(tmp_comp_addr, Variant::OP_EQUAL, p_type_addr, type_string_addr);
                p_context.generator->write_binary_operator(type_equality_addr, Variant::OP_OR, type_equality_addr, tmp_comp_addr);

                p_context.generator->pop_temporary(); // Remove tmp_comp_addr from stack.
            }

            p_context.generator->write_and_left_operand(type_equality_addr);

            // Get literal.
            OScriptCodeGenerator::Address literal_addr = parse_expression(p_context, r_error, p_pattern->literal);
            if (r_error) {
                return OScriptCodeGenerator::Address();
            }

            // Check value equality.
            OScriptCodeGenerator::Address equality_addr = p_context.add_temporary(equality_type);
            p_context.generator->write_binary_operator(equality_addr, Variant::OP_EQUAL, p_value_addr, literal_addr);
            p_context.generator->write_and_right_operand(equality_addr);

            // AND both together (reuse temporary location).
            p_context.generator->write_end_and(type_equality_addr);

            p_context.generator->pop_temporary(); // Remove equality_addr from stack.

            if (literal_addr.mode == OScriptCodeGenerator::Address::TEMPORARY) {
                p_context.generator->pop_temporary();
            }

            // If this isn't the first, we need to OR with the previous pattern. If it's nested, we use AND instead.
            if (p_is_nested) {
                // Use the previous value as target, since we only need one temporary variable.
                p_context.generator->write_and_right_operand(type_equality_addr);
                p_context.generator->write_end_and(p_prev_test);
            } else if (!p_is_first) {
                // Use the previous value as target, since we only need one temporary variable.
                p_context.generator->write_or_right_operand(type_equality_addr);
                p_context.generator->write_end_or(p_prev_test);
            } else {
                // Just assign this value to the accumulator temporary.
                p_context.generator->write_assign(p_prev_test, type_equality_addr);
            }
            p_context.generator->pop_temporary(); // Remove type_equality_addr.

            return p_prev_test;
        }
        case OScriptParser::PatternNode::PT_EXPRESSION: {
            if (p_is_nested) {
                p_context.generator->write_and_left_operand(p_prev_test);
            } else if (!p_is_first) {
                p_context.generator->write_or_left_operand(p_prev_test);
            }

            OScriptCodeGenerator::Address type_string_addr = p_context.add_constant(Variant::STRING);
            OScriptCodeGenerator::Address type_stringname_addr = p_context.add_constant(Variant::STRING_NAME);

            // Equality is always a boolean.
            OScriptDataType equality_type;
            equality_type.kind = OScriptDataType::BUILTIN;
            equality_type.builtin_type = Variant::BOOL;

            // Create the result temps first since it's the last to go away.
            OScriptCodeGenerator::Address result_addr = p_context.add_temporary(equality_type);
            OScriptCodeGenerator::Address equality_test_addr = p_context.add_temporary(equality_type);
            OScriptCodeGenerator::Address stringy_comp_addr = p_context.add_temporary(equality_type);
            OScriptCodeGenerator::Address stringy_comp_addr_2 = p_context.add_temporary(equality_type);
            OScriptCodeGenerator::Address expr_type_addr = p_context.add_temporary();

            // Evaluate expression.
            OScriptCodeGenerator::Address expr_addr;
            expr_addr = parse_expression(p_context, r_error, p_pattern->expression);
            if (r_error) {
                return OScriptCodeGenerator::Address();
            }

            // Evaluate expression type.
            Vector<OScriptCodeGenerator::Address> typeof_args;
            typeof_args.push_back(expr_addr);
            p_context.generator->write_call_utility(expr_type_addr, "typeof", typeof_args);

            // Check type equality.
            p_context.generator->write_binary_operator(result_addr, Variant::OP_EQUAL, p_type_addr, expr_type_addr);

            // Check for String <-> StringName comparison.
            p_context.generator->write_binary_operator(stringy_comp_addr, Variant::OP_EQUAL, p_type_addr, type_string_addr);
            p_context.generator->write_binary_operator(stringy_comp_addr_2, Variant::OP_EQUAL, expr_type_addr, type_stringname_addr);
            p_context.generator->write_binary_operator(stringy_comp_addr, Variant::OP_AND, stringy_comp_addr, stringy_comp_addr_2);
            p_context.generator->write_binary_operator(result_addr, Variant::OP_OR, result_addr, stringy_comp_addr);

            // Check for StringName <-> String comparison.
            p_context.generator->write_binary_operator(stringy_comp_addr, Variant::OP_EQUAL, p_type_addr, type_stringname_addr);
            p_context.generator->write_binary_operator(stringy_comp_addr_2, Variant::OP_EQUAL, expr_type_addr, type_string_addr);
            p_context.generator->write_binary_operator(stringy_comp_addr, Variant::OP_AND, stringy_comp_addr, stringy_comp_addr_2);
            p_context.generator->write_binary_operator(result_addr, Variant::OP_OR, result_addr, stringy_comp_addr);

            p_context.generator->pop_temporary(); // Remove expr_type_addr from stack.
            p_context.generator->pop_temporary(); // Remove stringy_comp_addr_2 from stack.
            p_context.generator->pop_temporary(); // Remove stringy_comp_addr from stack.

            p_context.generator->write_and_left_operand(result_addr);

            // Check value equality.
            p_context.generator->write_binary_operator(equality_test_addr, Variant::OP_EQUAL, p_value_addr, expr_addr);
            p_context.generator->write_and_right_operand(equality_test_addr);

            // AND both type and value equality.
            p_context.generator->write_end_and(result_addr);

            // We don't need the expression temporary anymore.
            if (expr_addr.mode == OScriptCodeGenerator::Address::TEMPORARY) {
                p_context.generator->pop_temporary();
            }
            p_context.generator->pop_temporary(); // Remove equality_test_addr from stack.

            // If this isn't the first, we need to OR with the previous pattern. If it's nested, we use AND instead.
            if (p_is_nested) {
                // Use the previous value as target, since we only need one temporary variable.
                p_context.generator->write_and_right_operand(result_addr);
                p_context.generator->write_end_and(p_prev_test);
            } else if (!p_is_first) {
                // Use the previous value as target, since we only need one temporary variable.
                p_context.generator->write_or_right_operand(result_addr);
                p_context.generator->write_end_or(p_prev_test);
            } else {
                // Just assign this value to the accumulator temporary.
                p_context.generator->write_assign(p_prev_test, result_addr);
            }
            p_context.generator->pop_temporary(); // Remove temp result addr.

            return p_prev_test;
        }
        case OScriptParser::PatternNode::PT_ARRAY: {
            if (p_is_nested) {
                p_context.generator->write_and_left_operand(p_prev_test);
            } else if (!p_is_first) {
                p_context.generator->write_or_left_operand(p_prev_test);
            }
            // Get array type into constant map.
            OScriptCodeGenerator::Address array_type_addr = p_context.add_constant((int)Variant::ARRAY);

            // Equality is always a boolean.
            OScriptDataType temp_type;
            temp_type.kind = OScriptDataType::BUILTIN;
            temp_type.builtin_type = Variant::BOOL;

            // Check type equality.
            OScriptCodeGenerator::Address result_addr = p_context.add_temporary(temp_type);
            p_context.generator->write_binary_operator(result_addr, Variant::OP_EQUAL, p_type_addr, array_type_addr);
            p_context.generator->write_and_left_operand(result_addr);

            // Store pattern length in constant map.
            OScriptCodeGenerator::Address array_length_addr = p_context.add_constant(p_pattern->rest_used ? p_pattern->array.size() - 1 : p_pattern->array.size());

            // Get value length.
            temp_type.builtin_type = Variant::INT;
            OScriptCodeGenerator::Address value_length_addr = p_context.add_temporary(temp_type);
            Vector<OScriptCodeGenerator::Address> len_args;
            len_args.push_back(p_value_addr);
            p_context.generator->write_call_oscript_utility(value_length_addr, "len", len_args);

            // Test length compatibility.
            temp_type.builtin_type = Variant::BOOL;
            OScriptCodeGenerator::Address length_compat_addr = p_context.add_temporary(temp_type);
            p_context.generator->write_binary_operator(length_compat_addr, p_pattern->rest_used ? Variant::OP_GREATER_EQUAL : Variant::OP_EQUAL, value_length_addr, array_length_addr);
            p_context.generator->write_and_right_operand(length_compat_addr);

            // AND type and length check.
            p_context.generator->write_end_and(result_addr);

            // Remove length temporaries.
            p_context.generator->pop_temporary();
            p_context.generator->pop_temporary();

            // Create temporaries outside the loop so they can be reused.
            OScriptCodeGenerator::Address element_addr = p_context.add_temporary();
            OScriptCodeGenerator::Address element_type_addr = p_context.add_temporary();

            // Evaluate element by element.
            for (int i = 0; i < p_pattern->array.size(); i++) {
                if (p_pattern->array[i]->pattern_type == OScriptParser::PatternNode::PT_REST) {
                    // Don't want to access an extra element of the user array.
                    break;
                }

                // Use AND here too, as we don't want to be checking elements if previous test failed (which means this might be an invalid get).
                p_context.generator->write_and_left_operand(result_addr);

                // Add index to constant map.
                OScriptCodeGenerator::Address index_addr = p_context.add_constant(i);

                // Get the actual element from the user-sent array.
                p_context.generator->write_get(element_addr, index_addr, p_value_addr);

                // Also get type of element.
                Vector<OScriptCodeGenerator::Address> typeof_args;
                typeof_args.push_back(element_addr);
                p_context.generator->write_call_utility(element_type_addr, "typeof", typeof_args);

                // Try the pattern inside the element.
                result_addr = parse_match_pattern(p_context, r_error, p_pattern->array[i], element_addr, element_type_addr, result_addr, false, true);
                if (r_error != OK) {
                    return OScriptCodeGenerator::Address();
                }

                p_context.generator->write_and_right_operand(result_addr);
                p_context.generator->write_end_and(result_addr);
            }
            // Remove element temporaries.
            p_context.generator->pop_temporary();
            p_context.generator->pop_temporary();

            // If this isn't the first, we need to OR with the previous pattern. If it's nested, we use AND instead.
            if (p_is_nested) {
                // Use the previous value as target, since we only need one temporary variable.
                p_context.generator->write_and_right_operand(result_addr);
                p_context.generator->write_end_and(p_prev_test);
            } else if (!p_is_first) {
                // Use the previous value as target, since we only need one temporary variable.
                p_context.generator->write_or_right_operand(result_addr);
                p_context.generator->write_end_or(p_prev_test);
            } else {
                // Just assign this value to the accumulator temporary.
                p_context.generator->write_assign(p_prev_test, result_addr);
            }
            p_context.generator->pop_temporary(); // Remove temp result addr.

            return p_prev_test;
        }
        case OScriptParser::PatternNode::PT_DICTIONARY: {
            if (p_is_nested) {
                p_context.generator->write_and_left_operand(p_prev_test);
            } else if (!p_is_first) {
                p_context.generator->write_or_left_operand(p_prev_test);
            }
            // Get dictionary type into constant map.
            OScriptCodeGenerator::Address dict_type_addr = p_context.add_constant((int)Variant::DICTIONARY);

            // Equality is always a boolean.
            OScriptDataType temp_type;
            temp_type.kind = OScriptDataType::BUILTIN;
            temp_type.builtin_type = Variant::BOOL;

            // Check type equality.
            OScriptCodeGenerator::Address result_addr = p_context.add_temporary(temp_type);
            p_context.generator->write_binary_operator(result_addr, Variant::OP_EQUAL, p_type_addr, dict_type_addr);
            p_context.generator->write_and_left_operand(result_addr);

            // Store pattern length in constant map.
            OScriptCodeGenerator::Address dict_length_addr = p_context.add_constant(p_pattern->rest_used ? p_pattern->dictionary.size() - 1 : p_pattern->dictionary.size());

            // Get user's dictionary length.
            temp_type.builtin_type = Variant::INT;
            OScriptCodeGenerator::Address value_length_addr = p_context.add_temporary(temp_type);
            Vector<OScriptCodeGenerator::Address> func_args;
            func_args.push_back(p_value_addr);
            p_context.generator->write_call_oscript_utility(value_length_addr, "len", func_args);

            // Test length compatibility.
            temp_type.builtin_type = Variant::BOOL;
            OScriptCodeGenerator::Address length_compat_addr = p_context.add_temporary(temp_type);
            p_context.generator->write_binary_operator(length_compat_addr, p_pattern->rest_used ? Variant::OP_GREATER_EQUAL : Variant::OP_EQUAL, value_length_addr, dict_length_addr);
            p_context.generator->write_and_right_operand(length_compat_addr);

            // AND type and length check.
            p_context.generator->write_end_and(result_addr);

            // Remove length temporaries.
            p_context.generator->pop_temporary();
            p_context.generator->pop_temporary();

            // Create temporaries outside the loop so they can be reused.
            OScriptCodeGenerator::Address element_addr = p_context.add_temporary();
            OScriptCodeGenerator::Address element_type_addr = p_context.add_temporary();

            // Evaluate element by element.
            for (int i = 0; i < p_pattern->dictionary.size(); i++) {
                const OScriptParser::PatternNode::Pair &element = p_pattern->dictionary[i];
                if (element.value_pattern && element.value_pattern->pattern_type == OScriptParser::PatternNode::PT_REST) {
                    // Ignore rest pattern.
                    break;
                }

                // Use AND here too, as we don't want to be checking elements if previous test failed (which means this might be an invalid get).
                p_context.generator->write_and_left_operand(result_addr);

                // Get the pattern key.
                OScriptCodeGenerator::Address pattern_key_addr = parse_expression(p_context, r_error, element.key);
                if (r_error) {
                    return OScriptCodeGenerator::Address();
                }

                // Check if pattern key exists in user's dictionary. This will be AND-ed with next result.
                func_args.clear();
                func_args.push_back(pattern_key_addr);
                p_context.generator->write_call(result_addr, p_value_addr, "has", func_args);

                if (element.value_pattern != nullptr) {
                    // Use AND here too, as we don't want to be checking elements if previous test failed (which means this might be an invalid get).
                    p_context.generator->write_and_left_operand(result_addr);

                    // Get actual value from user dictionary.
                    p_context.generator->write_get(element_addr, pattern_key_addr, p_value_addr);

                    // Also get type of value.
                    func_args.clear();
                    func_args.push_back(element_addr);
                    p_context.generator->write_call_utility(element_type_addr, "typeof", func_args);

                    // Try the pattern inside the value.
                    result_addr = parse_match_pattern(p_context, r_error, element.value_pattern, element_addr, element_type_addr, result_addr, false, true);
                    if (r_error != OK) {
                        return OScriptCodeGenerator::Address();
                    }
                    p_context.generator->write_and_right_operand(result_addr);
                    p_context.generator->write_end_and(result_addr);
                }

                p_context.generator->write_and_right_operand(result_addr);
                p_context.generator->write_end_and(result_addr);

                // Remove pattern key temporary.
                if (pattern_key_addr.mode == OScriptCodeGenerator::Address::TEMPORARY) {
                    p_context.generator->pop_temporary();
                }
            }

            // Remove element temporaries.
            p_context.generator->pop_temporary();
            p_context.generator->pop_temporary();

            // If this isn't the first, we need to OR with the previous pattern. If it's nested, we use AND instead.
            if (p_is_nested) {
                // Use the previous value as target, since we only need one temporary variable.
                p_context.generator->write_and_right_operand(result_addr);
                p_context.generator->write_end_and(p_prev_test);
            } else if (!p_is_first) {
                // Use the previous value as target, since we only need one temporary variable.
                p_context.generator->write_or_right_operand(result_addr);
                p_context.generator->write_end_or(p_prev_test);
            } else {
                // Just assign this value to the accumulator temporary.
                p_context.generator->write_assign(p_prev_test, result_addr);
            }
            p_context.generator->pop_temporary(); // Remove temp result addr.

            return p_prev_test;
        }
        case OScriptParser::PatternNode::PT_REST: {
            // Do nothing.
            return p_prev_test;
        }
		case OScriptParser::PatternNode::PT_BIND: {
			if (p_is_nested) {
				p_context.generator->write_and_left_operand(p_prev_test);
			} else if (!p_is_first) {
				p_context.generator->write_or_left_operand(p_prev_test);
			}
			// Get the bind address.
			OScriptCodeGenerator::Address bind = p_context.locals[p_pattern->bind->name];

			// Assign value to bound variable.
			p_context.generator->write_assign(bind, p_value_addr);
		} [[fallthrough]]; // Act like matching anything too.
		case OScriptParser::PatternNode::PT_WILDCARD: {
		    // If this is a fall through we don't want to do this again.
		    if (p_pattern->pattern_type != OScriptParser::PatternNode::PT_BIND) {
		        if (p_is_nested) {
		            p_context.generator->write_and_left_operand(p_prev_test);
		        } else if (!p_is_first) {
		            p_context.generator->write_or_left_operand(p_prev_test);
		        }
		    }
		    // This matches anything so just do the same as `if(true)`.
		    // If this isn't the first, we need to OR with the previous pattern. If it's nested, we use AND instead.
		    if (p_is_nested) {
		        // Use the operator with the `true` constant so it works as always matching.
		        OScriptCodeGenerator::Address constant = p_context.add_constant(true);
		        p_context.generator->write_and_right_operand(constant);
		        p_context.generator->write_end_and(p_prev_test);
		    } else if (!p_is_first) {
		        // Use the operator with the `true` constant so it works as always matching.
		        OScriptCodeGenerator::Address constant = p_context.add_constant(true);
		        p_context.generator->write_or_right_operand(constant);
		        p_context.generator->write_end_or(p_prev_test);
		    } else {
		        // Just assign this value to the accumulator temporary.
		        p_context.generator->write_assign_true(p_prev_test);
		    }
		    return p_prev_test;
		}
    }

    set_error("Compiler bug (please report): Reaching the end of pattern compilation without matching a pattern.", p_pattern);
    r_error = ERR_COMPILATION_FAILED;
    return p_prev_test;
}

Error OScriptCompiler::prepare_compilation(OScript* p_script, const OScriptParser::ClassNode* p_class, bool p_keep_state) {
    if (parsed_classes.has(p_script)) {
        return OK;
    }

    if (parsing_classes.has(p_script)) {
        String class_name = p_class->identifier ? String(p_class->identifier->name) : p_class->fqcn;
        set_error(vformat(R"(Cyclic class reference for "%s".)", class_name), p_class);
        return ERR_PARSE_ERROR;
    }

    parsing_classes.insert(p_script);

    p_script->clearing = true;
    p_script->cancel_pending_functions(true);
    p_script->native = Ref<OScriptNativeClass>();
    p_script->base = Ref<OScript>();
    p_script->members.clear();

    HashMap<StringName, Variant> constants;
    for (const KeyValue<StringName, Variant>& E : p_script->constants) {
        constants.insert(E.key, E.value);
    }
    p_script->constants.clear();
    constants.clear();

    HashMap<StringName, OScriptCompiledFunction*> member_functions;
    for (const KeyValue<StringName, OScriptCompiledFunction*>& E : p_script->member_functions) {
        member_functions.insert(E.key, E.value);
    }
    p_script->member_functions.clear();

    for (const KeyValue<StringName, OScriptCompiledFunction*>& E : member_functions) {
        memdelete(E.value);
    }
    member_functions.clear();

    p_script->static_variables.clear();

    if (p_script->implicit_initializer) {
        memdelete(p_script->implicit_initializer);
    }
    if (p_script->implicit_ready) {
        memdelete(p_script->implicit_ready);
    }
    if (p_script->static_initializer) {
        memdelete(p_script->static_initializer);
    }

    p_script->member_functions.clear();
    p_script->member_indices.clear();
    p_script->static_variables_indices.clear();
    p_script->static_variables.clear();
    p_script->signals.clear();
    p_script->initializer = nullptr;
    p_script->implicit_initializer = nullptr;
    p_script->implicit_ready = nullptr;
    p_script->static_initializer = nullptr;
    p_script->rpc_config.clear();

    p_script->lambda_info.clear();

    p_script->clearing = false;
    p_script->_tool = parser->is_tool();
    p_script->is_abstract = p_class->is_abstract;

    if (p_script->local_name != StringName()) {
        if (OScriptAnalyzer::class_exists(p_script->local_name)) {
            set_error(vformat(R"(The class "%s" shadows a native class)", p_script->local_name), p_class);
            return ERR_ALREADY_EXISTS;
        }
    }

    OScriptDataType base_type = resolve_type(p_class->base_type, p_script, false);
    if (base_type.native_type == StringName()) {
        set_error(vformat(R"(Parser bug (please report): Empty native type in base class "%s")", p_script->path), p_class);
        return ERR_BUG;
    }

    int native_index = OScriptLanguage::get_singleton()->get_global_map()[base_type.native_type];
    p_script->native = OScriptLanguage::get_singleton()->get_global_array()[native_index];
    if (p_script->native.is_null()) {
        set_error(vformat(R"(Compiler bug (please report): script native type is null with index %d.)", native_index), nullptr);
        return ERR_BUG;
    }

    // Inheritance
    switch (base_type.kind) {
        case OScriptDataType::NATIVE: {
            // Nothing more to do
            break;
        }
        case OScriptDataType::OSCRIPT: {
            Ref<OScript> base = Ref<OScript>(base_type.script_type);
            if (base.is_null()) {
                set_error("Compiler bug (please report): base script type is null.", nullptr);
                return ERR_BUG;
            }

            if (main_script->has_class(base.ptr())) {
                Error err = prepare_compilation(base.ptr(), p_class->base_type.class_type, p_keep_state);
                if (err) {
                    return err;
                }
            } else if (!base->_is_valid()) {
                Error err = OK;
                Ref<OScript> base_root = OScriptCache::get_shallow_script(base->path, err, p_script->path);
                if (err) {
                    set_error(vformat(R"(Could not parse base class "%s" from "%s": %s)", base->fully_qualified_name, base->path, error_names[err]), nullptr);
                    return err;
                }
                if (base_root.is_valid()) {
                    base = Ref<OScript>(base_root->find_class(base->fully_qualified_name));
                }
                if (base.is_null()) {
                    set_error(vformat(R"(Could not find class "%s" in "%s".)", base->fully_qualified_name, base->path), nullptr);
                    return ERR_COMPILATION_FAILED;
                }
                err = prepare_compilation(base.ptr(), p_class->base_type.class_type, p_keep_state);
                if (err) {
                    set_error(vformat(R"(Could not populate class members of base class "%s" in %s".)", base->fully_qualified_name, base->path), nullptr);
                    return err;
                }
            }

            p_script->base = base;
            p_script->member_indices = base->member_indices;
            break;
        }
        default: {
            set_error("Parser bug (please report): invalid inheritance.", nullptr);
            return ERR_BUG;
            break;
        }
    }

    // Duplicate RPC information from base OScript
    // Base script isn't valid because it should not have been compiled yet, but the reference contains info
    if (base_type.kind == OScriptDataType::OSCRIPT && p_script->base.is_valid()) {
        p_script->rpc_config = p_script->base->rpc_config.duplicate();
    }

    for (int i = 0; i < p_class->members.size(); i++) {
        const OScriptParser::ClassNode::Member& member = p_class->members[i];
        switch (member.type) {
            case OScriptParser::ClassNode::Member::VARIABLE: {
                const OScriptParser::VariableNode* variable = member.variable;
                StringName name = variable->identifier->name;

                OScript::MemberInfo minfo;
                switch (variable->style) {
                    case OScriptParser::VariableNode::NONE: {
                        // Nothing to do
                        break;
                    }
                    case OScriptParser::VariableNode::SETGET: {
                        if (variable->setter_pointer != nullptr) {
                            minfo.setter = variable->setter_pointer->name;
                        }
                        if (variable->getter_pointer != nullptr) {
                            minfo.getter = variable->getter_pointer->name;
                        }
                        break;
                    }
                    case OScriptParser::VariableNode::INLINE: {
                        if (variable->setter != nullptr) {
                            minfo.setter = "@" + variable->identifier->name + "_setter";
                        }
                        if (variable->getter != nullptr) {
                            minfo.getter = "@" + variable->identifier->name + "_getter";
                        }
                        break;
                    }
                }

                minfo.data_type = resolve_type(variable->get_datatype(), p_script);

                PropertyInfo property = variable->get_datatype().to_property_info(name);
                PropertyInfo export_info = variable->export_info;

                if (variable->exported) {
                    if (!minfo.data_type.has_type()) {
                        property.type = export_info.type;
                        property.class_name = export_info.class_name;
                    }
                    property.hint = export_info.hint;
                    property.hint_string = export_info.hint_string;
                    property.usage = export_info.usage;
                }
                property.usage |= PROPERTY_USAGE_SCRIPT_VARIABLE;
                minfo.property_info = property;

                if (variable->is_static) {
                    minfo.index = p_script->static_variables_indices.size();
                    p_script->static_variables_indices[name] = minfo;
                } else {
                    minfo.index = p_script->member_indices.size();
                    p_script->member_indices[name] = minfo;
                    p_script->members.insert(name);
                }

                #ifdef TOOLS_ENABLED
                if (variable->initializer != nullptr && variable->initializer->is_constant) {
                    p_script->member_default_values[name] = variable->initializer->reduced_value;
                    convert_to_initializer_type(p_script->member_default_values[name], variable);
                } else {
                    p_script->member_default_values.erase(name);
                }
                #endif

                break;
            }
            case OScriptParser::ClassNode::Member::CONSTANT: {
                const OScriptParser::ConstantNode* constant = member.constant;
                StringName name = constant->identifier->name;
                p_script->constants.insert(name, constant->initializer->reduced_value);
                break;
            }
            case OScriptParser::ClassNode::Member::ENUM_VALUE: {
                const OScriptParser::EnumNode::Value& enum_value = member.enum_value;
                StringName name = enum_value.identifier->name;
                p_script->constants.insert(name, enum_value.value);
                break;
            }
            case OScriptParser::ClassNode::Member::SIGNAL: {
                const OScriptParser::SignalNode* signal = member.signal;
                StringName name = signal->identifier->name;
                p_script->signals[name] = signal->method;
                break;
            }
            case OScriptParser::ClassNode::Member::ENUM: {
                const OScriptParser::EnumNode* enum_n = member.m_enum;
                StringName name = enum_n->identifier->name;
                p_script->constants.insert(name, enum_n->dictionary);
                break;
            }
            case OScriptParser::ClassNode::Member::GROUP: {
                const OScriptParser::AnnotationNode *annotation = member.annotation;
                // Avoid name conflict. See GH-78252.
                StringName name = vformat("@group_%d_%s", p_script->members.size(), annotation->export_info.name);

                // This is not a normal member, but we need this to keep indices in order.
                OScript::MemberInfo minfo;
                minfo.index = p_script->member_indices.size();

                PropertyInfo prop_info;
                prop_info.name = annotation->export_info.name;
                prop_info.usage = annotation->export_info.usage;
                prop_info.hint_string = annotation->export_info.hint_string;
                minfo.property_info = prop_info;

                p_script->member_indices[name] = minfo;
                p_script->members.insert(name);
                break;
            }
            case OScriptParser::ClassNode::Member::FUNCTION: {
                const OScriptParser::FunctionNode* function = member.function;
                Variant config = function->rpc_config;
                if (config.get_type() != Variant::NIL) {
                    p_script->rpc_config[function->identifier->name] = config;
                }
                break;
            }
            default: {
                // Nothing to do
                break;
            }
        }
    }

    p_script->static_variables.resize(p_script->static_variables_indices.size());

    parsed_classes.insert(p_script);
    parsing_classes.erase(p_script);

    // Populate inner classes.
    for (int i = 0; i < p_class->members.size(); i++) {
        const OScriptParser::ClassNode::Member& member = p_class->members[i];
        if (member.type != OScriptParser::ClassNode::Member::CLASS) {
            continue;
        }
        const OScriptParser::ClassNode *inner_class = member.m_class;
        StringName name = inner_class->identifier->name;
        Ref<OScript> &subclass = p_script->subclasses[name];
        OScript *subclass_ptr = subclass.ptr();

        // Subclass might still be parsing, just skip it
        if (!parsing_classes.has(subclass_ptr)) {
            Error err = prepare_compilation(subclass_ptr, inner_class, p_keep_state);
            if (err) {
                return err;
            }
        }

        p_script->constants.insert(name, subclass); //once parsed, goes to the list of constants
    }

    return OK;
}

Error OScriptCompiler::compile_class(OScript* p_script, const OScriptParser::ClassNode* p_class, bool p_keep_state) {
    for (int i = 0; i < p_class->members.size(); i++) {
        const OScriptParser::ClassNode::Member& member = p_class->members[i];
        if (member.type == OScriptParser::ClassNode::Member::FUNCTION) {
            Error err;
            const OScriptParser::FunctionNode* function = member.function;
            parse_function(err, p_script, p_class, function);
            if (err) {
                return err;
            }
        } else if (member.type == OScriptParser::ClassNode::Member::VARIABLE) {
            const OScriptParser::VariableNode* variable = member.variable;
            if (variable->style == OScriptParser::VariableNode::INLINE) {
                if (variable->setter != nullptr) {
                    Error err = parse_setter_getter(p_script, p_class, variable, true);
                    if (err) {
                        return err;
                    }
                }
                if (variable->getter != nullptr) {
                    Error err = parse_setter_getter(p_script, p_class, variable, false);
                    if (err) {
                        return err;
                    }
                }
            }
        }
    }

    // Create `@implicit_new()` special function
    Error err = OK;
    parse_function(err, p_script, p_class, nullptr);
    if (err) {
        return err;
    }

    if (p_class->onready_used) {
        // Create `@implicit_ready()` special function
        parse_function(err, p_script, p_class, nullptr, true);
        if (err) {
            return err;
        }
    }

    if (p_class->has_static_data) {
        OScriptCompiledFunction* func = make_static_initializer(err, p_script, p_class);
        p_script->static_initializer = func;
        if (err) {
            return err;
        }
    }

    #ifdef DEBUG_ENABLED
    if (p_keep_state) {
        for (RBSet<Object*>::Element* E = p_script->instances.front(); E;) {
            RBSet<Object*>::Element* N = E->next();

            OScriptInstanceBase* sib = p_script->instance_script_instances[E->get()];
            if (sib->is_placeholder()) {
                #ifdef TOOLS_ENABLED
                OScriptPlaceHolderInstance* psi = static_cast<OScriptPlaceHolderInstance*>(sib);
                if (p_script->is_tool()) {
                    // Recreate as an instance
                    p_script->placeholders.erase(psi);

                    p_script->_instance_create(E->get());
                    OScriptInstance* si = static_cast<OScriptInstance*>(p_script->instance_script_instances[E->get()]);
                    si->_members.resize(p_script->member_indices.size());
                    si->_script = Ref<OScript>(p_script);
                    si->_owner = E->get();

                    // Hot reloading
                    for (const KeyValue<StringName, OScript::MemberInfo>& F : p_script->member_indices) {
                        si->_member_indices_cache[F.key] = F.value.index;
                    }

                    GDExtensionCallError error;
                    p_script->initializer->call(si, nullptr, 0, error);
                    if (error.error != GDEXTENSION_CALL_OK) {
                        // well, not gonna do anything
                    }
                }
                #endif
            } else {
                OScriptInstance* si = static_cast<OScriptInstance*>(sib);
                si->reload_members();
            }

            E = N;
        }
    }
    #endif

    has_static_data = p_class->has_static_data;

    for (int i = 0; i < p_class->members.size(); i++) {
        if (p_class->members[i].type != OScriptParser::ClassNode::Member::CLASS) {
            continue;
        }
        const OScriptParser::ClassNode *inner_class = p_class->members[i].m_class;
        StringName name = inner_class->identifier->name;
        OScript *subclass = p_script->subclasses[name].ptr();

        err = compile_class(subclass, inner_class, p_keep_state);
        if (err) {
            return err;
        }

        has_static_data = has_static_data || inner_class->has_static_data;
    }

    p_script->_static_default_init();
    p_script->_valid = true;

    return OK;
}

void OScriptCompiler::convert_to_initializer_type(Variant& p_variant, const OScriptParser::VariableNode* p_node) {
    // Set p_variant to the value of p_node's initializer, with the type of p_node's variable.
    OScriptParser::DataType member_t = p_node->data_type;
    OScriptParser::DataType init_t = p_node->initializer->data_type;

    if (member_t.is_hard_type()
            && init_t.is_hard_type()
            && member_t.kind == OScriptParser::DataType::BUILTIN
            && init_t.kind == OScriptParser::DataType::BUILTIN) {
        if (Variant::can_convert_strict(init_t.builtin_type, member_t.builtin_type)) {
            const Variant *v = &p_node->initializer->reduced_value;
            GDE::Variant::construct(member_t.builtin_type, p_variant, &v, 1);
        }
    }
}

void OScriptCompiler::make_scripts(OScript* p_script, const OScriptParser::ClassNode* p_class, bool p_keep_state) {

    p_script->fully_qualified_name = p_class->fqcn;
    p_script->local_name = p_class->identifier ? p_class->identifier->name : StringName();
    p_script->global_name = p_class->get_global_name();
    p_script->simplified_icon_path = p_class->simplified_icon_path;

    HashMap<StringName, Ref<OScript>> old_subclasses;

    if (p_keep_state) {
        old_subclasses = p_script->subclasses;
    }

    p_script->subclasses.clear();

    for (int i = 0; i < p_class->members.size(); i++) {
        if (p_class->members[i].type != OScriptParser::ClassNode::Member::CLASS) {
            continue;
        }
        const OScriptParser::ClassNode *inner_class = p_class->members[i].m_class;
        StringName name = inner_class->identifier->name;

        Ref<OScript> subclass;

        if (old_subclasses.has(name)) {
            subclass = old_subclasses[name];
        } else {
            subclass = OScriptLanguage::get_singleton()->get_orphan_subclass(inner_class->fqcn);
        }

        if (subclass.is_null()) {
            subclass.instantiate();
        }

        subclass->_owner = p_script;
        subclass->path = p_script->path;
        p_script->subclasses.insert(name, subclass);

        make_scripts(subclass.ptr(), inner_class, p_keep_state);
    }
}

Error OScriptCompiler::compile(const OScriptParser* p_parser, OScript* p_script, bool p_keep_state) {
    ERR_FAIL_NULL_V(p_parser, ERR_COMPILATION_FAILED);
    ERR_FAIL_NULL_V(p_script, ERR_COMPILATION_FAILED);

    err_node_id = -1;
    error = "";

    parser = p_parser;
    main_script = p_script;

    const OScriptParser::ClassNode* root = parser->get_tree();
    ERR_FAIL_NULL_V(root, ERR_COMPILATION_FAILED);

    source = p_script->get_path();

    // Create scripts for subclasses beforehand so they can be referenced
    make_scripts(p_script, root, p_keep_state);

    main_script->subclass_owner = nullptr;
    Error err = prepare_compilation(main_script, parser->get_tree(), p_keep_state);
    if (err) {
        return err;
    }

    err = compile_class(main_script, root, p_keep_state);
    if (err) {
        return err;
    }

    String root_path = main_script->path;
    if (root_path.is_empty()) {
        root_path = main_script->get_path();
        if (root_path.is_empty()) {

        }
    }

    err = OScriptCache::finish_compiling(root_path);
    if (err) {
        set_error(R"(Failed to compile depended scripts.)", nullptr);
    }

    return err;
}

String OScriptCompiler::get_error() const {
    return error;
}

int OScriptCompiler::get_error_node_id() const {
    return err_node_id;
}

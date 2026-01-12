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
#include "script/compiler/bytecode_generator.h"

#include "api/extension_db.h"
#include "common/method_utils.h"
#include "core/godot/method_bind.h"
#include "core/godot/variant/variant.h"
#include "script/language.h"
#include "script/script.h"

#define HAS_BUILTIN_TYPE(m_var) \
(m_var.type.kind == OScriptDataType::BUILTIN)

#define IS_BUILTIN_TYPE(m_var, m_type) \
(m_var.type.kind == OScriptDataType::BUILTIN && m_var.type.builtin_type == m_type && m_type != Variant::NIL)

void OScriptBytecodeGenerator::add_stack_identifier(const StringName& p_id, int p_pos) {
    if (locals.size() > max_locals) {
        max_locals = locals.size();
    }

    stack_identifiers[p_id] = p_pos;
    if (OScriptLanguage::get_singleton()->should_track_locals()) {
        block_identifiers[p_id] = p_pos;
        OScriptCompiledFunction::StackDebug sd;
        sd.added = true;
        sd.source_node_id = current_script_node_id;
        sd.identifier = p_id;
        sd.pos = p_pos;
        stack_debug.push_back(sd);
    }
}

void OScriptBytecodeGenerator::push_stack_identifiers() {
    stack_identifiers_counts.push_back(locals.size());
    stack_id_stack.push_back(stack_identifiers);

    if (OScriptLanguage::get_singleton()->should_track_locals()) {
        RBMap<StringName, int> block_ids(block_identifiers);
        block_identifier_stack.push_back(block_ids);
        block_identifiers.clear();
    }
}

void OScriptBytecodeGenerator::pop_stack_identifiers() {
    int current_locals = stack_identifiers_counts.back()->get();
    stack_identifiers_counts.pop_back();
    stack_identifiers = stack_id_stack.back()->get();
    stack_id_stack.pop_back();

    #ifdef DEBUG_ENABLED
    if (!used_temporaries.is_empty()) {
        ERR_PRINT("Leaving block with non-zero temporary variables: " + itos(used_temporaries.size()));
    }
    #endif

    for (int i = current_locals; i < locals.size(); i++) {
        dirty_locals.insert(i + OScriptCompiledFunction::FIXED_ADDRESSES_MAX);
    }
    locals.resize(current_locals);

    if (OScriptLanguage::get_singleton()->should_track_locals()) {
        for (const KeyValue<StringName, int>& E : block_identifiers) {
            OScriptCompiledFunction::StackDebug sd;
            sd.added = false;
            sd.identifier = E.key;
            sd.source_node_id = current_script_node_id;
            sd.pos = E.value;
            stack_debug.push_back(sd);
        }
        block_identifiers = block_identifier_stack.back()->get();
        block_identifier_stack.pop_back();
    }
}

int OScriptBytecodeGenerator::get_name_map_pos(const StringName& p_identifier) {
    if (name_map.has(p_identifier)) {
        return name_map[p_identifier];
    }

    const int index = name_map.size();
    name_map[p_identifier] = index;
    return index;
}

int OScriptBytecodeGenerator::get_constant_pos(const Variant& p_value) {
    if (constant_map.has(p_value)) {
        return constant_map[p_value];
    }

    const int pos = constant_map.size();
    constant_map[p_value] = pos;
    return pos;
}

int OScriptBytecodeGenerator::get_operation_pos(GDExtensionPtrOperatorEvaluator p_operation) {
    if (operator_func_map.has(p_operation)) {
        return operator_func_map[p_operation];
    }

    const int pos = operator_func_map.size();
    operator_func_map[p_operation] = pos;
    return pos;
}

int OScriptBytecodeGenerator::get_setter_pos(GDExtensionPtrSetter p_setter) {
    if (setters_map.has(p_setter)) {
        return setters_map[p_setter];
    }

    const int pos = setters_map.size();
    setters_map[p_setter] = pos;
    return pos;
}

int OScriptBytecodeGenerator::get_getter_pos(GDExtensionPtrGetter p_getter) {
    if (getters_map.has(p_getter)) {
        return getters_map[p_getter];
    }

    const int pos = getters_map.size();
    getters_map[p_getter] = pos;
    return pos;
}

int OScriptBytecodeGenerator::get_indexed_setter_pos(GDExtensionPtrIndexedSetter p_setter) {
    if (indexed_setters_map.has(p_setter)) {
        return indexed_setters_map[p_setter];
    }

    const int pos = indexed_setters_map.size();
    indexed_setters_map[p_setter] = pos;
    return pos;
}

int OScriptBytecodeGenerator::get_indexed_getter_pos(GDExtensionPtrIndexedGetter p_getter) {
    if (indexed_getters_map.has(p_getter)) {
        return indexed_getters_map[p_getter];
    }

    const int pos = indexed_getters_map.size();
    indexed_getters_map[p_getter] = pos;
    return pos;
}

int OScriptBytecodeGenerator::get_keyed_setter_pos(GDExtensionPtrKeyedSetter p_setter) {
    if (keyed_setters_map.has(p_setter)) {
        return keyed_setters_map[p_setter];
    }

    const int pos = keyed_setters_map.size();
    keyed_setters_map[p_setter] = pos;
    return pos;
}

int OScriptBytecodeGenerator::get_keyed_getter_pos(GDExtensionPtrKeyedGetter p_getter) {
    if (keyed_getters_map.has(p_getter)) {
        return keyed_getters_map[p_getter];
    }

    const int pos = keyed_getters_map.size();
    keyed_getters_map[p_getter] = pos;
    return pos;
}

int OScriptBytecodeGenerator::get_utility_pos(GDExtensionPtrUtilityFunction p_function) {
    if (utility_functions_map.has(p_function)) {
        return utility_functions_map[p_function];
    }

    const int pos = utility_functions_map.size();
    utility_functions_map[p_function] = pos;
    return pos;
}

int OScriptBytecodeGenerator::get_os_utility_pos(OScriptUtilityFunctions::FunctionPtr p_function) {
    if (os_functions_map.has(p_function)) {
        return os_functions_map[p_function];
    }

    const int pos = os_functions_map.size();
    os_functions_map[p_function] = pos;
    return pos;
}

int OScriptBytecodeGenerator::get_constructor_pos(GDExtensionPtrConstructor p_constructor) {
    if (constructors_map.has(p_constructor)) {
        return constructors_map[p_constructor];
    }

    const int pos = constructors_map.size();
    constructors_map[p_constructor] = pos;
    return pos;
}

int OScriptBytecodeGenerator::get_builtin_method_pos(GDExtensionPtrBuiltInMethod p_method) {
    if (built_in_methods_map.has(p_method)) {
        return built_in_methods_map[p_method];
    }

    const int pos = built_in_methods_map.size();
    built_in_methods_map[p_method] = pos;
    return pos;
}

int OScriptBytecodeGenerator::get_method_bind_pos(MethodBind* p_method) {
    if (method_bind_map.has(p_method)) {
        return method_bind_map[p_method];
    }

    const int pos = method_bind_map.size();
    method_bind_map[p_method] = pos;
    return pos;
}

int OScriptBytecodeGenerator::get_lambda_function_pos(OScriptCompiledFunction* p_function) {
    if (lambdas_map.has(p_function)) {
        return lambdas_map[p_function];
    }

    const int pos = lambdas_map.size();
    lambdas_map[p_function] = pos;
    return pos;
}

OScriptBytecodeGenerator::CallTarget OScriptBytecodeGenerator::get_call_target(const Address& p_target, Variant::Type p_type) {
    if (p_target.mode == Address::NIL) {
        OScriptDataType type;
        if (p_type != Variant::NIL) {
            type.kind = OScriptDataType::BUILTIN;
            type.builtin_type = p_type;
        }
        uint32_t addr = add_temporary(type);
        return CallTarget(Address(Address::TEMPORARY, addr, type), true, this);
    }

    return CallTarget(p_target, false, this);
}

int OScriptBytecodeGenerator::address_of(const Address& p_address) {
    switch (p_address.mode) {
        case Address::SELF: {
            return OScriptCompiledFunction::ADDR_SELF;
        }
        case Address::CLASS: {
            return OScriptCompiledFunction::ADDR_CLASS;
        }
        case Address::MEMBER: {
            return p_address.address | (OScriptCompiledFunction::ADDR_TYPE_MEMBER << OScriptCompiledFunction::ADDR_BITS);
        }
        case Address::CONSTANT: {
            return p_address.address | (OScriptCompiledFunction::ADDR_TYPE_CONSTANT << OScriptCompiledFunction::ADDR_BITS);
        }
        case Address::LOCAL_VARIABLE:
        case Address::FUNCTION_PARAMETER: {
            return p_address.address | (OScriptCompiledFunction::ADDR_TYPE_STACK << OScriptCompiledFunction::ADDR_BITS);
        }
        case Address::TEMPORARY: {
            temporaries.write[p_address.address].bytecode_indices.push_back(opcodes.size());
            return -1;
        }
        case Address::NIL: {
            return OScriptCompiledFunction::ADDR_NIL;
        }
    }
    return -1; // Unreachable.
}

void OScriptBytecodeGenerator::append_opcode(OScriptCompiledFunction::Opcode p_code) {
    opcodes.push_back(p_code);
}

void OScriptBytecodeGenerator::append_opcode_and_argcount(OScriptCompiledFunction::Opcode p_code, int p_arg_count) {
    opcodes.push_back(p_code);
    opcodes.push_back(p_arg_count);
    instr_args_max = MAX(instr_args_max, p_arg_count);
}

void OScriptBytecodeGenerator::append(int p_code) {
    opcodes.push_back(p_code);
}

void OScriptBytecodeGenerator::append(const Address& p_address) {
    opcodes.push_back(address_of(p_address));
}

void OScriptBytecodeGenerator::append(const StringName& p_name) {
    opcodes.push_back(get_name_map_pos(p_name));
}

void OScriptBytecodeGenerator::append_op_eval(GDExtensionPtrOperatorEvaluator p_operation) {
    opcodes.push_back(get_operation_pos(p_operation));
}

void OScriptBytecodeGenerator::append(GDExtensionPtrSetter p_setter) {
    opcodes.push_back(get_setter_pos(p_setter));
}

void OScriptBytecodeGenerator::append(GDExtensionPtrGetter p_getter) {
    opcodes.push_back(get_getter_pos(p_getter));
}

void OScriptBytecodeGenerator::append(GDExtensionPtrIndexedSetter p_setter) {
    opcodes.push_back(get_indexed_setter_pos(p_setter));
}

void OScriptBytecodeGenerator::append(GDExtensionPtrIndexedGetter p_getter) {
    opcodes.push_back(get_indexed_getter_pos(p_getter));
}

void OScriptBytecodeGenerator::append(GDExtensionPtrKeyedSetter p_setter) {
    opcodes.push_back(get_keyed_setter_pos(p_setter));
}

void OScriptBytecodeGenerator::append(GDExtensionPtrKeyedGetter p_getter) {
    opcodes.push_back(get_keyed_getter_pos(p_getter));
}

void OScriptBytecodeGenerator::append(GDExtensionPtrUtilityFunction p_function) {
    opcodes.push_back(get_utility_pos(p_function));
}

void OScriptBytecodeGenerator::append(OScriptUtilityFunctions::FunctionPtr p_function) {
    opcodes.push_back(get_os_utility_pos(p_function));
}

void OScriptBytecodeGenerator::append(GDExtensionPtrConstructor p_constructor) {
    opcodes.push_back(get_constructor_pos(p_constructor));
}

void OScriptBytecodeGenerator::append(GDExtensionPtrBuiltInMethod p_method) {
    opcodes.push_back(get_builtin_method_pos(p_method));
}

void OScriptBytecodeGenerator::append(MethodBind* p_method) {
    opcodes.push_back(get_method_bind_pos(p_method));
}

void OScriptBytecodeGenerator::append(OScriptCompiledFunction* p_lambda) {
    opcodes.push_back(get_lambda_function_pos(p_lambda));
}

void OScriptBytecodeGenerator::patch_jump(int p_address) {
    opcodes.write[p_address] = opcodes.size();
}

uint32_t OScriptBytecodeGenerator::add_parameter(const StringName& p_name, bool p_is_optional, const OScriptDataType& p_type) {
    function->argument_count++;
    function->argument_types.push_back(p_type);
    if (p_is_optional) {
        function->default_arg_count++;
    }
    return add_local(p_name, p_type);
}

uint32_t OScriptBytecodeGenerator::add_local(const StringName& p_name, const OScriptDataType& p_type) {
    const int stack_pos = locals.size() + OScriptCompiledFunction::FIXED_ADDRESSES_MAX;
    locals.push_back(StackSlot(p_type.builtin_type, p_type.can_contain_object()));
    add_stack_identifier(p_name, stack_pos);
    return stack_pos;
}

uint32_t OScriptBytecodeGenerator::add_local_constant(const StringName& p_name, const Variant& p_value) {
    const int index = add_or_get_constant(p_value);
    local_constants[p_name] = index;
    return index;
}

uint32_t OScriptBytecodeGenerator::add_or_get_constant(const Variant& p_value) {
    return get_constant_pos(p_value);
}

uint32_t OScriptBytecodeGenerator::add_or_get_name(const StringName& p_name) {
    return get_name_map_pos(p_name);
}

uint32_t OScriptBytecodeGenerator::add_temporary(const OScriptDataType& p_type) {
    Variant::Type temp_type = Variant::NIL;

    if (p_type.kind == OScriptDataType::BUILTIN) {
        switch (p_type.builtin_type) {
            case Variant::NIL:
            case Variant::BOOL:
            case Variant::INT:
            case Variant::FLOAT:
            case Variant::STRING:
            case Variant::VECTOR2:
            case Variant::VECTOR2I:
            case Variant::RECT2:
            case Variant::RECT2I:
            case Variant::VECTOR3:
            case Variant::VECTOR3I:
            case Variant::TRANSFORM2D:
            case Variant::VECTOR4:
            case Variant::VECTOR4I:
            case Variant::PLANE:
            case Variant::QUATERNION:
            case Variant::AABB:
            case Variant::BASIS:
            case Variant::TRANSFORM3D:
            case Variant::PROJECTION:
            case Variant::COLOR:
            case Variant::STRING_NAME:
            case Variant::NODE_PATH:
            case Variant::RID:
            case Variant::CALLABLE:
            case Variant::SIGNAL: {
                temp_type = p_type.builtin_type;
                break;
            }
            case Variant::OBJECT:
            case Variant::DICTIONARY:
            case Variant::ARRAY:
            case Variant::PACKED_BYTE_ARRAY:
            case Variant::PACKED_INT32_ARRAY:
            case Variant::PACKED_INT64_ARRAY:
            case Variant::PACKED_FLOAT32_ARRAY:
            case Variant::PACKED_FLOAT64_ARRAY:
            case Variant::PACKED_STRING_ARRAY:
            case Variant::PACKED_VECTOR2_ARRAY:
            case Variant::PACKED_VECTOR3_ARRAY:
            case Variant::PACKED_COLOR_ARRAY:
            case Variant::PACKED_VECTOR4_ARRAY:
            case Variant::VARIANT_MAX: {
                // Arrays, dictionaries, and objects are reference counted, so we don't use the pool for them.
                temp_type = Variant::NIL;
                break;
            }
        }
    }

    if (!temporaries_pool.has(temp_type)) {
        temporaries_pool[temp_type] = List<int>();
    }

    List<int>& pool = temporaries_pool[temp_type];
    if (pool.is_empty()) {
        StackSlot new_temp(temp_type, p_type.can_contain_object());
        int idx = temporaries.size();
        pool.push_back(idx);
        temporaries.push_back(new_temp);
    }

    const int slot = pool.front()->get();
    pool.pop_front();
    used_temporaries.push_back(slot);
    return slot;
}

void OScriptBytecodeGenerator::pop_temporary() {
    ERR_FAIL_COND(used_temporaries.is_empty());

    const int slot_idx = used_temporaries.back()->get();
    if (temporaries[slot_idx].can_contain_object) {
        // Avoid keeping in the stack long-lived references to objects,
        // which may prevent `RefCounted` objects from being freed.
        // However, the cleanup will be performed an the end of the
        // statement, to allow object references to survive chaining.
        temporaries_pending_clear.insert(slot_idx);
    }

    temporaries_pool[temporaries[slot_idx].type].push_back(slot_idx);
    used_temporaries.pop_back();
}

void OScriptBytecodeGenerator::clear_temporaries() {
    for (int slot_idx : temporaries_pending_clear) {
        // The temporary may have been reused as something else since it was added to the list.
        // In that case, there's **no** need to clear it.
        if (temporaries[slot_idx].can_contain_object) {
            clear_address(Address(Address::TEMPORARY, slot_idx)); // Can contain `RefCounted`, so clear it.
        }
    }
    temporaries_pending_clear.clear();
}

void OScriptBytecodeGenerator::clear_address(const Address& p_address) {
    // Do not check `is_local_dirty()` here! Always clear the address since the codegen doesn't track the compiler.
    // Also, this method is used to initialize local variables of built-in types, since they cannot be `null`.

    if (p_address.type.kind == OScriptDataType::BUILTIN) {
        switch (p_address.type.builtin_type) {
            case Variant::BOOL: {
                write_assign_false(p_address);
                break;
            }
            case Variant::DICTIONARY: {
                if (p_address.type.has_container_element_types()) {
                    write_construct_typed_dictionary(p_address, p_address.type.get_container_element_type_or_variant(0), p_address.type.get_container_element_type_or_variant(1), Vector<Address>());
                } else {
                    write_construct(p_address, p_address.type.builtin_type, Vector<Address>());
                }
                break;
            }
            case Variant::ARRAY: {
                if (p_address.type.has_container_element_type(0)) {
                    write_construct_typed_array(p_address, p_address.type.get_container_element_type(0), Vector<Address>());
                } else {
                    write_construct(p_address, p_address.type.builtin_type, Vector<Address>());
                }
                break;
            }
            case Variant::NIL:
            case Variant::OBJECT: {
                write_assign_null(p_address);
                break;
            }
            default: {
                write_construct(p_address, p_address.type.builtin_type, Vector<Address>());
                break;
            }
        }
    } else {
        write_assign_null(p_address);
    }

    if (p_address.mode == Address::LOCAL_VARIABLE) {
        dirty_locals.erase(p_address.address);
    }
}

// Returns `true` if the local has been reused and not cleaned up with `clear_address()`.
bool OScriptBytecodeGenerator::is_local_dirty(const Address& p_address) {
    ERR_FAIL_COND_V(p_address.mode != Address::LOCAL_VARIABLE, false);
    return dirty_locals.has(p_address.address);
}

void OScriptBytecodeGenerator::start_parameters() {
    if (function->default_arg_count > 0) {
        append(OScriptCompiledFunction::OPCODE_JUMP_TO_DEF_ARGUMENT);
        function->default_arguments.push_back(opcodes.size());
    }
}

void OScriptBytecodeGenerator::end_parameters() {
    function->default_arguments.reverse();
}

void OScriptBytecodeGenerator::start_block() {
    push_stack_identifiers();
}

void OScriptBytecodeGenerator::end_block() {
    pop_stack_identifiers();
}

void OScriptBytecodeGenerator::write_start(OScript* p_script, const StringName& p_function_name, bool p_static, Variant p_rpc_config, const OScriptDataType& p_return_type) {
    function = memnew(OScriptCompiledFunction);

    function->name = p_function_name;
    function->_script = p_script;
    function->source = p_script->get_script_path();

    #ifdef DEBUG_ENABLED
    function->func_cname = (String(function->source) + " - " + String(p_function_name)).utf8();
    function->_func_cname = function->func_cname.get_data();
    #endif

    function->_static = p_static;
    function->return_type = p_return_type;
    function->rpc_config = p_rpc_config;
    function->argument_count = 0;
}

OScriptCompiledFunction* OScriptBytecodeGenerator::write_end() {
    #ifdef DEBUG_ENABLED
	if (!used_temporaries.is_empty()) {
		ERR_PRINT("Non-zero temporary variables at end of function: " + itos(used_temporaries.size()));
	}
    #endif
	append_opcode(OScriptCompiledFunction::OPCODE_END);

	for (int i = 0; i < temporaries.size(); i++) {
		int stack_index = i + max_locals + OScriptCompiledFunction::FIXED_ADDRESSES_MAX;
		for (int j = 0; j < temporaries[i].bytecode_indices.size(); j++) {
			opcodes.write[temporaries[i].bytecode_indices[j]] = stack_index | (OScriptCompiledFunction::ADDR_TYPE_STACK << OScriptCompiledFunction::ADDR_BITS);
		}
		if (temporaries[i].type != Variant::NIL) {
			function->temporary_slots[stack_index] = temporaries[i].type;
		}
	}

	if (constant_map.size()) {
		function->constant_count = constant_map.size();
		function->constants.resize(constant_map.size());
		function->constants_ptr = function->constants.ptrw();
		for (const KeyValue<Variant, int>& K : constant_map) {
			function->constants.write[K.value] = K.key;
		}
	} else {
		function->constants_ptr = nullptr;
		function->constant_count = 0;
	}

	if (name_map.size()) {
		function->global_names.resize(name_map.size());
		function->global_names_ptr = &function->global_names[0];
		for (const KeyValue<StringName, int>& E : name_map) {
			function->global_names.write[E.value] = E.key;
		}
		function->global_names_count = function->global_names.size();

	} else {
		function->global_names_ptr = nullptr;
		function->global_names_count = 0;
	}

	if (opcodes.size()) {
		function->code = opcodes;
		function->code_ptr = &function->code.write[0];
		function->code_size = opcodes.size();

	} else {
		function->code_ptr = nullptr;
		function->code_size = 0;
	}

	if (function->default_arguments.size()) {
		function->default_arg_count = function->default_arguments.size() - 1;
		function->default_arg_ptr = &function->default_arguments[0];
	} else {
		function->default_arg_count = 0;
		function->default_arg_ptr = nullptr;
	}

	if (operator_func_map.size()) {
		function->operator_funcs.resize(operator_func_map.size());
		function->operator_funcs_count = function->operator_funcs.size();
		function->operator_funcs_ptr = function->operator_funcs.ptr();
		for (const KeyValue<GDExtensionPtrOperatorEvaluator, int>& E : operator_func_map) {
			function->operator_funcs.write[E.value] = E.key;
		}
	} else {
		function->operator_funcs_count = 0;
		function->operator_funcs_ptr = nullptr;
	}

	if (setters_map.size()) {
		function->setters.resize(setters_map.size());
		function->setters_count = function->setters.size();
		function->setters_ptr = function->setters.ptr();
		for (const KeyValue<GDExtensionPtrSetter, int>& E : setters_map) {
			function->setters.write[E.value] = E.key;
		}
	} else {
		function->setters_count = 0;
		function->setters_ptr = nullptr;
	}

	if (getters_map.size()) {
		function->getters.resize(getters_map.size());
		function->getters_count = function->getters.size();
		function->getters_ptr = function->getters.ptr();
		for (const KeyValue<GDExtensionPtrGetter, int>& E : getters_map) {
			function->getters.write[E.value] = E.key;
		}
	} else {
		function->getters_count = 0;
		function->getters_ptr = nullptr;
	}

	if (keyed_setters_map.size()) {
		function->keyed_setters.resize(keyed_setters_map.size());
		function->keyed_setters_count = function->keyed_setters.size();
		function->keyed_setters_ptr = function->keyed_setters.ptr();
		for (const KeyValue<GDExtensionPtrKeyedSetter, int>& E : keyed_setters_map) {
			function->keyed_setters.write[E.value] = E.key;
		}
	} else {
		function->keyed_setters_count = 0;
		function->keyed_setters_ptr = nullptr;
	}

	if (keyed_getters_map.size()) {
		function->keyed_getters.resize(keyed_getters_map.size());
		function->keyed_getters_count = function->keyed_getters.size();
		function->keyed_getters_ptr = function->keyed_getters.ptr();
		for (const KeyValue<GDExtensionPtrKeyedGetter, int>& E : keyed_getters_map) {
			function->keyed_getters.write[E.value] = E.key;
		}
	} else {
		function->keyed_getters_count = 0;
		function->keyed_getters_ptr = nullptr;
	}

	if (indexed_setters_map.size()) {
		function->indexed_setters.resize(indexed_setters_map.size());
		function->indexed_setters_count = function->indexed_setters.size();
		function->indexed_setters_ptr = function->indexed_setters.ptr();
		for (const KeyValue<GDExtensionPtrIndexedSetter, int>& E : indexed_setters_map) {
			function->indexed_setters.write[E.value] = E.key;
		}
	} else {
		function->indexed_setters_count = 0;
		function->indexed_setters_ptr = nullptr;
	}

	if (indexed_getters_map.size()) {
		function->indexed_getters.resize(indexed_getters_map.size());
		function->indexed_getters_count = function->indexed_getters.size();
		function->indexed_getters_ptr = function->indexed_getters.ptr();
		for (const KeyValue<GDExtensionPtrIndexedGetter, int>& E : indexed_getters_map) {
			function->indexed_getters.write[E.value] = E.key;
		}
	} else {
		function->indexed_getters_count = 0;
		function->indexed_getters_ptr = nullptr;
	}

	if (built_in_methods_map.size()) {
		function->builtin_methods.resize(built_in_methods_map.size());
		function->builtin_methods_ptr = function->builtin_methods.ptr();
		function->builtin_methods_count = built_in_methods_map.size();
		for (const KeyValue<GDExtensionPtrBuiltInMethod, int>& E : built_in_methods_map) {
			function->builtin_methods.write[E.value] = E.key;
		}
	} else {
		function->builtin_methods_ptr = nullptr;
		function->builtin_methods_count = 0;
	}

	if (constructors_map.size()) {
		function->constructors.resize(constructors_map.size());
		function->constructors_ptr = function->constructors.ptr();
		function->constructors_count = constructors_map.size();
		for (const KeyValue<GDExtensionPtrConstructor, int>& E : constructors_map) {
			function->constructors.write[E.value] = E.key;
		}
	} else {
		function->constructors_ptr = nullptr;
		function->constructors_count = 0;
	}

	if (utility_functions_map.size()) {
		function->utilities.resize(utility_functions_map.size());
		function->utilities_ptr = function->utilities.ptr();
		function->utilities_count = utility_functions_map.size();
		for (const KeyValue<GDExtensionPtrUtilityFunction, int>& E : utility_functions_map) {
			function->utilities.write[E.value] = E.key;
		}
	} else {
		function->utilities_ptr = nullptr;
		function->utilities_count = 0;
	}

	if (os_functions_map.size()) {
		function->os_utilities.resize(os_functions_map.size());
		function->os_utilities_ptr = function->os_utilities.ptr();
		function->os_utilities_count = os_functions_map.size();
		for (const KeyValue<OScriptUtilityFunctions::FunctionPtr, int>& E : os_functions_map) {
			function->os_utilities.write[E.value] = E.key;
		}
	} else {
		function->os_utilities_ptr = nullptr;
		function->os_utilities_count = 0;
	}

	if (method_bind_map.size()) {
		function->methods.resize(method_bind_map.size());
		function->methods_ptr = function->methods.ptrw();
		function->methods_count = method_bind_map.size();
		for (const KeyValue<MethodBind *, int>& E : method_bind_map) {
			function->methods.write[E.value] = E.key;
		}
	} else {
		function->methods_ptr = nullptr;
		function->methods_count = 0;
	}

	if (lambdas_map.size()) {
		function->lambdas.resize(lambdas_map.size());
		function->_lambdas_ptr = function->lambdas.ptrw();
		function->lambdas_count = lambdas_map.size();
		for (const KeyValue<OScriptCompiledFunction*, int>& E : lambdas_map) {
			function->lambdas.write[E.value] = E.key;
		}
	} else {
		function->_lambdas_ptr = nullptr;
		function->lambdas_count = 0;
	}

	if (OScriptLanguage::get_singleton()->should_track_locals()) {
		function->stack_debug = stack_debug;
	}
	function->stack_size = OScriptCompiledFunction::FIXED_ADDRESSES_MAX + max_locals + temporaries.size();
	function->instruction_arg_size = instr_args_max;

    #ifdef DEBUG_ENABLED
	function->operator_names = operator_names;
	function->setter_names = setter_names;
	function->getter_names = getter_names;
	function->builtin_methods_names = builtin_methods_names;
	function->constructors_names = constructors_names;
	function->utilities_names = utilities_names;
	function->os_utilities_names = os_utilities_names;
    #endif

	ended = true;
	return function;
}


#ifdef DEBUG_ENABLED
void OScriptBytecodeGenerator::set_signature(const String& p_signature) {
    function->profile.signature = p_signature;
}
#endif

void OScriptBytecodeGenerator::set_initial_node_id(int p_node_id) {
    function->initial_node = p_node_id;
}

void OScriptBytecodeGenerator::write_type_adjust(const Address& p_target, Variant::Type p_new_type) {
    switch (p_new_type) {
		case Variant::BOOL: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_BOOL);
			break;
		}
		case Variant::INT: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_INT);
			break;
		}
		case Variant::FLOAT: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_FLOAT);
			break;
		}
		case Variant::STRING: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_STRING);
			break;
		}
		case Variant::VECTOR2: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_VECTOR2);
			break;
		}
		case Variant::VECTOR2I: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_VECTOR2I);
			break;
		}
		case Variant::RECT2: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_RECT2);
			break;
		}
		case Variant::RECT2I: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_RECT2I);
			break;
		}
		case Variant::VECTOR3: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_VECTOR3);
			break;
		}
		case Variant::VECTOR3I: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_VECTOR3I);
			break;
		}
		case Variant::TRANSFORM2D: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_TRANSFORM2D);
			break;
		}
		case Variant::VECTOR4: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_VECTOR3);
			break;
		}
		case Variant::VECTOR4I: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_VECTOR3I);
			break;
		}
		case Variant::PLANE: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_PLANE);
			break;
		}
		case Variant::QUATERNION: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_QUATERNION);
			break;
		}
		case Variant::AABB: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_AABB);
			break;
		}
		case Variant::BASIS: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_BASIS);
			break;
		}
		case Variant::TRANSFORM3D: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_TRANSFORM3D);
			break;
		}
		case Variant::PROJECTION: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_PROJECTION);
			break;
		}
		case Variant::COLOR: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_COLOR);
			break;
		}
		case Variant::STRING_NAME: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_STRING_NAME);
			break;
		}
		case Variant::NODE_PATH: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_NODE_PATH);
			break;
		}
		case Variant::RID: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_RID);
			break;
		}
		case Variant::OBJECT: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_OBJECT);
			break;
		}
		case Variant::CALLABLE: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_CALLABLE);
			break;
		}
		case Variant::SIGNAL: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_SIGNAL);
			break;
		}
		case Variant::DICTIONARY: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_DICTIONARY);
			break;
		}
		case Variant::ARRAY: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_ARRAY);
			break;
		}
		case Variant::PACKED_BYTE_ARRAY: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_PACKED_BYTE_ARRAY);
			break;
		}
		case Variant::PACKED_INT32_ARRAY: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_PACKED_INT32_ARRAY);
			break;
		}
		case Variant::PACKED_INT64_ARRAY: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_PACKED_INT64_ARRAY);
			break;
		}
		case Variant::PACKED_FLOAT32_ARRAY: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_PACKED_FLOAT32_ARRAY);
			break;
		}
		case Variant::PACKED_FLOAT64_ARRAY: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_PACKED_FLOAT64_ARRAY);
			break;
		}
		case Variant::PACKED_STRING_ARRAY: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_PACKED_STRING_ARRAY);
			break;
		}
		case Variant::PACKED_VECTOR2_ARRAY: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_PACKED_VECTOR2_ARRAY);
			break;
		}
		case Variant::PACKED_VECTOR3_ARRAY: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_PACKED_VECTOR3_ARRAY);
			break;
		}
		case Variant::PACKED_COLOR_ARRAY: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_PACKED_COLOR_ARRAY);
			break;
		}
		case Variant::PACKED_VECTOR4_ARRAY: {
		    append_opcode(OScriptCompiledFunction::OPCODE_TYPE_ADJUST_PACKED_VECTOR4_ARRAY);
		    break;
		}
		case Variant::NIL:
		case Variant::VARIANT_MAX: {
		    return;
		}
	}
	append(p_target);
}

void OScriptBytecodeGenerator::write_unary_operator(const Address& p_target, Variant::Operator p_operator, const Address& p_operand) {
    if (HAS_BUILTIN_TYPE(p_operand)) {
        // todo: add back in
        // GDExtensionPtrOperatorEvaluator op_func = VariantUtils::get_validated_operator_evaluator(p_operator, p_operand.type.builtin_type, Variant::NIL);
        // append_opcode(OScriptCompiledFunction::OPCODE_OPERATOR_VALIDATED);
        // append(p_operand);
        // append(Address());
        // append(p_target);
        // append_op_eval(op_func);
        // #ifdef DEBUG_ENABLED
        // add_debug_name(operator_names, get_operation_pos(op_func), VariantUtils::get_operator_name(p_operator));
        // #endif
        // return;
    }

    // todo:
    //  This is intentionally used to short-circuit the logic below.
    //  See script_vm OPCODE_OPERATOR_EVALUATE opcode handler for details why.
    if (true) {
        append_opcode(OScriptCompiledFunction::OPCODE_OPERATOR_EVALUATE);
        append(p_operand);
        append(Address());
        append(p_target);
        append(p_operator);
        #ifdef DEBUG_ENABLED
        add_debug_name(operator_names, p_operator, GDE::Variant::get_operator_name(p_operator));
        #endif
        return;
    }

    // No specific types, perform variant evaluation.
    append_opcode(OScriptCompiledFunction::OPCODE_OPERATOR);
    append(p_operand);
    append(Address());
    append(p_target);
    append(p_operator);
    append(0); // Signature storage.
    append(0); // Return type storage.
    constexpr int _pointer_size = sizeof(GDExtensionPtrOperatorEvaluator) / sizeof(*(opcodes.ptr()));
    for (int i = 0; i < _pointer_size; i++) {
        append(0); // Space for function pointer.
    }
}

void OScriptBytecodeGenerator::write_binary_operator(const Address& p_target, Variant::Operator p_operator, const Address& p_left, const Address& p_right) {
    bool valid = HAS_BUILTIN_TYPE(p_left) && HAS_BUILTIN_TYPE(p_right);

	// Avoid validated evaluator for modulo and division when operands are int or integer vector, since there's no check for division by zero.
	if (valid && (p_operator == Variant::OP_DIVIDE || p_operator == Variant::OP_MODULE)) {
		switch (p_left.type.builtin_type) {
			case Variant::INT: {
			    // Cannot use modulo between int / float, we should raise an error later in GDScript
			    valid = p_right.type.builtin_type != Variant::INT && p_operator == Variant::OP_DIVIDE;
			    break;
			}
			case Variant::VECTOR2I:
			case Variant::VECTOR3I:
			case Variant::VECTOR4I: {
			    valid = p_right.type.builtin_type != Variant::INT && p_right.type.builtin_type != p_left.type.builtin_type;
			    break;
			}
			default: {
			    break;
			}
		}
	}

    // todo:
    //  This is intentionally used to short-circuit the logic below.
    //  See script_vm OPCODE_OPERATOR_EVALUATE opcode handler for details why.
    if (true) {
        append_opcode(OScriptCompiledFunction::OPCODE_OPERATOR_EVALUATE);
        append(p_left);
        append(p_right);
        append(p_target);
        append(p_operator);
        #ifdef DEBUG_ENABLED
        add_debug_name(operator_names, p_operator, GDE::Variant::get_operator_name(p_operator));
        #endif
        return;
    }

	if (valid) {
		if (p_target.mode == Address::TEMPORARY) {
			Variant::Type result_type = GDE::Variant::get_operator_return_type(p_operator, p_left.type.builtin_type, p_right.type.builtin_type);
			Variant::Type temp_type = temporaries[p_target.address].type;
			if (result_type != temp_type) {
				write_type_adjust(p_target, result_type);
			}
		}

		// Gather specific operator.
	 //    GDExtensionPtrOperatorEvaluator op_func = VariantUtils::get_validated_operator_evaluator(p_operator, p_left.type.builtin_type, p_right.type.builtin_type);
		// append_opcode(OScriptCompiledFunction::OPCODE_OPERATOR_VALIDATED);
		// append(p_left);
		// append(p_right);
		// append(p_target);
		// append_op_eval(op_func);
	 //    UtilityFunctions::print("Using OPCODE_OPERATOR_VALIDATED for Operation ", p_operator, " with lhs=", p_left.type.builtin_type, " and rhs=", p_right.type.builtin_type);
  //       #ifdef DEBUG_ENABLED
		// add_debug_name(operator_names, get_operation_pos(op_func), VariantUtils::get_operator_name(p_operator));
  //       #endif
		// return;
	}

	// No specific types, perform variant evaluation.
	append_opcode(OScriptCompiledFunction::OPCODE_OPERATOR);
	append(p_left);
	append(p_right);
	append(p_target);
	append(p_operator);
	append(0); // Signature storage.
	append(0); // Return type storage.

	constexpr int _pointer_size = sizeof(GDExtensionPtrOperatorEvaluator) / sizeof(*(opcodes.ptr()));
	for (int i = 0; i < _pointer_size; i++) {
		append(0); // Space for function pointer.
	}
}

void OScriptBytecodeGenerator::write_type_test(const Address& p_target, const Address& p_source, const OScriptDataType& p_type) {
    switch (p_type.kind) {
		case OScriptDataType::BUILTIN: {
			if (p_type.builtin_type == Variant::ARRAY && p_type.has_container_element_type(0)) {
				const OScriptDataType &element_type = p_type.get_container_element_type(0);
				append_opcode(OScriptCompiledFunction::OPCODE_TYPE_TEST_ARRAY);
				append(p_target);
				append(p_source);
				append(get_constant_pos(element_type.script_type) | (OScriptCompiledFunction::ADDR_TYPE_CONSTANT << OScriptCompiledFunction::ADDR_BITS));
				append(element_type.builtin_type);
				append(element_type.native_type);
			} else if (p_type.builtin_type == Variant::DICTIONARY && p_type.has_container_element_types()) {
				const OScriptDataType &key_element_type = p_type.get_container_element_type_or_variant(0);
				const OScriptDataType &value_element_type = p_type.get_container_element_type_or_variant(1);
				append_opcode(OScriptCompiledFunction::OPCODE_TYPE_TEST_DICTIONARY);
				append(p_target);
				append(p_source);
				append(get_constant_pos(key_element_type.script_type) | (OScriptCompiledFunction::ADDR_TYPE_CONSTANT << OScriptCompiledFunction::ADDR_BITS));
				append(get_constant_pos(value_element_type.script_type) | (OScriptCompiledFunction::ADDR_TYPE_CONSTANT << OScriptCompiledFunction::ADDR_BITS));
				append(key_element_type.builtin_type);
				append(key_element_type.native_type);
				append(value_element_type.builtin_type);
				append(value_element_type.native_type);
			} else {
				append_opcode(OScriptCompiledFunction::OPCODE_TYPE_TEST_BUILTIN);
				append(p_target);
				append(p_source);
				append(p_type.builtin_type);
			}
		} break;

		case OScriptDataType::NATIVE: {
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_TEST_NATIVE);
			append(p_target);
			append(p_source);
			append(p_type.native_type);
		} break;

		case OScriptDataType::SCRIPT:
		case OScriptDataType::OSCRIPT: {
			const Variant &script = p_type.script_type;
			append_opcode(OScriptCompiledFunction::OPCODE_TYPE_TEST_SCRIPT);
			append(p_target);
			append(p_source);
			append(get_constant_pos(script) | (OScriptCompiledFunction::ADDR_TYPE_CONSTANT << OScriptCompiledFunction::ADDR_BITS));
		} break;

		default: {
			ERR_PRINT("Compiler bug: unresolved type in type test.");
			append_opcode(OScriptCompiledFunction::OPCODE_ASSIGN_FALSE);
			append(p_target);
		}
	}
}

void OScriptBytecodeGenerator::write_and_left_operand(const Address& p_left_operand) {
    append_opcode(OScriptCompiledFunction::OPCODE_JUMP_IF_NOT);
    append(p_left_operand);
    logic_op_jump_pos1.push_back(opcodes.size());
    append(0); // Jump target, will be patched.
}

void OScriptBytecodeGenerator::write_and_right_operand(const Address& p_right_operand) {
    append_opcode(OScriptCompiledFunction::OPCODE_JUMP_IF_NOT);
    append(p_right_operand);
    logic_op_jump_pos2.push_back(opcodes.size());
    append(0); // Jump target, will be patched.
}

void OScriptBytecodeGenerator::write_end_and(const Address& p_target) {
    // If here means both operands are true.
    append_opcode(OScriptCompiledFunction::OPCODE_ASSIGN_TRUE);
    append(p_target);

    // Jump away from the fail condition.
    append_opcode(OScriptCompiledFunction::OPCODE_JUMP);
    append(opcodes.size() + 3);

    // Here it means one of operands is false.
    patch_jump(logic_op_jump_pos1.back()->get());
    patch_jump(logic_op_jump_pos2.back()->get());
    logic_op_jump_pos1.pop_back();
    logic_op_jump_pos2.pop_back();

    append_opcode(OScriptCompiledFunction::OPCODE_ASSIGN_FALSE);
    append(p_target);
}

void OScriptBytecodeGenerator::write_or_left_operand(const Address& p_left_operand) {
    append_opcode(OScriptCompiledFunction::OPCODE_JUMP_IF);
    append(p_left_operand);
    logic_op_jump_pos1.push_back(opcodes.size());
    append(0); // Jump target, will be patched.
}

void OScriptBytecodeGenerator::write_or_right_operand(const Address& p_right_operand) {
    append_opcode(OScriptCompiledFunction::OPCODE_JUMP_IF);
    append(p_right_operand);
    logic_op_jump_pos2.push_back(opcodes.size());
    append(0); // Jump target, will be patched.
}

void OScriptBytecodeGenerator::write_end_or(const Address& p_target) {
    // If here means both operands are false.
    append_opcode(OScriptCompiledFunction::OPCODE_ASSIGN_FALSE);
    append(p_target);

    // Jump away from the success condition.
    append_opcode(OScriptCompiledFunction::OPCODE_JUMP);
    append(opcodes.size() + 3);

    // Here it means one of operands is true.
    patch_jump(logic_op_jump_pos1.back()->get());
    patch_jump(logic_op_jump_pos2.back()->get());
    logic_op_jump_pos1.pop_back();
    logic_op_jump_pos2.pop_back();

    append_opcode(OScriptCompiledFunction::OPCODE_ASSIGN_TRUE);
    append(p_target);
}

void OScriptBytecodeGenerator::write_start_ternary(const Address& p_target) {
    ternary_result.push_back(p_target);
}

void OScriptBytecodeGenerator::write_ternary_condition(const Address& p_condition) {
    append_opcode(OScriptCompiledFunction::OPCODE_JUMP_IF_NOT);
    append(p_condition);
    ternary_jump_fail_pos.push_back(opcodes.size());
    append(0); // Jump target, will be patched.
}

void OScriptBytecodeGenerator::write_ternary_true_expr(const Address& p_expr) {
    append_opcode(OScriptCompiledFunction::OPCODE_ASSIGN);
    append(ternary_result.back()->get());
    append(p_expr);

    // Jump away from the false path.
    append_opcode(OScriptCompiledFunction::OPCODE_JUMP);
    ternary_jump_skip_pos.push_back(opcodes.size());
    append(0);

    // Fail must jump here.
    patch_jump(ternary_jump_fail_pos.back()->get());
    ternary_jump_fail_pos.pop_back();
}

void OScriptBytecodeGenerator::write_ternary_false_expr(const Address& p_expr) {
    append_opcode(OScriptCompiledFunction::OPCODE_ASSIGN);
    append(ternary_result.back()->get());
    append(p_expr);
}

void OScriptBytecodeGenerator::write_end_ternary() {
    patch_jump(ternary_jump_skip_pos.back()->get());
    ternary_jump_skip_pos.pop_back();
    ternary_result.pop_back();
}

void OScriptBytecodeGenerator::write_set(const Address& p_target, const Address& p_index, const Address& p_source) {
    if (HAS_BUILTIN_TYPE(p_target)) {
        // todo: add back in
        // if (IS_BUILTIN_TYPE(p_index, Variant::INT)) {
        //     GDExtensionPtrIndexedSetter indexed_setter = VariantUtils::get_member_validated_indexed_setter(p_target.type.builtin_type);
        //     if (indexed_setter) {
        //         const BuiltInType built_in_type = ExtensionDB::get_builtin_type(p_target.type.builtin_type);
        //         const Variant::Type index_type = built_in_type.index_returning_type;
        //         if (IS_BUILTIN_TYPE(p_source, index_type)) {
        //             // Use indexed setter instead.
        //             append_opcode(OScriptCompiledFunction::OPCODE_SET_INDEXED_VALIDATED);
        //             append(p_target);
        //             append(p_index);
        //             append(p_source);
        //             append(indexed_setter);
        //             return;
        //         }
        //     }
        //
        //     GDExtensionPtrKeyedSetter keyed_setter = VariantUtils::get_member_validated_keyed_setter(p_target.type.builtin_type);
        //     if (keyed_setter) {
        //         append_opcode(OScriptCompiledFunction::OPCODE_SET_KEYED_VALIDATED);
        //         append(p_target);
        //         append(p_index);
        //         append(p_source);
        //         append(keyed_setter);
        //         return;
        //     }
        // }
    }

    append_opcode(OScriptCompiledFunction::OPCODE_SET_KEYED);
    append(p_target);
    append(p_index);
    append(p_source);
}

void OScriptBytecodeGenerator::write_get(const Address& p_target, const Address& p_index, const Address& p_source) {
    if (HAS_BUILTIN_TYPE(p_source)) {
        // todo: add back in
        // if (IS_BUILTIN_TYPE(p_index, Variant::INT)) {
        //     GDExtensionPtrIndexedGetter indexed_getter = VariantUtils::get_member_validated_indexed_getter(p_source.type.builtin_type);
        //     if (indexed_getter) {
        //         append_opcode(OScriptCompiledFunction::OPCODE_GET_INDEXED_VALIDATED);
        //         append(p_source);
        //         append(p_index);
        //         append(p_target);
        //         append(indexed_getter);
        //         return;
        //     }
        // }
        //
        // GDExtensionPtrKeyedGetter keyed_getter = VariantUtils::get_member_validated_keyed_getter(p_source.type.builtin_type);
        // if (keyed_getter) {
        //     append_opcode(OScriptCompiledFunction::OPCODE_GET_KEYED_VALIDATED);
        //     append(p_source);
        //     append(p_index);
        //     append(p_target);
        //     append(keyed_getter);
        //     return;
        // }
    }

    append_opcode(OScriptCompiledFunction::OPCODE_GET_KEYED);
    append(p_source);
    append(p_index);
    append(p_target);
}

void OScriptBytecodeGenerator::write_set_named(const Address& p_target, const StringName& p_name, const Address& p_source) {
    if (HAS_BUILTIN_TYPE(p_target)) {
        // todo: add back in
        // GDExtensionPtrSetter setter = VariantUtils::get_member_validated_setter(p_target.type.builtin_type, p_name);
        // if (setter && IS_BUILTIN_TYPE(p_source, VariantUtils::get_member_type(p_target.type.builtin_type, p_name))) {
        //     append_opcode(OScriptCompiledFunction::OPCODE_SET_NAMED_VALIDATED);
        //     append(p_target);
        //     append(p_source);
        //     append(setter);
        //     #ifdef DEBUG_ENABLED
        //     add_debug_name(setter_names, get_setter_pos(setter), p_name);
        //     #endif
        //     return;
        // }
    }

    append_opcode(OScriptCompiledFunction::OPCODE_SET_NAMED);
    append(p_target);
    append(p_source);
    append(p_name);
}

void OScriptBytecodeGenerator::write_get_named(const Address& p_target, const StringName& p_name, const Address& p_source) {
    if (HAS_BUILTIN_TYPE(p_source)) {
        // todo: add validated back in after determining why this fails in GDE
        // GDExtensionPtrGetter getter = VariantUtils::get_member_validated_getter(p_source.type.builtin_type, p_name);
        // if (getter) {
        //     append_opcode(OScriptCompiledFunction::OPCODE_GET_NAMED_VALIDATED);
        //     append(p_source);
        //     append(p_target);
        //     append(getter);
        //     #ifdef DEBUG_ENABLED
        //     add_debug_name(getter_names, get_getter_pos(getter), p_name);
        //     #endif
        //     return;
        // }
    }

    append_opcode(OScriptCompiledFunction::OPCODE_GET_NAMED);
    append(p_source);
    append(p_target);
    append(p_name);
}

void OScriptBytecodeGenerator::write_set_member(const Address& p_value, const StringName& p_name) {
    append_opcode(OScriptCompiledFunction::OPCODE_SET_MEMBER);
    append(p_value);
    append(p_name);
}

void OScriptBytecodeGenerator::write_get_member(const Address& p_target, const StringName& p_name) {
    append_opcode(OScriptCompiledFunction::OPCODE_GET_MEMBER);
    append(p_target);
    append(p_name);
}

void OScriptBytecodeGenerator::write_set_static_variable(const Address& p_value, const Address& p_class, int p_index) {
    append_opcode(OScriptCompiledFunction::OPCODE_SET_STATIC_VARIABLE);
    append(p_value);
    append(p_class);
    append(p_index);
}

void OScriptBytecodeGenerator::write_get_static_variable(const Address& p_target, const Address& p_class, int p_index) {
    append_opcode(OScriptCompiledFunction::OPCODE_GET_STATIC_VARIABLE);
    append(p_target);
    append(p_class);
    append(p_index);
}

void OScriptBytecodeGenerator::write_assign(const Address& p_target, const Address& p_source) {
    if (p_target.type.kind == OScriptDataType::BUILTIN && p_target.type.builtin_type == Variant::ARRAY && p_target.type.has_container_element_type(0)) {
        const OScriptDataType &element_type = p_target.type.get_container_element_type(0);
        append_opcode(OScriptCompiledFunction::OPCODE_ASSIGN_TYPED_ARRAY);
        append(p_target);
        append(p_source);
        append(get_constant_pos(element_type.script_type) | (OScriptCompiledFunction::ADDR_TYPE_CONSTANT << OScriptCompiledFunction::ADDR_BITS));
        append(element_type.builtin_type);
        append(element_type.native_type);
    } else if (p_target.type.kind == OScriptDataType::BUILTIN && p_target.type.builtin_type == Variant::DICTIONARY && p_target.type.has_container_element_types()) {
        const OScriptDataType &key_type = p_target.type.get_container_element_type_or_variant(0);
        const OScriptDataType &value_type = p_target.type.get_container_element_type_or_variant(1);
        append_opcode(OScriptCompiledFunction::OPCODE_ASSIGN_TYPED_DICTIONARY);
        append(p_target);
        append(p_source);
        append(get_constant_pos(key_type.script_type) | (OScriptCompiledFunction::ADDR_TYPE_CONSTANT << OScriptCompiledFunction::ADDR_BITS));
        append(get_constant_pos(value_type.script_type) | (OScriptCompiledFunction::ADDR_TYPE_CONSTANT << OScriptCompiledFunction::ADDR_BITS));
        append(key_type.builtin_type);
        append(key_type.native_type);
        append(value_type.builtin_type);
        append(value_type.native_type);
    } else if (p_target.type.kind == OScriptDataType::BUILTIN && p_source.type.kind == OScriptDataType::BUILTIN && p_target.type.builtin_type != p_source.type.builtin_type) {
        // Need conversion.
        append_opcode(OScriptCompiledFunction::OPCODE_ASSIGN_TYPED_BUILTIN);
        append(p_target);
        append(p_source);
        append(p_target.type.builtin_type);
    } else {
        append_opcode(OScriptCompiledFunction::OPCODE_ASSIGN);
        append(p_target);
        append(p_source);
    }
}

void OScriptBytecodeGenerator::write_assign_with_conversion(const Address& p_target, const Address& p_source) {
    switch (p_target.type.kind) {
		case OScriptDataType::BUILTIN: {
			if (p_target.type.builtin_type == Variant::ARRAY && p_target.type.has_container_element_type(0)) {
				const OScriptDataType &element_type = p_target.type.get_container_element_type(0);
				append_opcode(OScriptCompiledFunction::OPCODE_ASSIGN_TYPED_ARRAY);
				append(p_target);
				append(p_source);
				append(get_constant_pos(element_type.script_type) | (OScriptCompiledFunction::ADDR_TYPE_CONSTANT << OScriptCompiledFunction::ADDR_BITS));
				append(element_type.builtin_type);
				append(element_type.native_type);
			} else if (p_target.type.builtin_type == Variant::DICTIONARY && p_target.type.has_container_element_types()) {
				const OScriptDataType &key_type = p_target.type.get_container_element_type_or_variant(0);
				const OScriptDataType &value_type = p_target.type.get_container_element_type_or_variant(1);
				append_opcode(OScriptCompiledFunction::OPCODE_ASSIGN_TYPED_DICTIONARY);
				append(p_target);
				append(p_source);
				append(get_constant_pos(key_type.script_type) | (OScriptCompiledFunction::ADDR_TYPE_CONSTANT << OScriptCompiledFunction::ADDR_BITS));
				append(get_constant_pos(value_type.script_type) | (OScriptCompiledFunction::ADDR_TYPE_CONSTANT << OScriptCompiledFunction::ADDR_BITS));
				append(key_type.builtin_type);
				append(key_type.native_type);
				append(value_type.builtin_type);
				append(value_type.native_type);
			} else {
				append_opcode(OScriptCompiledFunction::OPCODE_ASSIGN_TYPED_BUILTIN);
				append(p_target);
				append(p_source);
				append(p_target.type.builtin_type);
			}
		    break;
		}
		case OScriptDataType::NATIVE: {
			int class_idx = OScriptLanguage::get_singleton()->get_global_map()[p_target.type.native_type];
			Variant nc = OScriptLanguage::get_singleton()->get_global_array()[class_idx];
			class_idx = get_constant_pos(nc) | (OScriptCompiledFunction::ADDR_TYPE_CONSTANT << OScriptCompiledFunction::ADDR_BITS);
			append_opcode(OScriptCompiledFunction::OPCODE_ASSIGN_TYPED_NATIVE);
			append(p_target);
			append(p_source);
			append(class_idx);
		    break;
		}
		case OScriptDataType::SCRIPT:
		case OScriptDataType::OSCRIPT: {
			Variant script = p_target.type.script_type;
			int idx = get_constant_pos(script) | (OScriptCompiledFunction::ADDR_TYPE_CONSTANT << OScriptCompiledFunction::ADDR_BITS);
			append_opcode(OScriptCompiledFunction::OPCODE_ASSIGN_TYPED_SCRIPT);
			append(p_target);
			append(p_source);
			append(idx);
		    break;
		}
		default: {
			ERR_PRINT("Compiler bug: unresolved assign.");
			// Shouldn't get here, but fail-safe to a regular assignment
			append_opcode(OScriptCompiledFunction::OPCODE_ASSIGN);
			append(p_target);
			append(p_source);
		    break;
		}
	}
}

void OScriptBytecodeGenerator::write_assign_null(const Address& p_target) {
    append_opcode(OScriptCompiledFunction::OPCODE_ASSIGN_NULL);
    append(p_target);
}

void OScriptBytecodeGenerator::write_assign_true(const Address& p_target) {
    append_opcode(OScriptCompiledFunction::OPCODE_ASSIGN_TRUE);
    append(p_target);
}

void OScriptBytecodeGenerator::write_assign_false(const Address& p_target) {
    append_opcode(OScriptCompiledFunction::OPCODE_ASSIGN_FALSE);
    append(p_target);
}

void OScriptBytecodeGenerator::write_assign_default_parameter(const Address& p_dst, const Address& p_src, bool p_use_conversion) {
    if (p_use_conversion) {
        write_assign_with_conversion(p_dst, p_src);
    } else {
        write_assign(p_dst, p_src);
    }
    function->default_arguments.push_back(opcodes.size());
}

void OScriptBytecodeGenerator::write_store_global(const Address& p_dest, int p_global_index) {
    append_opcode(OScriptCompiledFunction::OPCODE_STORE_GLOBAL);
    append(p_dest);
    append(p_global_index);
}

void OScriptBytecodeGenerator::write_store_named_global(const Address& p_dest, const StringName& p_global) {
    append_opcode(OScriptCompiledFunction::OPCODE_STORE_NAMED_GLOBAL);
    append(p_dest);
    append(p_global);
}

void OScriptBytecodeGenerator::write_cast(const Address& p_target, const Address& p_source, const OScriptDataType& p_type) {
    int index = 0;

    switch (p_type.kind) {
        case OScriptDataType::BUILTIN: {
            append_opcode(OScriptCompiledFunction::OPCODE_CAST_TO_BUILTIN);
            index = p_type.builtin_type;
            break;
        }
        case OScriptDataType::NATIVE: {
            int class_idx = OScriptLanguage::get_singleton()->get_global_map()[p_type.native_type];
            Variant nc = OScriptLanguage::get_singleton()->get_global_array()[class_idx];
            append_opcode(OScriptCompiledFunction::OPCODE_CAST_TO_NATIVE);
            index = get_constant_pos(nc) | (OScriptCompiledFunction::ADDR_TYPE_CONSTANT << OScriptCompiledFunction::ADDR_BITS);
            break;
        }
        case OScriptDataType::SCRIPT:
        case OScriptDataType::OSCRIPT: {
            Variant script = p_type.script_type;
            int idx = get_constant_pos(script) | (OScriptCompiledFunction::ADDR_TYPE_CONSTANT << OScriptCompiledFunction::ADDR_BITS);
            append_opcode(OScriptCompiledFunction::OPCODE_CAST_TO_SCRIPT);
            index = idx;
            break;
        }
        default: {
            return;
        }
    }

    append(p_source);
    append(p_target);
    append(index);
}

void OScriptBytecodeGenerator::write_call(const Address& p_target, const Address& p_base, const StringName& p_function_name, const Vector<Address>& p_arguments) {
    append_opcode_and_argcount(p_target.mode == Address::NIL ? OScriptCompiledFunction::OPCODE_CALL : OScriptCompiledFunction::OPCODE_CALL_RETURN, 2 + p_arguments.size());
    for (int i = 0; i < p_arguments.size(); i++) {
        append(p_arguments[i]);
    }
    append(p_base);

    CallTarget ct = get_call_target(p_target);
    append(ct.target);
    append(p_arguments.size());
    append(p_function_name);
    ct.cleanup();
}

void OScriptBytecodeGenerator::write_super_call(const Address& p_target, const StringName& p_function_name, const Vector<Address>& p_arguments) {
    append_opcode_and_argcount(OScriptCompiledFunction::OPCODE_CALL_SELF_BASE, 1 + p_arguments.size());
    for (int i = 0; i < p_arguments.size(); i++) {
        append(p_arguments[i]);
    }

    CallTarget ct = get_call_target(p_target);
    append(ct.target);
    append(p_arguments.size());
    append(p_function_name);
    ct.cleanup();
}

void OScriptBytecodeGenerator::write_call_async(const Address& p_target, const Address& p_base, const StringName& p_function_name, const Vector<Address>& p_arguments) {
    append_opcode_and_argcount(OScriptCompiledFunction::OPCODE_CALL_ASYNC, 2 + p_arguments.size());
    for (int i = 0; i < p_arguments.size(); i++) {
        append(p_arguments[i]);
    }
    append(p_base);

    CallTarget ct = get_call_target(p_target);
    append(ct.target);
    append(p_arguments.size());
    append(p_function_name);
    ct.cleanup();
}

void OScriptBytecodeGenerator::write_call_utility(const Address& p_target, const StringName& p_function, const Vector<Address>& p_arguments) {
    bool is_validated = true;

    const FunctionInfo& fi = ExtensionDB::get_utility_function(p_function);
    if (fi.is_vararg()) {
        is_validated = false; // Vararg needs runtime checks, can't use validated call.
    } else if (p_arguments.size() == fi.method.arguments.size()) {
        bool all_types_exact = true;
        for (int i = 0; i < p_arguments.size(); i++) {
            if (!IS_BUILTIN_TYPE(p_arguments[i], fi.method.arguments[i].type)) {
                all_types_exact = false;
                break;
            }
        }
        is_validated = all_types_exact;
    }

    // todo: fix
    // Using the validated method calls have pointer requirements, similar to the issue for why
    // we needed to use OPCODE_OPERATOR_EVALUATE new opcode.
    is_validated = false;

    if (is_validated) {
        Variant::Type result_type = MethodUtils::has_return_value(fi.method.return_val) ? fi.method.return_val.type : Variant::NIL;
        CallTarget ct = get_call_target(p_target, result_type);
        Variant::Type temp_type = temporaries[ct.target.address].type;
        if (result_type != temp_type) {
            write_type_adjust(ct.target, result_type);
        }

        append_opcode_and_argcount(OScriptCompiledFunction::OPCODE_CALL_UTILITY_VALIDATED, 1 + p_arguments.size());
        for (int i = 0; i < p_arguments.size(); i++) {
            append(p_arguments[i]);
        }

        GDExtensionPtrUtilityFunction func = internal::gdextension_interface_variant_get_ptr_utility_function(p_function._native_ptr(), fi.hash);

        append(ct.target);
        append(p_arguments.size());
        append(func);
        ct.cleanup();

        #ifdef DEBUG_ENABLED
        add_debug_name(utilities_names, get_utility_pos(func), p_function);
        #endif
    } else {
        append_opcode_and_argcount(OScriptCompiledFunction::OPCODE_CALL_UTILITY, 1 + p_arguments.size());
        for (int i = 0; i < p_arguments.size(); i++) {
            append(p_arguments[i]);
        }

        CallTarget ct = get_call_target(p_target);
        append(ct.target);
        append(p_arguments.size());
        append(p_function);
        ct.cleanup();
    }
}

void OScriptBytecodeGenerator::write_call_oscript_utility(const Address& p_target, const StringName& p_function, const Vector<Address>& p_arguments) {
    append_opcode_and_argcount(OScriptCompiledFunction::OPCODE_CALL_OSCRIPT_UTILITY, 1 + p_arguments.size());
    OScriptUtilityFunctions::FunctionPtr os_function = OScriptUtilityFunctions::get_function(p_function);
    for (int i = 0; i < p_arguments.size(); i++) {
        append(p_arguments[i]);
    }

    CallTarget ct = get_call_target(p_target);
    append(ct.target);
    append(p_arguments.size());
    append(os_function);
    ct.cleanup();

    #ifdef DEBUG_ENABLED
    add_debug_name(os_utilities_names, get_os_utility_pos(os_function), p_function);
    #endif
}

void OScriptBytecodeGenerator::write_call_builtin_type(const Address& p_target, const Address& p_base, Variant::Type p_type, const StringName& p_method, bool p_is_static, const Vector<Address>& p_arguments) {
    bool is_validated = false;

    const MethodInfo method = GDE::Variant::get_builtin_method(p_type, p_method);
    const int64_t hash = GDE::Variant::get_builtin_method_hash(p_type, p_method);
    if (method.flags & METHOD_FLAG_VARARG) {
        is_validated = false; // Vararg needs runtime checks, can't use validated call.
    } else if (p_arguments.size() == method.arguments.size()) {
        bool all_types_exact = true;
        for (int i = 0; i < p_arguments.size(); i++) {
            if (!IS_BUILTIN_TYPE(p_arguments[i], method.arguments[i].type)) {
                all_types_exact = false;
                break;
            }
        }
        is_validated = all_types_exact;
    }

    // todo: fix
    // Always force non-validated due to validation method call issues.
    is_validated = false;

    if (!is_validated) {
        // Perform regular call.
        if (p_is_static) {
            append_opcode_and_argcount(OScriptCompiledFunction::OPCODE_CALL_BUILTIN_STATIC, p_arguments.size() + 1);
            for (int i = 0; i < p_arguments.size(); i++) {
                append(p_arguments[i]);
            }

            CallTarget ct = get_call_target(p_target);
            append(ct.target);
            append(p_type);
            append(p_method);
            append(p_arguments.size());
            ct.cleanup();
        } else {
            write_call(p_target, p_base, p_method, p_arguments);
        }
        return;
    }

    Variant::Type result_type = method.return_val.type;
    CallTarget ct = get_call_target(p_target, result_type);
    Variant::Type temp_type = temporaries[ct.target.address].type;
    if (result_type != temp_type) {
        write_type_adjust(ct.target, result_type);
    }

    append_opcode_and_argcount(OScriptCompiledFunction::OPCODE_CALL_BUILTIN_TYPE_VALIDATED, 2 + p_arguments.size());
    for (int i = 0; i < p_arguments.size(); i++) {
        append(p_arguments[i]);
    }
    append(p_base);
    append(ct.target);
    append(p_arguments.size());

    GDExtensionVariantType type = static_cast<GDExtensionVariantType>(p_type);
    GDExtensionPtrBuiltInMethod ptr = internal::gdextension_interface_variant_get_ptr_builtin_method(type, p_method._native_ptr(), hash);
    append(ptr);
    ct.cleanup();

    #ifdef DEBUG_ENABLED
    add_debug_name(builtin_methods_names, get_builtin_method_pos(ptr), p_method);
    #endif
}

void OScriptBytecodeGenerator::write_call_builtin_type(const Address& p_target, const Address& p_base, Variant::Type p_type, const StringName& p_method, const Vector<Address>& p_arguments) {
    return write_call_builtin_type(p_target, p_base, p_type, p_method, false, p_arguments);
}

void OScriptBytecodeGenerator::write_call_builtin_type_static(const Address& p_target, Variant::Type p_type, const StringName& p_method, const Vector<Address>& p_arguments) {
    return write_call_builtin_type(p_target, Address(), p_type, p_method, true, p_arguments);
}

void OScriptBytecodeGenerator::write_call_native_static(const Address& p_target, const StringName& p_class, const StringName& p_method, const Vector<Address>& p_arguments) {
    MethodBind* method = ExtensionDB::get_method(p_class, p_method);

    // Perform regular call.
    append_opcode_and_argcount(OScriptCompiledFunction::OPCODE_CALL_NATIVE_STATIC, p_arguments.size() + 1);
    for (int i = 0; i < p_arguments.size(); i++) {
        append(p_arguments[i]);
    }

    CallTarget ct = get_call_target(p_target);
    append(ct.target);
    append(method);
    append(p_arguments.size());
    ct.cleanup();
}

void OScriptBytecodeGenerator::write_call_native_static_validated(const Address& p_target, MethodBind* p_method, const Vector<Address>& p_arguments) {
    Variant::Type return_type = Variant::NIL;
    bool has_return = p_method->has_return();

    if (has_return) {
        PropertyInfo return_info = GDE::MethodBind::get_return_info(p_method);
        return_type = return_info.type;
    }

    CallTarget ct = get_call_target(p_target, return_type);

    if (has_return) {
        Variant::Type temp_type = temporaries[ct.target.address].type;
        if (temp_type != return_type) {
            write_type_adjust(ct.target, return_type);
        }
    }

    OScriptCompiledFunction::Opcode code = p_method->has_return()
        ? OScriptCompiledFunction::OPCODE_CALL_NATIVE_STATIC_VALIDATED_RETURN
        : OScriptCompiledFunction::OPCODE_CALL_NATIVE_STATIC_VALIDATED_NO_RETURN;
    append_opcode_and_argcount(code, 1 + p_arguments.size());

    for (int i = 0; i < p_arguments.size(); i++) {
        append(p_arguments[i]);
    }
    append(ct.target);
    append(p_arguments.size());
    append(p_method);
    ct.cleanup();
}

void OScriptBytecodeGenerator::write_call_method_bind(const Address& p_target, const Address& p_base, MethodBind* p_method, const Vector<Address>& p_arguments) {
    append_opcode_and_argcount(p_target.mode == Address::NIL ? OScriptCompiledFunction::OPCODE_CALL_METHOD_BIND : OScriptCompiledFunction::OPCODE_CALL_METHOD_BIND_RET, 2 + p_arguments.size());

    for (int i = 0; i < p_arguments.size(); i++) {
        append(p_arguments[i]);
    }

    CallTarget ct = get_call_target(p_target);
    append(p_base);
    append(ct.target);
    append(p_arguments.size());
    append(p_method);
    ct.cleanup();
}

void OScriptBytecodeGenerator::write_call_method_bind_validated(const Address& p_target, const Address& p_base, MethodBind* p_method, const Vector<Address>& p_arguments) {
    Variant::Type return_type = Variant::NIL;
    bool has_return = p_method->has_return();

    if (has_return) {
        PropertyInfo return_info = GDE::MethodBind::get_return_info(p_method);
        return_type = return_info.type;
    }

    CallTarget ct = get_call_target(p_target, return_type);

    if (has_return) {
        Variant::Type temp_type = temporaries[ct.target.address].type;
        if (temp_type != return_type) {
            write_type_adjust(ct.target, return_type);
        }
    }

    OScriptCompiledFunction::Opcode code = p_method->has_return()
        ? OScriptCompiledFunction::OPCODE_CALL_METHOD_BIND_VALIDATED_RETURN
        : OScriptCompiledFunction::OPCODE_CALL_METHOD_BIND_VALIDATED_NO_RETURN;

    append_opcode_and_argcount(code, 2 + p_arguments.size());

    for (int i = 0; i < p_arguments.size(); i++) {
        append(p_arguments[i]);
    }
    append(p_base);
    append(ct.target);
    append(p_arguments.size());
    append(p_method);
    ct.cleanup();
}

void OScriptBytecodeGenerator::write_call_self(const Address& p_target, const StringName& p_function_name, const Vector<Address>& p_arguments) {
    append_opcode_and_argcount(p_target.mode == Address::NIL ? OScriptCompiledFunction::OPCODE_CALL : OScriptCompiledFunction::OPCODE_CALL_RETURN, 2 + p_arguments.size());
    for (int i = 0; i < p_arguments.size(); i++) {
        append(p_arguments[i]);
    }
    append(OScriptCompiledFunction::ADDR_TYPE_STACK << OScriptCompiledFunction::ADDR_BITS);

    CallTarget ct = get_call_target(p_target);
    append(ct.target);
    append(p_arguments.size());
    append(p_function_name);
    ct.cleanup();
}

void OScriptBytecodeGenerator::write_call_self_async(const Address& p_target, const StringName& p_function_name, const Vector<Address>& p_arguments) {
    append_opcode_and_argcount(OScriptCompiledFunction::OPCODE_CALL_ASYNC, 2 + p_arguments.size());
    for (int i = 0; i < p_arguments.size(); i++) {
        append(p_arguments[i]);
    }
    append(OScriptCompiledFunction::ADDR_SELF);

    CallTarget ct = get_call_target(p_target);
    append(ct.target);
    append(p_arguments.size());
    append(p_function_name);
    ct.cleanup();
}

void OScriptBytecodeGenerator::write_call_script_function(const Address& p_target, const Address& p_base, const StringName& p_function_name, const Vector<Address>& p_arguments) {
    append_opcode_and_argcount(p_target.mode == Address::NIL ? OScriptCompiledFunction::OPCODE_CALL : OScriptCompiledFunction::OPCODE_CALL_RETURN, 2 + p_arguments.size());
    for (int i = 0; i < p_arguments.size(); i++) {
        append(p_arguments[i]);
    }
    append(p_base);

    CallTarget ct = get_call_target(p_target);
    append(ct.target);
    append(p_arguments.size());
    append(p_function_name);
    ct.cleanup();
}

void OScriptBytecodeGenerator::write_lambda(const Address& p_target, OScriptCompiledFunction* p_function, const Vector<Address>& p_captures, bool p_use_self) {
    append_opcode_and_argcount(p_use_self
        ? OScriptCompiledFunction::OPCODE_CREATE_SELF_LAMBDA
        : OScriptCompiledFunction::OPCODE_CREATE_LAMBDA,
        1 + p_captures.size());

    for (const Address& capture : p_captures) {
        append(capture);
    }

    CallTarget ct = get_call_target(p_target);
    append(ct.target);
    append(p_captures.size());
    append(p_function);
    ct.cleanup();
}

void OScriptBytecodeGenerator::write_construct(const Address& p_target, Variant::Type p_type, const Vector<Address>& p_arguments) {
    // Try to find an appropriate constructor.
    bool all_have_type = true;
    Vector<Variant::Type> arg_types;
    for (int i = 0; i < p_arguments.size(); i++) {
        if (!HAS_BUILTIN_TYPE(p_arguments[i])) {
            all_have_type = false;
            break;
        }
        arg_types.push_back(p_arguments[i].type.builtin_type);
    }

    if (all_have_type) {
        // todo: add back in
        // int valid_constructor = -1;
        // const BuiltInType built_in_type = ExtensionDB::get_builtin_type(p_type);
        // for (int i = 0; i < built_in_type.constructors.size(); i++) {
        //     if (built_in_type.constructors[i].arguments.size() != p_arguments.size()) {
        //         continue;
        //     }
        //     int types_correct = true;
        //     for (int j = 0; j < arg_types.size(); j++) {
        //         if (arg_types[j] != built_in_type.constructors[i].arguments[j].type) {
        //             types_correct = false;
        //             break;
        //         }
        //     }
        //     if (types_correct) {
        //         valid_constructor = i;
        //         break;
        //     }
        // }
        // if (valid_constructor >= 0) {
        //     const GDExtensionVariantType type = static_cast<GDExtensionVariantType>(p_type);
        //     GDExtensionPtrConstructor ctor = internal::gdextension_interface_variant_get_ptr_constructor(type, valid_constructor);
        //     append_opcode_and_argcount(OScriptCompiledFunction::OPCODE_CONSTRUCT_VALIDATED, 1 + p_arguments.size());
        //     for (int i = 0; i < p_arguments.size(); i++) {
        //         append(p_arguments[i]);
        //     }
        //     CallTarget ct = get_call_target(p_target);
        //     append(ct.target);
        //     append(p_arguments.size());
        //     append(ctor);
        //     ct.cleanup();
        //
        //     #ifdef DEBUG_ENABLED
        //     add_debug_name(constructors_names, get_constructor_pos(ctor), Variant::get_type_name(p_type));
        //     #endif
        //
        //     return;
        // }
    }

    append_opcode_and_argcount(OScriptCompiledFunction::OPCODE_CONSTRUCT, 1 + p_arguments.size());
    for (int i = 0; i < p_arguments.size(); i++) {
        append(p_arguments[i]);
    }

    CallTarget ct = get_call_target(p_target);
    append(ct.target);
    append(p_arguments.size());
    append(p_type);
    ct.cleanup();
}

void OScriptBytecodeGenerator::write_construct_array(const Address& p_target, const Vector<Address>& p_arguments) {
    append_opcode_and_argcount(OScriptCompiledFunction::OPCODE_CONSTRUCT_ARRAY, 1 + p_arguments.size());
    for (int i = 0; i < p_arguments.size(); i++) {
        append(p_arguments[i]);
    }

    CallTarget ct = get_call_target(p_target);
    append(ct.target);
    append(p_arguments.size());
    ct.cleanup();
}

void OScriptBytecodeGenerator::write_construct_typed_array(const Address& p_target, const OScriptDataType& p_element_type, const Vector<Address>& p_arguments) {
    append_opcode_and_argcount(OScriptCompiledFunction::OPCODE_CONSTRUCT_TYPED_ARRAY, 2 + p_arguments.size());
    for (int i = 0; i < p_arguments.size(); i++) {
        append(p_arguments[i]);
    }

    CallTarget ct = get_call_target(p_target);
    append(ct.target);
    append(get_constant_pos(p_element_type.script_type) | (OScriptCompiledFunction::ADDR_TYPE_CONSTANT << OScriptCompiledFunction::ADDR_BITS));
    append(p_arguments.size());
    append(p_element_type.builtin_type);
    append(p_element_type.native_type);
    ct.cleanup();
}

void OScriptBytecodeGenerator::write_construct_dictionary(const Address& p_target, const Vector<Address>& p_arguments) {
    append_opcode_and_argcount(OScriptCompiledFunction::OPCODE_CONSTRUCT_DICTIONARY, 1 + p_arguments.size());
    for (int i = 0; i < p_arguments.size(); i++) {
        append(p_arguments[i]);
    }

    CallTarget ct = get_call_target(p_target);
    append(ct.target);
    append(p_arguments.size() / 2); // This is number of key-value pairs, so only half of actual arguments.
    ct.cleanup();
}

void OScriptBytecodeGenerator::write_construct_typed_dictionary(const Address& p_target, const OScriptDataType& p_key_type, const OScriptDataType& p_value_type, const Vector<Address>& p_arguments) {
    append_opcode_and_argcount(OScriptCompiledFunction::OPCODE_CONSTRUCT_TYPED_DICTIONARY, 3 + p_arguments.size());
    for (int i = 0; i < p_arguments.size(); i++) {
        append(p_arguments[i]);
    }

    CallTarget ct = get_call_target(p_target);
    append(ct.target);
    append(get_constant_pos(p_key_type.script_type) | (OScriptCompiledFunction::ADDR_TYPE_CONSTANT << OScriptCompiledFunction::ADDR_BITS));
    append(get_constant_pos(p_value_type.script_type) | (OScriptCompiledFunction::ADDR_TYPE_CONSTANT << OScriptCompiledFunction::ADDR_BITS));
    append(p_arguments.size() / 2); // This is number of key-value pairs, so only half of actual arguments.
    append(p_key_type.builtin_type);
    append(p_key_type.native_type);
    append(p_value_type.builtin_type);
    append(p_value_type.native_type);
    ct.cleanup();
}

void OScriptBytecodeGenerator::write_await(const Address& p_target, const Address& p_operand) {
    append_opcode(OScriptCompiledFunction::OPCODE_AWAIT);
    append(p_operand);
    append_opcode(OScriptCompiledFunction::OPCODE_AWAIT_RESUME);
    append(p_target);
}

void OScriptBytecodeGenerator::write_if(const Address& p_condition) {
    append_opcode(OScriptCompiledFunction::OPCODE_JUMP_IF_NOT);
    append(p_condition);
    if_jmp_addrs.push_back(opcodes.size());
    append(0); // Jump destination, will be patched.
}

void OScriptBytecodeGenerator::write_else() {
    append_opcode(OScriptCompiledFunction::OPCODE_JUMP); // Jump from true if block;
    int else_jmp_addr = opcodes.size();
    append(0); // Jump destination, will be patched.

    patch_jump(if_jmp_addrs.back()->get());
    if_jmp_addrs.pop_back();
    if_jmp_addrs.push_back(else_jmp_addr);
}

void OScriptBytecodeGenerator::write_endif() {
    patch_jump(if_jmp_addrs.back()->get());
    if_jmp_addrs.pop_back();
}

void OScriptBytecodeGenerator::write_jump_if_shared(const Address& p_value) {
    append_opcode(OScriptCompiledFunction::OPCODE_JUMP_IF_SHARED);
    append(p_value);
    if_jmp_addrs.push_back(opcodes.size());
    append(0); // Jump destination, will be patched.
}

void OScriptBytecodeGenerator::write_end_jump_if_shared() {
    patch_jump(if_jmp_addrs.back()->get());
    if_jmp_addrs.pop_back();
}

void OScriptBytecodeGenerator::start_for(const OScriptDataType& p_iterator_type, const OScriptDataType& p_list_type, bool p_is_range) {
    Address counter(Address::LOCAL_VARIABLE, add_local("@counter_pos", p_iterator_type), p_iterator_type);

    // Store state.
    for_counter_variables.push_back(counter);

    if (p_is_range) {
        OScriptDataType int_type;
        int_type.kind = OScriptDataType::BUILTIN;
        int_type.builtin_type = Variant::INT;

        Address range_from(Address::LOCAL_VARIABLE, add_local("@range_from", int_type), int_type);
        Address range_to(Address::LOCAL_VARIABLE, add_local("@range_to", int_type), int_type);
        Address range_step(Address::LOCAL_VARIABLE, add_local("@range_step", int_type), int_type);

        // Store state.
        for_range_from_variables.push_back(range_from);
        for_range_to_variables.push_back(range_to);
        for_range_step_variables.push_back(range_step);
    } else {
        Address container(Address::LOCAL_VARIABLE, add_local("@container_pos", p_list_type), p_list_type);

        // Store state.
        for_container_variables.push_back(container);
    }
}

void OScriptBytecodeGenerator::write_for_list_assignment(const Address& p_list) {
    const Address &container = for_container_variables.back()->get();

    // Assign container.
    append_opcode(OScriptCompiledFunction::OPCODE_ASSIGN);
    append(container);
    append(p_list);
}

void OScriptBytecodeGenerator::write_for_range_assignment(const Address& p_from, const Address& p_to, const Address& p_step) {
    const Address &range_from = for_range_from_variables.back()->get();
    const Address &range_to = for_range_to_variables.back()->get();
    const Address &range_step = for_range_step_variables.back()->get();

    // Assign range args.
    if (range_from.type == p_from.type) {
        write_assign(range_from, p_from);
    } else {
        write_assign_with_conversion(range_from, p_from);
    }

    if (range_to.type == p_to.type) {
        write_assign(range_to, p_to);
    } else {
        write_assign_with_conversion(range_to, p_to);
    }

    if (range_step.type == p_step.type) {
        write_assign(range_step, p_step);
    } else {
        write_assign_with_conversion(range_step, p_step);
    }
}

void OScriptBytecodeGenerator::write_for(const Address& p_variable, bool p_use_conversion, bool p_is_range) {
    const Address &counter = for_counter_variables.back()->get();
	const Address &container = p_is_range ? Address() : for_container_variables.back()->get();
	const Address &range_from = p_is_range ? for_range_from_variables.back()->get() : Address();
	const Address &range_to = p_is_range ? for_range_to_variables.back()->get() : Address();
	const Address &range_step = p_is_range ? for_range_step_variables.back()->get() : Address();

	current_breaks_to_patch.push_back(List<int>());

	OScriptCompiledFunction::Opcode begin_opcode = OScriptCompiledFunction::OPCODE_ITERATE_BEGIN;
	OScriptCompiledFunction::Opcode iterate_opcode = OScriptCompiledFunction::OPCODE_ITERATE;

	if (p_is_range) {
		begin_opcode = OScriptCompiledFunction::OPCODE_ITERATE_BEGIN_RANGE;
		iterate_opcode = OScriptCompiledFunction::OPCODE_ITERATE_RANGE;
	} else if (container.type.has_type()) {
		if (container.type.kind == OScriptDataType::BUILTIN) {
			switch (container.type.builtin_type) {
				case Variant::INT: {
				    begin_opcode = OScriptCompiledFunction::OPCODE_ITERATE_BEGIN_INT;
				    iterate_opcode = OScriptCompiledFunction::OPCODE_ITERATE_INT;
				    break;
				}
				case Variant::FLOAT: {
				    begin_opcode = OScriptCompiledFunction::OPCODE_ITERATE_BEGIN_FLOAT;
				    iterate_opcode = OScriptCompiledFunction::OPCODE_ITERATE_FLOAT;
				    break;
				}
				case Variant::VECTOR2: {
				    begin_opcode = OScriptCompiledFunction::OPCODE_ITERATE_BEGIN_VECTOR2;
				    iterate_opcode = OScriptCompiledFunction::OPCODE_ITERATE_VECTOR2;
				    break;
				}
				case Variant::VECTOR2I: {
				    begin_opcode = OScriptCompiledFunction::OPCODE_ITERATE_BEGIN_VECTOR2I;
				    iterate_opcode = OScriptCompiledFunction::OPCODE_ITERATE_VECTOR2I;
				    break;
				}
				case Variant::VECTOR3: {
				    begin_opcode = OScriptCompiledFunction::OPCODE_ITERATE_BEGIN_VECTOR3;
				    iterate_opcode = OScriptCompiledFunction::OPCODE_ITERATE_VECTOR3;
				    break;
				}
				case Variant::VECTOR3I: {
				    begin_opcode = OScriptCompiledFunction::OPCODE_ITERATE_BEGIN_VECTOR3I;
				    iterate_opcode = OScriptCompiledFunction::OPCODE_ITERATE_VECTOR3I;
				    break;
				}
				case Variant::STRING: {
				    begin_opcode = OScriptCompiledFunction::OPCODE_ITERATE_BEGIN_STRING;
				    iterate_opcode = OScriptCompiledFunction::OPCODE_ITERATE_STRING;
				    break;
				}
				case Variant::DICTIONARY: {
				    begin_opcode = OScriptCompiledFunction::OPCODE_ITERATE_BEGIN_DICTIONARY;
				    iterate_opcode = OScriptCompiledFunction::OPCODE_ITERATE_DICTIONARY;
				    break;
				}
				case Variant::ARRAY: {
				    begin_opcode = OScriptCompiledFunction::OPCODE_ITERATE_BEGIN_ARRAY;
				    iterate_opcode = OScriptCompiledFunction::OPCODE_ITERATE_ARRAY;
				    break;
				}
				case Variant::PACKED_BYTE_ARRAY: {
				    begin_opcode = OScriptCompiledFunction::OPCODE_ITERATE_BEGIN_PACKED_BYTE_ARRAY;
				    iterate_opcode = OScriptCompiledFunction::OPCODE_ITERATE_PACKED_BYTE_ARRAY;
				    break;
				}
			    case Variant::PACKED_INT32_ARRAY: {
				    begin_opcode = OScriptCompiledFunction::OPCODE_ITERATE_BEGIN_PACKED_INT32_ARRAY;
				    iterate_opcode = OScriptCompiledFunction::OPCODE_ITERATE_PACKED_INT32_ARRAY;
				    break;
				}
				case Variant::PACKED_INT64_ARRAY: {
				    begin_opcode = OScriptCompiledFunction::OPCODE_ITERATE_BEGIN_PACKED_INT64_ARRAY;
				    iterate_opcode = OScriptCompiledFunction::OPCODE_ITERATE_PACKED_INT64_ARRAY;
				    break;
				}
				case Variant::PACKED_FLOAT32_ARRAY: {
				    begin_opcode = OScriptCompiledFunction::OPCODE_ITERATE_BEGIN_PACKED_FLOAT32_ARRAY;
				    iterate_opcode = OScriptCompiledFunction::OPCODE_ITERATE_PACKED_FLOAT32_ARRAY;
				    break;
				}
				case Variant::PACKED_FLOAT64_ARRAY: {
				    begin_opcode = OScriptCompiledFunction::OPCODE_ITERATE_BEGIN_PACKED_FLOAT64_ARRAY;
				    iterate_opcode = OScriptCompiledFunction::OPCODE_ITERATE_PACKED_FLOAT64_ARRAY;
				    break;
				}
				case Variant::PACKED_STRING_ARRAY: {
				    begin_opcode = OScriptCompiledFunction::OPCODE_ITERATE_BEGIN_PACKED_STRING_ARRAY;
				    iterate_opcode = OScriptCompiledFunction::OPCODE_ITERATE_PACKED_STRING_ARRAY;
				    break;
				}
				case Variant::PACKED_VECTOR2_ARRAY: {
				    begin_opcode = OScriptCompiledFunction::OPCODE_ITERATE_BEGIN_PACKED_VECTOR2_ARRAY;
				    iterate_opcode = OScriptCompiledFunction::OPCODE_ITERATE_PACKED_VECTOR2_ARRAY;
				    break;
				}
			    case Variant::PACKED_VECTOR3_ARRAY: {
				    begin_opcode = OScriptCompiledFunction::OPCODE_ITERATE_BEGIN_PACKED_VECTOR3_ARRAY;
				    iterate_opcode = OScriptCompiledFunction::OPCODE_ITERATE_PACKED_VECTOR3_ARRAY;
				    break;
				}
				case Variant::PACKED_COLOR_ARRAY: {
				    begin_opcode = OScriptCompiledFunction::OPCODE_ITERATE_BEGIN_PACKED_COLOR_ARRAY;
				    iterate_opcode = OScriptCompiledFunction::OPCODE_ITERATE_PACKED_COLOR_ARRAY;
				    break;
				}
				case Variant::PACKED_VECTOR4_ARRAY: {
				    begin_opcode = OScriptCompiledFunction::OPCODE_ITERATE_BEGIN_PACKED_VECTOR4_ARRAY;
				    iterate_opcode = OScriptCompiledFunction::OPCODE_ITERATE_PACKED_VECTOR4_ARRAY;
				    break;
				}
				default: {
				    break;
				}
			}
		} else {
			begin_opcode = OScriptCompiledFunction::OPCODE_ITERATE_BEGIN_OBJECT;
			iterate_opcode = OScriptCompiledFunction::OPCODE_ITERATE_OBJECT;
		}
	}

	Address temp;
	if (p_use_conversion) {
		temp = Address(Address::LOCAL_VARIABLE, add_local("@iterator_temp", OScriptDataType()));
	}

	// Begin loop.
	append_opcode(begin_opcode);
	append(counter);
	if (p_is_range) {
		append(range_from);
		append(range_to);
		append(range_step);
	} else {
		append(container);
	}
	append(p_use_conversion ? temp : p_variable);
	for_jmp_addrs.push_back(opcodes.size());
	append(0); // End of loop address, will be patched.
	append_opcode(OScriptCompiledFunction::OPCODE_JUMP);
	append(opcodes.size() + (p_is_range ? 7 : 6)); // Skip over 'continue' code.

	// Next iteration.
	int continue_addr = opcodes.size();
	continue_addrs.push_back(continue_addr);
	append_opcode(iterate_opcode);
	append(counter);
	if (p_is_range) {
		append(range_to);
		append(range_step);
	} else {
		append(container);
	}
	append(p_use_conversion ? temp : p_variable);
	for_jmp_addrs.push_back(opcodes.size());
	append(0); // Jump destination, will be patched.

	if (p_use_conversion) {
		write_assign_with_conversion(p_variable, temp);
		if (p_variable.type.can_contain_object()) {
			clear_address(temp); // Can contain `RefCounted`, so clear it.
		}
	}
}

void OScriptBytecodeGenerator::write_endfor(bool p_is_range) {
    // Jump back to loop check.
    append_opcode(OScriptCompiledFunction::OPCODE_JUMP);
    append(continue_addrs.back()->get());
    continue_addrs.pop_back();

    // Patch end jumps (two of them).
    for (int i = 0; i < 2; i++) {
        patch_jump(for_jmp_addrs.back()->get());
        for_jmp_addrs.pop_back();
    }

    // Patch break statements.
    for (const int &E : current_breaks_to_patch.back()->get()) {
        patch_jump(E);
    }
    current_breaks_to_patch.pop_back();

    // Pop state.
    for_counter_variables.pop_back();
    if (p_is_range) {
        for_range_from_variables.pop_back();
        for_range_to_variables.pop_back();
        for_range_step_variables.pop_back();
    } else {
        for_container_variables.pop_back();
    }
}

void OScriptBytecodeGenerator::start_while_condition() {
    current_breaks_to_patch.push_back(List<int>());
    continue_addrs.push_back(opcodes.size());
}

void OScriptBytecodeGenerator::write_while(const Address& p_condition) {
    // Condition check.
    append_opcode(OScriptCompiledFunction::OPCODE_JUMP_IF_NOT);
    append(p_condition);
    while_jmp_addrs.push_back(opcodes.size());
    append(0); // End of loop address, will be patched.
}

void OScriptBytecodeGenerator::write_endwhile() {
    // Jump back to loop check.
    append_opcode(OScriptCompiledFunction::OPCODE_JUMP);
    append(continue_addrs.back()->get());
    continue_addrs.pop_back();

    // Patch end jump.
    patch_jump(while_jmp_addrs.back()->get());
    while_jmp_addrs.pop_back();

    // Patch break statements.
    for (const int &E : current_breaks_to_patch.back()->get()) {
        patch_jump(E);
    }
    current_breaks_to_patch.pop_back();
}

void OScriptBytecodeGenerator::write_break() {
    append_opcode(OScriptCompiledFunction::OPCODE_JUMP);
    current_breaks_to_patch.back()->get().push_back(opcodes.size());
    append(0);
}

void OScriptBytecodeGenerator::write_continue() {
    append_opcode(OScriptCompiledFunction::OPCODE_JUMP);
    append(continue_addrs.back()->get());
}

void OScriptBytecodeGenerator::write_breakpoint() {
    append_opcode(OScriptCompiledFunction::OPCODE_BREAKPOINT);
}

void OScriptBytecodeGenerator::write_newline(int p_node) {
    if (OScriptLanguage::get_singleton()->should_track_call_stack() && p_node >= 0 && p_node != current_script_node_id) {
        // Add newline for debugger and stack tracking if enabled in the project settings.
        append_opcode(OScriptCompiledFunction::OPCODE_SCRIPT_NODE);
        append(p_node);
        current_script_node_id = p_node;
    }
}

void OScriptBytecodeGenerator::write_return(const Address& p_return_value) {
    if (!function->return_type.has_type() || p_return_value.type.has_type()) {
		// Either the function is untyped or the return value is also typed.

		// If this is a typed function, then we need to check for potential conversions.
		if (function->return_type.has_type()) {
			if (function->return_type.kind == OScriptDataType::BUILTIN && function->return_type.builtin_type == Variant::ARRAY && function->return_type.has_container_element_type(0)) {
				// Typed array.
				const OScriptDataType &element_type = function->return_type.get_container_element_type(0);
				append_opcode(OScriptCompiledFunction::OPCODE_RETURN_TYPED_ARRAY);
				append(p_return_value);
				append(get_constant_pos(element_type.script_type) | (OScriptCompiledFunction::ADDR_TYPE_CONSTANT << OScriptCompiledFunction::ADDR_BITS));
				append(element_type.builtin_type);
				append(element_type.native_type);
			} else if (function->return_type.kind == OScriptDataType::BUILTIN && function->return_type.builtin_type == Variant::DICTIONARY &&
					function->return_type.has_container_element_types()) {
				// Typed dictionary.
				const OScriptDataType &key_type = function->return_type.get_container_element_type_or_variant(0);
				const OScriptDataType &value_type = function->return_type.get_container_element_type_or_variant(1);
				append_opcode(OScriptCompiledFunction::OPCODE_RETURN_TYPED_DICTIONARY);
				append(p_return_value);
				append(get_constant_pos(key_type.script_type) | (OScriptCompiledFunction::ADDR_TYPE_CONSTANT << OScriptCompiledFunction::ADDR_BITS));
				append(get_constant_pos(value_type.script_type) | (OScriptCompiledFunction::ADDR_TYPE_CONSTANT << OScriptCompiledFunction::ADDR_BITS));
				append(key_type.builtin_type);
				append(key_type.native_type);
				append(value_type.builtin_type);
				append(value_type.native_type);
			} else if (function->return_type.kind == OScriptDataType::BUILTIN && p_return_value.type.kind == OScriptDataType::BUILTIN && function->return_type.builtin_type != p_return_value.type.builtin_type) {
				// Add conversion.
				append_opcode(OScriptCompiledFunction::OPCODE_RETURN_TYPED_BUILTIN);
				append(p_return_value);
				append(function->return_type.builtin_type);
			} else {
				// Just assign.
				append_opcode(OScriptCompiledFunction::OPCODE_RETURN);
				append(p_return_value);
			}
		} else {
			append_opcode(OScriptCompiledFunction::OPCODE_RETURN);
			append(p_return_value);
		}
	} else {
		switch (function->return_type.kind) {
			case OScriptDataType::BUILTIN: {
				if (function->return_type.builtin_type == Variant::ARRAY && function->return_type.has_container_element_type(0)) {
					const OScriptDataType &element_type = function->return_type.get_container_element_type(0);
					append_opcode(OScriptCompiledFunction::OPCODE_RETURN_TYPED_ARRAY);
					append(p_return_value);
					append(get_constant_pos(element_type.script_type) | (OScriptCompiledFunction::ADDR_TYPE_CONSTANT << OScriptCompiledFunction::ADDR_BITS));
					append(element_type.builtin_type);
					append(element_type.native_type);
				} else if (function->return_type.builtin_type == Variant::DICTIONARY && function->return_type.has_container_element_types()) {
					const OScriptDataType &key_type = function->return_type.get_container_element_type_or_variant(0);
					const OScriptDataType &value_type = function->return_type.get_container_element_type_or_variant(1);
					append_opcode(OScriptCompiledFunction::OPCODE_RETURN_TYPED_DICTIONARY);
					append(p_return_value);
					append(get_constant_pos(key_type.script_type) | (OScriptCompiledFunction::ADDR_TYPE_CONSTANT << OScriptCompiledFunction::ADDR_BITS));
					append(get_constant_pos(value_type.script_type) | (OScriptCompiledFunction::ADDR_TYPE_CONSTANT << OScriptCompiledFunction::ADDR_BITS));
					append(key_type.builtin_type);
					append(key_type.native_type);
					append(value_type.builtin_type);
					append(value_type.native_type);
				} else {
					append_opcode(OScriptCompiledFunction::OPCODE_RETURN_TYPED_BUILTIN);
					append(p_return_value);
					append(function->return_type.builtin_type);
				}
			    break;
			}
			case OScriptDataType::NATIVE: {
				append_opcode(OScriptCompiledFunction::OPCODE_RETURN_TYPED_NATIVE);
				append(p_return_value);
				int class_idx = OScriptLanguage::get_singleton()->get_global_map()[function->return_type.native_type];
				Variant nc = OScriptLanguage::get_singleton()->get_global_array()[class_idx];
				class_idx = get_constant_pos(nc) | (OScriptCompiledFunction::ADDR_TYPE_CONSTANT << OScriptCompiledFunction::ADDR_BITS);
				append(class_idx);
			    break;
			}
			case OScriptDataType::OSCRIPT:
			case OScriptDataType::SCRIPT: {
				Variant script = function->return_type.script_type;
				int script_idx = get_constant_pos(script) | (OScriptCompiledFunction::ADDR_TYPE_CONSTANT << OScriptCompiledFunction::ADDR_BITS);

				append_opcode(OScriptCompiledFunction::OPCODE_RETURN_TYPED_SCRIPT);
				append(p_return_value);
				append(script_idx);
			    break;
			}
			default: {
				ERR_PRINT("Compiler bug: unresolved return.");

				// Shouldn't get here, but fail-safe to a regular return;
				append_opcode(OScriptCompiledFunction::OPCODE_RETURN);
				append(p_return_value);
			    break;
			}
		}
	}
}

void OScriptBytecodeGenerator::write_assert(const Address& p_test, const Address& p_message) {
    append_opcode(OScriptCompiledFunction::OPCODE_ASSERT);
    append(p_test);
    append(p_message);
}

OScriptBytecodeGenerator::~OScriptBytecodeGenerator() {
    if (!ended && function != nullptr) {
        memdelete(function);
    }
}
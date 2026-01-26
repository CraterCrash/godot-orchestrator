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
#include "script/compiler/compiled_function.h"

#include "common/dictionary_utils.h"
#include "core/godot/variant/variant.h"
#include "script/script.h"

#include <godot_cpp/core/mutex_lock.hpp>

bool OScriptDataType::is_type(const Variant& p_variant, bool p_allow_implicit_conversion) const {
    switch (kind) {
		case VARIANT: {
			return true;
		}
		case BUILTIN: {
			Variant::Type var_type = p_variant.get_type();
			bool valid = builtin_type == var_type;
			if (valid && builtin_type == Variant::ARRAY && has_container_element_type(0)) {
				Array array = p_variant;
				if (array.is_typed()) {
					const OScriptDataType& elem_type = container_element_types[0];
				    Variant::Type array_builtin_type = static_cast<Variant::Type>(array.get_typed_builtin());
					StringName array_native_type = array.get_typed_class_name();
					Ref<Script> array_script_type_ref = array.get_typed_script();

					if (array_script_type_ref.is_valid()) {
						valid = (elem_type.kind == SCRIPT || elem_type.kind == OSCRIPT) && elem_type.script_type == array_script_type_ref.ptr();
					} else if (array_native_type != StringName()) {
						valid = elem_type.kind == NATIVE && elem_type.native_type == array_native_type;
					} else {
						valid = elem_type.kind == BUILTIN && elem_type.builtin_type == array_builtin_type;
					}
				} else {
					valid = false;
				}
			} else if (valid && builtin_type == Variant::DICTIONARY && has_container_element_types()) {
				Dictionary dictionary = p_variant;
				if (dictionary.is_typed()) {
					if (dictionary.is_typed_key()) {
						OScriptDataType key = get_container_element_type_or_variant(0);
						Variant::Type key_builtin_type = static_cast<Variant::Type>(dictionary.get_typed_key_builtin());
						StringName key_native_type = dictionary.get_typed_key_class_name();
						Ref<Script> key_script_type_ref = dictionary.get_typed_key_script();

						if (key_script_type_ref.is_valid()) {
							valid = (key.kind == SCRIPT || key.kind == OSCRIPT) && key.script_type == key_script_type_ref.ptr();
						} else if (key_native_type != StringName()) {
							valid = key.kind == NATIVE && key.native_type == key_native_type;
						} else {
							valid = key.kind == BUILTIN && key.builtin_type == key_builtin_type;
						}
					}

					if (valid && dictionary.is_typed_value()) {
						OScriptDataType value = get_container_element_type_or_variant(1);
						Variant::Type value_builtin_type = static_cast<Variant::Type>(dictionary.get_typed_value_builtin());
						StringName value_native_type = dictionary.get_typed_value_class_name();
						Ref<Script> value_script_type_ref = dictionary.get_typed_value_script();

						if (value_script_type_ref.is_valid()) {
							valid = (value.kind == SCRIPT || value.kind == OSCRIPT) && value.script_type == value_script_type_ref.ptr();
						} else if (value_native_type != StringName()) {
							valid = value.kind == NATIVE && value.native_type == value_native_type;
						} else {
							valid = value.kind == BUILTIN && value.builtin_type == value_builtin_type;
						}
					}
				} else {
					valid = false;
				}
			} else if (!valid && p_allow_implicit_conversion) {
				valid = Variant::can_convert_strict(var_type, builtin_type);
			}
			return valid;
		}
		case NATIVE: {
			if (p_variant.get_type() == Variant::NIL) {
				return true;
			}
			if (p_variant.get_type() != Variant::OBJECT) {
				return false;
			}

			bool was_freed = false;
			Object *obj = GDE::Variant::get_validated_object_with_check(p_variant, was_freed);
			if (!obj) {
				return !was_freed;
			}

			if (!ClassDB::is_parent_class(obj->get_class(), native_type)) {
				return false;
			}
			return true;
		}
		case SCRIPT:
		case OSCRIPT: {
			if (p_variant.get_type() == Variant::NIL) {
				return true;
			}
			if (p_variant.get_type() != Variant::OBJECT) {
				return false;
			}

			bool was_freed = false;
			Object *obj = GDE::Variant::get_validated_object_with_check(p_variant, was_freed);
			if (!obj) {
				return !was_freed;
			}

		    bool valid = false;
		    Ref<Script> base = obj->get_script();
			while (base.is_valid()) {
				if (base == script_type) {
					valid = true;
					break;
				}
				base = base->get_base_script();
			}
			return valid;
		}
	}
	return false;
}

bool OScriptDataType::can_contain_object() const {
    if (kind == BUILTIN) {
        switch (builtin_type) {
            case Variant::ARRAY: {
                if (has_container_element_type(0)) {
                    return container_element_types[0].can_contain_object();
                }
                return true;
            }
            case Variant::DICTIONARY: {
                if (has_container_element_types()) {
                    return get_container_element_type_or_variant(0).can_contain_object()
                            || get_container_element_type_or_variant(1).can_contain_object();
                }
                return true;
            }
            case Variant::NIL:
            case Variant::OBJECT: {
                return true;
            }
            default: {
                return false;
            }
        }
    }
    return true;
}

void OScriptDataType::set_container_element_type(int p_index, const OScriptDataType& p_element_type) {
    ERR_FAIL_COND(p_index < 0);
    while (p_index >= container_element_types.size()) {
        container_element_types.push_back(OScriptDataType());
    }
    container_element_types.write[p_index] = OScriptDataType(p_element_type);
}

OScriptDataType OScriptDataType::get_container_element_type(int p_index) const {
    ERR_FAIL_INDEX_V(p_index, container_element_types.size(), OScriptDataType());
    return container_element_types[p_index];
}

OScriptDataType OScriptDataType::get_container_element_type_or_variant(int p_index) const {
    if (p_index < 0 || p_index >= container_element_types.size()) {
        return {};
    }
    return container_element_types[p_index];
}

bool OScriptDataType::has_container_element_type(int p_index) const {
    return p_index >= 0 && p_index < container_element_types.size();
}

bool OScriptDataType::has_container_element_types() const {
    return !container_element_types.is_empty();
}

bool OScriptDataType::operator==(const OScriptDataType& p_other) const {
    return kind == p_other.kind
        && builtin_type == p_other.builtin_type
        && native_type == p_other.native_type
        && (script_type == p_other.script_type || script_type_ref == p_other.script_type_ref)
        && container_element_types == p_other.container_element_types;
}

bool OScriptDataType::operator!=(const OScriptDataType& p_other) const {
    return !(*this == p_other);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OScriptCompiledFunction

Variant OScriptCompiledFunction::get_constant(int p_index) const {
    ERR_FAIL_INDEX_V(p_index, constants.size(), "<errconst>");
    return constants[p_index];
}

StringName OScriptCompiledFunction::get_global_name(int p_index) const {
    ERR_FAIL_INDEX_V(p_index, global_names.size(), "<errgname>");
    return global_names[p_index];
}

struct _GDFKC {
    int order = 0;
    List<int> pos;
};

struct _GDFKCS {
    int order = 0;
    StringName id;
    int pos = 0;

    bool operator<(const _GDFKCS &p_r) const {
        return order < p_r.order;
    }
};

void OScriptCompiledFunction::debug_get_stack_member_state(int p_node, List<Pair<StringName, int>>* r_stack_vars) const {
    int oc = 0;

    HashMap<StringName, _GDFKC> sdmap;
    for (const StackDebug& sd : stack_debug) {
        if (sd.source_node_id >= p_node) {
            break;
        }

        if (sd.added) {
            if (!sdmap.has(sd.identifier)) {
                _GDFKC d;
                d.order = oc++;
                d.pos.push_back(sd.pos);
                sdmap[sd.identifier] = d;
            } else {
                sdmap[sd.identifier].pos.push_back(sd.pos);
            }
        } else {
            ERR_CONTINUE(!sdmap.has(sd.identifier));

            sdmap[sd.identifier].pos.pop_back();
            if (sdmap[sd.identifier].pos.is_empty()) {
                sdmap.erase(sd.identifier);
            }
        }
    }

    List<_GDFKCS> stack_positions;
    for (const KeyValue<StringName, _GDFKC>& E : sdmap) {
        _GDFKCS sp;
        sp.id = E.key;
        sp.order = E.value.order;
        sp.pos = E.value.pos.back()->get();
        stack_positions.push_back(sp);
    }

    stack_positions.sort();

    for (_GDFKCS& E : stack_positions) {
        Pair<StringName, int> p;
        p.first = E.id;
        p.second = E.pos;
        r_stack_vars->push_back(p);
    }
}

String OScriptCompiledFunction::to_string() {
    String result;
    result += vformat("Name        : %s.%s\n", source, name);
    result += vformat("Is Static   : %s\n", _static ? "Yes" : "No");
    result += vformat("Method      : %s\n", DictionaryUtils::from_method(method_info));
    result += vformat("RPC         : %s\n", rpc_config);
    result += vformat("Argument Cnt: %d\n", argument_count);
    result += vformat("VarArg Index: %d\n", vararg_index);
    result += vformat("Stack Size  : %d\n", stack_size);
    result += vformat("InstrArgSize: %d\n", instruction_arg_size);
    result += vformat("Temp Slots  : %d\n", temporary_slots.size());
    for (const KeyValue<int, Variant::Type>& E : temporary_slots) {
        result += vformat("\t[%d]: %s\n", E.key, Variant::get_type_name(E.value));
    }
    result += vformat("Code Size   : %d\n", code_size);

    String code_str;
    for (int i = 0; i < code_size; i++) {
        code_str += vformat("%d ", code[i]);
    }
    result += vformat("Code        : %s\n", code_str.strip_edges());

    result += "\n";

    #ifdef DEBUG_ENABLED
    Vector<String> lines;
    disassemble(Vector<String>(), lines);
    for (const String& line : lines) {
        result += vformat("%s\n", line);
    }
    #endif

    return result;
}

OScriptCompiledFunction::OScriptCompiledFunction() {
    name = "<anonymous>";
    #ifdef DEBUG_ENABLED
    {
        MutexLock lock(*OScriptLanguage::get_singleton()->lock.ptr());
        OScriptLanguage::get_singleton()->function_list.add(&function_list);
    }
    #endif
}

OScriptCompiledFunction::~OScriptCompiledFunction() {
    get_script()->member_functions.erase(name);

    for (int i = 0; i < argument_types.size(); i++) {
        argument_types.write[i].script_type_ref = Ref<Script>();
    }

    return_type.script_type_ref = Ref<Script>();

    #ifdef DEBUG_ENABLED
    MutexLock lock(*OScriptLanguage::get_singleton()->lock.ptr());
    OScriptLanguage::get_singleton()->function_list.remove(&function_list);
    #endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OScriptFunctionState

Variant OScriptFunctionState::_signal_callback(const Variant** p_args, GDExtensionInt p_arg_count, GDExtensionCallError& r_error) {
    Variant arg;
    r_error.error = GDEXTENSION_CALL_OK;

    if (p_arg_count == 0) {
        r_error.error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS;
        r_error.expected = 1;
        return arg;
    } else if (p_arg_count == 1) {
        // nooneee
    } else if (p_arg_count == 2) {
        arg = *p_args[0];
    } else {
        Array extra_args;
        for (int i = 0; i < p_arg_count - 1; i++) {
            extra_args.push_back(*p_args[i]);
        }
        arg = extra_args;
    }

    Ref<OScriptFunctionState> self = *p_args[p_arg_count - 1];
    if (self.is_null()) {
        r_error.error = GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT;
        r_error.argument = p_arg_count - 1;
        r_error.expected = Variant::OBJECT;
        return {};
    }

    return resume(arg);
}

bool OScriptFunctionState::is_valid(bool p_extended_check) const {
    if (function == nullptr) {
        return false;
    }

    if (p_extended_check) {
        MutexLock lock(*OScriptLanguage::get_singleton()->lock.ptr());
        if (!scripts_list.in_list()) {
            return false;
        }
        if (state.instance && !instances_list.in_list()) {
            return false;
        }
    }

    return true;
}

Variant OScriptFunctionState::resume(const Variant& p_arg) {
    ERR_FAIL_NULL_V(function, Variant());
    {
        MutexLock lock(*OScriptLanguage::get_singleton()->lock.ptr());
        if (!scripts_list.in_list()) {
            #ifdef DEBUG_ENABLED
            ERR_FAIL_V_MSG(Variant(), "Resumed function '" + state.function_name
                + "()' after await, but script is gone. At script: " + state.script_path + ":" + itos(state.node_id));
            #else
            return Variant();
            #endif
        }

        if (state.instance && !instances_list.in_list()) {
            #ifdef DEBUG_ENABLED
            ERR_FAIL_V_MSG(Variant(), "Resumed function '" + state.function_name
                + "()' after await, but class instance is gone. At script: " + state.script_path + ":" + itos(state.node_id));
            #else
            return Variant();
            #endif
        }

        scripts_list.remove_from_list();
        instances_list.remove_from_list();
    }

    state.result = p_arg;
    GDExtensionCallError error;
    Variant result = function->call(nullptr, nullptr, 0, error, &state);

    bool completed = true;
    if (result.get_type() == Variant::OBJECT) {
        ObjectID id(result.get_validated_object()->get_instance_id());
        if (id.is_ref_counted()) {
            OScriptFunctionState* fs = cast_to<OScriptFunctionState>(result);
            if (fs && fs->function == function) {
                completed = false;
                fs->first_state = first_state.is_valid() ? first_state : Ref<OScriptFunctionState>(this);
            }
        }
    }

    function = nullptr;
    state.result = Variant();

    if (completed) {
        _clear_stack();
    }

    return result;
}

void OScriptFunctionState::_clear_stack() {
    if (state.stack_size) {
        Variant* stack = (Variant*)state.stack.ptr();
        for (int i = OScriptCompiledFunction::FIXED_ADDRESSES_MAX; i < state.stack_size; i++) {
            stack[i].~Variant();
        }
        state.stack_size = 0;
    }
}

void OScriptFunctionState::_clear_connections() {
    const TypedArray<Dictionary> signals = get_incoming_connections();
    for (uint32_t i = 0; i < signals.size(); i++) {
        const Dictionary& dict = signals[i];
        if (dict.has("signal")) {
            const Signal& signal = dict["signal"];
            const Callable& callable = dict["callable"];
            const_cast<Signal&>(signal).disconnect(callable);
        }
    }
}

void OScriptFunctionState::_bind_methods() {
    ClassDB::bind_method(D_METHOD("resume", "arg"), &OScriptFunctionState::resume, DEFVAL(Variant()));
    ClassDB::bind_method(D_METHOD("is_valid", "extended_check"), &OScriptFunctionState::is_valid, DEFVAL(false));

    MethodInfo mi;
    mi.name = "_signal_callback";
    mi.arguments.push_back(PropertyInfo(Variant::OBJECT, "data"));
    ClassDB::bind_vararg_method(METHOD_FLAGS_DEFAULT, "_signal_callback", &OScriptFunctionState::_signal_callback, mi);

    ADD_SIGNAL(MethodInfo("completed", PropertyInfo(Variant::NIL, "result", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NIL_IS_VARIANT)));
}

OScriptFunctionState::OScriptFunctionState() : scripts_list(this), instances_list(this) {
}

OScriptFunctionState::~OScriptFunctionState() {
    MutexLock lock(*OScriptLanguage::get_singleton()->lock.ptr());
    scripts_list.remove_from_list();
    instances_list.remove_from_list();
}

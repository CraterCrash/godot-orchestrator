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
#include "script/nodes/utilities/await.h"

#include "common/dictionary_utils.h"
#include "common/method_utils.h"
#include "common/property_utils.h"
#include "core/godot/gdextension_compat.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OScriptNodeAwaitCoroutine

void OScriptNodeAwaitCoroutine::allocate_default_pins() {
    create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"));
    create_pin(PD_Input, PT_Data, PropertyUtils::make_object("target"));

    Ref<OScriptNodePin> func_name = create_pin(PD_Input, PT_Data, PropertyUtils::make_typed("function_name", Variant::STRING), _method.name);
    if (func_name.is_valid()) {
        func_name->set_flag(OScriptNodePin::NO_CONNECTION);
    }

    const size_t default_start_index = is_vector_empty(_method.arguments)
        ? 0
        : _method.arguments.size() - _method.default_arguments.size();

    size_t argument_index = 0;
    size_t default_index = 0;
    for (const PropertyInfo& pi : _method.arguments) {
        Ref<OScriptNodePin> pin = create_pin(PD_Input, PT_Data, pi);

        if (pin.is_valid()) {
            if (argument_index >= default_start_index) {
                pin->set_generated_default_value(_method.default_arguments[default_index]);
                pin->set_default_value(_method.default_arguments[default_index++]);
            }
        }
        argument_index++;
    }

    // todo: support varargs

    create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("ExecOut"));

    if (MethodUtils::has_return_value(_method)) {
        create_pin(PD_Output, PT_Data, PropertyUtils::as("result", _method.return_val));
    }

    super::allocate_default_pins();
}

String OScriptNodeAwaitCoroutine::get_tooltip_text() const {
    return "Yields/Awaits the script's execution until the given coroutine returns.";
}

String OScriptNodeAwaitCoroutine::get_node_title() const {
    return "Await Coroutine";
}

void OScriptNodeAwaitCoroutine::on_pin_connected(const Ref<OScriptNodePin>& p_pin) {
    // Makes sure that signal list pin changes to string renderer
    if (p_pin.is_valid() && p_pin->get_pin_name().match("target")) {
        _method = MethodInfo();
        p_pin->set_generated_default_value(Variant());
        p_pin->set_default_value(Variant());

        _notify_pins_changed();
    }
    super::on_pin_connected(p_pin);
}

void OScriptNodeAwaitCoroutine::on_pin_disconnected(const Ref<OScriptNodePin>& p_pin) {
    // Makes sure that signal list pin changes to string renderer
    if (p_pin.is_valid() && p_pin->get_pin_name().match("target")) {
        _method = MethodInfo();
        p_pin->set_generated_default_value(Variant());
        p_pin->set_default_value(Variant());

        _notify_pins_changed();
    }
    super::on_pin_disconnected(p_pin);
}

void OScriptNodeAwaitCoroutine::pin_default_value_changed(const Ref<OScriptNodePin>& p_pin) {
    if (!_is_queued_for_reconstruction() && p_pin.is_valid() && p_pin->get_pin_name().match("function_name")) {

        const StringName function_name = p_pin->get_default_value();
        if (function_name.is_empty()) {
            _method = MethodInfo();
            _queue_reconstruct();
        } else if (!function_name.match(_method.name)) {
            if (get_orchestration()) {
                const TypedArray<Dictionary> methods = get_orchestration()->as_script()->get_script_method_list();

                bool found = false;
                for (int i = 0; i < methods.size(); i++) {
                    const Dictionary& dict = methods[i];
                    if (function_name.match(dict["name"])) {
                        _method = DictionaryUtils::to_method(dict);
                        found = true;
                        if (_is_in_editor()) {
                            set_size(Vector2(0,0));
                        }
                        break;
                    }
                }

                if (!found) {
                    _method = MethodInfo(function_name);
                }

                _queue_reconstruct();
            }
        }
    }

    super::pin_default_value_changed(p_pin);
}

PackedStringArray OScriptNodeAwaitCoroutine::get_suggestions(const Ref<OScriptNodePin>& p_pin) {
    if (p_pin.is_valid() && p_pin->is_input() && p_pin->get_pin_name().match("function_name")) {
        const Ref<OScriptNodePin> target_pin = find_pin("target", PD_Input);
        if (target_pin.is_valid()) {
            Ref<Script> target_script;
            if (target_pin->has_any_connections()) {
                Ref<OScriptTargetObject> target = target_pin->get_connections()[0]->resolve_target();
                if (target.is_valid() && target->has_target()) {
                    target_script = target->get_target();
                    if (target_script.is_null()) {
                        Object* object = target->get_target();
                        target_script = object->get_script();
                    }
                }
            } else {
                target_script = get_orchestration()->as_script();
            }

            if (target_script.is_valid()) {
                PackedStringArray names;
                const TypedArray<Dictionary> methods = target_script->get_script_method_list();
                for (int i = 0; i < methods.size(); i++) {
                    const Dictionary& dict = methods[i];
                    const String function_name = dict["name"];
                    if (!function_name.is_empty() && !names.has(function_name)) {
                        names.push_back(function_name);
                    }
                }
                names.sort();
                return names;
            }
        }
    }

    return super::get_suggestions(p_pin);
}

Dictionary OScriptNodeAwaitCoroutine::get_method() const {
    return DictionaryUtils::from_method(_method);
}

void OScriptNodeAwaitCoroutine::set_method(const Dictionary& p_method) {
    _method = DictionaryUtils::to_method(p_method);
    _notify_pins_changed();
    emit_changed();
}

MethodInfo OScriptNodeAwaitCoroutine::get_method_info() const {
    return _method;
}

void OScriptNodeAwaitCoroutine::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_method", "method"), &OScriptNodeAwaitCoroutine::set_method);
    ClassDB::bind_method(D_METHOD("get_method"), &OScriptNodeAwaitCoroutine::get_method);
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "method", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_method", "get_method");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OScriptNodeAwaitSignal

void OScriptNodeAwaitSignal::_upgrade(uint32_t p_version, uint32_t p_current_version) {
    if (find_pin("result", PD_Output).is_null()) {
        reconstruct_node();
    }
}

void OScriptNodeAwaitSignal::allocate_default_pins() {
    create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"));
    create_pin(PD_Input, PT_Data, PropertyUtils::make_object("target"));
    create_pin(PD_Input, PT_Data, PropertyUtils::make_typed("signal_name", Variant::STRING));
    create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("ExecOut"));
    create_pin(PD_Output, PT_Data, PropertyUtils::make_variant("result"));
    super::allocate_default_pins();
}

String OScriptNodeAwaitSignal::get_tooltip_text() const {
    return "Yields/Awaits the script's execution until the given signal occurs.";
}

String OScriptNodeAwaitSignal::get_node_title() const {
    return "Await Signal";
}

void OScriptNodeAwaitSignal::on_pin_disconnected(const Ref<OScriptNodePin>& p_pin) {
    // Makes sure that signal list pin changes to string renderer
    if (p_pin.is_valid() && p_pin->get_pin_name().match("target")) {
        _notify_pins_changed();
    }
    super::on_pin_disconnected(p_pin);
}

PackedStringArray OScriptNodeAwaitSignal::get_suggestions(const Ref<OScriptNodePin>& p_pin) {
    if (p_pin.is_valid() && p_pin->is_input() && p_pin->get_pin_name().match("signal_name")) {
        const Ref<OScriptNodePin> target_pin = find_pin("target", PD_Input);
        if (target_pin.is_valid()) {
            return target_pin->resolve_signal_names(true);
        }
    }
    return super::get_suggestions(p_pin);
}
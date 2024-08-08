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
#include "event.h"

#include "common/variant_utils.h"
#include "script/script.h"

String OScriptNodeEvent::get_tooltip_text() const
{
    if (_function.is_valid())
        return vformat("Executes when Godot calls the '%s' function.", _function->get_function_name());

    return super::get_tooltip_text();
}

String OScriptNodeEvent::get_node_title() const
{
    if (_function.is_valid())
        return vformat("%s Event", _function->get_function_name().capitalize());

    return super::get_node_title();
}

StringName OScriptNodeEvent::resolve_type_class(const Ref<OScriptNodePin>& p_pin) const
{
    if (_function.is_valid())
    {
        const int32_t pin_index = p_pin->get_pin_index() - 1;
        if (pin_index >= 0 && pin_index < int(_function->get_argument_count()))
        {
            // Return the specialized "InputEventKey" in this use case.
            if (_function->get_method_info().name.match("_unhandled_key_input"))
                return "InputEventKey";

            return _function->get_method_info().arguments[pin_index].class_name;
        }
    }
    return super::resolve_type_class(p_pin);
}

bool OScriptNodeEvent::is_event_method(const MethodInfo& p_method)
{
    static PackedStringArray method_names = []() {
        PackedStringArray array;
        array.push_back("_enter_tree");
        array.push_back("_exit_tree");
        array.push_back("_gui_input");
        array.push_back("_init");
        array.push_back("_input");
        array.push_back("_notification");
        array.push_back("_physics_process");
        array.push_back("_process");
        array.push_back("_ready");
        array.push_back("_unhandled_input");
        array.push_back("_unhandled_key_input");
        return array;
    }();
    return method_names.has(p_method.name);
}
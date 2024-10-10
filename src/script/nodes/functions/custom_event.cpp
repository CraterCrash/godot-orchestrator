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
#include "custom_event.h"

#include "common/dictionary_utils.h"
#include "common/method_utils.h"
#include "common/variant_utils.h"
#include "common/version.h"
#include "script/script.h"

OScriptNodeCustomEvent::OScriptNodeCustomEvent()
{
    _flags = ScriptNodeFlags::CATALOGABLE;

    // can I initialize things here?
    // const MethodInfo mi = MethodInfo("NewEvent");
    // _function = get_orchestration()->create_function(mi, get_id(), _is_user_defined());
    // _guid = _function->get_guid();
}

String OScriptNodeCustomEvent::get_tooltip_text() const
{
    if (_function.is_valid())
        return "This is an experimental node";

    return super::get_tooltip_text();
}

String OScriptNodeCustomEvent::get_node_title() const
{
    if (_function.is_valid())
        return "Custom Event";

    return super::get_node_title();
}

// eliminated, for now
// String OScriptNodeCustomEvent::get_help_topic() const
// {
//     #if GODOT_VERSION >= 0x040300
//     if (_function.is_valid())
//     {
//         String class_name = MethodUtils::get_method_class(_orchestration->get_base_type(), _function->get_function_name());
//         if (!class_name.is_empty())
//             return vformat("class_method:%s:%s", class_name, _function->get_function_name());
//     }
//     #endif
//     return super::get_help_topic();
// }

StringName OScriptNodeCustomEvent::resolve_type_class(const Ref<OScriptNodePin>& p_pin) const
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

void OScriptNodeCustomEvent::initialize(const OScriptNodeInitContext& p_context)
{
    UtilityFunctions::print("Initializing custom event.");

    // This is what happens in function_entry.cpp
    // const MethodInfo& mi = MethodInfo("NewEvent");
    // _function = get_orchestration()->create_function(mi, get_id(), _is_user_defined());
    // _guid = _function->get_guid();

    // this does not work for some reason...
    // p_context.method = mi

    // This causes an error so long as the context has no method
    // super::initialize(p_context);

    // This works, bypasses parent initialize()
    super::super::initialize(p_context);
}

void OScriptNodeCustomEvent::_get_property_list(List<PropertyInfo>* r_list) const
{
    r_list->push_back(PropertyInfo(Variant::STRING, "function_id", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    r_list->push_back(PropertyInfo(Variant::STRING, "event_name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_STORAGE));
}

bool OScriptNodeCustomEvent::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("function_id"))
    {
        r_value = _guid.to_string();
        return true;
    }
    else if (p_name.match("event_name"))
    {
        r_value = event_name;
        return true;
    }
    return false;
}

bool OScriptNodeCustomEvent::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("function_id"))
    {
        _guid = Guid(p_value);
        return true;
    }
    else if (p_name.match("event_name"))
    {
        event_name = p_value;
        // I'm not sure if this works... the nature of input is that this runs with each character enetered.
        // there is then a whole bunch of non-existant functions created.

        // get_orchestration()-> remove_function(get_function()->get_function_name());

        // const MethodInfo mi = MethodInfo(p_value);
        // _function = get_orchestration()->create_function(mi, get_id(), _is_user_defined());
        // _guid = _function->get_guid();
    }
    return false;
}

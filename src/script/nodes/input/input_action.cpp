// This file is part of the Godot Orchestrator project.
//
// Copyright (c) 2023-present Vahera Studios LLC and its contributors.
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
#include "input_action.h"

#include "common/dictionary_utils.h"
#include "common/property_utils.h"
#include "common/string_utils.h"

#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/project_settings.hpp>

class OScriptNodeInputActionInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeInputAction);

    String _action_name;
    OScriptNodeInputAction::ActionMode _mode;

public:
    int step(OScriptExecutionContext& p_context) override
    {
        Input* input = Input::get_singleton();
        if (!input)
        {
            p_context.set_error("Unable to locate Input singleton.");
            return -1 | STEP_FLAG_END;
        }

        if (_action_name.is_empty())
        {
            p_context.set_error("An action name must be specified.");
            return -1 | STEP_FLAG_END;
        }

        if (_action_name.is_empty())
        {
            p_context.set_error("An action name must be specified.");
            return -1 | STEP_FLAG_END;
        }

        bool result = false;
        switch (_mode)
        {
            case OScriptNodeInputAction::AM_PRESSED:
                result = input->is_action_pressed(_action_name);
                break;
            case OScriptNodeInputAction::AM_RELEASED:
                result = !input->is_action_pressed(_action_name);
                break;
            case OScriptNodeInputAction::AM_JUST_PRESSED:
                result = input->is_action_just_pressed(_action_name);
                break;
            case OScriptNodeInputAction::AM_JUST_RELEASED:
                result = input->is_action_just_released(_action_name);
                break;
        }
        p_context.set_output(0, result);

        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeInputAction::_settings_changed()
{
    // If the node is selected and the user modifies the project settings, this makes sure that the action
    // list will be regenerated in the InspectorDock to reflect the changes potentially to any new InputMap
    // actions that were defined.
    notify_property_list_changed();
}

PackedStringArray OScriptNodeInputAction::_get_action_names() const
{
    PackedStringArray action_names;
    TypedArray<Dictionary> properties = ProjectSettings::get_singleton()->get_property_list();
    for (int i = 0; i < properties.size(); i++)
    {
        const PropertyInfo pi = DictionaryUtils::to_property(properties[i]);
        if (pi.name.begins_with("input/"))
        {
            const String action_name = pi.name.substr(pi.name.find("/") + 1, pi.name.length());
            action_names.push_back(action_name);
        }
    }
    return action_names;
}

String OScriptNodeInputAction::_get_mode() const
{
    switch (_mode)
    {
        case AM_RELEASED:
            return "Released";
        case AM_JUST_PRESSED:
            return "Just Pressed";
        case AM_JUST_RELEASED:
            return "Just Released";
        default:
            return "Pressed";
    }
}

void OScriptNodeInputAction::_get_property_list(List<PropertyInfo>* r_list) const
{
    const String actions = StringUtils::join(",", _get_action_names());
    const String modes = "Pressed,Released,Just Pressed,Just Released";

    r_list->push_back(PropertyInfo(Variant::STRING, "action", PROPERTY_HINT_ENUM, actions));
    r_list->push_back(PropertyInfo(Variant::INT, "mode", PROPERTY_HINT_ENUM, modes));
}

bool OScriptNodeInputAction::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("action"))
    {
        r_value = _action_name;
        return true;
    }
    else if (p_name.match("mode"))
    {
        r_value = _mode;
        return true;
    }
    return false;
}

bool OScriptNodeInputAction::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("action"))
    {
        _action_name = p_value;
        _notify_pins_changed();
        return true;
    }
    else if (p_name.match("mode"))
    {
        _mode = ActionMode(int(p_value));
        _notify_pins_changed();
        return true;
    }
    return false;
}

void OScriptNodeInputAction::_bind_methods()
{
    BIND_ENUM_CONSTANT(AM_PRESSED)
    BIND_ENUM_CONSTANT(AM_RELEASED)
    BIND_ENUM_CONSTANT(AM_JUST_PRESSED)
    BIND_ENUM_CONSTANT(AM_JUST_RELEASED)
}

void OScriptNodeInputAction::post_initialize()
{
    if (_is_in_editor())
    {
        ProjectSettings* settings = ProjectSettings::get_singleton();
        settings->connect("settings_changed", callable_mp(this, &OScriptNodeInputAction::_settings_changed));
    }

    super::post_initialize();
}

void OScriptNodeInputAction::post_placed_new_node()
{
    if (_is_in_editor())
    {
        ProjectSettings* settings = ProjectSettings::get_singleton();
        settings->connect("settings_changed", callable_mp(this, &OScriptNodeInputAction::_settings_changed));
    }

    super::post_placed_new_node();
}

void OScriptNodeInputAction::allocate_default_pins()
{
    create_pin(PD_Output, PT_Data, PropertyUtils::make_typed("state", Variant::BOOL))->set_label(_get_mode());

    super::allocate_default_pins();
}

String OScriptNodeInputAction::get_tooltip_text() const
{
    return "Checks the specified state of an input action.";
}

String OScriptNodeInputAction::get_node_title() const
{
    return vformat("Action %s", _action_name);
}

String OScriptNodeInputAction::get_icon() const
{
    return "InputEventAction";
}

OScriptNodeInstance* OScriptNodeInputAction::instantiate()
{
    OScriptNodeInputActionInstance* i = memnew(OScriptNodeInputActionInstance);
    i->_node = this;
    i->_action_name = _action_name;
    i->_mode = ActionMode(_mode);
    return i;
}

void OScriptNodeInputAction::validate_node_during_build(BuildLog& p_log) const
{
    if (_action_name.is_empty())
        p_log.error(this, "No input action name specified.");
    else if (!_get_action_names().has(_action_name))
        p_log.error(this, "Input action '" + _action_name + "' is not defined.");

    super::validate_node_during_build(p_log);
}

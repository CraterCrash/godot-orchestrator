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
#include "graph_node_pin_input_action.h"

#include "common/callable_lambda.h"
#include "common/macros.h"

#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/project_settings.hpp>

void OrchestratorGraphNodePinInputAction::_populate_action_list()
{
    _button->clear();

    String default_value = _pin->get_effective_default_value();
    bool found = false;

    Ref<ConfigFile> project(memnew(ConfigFile));
    if (project->load("res://project.godot") == OK)
    {
        const PackedStringArray custom_action_names = project->get_section_keys("input");
        for (const String& custom_action_name : custom_action_names)
        {
            _button->add_item(custom_action_name);
            if (custom_action_name.match(default_value))
            {
                _button->select(_button->get_item_count() - 1);
                found = true;
            }
        }
    }

    const TypedArray<StringName> action_names = InputMap::get_singleton()->get_actions();
    for (int i = 0; i < action_names.size(); i++)
    {
        const StringName& action_name = action_names[i];
        if (action_name.begins_with("spatial_editor/"))
            continue;

        _button->add_item(action_name);
        if (action_name.match(default_value))
        {
            _button->select(_button->get_item_count() - 1);
            found = true;
        }
    }

    if (!found)
    {
        _button->select(0);
        _pin->set_default_value(_button->get_item_text(0));
    }
}

Control* OrchestratorGraphNodePinInputAction::_get_default_value_widget()
{
    _button = memnew(OptionButton);
    _button->set_allow_reselect(true);
    _button->get_popup()->set_max_size(Vector2(32768, 400));
    _button->set_tooltip_text("Actions defined in Project Settings: Input Map");

    _button->connect("item_selected", callable_mp_lambda(this, [&](int index) {
        const String action_name = _button->get_item_text(index);
        _pin->set_default_value(action_name);
        _button->release_focus();
    }));

    ProjectSettings::get_singleton()->connect("settings_changed", callable_mp_this(_populate_action_list));

    _populate_action_list();

    return _button;
}

OrchestratorGraphNodePinInputAction::OrchestratorGraphNodePinInputAction(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin)
    : OrchestratorGraphNodePin(p_node, p_pin)
{
}
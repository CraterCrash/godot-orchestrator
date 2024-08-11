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
#include "graph_node_pin_color.h"

#include <godot_cpp/classes/color_picker_button.hpp>
#include <godot_cpp/classes/editor_interface.hpp>

OrchestratorGraphNodePinColor::OrchestratorGraphNodePinColor(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin)
    : OrchestratorGraphNodePin(p_node, p_pin)
{
}

void OrchestratorGraphNodePinColor::_bind_methods()
{
}

void OrchestratorGraphNodePinColor::_on_default_value_changed(const Color& p_new_value)
{
    _pin->set_default_value(p_new_value);
}

Control* OrchestratorGraphNodePinColor::_get_default_value_widget()
{
    const double scale = EditorInterface::get_singleton()->get_editor_scale();

    ColorPickerButton* button = memnew(ColorPickerButton);
    button->set_focus_mode(FOCUS_NONE);
    button->set_h_size_flags(SIZE_EXPAND);
    button->set_custom_minimum_size(Vector2(24.f * scale, 24.f * scale));
    button->set_pick_color(_pin->get_default_value());
    button->connect("color_changed", callable_mp(this, &OrchestratorGraphNodePinColor::_on_default_value_changed));
    return button;
}

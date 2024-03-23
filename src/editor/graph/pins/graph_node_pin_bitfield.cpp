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
#include "editor/graph/pins/graph_node_pin_bitfield.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/grid_container.hpp>
#include <godot_cpp/classes/popup_panel.hpp>

OrchestratorGraphNodePinBitField::OrchestratorGraphNodePinBitField(OrchestratorGraphNode* p_node,
                                                                   const Ref<OScriptNodePin>& p_pin)
    : OrchestratorGraphNodePin(p_node, p_pin)
{
}

void OrchestratorGraphNodePinBitField::_bind_methods()
{
}

void OrchestratorGraphNodePinBitField::_on_bit_toggle(bool p_state, int64_t p_enum_value)
{
    int64_t current_value = _pin->get_effective_default_value();
    if (p_state)
        current_value |= p_enum_value;
    else
        current_value &= p_enum_value;

    _pin->set_default_value(current_value);

    if (_button)
        _button->set_text(vformat("%d", _pin->get_effective_default_value()));
}

void OrchestratorGraphNodePinBitField::_on_hide_flags(PopupPanel* p_panel)
{
    p_panel->queue_free();
}

void OrchestratorGraphNodePinBitField::_on_show_flags()
{
    if (!_button)
        return;

    PopupPanel* panel = memnew(PopupPanel);
    panel->set_size(Vector2(100, 100));
    panel->set_position(_button->get_screen_position() + Vector2(0, _button->get_size().height));
    panel->connect("popup_hide", callable_mp(this, &OrchestratorGraphNodePinBitField::_on_hide_flags).bind(panel));

    const String target_class = _pin->get_target_class();
    if (!target_class.is_empty() && target_class.contains("."))
    {
        const int64_t dot = target_class.find(".");
        const String class_name = target_class.substr(0, dot);
        const String enum_name  = target_class.substr(dot + 1);

        GridContainer* grid = memnew(GridContainer);
        grid->set_columns(2);
        panel->add_child(grid);

        const int default_value = _pin->get_effective_default_value();

        const PackedStringArray bitfield_values = ClassDB::class_get_enum_constants(class_name, enum_name, true);
        for (const String& bitfield : bitfield_values)
        {
            const int64_t enum_value = ClassDB::class_get_integer_constant(class_name, bitfield);

            CheckBox* cb = memnew(CheckBox);
            grid->add_child(cb);

            if (default_value & enum_value)
                cb->set_pressed(true);

            Label* label = memnew(Label);
            label->set_text(bitfield);
            grid->add_child(label);

            cb->connect("toggled", callable_mp(this, &OrchestratorGraphNodePinBitField::_on_bit_toggle).bind(enum_value));
        }
    }

    panel->reset_size();

    // Position panel centered until the button widget
    Vector2 new_position = panel->get_position()
        - Vector2i(panel->get_size().width / 2, 0)
        + Vector2(_button->get_size().width / 2, 0);
    panel->set_position(new_position);

    _button->add_child(panel);
    panel->popup();
}

Control* OrchestratorGraphNodePinBitField::_get_default_value_widget()
{
    _button = memnew(Button);
    _button->set_text(vformat("%d", _pin->get_effective_default_value()));
    _button->set_h_size_flags(SIZE_SHRINK_BEGIN);
    _button->connect("pressed", callable_mp(this, &OrchestratorGraphNodePinBitField::_on_show_flags));
    return _button;
}


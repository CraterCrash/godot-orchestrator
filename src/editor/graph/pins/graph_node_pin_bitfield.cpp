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

#include "api/extension_db.h"
#include "common/scene_utils.h"
#include "common/string_utils.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/grid_container.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/popup_panel.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

OrchestratorGraphNodePinBitField::OrchestratorGraphNodePinBitField(OrchestratorGraphNode* p_node,
                                                                   const Ref<OScriptNodePin>& p_pin)
    : OrchestratorGraphNodePin(p_node, p_pin)
{
}

void OrchestratorGraphNodePinBitField::_bind_methods()
{
}

String OrchestratorGraphNodePinBitField::_get_enum_prefix(const PackedStringArray& p_values)
{
    // Taken from ExtensionDB::_resolve_enum_prefix
    if (p_values.size() == 0)
        return {};

    String prefix = p_values[0];
    // Some Godot enums are prefixed with a trailing underscore, those are our target.
    if (!prefix.contains("_"))
        return {};

    for (const String& value : p_values)
    {
        while (value.find(prefix) != 0)
        {
            prefix = prefix.substr(0, prefix.length() - 1);
            if (prefix.is_empty())
                return {};
        }
    }
    return prefix;
}

void OrchestratorGraphNodePinBitField::_get_bitfield_values(HashMap<String, int64_t>& r_values,
                                                            HashMap<String, String>& r_friendly_names) const
{
    const String target_class = _pin->get_target_class();
    if (!target_class.is_empty())
    {
        if (target_class.contains("."))
        {
            // Class specific bitfield
            const int64_t dot = target_class.find(".");
            const String class_name = target_class.substr(0, dot);
            const String enum_name  = target_class.substr(dot + 1);

            const PackedStringArray bitfield_values = ClassDB::class_get_enum_constants(class_name, enum_name, true);
            const String bitfield_prefix = _get_enum_prefix(bitfield_values);
            for (const String& bitfield : bitfield_values)
            {
                const int64_t enum_value = ClassDB::class_get_integer_constant(class_name, bitfield);
                r_values[bitfield] = enum_value;
                // This friendly logic is part of ExtensionDB::internal::_sanitize_enum
                r_friendly_names[bitfield] = bitfield.replace(bitfield_prefix, "").capitalize();
            }
        }
        else
        {
            // @GlobalScope bitfields
            const EnumInfo& enum_info = ExtensionDB::get_global_enum(target_class);
            if (enum_info.is_bitfield)
            {
                for (const EnumValue& ev : enum_info.values)
                {
                    r_values[ev.name] = ev.value;
                    r_friendly_names[ev.name] = ev.friendly_name;
                }
            }
        }
    }
}

void OrchestratorGraphNodePinBitField::_update_button_value()
{
    if (_button)
    {
        const int64_t value = _pin->get_effective_default_value();
        _button->set_text(vformat("%d", value));
        for (CheckBox* checkbox : _checkboxes)
        {
            int64_t cb_value = checkbox->get_meta("__enum_value");
            checkbox->set_pressed_no_signal(cb_value == value || value & cb_value);
        }
    }
}

void OrchestratorGraphNodePinBitField::_on_bit_toggle(bool p_state, int64_t p_enum_value)
{
    int64_t current_value = _pin->get_effective_default_value();
    if (p_state)
        current_value |= p_enum_value;
    else
        current_value &= ~p_enum_value;

    _pin->set_default_value(current_value);
    _update_button_value();
}

void OrchestratorGraphNodePinBitField::_on_hide_flags(PopupPanel* p_panel)
{
    _checkboxes.clear();
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
    _button->add_child(panel);

    const int64_t default_value = _pin->get_effective_default_value();

    HashMap<String, int64_t> bitfield_values;
    HashMap<String, String> bitfield_friendly_names;
    _get_bitfield_values(bitfield_values, bitfield_friendly_names);

    // Some bitfield enums constants overlap with one another, either where one constant is the exact same
    // value as another constant, i.e. METHOD_FLAGS_NORMAL vs METHOD_FLAGS_DEFAULT, and others where a
    // specific enum constant is a combination of others, i.e. BARRIER_MASK_ALL_BARRIERS.

    // Iterate values and identify which are masks that toggle multiple bits on/off.
    Vector<String> multi_flag_constants;
    for (const KeyValue<String, int64_t>& E : bitfield_values)
    {
        if (E.value != 0 && (E.value - (E.value & -E.value)) != 0)
            multi_flag_constants.push_back(E.key);
    }

    // Create a map of the various names by their respective unique values
    HashMap<int64_t, Vector<String>> duplicate_constants;
    for (const KeyValue<String, int64_t>& E : bitfield_values)
    {
        // We only add the non-multi-flag constants to this list
        if (!multi_flag_constants.has(E.key))
            duplicate_constants[E.value].push_back(bitfield_friendly_names[E.key]);
    }

    GridContainer* grid = memnew(GridContainer);
    grid->set_columns(1);
    panel->add_child(grid);

    // Iterate and add the single value, potential multi-named entries first
    for (const KeyValue<int64_t, Vector<String>>& E : duplicate_constants)
    {
        CheckBox* cb = memnew(CheckBox);
        cb->set_pressed(default_value & E.key);
        cb->set_text(StringUtils::join(" / ", E.value));
        cb->set_meta("__enum_value", E.key);
        grid->add_child(cb);
        cb->connect("toggled", callable_mp(this, &OrchestratorGraphNodePinBitField::_on_bit_toggle).bind(E.key));
        _checkboxes.push_back(cb);
    }

    // If there are any multi-flag constants, add those after the separator
    if (!multi_flag_constants.is_empty())
    {
        grid->add_child(memnew(HSeparator));
        for (const String& flag_name : multi_flag_constants)
        {
            const int64_t value = bitfield_values[flag_name];
            CheckBox* cb = memnew(CheckBox);
            cb->set_pressed(default_value & value);
            cb->set_text(bitfield_friendly_names[flag_name]);
            cb->set_meta("__enum_value", value);
            grid->add_child(cb);
            cb->connect("toggled", callable_mp(this, &OrchestratorGraphNodePinBitField::_on_bit_toggle).bind(value));
            _checkboxes.push_back(cb);
        }
    }

    panel->reset_size();

    // Position panel centered until the button widget
    Vector2 new_position = panel->get_position()
        - Vector2i(panel->get_size().width / 2, 0)
        + Vector2(_button->get_size().width / 2, 0);
    panel->set_position(new_position);
    panel->popup();

    _update_button_value();
}

Control* OrchestratorGraphNodePinBitField::_get_default_value_widget()
{
    _button = memnew(Button);
    _button->set_h_size_flags(SIZE_SHRINK_BEGIN);
    _button->set_button_icon(SceneUtils::get_editor_icon("GuiOptionArrow"));
    _button->set_icon_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
    _button->connect("pressed", callable_mp(this, &OrchestratorGraphNodePinBitField::_on_show_flags));

    _update_button_value();

    return _button;
}


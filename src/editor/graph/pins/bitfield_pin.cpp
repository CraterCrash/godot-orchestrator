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
#include "editor/graph/pins/bitfield_pin.h"

#include "api/extension_db.h"
#include "common/callable_lambda.h"
#include "common/macros.h"
#include "common/string_utils.h"

#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/grid_container.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/popup_panel.hpp>

String OrchestratorEditorGraphPinBitfield::_compute_enum_prefix(const PackedStringArray& p_names)
{
    // Taken from ExtensionDB::_resolve_enum_prefix
    if (p_names.size() == 0)
        return {};

    // Some Godot enums are prefixed with trailing underscores that we want to cleanup
    String prefix = p_names[0];
    if (!prefix.contains("_"))
        return {};

    for (const String& value : p_names)
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

void OrchestratorEditorGraphPinBitfield::_handle_selector_button_pressed()
{
    Button* button = const_cast<Button*>(_get_selector_button());

    PopupPanel* popup = memnew(PopupPanel);
    popup->set_size(Vector2(100, 100));
    popup->set_position(button->get_screen_position() + Vector2(0, button->get_size().height));
    popup->connect("popup_hide", callable_mp_cast(popup, Node, queue_free));
    button->add_child(popup);

    HashMap<String, int64_t> values;
    HashMap<String, String> friendly_names;

    const String target_class = get_property_info().class_name;
    if (!target_class.is_empty() && target_class.contains("."))
    {
        // Class-specific bitfield
        const int64_t dot_index = target_class.find(".");
        const String class_name = target_class.substr(0, dot_index);
        const String enum_name  = target_class.substr(dot_index + 1);

        const PackedStringArray bitfield_values = ClassDB::class_get_enum_constants(class_name, enum_name, true);
        const String prefix = _compute_enum_prefix(bitfield_values);

        for (const String& bitfield : bitfield_values)
        {
            const int64_t value = ClassDB::class_get_integer_constant(class_name, bitfield);
            values[bitfield] = value;
            friendly_names[bitfield] = bitfield.replace(prefix, "").capitalize();
        }
    }
    else if (!target_class.is_empty())
    {
        // @GlobalScope bitfield
        const EnumInfo& info = ExtensionDB::get_global_enum(target_class);
        if (info.is_bitfield)
        {
            for (const EnumValue& enum_value : info.values)
            {
                values[enum_value.name] = enum_value.value;
                friendly_names[enum_value.name] = enum_value.friendly_name;
            }
        }
    }
    else
    {
        // PropertyInfo.hint_string
        const PackedStringArray elements = get_property_info().hint_string.split(",");
        for (int i = 0; i < elements.size(); i++)
        {
            const PackedStringArray parts = elements[i].split(":");
            values[parts[0]] = parts[1].to_int();
            friendly_names[parts[0]] = parts[0];
        }
    }

    Vector<String> multi_value_constants;
    for (const KeyValue<String, int64_t>& E : values)
    {
        if (E.value != 0 && (E.value - (E.value & -E.value)) != 0)
            multi_value_constants.push_back(E.key);
    }

    HashMap<int64_t, Vector<String>> single_value_constants;
    for (const KeyValue<String, int64_t>& E : values)
    {
        if (!multi_value_constants.has(E.key))
            single_value_constants[E.value].push_back(friendly_names[E.key]);
    }

    const int64_t default_value = _get_button_value();

    GridContainer* container = memnew(GridContainer);
    container->set_columns(1);
    popup->add_child(container);

    for (const KeyValue<int64_t, Vector<String>>& E : single_value_constants)
    {
        CheckBox* check = memnew(CheckBox);
        check->set_pressed(default_value & E.key);
        check->set_text(StringUtils::join(" / ", E.value));
        container->add_child(check);
        check->connect("toggled", callable_mp_lambda(this, [this, E] (bool state) {
            int64_t current_value = _get_button_value();
            current_value = state ? (current_value | E.key) : (current_value & ~E.key);

            _handle_selector_button_response(current_value);
        }));
    }

    if (!multi_value_constants.is_empty())
    {
        container->add_child(memnew(HSeparator));

        for (const String& name : multi_value_constants)
        {
            const int64_t value = values[name];

            CheckBox* check = memnew(CheckBox);
            check->set_pressed(default_value & value);
            check->set_text(friendly_names[name]);
            container->add_child(check);
            check->connect("toggled", callable_mp_lambda(this, [this, value] (bool state) {
                int64_t current_value = _get_button_value();
                current_value = state ? (current_value | value) : (current_value & ~value);

                _handle_selector_button_response(current_value);
            }));
        }
    }

    popup->reset_size();

    const Vector2 pos = popup->get_position()
        - Vector2(popup->get_size().width / 2, 0)
        + Vector2(button->get_size().width / 2, 0);

    popup->set_position(pos);
    popup->popup();
}

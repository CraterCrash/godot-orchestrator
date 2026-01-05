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
#include "core/godot/object/bitfield_resolver.h"
#include <godot_cpp/classes/grid_container.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/popup_panel.hpp>

void OrchestratorEditorGraphPinBitfield::_update_checkboxes(bool p_state, const CheckBox* p_box_control)
{
    ERR_FAIL_NULL(p_box_control);
    const int64_t item_value = p_box_control->get_meta("bitmask_value", 0);

    const int64_t button_value = _get_button_value();
    const int64_t new_value = p_state ? button_value | item_value : button_value & ~item_value;

    GridContainer* container = cast_to<GridContainer>(p_box_control->get_parent());
    if (container)
    {
        bool separator_found = false;
        for (uint32_t i = 0; i < container->get_child_count(); i++)
        {
            Node* child = container->get_child(i);
            if (!separator_found)
            {
                HSeparator* separator = cast_to<HSeparator>(child);
                if (!separator)
                    continue;

                separator_found = true;
                continue;
            }

            CheckBox* box = cast_to<CheckBox>(child);
            if (!box && box != p_box_control)
                continue;

            // Only update other boxes, not the one that triggered the event
            const int64_t box_bitmask_value = box->get_meta("bitmask_value", 0);
            box->set_pressed_no_signal((new_value & box_bitmask_value) == box_bitmask_value);
        }
    }

    _handle_selector_button_response(new_value);
}

void OrchestratorEditorGraphPinBitfield::_handle_selector_button_pressed()
{
    using BitfieldItem = BitfieldResolver::BitfieldItem;

    const int64_t default_value = _get_button_value();
    Button* button = const_cast<Button*>(_get_selector_button());

    PopupPanel* popup = memnew(PopupPanel);
    popup->set_size(Vector2(100, 100));
    popup->set_position(button->get_screen_position() + Vector2(0, button->get_size().height));
    popup->connect("popup_hide", callable_mp_cast(popup, Node, queue_free));
    button->add_child(popup);

    GridContainer* container = memnew(GridContainer);
    container->set_columns(1);
    popup->add_child(container);

    const List<BitfieldItem> items = BitfieldResolver::resolve(get_property_info());

    // Create a map of all multivalued bitfield items
    // These will be appended at the end of the widget after a separator.
    HashMap<String, BitfieldItem> multivalued_items;
    for (const BitfieldItem& item : items)
    {
        if (!item.components.is_empty())
            multivalued_items[item.name] = item;
    }

    HashSet<String> added_keys;
    for (const BitfieldItem& item : items)
    {
        if (!multivalued_items.has(item.name) && !added_keys.has(item.name))
        {
            CheckBox* check = memnew(CheckBox);
            check->set_pressed(default_value & item.value);

            PackedStringArray names;
            names.push_back(item.friendly_name);
            for (const BitfieldItem& match : item.matches)
            {
                if (!names.has(match.friendly_name) && !added_keys.has(match.name))
                {
                    names.push_back(match.friendly_name);
                    added_keys.insert(match.name);
                }
            }
            added_keys.insert(item.name);

            check->set_text(StringUtils::join(" / ", names));
            check->set_meta("bitmask_value", item.value);
            check->connect("toggled", callable_mp_this(_update_checkboxes).bind(check));

            container->add_child(check);
        }
    }

    if (!multivalued_items.is_empty())
    {
        container->add_child(memnew(HSeparator));
        for (const KeyValue<String, BitfieldItem>& E : multivalued_items)
        {
            CheckBox* check = memnew(CheckBox);
            check->set_pressed((default_value & E.value.value) == E.value.value);
            check->set_text(E.value.friendly_name);
            check->set_meta("bitmask_value", E.value.value);
            check->connect("toggled", callable_mp_this(_update_checkboxes).bind(check));

            container->add_child(check);
        }
    }

    popup->reset_size();

    const Vector2 pos = popup->get_position()
        - Vector2(popup->get_size().width / 2, 0)
        + Vector2(button->get_size().width / 2, 0);

    popup->set_position(pos);
    popup->popup();
}

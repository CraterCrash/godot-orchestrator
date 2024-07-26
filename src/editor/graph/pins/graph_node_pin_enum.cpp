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
#include "graph_node_pin_enum.h"

#include "api/extension_db.h"
#include "common/string_utils.h"

#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

OrchestratorGraphNodePinEnum::OrchestratorGraphNodePinEnum(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin)
    : OrchestratorGraphNodePin(p_node, p_pin)
{
}

void OrchestratorGraphNodePinEnum::_bind_methods()
{
}

void OrchestratorGraphNodePinEnum::_on_item_selected(int p_index, OptionButton* p_button)
{
    if (p_index >= 0 && p_index < _items.size())
    {
        const ListItem& item = _items[p_index];
        _pin->set_default_value(item.value);
    }

    p_button->release_focus();
}

void OrchestratorGraphNodePinEnum::_generate_items()
{
    _items.clear();

    const String enum_class = _pin->get_target_class();
    if (!enum_class.is_empty() && enum_class.contains(".") && enum_class != "Variant.Type")
    {
        // Represents a nested enum in a Class or BuiltInType
        // Variant.Type is excluded as its treated as a global "enum" despite the dot.
        
        const int64_t dot = enum_class.find(".");
        const String class_name = enum_class.substr(0, dot);
        const String enum_name = enum_class.substr(dot + 1);

        if (ExtensionDB::get_builtin_type_names().has(class_name))
        {
            // Handle BuiltInType
            BuiltInType type = ExtensionDB::get_builtin_type(class_name);
            for (const EnumInfo& E : type.enums)
            {
                for (const EnumValue& V : E.values)
                {
                    ListItem item;
                    item.name = V.name;
                    item.friendly_name = V.friendly_name;
                    item.value = V.value;
                    _items.push_back(item);
                }
            }
        }
        else
        {
            // Handle Nested Class Enum
            const PackedStringArray enum_values = ClassDB::class_get_enum_constants(class_name, enum_name, true);
            const String prefix = _calculate_enum_prefix(enum_values);
            for (int index = 0; index < enum_values.size(); index++)
            {
                ListItem item;
                item.name = enum_values[index];
                item.friendly_name = _generate_friendly_name(prefix, item.name);
                item.value = index;
                _items.push_back(item);
            }
        }
    }
    else if (ExtensionDB::get_global_enum_names().has(enum_class))
    {
        // Handle global enum
        const EnumInfo& ei = ExtensionDB::get_global_enum(enum_class);
        for (const EnumValue& value : ei.values)
        {
            ListItem item;
            item.name = value.name;
            item.friendly_name = StringUtils::default_if_empty(value.friendly_name, value.name);
            item.value = value.value;
            _items.push_back(item);
        }
    }
}

String OrchestratorGraphNodePinEnum::_calculate_enum_prefix(const PackedStringArray& p_values)
{
    if (p_values.size() == 0)
        return {};

    String prefix = p_values[0];

    // Some Godot enums contain underscores, those are our target
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

String OrchestratorGraphNodePinEnum::_generate_friendly_name(const String& p_prefix, const String& p_enum_name)
{
    if (p_prefix.is_empty())
        return p_enum_name.capitalize();

    const bool is_key = p_enum_name.match("Key");
    const bool is_error = p_enum_name.match("Error");
    const bool is_method_flags = p_enum_name.match("MethodFlags");
    const bool is_upper = p_enum_name.match("EulerOrder");

    String friendly_name = p_enum_name.replace(p_prefix, "").capitalize();

    // Handle unique fixups
    if (is_key && friendly_name.begins_with("Kp "))
        friendly_name = friendly_name.substr(3, friendly_name.length()) + " (Keypad)";
    else if (is_key && friendly_name.begins_with("F "))
        friendly_name = friendly_name.replace(" ", "");
    else if (is_error && friendly_name.begins_with("Err "))
        friendly_name = friendly_name.substr(4, friendly_name.length());
    else if (is_method_flags & p_enum_name.match("METHOD_FLAGS_DEFAULT"))
        friendly_name = ""; // Skipped by some nodes

    if (is_upper)
        friendly_name = friendly_name.to_upper();

    return friendly_name;
}

Control* OrchestratorGraphNodePinEnum::_get_default_value_widget()
{
    OptionButton* button = memnew(OptionButton);
    button->connect("item_selected", callable_mp(this, &OrchestratorGraphNodePinEnum::_on_item_selected).bind(button));

    _generate_items();

    uint64_t effective_default = _pin->get_effective_default_value();
    for (const ListItem& item : _items)
    {
        button->add_item(item.friendly_name);
        if (effective_default == item.value)
            button->select(button->get_item_count() - 1);
    }

    return button;
}
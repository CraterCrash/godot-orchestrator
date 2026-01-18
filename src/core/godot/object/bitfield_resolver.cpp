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
#include "core/godot/object/bitfield_resolver.h"

#include "api/extension_db.h"
#include "godot_cpp/core/class_db.hpp"

String BitfieldResolver::_compute_prefix(const PackedStringArray& p_values) {
    if (p_values.size() == 0) {
        return {};
    }

    String prefix = p_values[0];

    // Some Godot enums contain underscores, those are our target
    if (!prefix.contains("_")) {
        return {};
    }

    for (const String& value : p_values) {
        while (value.find(prefix) != 0) {
            prefix = prefix.substr(0, prefix.length() - 1);
            if (prefix.is_empty()) {
                return {};
            }
        }
    }

    return prefix;
}

List<BitfieldResolver::BitfieldItem> BitfieldResolver::_resolve_class_bitfield(const String& p_class_name) {
    List<BitfieldItem> results;

    if (p_class_name.contains(".")) {
        // Class-specific bitfield
        const int64_t last_dot = p_class_name.rfind(".");
        const String class_name = p_class_name.substr(0, last_dot);
        const String enum_name = p_class_name.substr(last_dot + 1);

        // todo: what if its in a global class?
        const PackedStringArray bitfield_values = ClassDB::class_get_enum_constants(class_name, enum_name, true);
        const String prefix = _compute_prefix(bitfield_values);

        for (const String& bitfield : bitfield_values) {
            BitfieldItem item;
            item.name = bitfield;
            item.friendly_name = bitfield.replace(prefix, "").capitalize();
            item.value = ClassDB::class_get_integer_constant(class_name, bitfield);
            results.push_back(item);
        }
    } else {
        // @GlobalScope
        const EnumInfo& enum_info = ExtensionDB::get_global_enum(p_class_name);
        if (enum_info.is_bitfield) {
            for (const EnumValue& enum_value : enum_info.values) {
                BitfieldItem item;
                item.name = enum_value.name;
                item.friendly_name = enum_value.friendly_name;
                item.value = enum_value.value;
                results.push_back(item);
            }
        }
    }
    return results;
}

List<BitfieldResolver::BitfieldItem> BitfieldResolver::_resolve_comma_separated_items(const String& p_hint_string) {
    List<BitfieldItem> results;

    const PackedStringArray items = p_hint_string.split(",", false);
    for (uint32_t i = 0; i < items.size(); i++) {
        const String entry = items[i];

        String item_name;
        int64_t item_value = 0;
        if (entry.contains(":")) {
            item_value = entry.substr(entry.find(":") + 1).to_int();
            item_name = entry.substr(0, entry.find(":"));
        } else {
            item_value = i;
            item_name = entry;
        }

        BitfieldItem item;
        item.name = item_name;
        item.friendly_name = item.name.capitalize();
        item.value = item_value;
        results.push_back(item);
    }

    return results;
}

List<BitfieldResolver::BitfieldItem> BitfieldResolver::resolve(const PropertyInfo& p_property) {
    List<BitfieldItem> results;

    if (!p_property.class_name.is_empty()) {
        results = _resolve_class_bitfield(p_property.class_name);
    } else if (!p_property.hint_string.is_empty()) {
        results = _resolve_comma_separated_items(p_property.hint_string);
    }

    for (BitfieldItem& item : results) {
        if (item.value != 0 && (item.value - (item.value & -item.value)) != 0) {
            // This item is a composite.
            // Iterate bitfields and determine what are its components
            for (const BitfieldItem& check : results) {
                if (check.value != 0 && (item.value & check.value) == check.value && check.name != item.name) {
                    item.components.push_back(check);
                }
            }
        }
    }

    // In some cases two bitmask will have different names but the same values.
    // This provides that cross-reference.
    for (BitfieldItem& item : results) {
        for (const BitfieldItem& check : results) {
            if (item.value != 0 && item.value == check.value && item.name != check.name) {
                item.matches.push_back(check);
            }
        }
    }

    return results;
}
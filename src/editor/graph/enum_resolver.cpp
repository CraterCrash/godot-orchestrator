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
#include "editor/graph/enum_resolver.h"

#include "api/extension_db.h"
#include "common/string_utils.h"
#include "godot_cpp/classes/ref.hpp"
#include "script/script_server.h"

#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/script.hpp>

static String _calculate_enum_prefix(const PackedStringArray& p_values)
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

static String _generate_friendly_name(const String& p_prefix, const String& p_name)
{
    if (p_prefix.is_empty())
        return p_name.capitalize();

    const bool is_key = p_name.match("Key");
    const bool is_error = p_name.match("Error");
    const bool is_method_flags = p_name.match("MethodFlags");
    const bool is_upper = p_name.match("EulerOrder");

    String friendly_name = p_name.replace(p_prefix, "").capitalize();

    // Handle unique fixups
    if (is_key && friendly_name.begins_with("Kp "))
        friendly_name = friendly_name.substr(3, friendly_name.length()) + " (Keypad)";
    else if (is_key && friendly_name.begins_with("F "))
        friendly_name = friendly_name.replace(" ", "");
    else if (is_error && friendly_name.begins_with("Err "))
        friendly_name = friendly_name.substr(4, friendly_name.length());
    else if (is_method_flags & p_name.match("METHOD_FLAGS_DEFAULT"))
        friendly_name = ""; // Skipped by some nodes

    if (is_upper)
        friendly_name = friendly_name.to_upper();

    return friendly_name;
}

List<OrchestratorEditorEnumResolver::EnumItem> OrchestratorEditorEnumResolver::resolve_enum_items(const String& p_target_class)
{
    List<EnumItem> results;

    if (!p_target_class.is_empty() && p_target_class.contains(".") && p_target_class != "Variant.Type")
    {
        // Represents a nested enum in a Class or BuiltInType
        // Variant.Type is excluded as its treated as a global "enum" despite the dot.
        if (p_target_class.begins_with("res://"))
        {
            // Represents an enum that is defined within a Script.
            const int64_t last_dot = p_target_class.rfind(".");
            const String class_name = p_target_class.substr(0, last_dot);
            const String enum_name = p_target_class.substr(last_dot + 1);

            const Ref<Script> script = ResourceLoader::get_singleton()->load(class_name);
            ERR_FAIL_COND_V_MSG(!script.is_valid(), {}, "Failed to load enum " + p_target_class + " from script " + class_name);

            const Dictionary constants = script->get_script_constant_map();

            const Array constant_keys = constants.keys();
            for (int i = 0; i < constant_keys.size(); i++)
            {
                const String& constant_key = constant_keys[i];
                if (constant_key == enum_name)
                {
                    const Dictionary& value = constants[constant_key];
                    const Array value_keys = value.keys();
                    for (int j = 0; j < value_keys.size(); ++j)
                    {
                        const String& value_key = value_keys[j];

                        EnumItem item;
                        item.real_name = value_key;
                        item.friendly_name = value_key.capitalize();
                        item.value = value[value_key];
                        results.push_back(item);
                    }
                }
            }
        }
        else
        {
            const int64_t dot = p_target_class.find(".");
            const String class_name = p_target_class.substr(0, dot);
            const String enum_name = p_target_class.substr(dot + 1);

            if (ExtensionDB::get_builtin_type_names().has(class_name))
            {
                // Handle BuiltInType
                BuiltInType type = ExtensionDB::get_builtin_type(class_name);
                for (const EnumInfo& enum_info : type.enums)
                {
                    for (const EnumValue& enum_value : enum_info.values)
                    {
                        EnumItem item;
                        item.real_name = enum_value.name;
                        item.friendly_name = enum_value.friendly_name;
                        item.value = enum_value.value;
                        results.push_back(item);
                    }
                }
            }
            else if (ClassDB::class_exists(class_name))
            {
                // Handle Nested Class Enum
                const PackedStringArray enum_values = ClassDB::class_get_enum_constants(class_name, enum_name, true);
                const String prefix = _calculate_enum_prefix(enum_values);
                for (int index = 0; index < enum_values.size(); index++)
                {
                    EnumItem item;
                    item.real_name = enum_values[index];
                    item.friendly_name = _generate_friendly_name(prefix, item.real_name);
                    item.value = index;
                    results.push_back(item);
                }
            }
            else if (ScriptServer::is_global_class(class_name))
            {
                Dictionary constants = ScriptServer::get_global_class(class_name).get_constants_list();
                if (constants.has(enum_name))
                {
                    const Dictionary entries = constants[enum_name];
                    const Array entries_keys = entries.keys();

                    for (int i = 0; i < entries_keys.size(); i++)
                    {
                        EnumItem item;
                        item.real_name = entries_keys[i];
                        item.friendly_name = _generate_friendly_name("", item.real_name);
                        item.value = entries[entries_keys[i]];
                        results.push_back(item);
                    }
                }
            }
        }
    }
    else if (ExtensionDB::get_global_enum_names().has(p_target_class))
    {
        // Handle global enum
        const EnumInfo& ei = ExtensionDB::get_global_enum(p_target_class);
        for (const EnumValue& value : ei.values)
        {
            EnumItem item;
            item.real_name = value.name;
            item.friendly_name = StringUtils::default_if_empty(value.friendly_name, value.name);
            item.value = value.value;
            results.push_back(item);
        }
    }

    return results;
}

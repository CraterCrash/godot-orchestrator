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
#ifndef ORCHESTRATOR_CORE_GODOT_OBJECT_ENUM_RESOLVER_H
#define ORCHESTRATOR_CORE_GODOT_OBJECT_ENUM_RESOLVER_H

#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/templates/list.hpp>

using namespace godot;

/// Resolves a <code>PropertyInfo</code> to a list of <code>EnumItem</code>.
class EnumResolver {
public:
    struct EnumItem {
        String name;
        String friendly_name;
        int64_t value = 0;
    };

private:
    static String _calculate_enum_prefix(const PackedStringArray& p_values);
    static String _generate_friendly_name(const String& p_prefix, const String& p_name);

    static List<EnumItem> _resolve_class_enums(const String& p_class_name);
    static List<EnumItem> _resolve_comma_separated_items(const String& p_hint_string);

public:
    static List<EnumItem> resolve(const PropertyInfo& p_property);
};

#endif // ORCHESTRATOR_CORE_GODOT_OBJECT_ENUM_RESOLVER_H
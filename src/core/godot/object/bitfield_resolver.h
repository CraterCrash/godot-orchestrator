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
#ifndef ORCHESTRATOR_CORE_GODOT_OBJECT_BITFIELD_RESOLVER_H
#define ORCHESTRATOR_CORE_GODOT_OBJECT_BITFIELD_RESOLVER_H

#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/templates/local_vector.hpp>

using namespace godot;

/// Resolves a <code>PropertyInfo</code> to a list of <code>BitfieldItem</code>.
class BitfieldResolver {
public:
    struct BitfieldItem {
        String name;
        String friendly_name;
        int64_t value = 0;
        LocalVector<BitfieldItem> components;
        LocalVector<BitfieldItem> matches;
    };

private:
    static String _compute_prefix(const PackedStringArray& p_values);
    static List<BitfieldItem> _resolve_class_bitfield(const String& p_class_name);
    static List<BitfieldItem> _resolve_comma_separated_items(const String& p_hint_string);

public:
    static List<BitfieldItem> resolve(const PropertyInfo& p_property);
};

#endif // ORCHESTRATOR_CORE_GODOT_OBJECT_BITFIELD_RESOLVER_H
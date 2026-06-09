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
#pragma once

#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

namespace GDE {

    struct GodotType {
        enum Kind {
            VARIANT,
            BUILTIN,
            CLASS,
            ENUM,
            BITFIELD,
            UNKNOWN
        };

        Kind kind;
        Variant::Type type;
        String name;
        String icon;
    };

    class TypeResolver {

    public:
        /// Resolves the type from a string-name
        /// @param p_type_name the type name
        /// @return a <code>GodotType</code>
        static GodotType resolve(const String& p_type_name);

        /// Resolves the type from a PropertyInfo
        /// @param p_property the property
        /// @return a <code>GodotType</code>
        static Vector<GodotType> resolve(const PropertyInfo& p_property);
    };
}
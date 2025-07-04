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
#ifndef ORCHESTRATOR_CLASSDB_UTILS_H
#define ORCHESTRATOR_CLASSDB_UTILS_H

#include <godot_cpp/templates/hash_map.hpp>

using namespace godot;

namespace ClassDBUtils
{
    namespace internal
    {
        // Defines a cache of default values from loaded classes
        HashMap<String, HashMap<String, Variant>> default_value_cache; // NOLINT

        /// Clears the default value cache when needed
        _FORCE_INLINE_ void clear_default_value_cache() { default_value_cache.clear(); }
    }

    /// Gets the class property default value
    /// @param p_class_name the class name
    /// @param p_property_name the property name
    /// @return the default value or an empty variant if there is no default
    Variant class_get_property_default_value(const StringName& p_class_name, const StringName& p_property_name);
};

#endif // OORCHESTRATOR_CLASSDB_UTILS_H
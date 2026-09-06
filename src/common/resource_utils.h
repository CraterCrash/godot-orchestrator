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

#include <godot_cpp/classes/resource.hpp>

using namespace godot;

namespace ResourceUtils {

    /// Check whether the resource path is a file
    /// @return true if it is a file path; false otherwise
    bool is_file(const String& p_path);

    /// Collects the persisted state of a resource, the same properties the script serializers write.
    /// Resource base properties, the script, and transient metadata are excluded, as are values that
    /// equal the class default, so consumers must treat an absent property as the default.
    /// @param p_resource the resource
    /// @return property name to value map, in property list order
    Dictionary get_storage_properties(const Ref<Resource>& p_resource);

    /// Reads a property from a map produced by <code>get_storage_properties</code>, falling back to the
    /// class default when the map omits it.
    /// @param p_properties the property map
    /// @param p_class the class the map was collected from
    /// @param p_name the property name
    /// @return the stored value, or the class default
    Variant get_storage_property(const Dictionary& p_properties, const StringName& p_class, const StringName& p_name);

    /// Applies a map produced by <code>get_storage_properties</code> to a resource.
    /// @param p_resource the resource
    /// @param p_properties the property map
    /// @param p_excluded property names that are not applied
    void apply_storage_properties(const Ref<Resource>& p_resource, const Dictionary& p_properties, const Vector<StringName>& p_excluded = Vector<StringName>());

}
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
#ifndef ORCHESTRATOR_RESOURCE_UTILS_H
#define ORCHESTRATOR_RESOURCE_UTILS_H

#include <godot_cpp/classes/resource.hpp>

using namespace godot;

namespace ResourceUtils
{
    /// Checks whether missing resources are used if the class cannot be created
    /// @return true if missing resources should be used, false otherwise
    bool is_creating_missing_resources_if_class_unavailable_enabled();

    /// Sets the resource as edited
    void set_edited(const Ref<Resource>& p_resource, bool p_edited);

    /// Generates the scene unique identifier
    /// @return the unique scene identifier
    String generate_scene_unique_id();

    /// Gets the scene unique id for a resource
    /// @param p_resource the resource, should be valid
    /// @return p_path the resource path
    /// @return the unique id for the resource, but may be empty
    String get_scene_unique_id(const Ref<Resource>& p_resource, const String& p_path = String());

    /// Sets the scene unique id on the resource
    /// @param p_resource the resource, should be valid
    /// @param p_path the resource path
    /// @param p_id the unique id to set
    void set_scene_unique_id(const Ref<Resource>& p_resource, const String& p_path, const String& p_id);

    /// Sets the resource's id
    /// @param p_resource the resource, should be valid
    /// @param p_path the resource path
    /// @param p_id the resource identifier
    void set_id_for_path(const Ref<Resource>& p_resource, const String& p_path, const String& p_id);

    /// Gets the resource id for the given path
    /// @param p_path the path, should never be empty
    /// @param p_generate whether to generate a new id if not found
    /// @return the resource id
    int64_t get_resource_id_for_path(const String& p_path, bool p_generate = true);

    /// Check whether the resource represents a built-in resource
    /// @return true if it is built-in, false otherwise
    bool is_builtin(const Ref<Resource>& p_resource);

    /// Check whether the resource path is a file
    /// @return true if it is a file path; false otherwise
    bool is_file(const String& p_path);

    String get_class(const Ref<Resource>& p_resource);
}

#endif // ORCHESTRATOR_RESOURCE_UTILS_H
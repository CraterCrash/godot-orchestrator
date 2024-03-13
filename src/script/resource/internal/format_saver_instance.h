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
#ifndef ORCHESTRATOR_SCRIPT_FORMAT_SAVER_INSTANCE_H
#define ORCHESTRATOR_SCRIPT_FORMAT_SAVER_INSTANCE_H

#include "resource_format.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/templates/rb_map.hpp>
#include <godot_cpp/templates/vector.hpp>

using namespace godot;

/// A runtime instance for saving Orchestrator scripts
class OScriptResourceSaverInstance : protected OrchestratorResourceFormat
{
    struct NonPersistentKey
    {
        Ref<Resource> base;
        StringName property;

        bool operator<(const NonPersistentKey& p_key) const
        {
            return base == p_key.base ? property < p_key.property : base < p_key.base;
        }
    };

    struct Property
    {
        int name_index;
        Variant value;
        PropertyInfo pi;
    };

    struct ResourceData
    {
        String type;
        List<Property> properties;
    };

    bool big_endian;
    bool relative_paths;
    bool skip_editor;
    bool bundle_resources;
    bool takeover_paths;

    String magic;
    String local_path;
    String path;

    HashSet<Ref<Resource>> resource_set;
    RBMap<NonPersistentKey, Variant> non_persistent_map;
    HashMap<StringName, int> string_map;
    Vector<StringName> strings;
    List<Ref<Resource>> saved_resources;

    /// Find resources within the provided variant
    /// @param p_variant the variant to inspect
    /// @param p_main
    void _find_resources(const Variant& p_variant, bool p_main = false);

    /// Gets the string's index from the string map, adding it if it doesn't exist.
    /// @param p_value the string to lookup and add.
    /// @return the index of the cached string or the new index assigned
    int _get_string_index(const String& p_value);

    /// Save the specified string in the given file in unicode format.
    /// @param p_file the file reference
    /// @param p_value the string to be stored
    /// @param p_bit_on_length ??
    void _save_unicode_string(Ref<FileAccess> p_file, const String& p_value, bool p_bit_on_length = false);

    /// Get the class name of the resource
    /// @param p_resource the resource
    /// @return class name
    String _resource_get_class(Ref<Resource> p_resource);

    /// Writes the variant value to the file
    /// @param p_file the file reference
    /// @param p_value the value to be written
    /// @param p_resource_map the resource map
    /// @param p_string_map the string map
    /// @param p_hint the property information
    void _write_variant(Ref<FileAccess> p_file, const Variant& p_value, HashMap<Ref<Resource>, int>& p_resource_map,
                        HashMap<StringName, int>& p_string_map, const PropertyInfo& p_hint = PropertyInfo());

    /// Checks if the resource is built-in
    /// @param p_resource the resource
    /// @return true if its a built-in resource, otherwise false
    bool _is_resource_built_in(Ref<Resource> p_resource) const;

    void _pad_buffer(Ref<FileAccess> p_file, int size);

public:
    /// Save the specified resource to the given file
    /// @param p_path the file path to be used for persisting the resource
    /// @param p_resource the resource to be written
    /// @param p_flags the flags
    /// @return error code
    Error save(const String& p_path, const Ref<Resource>& p_resource, uint32_t p_flags = 0);
};

#endif  // ORCHESTRATOR_SCRIPT_FORMAT_SAVER_INSTANCE_H

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
#ifndef ORCHESTRATOR_SCRIPT_TEXT_SAVER_INSTANCE_H
#define ORCHESTRATOR_SCRIPT_TEXT_SAVER_INSTANCE_H

#include "common/version.h"
#include "script/serialization/instance.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/rb_map.hpp>

using namespace godot;

/// Defines a resource format instance implementation for saving Orchestrator scripts as text.
class OScriptTextResourceSaverInstance : public OScriptResourceTextFormatInstance
{
protected:
    struct ResourceSort
    {
        Ref<Resource> resource;
        String id;

        bool operator<(const ResourceSort& p_right) const { return id.naturalnocasecmp_to(p_right.id) < 0; }
    };

    struct NonPersistentKey
    {
        Ref<Resource> base;
        StringName property;

        bool operator<(const NonPersistentKey& p_key) const
        {
            return base == p_key.base ? property < p_key.property : base < p_key.base;
        }
    };

    RBMap<NonPersistentKey, Variant> _non_persistent_map;  //! Map of non-persistent properties
    HashSet<Ref<Resource>> _resource_set;                  //! Set of all resources
    HashMap<Ref<Resource>, String> _external_resources;    //! External resources
    HashMap<Ref<Resource>, String> _internal_resources;    //! Internal resources
    List<Ref<Resource>> _saved_resources;                  //! Resources that have been serialized

    HashMap<String, HashMap<String, Variant>> _default_value_cache;  //! Cache of default values

    bool _skip_editor{ false };       //! Whether to skeip editor properties
    bool _relative_paths{ false };    //! Use relative paths
    bool _bundle_resources{ false };  //! Bundle resources in serialization
    bool _take_over_paths{ false };   //! Are paths taken over on save?
    String _local_path;               //! Local path

    static String _write_resources(void* p_userdata, const Ref<Resource>& p_resource);

    /// Write the specified resource
    /// @param p_resource the resource
    /// @return the resource as a string
    String _write_resource(const Ref<Resource>& p_resource);

    /// Find object-based resources
    /// @param p_variant the variant
    /// @param p_main whether the resource is the main resource
    void _find_resources_object(const Variant& p_variant, bool p_main = false);

    /// Find array-based resources
    /// @param p_variant the variant
    /// @param p_main whether the resource is the main resource
    void _find_resources_array(const Variant& p_variant, bool p_main = false);

    /// Find dictionary-based resources
    /// @param p_variant the variant
    /// @param p_main whether the resource is the main resource
    void _find_resources_dictionary(const Variant& p_variant, bool p_main = false);

    /// Get the resource class name
    /// @param p_resource the resource
    /// @return the class name
    String _resource_get_class(const Ref<Resource>& p_resource) const;

    /// Generate a unique scene id
    #if GODOT_VERSION < 0x040300
    String _generate_scene_unique_id();
    #endif

    /// Find resources
    /// @param p_variant the variant
    /// @param p_main whether the resource is the main resource
    void _find_resources(const Variant& p_variant, bool p_main = false);

    /// Gets the default value for the specified property
    /// @param p_class_name the class name
    /// @param p_property the property name
    /// @return the property's default value
    Variant _class_get_property_default_value(const StringName &p_class_name, const StringName &p_property);

public:
    /// Save the resource to the specified path.
    /// @param p_path the path to save the resource to
    /// @param p_resource the resource to save
    /// @param p_flags the flags to use when saving the resource
    /// @return the error code, <code>OK</code> if successful
    Error save(const String& p_path, const Ref<Resource>& p_resource, uint32_t p_flags = 0);

    /// Set the unique identifier
    /// @param p_path the file path
    /// @param p_uid the unique id
    /// @return error code
    Error set_uid(const String& p_path, uint64_t p_uid);
};

#endif // ORCHESTRATOR_SCRIPT_TEXT_SAVER_INSTANCE_H
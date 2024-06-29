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
#ifndef ORCHESTRATOR_SCRIPT_RESOURCE_CACHE_H
#define ORCHESTRATOR_SCRIPT_RESOURCE_CACHE_H

#include "common/version.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/mutex_lock.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/list.hpp>

using namespace godot;

/// An Orchestrator resource cache, mimicking the Godot ResourceCache.
/// This primarily exists to track resource IDs across loads and saves, reducing data diffs.
class ResourceCache
{
    static ResourceCache* _singleton;

    struct CacheEntry
    {
        Variant reference;
        String id;

        bool operator==(const CacheEntry& p_entry) const { return reference == p_entry.reference; }
        bool is_resource(const Ref<Resource>& p_resource) const;
    };

protected:
    Ref<Mutex> _mutex;                                              //! Mutex for the resource cache
    Ref<Mutex> _path_cache_lock;                                    //! Mutex for the resource path cache
    HashMap<String, Resource*> _resources;                          //! Map of resources
    HashMap<String, HashMap<String, String>> _resource_path_cache;  //! Map of resource path to resource IDs
    #if GODOT_VERSION < 0x040400
    HashMap<String, List<CacheEntry>> _resource_scene_unique_ids;   //! Map of scene unique IDs for resources
    #endif

    /// Clears the cache
    void _clear();

public:
    /// Get the singleton instance
    /// @return the resource cache singleton, should never be <code>null</code>
    static ResourceCache* get_singleton() { return _singleton; }

    /// Check if the resource cache has the specified resource
    /// @param p_path the resource path
    /// @return <code>true</code> if the resource cache has the specified resource, <code>false</code> otherwise
    static bool has(const String& p_path) { return get_singleton()->has_path(p_path); }

    /// Check if the resource cache has the specified resource
    /// @param p_path the resource path
    /// @return <code>true</code> if the resource cache has the specified resource, <code>false</code> otherwise
    bool has_path(const String& p_path);

    /// Get a reference to the specified resource
    /// @param p_path the resource path
    /// @return the resource reference, if it exists
    Ref<Resource> get_ref(const String& p_path);

    /// Removes a reference to the specified resource
    /// @param p_path the resouce path
    void remove_ref(const String& p_path);

    /// Adds an entry to the resource cache
    /// @param p_path the file path
    /// @param p_res_path the resource path
    /// @param p_id the resource ID
    void add_path_cache(const String& p_path, const String& p_res_path, const String& p_id);

    /// Removes an entry from the resource cache
    /// @param p_path the file path
    /// @param p_res_path the resource path
    /// @param p_id the resource ID
    void remove_path_cache(const String& p_path, const String& p_res_path, const String& p_id);

    /// Get the resource ID for the specified path.
    /// @param p_path the file path
    /// @param p_res_path the resource path
    /// @return the resource ID
    String get_id_for_path(const String& p_path, const String& p_res_path);

    /// Helper method for Resource::set_id_for_path
    void set_id_for_path(const String& p_path, const String& p_res_path, const String& p_id);

    #if GODOT_VERSION < 0x040400
    String get_scene_unique_id(const String& p_path, const Ref<Resource>& p_resource);
    void set_scene_unique_id(const String& p_path, const Ref<Resource>& p_resource, const String& p_id);
    #endif

    ResourceCache();
    ~ResourceCache();
};

#endif  // ORCHESTRATOR_SCRIPT_RESOURCE_CACHE_H
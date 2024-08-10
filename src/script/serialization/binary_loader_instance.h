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
#ifndef ORCHESTRATOR_SCRIPT_BINARY_LOADER_INSTANCE_H
#define ORCHESTRATOR_SCRIPT_BINARY_LOADER_INSTANCE_H

#include "script/serialization/instance.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/resource_format_loader.hpp>
#include <godot_cpp/classes/resource_uid.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/templates/vector.hpp>

using namespace godot;

/// Forward declarations
class OScriptBinaryResourceLoader;

/// A runtime instance that can load a binary Orchestrator resource format
class OScriptBinaryResourceLoaderInstance : protected OScriptResourceBinaryFormatInstance
{
    friend class OScriptBinaryResourceLoader;

    // Represents an internal resource
    struct InternalResource
    {
        String path;
        uint64_t offset;
    };

    struct ExternalResource
    {
        String path;
        String type;
        int64_t uid{ ResourceUID::INVALID_ID };
    };

    // todo: add support for _uid & flags

    Error _error;                                          //! The error code
    bool _translation_remapped{ false };                   //! Whether translations are remapped
    Ref<Resource> _resource;                               //! The resource, if loaded
    Ref<FileAccess> _file;                                 //! The file instance to read from
    String _local_path;                                    //! The local resource path
    String _resource_path;                                 //! The resource path
    String _type;                                          //! The resource type
    String _script_class;                                  //! The script class
    uint32_t _version;                                     //! The resource file format version
    uint64_t _godot_version;                               //! The Godot version used to save the resource last
    uint64_t _uid;                                         //! The resource's unique identifier
    Vector<char> _string_buffer;                           //! The string buffer
    List<Ref<Resource>> _resource_cache;                   //! The resource cache
    Vector<StringName> _string_map;                        //! The string map
    Vector<InternalResource> _internal_resources;          //! Collection of internal resources
    Vector<ExternalResource> _external_resources;          //! Collection of external resources
    HashMap<String, Ref<Resource>> _internal_index_cache;  //! The internal index cache
    ResourceFormatLoader::CacheMode _cache_mode;           //! The cache mode
    ResourceFormatLoader::CacheMode _cache_mode_ext;       //! The cache mode for external resources
    HashMap<String, String> _remaps;                       //! Remap cache

    /// Check whether the path has been cached
    /// @param p_path the resource path
    /// @return true if the path is cached, false otherwise
    static bool _is_cached(const String& p_path);

    /// Get the cached resource
    /// @param p_path the resource path
    /// @return the cached resource if it exists, otherwise an invalid reference
    static Ref<Resource> _get_cached_ref(const String& p_path);

    /// Read the string from the file
    /// @return the string value
    String _read_string();

    /// Parse the variant from the file stream
    /// @param r_value the returned parsed variant value
    /// @return parser error code, OK if the parse was successful
    Error _parse_variant(Variant& r_value);

    /// Advance the file straem based on the padding size
    /// @param p_file the file stream, should be valid
    /// @param p_size the padding size
    static void _advance_padding(const Ref<FileAccess>& p_file, int p_size);

public:
    /// Gets all dependencies
    /// @param p_file the opened file stream
    /// @param p_add_types whether to add types
    /// @return the list of dependencies
    PackedStringArray get_dependencies(const Ref<FileAccess>& p_file, bool p_add_types);

    /// Rename dependencies
    /// @param p_file the opened file stream
    /// @param p_path the file path that was opened
    /// @param p_renames the dependencies to be renamed, mapped old to new filenames
    /// @return the error status code
    Error rename_dependencies(const Ref<FileAccess>& p_file, const String& p_path, const Dictionary& p_renames);

    /// Get the script class
    /// @param p_file the opened file stream
    /// @return the script class name
    String recognize_script_class(const Ref<FileAccess>& p_file);

    /// Opens the resource file stream
    /// @param p_file the opened file stream
    /// @param p_no_resources whether to load the resources
    /// @param p_keep_uuid_paths whether to keep the uuid paths
    void open(const Ref<FileAccess>& p_file, bool p_no_resources = false, bool p_keep_uuid_paths = false);

    /// Loads the resource contents from the file stream
    /// @return OK if the load was successful
    Error load();

    /// Get the classes used in the resource file
    /// @return packed string array of all class names used
    PackedStringArray get_classes_used(const Ref<FileAccess>& p_file);

    /// Constructs the binary resource loader instance
    OScriptBinaryResourceLoaderInstance();
};

#endif  // ORCHESTRATOR_SCRIPT_BINARY_LOADER_INSTANCE_H
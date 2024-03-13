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
#ifndef ORCHESTRATOR_SCRIPT_FORMAT_LOADER_INSTANCE_H
#define ORCHESTRATOR_SCRIPT_FORMAT_LOADER_INSTANCE_H

#include "resource_format.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/resource_format_loader.hpp>
#include <godot_cpp/classes/resource_uid.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/templates/vector.hpp>

using namespace godot;

/// A runtime instance for loading Orchestrator scripts
class OScriptResourceLoaderInstance : protected OrchestratorResourceFormat
{
    friend class OScriptResourceLoader;

    struct InternalResource
    {
        String path;
        uint64_t offset;
    };

    bool translation_remapped = false;

    uint32_t version{ 0 };
    uint64_t godot_version{ 0 };
    uint64_t imported_offsets{ 0 };
    uint64_t uid = ResourceUID::INVALID_ID;

    String local_path;
    String res_path;
    String type;

    Ref<Resource> resource;
    Ref<FileAccess> f;

    Vector<char> string_buffer;
    List<Ref<Resource>> resource_cache;
    Vector<StringName> string_map;
    Vector<InternalResource> internal_resources;
    HashMap<String, Ref<Resource>> internal_index_cache;

    ResourceFormatLoader::CacheMode cache_mode = ResourceFormatLoader::CACHE_MODE_REUSE;

    static bool is_cached(const String& p_path);

    /// Read a unicode encoded string from the file
    /// @return the string value
    String _read_unicode_string();

    /// Read a string from the file
    /// @return the string value
    String _read_string();

    /// Parse the variant from the stream
    /// @param r_value the returned variant
    /// @return parser error code
    Error _parse_variant(Variant& r_value);

    void _advance_padding(Ref<FileAccess>& p_file, int p_size);

public:
    Error load(const Ref<FileAccess>& p_file);
};

#endif  // ORCHESTRATOR_SCRIPT_FORMAT_LOADER_INSTANCE_H
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
#ifndef ORCHESTRATOR_ORCHESTRATION_PARSER_BINARY_H
#define ORCHESTRATOR_ORCHESTRATION_PARSER_BINARY_H

#include "orchestration/serialization/parser.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/missing_resource.hpp>
#include <godot_cpp/classes/resource_format_loader.hpp>
#include <godot_cpp/classes/resource_uid.hpp>
#include <godot_cpp/templates/hash_map.hpp>

/// Parser that reads binary-based files and produces an <code>Orchestration</code> resource.
class OrchestrationBinaryParser : public OrchestrationParser {

    struct InternalResource {
        String path;
        uint64_t offset;
    };

    struct ExternalResource {
        String path;
        String type;
        int64_t uid = ResourceUID::INVALID_ID;
    };

    Ref<FileAccess> _file;
    uint64_t _header_block_size = 0;
    uint64_t _resource_metadata_block_size = 0;

    Vector<char> _string_buffer;
    Vector<StringName> _string_map;

    List<Ref<Resource>> _resource_cache;
    HashMap<String, Ref<Resource>> _internal_index_cache;

    Vector<ExternalResource> _external_resources;
    Vector<InternalResource> _internal_resources;
    HashMap<String, String> _remaps;

    uint32_t _version = 1;
    uint32_t _godot_version = 0;
    uint32_t _flags = 0;

    bool _translation_remapped = false;

    String _path;
    String _type;
    String _resource_type;
    String _script_class;
    String _icon_path;
    int64_t _uid = ResourceUID::INVALID_ID;

    String _error_text;

    Ref<Resource> _resource;

    ResourceFormatLoader::CacheMode _cache_mode = ResourceFormatLoader::CACHE_MODE_REUSE;
    ResourceFormatLoader::CacheMode _cache_mode_for_external = ResourceFormatLoader::CACHE_MODE_REUSE;

    static bool _is_cached(const String& p_path);
    static Ref<Resource> _get_cache_ref(const String& p_path);

    String _read_unicode_string();
    String _read_string();
    Error _parse_variant(Variant& r_value);

    Error _read_header_block();
    Error _read_resource_metadata(bool p_keep_uuid_paths);
    Error _load_resource_properties(Ref<Resource>& r_resource, MissingResource* r_missing_resource);

    Error _open(const Ref<FileAccess>& p_file, bool p_no_resources = false, bool p_keep_uuid_paths = false);
    Error _load();

public:
    //~ Begin OrchestrationParser Interface
    String get_resource_script_class(const String& p_path) override;
    int64_t get_resource_uid(const String& p_path) override;
    PackedStringArray get_dependencies(const String& p_path, bool p_add_types) override;
    Error rename_dependencies(const String& p_path, const Dictionary& p_renames) override;
    PackedStringArray get_classes_used(const String& p_path) override;
    Variant load(const String& p_path) override;
    //~ End OrchestrationParser Interface
};

#endif // ORCHESTRATOR_ORCHESTRATION_PARSER_BINARY_H
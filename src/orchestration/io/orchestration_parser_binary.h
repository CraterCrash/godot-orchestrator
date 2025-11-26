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

#include "orchestration/io/orchestration_parser.h"
#include "orchestration_stream.h"

#include <godot_cpp/classes/resource_uid.hpp>
#include <godot_cpp/templates/hash_map.hpp>

using namespace godot;

/// Responsible for parsing an orchestration's binary-based source.
class OrchestrationBinaryParser : public OrchestrationParser
{
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

    String _res_type;
    String _script_class;
    uint32_t _version{ 0 };
    uint64_t _godot_version{ 0 };
    uint32_t _flags{ 0 };
    uint64_t _res_uid = ResourceUID::INVALID_ID;
    ResourceFormatLoader::CacheMode _cache_mode = ResourceFormatLoader::CACHE_MODE_REUSE;
    HashMap<String, String> _remaps; // todo: always empty
    Vector<InternalResource> _internal_resources;           //! Internal resource metadata record
    Vector<ExternalResource> _external_resources;           //! External resource metadata record
    Vector<char> _string_buffer;
    Vector<StringName> _string_map;
    List<Ref<Resource>> _resource_cache;                    //! All constructed resources during parse.
    HashMap<String, Ref<Resource>> _internal_index_cache;   //! Internal resource path to reference lookup during parse.
    bool _keep_uuid_paths{ false };
    bool _translation_remapped{ false };

    String _read_string(OrchestrationByteStream& p_stream);

    Error _parse_magic(OrchestrationByteStream& p_stream);
    Error _parse_header(OrchestrationByteStream& p_stream);
    Error _parse_string_map(OrchestrationByteStream& p_stream);
    Error _parse_resource_metadata(OrchestrationByteStream& p_stream);
    Error _parse_resource(OrchestrationByteStream& p_stream, Ref<Orchestration>& r_orchestration);
    Error _parse_variant(OrchestrationByteStream& p_stream, Variant& r_value);

    Error _parse(const String& p_path, ResourceFormatLoader::CacheMode p_cache_mode, bool p_parse_resources = true);

public:
    //~ Begin OrchestrationParser Interface
    Ref<Orchestration> parse(const Variant& p_source, const String& p_path, ResourceFormatLoader::CacheMode p_cache_mode) override;
    int64_t get_uid(const String& p_path) override;
    String get_script_class(const String& p_path) override;
    PackedStringArray get_classes_used(const String& p_path) override;
    PackedStringArray get_dependencies(const String& p_path, bool p_add_types) override;
    Error rename_dependencies(const String& p_path, const Dictionary& p_renames) override;
    //~ End OrchestrationParser Interface

    ~OrchestrationBinaryParser() override = default;
};

#endif // ORCHESTRATOR_ORCHESTRATION_PARSER_BINARY_H
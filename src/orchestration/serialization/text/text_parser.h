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
#ifndef ORCHESTRATOR_ORCHESTRATION_PARSER_TEXT_H
#define ORCHESTRATOR_ORCHESTRATION_PARSER_TEXT_H

#include "orchestration/serialization/parser.h"
#include "orchestration/serialization/text/variant_parser.h"

#include <godot_cpp/classes/resource_format_loader.hpp>
#include <godot_cpp/classes/resource_uid.hpp>

/// Parser that reads text-based files and produces an <code>Orchestration</code> resource.
class OrchestrationTextParser : public OrchestrationParser {

    struct ExternalResource {
        String path;
        String type;
        Ref<Resource> resource; // todo: convert to load tokens
    };

    // For converters
    class DummyResource : public Resource {
    };

    struct DummyReadData {
        bool no_placeholders = false;
        HashMap<Ref<Resource>, int> external_resources;
        HashMap<String, Ref<Resource>> rev_external_resources;
        HashMap<Ref<Resource>, int> resource_index_map;
        HashMap<String, Ref<Resource>> resource_map;
    };

    OScriptVariantParser::StreamFile _stream;
    OScriptVariantParser::ResourceParser _rp;
    OScriptVariantParser::Tag _next_tag;

    HashMap<String, ExternalResource> _external_resources;
    HashMap<String, Ref<Resource>> _internal_resources;
    HashMap<String, String> _remaps;

    uint32_t _version = 1;

    bool _translation_remapped = false;
    bool _is_scene = false;
    bool _ignore_resource_parsing = false;
    bool _use_subthreads = false;

    String _path;
    String _type;
    String _resource_type;
    String _script_class;
    int64_t _uid = ResourceUID::INVALID_ID;

    String _error_text;
    int _lines = 0;

    int _resources_total = 0;
    int _resources_current = 0;

    float* _progress = nullptr;

    ResourceFormatLoader::CacheMode _cache_mode = ResourceFormatLoader::CACHE_MODE_REUSE;
    ResourceFormatLoader::CacheMode _cache_mode_for_external = ResourceFormatLoader::CACHE_MODE_REUSE;

    Ref<Resource> _resource;

    String _remap_class(const String& p_class);

    static Error _parse_sub_resource_dummys(void* p_self, OScriptVariantParser::Stream* p_stream, Ref<Resource>& r_res, int& r_line, String& r_err_string);
    static Error _parse_sub_resource_dummy(DummyReadData* p_data, OScriptVariantParser::Stream* p_stream, Ref<Resource>& r_res, int& r_line, String& r_err_string);

    static Error _parse_ext_resource_dummys(void* p_self, OScriptVariantParser::Stream* p_stream, Ref<Resource>& r_res, int& r_line, String& r_err_string);
    static Error _parse_ext_resource_dummy(DummyReadData* p_data, OScriptVariantParser::Stream* p_stream, Ref<Resource>& r_res, int& r_line, String& r_err_string);

    static Error _parse_sub_resources(void* p_self, OScriptVariantParser::Stream* p_stream, Ref<Resource>& r_res, int& r_line, String& r_err_string);
    static Error _parse_ext_resources(void* p_self, OScriptVariantParser::Stream* p_stream, Ref<Resource>& r_res, int& r_line, String& r_err_string);

    Error _parse_sub_resource(OScriptVariantParser::Stream* p_stream, Ref<Resource>& r_res, int& r_line, String& r_err_string);
    Error _parse_ext_resource(OScriptVariantParser::Stream* p_stream, Ref<Resource>& r_res, int& r_line, String& r_err_string);

    Error _open(const Ref<FileAccess>& p_file, bool p_skip_first_tag = false);
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

    Error set_uid(const String& p_path, int64_t p_uid);
};

#endif // ORCHESTRATOR_ORCHESTRATION_PARSER_TEXT_H
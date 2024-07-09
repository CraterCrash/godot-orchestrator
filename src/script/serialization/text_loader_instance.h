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
#ifndef ORCHESTRATOR_SCRIPT_TEXT_LOADER_INSTANCE_H
#define ORCHESTRATOR_SCRIPT_TEXT_LOADER_INSTANCE_H

#include "script/serialization/instance.h"
#include "script/serialization/variant_parser.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource_format_loader.hpp>
#include <godot_cpp/classes/resource_uid.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/templates/hash_map.hpp>

using namespace godot;

/// Forward declarations
class OrchestratorEditorExportPlugin;
class OScriptTextResourceLoader;

/// Defines a resource format instance implementation for loading Orchestrator scripts from text.
class OScriptTextResourceLoaderInstance : public OScriptResourceFormatInstance
{
    friend class OScriptTextResourceLoader;
    friend class OrchestratorEditorExportPlugin;

    struct ExtResource
    {
        String path;
        String type;
        // todo: convert to load token
        Ref<Resource> resource;
    };

    OScriptVariantParser::StreamFile _stream;
    OScriptVariantParser::ResourceParser _rp;
    OScriptVariantParser::Tag _next_tag;

    Ref<FileAccess> _file;
    HashMap<String, ExtResource> _external_resources;
    HashMap<String, Ref<Resource>> _internal_resources;
    HashMap<String, String> _remaps;

    bool _translation_remapped{ false };
    bool _is_scene{ false };
    bool _ignore_resource_parsing{ false };
    bool _use_subthreads{ false };

    String _res_type;
    String _local_path;
    String _res_path;
    String _error_text;
    String _resource_type;
    String _script_class;

    int _resources_total{ 0 };
    int _resource_current{ 0 };
    mutable int _lines{ 0 };

    float* _progress{ nullptr };

    ResourceFormatLoader::CacheMode _cache_mode = ResourceFormatLoader::CACHE_MODE_REUSE;
    ResourceFormatLoader::CacheMode _cache_mode_for_external = ResourceFormatLoader::CACHE_MODE_REUSE;
    int64_t _res_uid = ResourceUID::INVALID_ID;

    Error _error{ OK };

    uint32_t _version{ 1 };
    Ref<Resource> _resource;

    /// For converters
    class DummyResource : public Resource
    {
    };

    struct DummyReadData
    {
        bool no_placeholderss{ false };
        HashMap<Ref<Resource>, int> external_resources;
        HashMap<String, Ref<Resource>> rev_external_resources;
        HashMap<Ref<Resource>, int> resource_index_map;
        HashMap<String, Ref<Resource>> resource_map;
    };

    /// Return whether to create missing resources if unavailable
    /// @return <code>true</code> if in editor; <code>false</code> otherwise
    bool _is_creating_missing_resources_if_class_unavailable_enabled() const;

    static Error _parse_sub_resource_dummys(void* p_self, OScriptVariantParser::Stream* p_stream, Ref<Resource>& r_res, int& r_line, String& r_err_string);
    static Error _parse_sub_resource_dummy(DummyReadData* p_data, OScriptVariantParser::Stream* p_stream, Ref<Resource>& r_res, int& r_line, String& r_err_string);

    static Error _parse_ext_resource_dummys(void* p_self, OScriptVariantParser::Stream* p_stream, Ref<Resource>& r_res, int& r_line, String& r_err_string);
    static Error _parse_ext_resource_dummy(DummyReadData* p_data, OScriptVariantParser::Stream* p_stream, Ref<Resource>& r_res, int& r_line, String& r_err_string);

    static Error _parse_sub_resources(void* p_self, OScriptVariantParser::Stream* p_stream, Ref<Resource>& r_res, int& r_line, String& r_err_string);
    static Error _parse_ext_resources(void* p_self, OScriptVariantParser::Stream* p_stream, Ref<Resource>& r_res, int& r_line, String& r_err_string);

    Error _parse_sub_resource(OScriptVariantParser::Stream* p_stream, Ref<Resource>& r_res, int& r_line, String& r_err_string);
    Error _parse_ext_resource(OScriptVariantParser::Stream* p_stream, Ref<Resource>& r_res, int& r_line, String& r_err_string);

public:
    /// Opens the file provided
    /// @param p_file the file resource to open/read
    /// @param p_skip_first_tag whether to skip the first tag
    void open(const Ref<FileAccess>& p_file, bool p_skip_first_tag = false);

    /// Completes the loading of the text resource
    /// @return the error code, <code>OK</code> if successful
    Error load();

    /// Get the resource uid for the file
    /// @param p_file the file
    /// @return the uid
    int64_t get_uid(const Ref<FileAccess>& p_file);

    /// Get the classes used in the resource file
    /// @return packed string array of all class names used
    PackedStringArray get_classes_used(const Ref<FileAccess>& p_file);

    /// Constructs the text resource loader instance
    OScriptTextResourceLoaderInstance();
};

#endif // ORCHESTRATOR_SCRIPT_TEXT_LOADER_INSTANCE_H
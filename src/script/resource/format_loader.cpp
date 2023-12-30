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
#include "format_loader.h"

#include "common/logger.h"
#include "internal/format_loader_instance.h"

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/time.hpp>

PackedStringArray OScriptResourceLoader::_get_recognized_extensions() const
{
    return { Array::make(ORCHESTRATOR_SCRIPT_EXTENSION) };
}

bool OScriptResourceLoader::_recognize_path(const String& p_path, const StringName& type) const
{
    return p_path.ends_with(ORCHESTRATOR_SCRIPT_QUALIFIED_EXTENSION);
}

bool OScriptResourceLoader::_handles_type(const StringName& p_type) const
{
    return p_type.match(ORCHESTRATOR_SCRIPT_TYPE);
}

String OScriptResourceLoader::_get_resource_type(const String& p_path) const
{
    return p_path.ends_with(ORCHESTRATOR_SCRIPT_QUALIFIED_EXTENSION) ? ORCHESTRATOR_SCRIPT_TYPE : "";
}

String OScriptResourceLoader::_get_resource_script_class(const String& p_path) const
{
    return p_path.ends_with(ORCHESTRATOR_SCRIPT_QUALIFIED_EXTENSION) ? ORCHESTRATOR_SCRIPT_TYPE : "";
}

int64_t OScriptResourceLoader::_get_resource_uid(const String& p_path) const
{
    return ResourceUID::INVALID_ID;
}

PackedStringArray OScriptResourceLoader::_get_dependencies(const String& p_path, bool p_add_types) const
{
    // We have no dependencies
    return {};
}

Error OScriptResourceLoader::_rename_dependencies(const String& p_path, const Dictionary& p_renames) const
{
    // We have no dependencies
    return OK;
}

bool OScriptResourceLoader::_exists(const String& p_path) const
{
    return FileAccess::file_exists(p_path);
}

PackedStringArray OScriptResourceLoader::_get_classes_used(const String& p_path) const
{
    // We don't use any classes
    return {};
}

Variant OScriptResourceLoader::_load(const String& p_path, const String& p_original_path, bool p_use_threads,
                                                int32_t p_cache_mode) const
{
    Ref<FileAccess> f = FileAccess::open_compressed(p_path, FileAccess::READ);
    ERR_FAIL_COND_V_MSG(!f.is_valid(), Variant(), "Cannot open file '" + p_path + "'");

    const String path = !p_original_path.is_empty() ? p_original_path : p_path;
    const String local_path = ProjectSettings::get_singleton()->localize_path(path);

    OScriptResourceLoaderInstance loader;
    loader.cache_mode = (ResourceFormatLoader::CacheMode)p_cache_mode;
    loader.local_path = local_path;
    loader.res_path = loader.local_path;

    Error result = loader.load(f);
    if (result != OK)
        return {};

    return loader.resource;
}

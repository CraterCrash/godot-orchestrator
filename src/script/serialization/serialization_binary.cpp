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
#include "script/serialization/serialization_binary.h"

#include "orchestration/io/orchestration_parser_binary.h"
#include "orchestration/io/orchestration_serializer_binary.h"
#include "script/script.h"
#include "script/serialization/format_defs.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_uid.hpp>

PackedStringArray OScriptBinaryResourceLoader::_get_recognized_extensions() const
{
    return Array::make(ORCHESTRATOR_SCRIPT_EXTENSION);
}

bool OScriptBinaryResourceLoader::_recognize_path(const String& p_path, const StringName& type) const
{
    return p_path.ends_with(ORCHESTRATOR_SCRIPT_QUALIFY_EXTENSION(ORCHESTRATOR_SCRIPT_EXTENSION));
}

bool OScriptBinaryResourceLoader::_handles_type(const StringName& p_type) const
{
    return p_type.match(OScript::get_class_static());
}

String OScriptBinaryResourceLoader::_get_resource_type(const String& p_path) const
{
    if (p_path.ends_with(ORCHESTRATOR_SCRIPT_QUALIFY_EXTENSION(ORCHESTRATOR_SCRIPT_EXTENSION)))
        return OScript::get_class_static();

    return "";
}

String OScriptBinaryResourceLoader::_get_resource_script_class(const String& p_path) const
{
    if (!_get_recognized_extensions().has(p_path.get_extension()))
        return "";

    OrchestrationBinaryParser parser;
    return parser.get_script_class(p_path);
}

int64_t OScriptBinaryResourceLoader::_get_resource_uid(const String& p_path) const
{
    if (!_get_recognized_extensions().has(p_path.get_extension()))
        return ResourceUID::INVALID_ID;

    OrchestrationBinaryParser parser;
    return parser.get_uid(p_path);
}

PackedStringArray OScriptBinaryResourceLoader::_get_dependencies(const String& p_path, bool p_add_types) const
{
    OrchestrationBinaryParser parser;
    return parser.get_dependencies(p_path, p_add_types);
}

Error OScriptBinaryResourceLoader::_rename_dependencies(const String& p_path, const Dictionary& p_renames) const
{
    OrchestrationBinaryParser parser;

    const Error result = parser.rename_dependencies(p_path, p_renames);
    if (result != OK)
        return result;

    // todo: the problem is that if the orchestration is opened & modified,
    //       this will cause any pending edits to be lost if the user does
    //       not save the orchestration. This is a universal Godot issue.

    const Ref<DirAccess> dir = DirAccess::open("res://");
    if (!dir.is_valid())
        return FAILED;

    const String depren_file = vformat("%s.depren", p_path);
    if (dir->remove(p_path) != OK)
    {
        dir->remove(depren_file);
        return FAILED;
    }

    return dir->rename(depren_file, p_path);
}

bool OScriptBinaryResourceLoader::_exists(const String& p_path) const
{
    return FileAccess::file_exists(p_path);
}

PackedStringArray OScriptBinaryResourceLoader::_get_classes_used(const String& p_path) const
{
    OrchestrationBinaryParser parser;
    return parser.get_classes_used(p_path);
}

Variant OScriptBinaryResourceLoader::_load(const String& p_path, const String& p_original_path, bool p_use_threads, int32_t p_cache_mode) const
{
    const Ref<FileAccess> file = FileAccess::open_compressed(p_path, FileAccess::READ);
    ERR_FAIL_COND_V_MSG(!file.is_valid(), nullptr, "Cannot open file '" + p_path + "'");

    const String path = !p_original_path.is_empty() ? p_original_path : p_path;
    const String local_path = ProjectSettings::get_singleton()->localize_path(p_path);
    const PackedByteArray source_code = file->get_buffer(file->get_length());

    OrchestrationBinaryParser parser;
    const Ref<Orchestration> orchestration = parser.parse(source_code, path, static_cast<CacheMode>(p_cache_mode));
    if (orchestration.is_null())
    {
        ERR_PRINT("Failed to parse " + p_path + ": " + parser.get_error_text());
        return {};
    }

    Ref<OScript> script;
    script.instantiate();
    if (script.is_valid())
    {
        if (p_cache_mode != CACHE_MODE_IGNORE)
        {
            if (!ResourceLoader::get_singleton()->has_cached(p_path))
                script->set_path(local_path);
        }
        else
            script->set_path_cache(local_path);

        script->set_orchestration(orchestration);
    }
    return script;
}

void OScriptBinaryResourceLoader::_bind_methods()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


String OScriptBinaryResourceSaver::_get_local_path(const String& p_path) const
{
    return ProjectSettings::get_singleton()->localize_path(p_path);
}

PackedStringArray OScriptBinaryResourceSaver::_get_recognized_extensions(const Ref<Resource>& p_resource) const
{
    if (p_resource->get_name().ends_with(ORCHESTRATOR_SCRIPT_QUALIFY_EXTENSION(ORCHESTRATOR_SCRIPT_EXTENSION)))
        return Array::make(ORCHESTRATOR_SCRIPT_EXTENSION);

    return {};
}

bool OScriptBinaryResourceSaver::_recognize(const Ref<Resource>& p_resource) const
{
    // Currently allow saving any resource object as OScript format
    // todo: should we restrict this?
    return true;
}

Error OScriptBinaryResourceSaver::_set_uid(const String& p_path, int64_t p_uid)
{
    // todo: needs implementation
    return OK;
}

bool OScriptBinaryResourceSaver::_recognize_path(const Ref<Resource>& p_resource, const String& p_path) const
{
    return p_path.ends_with(ORCHESTRATOR_SCRIPT_QUALIFY_EXTENSION(ORCHESTRATOR_SCRIPT_EXTENSION));
}

Error OScriptBinaryResourceSaver::_save(const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags)
{
    const Ref<OScript> script = p_resource;
    if (!script.is_valid())
        return ERR_INVALID_PARAMETER;

    OrchestrationBinarySerializer serializer;

    const Variant result = serializer.serialize(script->get_orchestration(), p_path, p_flags);
    if (result.get_type() != Variant::PACKED_BYTE_ARRAY)
        return ERR_FILE_CANT_WRITE;

    const Ref<FileAccess> file = FileAccess::open_compressed(p_path, FileAccess::WRITE);
    ERR_FAIL_COND_V_MSG(file.is_null(), FileAccess::get_open_error(), "Cannot write file '" + p_path + "'");

    file->store_buffer(result);
    ERR_FAIL_COND_V_MSG(file->get_error() != OK, file->get_error(), "Cannot write file '" + p_path + "'");

    file->flush();
    file->close();

    return OK;
}

void OScriptBinaryResourceSaver::_bind_methods()
{
}

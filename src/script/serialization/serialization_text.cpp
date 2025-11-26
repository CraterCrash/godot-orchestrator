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
#include "script/serialization/serialization_text.h"

#include "orchestration/io/orchestration_parser_text.h"
#include "orchestration/io/orchestration_serializer_text.h"
#include "script/script.h"
#include "script/serialization/format_defs.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_uid.hpp>

PackedStringArray OScriptTextResourceLoader::_get_recognized_extensions() const
{
    return Array::make(ORCHESTRATOR_SCRIPT_TEXT_EXTENSION);
}

bool OScriptTextResourceLoader::_recognize_path(const String& p_path, const StringName& type) const
{
    return p_path.ends_with(ORCHESTRATOR_SCRIPT_QUALIFY_EXTENSION(ORCHESTRATOR_SCRIPT_TEXT_EXTENSION));
}

bool OScriptTextResourceLoader::_handles_type(const StringName& p_type) const
{
    return p_type.match(OScript::get_class_static());
}

String OScriptTextResourceLoader::_get_resource_type(const String& p_path) const
{
    if (p_path.ends_with(ORCHESTRATOR_SCRIPT_QUALIFY_EXTENSION(ORCHESTRATOR_SCRIPT_TEXT_EXTENSION)))
        return OScript::get_class_static();

    return "";
}

String OScriptTextResourceLoader::_get_resource_script_class(const String& p_path) const
{
    const String ext = p_path.get_extension().to_lower();
    if (ext != ORCHESTRATOR_SCRIPT_QUALIFY_EXTENSION(ORCHESTRATOR_SCRIPT_TEXT_EXTENSION))
        return {};

    OrchestrationTextParser parser;
    return parser.get_script_class(p_path);
}

int64_t OScriptTextResourceLoader::_get_resource_uid(const String& p_path) const
{
    if (!_get_recognized_extensions().has(p_path.get_extension().to_lower()))
        return ResourceUID::INVALID_ID;

    OrchestrationTextParser parser;
    return parser.get_uid(p_path);
}

PackedStringArray OScriptTextResourceLoader::_get_dependencies(const String& p_path, bool p_add_types) const
{
    OrchestrationTextParser parser;
    return parser.get_dependencies(p_path, p_add_types);
}

Error OScriptTextResourceLoader::_rename_dependencies(const String& p_path, const Dictionary& p_renames) const
{
    OrchestrationTextParser parser;

    Error result = parser.rename_dependencies(p_path, p_renames);
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

bool OScriptTextResourceLoader::_exists(const String& p_path) const
{
    return FileAccess::file_exists(p_path);
}

PackedStringArray OScriptTextResourceLoader::_get_classes_used(const String& p_path) const
{
    OrchestrationTextParser parser;
    return parser.get_classes_used(p_path);
}

Variant OScriptTextResourceLoader::_load(const String& p_path, const String& p_original_path, bool p_use_threads, int32_t p_cache_mode) const
{
    const Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
    ERR_FAIL_COND_V_MSG(!file.is_valid(), nullptr, "Cannot open file '" + p_path + "'");

    const String path = !p_original_path.is_empty() ? p_original_path : p_path;
    const String local_path = ProjectSettings::get_singleton()->localize_path(p_path);
    const String source_code = file->get_as_text();

    OrchestrationTextParser parser;
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
        script->set_path(local_path);
        script->set_orchestration(orchestration);
    }
    return script;
}

void OScriptTextResourceLoader::_bind_methods()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

String OScriptTextResourceSaver::_get_local_path(const String& p_path) const
{
    return ProjectSettings::get_singleton()->localize_path(p_path);
}

PackedStringArray OScriptTextResourceSaver::_get_recognized_extensions(const Ref<Resource>& p_resource) const
{
    if (p_resource->get_name().ends_with(ORCHESTRATOR_SCRIPT_QUALIFY_EXTENSION(ORCHESTRATOR_SCRIPT_TEXT_EXTENSION)))
        return Array::make(ORCHESTRATOR_SCRIPT_TEXT_EXTENSION);

    return {};
}

bool OScriptTextResourceSaver::_recognize(const Ref<Resource>& p_resource) const
{
    // Currently allow saving any resource object as OScript format
    // todo: should we restrict this?
    return true;
}

Error OScriptTextResourceSaver::_set_uid(const String& p_path, int64_t p_uid)
{
    // todo: needs implementation
    return OK;
}

bool OScriptTextResourceSaver::_recognize_path(const Ref<Resource>& p_resource, const String& p_path) const
{
    return p_path.ends_with(ORCHESTRATOR_SCRIPT_QUALIFY_EXTENSION(ORCHESTRATOR_SCRIPT_TEXT_EXTENSION));
}

Error OScriptTextResourceSaver::_save(const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags)
{
    const Ref<OScript> script = p_resource;
    if (!script.is_valid())
        return ERR_INVALID_PARAMETER;

    OrchestrationTextSerializer serializer;

    const Variant result = serializer.serialize(script->get_orchestration(), p_path, p_flags);
    if (result.get_type() != Variant::STRING)
        return ERR_FILE_CANT_WRITE;

    const String source = result;
    if (source.is_empty())
        return ERR_FILE_CANT_WRITE;

    const Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE);
    ERR_FAIL_COND_V_MSG(file.is_null(), FileAccess::get_open_error(), "Cannot write file '" + p_path + "'");

    file->store_string(source);
    ERR_FAIL_COND_V_MSG(file->get_error() != OK, file->get_error(), "Cannot write file '" + p_path + "'");

    file->flush();
    file->close();

    return OK;
}

void OScriptTextResourceSaver::_bind_methods()
{
}
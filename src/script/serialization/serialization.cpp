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
#include "script/serialization/serialization.h"

#include "script/script.h"
#include "script/serialization/binary_loader_instance.h"
#include "script/serialization/binary_saver_instance.h"
#include "script/serialization/format_defs.h"
#include "script/serialization/text_loader_instance.h"
#include "script/serialization/text_saver_instance.h"

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
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
    else
        return "";
}

String OScriptBinaryResourceLoader::_get_resource_script_class(const String& p_path) const
{
    return "";
}

int64_t OScriptBinaryResourceLoader::_get_resource_uid(const String& p_path) const
{
    if (!_get_recognized_extensions().has(p_path.get_extension()))
        return ResourceUID::INVALID_ID;

    Ref<FileAccess> file = FileAccess::open_compressed(p_path, FileAccess::READ);
    if (!file.is_valid())
        return ResourceUID::INVALID_ID;

    OScriptBinaryResourceLoaderInstance loader;
    loader._local_path = ProjectSettings::get_singleton()->localize_path(p_path);
    loader._resource_path = loader._local_path;
    loader.open(file, true);

    if (loader._error != OK)
        return ResourceUID::INVALID_ID;

    return loader._uid;
}

PackedStringArray OScriptBinaryResourceLoader::_get_dependencies(const String& p_path, bool p_add_types) const
{
    return {}; // no dependencies yet
}

Error OScriptBinaryResourceLoader::_rename_dependencies(const String& p_path, const Dictionary& p_renames) const
{
    return OK; // no dependencies yet
}

bool OScriptBinaryResourceLoader::_exists(const String& p_path) const
{
    return FileAccess::file_exists(p_path);
}

PackedStringArray OScriptBinaryResourceLoader::_get_classes_used(const String& p_path) const
{
    Ref<FileAccess> file = FileAccess::open_compressed(p_path, FileAccess::READ);
    if (!file.is_valid())
        return {};

    OScriptBinaryResourceLoaderInstance loader;
    loader._local_path = ProjectSettings::get_singleton()->localize_path(p_path);
    loader._resource_path = loader._local_path;

    return loader.get_classes_used(file);
}

Variant OScriptBinaryResourceLoader::_load(const String& p_path, const String& p_original_path, bool p_use_threads, int32_t p_cache_mode) const
{
    Ref<FileAccess> file = FileAccess::open_compressed(p_path, FileAccess::READ);
    if (!file.is_valid())
        file = FileAccess::open(p_path, FileAccess::READ);
    ERR_FAIL_COND_V_MSG(!file.is_valid(), nullptr, "Cannot open file '" + p_path + "'");

    const String path = !p_original_path.is_empty() ? p_original_path : p_path;
    const String local_path = ProjectSettings::get_singleton()->localize_path(p_path);

    OScriptBinaryResourceLoaderInstance loader;
    loader._cache_mode = static_cast<CacheMode>(p_cache_mode);
    loader._local_path = local_path;
    loader._resource_path = loader._local_path;
    loader.open(file);

    const Error result = loader.load();
    if (result != OK)
        return Ref<Resource>();

    Ref<OScript> script = loader._resource;
    if (script.is_valid())
    {
        script->set_path(local_path);
        script->_version = loader._version;

        // Sanity check, used to be in OrchestratorScriptView, but belongs here instead
        if (script->get_orchestration()->get_type() == OT_Script && !script->has_graph("EventGraph"))
        {
            WARN_PRINT("Legacy orchestration '" + script->get_path() + "' loaded, creating event graph...");
            script->create_graph("EventGraph", OScriptGraph::GraphFlags::GF_EVENT);
        }

        script->post_initialize();
    }

    return loader._resource;
}

void OScriptBinaryResourceLoader::_bind_methods()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
    else
        return "";
}

String OScriptTextResourceLoader::_get_resource_script_class(const String& p_path) const
{
    return "";
}

int64_t OScriptTextResourceLoader::_get_resource_uid(const String& p_path) const
{
    if (!_get_recognized_extensions().has(p_path.get_extension().to_lower()))
        return ResourceUID::INVALID_ID;

    Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
    if (file.is_null())
        return ResourceUID::INVALID_ID;

    OScriptTextResourceLoaderInstance loader;
    loader._local_path = ProjectSettings::get_singleton()->localize_path(p_path);
    loader._res_path = loader._local_path;

    return loader.get_uid(file);
}

PackedStringArray OScriptTextResourceLoader::_get_dependencies(const String& p_path, bool p_add_types) const
{
    return {}; // no dependencies yet
}

Error OScriptTextResourceLoader::_rename_dependencies(const String& p_path, const Dictionary& p_renames) const
{
    return OK; // no dependencies yet
}

bool OScriptTextResourceLoader::_exists(const String& p_path) const
{
    return FileAccess::file_exists(p_path);
}

PackedStringArray OScriptTextResourceLoader::_get_classes_used(const String& p_path) const
{
    Ref<FileAccess> file = FileAccess::open_compressed(p_path, FileAccess::READ);
    if (!file.is_valid())
        return {};

    OScriptTextResourceLoaderInstance loader;
    loader._local_path = ProjectSettings::get_singleton()->localize_path(p_path);
    loader._res_path = loader._local_path;

    return loader.get_classes_used(file);
}

Variant OScriptTextResourceLoader::_load(const String& p_path, const String& p_original_path, bool p_use_threads, int32_t p_cache_mode) const
{
    const Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
    ERR_FAIL_COND_V_MSG(!file.is_valid(), nullptr, "Cannot open file '" + p_path + "'");

    const String path = !p_original_path.is_empty() ? p_original_path : p_path;
    const String local_path = ProjectSettings::get_singleton()->localize_path(p_path);

    OScriptTextResourceLoaderInstance loader;
    loader._cache_mode = static_cast<CacheMode>(p_cache_mode);
    loader._local_path = local_path;
    loader._res_path = loader._local_path;
    loader.open(file);

    const Error result = loader.load();
    if (result != OK)
        return Ref<Resource>();

    Ref<OScript> script = loader._resource;
    if (script.is_valid())
    {
        script->set_path(local_path);
        script->_version = loader._version;

        // Sanity check, used to be in OrchestratorScriptView, but belongs here instead
        if (script->get_orchestration()->get_type() == OT_Script && !script->has_graph("EventGraph"))
        {
            WARN_PRINT("Legacy orchestration '" + script->get_path() + "' loaded, creating event graph...");
            script->create_graph("EventGraph", OScriptGraph::GraphFlags::GF_EVENT);
        }

        script->post_initialize();
    }

    return loader._resource;
}

void OScriptTextResourceLoader::_bind_methods()
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
    OScriptBinaryResourceSaverInstance saver;
    return saver.set_uid(_get_local_path(p_path), p_uid);
}

bool OScriptBinaryResourceSaver::_recognize_path(const Ref<Resource>& p_resource, const String& p_path) const
{
    return p_path.ends_with(ORCHESTRATOR_SCRIPT_QUALIFY_EXTENSION(ORCHESTRATOR_SCRIPT_EXTENSION));
}

Error OScriptBinaryResourceSaver::_save(const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags)
{
    OScriptBinaryResourceSaverInstance saver;
    return saver.save(_get_local_path(p_path), p_resource, p_flags);
}

void OScriptBinaryResourceSaver::_bind_methods()
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
    OScriptTextResourceSaverInstance saver;
    return saver.set_uid(_get_local_path(p_path), p_uid);
}

bool OScriptTextResourceSaver::_recognize_path(const Ref<Resource>& p_resource, const String& p_path) const
{
    return p_path.ends_with(ORCHESTRATOR_SCRIPT_QUALIFY_EXTENSION(ORCHESTRATOR_SCRIPT_TEXT_EXTENSION));
}

Error OScriptTextResourceSaver::_save(const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags)
{
    OScriptTextResourceSaverInstance saver;
    return saver.save(_get_local_path(p_path), p_resource, p_flags);
}

void OScriptTextResourceSaver::_bind_methods()
{
}
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
#include "orchestration/io/orchestration_parser.h"

#include "editor/plugins/orchestrator_editor_plugin.h"

#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_uid.hpp>

bool OrchestrationParser::_is_cached(const String& p_path) const
{
    return ResourceLoader::get_singleton()->has_cached(p_path);
}

Ref<Resource> OrchestrationParser::_get_cached_resource(const String& p_path)
{
    #if GODOT_VERSION >= 0x040400
    return ResourceLoader::get_singleton()->get_cached_ref(p_path);
    #else
    return nullptr;
    #endif
}

void OrchestrationParser::_set_resource_edited(const Ref<Resource>& p_resource, bool p_edited)
{
    #ifdef TOOLS_ENABLED
    // todo: advocate for merging the upstream pull request for this
    #if GODOT_VERSION >= 0x040600
    p_resource->set_edited(p_edited);
    #endif
    #endif
}

bool OrchestrationParser::_is_creating_missing_resources_if_class_unavailable_enabled() // NOLINT
{
    // EditorNode sets this to true, existence of our plugin should be sufficient?
    // todo: not exposed on ResourceLoader
    return OrchestratorPlugin::get_singleton() != nullptr;
}

bool OrchestrationParser::_is_parse_error(const String& p_reason) // NOLINT
{
    return _error == ERR_PARSE_ERROR && p_reason == _error_text;
}

Error OrchestrationParser::_set_error(const String& p_reason)
{
    return _set_error(ERR_PARSE_ERROR, p_reason);
}

Error OrchestrationParser::_set_error(Error p_error, const String& p_reason)
{
    _error = p_error;
    _error_text = p_reason;

    return _error;
}

int64_t OrchestrationParser::_get_resource_id_for_path(const String& p_path, bool p_generate)
{
    const int64_t fallback = ResourceLoader::get_singleton()->get_resource_uid(p_path);
    if (fallback != ResourceUID::INVALID_ID)
        return fallback;

    if (p_generate)
        return ResourceUID::get_singleton()->create_id();

    return ResourceUID::INVALID_ID;
}

void OrchestrationParser::_warn_invalid_external_resource_uid(uint32_t p_index, const String& p_path, uint64_t p_uid)
{
    const String message = vformat(
        "%s: In editor resource %d, invalid UID: %d - using text path instead: %s",
        _local_path, p_index, p_uid, p_path);

    #ifdef TOOLS_ENABLED
    if (ResourceLoader::get_singleton()->get_resource_uid(_local_path) != static_cast<int64_t>(p_uid))
        WARN_PRINT(message);
    #else
    WARN_PRINT(message);
    #endif
}
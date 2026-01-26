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
#include "script/serialization/binary/resource_loader_binary.h"

#include "common/error_list.h"
#include "orchestration/serialization/binary/binary_parser.h"
#include "script/script.h"
#include "script/script_cache.h"
#include "script/serialization/format_defs.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_uid.hpp>

bool OScriptBinaryResourceFormatLoader::_is_binary_script(const String& p_path) {
    return p_path.get_extension().naturalnocasecmp_to(ORCHESTRATOR_SCRIPT_EXTENSION) == 0;
}

PackedStringArray OScriptBinaryResourceFormatLoader::_get_recognized_extensions() const {
    return Array::make(ORCHESTRATOR_SCRIPT_EXTENSION);
}

bool OScriptBinaryResourceFormatLoader::_recognize_path(const String& p_path, const StringName& type) const {
    return _is_binary_script(p_path);
}

bool OScriptBinaryResourceFormatLoader::_handles_type(const StringName& p_type) const {
    return p_type == OScript::get_class_static();
}

String OScriptBinaryResourceFormatLoader::_get_resource_type(const String& p_path) const {
    return _is_binary_script(p_path) ? OScript::get_class_static() : "";
}

String OScriptBinaryResourceFormatLoader::_get_resource_script_class(const String& p_path) const {
    if (!_is_binary_script(p_path)) {
        return {};
    }

    OrchestrationBinaryParser parser;
    return parser.get_resource_script_class(ProjectSettings::get_singleton()->localize_path(p_path));
}

int64_t OScriptBinaryResourceFormatLoader::_get_resource_uid(const String& p_path) const {
    if (!_is_binary_script(p_path)) {
        return ResourceUID::INVALID_ID;
    }

    OrchestrationBinaryParser parser;
    return parser.get_resource_uid(ProjectSettings::get_singleton()->localize_path(p_path));
}

PackedStringArray OScriptBinaryResourceFormatLoader::_get_dependencies(const String& p_path, bool p_add_types) const {
    if (!_is_binary_script(p_path)) {
        return {};
    }

    OrchestrationBinaryParser parser;
    return parser.get_dependencies(ProjectSettings::get_singleton()->localize_path(p_path), p_add_types);
}

Error OScriptBinaryResourceFormatLoader::_rename_dependencies(const String& p_path, const Dictionary& p_renames) const {
    if (!_is_binary_script(p_path)) {
        return OK;
    }

    OrchestrationBinaryParser parser;
    Error error = parser.rename_dependencies(p_path, p_renames); // Intentionally doesn't use local_path here

    const String rename_file = vformat("%s.depren", p_path);
    const Ref<DirAccess> dir = DirAccess::open("res://");
    if (error == OK && dir->file_exists(rename_file)) {
        dir->remove(p_path);
        dir->rename(rename_file, p_path);
    }

    return error;
}

bool OScriptBinaryResourceFormatLoader::_exists(const String& p_path) const {
    return FileAccess::file_exists(ProjectSettings::get_singleton()->localize_path(p_path));
}

PackedStringArray OScriptBinaryResourceFormatLoader::_get_classes_used(const String& p_path) const {
    if (!_is_binary_script(p_path)) {
        return {};
    }

    OrchestrationBinaryParser parser;
    return parser.get_classes_used(ProjectSettings::get_singleton()->localize_path(p_path));
}

Variant OScriptBinaryResourceFormatLoader::_load(const String& p_path, const String& p_original_path, bool p_use_sub_threads, int32_t p_cache_mode) const {
    if (!_is_binary_script(p_path)) {
        return {};
    }

    Error error;
    const bool ignoring = p_cache_mode == CACHE_MODE_IGNORE || p_cache_mode == CACHE_MODE_IGNORE_DEEP;

    Ref<OScript> script = OScriptCache::get_full_script(p_original_path, error, "", ignoring);
    if (error && script.is_valid()) {
        ERR_PRINT_ED(vformat(R"(Failed to load script "%s" with error "%s".)", p_original_path, error_names[error]));
    }

    return script;
}

void OScriptBinaryResourceFormatLoader::_bind_methods() {
}
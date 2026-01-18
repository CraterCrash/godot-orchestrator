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
#include "core/godot/io/resource_uid.h"

#include "common/version.h"

#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_uid.hpp>

namespace GDE {
    String ResourceUID::ensure_path(const String& p_path) {
        #if GODOT_VERSION >= 0x040500
        return godot::ResourceUID::ensure_path(p_path);
        #else
        if (p_path.begins_with("uid://")) {
            return uid_to_path(p_path);
        }
        return p_path;
        #endif
    }

    String ResourceUID::uid_to_path(const String& p_uid) {
        #if GODOT_VERSION >= 0x040500
        return godot::ResourceUID::uid_to_path(p_uid);
        #else
        const int64_t id = godot::ResourceUID::get_singleton()->text_to_id(p_uid);
        return godot::ResourceUID::get_singleton()->get_id_path(id);
        #endif
    }

    String ResourceUID::path_to_uid(const String& p_path) {
        #if GODOT_VERSION >= 0x040500
        return godot::ResourceUID::path_to_uid(p_path);
        #else
        const int64_t id = ResourceLoader::get_singleton()->get_resource_uid(p_path);
        if (id == godot::ResourceUID::INVALID_ID) {
            return p_path;
        }
        return godot::ResourceUID::get_singleton()->id_to_text(id);
        #endif
    }
}
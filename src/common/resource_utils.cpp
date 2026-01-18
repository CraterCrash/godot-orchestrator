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
#include "common/resource_utils.h"

#include "common/version.h"
#include "editor/plugins/orchestrator_editor_plugin.h"
#include "script/serialization/resource_cache.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/missing_resource.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_uid.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace ResourceUtils {

    bool is_creating_missing_resources_if_class_unavailable_enabled() {
        // EditorNode sets this to true in its constructor.
        // Since OrchestratorPlugin should only be loaded in the editor, this should be equivalent
        return OrchestratorPlugin::get_singleton() != nullptr;
    }

    void set_edited(const Ref<Resource>& p_resource, bool p_edited) {
        #ifdef TOOLS_ENABLED
        #if GODOT_VERSION >= 0x040700
        p_resource->set_edited(p_edited);
        #endif
        #endif
    }

    String generate_scene_unique_id() {
        #if GODOT_VERSION >= 0x040300
        return Resource::generate_scene_unique_id();
        #else
        const Dictionary dt = Time::get_singleton()->get_datetime_dict_from_system();

        uint32_t hash = hash_murmur3_one_32(Time::get_singleton()->get_ticks_usec());
        hash = hash_murmur3_one_32(dt["year"], hash);
        hash = hash_murmur3_one_32(dt["month"], hash);
        hash = hash_murmur3_one_32(dt["day"], hash);
        hash = hash_murmur3_one_32(dt["hour"], hash);
        hash = hash_murmur3_one_32(dt["minute"], hash);
        hash = hash_murmur3_one_32(dt["second"], hash);
        hash = hash_murmur3_one_32(UtilityFunctions::randi(), hash);

        static constexpr uint32_t characters = 5;
        static constexpr uint32_t char_count = ('z' - 'a');
        static constexpr uint32_t base = char_count + ('9' - '0');

        String id;
        for (uint32_t i = 0; i < characters; i++) {
            const uint32_t c = hash % base;
            if (c < char_count) {
                id += String::chr('a' + c);
            } else {
                id += String::chr('0' + (c - char_count));
            }
            hash /= base;
        }

        return id;
        #endif
    }

    String get_scene_unique_id(const Ref<Resource>& p_resource, const String& p_path) {
        ERR_FAIL_COND_V_MSG(!p_resource.is_valid(), "", "Cannot get scene unique id on an invalid resource");

        #if GODOT_VERSION >= 0x040300
        return p_resource->get_scene_unique_id();
        #else
        return ResourceCache::get_singleton()->get_scene_unique_id(p_path, p_resource);
        #endif
    }

    void set_scene_unique_id(const Ref<Resource>& p_resource, const String& p_path, const String& p_id) {
        ERR_FAIL_COND_MSG(!p_resource.is_valid(), "Cannot set id on an invalid resource");

        #if GODOT_VERSION >= 0x040300
        p_resource->set_scene_unique_id(p_id);
        #else
        ResourceCache::get_singleton()->set_scene_unique_id(p_path, p_resource, p_id);
        #endif
    }

    void set_id_for_path(const Ref<Resource>& p_resource, const String& p_path, const String& p_id) {
        #if GODOT_VERSION >= 0x040400
        p_resource->set_id_for_path(p_path, p_id);
        #else
        ResourceCache::get_singleton()->set_id_for_path(p_path, p_resource->get_path(), p_id);
        #endif
    }

    int64_t get_resource_id_for_path(const String& p_path, bool p_generate) {
        #if GODOT_VERSION >= 0x040300
        const int64_t fallback = ResourceLoader::get_singleton()->get_resource_uid(p_path);
        if (fallback != ResourceUID::INVALID_ID) {
            return fallback;
        }
        if (p_generate) {
            return ResourceUID::get_singleton()->create_id();
        }
        return ResourceUID::INVALID_ID;
        #else
        // In orchestrations, we did not serialize the UID in this context
        return ResourceUID::INVALID_ID;
        #endif
    }

    bool is_builtin(const Ref<Resource>& p_resource) {
        String path_cache = p_resource->get_path();
        return path_cache.is_empty() || path_cache.contains("::") || path_cache.begins_with("local://");
    }

    bool is_file(const String& p_path) {
        return p_path.begins_with("res://") && p_path.find("::") == -1;
    }

    String get_class(const Ref<Resource>& p_resource) {
        const Ref<MissingResource> missing = p_resource;
        if (missing.is_valid()) {
            return missing->get_original_class();
        }
        return p_resource->get_class();
    }

}
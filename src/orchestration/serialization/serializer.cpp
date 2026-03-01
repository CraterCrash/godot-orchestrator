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
#include "orchestration/serialization/serializer.h"

#include "common/dictionary_utils.h"
#include "common/version.h"
#include "orchestration/serialization/text/text_format.h"

#include <godot_cpp/classes/missing_resource.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>

void OrchestrationSerializer::_decode_and_set_flags(const String& p_path, uint32_t p_flags) {
    _path = ProjectSettings::get_singleton()->localize_path(p_path);

    _relative_paths = p_flags & ResourceSaver::FLAG_RELATIVE_PATHS;
    _skip_editor = p_flags & ResourceSaver::FLAG_OMIT_EDITOR_PROPERTIES;
    _bundle_resources = p_flags & ResourceSaver::FLAG_BUNDLE_RESOURCES;

    _take_over_paths = p_flags & ResourceSaver::FLAG_REPLACE_SUBRESOURCE_PATHS;
    if (!p_path.begins_with("res://")) {
        _take_over_paths = false;
    }
}

bool OrchestrationSerializer::_is_resource_built_in(const Ref<Resource>& p_resource) {
    if (p_resource.is_valid()) {
        #if GODOT_VERSION >= 0x040500
        return p_resource->is_built_in();
        #else
        String path_cache = p_resource->get_path();
        return path_cache.is_empty() || path_cache.contains("::") || path_cache.begins_with("local://");
        #endif
    }
    return false;
}

int64_t OrchestrationSerializer::_get_resource_id_for_path(const String& p_path, bool p_generate) {
    return OrchestrationTextFormat::get_resource_id_for_path(p_path, p_generate);
}

void OrchestrationSerializer::_set_resource_edited(const Ref<Resource>& p_resource, bool p_edited) {
    #ifdef TOOLS_ENABLED
    #if GODOT_VERSION >= 0x041500
    // todo: advocate for this to be merged
    p_resource->set_edited(p_edited);
    #endif
    #endif
}

String OrchestrationSerializer::_resource_get_class(const Ref<Resource>& p_resource) {
    const Ref<MissingResource> missing_resource = p_resource;
    if (missing_resource.is_valid()) {
        return missing_resource->get_original_class();
    }
    return p_resource->get_class();
}

String OrchestrationSerializer::_generate_scene_unique_id() {
    return Resource::generate_scene_unique_id();
}

String OrchestrationSerializer::_create_resource_uid(const Ref<Resource>& p_resource, const HashSet<String>& p_used_ids, bool& r_generated) {
    String uid = p_resource->get_scene_unique_id();

    if (!uid.is_empty()) {
        r_generated = false;
        return uid;
    }

    while (true) {
        uid = _resource_get_class(p_resource) + "_" + _generate_scene_unique_id();
        if (!p_used_ids.has(uid)) {
            break;
        }
    }

    r_generated = true;
    return uid;
}


void OrchestrationSerializer::_find_resources(const Variant& p_variant, bool p_main) { // NOLINT
    switch (p_variant.get_type()) {
        case Variant::OBJECT: {
            _find_resources_object(p_variant, p_main);
            break;
        }
        case Variant::ARRAY: {
            _find_resources_array(p_variant, p_main);
            break;
        }
        case Variant::DICTIONARY: {
            _find_resources_dictionary(p_variant, p_main);
            break;
        }
        case Variant::NODE_PATH: {
            _find_resources_node_path(p_variant, p_main);
            break;
        }
        default: {
            // no-op
            break;
        }
    }
}

void OrchestrationSerializer::_find_resources_array(const Array& p_array, bool p_main) { // NOLINT
    const int64_t size = p_array.size();
    for (int64_t i = 0; i < size; i++) {
        _find_resources(p_array[i], false);
    }
}

void OrchestrationSerializer::_find_resources_dictionary(const Dictionary& p_dictionary, bool p_main) { // NOLINT
    const Array keys = p_dictionary.keys();
    for (uint32_t i = 0; i < keys.size(); i++) {
        const Variant& key = keys[i];
        _find_resources(key, false);
        _find_resources(p_dictionary[key], false);
    }
}

void OrchestrationSerializer::_find_resources_resource_properties(const Ref<Resource>& p_resource, bool p_main) {
    _resource_set.insert(p_resource);

    const List<PropertyInfo> properties = DictionaryUtils::to_properties(p_resource->get_property_list(), true);
    for (const PropertyInfo& info : properties) {
        if (info.usage & PROPERTY_USAGE_STORAGE) {
            const Variant value = p_resource->get(info.name);

            if (info.usage & PROPERTY_USAGE_RESOURCE_NOT_PERSISTENT) {
                NonPersistentKey key;
                key.base = p_resource;
                key.property = info.name;
                _non_persistent_map[key] = value;

                Ref<Resource> r = value;
                if (r.is_valid()) {
                    _resource_set.insert(r);
                    _saved_resources.push_back(r);
                } else {
                    _find_resources(value, false);
                }
            } else {
                _find_resources(value, false);
            }
        }
    }

    _saved_resources.push_back(p_resource);
}
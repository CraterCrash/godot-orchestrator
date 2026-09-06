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

#include "common/dictionary_utils.h"

#include <godot_cpp/classes/file_access.hpp>

namespace ResourceUtils {

    bool is_file(const String& p_path) {
        return p_path.begins_with("res://") && p_path.find("::") == -1;
    }

    Dictionary get_storage_properties(const Ref<Resource>& p_resource) {
        Dictionary properties;
        ERR_FAIL_COND_V(p_resource.is_null(), properties);

        const TypedArray<Dictionary> property_list = p_resource->get_property_list();
        for (int i = 0; i < property_list.size(); i++) {
            const PropertyInfo property = DictionaryUtils::to_property(property_list[i]);
            if (!(property.usage & PROPERTY_USAGE_STORAGE)) {
                continue;
            }

            if (property.name.match("script") || property.name.begins_with("resource_") || property.name.begins_with("metadata/")) {
                continue;
            }

            properties[property.name] = p_resource->get(property.name);
        }

        return properties;
    }

}
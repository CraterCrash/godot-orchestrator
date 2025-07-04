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
#include "common/classdb_utils.h"

#include "common/dictionary_utils.h"
#include "common/version.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace ClassDBUtils
{
    Variant class_get_property_default_value(const StringName& p_class_name, const StringName& p_property_name)
    {
        #if GODOT_VERSION >= 0x040300
        // See https://github.com/godotengine/godot/pull/90916
        return ClassDB::class_get_property_default_value(p_class_name, p_property_name);
        #else
        if (!internal::default_value_cache.has(p_class_name))
        {
            if (ClassDB::can_instantiate(p_class_name))
            {
                Variant instance = ClassDB::instantiate(p_class_name);

                const Ref<Resource> resource = instance;
                if (resource.is_valid())
                {
                    const List<PropertyInfo> properties = DictionaryUtils::to_properties(resource->get_property_list());
                    for (const PropertyInfo& property : properties)
                    {
                        if (property.usage & (PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_EDITOR))
                            internal::default_value_cache[p_class_name][property.name] = resource->get(property.name);
                    }
                }
                else
                {
                    // An object
                    memdelete(Object::cast_to<Object>(instance));
                }

            }
        }

        if (internal::default_value_cache.has(p_class_name))
            return internal::default_value_cache[p_class_name][p_property_name];

        return {};
        #endif
    }

}
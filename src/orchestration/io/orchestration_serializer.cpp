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
#include "orchestration/io/orchestration_serializer.h"

#include "common/dictionary_utils.h"
#include "editor/plugins/orchestrator_editor_plugin.h"

#include <godot_cpp/classes/missing_resource.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_uid.hpp>

bool OrchestrationSerializer::_is_cached(const String& p_path) const
{
    return ResourceLoader::get_singleton()->has_cached(p_path);
}

Ref<Resource> OrchestrationSerializer::_get_cached_resource(const String& p_path)
{
    #if GODOT_VERSION >= 0x040400
    return ResourceLoader::get_singleton()->get_cached_ref(p_path);
    #else
    return nullptr;
    #endif
}

String OrchestrationSerializer::_get_resource_class(const Ref<Resource>& p_resource) const
{
    Ref<MissingResource> missing = p_resource;
    if (missing.is_valid())
        return missing->get_original_class();

    return p_resource->get_class();
}

void OrchestrationSerializer::_set_resource_edited(const Ref<Resource>& p_resource, bool p_edited)
{
    #ifdef TOOLS_ENABLED
    // todo: advocate for merging the upstream pull request for this
    #if GODOT_VERSION >= 0x040600
    p_resource->set_edited(p_edited);
    #endif
    #endif
}

bool OrchestrationSerializer::_is_creating_missing_resources_if_class_unavailable_enabled() // NOLINT
{
    // EditorNode sets this to true, existence of our plugin should be sufficient?
    // todo: not exposed on ResourceLoader
    return OrchestratorPlugin::get_singleton() != nullptr;
}

bool OrchestrationSerializer::_is_parse_error(const String& p_reason) // NOLINT
{
    return _error == ERR_PARSE_ERROR && p_reason == _error_text;
}

Error OrchestrationSerializer::_set_error(const String& p_reason)
{
    return _set_error(ERR_PARSE_ERROR, p_reason);
}

Error OrchestrationSerializer::_set_error(Error p_error, const String& p_reason)
{
    _error = p_error;
    _error_text = p_reason;

    return _error;
}

bool OrchestrationSerializer::_is_built_in_resource(const Ref<Resource>& p_resource)
{
    // Taken from resource.h
    String path_cache = p_resource->get_path();
    return path_cache.is_empty() || path_cache.contains("::") || path_cache.begins_with("local://");
}

int64_t OrchestrationSerializer::_get_resource_id_for_path(const String& p_path, bool p_generate)
{
    int64_t fallback = ResourceLoader::get_singleton()->get_resource_uid(p_path);
    if (fallback != ResourceUID::INVALID_ID)
        return fallback;

    if (p_generate)
        return ResourceUID::get_singleton()->create_id();

    return ResourceUID::INVALID_ID;
}

void OrchestrationSerializer::_warn_invalid_external_resource_uid(uint32_t p_index, const String& p_path, uint64_t p_uid)
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

Variant OrchestrationSerializer::_get_class_property_default(const StringName& p_class, const StringName& p_property)
{
    // See https://github.com/godotengine/godot/pull/90916
    #if GODOT_VERSION >= 0x040300
    return ClassDB::class_get_property_default_value(p_class, p_property);
    #else
    if (!_default_value_cache.has(p_class))
    {
        if (ClassDB::can_instantiate(p_class))
        {
            Variant instance = ClassDB::instantiate(p_class);

            Ref<Resource> resource = instance;
            if (resource.is_valid())
            {
                List<PropertyInfo> properties = DictionaryUtils::to_properties(resource->get_property_list());
                for (const PropertyInfo& pi : properties)
                {
                    if (pi.usage & (PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_EDITOR))
                        _default_value_cache[p_class][pi.name] = resource->get(pi.name);
                }
            }
            else
            {
                // An object
                memdelete(Object::cast_to<Object>(instance));
            }
        }
    }

    if (_default_value_cache.has(p_class))
        return _default_value_cache[p_class][p_property];

    return {};
    #endif
}

void OrchestrationSerializer::_gather_resources(const Variant& p_value, bool p_main)
{
    switch (p_value.get_type())
    {
        case Variant::OBJECT:
            _gather_object_resources(p_value, p_main);
            break;
        case Variant::ARRAY:
            _gather_array_resources(p_value, p_main);
            break;
        case Variant::DICTIONARY:
            _gather_dictionary_resources(p_value, p_main);
            break;
        case Variant::NODE_PATH:
            _gather_node_path(p_value, p_main);
            break;
        default:
            break;
    }
}

void OrchestrationSerializer::_gather_object_resources(const Ref<Resource>& p_value, bool p_main)
{
    if (!_is_resource_gatherable(p_value, p_main))
        return;

    _resource_set.insert(p_value);

    List<PropertyInfo> properties = DictionaryUtils::to_properties(p_value->get_property_list(), true);
    List<PropertyInfo>::Element* E = properties.front();
    while (E)
    {
        const PropertyInfo& property = E->get();
        if (property.usage & PROPERTY_USAGE_STORAGE)
        {
            const Variant value = p_value->get(property.name);
            if (property.usage & PROPERTY_USAGE_RESOURCE_NOT_PERSISTENT)
            {
                NonPersistentKey key;
                key.base = p_value;
                key.property = property.name;
                _non_persistent_map[key] = value;

                const Ref<Resource> resource = value;
                if (resource.is_valid())
                {
                    _resource_set.insert(resource);
                    _saved_resources.push_back(resource);
                }
                else
                    _gather_resources(value, false);
            }
            else
                _gather_resources(value, false);
        }

        E = E->next();
    }

    _saved_resources.push_back(p_value);
}

void OrchestrationSerializer::_gather_array_resources(const Array& p_value, bool p_main)
{
    const uint64_t size = p_value.size();
    for (uint64_t i = 0; i < size; i++)
    {
        const Variant& value = p_value[i];
        _gather_resources(value, false);
    }
}

void OrchestrationSerializer::_gather_dictionary_resources(const Dictionary& p_value, bool p_main)
{
    const Array keys = p_value.keys();

    const int64_t size = keys.size();
    for (int64_t i = 0; i < size; i++)
    {
        // Of course keys should also be cached.
        // See ResourceFormatSaverBinaryInstance::_find_resources (when p_value is DICTIONARY)
        const Variant& key = keys[i];

        _gather_resources(keys[i], false);
        _gather_resources(p_value[key], false);
    }
}

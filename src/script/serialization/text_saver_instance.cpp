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
#include "script/serialization/text_saver_instance.h"

#include "common/dictionary_utils.h"
#include "common/string_utils.h"
#include "script/serialization/resource_cache.h"
#include "script/serialization/variant_parser.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/missing_resource.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/resource_uid.hpp>
#include <godot_cpp/classes/scene_state.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

String OScriptTextResourceSaverInstance::_write_resources(void* p_userdata, const Ref<Resource>& p_resource)
{
    OScriptTextResourceSaverInstance* saver = static_cast<OScriptTextResourceSaverInstance*>(p_userdata);
    return saver->_write_resource(p_resource);
}

String OScriptTextResourceSaverInstance::_write_resource(const Ref<Resource>& p_resource)
{
    if (p_resource->get_meta("_skip_save_", false))
        return "null";

    if (_external_resources.has(p_resource))
        return vformat("ExternalResource(\"%s\")", _external_resources[p_resource]);

    if (_internal_resources.has(p_resource))
        return vformat("SubResource(\"%s\")", _internal_resources[p_resource]);

    if (!_is_resource_built_in(p_resource))
    {
        if (p_resource->get_path() == _local_path) // Circular reference
            return "null";

        // External resource
        String path = _relative_paths ? StringUtils::path_to_file(_local_path, p_resource->get_path()) : p_resource->get_path();
        return vformat("Resource(\"%s\")", path);
    }

    // Internal resource
    ERR_FAIL_V_MSG("null", "Resource was not pre-cached for the resource section, bug?");
}

void OScriptTextResourceSaverInstance::_find_resources_object(const Variant& p_variant, bool p_main)
{
    const Ref<Resource> res = p_variant;
    if (res.is_null() || res->get_meta("_skip_save_", false) || _external_resources.has(res))
        return;

    if (!p_main && (!_bundle_resources) && !_is_resource_built_in(res))
    {
        if (res->get_path() == _local_path)
        {
            ERR_PRINT("Circular reference to reosurce being saved found: " + _local_path + " will be null next time its loaded.");
            return;
        }

        // Use a numeric ID as a base, beacuse they are sorted in natural order before saving.
        // This increases the chances of thread loading to fetch them first.
        #if GODOT_VERSION >= 0x040300
        String id = itos(_external_resources.size() + 1) + "-" + Resource::generate_scene_unique_id();
        #else
        String id = itos(_external_resources.size() + 1) + "_" + _generate_scene_unique_id();
        #endif
        _external_resources[res] = id;
        return;
    }

    if (_resource_set.has(res))
        return;

    _resource_set.insert(res);

    List<PropertyInfo> properties = DictionaryUtils::to_properties(res->get_property_list(), true);
    List<PropertyInfo>::Element* E = properties.front();
    while (E)
    {
        const PropertyInfo& pi = E->get();
        if (pi.usage & PROPERTY_USAGE_STORAGE)
        {
            Variant value = res->get(pi.name);
            if (pi.usage & PROPERTY_USAGE_RESOURCE_NOT_PERSISTENT)
            {
                NonPersistentKey key;
                key.base = res;
                key.property = pi.name;
                _non_persistent_map[key] = value;

                Ref<Resource> sres = value;
                if (sres.is_valid())
                {
                    _resource_set.insert(sres);
                    _saved_resources.push_back(sres);
                }
                else
                    _find_resources(value);
            }
            else
                _find_resources(value);
        }

        E = E->next();
    }
    _saved_resources.push_back(res);
}

void OScriptTextResourceSaverInstance::_find_resources_array(const Variant& p_variant, bool p_main)
{
    Array array = p_variant;
    int size = array.size();
    for (int i = 0; i < size; i++)
    {
        const Variant& value = array[i];
        _find_resources(value);
    }
}

void OScriptTextResourceSaverInstance::_find_resources_dictionary(const Variant& p_variant, bool p_main)
{
    const Dictionary dict = p_variant;
    const Array keys = dict.keys();
    for (int i = 0; i < keys.size(); i++)
    {
        // Of course keys should also be cached.
        // See ResourceFormatSaverBinaryInstance::_find_resources (when p_variant is of type Variant::DICTIONARY)
        const Variant& key = keys[i];
        _find_resources(key);

        const Variant& value = dict[key];
        _find_resources(value);
    }
}

bool OScriptTextResourceSaverInstance::_is_resource_built_in(const Ref<Resource>& p_resource) const
{
    // Taken from resource.h
    String path_cache = p_resource->get_path();
    return path_cache.is_empty() || path_cache.contains("::") || path_cache.begins_with("local://");
}

String OScriptTextResourceSaverInstance::_resource_get_class(const Ref<Resource>& p_resource) const
{
    Ref<MissingResource> missing_resource = p_resource;
    if (missing_resource.is_valid())
        return missing_resource->get_original_class();
    return p_resource->get_class();
}

#if GODOT_VERSION < 0x040300
String OScriptTextResourceSaverInstance::_generate_scene_unique_id()
{
    // Generate a unique enough hash, but still user-readable.
    // If it's not unique it does not matter because the saver will try again.
    Dictionary dt = Time::get_singleton()->get_datetime_dict_from_system();
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
    for (uint32_t i = 0; i < characters; i++)
    {
        uint32_t c = hash % base;
        if (c < char_count)
            id += String::chr('a' + c);
        else
            id += String::chr('0' + (c - char_count));

        hash /= base;
    }

    return id;
}
#endif

int64_t OScriptTextResourceSaverInstance::_get_resource_id_for_path(const String& p_path, bool p_generate)
{
    int64_t fallback = ResourceLoader::get_singleton()->get_resource_uid(p_path);
    if (fallback != ResourceUID::INVALID_ID)
        return fallback;

    if (p_generate)
        return ResourceUID::get_singleton()->create_id();

    return ResourceUID::INVALID_ID;
}

Variant OScriptTextResourceSaverInstance::_class_get_property_default_value(const StringName& p_class_name,
                                                                         const StringName& p_property)
{
    // See https://github.com/godotengine/godot/pull/90916
    #if GODOT_VERSION >= 0x040300
    return ClassDB::class_get_property_default_value(p_class_name, p_property);
    #else
    if (!_default_value_cache.has(p_class_name))
    {
        if (ClassDB::can_instantiate(p_class_name))
        {
            Variant instance = ClassDB::instantiate(p_class_name);

            Ref<Resource> resource = instance;
            if (resource.is_valid())
            {
                List<PropertyInfo> properties = DictionaryUtils::to_properties(resource->get_property_list());
                for (const PropertyInfo& pi : properties)
                {
                    if (pi.usage & (PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_EDITOR))
                        _default_value_cache[p_class_name][pi.name] = resource->get(pi.name);
                }
            }
            else
            {
                // An object
                memdelete(Object::cast_to<Object>(instance));
            }
        }
    }

    if (_default_value_cache.has(p_class_name))
        return _default_value_cache[p_class_name][p_property];

    return {};
    #endif
}

void OScriptTextResourceSaverInstance::_find_resources(const Variant& p_variant, bool p_main)
{
    switch (p_variant.get_type())
    {
        case Variant::OBJECT:
            _find_resources_object(p_variant, p_main);
            break;
        case Variant::ARRAY:
            _find_resources_array(p_variant, p_main);
            break;
        case Variant::DICTIONARY:
            _find_resources_dictionary(p_variant, p_main);
            break;
        default:
            // no-op
            break;
    }
}

Error OScriptTextResourceSaverInstance::save(const String& p_path, const Ref<Resource>& p_resource, uint32_t p_flags)
{
    Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE);
    ERR_FAIL_COND_V_MSG(!file.is_valid(), ERR_CANT_OPEN, "Cannot save file '" + p_path + "'.");
    ERR_FAIL_COND_V_MSG(!file->is_open(), ERR_CANT_OPEN, "Cannot open file '" + p_path + "' for saving.");

    _local_path = ProjectSettings::get_singleton()->localize_path(p_path);
    _relative_paths = p_flags & ResourceSaver::FLAG_RELATIVE_PATHS;
    _skip_editor = p_flags & ResourceSaver::FLAG_OMIT_EDITOR_PROPERTIES;
    _bundle_resources = p_flags & ResourceSaver::FLAG_BUNDLE_RESOURCES;

    _take_over_paths = p_flags & ResourceSaver::FLAG_REPLACE_SUBRESOURCE_PATHS;
    if (!p_path.begins_with("res://"))
        _take_over_paths = false;

    // Save resources
    _find_resources(p_resource, true);

    {
        String title = "[orchestration type=\"" + _resource_get_class(p_resource) + "\" ";
        #if GODOT_VERSION >= 0x040300
        Ref<Script> script = p_resource->get_script();
        if (script.is_valid() && !script->get_global_name().is_empty())
            title += "script_class=\"" + String(script->get_global_name()) + "\" ";
        #endif

        const uint32_t load_steps = _saved_resources.size() + _external_resources.size();
        if (load_steps > 1)
            title += "load_steps=" + itos(load_steps) + " ";

        title += "format=" + itos(FORMAT_VERSION) + "";

        int64_t uid = _get_resource_id_for_path(_local_path, true);
        if (uid != ResourceUID::INVALID_ID)
            title += " uid=\"" + ResourceUID::get_singleton()->id_to_text(uid) + "\"";

        file->store_string(title);
        file->store_line("]\n"); // One empty line.
    }

    #ifdef TOOLS_ENABLED
    // Keep order from cached ids.
    HashSet<String> cached_ids_found;
    for (KeyValue<Ref<Resource>, String>& E : _external_resources)
    {
        // Taken from resource.cpp
        // String cached_id = E.key->get_id_for_path(_local_path);
        String cached_id = ResourceCache::get_singleton()->get_id_for_path(_local_path, E.key->get_path());
        if (cached_id.is_empty() || cached_ids_found.has(cached_id))
        {
            int sep_pos = E.value.find("_");
            if (sep_pos != -1)
                E.value = E.value.substr(0, sep_pos + 1);
            else
                E.value = "";
        }
        else
        {
            E.value = cached_id;
            cached_ids_found.insert(cached_id);
        }
    }
    // Create IDs for non-cached resources.
    for (KeyValue<Ref<Resource>, String>& E :_external_resources)
    {
        if (cached_ids_found.has(E.value)) // Already cached, go on.
            continue;

        String attempt;
        while (true)
        {
            #if GODOT_VERSION >= 0x040300
            attempt = E.value + Resource::generate_scene_unique_id();
            #else
            attempt = E.value + _generate_scene_unique_id();
            #endif
            if (!cached_ids_found.has(attempt))
                break;
        }

        cached_ids_found.insert(attempt);
        E.value = attempt;

        // Update also in resource
        Ref<Resource> res = E.key;

        // Taken from resource_.cpp
        // res->set_id_for_path(_local_path, attempt);
        ResourceCache::get_singleton()->set_id_for_path(_local_path, res->get_path(), attempt);
    }
    #else
        int counter = 1;
        for (KeyValue<Ref<Resource>, String>& E : _external_resources)
            E.value = itos(counter++);
    #endif

    Vector<ResourceSort> sorted_external_resources;
    for (const KeyValue<Ref<Resource>, String>& E : _external_resources)
    {
        ResourceSort rs;
        rs.resource = E.key;
        rs.id = E.value;
        sorted_external_resources.push_back(rs);
    }

    sorted_external_resources.sort();

    for (int i = 0; i < sorted_external_resources.size(); i++)
    {
        String p = sorted_external_resources[i].resource->get_path();
        String s = "[ext_resource type=\"" + sorted_external_resources[i].resource->get_class() + "\"";

        #if GODOT_VERSION >= 0x040300
        int uid = _get_resource_id_for_path(p, false);
        #else
        int uid = ResourceUID::INVALID_ID;
        #endif
        if (uid != ResourceUID::INVALID_ID)
            s += " uid=\"" + ResourceUID::get_singleton()->id_to_text(uid) + "\"";

        s += " path=\"" + p + "\" id=\"" + sorted_external_resources[i].id + "\"]\n";
        file->store_string(s); // Bundled
    }

    if (_external_resources.size())
        file->store_line(String()); // Separate.

    HashSet<String> used_unique_ids;
    for (List<Ref<Resource>>::Element* E = _saved_resources.front(); E; E = E->next())
    {
        Ref<Resource> res = E->get();
        if (E->next() && _is_resource_built_in(res))
        {
            #if GODOT_VERSION >= 0x040300
            if (!res->get_scene_unique_id().is_empty())
            {
                if (used_unique_ids.has(res->get_scene_unique_id()))
                    res->set_scene_unique_id(""); // Repeated
                else
                    used_unique_ids.insert(res->get_scene_unique_id());
            }
            #else
            String res_scene_uid = ResourceCache::get_singleton()->get_scene_unique_id(_local_path, res);
            if (!res_scene_uid.is_empty())
            {
                if (used_unique_ids.has(res_scene_uid))
                    ResourceCache::get_singleton()->set_scene_unique_id(_local_path, res, "");
                else
                    used_unique_ids.insert(res_scene_uid);
            }
            #endif
        }
    }

    for (List<Ref<Resource>>::Element* E = _saved_resources.front(); E; E = E->next())
    {
        Ref<Resource> res = E->get();
        ERR_CONTINUE(!_resource_set.has(res));

        bool main = (E->next() == nullptr);

        if (main)
        {
            file->store_line("[resource]");
        }
        else
        {
            String line = "[obj ";
            #if GODOT_VERSION >= 0x040300
            if (res->get_scene_unique_id().is_empty())
            {
                String new_id;
                while(true)
                {
                    #if GODOT_VERSION >= 0x040300
                    new_id = _resource_get_class(res) + "_" + Resource::generate_scene_unique_id();
                    #else
                    new_id = _resource_get_class(res) + "_" + _generate_scene_unique_id();
                    #endif
                    if (!used_unique_ids.has(new_id))
                        break;
                }
                res->set_scene_unique_id(new_id);
                used_unique_ids.insert(new_id);
            }
            String id = res->get_scene_unique_id();
            #else
            String res_scene_uid = ResourceCache::get_singleton()->get_scene_unique_id(_local_path, res);
            if (res_scene_uid.is_empty())
            {
                String new_id;
                while (true)
                {
                    new_id = _resource_get_class(res) + "_" + _generate_scene_unique_id();
                    if (!used_unique_ids.has(new_id))
                        break;
                }
                ResourceCache::get_singleton()->set_scene_unique_id(_local_path, res, new_id);
                used_unique_ids.insert(new_id);
            }
            String id = ResourceCache::get_singleton()->get_scene_unique_id(_local_path, res);
            #endif

            line += "type=\"" + _resource_get_class(res) + "\" id=\"" + id;
            file->store_line(line + "\"]");
            if (_take_over_paths)
                res->set_path(vformat("%s::%s", p_path, id));

            _internal_resources[res] = id;
            #if (TOOLS_ENABLED && GODOT_VERSION >= 0x040400)
            res->set_edited(false);
            #endif
        }

        Dictionary missing_resource_properties = p_resource->get_meta("_missing_resources_", Dictionary());

        TypedArray<Dictionary> property_list = res->get_property_list();
        for (int i = 0; i < property_list.size(); i++)
        {
            const PropertyInfo pi = DictionaryUtils::to_property(property_list[i]);
            if (_skip_editor && pi.name.begins_with("__editor"))
                continue;
            if (pi.name.match("metadata/_missing_resources"))
                continue;

            if (pi.usage & PROPERTY_USAGE_STORAGE)
            {
                Variant value;
                String name = pi.name;

                if (pi.usage & PROPERTY_USAGE_RESOURCE_NOT_PERSISTENT)
                {
                    NonPersistentKey key;
                    key.base = res;
                    key.property = name;
                    if (_non_persistent_map.has(key))
                        value = _non_persistent_map[key];
                }
                else
                    value = res->get(name);

                if (pi.type == Variant::OBJECT && missing_resource_properties.has(name))
                {
                    // Was this missing resource overidden?
                    Ref<Resource> ures = value;
                    if (ures.is_null())
                        value = missing_resource_properties[name];
                }

                Variant default_value = _class_get_property_default_value(res->get_class(), pi.name);
                if (default_value.get_type() != Variant::NIL)
                {
                    Variant r_ret = false;
                    bool r_valid = true;
                    Variant::evaluate(Variant::OP_EQUAL, value, default_value, r_ret, r_valid);
                    if (r_valid && r_ret)
                        continue;
                }

                if (pi.type == Variant::OBJECT)
                {
                    Object* obj = Object::cast_to<Object>(value);
                    if (!obj && !(pi.usage & PROPERTY_USAGE_STORE_IF_NULL))
                        continue;
                }

                String vars;
                OScriptVariantWriter::write_to_string(value, vars, _write_resources, this);
                file->store_string(StringUtils::property_name_encode(name) + " = " + vars + "\n");
            }
        }

        if (E->next())
            file->store_line(String());
    }

    if (file->get_error() != OK && file->get_error() != ERR_FILE_EOF)
        return ERR_CANT_CREATE;

    return OK;
}

Error OScriptTextResourceSaverInstance::set_uid(const String& p_path, uint64_t p_uid)
{
    // todo: need to be completed
    return OK;
}
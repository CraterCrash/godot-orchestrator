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
#include "orchestration/io/orchestration_serializer_text.h"

#include "common/dictionary_utils.h"
#include "common/resource_utils.h"
#include "common/string_utils.h"
#include "common/version.h"
#include "editor/plugins/orchestrator_editor_plugin.h"
#include "orchestration/io/orchestration_format.h"
#include "script/script_server.h"
#include "script/serialization/resource_cache.h"

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/resource_uid.hpp>
#include <godot_cpp/classes/time.hpp>

#define READING_SIGN 0
#define READING_INT 1
#define READING_DEC 2
#define READING_EXP 3
#define READING_DONE 4

#define MAX_RECURSION 100

static String rtos_fix(double p_value)
{
    if (p_value == 0.0)
        return "0"; // Avoid negative (-0) written
    else if (std::isnan(p_value))
        return "nan";
    else if (std::isinf(p_value))
    {
        if (p_value > 0)
            return "inf";
        else
            return "inf_neg";
    }

    return rtoss(p_value);
}

#define RTOS(x) rtos_fix(x)

String OrchestrationTextSerializer::_generate_scene_unique_id()
{
    #if GODOT_VERSION >= 0x040300
    return Resource::generate_scene_unique_id();
    #else
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
    #endif
}



Error OrchestrationTextSerializer::_write_orchestration_tag(const Ref<Resource>& p_resource, String& r_value)
{
    String script_class;
    #if GODOT_VERSION >= 0x040300
    Ref<Script> script = p_resource->get_script();
    if (script.is_valid())
        script_class = ScriptServer::get_global_name(script);
    #endif

    const String type = _get_resource_class(p_resource);
    const uint32_t load_steps = _saved_resources.size() + _external_resources.size();
    const int64_t uid = _get_resource_id_for_path(_local_path, true);

    r_value += get_start_tag(type, script_class, load_steps, OrchestrationFormat::FORMAT_VERSION, uid) + "\n";
    return OK;
}

Error OrchestrationTextSerializer::_write_external_resource_tags(String& r_value)
{
    #ifdef TOOLS_ENABLED
    // Keep order from cached ids.
    HashSet<String> cached_ids_found;
    for (KeyValue<Ref<Resource>, String>& E : _external_resource_ids)
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
    for (KeyValue<Ref<Resource>, String>& E :_external_resource_ids)
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
    for (KeyValue<Ref<Resource>, String>& E : _external_resource_ids)
        E.value = itos(counter++);
    #endif

    Vector<ResourceSort> sorted_external_resources;
    for (const KeyValue<Ref<Resource>, String>& E : _external_resource_ids)
    {
        ResourceSort rs;
        rs.resource = E.key;
        rs.id = E.value;
        sorted_external_resources.push_back(rs);
    }

    sorted_external_resources.sort();

    for (int i = 0; i < sorted_external_resources.size(); i++)
    {
        String res_class = sorted_external_resources[i].resource->get_class();
        String res_path = sorted_external_resources[i].resource->get_path();
        String res_id = sorted_external_resources[i].id;

        r_value += get_ext_resource_tag(res_class, res_path, res_id, true); // Bundled
    }

    if (_external_resource_ids.size())
        r_value += "\n";

    return OK;
}

Error OrchestrationTextSerializer::_write_objects(const Ref<Resource>& p_orchestration, const String& p_path, String& r_value)
{
    HashSet<String> used_unique_ids;
    for (List<Ref<Resource>>::Element* E = _saved_resources.front(); E; E = E->next())
    {
        Ref<Resource> res = E->get();
        if (E->next() && _is_built_in_resource(res))
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

    // Iterates all but the last resource, which is the main orchestration
    for (List<Ref<Resource>>::Element* E = _saved_resources.front(); E && E->next() != nullptr; E = E->next())
    {
        const Ref<Resource> res = E->get();
        ERR_CONTINUE(!_resource_set.has(res));

        const String resource_class = _get_resource_class(res);

        String tag = "[obj ";

        #if GODOT_VERSION >= 0x040300
        if (res->get_scene_unique_id().is_empty())
        {
            String new_id;
            while(true)
            {
                new_id = vformat("%s_%s", resource_class, _generate_scene_unique_id());
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
                new_id = vformat("%s_%s", resource_class, _generate_scene_unique_id());
                if (!used_unique_ids.has(new_id))
                    break;
            }
            ResourceCache::get_singleton()->set_scene_unique_id(_local_path, res, new_id);
            used_unique_ids.insert(new_id);
        }
        String id = ResourceCache::get_singleton()->get_scene_unique_id(_local_path, res);
        #endif

        tag += "type=\"" + resource_class + "\" id=\"" + id;
        r_value += tag + "\"]\n";

        // todo: this really shouldn't be here as this is just a "serializer"
        if (_take_over_paths)
            res->set_path(vformat("%s::%s", p_path, id));

        _internal_resource_ids[res] = id;

        // todo: this really shouldn't be here as this is just a "serializer"
        _set_resource_edited(res, false);

        if (const Error err = _write_properties(p_orchestration, res, r_value))
            return err;

        r_value += "\n";
    }

    return OK;
}

Error OrchestrationTextSerializer::_write_resource(const Ref<Resource>& p_orchestration, String& r_value)
{
    if (_saved_resources.is_empty())
        return _set_error(ERR_FILE_CANT_WRITE, "Failed to find orchestration resource");

    if (!_resource_set.has(p_orchestration))
        return _set_error(ERR_FILE_CANT_WRITE, "Failed to write resource tag");

    r_value += "[resource]\n";

    return _write_properties(p_orchestration, p_orchestration, r_value);
}

Error OrchestrationTextSerializer::_write_properties(const Ref<Resource>& p_orchestration, const Ref<Resource>& p_resource, String& r_value)
{
    Dictionary missing_resource_properties = p_orchestration->get_meta("_missing_resources_", Dictionary());

    TypedArray<Dictionary> property_list = p_resource->get_property_list();
    for (int i = 0; i < property_list.size(); i++)
    {
        const PropertyInfo property = DictionaryUtils::to_property(property_list[i]);

        if (_skip_editor && property.name.begins_with("__editor"))
            continue;

        if (property.name.match("metadata/_missing_resources"))
            continue;

        if (property.usage & PROPERTY_USAGE_STORAGE)
        {
            Variant value;
            String name = property.name;

            if (property.usage & PROPERTY_USAGE_RESOURCE_NOT_PERSISTENT)
            {
                NonPersistentKey key;
                key.base = p_resource;
                key.property = name;
                if (_non_persistent_map.has(key))
                    value = _non_persistent_map[key];
            }
            else
                value = p_resource->get(name);

            if (property.type == Variant::OBJECT && missing_resource_properties.has(name))
            {
                // Was this missing resource overidden?
                Ref<Resource> ures = value;
                if (ures.is_null())
                    value = missing_resource_properties[name];
            }

            Variant default_value = _get_class_property_default(p_resource->get_class(), property.name);
            if (default_value.get_type() != Variant::NIL)
            {
                Variant r_ret = false;
                bool r_valid = true;
                Variant::evaluate(Variant::OP_EQUAL, value, default_value, r_ret, r_valid);
                if (r_valid && r_ret)
                    continue;
            }

            if (property.type == Variant::OBJECT)
            {
                Object* obj = Object::cast_to<Object>(value);
                if (!obj && !(property.usage & PROPERTY_USAGE_STORE_IF_NULL))
                    continue;
            }

            String result_value;
            if (const Error err = _write_property(value, result_value))
                return err;

            r_value += StringUtils::property_name_encode(name) + " = " + result_value + "\n";
        }
    }

    return OK;
}

Error OrchestrationTextSerializer::_write_property(const Variant& p_value, String& r_value, int p_recursion_count)
{
    switch (p_value.get_type())
    {
        case Variant::NIL:
        {
            r_value += "null";
            break;
        }
        case Variant::BOOL:
        {
            r_value += p_value.operator bool() ? "true" : "false";
            break;
        }
        case Variant::INT:
        {
            r_value += itos(p_value.operator int64_t());
            break;
        }
        case Variant::FLOAT:
        {
            String str = RTOS(p_value.operator double());
            if (str != "inf" && str != "inf_neg" && str != "nan" && !str.contains(".") && !str.contains("e"))
                str += ".0";

            r_value += str;
            break;
        }
        case Variant::STRING:
        {
            r_value += "\"" + StringUtils::c_escape_multiline(p_value) + "\"";
            break;
        }
        case Variant::VECTOR2:
        {
            const Vector2 v = p_value;
            r_value += vformat("Vector2(%s, %s)", RTOS(v.x), RTOS(v.y));
            break;
        }
            case Variant::VECTOR2I:
        {
            const Vector2i v = p_value;
            r_value += vformat("Vector2i(%s, %s)", itos(v.x), itos(v.y));
            break;
        }
        case Variant::RECT2:
        {
            const Rect2 r = p_value;
            r_value += vformat("Rect2(%s, %s, %s, %s)", RTOS(r.position.x), RTOS(r.position.y), RTOS(r.size.x), RTOS(r.size.y));
            break;
        }
        case Variant::RECT2I:
        {
            const Rect2i r = p_value;
            r_value += vformat("Rect2i(%s, %s, %s, %s)", itos(r.position.x), itos(r.position.y), itos(r.size.x), itos(r.size.y));
            break;
        }
        case Variant::VECTOR3:
        {
            const Vector3 v = p_value;
            r_value += vformat("Vector3(%s, %s, %s)", RTOS(v.x), RTOS(v.y), RTOS(v.z));
            break;
        }
        case Variant::VECTOR3I:
        {
            const Vector3i v = p_value;
            r_value += vformat("Vector3i(%s, %s, %s)", itos(v.x), itos(v.y), itos(v.z));
            break;
        }
        case Variant::VECTOR4:
        {
            const Vector4 v = p_value;
            r_value += vformat("Vector4(%s, %s, %s, %s)", RTOS(v.x), RTOS(v.y), RTOS(v.z), RTOS(v.w));
            break;
        }
        case Variant::VECTOR4I:
        {
            const Vector4i v = p_value;
            r_value += vformat("Vector4i(%s, %s, %s, %s)", itos(v.x), itos(v.y), itos(v.z), itos(v.w));
            break;
        }
        case Variant::PLANE:
        {
            const Plane p = p_value;
            r_value += vformat("Plane(%s, %s, %s %s)", RTOS(p.normal.x), RTOS(p.normal.y), RTOS(p.normal.z), RTOS(p.d));
            break;
        }
        case Variant::AABB:
        {
            const AABB aabb = p_value;
            r_value += vformat("AABB(%s, %s, %s, %s, %s, %s)",
                RTOS(aabb.position.x), RTOS(aabb.position.y), RTOS(aabb.position.z),
                RTOS(aabb.size.x), RTOS(aabb.size.y), RTOS(aabb.size.z));
            break;
        }
        case Variant::QUATERNION:
        {
            const Quaternion q = p_value;
            r_value += vformat("Quaternion(%s, %s, %s, %s)", RTOS(q.x), RTOS(q.y), RTOS(q.z), RTOS(q.w));
            break;
        }
        case Variant::TRANSFORM2D:
        {
            r_value += "Transform2D(";
            const Transform2D t = p_value;
            for (int i = 0; i < 3; i++)
            {
                for (int j = 0; j < 2; j++)
                {
                    if (i != 0 || j != 0)
                        r_value += ", ";

                    r_value += RTOS(t.columns[i][j]);
                }
            }
            r_value += ")";
            break;
        }
        case Variant::BASIS:
        {
            r_value += "Basis(";
            const Basis b = p_value;
            for (int i = 0; i < 3; i++)
            {
                for (int j = 0; j < 3; j++)
                {
                    if (i != 0 || j != 0)
                        r_value += ", ";

                    r_value += RTOS(b.rows[i][j]);
                }
            }
            r_value += ")";
            break;
        }
        case Variant::TRANSFORM3D:
        {
            r_value += "Transform3D(";
            const Transform3D t = p_value;
            const Basis& m3 = t.basis;
            for (int i = 0; i < 3; i++)
            {
                for (int j = 0; j < 3; j++)
                {
                    if (i != 0 || j != 0)
                        r_value += ", ";

                    r_value += RTOS(m3.rows[i][j]);
                }
            }
            r_value += RTOS(t.origin.x) + ", " + RTOS(t.origin.y) + ", " + RTOS(t.origin.z) + ")";
            break;
        }
        case Variant::PROJECTION:
        {
            r_value += "Projection(";
            const Projection p = p_value;
            for (int i = 0; i < 4; i++)
            {
                for (int j = 0; j < 4; j++)
                {
                    if (i != 0 || j != 0)
                        r_value += ", ";

                    r_value += RTOS(p.columns[i][j]);
                }
            }
            r_value += ")";
            break;
        }
        case Variant::COLOR:
        {
            const Color c = p_value;
            r_value += vformat("Color(%s, %s, %s, %s)", RTOS(c.r), RTOS(c.g), RTOS(c.b), RTOS(c.a));
            break;
        }
        case Variant::STRING_NAME:
        {
            const String str = p_value;
            r_value += vformat("&\"%s\"", str.c_escape());
            break;
        }
        case Variant::NODE_PATH:
        {
            const String str = p_value;
            r_value += vformat("NodePath(\"%s\")", str.c_escape());
            break;
        }
        case Variant::RID:
        {
            // RIDs are not stored
            r_value += "RID()";
            break;
        }
        case Variant::SIGNAL:
        {
            // Signals are not stored
            r_value += "Signal()";
            break;
        }
        case Variant::CALLABLE:
        {
            // Callables are not stored
            r_value += "Callable()";
            break;
        }
        case Variant::OBJECT:
        {
            if (unlikely(p_recursion_count > MAX_RECURSION))
            {
                ERR_PRINT("Max recursion reached");
                r_value += "null";
                return OK;
            }
            p_recursion_count++;

            Object* obj = p_value;
            if (!obj)
            {
                r_value += "null";
                break; // don't save it
            }

            Ref<Resource> res = p_value;
            if (res.is_valid())
            {
                String res_text = _write_encoded_resource(res);
                if (res_text.is_empty() && ResourceUtils::is_file(res->get_path()))
                {
                    // External Resource
                    String path = res->get_path();
                    res_text = "Resource(\"" + path + "\")";
                }

                // Could come up with some sort of text
                if (!res_text.is_empty())
                {
                    r_value += res_text;
                    break;
                }
            }

            // Generic Object
            r_value += "Object(" + obj->get_class() + ",";

            bool first{ true };
            List<PropertyInfo> properties = DictionaryUtils::to_properties(obj->get_property_list());
            for (const PropertyInfo& property : properties)
            {
                if (property.usage & PROPERTY_USAGE_STORAGE || property.usage & PROPERTY_USAGE_SCRIPT_VARIABLE)
                {
                    if (first)
                        first = false;
                    else
                        r_value += ",";

                    r_value += "\"" + property.name + "\":";

                    _write_property(obj->get(property.name), r_value, p_recursion_count);
                }
            }
            r_value += ")\n";
            break;
        }
        case Variant::DICTIONARY:
        {
            Dictionary dict = p_value;
            if (unlikely(p_recursion_count > MAX_RECURSION))
            {
                ERR_PRINT("Max recursion reached");
                r_value += "{}";
            }
            else
            {
                p_recursion_count++;

                Array keys = dict.keys();
                if (keys.is_empty())
                {
                    r_value += "{}";
                    break;
                }

                int64_t size = keys.size();
                r_value += "{\n";

                for (int64_t i = 0; i < size; i++)
                {
                    const Variant& key = keys[i];
                    _write_property(key, r_value, p_recursion_count);

                    r_value += ": ";

                    _write_property(dict[key], r_value, p_recursion_count);

                    r_value += ((i + 1) < size) ? ",\n" : "\n";
                }
                r_value += "}";
            }
            break;
        }
        case Variant::ARRAY:
        {
            Array array = p_value;
            if (array.get_typed_builtin() != Variant::NIL)
            {
                r_value += "Array[";

                Variant::Type builtin_type = (Variant::Type) array.get_typed_builtin();
                StringName class_name = array.get_typed_class_name();
                Ref<Script> script = array.get_typed_script();
                if (script.is_valid())
                {
                    String res_text = _write_encoded_resource(script);
                    if (res_text.is_empty() && ResourceUtils::is_file(script->get_path()))
                        res_text = "Resource(\"" + script->get_path() + "\")";

                    if (res_text.is_empty())
                    {
                        ERR_PRINT("Failed to encode a path to a custom script for an array type.");
                        r_value += class_name;
                    }
                    else
                        r_value += res_text;
                }
                else if (class_name != StringName())
                    r_value += class_name;
                else
                    r_value += Variant::get_type_name(builtin_type);

                r_value += "](";
            }

            if (unlikely(p_recursion_count > MAX_RECURSION))
            {
                ERR_PRINT("Max recursion reached");
                r_value += "[]";
            }
            else
            {
                p_recursion_count++;

                r_value += "[";
                int64_t size = array.size();
                for (int64_t i = 0; i < size; i++)
                {
                    if (i > 0)
                        r_value += ", ";

                    _write_property(array[i], r_value, p_recursion_count);
                }
                r_value += "]";
            }

            if (array.get_typed_builtin() != Variant::NIL)
                r_value += ")";

            break;
        }
        case Variant::PACKED_BYTE_ARRAY:
        {
            r_value += "PackedByteArray(";
            const PackedByteArray data = p_value;
            int64_t size = data.size();
            for (int64_t i = 0; i < size; i++)
            {
                if (i > 0)
                    r_value += ", ";

                r_value += itos(data[i]);
            }
            r_value += ")";
            break;
        }
        case Variant::PACKED_INT32_ARRAY:
        {
            r_value += "PackedInt32Array(";
            const PackedInt32Array data = p_value;
            int64_t size = data.size();
            for (int64_t i = 0; i < size; i++)
            {
                if (i > 0)
                    r_value += ", ";

                r_value += itos(data[i]);
            }
            r_value += ")";
            break;
        }
        case Variant::PACKED_INT64_ARRAY:
        {
            r_value += "PackedInt64Array(";
            const PackedInt64Array data = p_value;
            int64_t size = data.size();
            for (int64_t i = 0; i < size; i++)
            {
                if (i > 0)
                    r_value += ", ";

                r_value += itos(data[i]);
            }
            r_value += ")";
            break;
        }
        case Variant::PACKED_FLOAT32_ARRAY:
        {
            r_value += "PackedFloat32Array(";
            const PackedFloat32Array data = p_value;
            int64_t size = data.size();
            for (int64_t i = 0; i < size; i++)
            {
                if (i > 0)
                    r_value += ", ";

                r_value += RTOS(data[i]);
            }
            r_value += ")";
            break;
        }
        case Variant::PACKED_FLOAT64_ARRAY:
        {
            r_value += "PackedFloat64Array(";
            const PackedFloat64Array data = p_value;
            int64_t size = data.size();
            for (int64_t i = 0; i < size; i++)
            {
                if (i > 0)
                    r_value += ", ";

                r_value += RTOS(data[i]);
            }
            r_value += ")";
            break;
        }
        case Variant::PACKED_STRING_ARRAY:
        {
            r_value += "PackedStringArray(";
            const PackedStringArray data = p_value;
            int64_t size = data.size();
            for (int64_t i = 0; i < size; i++)
            {
                if (i > 0)
                    r_value += ", ";

                r_value += vformat("\"%s\"", data[i].c_escape());
            }
            r_value += ")";
            break;
        }
        case Variant::PACKED_VECTOR2_ARRAY:
        {
            r_value += "PackedVector2Array(";
            const PackedVector2Array data = p_value;
            int64_t size = data.size();
            for (int64_t i = 0; i < size; i++)
            {
                if (i > 0)
                    r_value += ", ";

                r_value += vformat("%s, %s", RTOS(data[i].x), RTOS(data[i].y));
            }
            r_value += ")";
            break;
        }
        case Variant::PACKED_VECTOR3_ARRAY:
        {
            r_value += "PackedVector3Array(";
            const PackedVector3Array data = p_value;
            int64_t size = data.size();
            for (int64_t i = 0; i < size; i++)
            {
                if (i > 0)
                    r_value += ", ";

                r_value += vformat("%s, %s, %s", RTOS(data[i].x), RTOS(data[i].y), RTOS(data[i].z));
            }
            r_value += ")";
            break;
        }
        case Variant::PACKED_COLOR_ARRAY:
        {
            r_value += "PackedColorArray(";
            const PackedColorArray data = p_value;
            int64_t size = data.size();
            for (int64_t i = 0; i < size; i++)
            {
                if (i > 0)
                    r_value += ", ";

                r_value += vformat("%s, %s, %s, %s", RTOS(data[i].r), RTOS(data[i].g), RTOS(data[i].b), RTOS(data[i].a));
            }
            r_value += ")";
            break;
        }
        case Variant::PACKED_VECTOR4_ARRAY:
        {
            r_value += "PackedVector4Array(";
            const PackedVector4Array data = p_value;
            int64_t size = data.size();
            for (int64_t i = 0; i < size; i++)
            {
                if (i > 0)
                    r_value += ", ";

                r_value += vformat("%s, %s, %s, %s", RTOS(data[i].x), RTOS(data[i].y), RTOS(data[i].z), RTOS(data[i].w));
            }
            r_value += ")";
            break;
        }
        default:
        {
            ERR_PRINT("Unknown variant type");
            return ERR_BUG;
        }
    }

    return OK;
}

String OrchestrationTextSerializer::_write_encoded_resource(const Ref<Resource>& p_resource)
{
    if (p_resource->get_meta("_skip_save_", false))
        return "null";

    if (_external_resource_ids.has(p_resource))
        return vformat("ExtResource(\"%s\")", _external_resource_ids[p_resource]);

    if (_internal_resource_ids.has(p_resource))
        return vformat("SubResource(\"%s\")", _internal_resource_ids[p_resource]);

    if (!_is_built_in_resource(p_resource))
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

bool OrchestrationTextSerializer::_is_resource_gatherable(const Ref<Resource>& p_resource, bool p_main)
{
    if (p_resource.is_null() || p_resource->get_meta("_skip_save_", false) || _external_resource_ids.has(p_resource))
        return false;

    if (!p_main && !_bundle_resources && !_is_built_in_resource(p_resource))
    {
        if (p_resource->get_path() == _local_path)
        {
            ERR_PRINT(vformat(
                "Circular reference to resource begin saved found: %s will be null next time it's loaded.",
                _local_path));
            return false;
        }

        // Use a numeric ID as a base, because they are sorted in natural order before saving.
        // This increases the chance of thread loading to fetch them first.
        const String id = itos(_external_resources.size() + 1) + "_" + _generate_scene_unique_id();
        _external_resource_ids[p_resource] = id;
        return false;
    }

    return !_resource_set.has(p_resource);
}

Variant OrchestrationTextSerializer::serialize(const Ref<Orchestration>& p_orchestration, const String& p_path, uint32_t p_flags)
{
    _local_path = ProjectSettings::get_singleton()->localize_path(p_path);
    _relative_paths = p_flags & ResourceSaver::FLAG_RELATIVE_PATHS;
    _skip_editor = p_flags & ResourceSaver::FLAG_OMIT_EDITOR_PROPERTIES;
    _bundle_resources = p_flags & ResourceSaver::FLAG_BUNDLE_RESOURCES;

    _take_over_paths = p_flags & ResourceSaver::FLAG_REPLACE_SUBRESOURCE_PATHS;
    if (!p_path.begins_with("res://"))
        _take_over_paths = false;

    _gather_resources(p_orchestration, true);

    String result;

    if (_write_orchestration_tag(p_orchestration, result) == OK)
    {
        if (_write_external_resource_tags(result) == OK)
        {
            if (_write_objects(p_orchestration, p_path, result) == OK)
            {
                if (_write_resource(p_orchestration, result) == OK)
                    return result;
            }
        }
    }

    return "";
}

String OrchestrationTextSerializer::get_start_tag(const String& p_type, const String& p_script_class, uint64_t p_resources, uint64_t p_version, int64_t p_uid)
{
    String tag = "[orchestration type=\"" + p_type + "\" ";
    if (!p_script_class.is_empty())
        tag += "script_class=\"" + p_script_class + "\" ";

    if (p_resources > 1)
        tag += "load_steps=" + itos(p_resources) + " ";

    tag += "format=" + itos(p_version) + "";

    if (p_uid != ResourceUID::INVALID_ID)
        tag += " uid=\"" + ResourceUID::get_singleton()->id_to_text(p_uid) + "\"";

    tag += "]\n";

    return tag;
}

String OrchestrationTextSerializer::get_ext_resource_tag(const String& p_type, const String& p_path, const String& p_uid, bool p_with_newline)
{
    String tag = "[ext_resource type=\"" + p_type + "\"";

    #if GODOT_VERSION >= 0x040300
    int64_t uid = _get_resource_id_for_path(p_path, false);
    #else
    int64_t uid = ResourceUID::INVALID_ID;
    #endif

    if (uid != ResourceUID::INVALID_ID)
        tag += " uid=\"" + ResourceUID::get_singleton()->id_to_text(uid) + "\"";

    tag += " path=\"" + p_path + "\" id=\"" + p_uid + "\"]";

    if (p_with_newline)
        tag += "\n";

    return tag;
}


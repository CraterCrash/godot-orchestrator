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
#include "script/serialization/binary_saver_instance.h"

#include "common/dictionary_utils.h"
#include "common/string_utils.h"
#include "common/version.h"
#include "script/script_server.h"

#include <godot_cpp/classes/missing_resource.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

bool OScriptBinaryResourceSaverInstance::NonPersistentKey::operator<(const NonPersistentKey& p_key) const
{
    return base == p_key.base ? property < p_key.property : base < p_key.base;
}

void OScriptBinaryResourceSaverInstance::_pad_buffer(Ref<FileAccess> p_file, int p_size)
{
    const int extra = 4 - (p_size % 4);
    if (extra < 4)
    {
        for (int i = 0; i < extra; i++)
            p_file->store_8(0); // pad to 32 bytes
    }
}

void OScriptBinaryResourceSaverInstance::_write_variant(
    const Ref<FileAccess>& p_file, const Variant& p_value, HashMap<Ref<Resource>, int>& p_resource_map,
    HashMap<Ref<Resource>, int>& p_external_resources, HashMap<StringName, int>& p_string_map, const PropertyInfo& p_hint)
{
switch (p_value.get_type())
    {
        case Variant::NIL:
        {
            p_file->store_32(VARIANT_NIL);
            // Do not store anything for null values
            break;
        }
        case Variant::BOOL:
        {
            p_file->store_32(VARIANT_BOOL);
            p_file->store_32(bool(p_value));
            break;
        }
        case Variant::INT:
        {
            int64_t val = p_value;
            if (val > 0x7FFFFFFF || val < -(int64_t)0x80000000)
            {
                p_file->store_32(VARIANT_INT64);
                p_file->store_64(val);
            }
            else
            {
                p_file->store_32(VARIANT_INT);
                p_file->store_32(int32_t(p_value));
            }
            break;
        }
        case Variant::FLOAT:
        {
            double d = p_value;
            float fl = d;
            if (double(fl) != d)
            {
                p_file->store_32(VARIANT_DOUBLE);
                p_file->store_double(d);
            }
            else
            {
                p_file->store_32(VARIANT_FLOAT);
                p_file->store_float(fl);
            }
            break;
        }
        case Variant::STRING:
        {
            p_file->store_32(VARIANT_STRING);
            _save_unicode_string(p_file, String(p_value));
            break;
        }
        case Variant::RECT2:
        {
            p_file->store_32(VARIANT_RECT2);
            Rect2 val = p_value;
            p_file->store_real(val.position.x);
            p_file->store_real(val.position.y);
            p_file->store_real(val.size.x);
            p_file->store_real(val.size.y);
            break;
        }
        case Variant::RECT2I:
        {
            p_file->store_32(VARIANT_RECT2I);
            Rect2i val = p_value;
            p_file->store_32(val.position.x);
            p_file->store_32(val.position.y);
            p_file->store_32(val.size.x);
            p_file->store_32(val.size.y);
            break;
        }
        case Variant::VECTOR2:
        {
            p_file->store_32(VARIANT_VECTOR2);
            Vector2 val = p_value;
            p_file->store_real(val.x);
            p_file->store_real(val.y);
            break;
        }
        case Variant::VECTOR2I:
        {
            p_file->store_32(VARIANT_VECTOR2I);
            Vector2i val = p_value;
            p_file->store_32(val.x);
            p_file->store_32(val.y);
            break;
        }
        case Variant::VECTOR3:
        {
            p_file->store_32(VARIANT_VECTOR3);
            Vector3 val = p_value;
            p_file->store_real(val.x);
            p_file->store_real(val.y);
            p_file->store_real(val.z);
            break;
        }
        case Variant::VECTOR3I:
        {
            p_file->store_32(VARIANT_VECTOR3I);
            Vector3i val = p_value;
            p_file->store_32(val.x);
            p_file->store_32(val.y);
            p_file->store_32(val.z);
            break;
        }
        case Variant::VECTOR4:
        {
            p_file->store_32(VARIANT_VECTOR4);
            Vector4 val = p_value;
            p_file->store_real(val.x);
            p_file->store_real(val.y);
            p_file->store_real(val.z);
            p_file->store_real(val.w);
            break;
        }
        case Variant::VECTOR4I:
        {
            p_file->store_32(VARIANT_VECTOR4I);
            Vector4i val = p_value;
            p_file->store_32(val.x);
            p_file->store_32(val.y);
            p_file->store_32(val.z);
            p_file->store_32(val.w);
            break;
        }
        case Variant::PLANE:
        {
            p_file->store_32(VARIANT_PLANE);
            Plane val = p_value;
            p_file->store_real(val.normal.x);
            p_file->store_real(val.normal.y);
            p_file->store_real(val.normal.z);
            p_file->store_real(val.d);
            break;
        }
        case Variant::QUATERNION:
        {
            p_file->store_32(VARIANT_QUATERNION);
            Quaternion val = p_value;
            p_file->store_real(val.x);
            p_file->store_real(val.y);
            p_file->store_real(val.z);
            p_file->store_real(val.w);
            break;
        }
        case Variant::AABB:
        {
            p_file->store_32(VARIANT_AABB);
            AABB val = p_value;
            p_file->store_real(val.position.x);
            p_file->store_real(val.position.y);
            p_file->store_real(val.position.z);
            p_file->store_real(val.size.x);
            p_file->store_real(val.size.y);
            p_file->store_real(val.size.z);
            break;
        }
        case Variant::TRANSFORM2D:
        {
            p_file->store_32(VARIANT_TRANSFORM2D);
            Transform2D val = p_value;
            p_file->store_real(val.columns[0].x);
            p_file->store_real(val.columns[0].y);
            p_file->store_real(val.columns[1].x);
            p_file->store_real(val.columns[1].y);
            p_file->store_real(val.columns[2].x);
            p_file->store_real(val.columns[2].y);
            break;
        }
        case Variant::BASIS:
        {
            p_file->store_32(VARIANT_BASIS);
            Basis val = p_value;
            p_file->store_real(val.rows[0].x);
            p_file->store_real(val.rows[0].y);
            p_file->store_real(val.rows[0].z);
            p_file->store_real(val.rows[1].x);
            p_file->store_real(val.rows[1].y);
            p_file->store_real(val.rows[1].z);
            p_file->store_real(val.rows[2].x);
            p_file->store_real(val.rows[2].y);
            p_file->store_real(val.rows[2].z);
            break;
        }
        case Variant::TRANSFORM3D:
        {
            p_file->store_32(VARIANT_TRANSFORM3D);
            Transform3D val = p_value;
            p_file->store_real(val.basis.rows[0].x);
            p_file->store_real(val.basis.rows[0].y);
            p_file->store_real(val.basis.rows[0].z);
            p_file->store_real(val.basis.rows[1].x);
            p_file->store_real(val.basis.rows[1].y);
            p_file->store_real(val.basis.rows[1].z);
            p_file->store_real(val.basis.rows[2].x);
            p_file->store_real(val.basis.rows[2].y);
            p_file->store_real(val.basis.rows[2].z);
            p_file->store_real(val.origin.x);
            p_file->store_real(val.origin.y);
            p_file->store_real(val.origin.z);
            break;
        }
        case Variant::PROJECTION:
        {
            p_file->store_32(VARIANT_PROJECTION);
            Projection val = p_value;
            p_file->store_real(val.columns[0].x);
            p_file->store_real(val.columns[0].y);
            p_file->store_real(val.columns[0].z);
            p_file->store_real(val.columns[0].w);
            p_file->store_real(val.columns[1].x);
            p_file->store_real(val.columns[1].y);
            p_file->store_real(val.columns[1].z);
            p_file->store_real(val.columns[1].w);
            p_file->store_real(val.columns[2].x);
            p_file->store_real(val.columns[2].y);
            p_file->store_real(val.columns[2].z);
            p_file->store_real(val.columns[2].w);
            p_file->store_real(val.columns[3].x);
            p_file->store_real(val.columns[3].y);
            p_file->store_real(val.columns[3].z);
            p_file->store_real(val.columns[3].w);
            break;
        }
        case Variant::COLOR:
        {
            p_file->store_32(VARIANT_COLOR);
            Color val = p_value;
            // Color are always floats
            p_file->store_float(val.r);
            p_file->store_float(val.g);
            p_file->store_float(val.b);
            p_file->store_float(val.a);
            break;
        }
        case Variant::STRING_NAME:
        {
            p_file->store_32(VARIANT_STRING_NAME);
            _save_unicode_string(p_file, String(p_value));
            break;
        }
        case Variant::NODE_PATH:
        {
            p_file->store_32(VARIANT_NODE_PATH);
            NodePath np = p_value;
            p_file->store_16(np.get_name_count());
            uint16_t snc = np.get_subname_count();
            if (np.is_absolute())
                snc |= 0x8000;

            p_file->store_16(snc);
            for (int i = 0; i < np.get_name_count(); i++)
            {
                if (_string_map.has(np.get_name(i)))
                    p_file->store_32(_string_map[np.get_name(i)]);
                else
                    _save_unicode_string(p_file, np.get_name(i), true);
            }
            for (int i = 0; i < np.get_subname_count(); i++)
            {
                if (_string_map.has(np.get_subname(i)))
                    p_file->store_32(_string_map[np.get_subname(i)]);
                else
                    _save_unicode_string(p_file, np.get_subname(i), true);
            }
            break;
        }
        case Variant::RID:
        {
            p_file->store_32(VARIANT_RID);
            WARN_PRINT("Cannot save RIDs (resource identifiers)");
            RID val = p_value;
            p_file->store_32(val.get_id());
            break;
        }
        case Variant::OBJECT:
        {
            p_file->store_32(VARIANT_OBJECT);
            Ref<Resource> res = p_value;
            if (res.is_null() || res->get_meta("_skip_save_", false))
            {
                // Object is empty
                p_file->store_32(OBJECT_EMPTY);
                return;
            }

            if (!_is_resource_built_in(res))
            {
                p_file->store_32(OBJECT_EXTERNAL_RESOURCE_INDEX);
                p_file->store_32(p_external_resources[res]);
            }
            else
            {
                if (!p_resource_map.has(res))
                {
                    p_file->store_32(OBJECT_EMPTY);
                    ERR_FAIL_MSG("Resource was not pre-cached, most likely a circular resource problem.");
                }

                p_file->store_32(OBJECT_INTERNAL_RESOURCE);
                p_file->store_32(p_resource_map[res]);
            }
            break;
        }
        case Variant::CALLABLE:
        {
            // There is no way to serialize a callable, only type is written.
            p_file->store_32(VARIANT_CALLABLE);
            break;
        }
        case Variant::SIGNAL:
        {
            // There is no way to serialize signals, only type is written.
            p_file->store_32(VARIANT_SIGNAL);
            break;
        }
        case Variant::DICTIONARY:
        {
            p_file->store_32(VARIANT_DICTIONARY);
            Dictionary d = p_value;
            p_file->store_32((uint32_t(d.size())));

            Array keys = d.keys();
            for (int i = 0; i < keys.size(); i++)
            {
                _write_variant(p_file, keys[i], p_resource_map, p_external_resources, p_string_map);
                _write_variant(p_file, d[keys[i]], p_resource_map, p_external_resources, p_string_map);
            }
            break;
        }
        case Variant::ARRAY:
        {
            p_file->store_32(VARIANT_ARRAY);
            Array array = p_value;
            p_file->store_32(array.size());
            for (int i = 0; i < array.size(); i++)
                _write_variant(p_file, array[i], p_resource_map, p_external_resources, p_string_map);
            break;
        }
        case Variant::PACKED_BYTE_ARRAY:
        {
            p_file->store_32(VARIANT_PACKED_BYTE_ARRAY);
            PackedByteArray array = p_value;
            const int size = static_cast<int>(array.size());
            p_file->store_32(size);
            const uint8_t*data = array.ptr();
            p_file->store_buffer(data, size);
            _pad_buffer(p_file, size);
            break;
        }
        case Variant::PACKED_INT32_ARRAY:
        {
            p_file->store_32(VARIANT_PACKED_INT32_ARRAY);
            PackedInt32Array array = p_value;
            const int size = array.size();
            p_file->store_32(size);
            for (int i = 0; i < size; i++)
                p_file->store_32(array[i]);
            break;
        }
        case Variant::PACKED_INT64_ARRAY:
        {
            p_file->store_32(VARIANT_PACKED_INT64_ARRAY);
            PackedInt64Array array = p_value;
            const int size = array.size();
            p_file->store_32(size);
            for (int i = 0; i < size; i++)
                p_file->store_64(array[i]);
            break;
        }
        case Variant::PACKED_FLOAT32_ARRAY:
        {
            p_file->store_32(VARIANT_PACKED_FLOAT32_ARRAY);
            PackedFloat32Array array = p_value;
            const int size = array.size();
            p_file->store_32(size);
            for (int i = 0; i < size; i++)
                p_file->store_float(array[i]);
            break;
        }
        case Variant::PACKED_FLOAT64_ARRAY:
        {
            p_file->store_32(VARIANT_PACKED_FLOAT64_ARRAY);
            PackedFloat64Array array = p_value;
            const int size = array.size();
            p_file->store_32(size);
            for (int i = 0; i < size; i++)
                p_file->store_double(array[i]);
            break;
        }
        case Variant::PACKED_STRING_ARRAY:
        {
            p_file->store_32(VARIANT_PACKED_STRING_ARRAY);
            PackedStringArray array = p_value;
            const int size = array.size();
            p_file->store_32(size);
            for (int i = 0; i < size; i++)
                _save_unicode_string(p_file, array[i]);
            break;
        }
        case Variant::PACKED_VECTOR2_ARRAY:
        {
            p_file->store_32(VARIANT_PACKED_VECTOR2_ARRAY);
            PackedVector2Array array = p_value;
            const int size = array.size();
            p_file->store_32(size);
            for (int i = 0; i < size; i++)
            {
                p_file->store_double(array[i].x);
                p_file->store_double(array[i].y);
            }
            break;
        }
        case Variant::PACKED_VECTOR3_ARRAY:
        {
            p_file->store_32(VARIANT_PACKED_VECTOR3_ARRAY);
            PackedVector3Array array = p_value;
            const int size = array.size();
            p_file->store_32(size);
            for (int i = 0; i < size; i++)
            {
                p_file->store_double(array[i].x);
                p_file->store_double(array[i].y);
                p_file->store_double(array[i].z);
            }
            break;
        }
        case Variant::PACKED_COLOR_ARRAY:
        {
            p_file->store_32(VARIANT_PACKED_COLOR_ARRAY);
            PackedColorArray array = p_value;
            const int size = array.size();
            p_file->store_32(size);
            for (int i = 0; i < size; i++)
            {
                p_file->store_float(array[i].r);
                p_file->store_float(array[i].g);
                p_file->store_float(array[i].b);
                p_file->store_float(array[i].a);
            }
            break;
        }
        case Variant::PACKED_VECTOR4_ARRAY:
        {
            p_file->store_32(VARIANT_PACKED_VECTOR4_ARRAY);
            PackedVector4Array array = p_value;
            const int size = array.size();
            p_file->store_32(size);
            for (int i = 0; i < size; i++)
            {
                p_file->store_double(array[i].x);
                p_file->store_double(array[i].y);
                p_file->store_double(array[i].z);
                p_file->store_double(array[i].w);
            }
            break;
        }
        default:
        {
            ERR_FAIL_MSG(vformat("Unable to serialize property type %s with name %s", p_value.get_type(), p_hint.name));
        }
    }
}

void OScriptBinaryResourceSaverInstance::_find_resources(const Variant& p_variant, bool p_main)
{
    switch (p_variant.get_type())
    {
        case Variant::OBJECT:
        {
            Ref<Resource> res = p_variant;
            if (res.is_null() || _external_resources.has(res) || res->get_meta(StringName("_skip_save_"), false))
                return;

            if (!p_main && !_is_resource_built_in(res))
            {
                if (res->get_path() == _path)
                {
                    ERR_PRINT(vformat("Circular references to resource being saved found: '%s' will be null next time its loaded.", _local_path));
                    return;
                }

                int idx = _external_resources.size();
                _external_resources[res] = idx;
                return;
            }

            if (_resource_set.has(res))
                return;

            _resource_set.insert(res);

            TypedArray<Dictionary> properties = res->get_property_list();
            for (int i = 0; i < properties.size(); i++)
            {
                const PropertyInfo pi = DictionaryUtils::to_property(properties[i]);
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
                        {
                            _find_resources(value);
                        }
                    }
                    else
                    {
                        _find_resources(value);
                    }
                }
            }
            _saved_resources.push_back(res);
        }
        break;

        case Variant::ARRAY:
        {
            Array array = p_variant;
            int size = array.size();
            for (int i = 0; i < size; i++)
            {
                const Variant& v = array[i];
                _find_resources(v);
            }
        }
        break;

        case Variant::DICTIONARY:
        {
            Dictionary dict = p_variant;
            Array keys = dict.keys();
            int size = keys.size();
            for (int i = 0; i < size; i++)
            {
                const Variant& key = keys[i];
                _find_resources(key);

                const Variant& value = dict[key];
                _find_resources(value);
            }
        }
        break;

        case Variant::NODE_PATH:
        {
            // Take the opportunity to save the node path strings
            NodePath np = p_variant;
            for (int i = 0; i < np.get_name_count(); i++)
                _get_string_index(np.get_name(i));
            for (int i = 0; i < np.get_subname_count(); i++)
                _get_string_index(np.get_subname(i));
        }
        break;

        default:
        {
        }
    }
}

int OScriptBinaryResourceSaverInstance::_get_string_index(const String& p_value)
{
    StringName sn = p_value;
    if (_string_map.has(sn))
        return _string_map[sn];

    _string_map[sn] = _strings.size();
    _strings.push_back(sn);

    return _strings.size() - 1;
}

String OScriptBinaryResourceSaverInstance::_resource_get_class(const Ref<Resource>& p_resource)
{
    Ref<MissingResource> missing = p_resource;
    if (missing.is_valid())
        return missing->get_original_class();

    return p_resource->get_class();
}

Error OScriptBinaryResourceSaverInstance::save(const String& p_path, const Ref<Resource>& p_resource, uint32_t p_flags)
{
    Ref<FileAccess> file = FileAccess::open_compressed(p_path, FileAccess::WRITE);
    ERR_FAIL_COND_V_MSG(!file.is_valid(), ERR_FILE_CANT_WRITE, "Cannot write to the file '" + p_path + "'.");

    _relative_paths = p_flags & ResourceSaver::FLAG_RELATIVE_PATHS;
    _skip_editor = p_flags & ResourceSaver::FLAG_OMIT_EDITOR_PROPERTIES;
    _bundle_resources = p_flags & ResourceSaver::FLAG_BUNDLE_RESOURCES;
    _big_endian = p_flags & ResourceSaver::FLAG_SAVE_BIG_ENDIAN;
    _takeover_paths = p_flags & ResourceSaver::FLAG_REPLACE_SUBRESOURCE_PATHS;

    if (!p_path.begins_with("res://"))
        _takeover_paths = false;

    _local_path = p_path.get_base_dir();
    _path = ProjectSettings::get_singleton()->localize_path(p_path);

    _find_resources(p_resource, true);

    static const uint8_t header[4] = { 'G', 'D', 'O', 'S' };
    file->store_buffer(header, 4);

    if (_big_endian)
    {
        file->store_32(1);
        file->set_big_endian(true);
    }
    else
    {
        file->store_32(0);
    }

    // 64-bit files, false for now
    file->store_32(0);

    // Store the format version of the file
    file->store_32(FORMAT_VERSION);

    // Store the version of Godot the extension was built with.
    file->store_32(GODOT_VERSION_MAJOR);
    file->store_32(GODOT_VERSION_MINOR);
    file->store_32(GODOT_VERSION_PATCH);

    if (file->get_error() != OK && file->get_error() != ERR_FILE_EOF)
        return ERR_CANT_CREATE;

    // Store the resource class name
    // This means that if the class is renamed, this will yield the file unloadable.
    // Therefore, if classes are renamed, a version bump and migration step will be necessary to reload.
    _save_unicode_string(file, p_resource->get_class());

    // Format 3 - script class, format flags, and uid
    String script_class;
    {
        uint32_t format_flags = FORMAT_FLAG_UIDS;
        Ref<Script> s = p_resource->get_script();
        if (s.is_valid())
        {
            script_class = ScriptServer::get_global_name(s);
            if (!script_class.is_empty())
                format_flags |= FORMAT_FLAG_HAS_SCRIPT_CLASS;
        }
        file->store_32(format_flags);
    }

    int64_t uid = _get_resource_id_for_path(p_path, true);
    file->store_64(uid);
    if (!script_class.is_empty())
        _save_unicode_string(file, script_class);

    // We explicitly leave some buffer for extended resource bits later on.
    // These fields will allow extension points without compromising the format.
    for (uint32_t i = 0; i < RESERVED_FIELDS; i++)
        file->store_32(0);

    Dictionary missing_resource_properties = p_resource->get_meta("_missing_resources", Dictionary());

    List<ResourceInfo> resources;
    for (const Ref<Resource>& E : _saved_resources)
    {
        ResourceInfo& ri = resources.push_back(ResourceInfo())->get();
        ri.type = _resource_get_class(E);

        TypedArray<Dictionary> properties = E->get_property_list();
        for (int i = 0; i < properties.size(); i++)
        {
            const PropertyInfo F = DictionaryUtils::to_property(properties[i]);

            if (_skip_editor && F.name.begins_with("__editor"))
                continue;

            if (F.name.match("metadata/_missing_resources"))
                continue;

            if (F.usage & PROPERTY_USAGE_STORAGE)
            {
                Property property;
                property.name_index = _get_string_index(F.name);

                if (F.usage & PROPERTY_USAGE_RESOURCE_NOT_PERSISTENT)
                {
                    NonPersistentKey key;
                    key.base = E;
                    key.property = F.name;
                    if (_non_persistent_map.has(key))
                        property.value = _non_persistent_map[key];
                }
                else
                {
                    property.value = E->get(F.name);
                }

                if (property.info.type == Variant::OBJECT && missing_resource_properties.has(F.name))
                {
                    // Was this missing resource overridden? If so, do not save the old value
                    Ref<Resource> res = property.value;
                    if (res.is_null())
                        property.value = missing_resource_properties[F.name];
                }

                #if GODOT_VERSION >= 0x040300
                Variant default_value = ClassDB::class_get_property_default_value(E->get_class(), F.name);
                #else
                Variant default_value;
                #endif
                if (default_value.get_type() != Variant::NIL)
                {
                    Variant result;
                    bool valid;
                    Variant::evaluate(Variant::OP_EQUAL, property.value, default_value, result, valid);
                    if (valid && bool(result))
                        continue;
                }

                property.info = F;
                ri.properties.push_back(property);
            }
        }
    }

    // Save string table
    // These are stored to minimize the file size rather than writing the string values multiple times
    file->store_32(_strings.size());
    for (int i = 0; i < _strings.size(); i++)
        _save_unicode_string(file, _strings[i]);

    // Store external resources
    file->store_32(_external_resources.size());
    Vector<Ref<Resource>> save_order;
    save_order.resize(_external_resources.size());
    for (const KeyValue<Ref<Resource>, int>& E : _external_resources)
        save_order.write[E.value] = E.key;

    for (int i = 0; i < save_order.size(); i++)
    {
        // get_save_class() delegates to get_class()
        _save_unicode_string(file, save_order[i]->get_class());
        String res_path = save_order[i]->get_path();
        res_path = _relative_paths ? StringUtils::path_to_file(_local_path, res_path) : res_path;
        _save_unicode_string(file, res_path);

        int64_t ruid = _get_resource_id_for_path(save_order[i]->get_path(), false);
        file->store_64(ruid);
    }

    // Store internal resources
    file->store_32(_saved_resources.size());

    HashSet<String> used_unique_ids;

    #if GODOT_VERSION >= 0x040300
    for (const Ref<Resource>& resource : _saved_resources)
    {
        if (_is_resource_built_in(resource))
        {
            if (!resource->get_scene_unique_id().is_empty())
            {
                if (used_unique_ids.has(resource->get_scene_unique_id()))
                    resource->set_scene_unique_id("");
                else
                    used_unique_ids.insert(resource->get_scene_unique_id());
            }
        }
    }
    #endif

    int res_index = 0;
    HashMap<Ref<Resource>, int> resource_map;
    Vector<uint64_t> offsets;

    for (const Ref<Resource>& resource : _saved_resources)
    {
        #if GODOT_VERSION >= 0x040300
        if (_is_resource_built_in(resource))
        {
            if (resource->get_scene_unique_id().is_empty())
            {
                String new_id;
                while(true)
                {
                    new_id = _resource_get_class(resource) + "_" + Resource::generate_scene_unique_id();
                    if (!used_unique_ids.has(new_id))
                        break;
                }
                resource->set_scene_unique_id(new_id);
                used_unique_ids.insert(new_id);
            }

            _save_unicode_string(file, "local://" + itos(res_index));
            if (_takeover_paths)
                resource->set_path(vformat("%s::%s", p_path, resource->get_scene_unique_id()));
            #if GODOT_VERSION >= 0x040400
            #ifdef TOOLS_ENABLED
            resource->set_edited(false);
            #endif
            #endif
        }
        else
        {
            _save_unicode_string(file, resource->get_path());
        }
        #else
        // All internal resources are written as "local://[index]"
        // This allows renaming and moving of files without impacting the data.
        //
        // When the file is loaded, the "local://" prefix is replaced with the resource path,
        // and "::" to handle uniquness within the Editor.
        _save_unicode_string(file, "local://" + itos(res_index));
        #endif

        // Save position reference and write placeholder, populating offset table later
        offsets.push_back(file->get_position());
        file->store_64(0);
        resource_map[resource] = res_index++;
    }

    Vector<uint64_t> offset_table;
    for (const ResourceInfo& ri : resources)
    {
        offset_table.push_back(file->get_position());
        _save_unicode_string(file, ri.type);

        file->store_32(ri.properties.size());
        for (const Property& property : ri.properties)
        {
            file->store_32(property.name_index);
            _write_variant(file, property.value, resource_map, _external_resources, _string_map, property.info);
        }
    }

    // Now flush offset table
    for (int i = 0; i < offset_table.size(); i++)
    {
        file->seek(offsets[i]);
        file->store_64(offset_table[i]);
    }

    file->seek_end();

    // Store sentinel at the end of the file
    file->store_buffer((const uint8_t*)"GDOS", 4);

    if (file->get_error() != OK && file->get_error() != ERR_FILE_EOF)
        return ERR_CANT_CREATE;

    return OK;
}

Error OScriptBinaryResourceSaverInstance::set_uid(const String& p_path, uint64_t p_uid)
{
    // todo: need to be completed
    return OK;
}

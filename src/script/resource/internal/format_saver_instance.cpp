// This file is part of the Godot Orchestrator project.
//
// Copyright (c) 2023-present Vahera Studios LLC and its contributors.
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
#include "format_saver_instance.h"

#include "common/dictionary_utils.h"

#include <godot_cpp/classes/missing_resource.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/core/version.hpp>

void OScriptResourceSaverInstance::_find_resources(const Variant& p_variant, bool p_main)
{
    switch (p_variant.get_type())
    {
        case Variant::OBJECT:
        {
            Ref<Resource> res = p_variant;
            if (res.is_null() || res->get_meta(StringName("_skip_save_"), false))
                return;

            if (!p_main && !_is_resource_built_in(res))
            {
                ERR_PRINT("External resources are not supported by the OrchestratorScript format");
                return;
            }

            if (resource_set.has(res))
                return;

            resource_set.insert(res);

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
                        non_persistent_map[key] = value;

                        Ref<Resource> sres = value;
                        if (sres.is_valid())
                        {
                            resource_set.insert(sres);
                            saved_resources.push_back(sres);
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

            saved_resources.push_back(res);
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

int OScriptResourceSaverInstance::_get_string_index(const String& p_value)
{
    StringName sn = p_value;
    if (string_map.has(sn))
        return string_map[sn];

    string_map[sn] = strings.size();
    strings.push_back(sn);

    return strings.size() - 1;
}

void OScriptResourceSaverInstance::_save_unicode_string(Ref<FileAccess> p_file, const String& p_value, bool p_bit_on_length)
{
    CharString utf8 = p_value.utf8();

    size_t len;
    if (p_bit_on_length)
        len = (utf8.length() + 1) | 0x8000000;
    else
        len = (utf8.length() + 1);

    p_file->store_32(len);
    p_file->store_buffer((const uint8_t*)utf8.get_data(), utf8.length() + 1);
}

String OScriptResourceSaverInstance::_resource_get_class(Ref<Resource> p_resource)
{
    Ref<MissingResource> missing = p_resource;
    if (missing.is_valid())
        return missing->get_original_class();

    return p_resource->get_class();
}

Error OScriptResourceSaverInstance::save(const String& p_path, const Ref<Resource>& p_resource, uint32_t p_flags)
{
    ERR_FAIL_COND_V_MSG(!p_path.ends_with(ORCHESTRATOR_SCRIPT_QUALIFIED_EXTENSION), ERR_FILE_UNRECOGNIZED,
                        "Unrecognized extension");
    ERR_FAIL_COND_V_MSG(!p_resource.is_valid(), ERR_INVALID_PARAMETER, "Resource is not valid");

    Ref<FileAccess> f = FileAccess::open_compressed(p_path, FileAccess::WRITE);
    ERR_FAIL_COND_V_MSG(!f.is_valid(), ERR_FILE_CANT_WRITE, "Cannot write to the file '" + p_path + "'");

    relative_paths = p_flags & ResourceSaver::FLAG_RELATIVE_PATHS;
    skip_editor = p_flags & ResourceSaver::FLAG_OMIT_EDITOR_PROPERTIES;
    bundle_resources = p_flags & ResourceSaver::FLAG_BUNDLE_RESOURCES;
    big_endian = p_flags & ResourceSaver::FLAG_SAVE_BIG_ENDIAN;
    takeover_paths = p_flags & ResourceSaver::FLAG_REPLACE_SUBRESOURCE_PATHS;

    if (!p_path.begins_with("res://"))
        takeover_paths = false;

    local_path = p_path.get_base_dir();
    path = ProjectSettings::get_singleton()->localize_path(p_path);

    _find_resources(p_resource, true);

    static const uint8_t header[4] = { 'G', 'D', 'O', 'S' };
    f->store_buffer(header, 4);

    if (big_endian)
    {
        f->store_32(1);
        f->set_big_endian(true);
    }
    else
    {
        f->store_32(0);
    }

    // 64-bit files, false for now
    f->store_32(0);

    // Store the format version of the file
    f->store_32(FORMAT_VERSION);

    // Store the version of Godot the extension was built with.
    f->store_32(GODOT_VERSION_MAJOR);
    f->store_32(GODOT_VERSION_MINOR);
    f->store_32(GODOT_VERSION_PATCH);

    if (f->get_error() != OK && f->get_error() != ERR_FILE_EOF)
        return ERR_CANT_CREATE;

    // Store the resource class name
    // This means that if the class is renamed, this will yield the file unloadable.
    // Therefore, if classes are renamed, a version bump and migration step will be necessary to reload.
    _save_unicode_string(f, p_resource->get_class());

    // We explicitly leave some buffer for extended resource bits later on.
    // These fields will allow extension points without compromising the format.
    for (int i = 0; i < RESERVED_FIELDS; i++)
        f->store_32(0);

    Dictionary missing_resource_properties = p_resource->get_meta("_missing_resources", Dictionary());

    List<ResourceData> resources;

    for (const Ref<Resource>& E : saved_resources)
    {
        ResourceData& rd = resources.push_back(ResourceData())->get();
        rd.type = _resource_get_class(E);

        TypedArray<Dictionary> properties = E->get_property_list();
        for (int i = 0; i < properties.size(); i++)
        {
            const PropertyInfo F = DictionaryUtils::to_property(properties[i]);
            if (skip_editor && F.name.begins_with("__editor"))
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
                    if (non_persistent_map.has(key))
                        property.value = non_persistent_map[key];
                }
                else
                    property.value = E->get(F.name);

                if (property.pi.type == Variant::OBJECT && missing_resource_properties.has(F.name))
                {
                    // Was this missing resource overridden?
                    // If so do not save the old value
                    Ref<Resource> res = property.value;
                    if (res.is_null())
                        property.value = missing_resource_properties[F.name];
                }

                Variant default_value = Variant();
                if (default_value.get_type() != Variant::NIL)
                {
                    Variant result;
                    bool valid;
                    Variant::evaluate(Variant::OP_EQUAL, property.value, default_value, result, valid);
                    if (valid && bool(result))
                        continue;
                }

                property.pi = F;
                rd.properties.push_back(property);
            }
        }
    }

    // String table
    // We store these in this way to minimize the file size rather than writing the string values
    // multiple times in the file, potentially allowing the file to grow unnecessarily.
    f->store_32(strings.size());
    for (int i = 0; i < strings.size(); i++)
        _save_unicode_string(f, strings[i]);

    // Internal resources
    f->store_32(saved_resources.size());
    {
        HashMap<Ref<Resource>, int> resource_map;
        Vector<uint64_t> offsets;

        int res_index = 0;
        for (Ref<Resource>& resource : saved_resources)
        {
            // All internal resources are written as "local://[index]".
            // This allows renaming and moving of the files without impacting the resource data
            // that is maintained within the resource file.
            //
            // When the file is loaded, the "local://" prefix is replaced with the resource path
            // and "::" to handle uniqueness within the Editor.
            _save_unicode_string(f, "local://" + itos(res_index));

            // Save position reference and write placeholder
            // The offset table will be populated later.
            offsets.push_back(f->get_position());
            f->store_64(0);
            resource_map[resource] = res_index++;
        }

        Vector<uint64_t> offset_table;
        for (const ResourceData& rd : resources)
        {
            offset_table.push_back(f->get_position());
            _save_unicode_string(f, rd.type);

            f->store_32(rd.properties.size());
            for (const Property& property : rd.properties)
            {
                f->store_32(property.name_index);
                _write_variant(f, property.value, resource_map, string_map, property.pi);
            }
        }

        // Now write offset table
        for (int i = 0; i < offset_table.size(); i++)
        {
            f->seek(offsets[i]);
            f->store_64(offset_table[i]);
        }

        f->seek_end();
    }

    // store a sentinel value at the end
    f->store_buffer((const uint8_t*)"GDOS", 4);

    if (f->get_error() != OK && f->get_error() != ERR_FILE_EOF)
        return ERR_CANT_CREATE;

    return OK;
}

void OScriptResourceSaverInstance::_write_variant(Ref<FileAccess> p_file, const Variant& p_value,
                                                  HashMap<Ref<Resource>,int>& p_resource_map,
                                                  HashMap<StringName, int>& p_string_map, const PropertyInfo& p_hint)
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
                if (string_map.has(np.get_name(i)))
                    p_file->store_32(string_map[np.get_name(i)]);
                else
                    _save_unicode_string(p_file, np.get_name(i), true);
            for (int i = 0; i < np.get_subname_count(); i++)
                if (string_map.has(np.get_subname(i)))
                    p_file->store_32(string_map[np.get_subname(i)]);
                else
                    _save_unicode_string(p_file, np.get_subname(i), true);
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

            if (!p_resource_map.has(res))
            {
                p_file->store_32(OBJECT_EMPTY);
                ERR_FAIL_MSG("Resource was not pre-cached, most likely a circular resource problem.");
            }

            p_file->store_32(OBJECT_INTERNAL_RESOURCE);
            p_file->store_32(p_resource_map[res]);
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
                _write_variant(p_file, keys[i], p_resource_map, p_string_map);
                _write_variant(p_file, d[keys[i]], p_resource_map, p_string_map);
            }
            break;
        }
        case Variant::ARRAY:
        {
            p_file->store_32(VARIANT_ARRAY);
            Array array = p_value;
            p_file->store_32(array.size());
            for (int i = 0; i < array.size(); i++)
                _write_variant(p_file, array[i], p_resource_map, p_string_map);
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
        default:
        {
            ERR_FAIL_MSG(vformat("Unable to serialize property type %s with name %s", p_value.get_type(), p_hint.name));
        }
    }
}

bool OScriptResourceSaverInstance::_is_resource_built_in(Ref<Resource> p_resource) const
{
    // taken from resource.h
    String path_cache = p_resource->get_path();
    return path_cache.is_empty() || path_cache.contains("::") || path_cache.begins_with("local://");
}

void OScriptResourceSaverInstance::_pad_buffer(Ref<FileAccess> p_file, int size)
{
    int extra = 4 - (size % 4);
    if (extra < 4)
    {
        for (int i = 0; i < extra; i++)
            p_file->store_8(0); // pad to 32 bytes
    }
}

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
#include "script/serialization/binary_loader_instance.h"

#include "common/string_utils.h"
#include "common/version.h"
#include "script/script.h"

#include <godot_cpp/classes/missing_resource.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_uid.hpp>

bool OScriptBinaryResourceLoaderInstance::_is_cached(const String& p_path)
{
    return ResourceLoader::get_singleton()->has_cached(p_path);
}

Ref<Resource> OScriptBinaryResourceLoaderInstance::_get_cached_ref(const String& p_path)
{
    #if GODOT_VERSION >= 0x040400
    return ResourceLoader::get_singleton()->get_cached_ref(p_path);
    #else
    return nullptr;
    #endif
}

String OScriptBinaryResourceLoaderInstance::_read_unicode_string()
{
    int length = _file->get_32();
    if (length == 0)
        return {};

    if (length > _string_buffer.size())
        _string_buffer.resize(length);

    _file->get_buffer((uint8_t*)&_string_buffer[0], length);

    return String::utf8(&_string_buffer[0]);
}

String OScriptBinaryResourceLoaderInstance::_read_string()
{
    uint32_t id = _file->get_32();
    if (id & 0x80000000)
    {
        uint32_t length = id & 0x7FFFFFFF;
        if ((int) length > _string_buffer.size())
            _string_buffer.resize(length);

        if (length == 0)
            return {};

        _file->get_buffer((uint8_t*)&_string_buffer[0], length);

        return String::utf8(&_string_buffer[0]);
    }
    return _string_map[id];
}

Error OScriptBinaryResourceLoaderInstance::_parse_variant(Variant& r_val)
{
    uint32_t variant_type = _file->get_32();
    switch (variant_type)
    {
        case VARIANT_NIL:
        {
            r_val = Variant();
            break;
        }
        case VARIANT_BOOL:
        {
            r_val = bool(_file->get_32());
            break;
        }
        case VARIANT_INT:
        {
            r_val = int(_file->get_32());
            break;
        }
        case VARIANT_INT64:
        {
            r_val = int64_t(_file->get_64());
            break;
        }
        case VARIANT_FLOAT:
        {
            r_val = _file->get_real();
            break;
        }
        case VARIANT_DOUBLE:
        {
            r_val = _file->get_double();
            break;
        }
        case VARIANT_STRING:
        {
            r_val = _read_unicode_string();
            break;
        }
        case VARIANT_RECT2:
        {
            Rect2 v;
            v.position.x = _file->get_real();
            v.position.y = _file->get_real();
            v.size.x = _file->get_real();
            v.size.y = _file->get_real();
            r_val = v;
            break;
        }
        case VARIANT_RECT2I:
        {
            Rect2i v;
            v.position.x = _file->get_32();
            v.position.y = _file->get_32();
            v.size.x = _file->get_32();
            v.size.y = _file->get_32();
            r_val = v;
            break;
        }
        case VARIANT_VECTOR2:
        {
            Vector2 v;
            v.x = _file->get_real();
            v.y = _file->get_real();
            r_val = v;
            break;
        }
        case VARIANT_VECTOR2I:
        {
            Vector2i v;
            v.x = _file->get_32();
            v.y = _file->get_32();
            r_val = v;
            break;
        }
        case VARIANT_VECTOR3:
        {
            Vector3 v;
            v.x = _file->get_real();
            v.y = _file->get_real();
            v.z = _file->get_real();
            r_val = v;
            break;
        }
        case VARIANT_VECTOR3I:
        {
            Vector3i v;
            v.x = _file->get_32();
            v.y = _file->get_32();
            v.z = _file->get_32();
            r_val = v;
            break;
        }
        case VARIANT_VECTOR4:
        {
            Vector4 v;
            v.x = _file->get_real();
            v.y = _file->get_real();
            v.z = _file->get_real();
            v.w = _file->get_real();
            r_val = v;
            break;
        }
        case VARIANT_VECTOR4I:
        {
            Vector4i v;
            v.x = _file->get_32();
            v.y = _file->get_32();
            v.z = _file->get_32();
            v.w = _file->get_32();
            r_val = v;
            break;
        }
        case VARIANT_PLANE:
        {
            Plane v;
            v.normal.x = _file->get_real();
            v.normal.y = _file->get_real();
            v.normal.z = _file->get_real();
            v.d = _file->get_real();
            r_val = v;
            break;
        }
        case VARIANT_QUATERNION:
        {
            Quaternion v;
            v.x = _file->get_real();
            v.y = _file->get_real();
            v.z = _file->get_real();
            v.w = _file->get_real();
            r_val = v;
            break;
        }
        case VARIANT_AABB:
        {
            AABB v;
            v.position.x = _file->get_real();
            v.position.y = _file->get_real();
            v.position.z = _file->get_real();
            v.size.x = _file->get_real();
            v.size.y = _file->get_real();
            v.size.z = _file->get_real();
            r_val = v;
            break;
        }
        case VARIANT_TRANSFORM2D:
        {
            Transform2D v;
            v.columns[0].x = _file->get_real();
            v.columns[0].y = _file->get_real();
            v.columns[1].x = _file->get_real();
            v.columns[1].y = _file->get_real();
            v.columns[2].x = _file->get_real();
            v.columns[2].y = _file->get_real();
            r_val = v;
            break;
        }
        case VARIANT_BASIS:
        {
            Basis v;
            v.rows[0].x = _file->get_real();
            v.rows[0].y = _file->get_real();
            v.rows[0].z = _file->get_real();
            v.rows[1].x = _file->get_real();
            v.rows[1].y = _file->get_real();
            v.rows[1].z = _file->get_real();
            v.rows[2].x = _file->get_real();
            v.rows[2].y = _file->get_real();
            v.rows[2].z = _file->get_real();
            r_val = v;
            break;
        }
        case VARIANT_TRANSFORM3D:
        {
            Transform3D v;
            v.basis.rows[0].x = _file->get_real();
            v.basis.rows[0].y = _file->get_real();
            v.basis.rows[0].z = _file->get_real();
            v.basis.rows[1].x = _file->get_real();
            v.basis.rows[1].y = _file->get_real();
            v.basis.rows[1].z = _file->get_real();
            v.basis.rows[2].x = _file->get_real();
            v.basis.rows[2].y = _file->get_real();
            v.basis.rows[2].z = _file->get_real();
            v.origin.x = _file->get_real();
            v.origin.y = _file->get_real();
            v.origin.z = _file->get_real();
            r_val = v;
            break;
        }
        case VARIANT_PROJECTION:
        {
            Projection v;
            v.columns[0].x = _file->get_real();
            v.columns[0].y = _file->get_real();
            v.columns[0].z = _file->get_real();
            v.columns[0].w = _file->get_real();
            v.columns[1].x = _file->get_real();
            v.columns[1].y = _file->get_real();
            v.columns[1].z = _file->get_real();
            v.columns[1].w = _file->get_real();
            v.columns[2].x = _file->get_real();
            v.columns[2].y = _file->get_real();
            v.columns[2].z = _file->get_real();
            v.columns[2].w = _file->get_real();
            v.columns[3].x = _file->get_real();
            v.columns[3].y = _file->get_real();
            v.columns[3].z = _file->get_real();
            v.columns[3].w = _file->get_real();
            r_val = v;
            break;
        }
        case VARIANT_COLOR:
        {
            // Colors should always be in single-precision.
            Color v;
            v.r = _file->get_float();
            v.g = _file->get_float();
            v.b = _file->get_float();
            v.a = _file->get_float();
            r_val = v;
            break;
        }
        case VARIANT_STRING_NAME:
        {
            r_val = StringName(_read_unicode_string());
            break;
        }
        case VARIANT_NODE_PATH:
        {
            [[maybe_unused]] bool absolute;
            Array names;
            Array subnames;

            int name_count = _file->get_16();
            int subname_count = _file->get_16();
            absolute = subname_count & 0x8000;
            subname_count &= 0x7FFF;

            for (int i = 0; i < name_count; i++)
                names.push_back(_read_string());

            for (int i = 0; i < subname_count; i++)
                subnames.push_back(_read_string());

            if (subname_count > 0)
            {
                ERR_FAIL_V_MSG(ERR_PARSE_ERROR, "Node paths cannot be read currently.");
            }

            const String name = StringUtils::join("/", names);
            r_val = NodePath(name);
            break;
        }
        case VARIANT_RID:
        {
            r_val = _file->get_32();
            break;
        }
        case VARIANT_OBJECT:
        {
            uint32_t obj_type = _file->get_32();
            switch (obj_type)
            {
                case OBJECT_EMPTY:
                {
                    // nothing else to do
                    break;
                }
                case OBJECT_INTERNAL_RESOURCE:
                {
                    uint32_t index = _file->get_32();

                    String path = _resource_path + "::" + itos(index);
                    if (!_internal_index_cache.has(path))
                    {
                        PackedStringArray known_names;
                        for (const KeyValue<String, Ref<Resource>>& E : _internal_index_cache)
                            known_names.push_back(E.key);

                        WARN_PRINT("Couldn't load resource (no cache): " + path
                                   + "; known: " + StringUtils::join(",", known_names));
                        r_val = Variant();
                    }
                    else
                    {
                        r_val = _internal_index_cache[path];
                    }
                    break;
                }
                case OBJECT_EXTERNAL_RESOURCE:
                case OBJECT_EXTERNAL_RESOURCE_INDEX:
                {
                    // Not currently used
                    r_val = Variant();
                    break;
                }
                default:
                    ERR_FAIL_V(ERR_FILE_CORRUPT);
            }
            break;
        }
        case VARIANT_CALLABLE:
        {
            // No data is stored, return an empty Variant
            r_val = Variant();
            break;
        }
        case VARIANT_SIGNAL:
        {
            // No data is stored, return an empty variant
            r_val = Variant();
            break;
        }
        case VARIANT_DICTIONARY:
        {
            uint32_t size = _file->get_32();
            size &= 0x7FFFFFFF;  // last bit means shared

            Dictionary dict;
            for (uint32_t i = 0; i < size; i++)
            {
                Variant key;
                Error err = _parse_variant(key);
                ERR_FAIL_COND_V_MSG(err, ERR_FILE_CORRUPT, "Error when trying to parse dictionary variant key");

                Variant value;
                err = _parse_variant(value);
                ERR_FAIL_COND_V_MSG(err, ERR_FILE_CORRUPT, "Error when trying to parse dictionary variant value");

                dict[key] = value;
            }
            r_val = dict;
            break;
        }
        case VARIANT_ARRAY:
        {
            uint32_t size = _file->get_32();
            size &= 0x7FFFFFFF;  // last bit means shared

            Array a;
            a.resize(size);
            for (uint32_t i = 0; i < size; i++)
            {
                Variant value;
                Error err = _parse_variant(value);
                ERR_FAIL_COND_V_MSG(err, ERR_FILE_CORRUPT, "Error when trying to parse array variant value");
                a[i] = value;
            }
            r_val = a;
            break;
        }
        case VARIANT_PACKED_BYTE_ARRAY:
        {
            uint32_t size = _file->get_32();

            PackedByteArray array;
            array.resize(size);

            uint8_t* data_ptr = array.ptrw();
            _file->get_buffer(data_ptr, size);

            _advance_padding(_file, size);

            r_val = array;
            break;
        }
        case VARIANT_PACKED_INT32_ARRAY:
        {
            uint32_t size = _file->get_32();

            PackedInt32Array array;
            array.resize(size);

            for (uint32_t i = 0; i < size; i++)
                array[i] = _file->get_32();

            r_val = array;
            break;
        }
        case VARIANT_PACKED_INT64_ARRAY:
        {
            uint32_t size = _file->get_32();

            PackedInt64Array array;
            array.resize(size);

            for (uint32_t i = 0; i < size; i++)
                array[i] = _file->get_64();

            r_val = array;
            break;
        }
        case VARIANT_PACKED_FLOAT32_ARRAY:
        {
            uint32_t size = _file->get_32();

            PackedFloat32Array array;
            array.resize(size);

            for (uint32_t i = 0; i < size; i++)
                array[i] = _file->get_float();

            r_val = array;
            break;
        }
        case VARIANT_PACKED_FLOAT64_ARRAY:
        {
            uint32_t size = _file->get_32();

            PackedFloat64Array array;
            array.resize(size);

            for (uint32_t i = 0; i < size; i++)
                array[i] = _file->get_double();

            r_val = array;
            break;
        }
        case VARIANT_PACKED_STRING_ARRAY:
        {
            uint32_t size = _file->get_32();

            PackedStringArray array;
            array.resize(size);

            for (uint32_t i = 0; i < size; i++)
                array[i] = _read_unicode_string();

            r_val = array;
            break;
        }
        case VARIANT_PACKED_VECTOR2_ARRAY:
        {
            uint32_t size = _file->get_32();

            PackedVector2Array array;
            array.resize(size);

            for (uint32_t i = 0; i < size; i++)
            {
                array[i].x = _file->get_double();
                array[i].y = _file->get_double();
            }

            r_val = array;
            break;
        }
        case VARIANT_PACKED_VECTOR3_ARRAY:
        {
            uint32_t size = _file->get_32();

            PackedVector3Array array;
            array.resize(size);

            for (uint32_t i = 0; i < size; i++)
            {
                array[i].x = _file->get_double();
                array[i].y = _file->get_double();
                array[i].z = _file->get_double();
            }

            r_val = array;
            break;
        }
        case VARIANT_PACKED_COLOR_ARRAY:
        {
            uint32_t size = _file->get_32();

            PackedColorArray array;
            array.resize(size);

            for (uint32_t i = 0; i < size; i++)
            {
                array[i].r = _file->get_float();
                array[i].g = _file->get_float();
                array[i].b = _file->get_float();
                array[i].a = _file->get_float();
            }

            r_val = array;
            break;
        }
        case VARIANT_PACKED_VECTOR4_ARRAY:
        {
            uint32_t size = _file->get_32();

            PackedVector4Array array;
            array.resize(size);

            for (uint32_t i = 0; i < size; i++)
            {
                array[i].x = _file->get_double();
                array[i].y = _file->get_double();
                array[i].z = _file->get_double();
                array[i].w = _file->get_double();
            }

            r_val = array;
            break;
        }
        default:
            ERR_FAIL_V(ERR_FILE_CORRUPT);
    }

    return OK;
}

void OScriptBinaryResourceLoaderInstance::_advance_padding(const Ref<FileAccess>& p_file, int p_size)
{
    const uint32_t extra = 4 - (p_size % 4);
    if (extra < 4)
    {
        for (uint32_t i = 0; i < extra; i++)
            p_file->get_8(); // pad to 32 bytes
    }
}

void OScriptBinaryResourceLoaderInstance::open(const Ref<FileAccess>& p_file, bool p_no_resources, bool p_keep_uuid_paths)
{
    _error = OK;
    _file = p_file;

    // Read the magic
    uint8_t header[4];
    _file->get_buffer(header, 4);
    if (header[0] != 'G' || header[1] != 'D' || header[2] != 'O' || header[3] != 'S')
    {
        _file.unref();
        _error = ERR_FILE_UNRECOGNIZED;
        ERR_FAIL_MSG("Unrecognized resource file: '" + _local_path + "'");
    }

    // Read the endianness
    bool big_endian = _file->get_32();
    _file->set_big_endian(big_endian != 0); // Set the file to read using big endian

    // Read whether to use double vs floats
    [[maybe_unused]] bool use_real64 = _file->get_32();

    // Read the file format version
    _version = _file->get_32();

    if (_version > FORMAT_VERSION)
    {
        _file.unref();
        ERR_FAIL_MSG(vformat(
            "File '%s' cannot be loaded, it uses a format (version %d) that is newer than the current version (%d).",
            _local_path,
            _version,
            FORMAT_VERSION));
    }

    uint32_t major = _file->get_32();
    uint32_t minor = _file->get_32();
    uint32_t patch = _file->get_32();
    _godot_version = major * 1000000 + minor * 1000 + patch;

    // Read resource type name
    _type = _read_unicode_string();

    // Skip reserved fields
    for (uint32_t i = 0; i < RESERVED_FIELDS; i++)
        [[maybe_unused]] uint32_t x = _file->get_32();

    // If resources aren't to be loaded, don't load them
    if (p_no_resources)
        return;

    // Read the string table
    _string_map.resize(_file->get_32());
    for (int i = 0; i < _string_map.size(); i++)
        _string_map.write[i] = _read_unicode_string();

    // Load external resource metadata (if needed)

    // Load internal resource metadata
    uint32_t internal_resource_count = _file->get_32();
    for (uint32_t i = 0; i < internal_resource_count; i++)
    {
        InternalResource ir;
        ir.path = _read_unicode_string();
        ir.offset = _file->get_64();
        _internal_resources.push_back(ir);
    }

    if (_file->eof_reached())
    {
        _error = ERR_FILE_CORRUPT;
        _file.unref();
        ERR_FAIL_MSG("Premature end of file (EOF): '" + _local_path + "'.");
    }
}

Error OScriptBinaryResourceLoaderInstance::load()
{
    // If the open call failed, immediately return
    if (_error != OK)
        return _error;

    // Load External Resources (if needed)

    // Load Internal Resources
    for (int i = 0; i < _internal_resources.size(); i++)
    {
        const bool main = i == (_internal_resources.size() - 1); // Whether main resource

        String path;
        String id;

        if (!main)
        {
            path = _internal_resources[i].path;
            if (path.begins_with("local://"))
            {
                path = StringUtils::replace_first(path, "local://", "");
                id = path;
                path = _resource_path + "::" + path;
                _internal_resources.write[i].path = path; // update path
            }

            #if GODOT_VERSION >= 0x040300
            if (_cache_mode == ResourceFormatLoader::CACHE_MODE_REUSE && _is_cached(path))
            {
                Ref<Resource> cached = _get_cached_ref(path);
                if (cached.is_valid())
                {
                    // Already loaded
                    _error = OK;
                    _internal_index_cache[path] = cached;
                    continue;
                }
            }
            #endif
        }
        else
        {
            if (ResourceFormatLoader::CACHE_MODE_IGNORE == _cache_mode && !_is_cached(_resource_path))
                path = _resource_path;
        }

        // Jump to the resource offset
        _file->seek(_internal_resources[i].offset);

        // Read the resource type
        String type = _read_unicode_string();

        Ref<Resource> res;
        #if GODOT_VERSION >= 0x040400
        if (_cache_mode == ResourceFormatLoader::CACHE_MODE_REPLACE && _is_cached(path))
        {
            // Use existing one
            Ref<Resource> cached = _get_cached_ref(path);
            if (cached->get_class() == type)
            {
                cached->reset_state();
                _resource = cached;
            }
        }
        #endif

        MissingResource* missing_resource = nullptr;
        if (res.is_null())
        {
            // Did not replace
            Variant instance = ClassDB::instantiate(type);
            Object* object = instance;
            if (!object)
            {
                _error = ERR_FILE_CORRUPT;
                ERR_FAIL_V_MSG(_error, _local_path + ": Resource of unrecognized type in file: " + type);
            }

            Resource* r = Object::cast_to<Resource>(object);
            if (!r)
            {
                String obj_class = object->get_class();
                memdelete(object);

                _error = ERR_FILE_CORRUPT;
                ERR_FAIL_V_MSG(_error, _local_path + ": Resource type is resource field not a resource, type is: " + obj_class);
            }

            res = Ref<Resource>(r);
            if (!path.is_empty() && _cache_mode != ResourceFormatLoader::CACHE_MODE_IGNORE)
            {
                // If we got here, it is because the resource with the same path has different ypes.
                // Ideally, should try to replace it.
                res->set_path(path);
            }

            #if GODOT_VERSION >= 0x040300
            res->set_scene_unique_id(id);
            #endif
        }

        if (!main)
            _internal_index_cache[path] = res;

        // Read properties
        int property_count = _file->get_32();
        Dictionary missing_resource_properties;
        for (int j = 0; j < property_count; j++)
        {
            const StringName name = _read_string();
            if (name == StringName())
            {
                _error = ERR_FILE_CORRUPT;
                ERR_FAIL_V(_error);
            }

            Variant value;
            _error = _parse_variant(value);
            if (_error != OK)
                return _error;

            bool set_valid = true;
            if (value.get_type() == Variant::OBJECT && missing_resource != nullptr)
            {
                // If the property being set is missing a resource (and the parent is not), then
                // setting it likely won't work, save it as metadata instead.
                Ref<MissingResource> mr = value;
                if (mr.is_valid())
                {
                    missing_resource_properties[name] = mr;
                    set_valid = false;
                }
            }

            if (value.get_type() == Variant::ARRAY)
            {
                const Array set_array = value;
                const Variant get_value = res->get(name);
                if (get_value.get_type() == Variant::ARRAY)
                {
                    const Array get_array = get_value;
                    if (!set_array.is_same_typed(get_array))
                    {
                        value = Array(set_array,
                            get_array.get_typed_builtin(),
                            get_array.get_typed_class_name(),
                            get_array.get_typed_script());
                    }
                }
            }

            if (set_valid)
                res->set(name, value);
        }

        if (missing_resource)
            missing_resource->set_recording_properties(false);

        if (!missing_resource_properties.is_empty())
            res->set_meta("_missing_resources", missing_resource_properties);

        #ifdef TOOLS_ENABLED
        #if GODOT_VERSION >= 0x040400
        res->set_edited(false);
        #endif
        #endif

        _resource_cache.push_back(res);

        if (main)
        {
            _resource_cache.push_back(res);

            _file.unref();

            _resource = res;
            _resource->set_message_translation(_translation_remapped);
            _error = OK;

            return _error;
        }
    }

    return ERR_FILE_EOF;
}

PackedStringArray OScriptBinaryResourceLoaderInstance::get_classes_used(const Ref<FileAccess>& p_file)
{
    PackedStringArray classes;
    open(p_file, false, true);
    if (_error == OK)
    {
        for(int i = 0; i < _internal_resources.size(); i++)
        {
            p_file->seek(_internal_resources[i].offset);
            String type = _read_unicode_string();
            ERR_FAIL_COND_V(p_file->get_error() != OK, {});
            if (type != String())
                classes.push_back(type);
        }
    }
    return classes;
}

OScriptBinaryResourceLoaderInstance::OScriptBinaryResourceLoaderInstance()
{
    _error = OK;
    _version = 0;
    _godot_version = 0;
    _uid = ResourceUID::INVALID_ID;
    _cache_mode = ResourceFormatLoader::CACHE_MODE_REUSE;
}
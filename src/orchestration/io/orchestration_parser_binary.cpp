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
#include "orchestration/io/orchestration_parser_binary.h"

#include "common/string_utils.h"
#include "orchestration/io/orchestration_format_binary.h"
#include "orchestration/orchestration.h"
#include "script/script.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/missing_resource.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

namespace godot
{
    class DirAccess;
}
String OrchestrationBinaryParser::_read_string(OrchestrationByteStream& p_stream)
{
    const uint32_t id = p_stream.read_u32();
    if (!p_stream.is_eof() && id & 0x80000000)
    {
        const uint32_t size = id & 0x7FFFFFFF;
        if (size == 0)
            return {};

        if (size > _string_buffer.size())
            _string_buffer.resize(size);

        p_stream.read_buffer(reinterpret_cast<uint8_t*>(_string_buffer.ptrw()), size);
        return String::utf8(&_string_buffer[0]);
    }

    return _string_map[id];
}

Error OrchestrationBinaryParser::_parse_magic(OrchestrationByteStream& p_stream)
{
    uint8_t header[4];
    if (p_stream.read_buffer(header, 4) == 4)
    {
        if (header[0] == 'G' || header[1] == 'D' || header[2] == 'O' || header[3] == 'S')
            return OK;

        return _set_error(ERR_FILE_UNRECOGNIZED, "Unrecognized resource file: '" + _local_path + "'");
    }

    return _set_error(ERR_FILE_CANT_READ, "Unrecognized resource file: '" + _local_path + "'");
}

Error OrchestrationBinaryParser::_parse_header(OrchestrationByteStream& p_stream)
{
    int field = 0;
    while (!p_stream.is_eof())
    {
        switch (field)
        {
            case 0: // endianness
            {
                // Shift stream's endianness
                bool big_endian = p_stream.read_u32();
                p_stream.set_big_endian(big_endian);
                field++;
                continue;
            }
            case 1: // 32 or 64 -bit floats
            {
                [[maybe_unused]] bool use_real64 = p_stream.read_u32();
                field++;
                continue;
            }
            case 2: // version
            {
                _version = p_stream.read_u32();
                if (_version > OrchestrationBinaryFormat::FORMAT_VERSION)
                {
                    return _set_error(
                        ERR_FILE_CANT_READ,
                        vformat("File '%s' cannot be read because it uses a format (%d) that is newer than the current format (%d).",
                            _local_path,
                            _version,
                            OrchestrationBinaryFormat::FORMAT_VERSION));
                }
                field++;
                continue;
            }
            case 3: // Godot version
            {
                bool eof = false;

                uint32_t parts[3];
                for (int i = 0; i < 3 && !eof; i++)
                {
                    parts[i] = p_stream.read_u32();
                    eof = p_stream.is_eof();
                }

                if (!eof)
                    _godot_version = parts[0] * 1000000 + parts[1] * 1000 + parts[2];

                field++;
                continue;
            }
            case 4:
                return OK;
        }
    }

    return _set_error(ERR_FILE_EOF, "Failed to read header");
}

Error OrchestrationBinaryParser::_parse_string_map(OrchestrationByteStream& p_stream)
{
    _string_map.resize(p_stream.read_u32());
    if (p_stream.is_eof())
        return _set_error(ERR_FILE_EOF, "Failed to read string map");

    for (int i = 0; i < _string_map.size(); i++)
    {
        _string_map.write[i] = p_stream.read_unicode_string();
        if (p_stream.is_eof())
            return _set_error(ERR_FILE_EOF, "Failed to read string map entry #" + itos(i));
    }

    return OK;
}

Error OrchestrationBinaryParser::_parse_resource_metadata(OrchestrationByteStream& p_stream)
{
    if (_version >= 3)
    {
        const uint32_t count = p_stream.read_u32();
        if (!p_stream.is_eof())
        {
            for (uint32_t i = 0; i < count; i++)
            {
                ExternalResource res;
                res.type = p_stream.read_unicode_string();
                if (p_stream.is_eof())
                    break;

                res.path = p_stream.read_unicode_string();
                if (p_stream.is_eof())
                    break;

                if (_flags & OrchestrationBinaryFormat::FORMAT_FLAG_UIDS)
                {
                    res.uid = p_stream.read_u64();
                    if (p_stream.is_eof())
                        break;

                    if (!_keep_uuid_paths && res.uid != ResourceUID::INVALID_ID)
                    {
                        if (ResourceUID::get_singleton()->has_id(res.uid))
                            res.path = ResourceUID::get_singleton()->get_id_path(res.uid);
                        else
                            _warn_invalid_external_resource_uid(i, res.path, res.uid);
                    }
                }

                _external_resources.push_back(res);
            }
        }
    }

    if (!p_stream.is_eof())
    {
        const uint32_t count = p_stream.read_u32();
        if (!p_stream.is_eof())
        {
            for (uint32_t i = 0; i < count; i++)
            {
                InternalResource res;
                res.path = p_stream.read_unicode_string();
                if (p_stream.is_eof())
                    break;

                res.offset = p_stream.read_u64();
                if (p_stream.is_eof())
                    break;

                _internal_resources.push_back(res);
            }
        }
    }

    return p_stream.is_eof() ? _set_error(ERR_FILE_EOF, "Unexpected end of file") : OK;
}

Error OrchestrationBinaryParser::_parse_resource(OrchestrationByteStream& p_stream, Ref<Orchestration>& r_orchestration)
{
    for (uint32_t i = 0; i < _external_resources.size(); i++)
    {
        String path = _external_resources[i].path;
        if (_remaps.has(path))
            path = _remaps[path];

        if (!path.contains("://") && path.is_relative_path())
            path = ProjectSettings::get_singleton()->localize_path(path.get_base_dir().path_join(_external_resources[i].path));

        _external_resources.write[i].path = path;
        // todo: support load tokens
    }

    for (uint32_t i = 0; i < _internal_resources.size(); i++)
    {
        const bool main = i == _internal_resources.size() - 1;

        String id;
        String path;

        if (!main)
        {
            path = _internal_resources[i].path;
            if (path.begins_with("local://"))
            {
                path = StringUtils::replace_first(path, "local://", "");
                id = path;
                path = _local_path + "::" + path;
                _internal_resources.write[i].path = path;
            }

            #if GODOT_VERSION >= 0x40300
            if (_cache_mode == ResourceFormatLoader::CACHE_MODE_REUSE && _is_cached(path))
            {
                const Ref<Resource> cached = _get_cached_resource(path);
                if (cached.is_valid())
                {
                    _set_error(OK);
                    _internal_index_cache[path] = cached;
                    continue;
                }
            }
            #endif
        }
        else if (ResourceFormatLoader::CACHE_MODE_IGNORE == _cache_mode && !_is_cached(_local_path))
            path = _local_path;

        p_stream.seek(_internal_resources[i].offset);

        String type = p_stream.read_unicode_string();
        if (p_stream.is_eof())
            return _set_error(ERR_FILE_EOF, "Unexpected end of file");

        Ref<Resource> res;
        #if GODOT_VERSION >= 0x040400
        if (_cache_mode == ResourceFormatLoader::CACHE_MODE_REPLACE && _is_cached(path))
        {
            const Ref<Resource> cached = _get_cached_resource(path);
            if (cached->get_class() == type)
            {
                cached->reset_state();
                res = cached;
            }
        }
        #endif

        MissingResource* missing_resource = nullptr;
        if (res.is_null())
        {
            if (OScript::get_class_static() == type && main)
                type = Orchestration::get_class_static();

            Variant instance = ClassDB::instantiate(type);

            Object* object = instance;
            if (!object)
            {
                if (_is_creating_missing_resources_if_class_unavailable_enabled())
                {
                    missing_resource = memnew(MissingResource);
                    missing_resource->set_original_class(type);
                    missing_resource->set_recording_properties(true);
                    object = missing_resource;
                }
                else
                    return _set_error(ERR_FILE_CORRUPT, _local_path + ": Resource of unrecognized type: " + type);
            }

            Resource* res_check = Object::cast_to<Resource>(object);
            if (!res_check)
            {
                const String class_name = object->get_class();
                memdelete(object);

                return _set_error(ERR_FILE_CORRUPT, _local_path + ": Resource type is not a resource: " + class_name);
            }

            res = Ref<Resource>(res_check);

            // The resource with the same path has a different type
            if (!path.is_empty() && _cache_mode != ResourceFormatLoader::CACHE_MODE_IGNORE)
                res->set_path(path);

            #if GODOT_VERSION >= 0x040300
            res->set_scene_unique_id(id);
            #endif
        }

        if (!main)
            _internal_index_cache[path] = res;

        const uint32_t count = p_stream.read_u32();
        if (p_stream.is_eof())
            return _set_error(ERR_FILE_EOF, "Unexpected end of file");

        Dictionary missing_resource_properties;
        for (uint32_t j = 0; j < count; j++)
        {
            const StringName property_name = _read_string(p_stream);
            if (property_name == StringName() || p_stream.is_eof())
                return _set_error(ERR_FILE_CORRUPT);

            Variant value;
            if (_parse_variant(p_stream, value) != OK)
                return _set_error(_error, _error_text);

            bool valid = true;
            if (value.get_type() == Variant::OBJECT && missing_resource == nullptr && _is_creating_missing_resources_if_class_unavailable_enabled())
            {
                Ref<MissingResource> mr = value;
                if (mr.is_valid())
                {
                    missing_resource_properties[property_name] = mr;
                    valid = false;
                }
            }

            if (value.get_type() == Variant::ARRAY)
            {
                const Array set_array = value;
                const Variant get_value = res->get(property_name);
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

            if (valid)
                res->set(property_name, value);
        }

        if (missing_resource)
            missing_resource->set_recording_properties(false);

        if (!missing_resource_properties.is_empty())
            res->set_meta("_missing_resources", missing_resource_properties);

        _set_resource_edited(res, false);
        _resource_cache.push_back(res);

        if (main)
        {
            _resource_cache.push_back(res);

            r_orchestration = res;
            r_orchestration->set_message_translation(_translation_remapped);

            return OK;
        }
    }

    return _set_error(ERR_FILE_EOF, "Unexpected end of file");
}

Error OrchestrationBinaryParser::_parse_variant(OrchestrationByteStream& p_stream, Variant& r_value)
{
    const uint32_t type = p_stream.read_u32();
    switch (type)
    {
        case OrchestrationBinaryFormat::VARIANT_NIL:
        {
            r_value = Variant();
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_BOOL:
        {
            r_value = static_cast<bool>(p_stream.read_u32());
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_INT:
        {
            r_value = static_cast<int>(p_stream.read_u32());
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_INT64:
        {
            r_value = static_cast<int64_t>(p_stream.read_u64());
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_FLOAT:
        {
            r_value = p_stream.read_real();
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_DOUBLE:
        {
            r_value = p_stream.read_double();
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_STRING:
        {
            r_value = p_stream.read_unicode_string();
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_RECT2:
        {
            Rect2 v;
            v.position.x = p_stream.read_real();
            v.position.y = p_stream.read_real();
            v.size.x = p_stream.read_real();
            v.size.y = p_stream.read_real();
            r_value = v;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_RECT2I:
        {
            Rect2i v;
            v.position.x = p_stream.read_u32();
            v.position.y = p_stream.read_u32();
            v.size.x = p_stream.read_u32();
            v.size.y = p_stream.read_u32();
            r_value = v;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_VECTOR2:
        {
            Vector2 v;
            v.x = p_stream.read_real();
            v.y = p_stream.read_real();
            r_value = v;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_VECTOR2I:
        {
            Vector2i v;
            v.x = p_stream.read_u32();
            v.y = p_stream.read_u32();
            r_value = v;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_VECTOR3:
        {
            Vector3 v;
            v.x = p_stream.read_real();
            v.y = p_stream.read_real();
            v.z = p_stream.read_real();
            r_value = v;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_VECTOR3I:
        {
            Vector3i v;
            v.x = p_stream.read_u32();
            v.y = p_stream.read_u32();
            v.z = p_stream.read_u32();
            r_value = v;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_VECTOR4:
        {
            Vector4 v;
            v.x = p_stream.read_real();
            v.y = p_stream.read_real();
            v.z = p_stream.read_real();
            v.w = p_stream.read_real();
            r_value = v;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_VECTOR4I:
        {
            Vector4i v;
            v.x = p_stream.read_u32();
            v.y = p_stream.read_u32();
            v.z = p_stream.read_u32();
            v.w = p_stream.read_u32();
            r_value = v;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_PLANE:
        {
            Plane v;
            v.normal.x = p_stream.read_real();
            v.normal.y = p_stream.read_real();
            v.normal.z = p_stream.read_real();
            v.d = p_stream.read_real();
            r_value = v;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_QUATERNION:
        {
            Quaternion v;
            v.x = p_stream.read_real();
            v.y = p_stream.read_real();
            v.z = p_stream.read_real();
            v.w = p_stream.read_real();
            r_value = v;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_AABB:
        {
            AABB v;
            v.position.x = p_stream.read_real();
            v.position.y = p_stream.read_real();
            v.position.z = p_stream.read_real();
            v.size.x = p_stream.read_real();
            v.size.y = p_stream.read_real();
            v.size.z = p_stream.read_real();
            r_value = v;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_TRANSFORM2D:
        {
            Transform2D v;
            v.columns[0].x = p_stream.read_real();
            v.columns[0].y = p_stream.read_real();
            v.columns[1].x = p_stream.read_real();
            v.columns[1].y = p_stream.read_real();
            v.columns[2].x = p_stream.read_real();
            v.columns[2].y = p_stream.read_real();
            r_value = v;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_BASIS:
        {
            Basis v;
            v.rows[0].x = p_stream.read_real();
            v.rows[0].y = p_stream.read_real();
            v.rows[0].z = p_stream.read_real();
            v.rows[1].x = p_stream.read_real();
            v.rows[1].y = p_stream.read_real();
            v.rows[1].z = p_stream.read_real();
            v.rows[2].x = p_stream.read_real();
            v.rows[2].y = p_stream.read_real();
            v.rows[2].z = p_stream.read_real();
            r_value = v;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_TRANSFORM3D:
        {
            Transform3D v;
            v.basis.rows[0].x = p_stream.read_real();
            v.basis.rows[0].y = p_stream.read_real();
            v.basis.rows[0].z = p_stream.read_real();
            v.basis.rows[1].x = p_stream.read_real();
            v.basis.rows[1].y = p_stream.read_real();
            v.basis.rows[1].z = p_stream.read_real();
            v.basis.rows[2].x = p_stream.read_real();
            v.basis.rows[2].y = p_stream.read_real();
            v.basis.rows[2].z = p_stream.read_real();
            v.origin.x = p_stream.read_real();
            v.origin.y = p_stream.read_real();
            v.origin.z = p_stream.read_real();
            r_value = v;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_PROJECTION:
        {
            Projection v;
            v.columns[0].x = p_stream.read_real();
            v.columns[0].y = p_stream.read_real();
            v.columns[0].z = p_stream.read_real();
            v.columns[0].w = p_stream.read_real();
            v.columns[1].x = p_stream.read_real();
            v.columns[1].y = p_stream.read_real();
            v.columns[1].z = p_stream.read_real();
            v.columns[1].w = p_stream.read_real();
            v.columns[2].x = p_stream.read_real();
            v.columns[2].y = p_stream.read_real();
            v.columns[2].z = p_stream.read_real();
            v.columns[2].w = p_stream.read_real();
            v.columns[3].x = p_stream.read_real();
            v.columns[3].y = p_stream.read_real();
            v.columns[3].z = p_stream.read_real();
            v.columns[3].w = p_stream.read_real();
            r_value = v;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_COLOR:
        {
            // Colors should always be in single-precision.
            Color v;
            v.r = p_stream.read_float();
            v.g = p_stream.read_float();
            v.b = p_stream.read_float();
            v.a = p_stream.read_float();
            r_value = v;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_STRING_NAME:
        {
            r_value = StringName(p_stream.read_unicode_string());
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_NODE_PATH:
        {
            [[maybe_unused]] bool absolute;
            Array names;
            Array subnames;

            int name_count = p_stream.read_u16();
            int subname_count = p_stream.read_u16();
            absolute = subname_count & 0x8000;
            subname_count &= 0x7FFF;

            for (int i = 0; i < name_count; i++)
                names.push_back(_read_string(p_stream));

            for (int i = 0; i < subname_count; i++)
                subnames.push_back(_read_string(p_stream));

            if (subname_count > 0)
            {
                ERR_FAIL_V_MSG(ERR_PARSE_ERROR, "Node paths cannot be read currently.");
            }

            const String name = StringUtils::join("/", names);
            r_value = NodePath(name);
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_RID:
        {
            r_value = p_stream.read_u32();
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_OBJECT:
        {
            uint32_t obj_type = p_stream.read_u32();
            switch (obj_type)
            {
                case OrchestrationBinaryFormat::OBJECT_EMPTY:
                {
                    // nothing else to do
                    break;
                }
                case OrchestrationBinaryFormat::OBJECT_INTERNAL_RESOURCE:
                {
                    uint32_t index = p_stream.read_u32();

                    String path = _local_path + "::" + itos(index);
                    if (!_internal_index_cache.has(path))
                    {
                        PackedStringArray known_names;
                        for (const KeyValue<String, Ref<Resource>>& E : _internal_index_cache)
                            known_names.push_back(E.key);

                        WARN_PRINT("Couldn't load resource (no cache): " + path
                                   + "; known: " + StringUtils::join(",", known_names));
                        r_value = Variant();
                    }
                    else
                    {
                        r_value = _internal_index_cache[path];
                    }
                    break;
                }
                case OrchestrationBinaryFormat::OBJECT_EXTERNAL_RESOURCE:
                {
                    String ext_type = p_stream.read_unicode_string();
                    String path = p_stream.read_unicode_string();
                    if (!path.contains("://") && path.is_relative_path())
                    {
                        // Path is relative to file being loaded, so convert to resource path
                        path = ProjectSettings::get_singleton()->localize_path(_local_path.get_base_dir().path_join(path));
                    }

                    if (_remaps.has(path))
                        path = _remaps[path];

                    Ref<Resource> res = ResourceLoader::get_singleton()->load(path, ext_type, ResourceLoader::CACHE_MODE_REUSE);

                    if (res.is_null())
                        WARN_PRINT(String("Couldn't load resource: " + path).utf8().get_data());

                    r_value = res;
                    break;
                }
                case OrchestrationBinaryFormat::OBJECT_EXTERNAL_RESOURCE_INDEX:
                {
                    // A new file format, refers to an index in the external list
                    int err_index = p_stream.read_u32();
                    if (err_index < 0 || err_index >= _external_resources.size())
                    {
                        WARN_PRINT("Broken external resource! (index out of size)");
                        r_value = Variant();
                    }
                    else
                    {
                        // todo: support load tokens
                        Ref<Resource> res = ResourceLoader::get_singleton()->load(_external_resources[err_index].path, _external_resources[err_index].type);
                        if (res.is_null())
                        {
                            ERR_FAIL_V_MSG(ERR_FILE_MISSING_DEPENDENCIES, "Cannot load dependency: " + _external_resources[err_index].path + ".");
                        }
                        r_value = res;
                    }
                    break;
                }
                default:
                    ERR_FAIL_V(ERR_FILE_CORRUPT);
            }
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_CALLABLE:
        {
            // No data is stored, return an empty Variant
            r_value = Variant();
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_SIGNAL:
        {
            // No data is stored, return an empty variant
            r_value = Variant();
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_DICTIONARY:
        {
            uint32_t size = p_stream.read_u32();
            size &= 0x7FFFFFFF;  // last bit means shared

            Dictionary dict;
            for (uint32_t i = 0; i < size; i++)
            {
                Variant key;
                Error err = _parse_variant(p_stream, key);
                ERR_FAIL_COND_V_MSG(err, ERR_FILE_CORRUPT, "Error when trying to parse dictionary variant key");

                Variant value;
                err = _parse_variant(p_stream, value);
                ERR_FAIL_COND_V_MSG(err, ERR_FILE_CORRUPT, "Error when trying to parse dictionary variant value");

                dict[key] = value;
            }
            r_value = dict;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_ARRAY:
        {
            uint32_t size = p_stream.read_u32();
            size &= 0x7FFFFFFF;  // last bit means shared

            Array a;
            a.resize(size);
            for (uint32_t i = 0; i < size; i++)
            {
                Variant value;
                Error err = _parse_variant(p_stream, value);
                ERR_FAIL_COND_V_MSG(err, ERR_FILE_CORRUPT, "Error when trying to parse array variant value");
                a[i] = value;
            }
            r_value = a;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_PACKED_BYTE_ARRAY:
        {
            uint32_t size = p_stream.read_u32();

            PackedByteArray array;
            array.resize(size);

            uint8_t* data_ptr = array.ptrw();
            p_stream.read_buffer(data_ptr, size);

            const uint32_t extra = 4 - (size % 4);
            if (extra < 4)
            {
                for (uint32_t i = 0; i < extra; i++)
                    p_stream.read_u8();
            }

            r_value = array;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_PACKED_INT32_ARRAY:
        {
            uint32_t size = p_stream.read_u32();

            PackedInt32Array array;
            array.resize(size);

            for (uint32_t i = 0; i < size; i++)
                array[i] = p_stream.read_u32();

            r_value = array;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_PACKED_INT64_ARRAY:
        {
            uint32_t size = p_stream.read_u32();

            PackedInt64Array array;
            array.resize(size);

            for (uint32_t i = 0; i < size; i++)
                array[i] = p_stream.read_u64();

            r_value = array;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_PACKED_FLOAT32_ARRAY:
        {
            uint32_t size = p_stream.read_u32();

            PackedFloat32Array array;
            array.resize(size);

            for (uint32_t i = 0; i < size; i++)
                array[i] = p_stream.read_float();

            r_value = array;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_PACKED_FLOAT64_ARRAY:
        {
            uint32_t size = p_stream.read_u32();

            PackedFloat64Array array;
            array.resize(size);

            for (uint32_t i = 0; i < size; i++)
                array[i] = p_stream.read_double();

            r_value = array;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_PACKED_STRING_ARRAY:
        {
            uint32_t size = p_stream.read_u32();

            PackedStringArray array;
            array.resize(size);

            for (uint32_t i = 0; i < size; i++)
                array[i] = p_stream.read_unicode_string();

            r_value = array;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_PACKED_VECTOR2_ARRAY:
        {
            uint32_t size = p_stream.read_u32();

            PackedVector2Array array;
            array.resize(size);

            for (uint32_t i = 0; i < size; i++)
            {
                array[i].x = p_stream.read_double();
                array[i].y = p_stream.read_double();
            }

            r_value = array;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_PACKED_VECTOR3_ARRAY:
        {
            uint32_t size = p_stream.read_u32();

            PackedVector3Array array;
            array.resize(size);

            for (uint32_t i = 0; i < size; i++)
            {
                array[i].x = p_stream.read_double();
                array[i].y = p_stream.read_double();
                array[i].z = p_stream.read_double();
            }

            r_value = array;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_PACKED_COLOR_ARRAY:
        {
            uint32_t size = p_stream.read_u32();

            PackedColorArray array;
            array.resize(size);

            for (uint32_t i = 0; i < size; i++)
            {
                array[i].r = p_stream.read_float();
                array[i].g = p_stream.read_float();
                array[i].b = p_stream.read_float();
                array[i].a = p_stream.read_float();
            }

            r_value = array;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_PACKED_VECTOR4_ARRAY:
        {
            uint32_t size = p_stream.read_u32();

            PackedVector4Array array;
            array.resize(size);

            for (uint32_t i = 0; i < size; i++)
            {
                array[i].x = p_stream.read_double();
                array[i].y = p_stream.read_double();
                array[i].z = p_stream.read_double();
                array[i].w = p_stream.read_double();
            }

            r_value = array;
            break;
        }
        default:
            return _set_error(ERR_FILE_CORRUPT, "Failed to parse variant value of type " + itos(type));
    }

    return OK;
}

Error OrchestrationBinaryParser::_parse(const String& p_path, ResourceFormatLoader::CacheMode p_cache_mode, bool p_parse_resources)
{
    if (!FileAccess::file_exists(p_path))
        return ERR_FILE_NOT_FOUND;

    _path = p_path;
    _local_path = ProjectSettings::get_singleton()->localize_path(p_path);
    _cache_mode = p_cache_mode;

    const Ref<FileAccess> file = FileAccess::open_compressed(_local_path, FileAccess::READ);
    ERR_FAIL_COND_V_MSG(!file.is_valid(), ERR_FILE_CANT_OPEN, "Failed to open file '" + _local_path + "'.");

    const PackedByteArray source = file->get_buffer(file->get_length());
    OrchestrationByteStream stream(source);

    if (_parse_magic(stream) == OK)
    {
        if (_parse_header(stream) == OK)
        {
            _res_type = stream.read_unicode_string();
            if (!stream.is_eof())
            {
                if (_version >= 3)
                    _flags = stream.read_u32();

                if (_flags & OrchestrationBinaryFormat::FORMAT_FLAG_UIDS)
                    _res_uid = stream.read_u64();

                if (_flags & OrchestrationBinaryFormat::FORMAT_FLAG_HAS_SCRIPT_CLASS)
                    _script_class = stream.read_unicode_string();

                // Skip reserved fields
                for (uint32_t i = 0; i < OrchestrationBinaryFormat::NUM_RESERVED_FIELDS; i++)
                    [[maybe_unused]] uint32_t field = stream.read_u32();

                if (_parse_string_map(stream) == OK)
                {
                    if (_parse_resource_metadata(stream) == OK)
                        return OK;
                }
            }
        }
    }

    return _error;
}

Ref<Orchestration> OrchestrationBinaryParser::parse(const Variant& p_source, const String& p_path, ResourceFormatLoader::CacheMode p_cache_mode)
{
    ERR_FAIL_COND_V_MSG(p_source.get_type() != Variant::PACKED_BYTE_ARRAY, {}, "Binary parser expects a PACKED_BYTE_ARRAY");

    _path = p_path;
    _local_path = ProjectSettings::get_singleton()->localize_path(p_path);
    _cache_mode = p_cache_mode;

    OrchestrationByteStream stream(p_source.operator PackedByteArray());

    if (_parse_magic(stream) == OK)
    {
        if (_parse_header(stream) == OK)
        {
            _res_type = stream.read_unicode_string();
            if (!stream.is_eof())
            {
                if (_version >= 3)
                    _flags = stream.read_u32();

                if (_flags & OrchestrationBinaryFormat::FORMAT_FLAG_UIDS)
                    _res_uid = stream.read_u64();

                if (_flags & OrchestrationBinaryFormat::FORMAT_FLAG_HAS_SCRIPT_CLASS)
                    _script_class = stream.read_unicode_string();

                // Skip reserved fields
                for (uint32_t i = 0; i < OrchestrationBinaryFormat::NUM_RESERVED_FIELDS; i++)
                    [[maybe_unused]] uint32_t field = stream.read_u32();

                if (_parse_string_map(stream) == OK)
                {
                    if (_parse_resource_metadata(stream) == OK)
                    {
                        Ref<Orchestration> orchestration;
                        if (_parse_resource(stream, orchestration) == OK)
                        {
                            if (orchestration.is_valid())
                            {
                                if (!orchestration->has_graph("EventGraph"))
                                    orchestration->create_graph("EventGraph", OScriptGraph::GraphFlags::GF_EVENT);

                                orchestration->post_initialize();
                                return orchestration;
                            }
                        }
                    }
                }
            }
        }
    }

    return {};
}

int64_t OrchestrationBinaryParser::get_uid(const String& p_path)
{
    if (_parse(p_path, ResourceFormatLoader::CACHE_MODE_IGNORE_DEEP, false) == OK)
        return _res_uid;

    return ResourceUID::INVALID_ID;
}

String OrchestrationBinaryParser::get_script_class(const String& p_path)
{
    if (_parse(p_path, ResourceFormatLoader::CACHE_MODE_IGNORE_DEEP, false) == OK)
        return _script_class;

    return "";
}

PackedStringArray OrchestrationBinaryParser::get_classes_used(const String& p_path)
{
    PackedStringArray classes_used;

    _path = p_path;
    _local_path = ProjectSettings::get_singleton()->localize_path(p_path);
    _cache_mode = ResourceFormatLoader::CACHE_MODE_IGNORE_DEEP;

    const Ref<FileAccess> file = FileAccess::open_compressed(_local_path, FileAccess::READ);
    ERR_FAIL_COND_V_MSG(!file.is_valid(), {}, "Failed to open file '" + p_path + "'.");

    const PackedByteArray source = file->get_buffer(file->get_length());
    OrchestrationByteStream stream(source);

    if (_parse_magic(stream) == OK)
    {
        if (_parse_header(stream) == OK)
        {
            _res_type = stream.read_unicode_string();
            if (!stream.is_eof())
            {
                if (_version >= 3)
                    _flags = stream.read_u32();

                if (_flags & OrchestrationBinaryFormat::FORMAT_FLAG_UIDS)
                    _res_uid = stream.read_u64();

                if (_flags & OrchestrationBinaryFormat::FORMAT_FLAG_HAS_SCRIPT_CLASS)
                    _script_class = stream.read_unicode_string();

                // Skip reserved fields
                for (uint32_t i = 0; i < OrchestrationBinaryFormat::NUM_RESERVED_FIELDS; i++)
                    [[maybe_unused]] uint32_t field = stream.read_u32();

                if (_parse_string_map(stream) == OK)
                {
                    if (_parse_resource_metadata(stream) == OK)
                    {
                        for (int i = 0; i < _internal_resources.size() - 1; i++)
                        {
                            const InternalResource& entry = _internal_resources[i];
                            stream.seek(entry.offset);

                            const String class_name = stream.read_unicode_string();
                            if (!class_name.is_empty() && !classes_used.has(class_name))
                                classes_used.push_back(class_name);
                        }
                    }
                }
            }
        }
    }
    return classes_used;
}

PackedStringArray OrchestrationBinaryParser::get_dependencies(const String& p_path, bool p_add_types)
{
    PackedStringArray dependencies;
    if (_parse(p_path, ResourceFormatLoader::CACHE_MODE_IGNORE_DEEP, false) == OK)
    {
        for (const ExternalResource& entry : _external_resources)
        {
            dependencies.push_back(p_add_types
                    ? vformat("%s::%s", entry.path, entry.type)
                    : entry.path);
        }
    }
    return dependencies;
}

Error OrchestrationBinaryParser::rename_dependencies(const String& p_path, const Dictionary& p_renames)
{
    _path = p_path;
    _local_path = ProjectSettings::get_singleton()->localize_path(p_path);
    _cache_mode = ResourceFormatLoader::CACHE_MODE_IGNORE_DEEP;

    const Ref<FileAccess> file = FileAccess::open_compressed(_local_path, FileAccess::READ);
    ERR_FAIL_COND_V_MSG(!file.is_valid(), {}, "Failed to open file '" + p_path + "'.");

    const PackedByteArray source = file->get_buffer(file->get_length());
    OrchestrationByteStream input(source);
    OrchestrationByteStream output;

    uint8_t header[4];
    input.read_buffer(header, sizeof(header));
    output.write_buffer(header, sizeof(header));

    bool big_endian = input.read_u32();
    bool use_real64 = input.read_u32();

    input.set_big_endian(big_endian != 0); // read big endian if saved as big endian

    output.write_u32(big_endian);
    output.set_big_endian(big_endian != 0);
    output.write_u32(use_real64);

    uint32_t version = input.read_u32();
    if (version > OrchestrationBinaryFormat::FORMAT_VERSION)
    {
        ERR_FAIL_V_MSG(
            ERR_FILE_UNRECOGNIZED,
            vformat("File '%s' cannot be loaded, it uses a format version (%d) which is not supported by the plugin version (%d)",
                p_path.get_base_dir(), version, OrchestrationBinaryFormat::FORMAT_VERSION));
    }

    output.write_u32(version);

    // Godot version
    output.write_u32(input.read_u32());
    output.write_u32(input.read_u32());
    output.write_u32(input.read_u32());

    // Resource type
    output.write_unicode_string(input.read_unicode_string());

    if (version >= 3)
    {
        uint32_t flags = input.read_u32();
        output.write_u32(flags);

        if (flags & OrchestrationBinaryFormat::FORMAT_FLAG_UIDS)
            output.write_u64(input.read_u64());

        if (flags & OrchestrationBinaryFormat::FORMAT_FLAG_HAS_SCRIPT_CLASS)
            output.write_unicode_string(input.read_unicode_string());
    }

    // Copy Reserved Fields
    for (uint32_t i = 0; i < OrchestrationBinaryFormat::NUM_RESERVED_FIELDS; i++)
        output.write_u32(input.read_u32());

    // String table
    uint32_t string_table_size = input.read_u32();
    output.write_u32(string_table_size);
    for (uint32_t i = 0; i < string_table_size; i++)
        output.write_unicode_string(input.read_unicode_string());

    if (version >= 3)
    {
        // External Resources
        uint32_t external_resource_size = input.read_u32();
        output.write_u32(external_resource_size);

        const String local_path = p_path.get_base_dir();
        for (uint32_t i = 0; i < external_resource_size; i++)
        {
            const String type = input.read_unicode_string();

            String path = input.read_unicode_string();

            int64_t uid = input.read_u64();
            if (uid != ResourceUID::INVALID_ID && ResourceUID::get_singleton()->has_id(uid))
                path = ResourceUID::get_singleton()->get_id_path(uid);

            bool relative = false;
            if (!path.begins_with("res://"))
            {
                path = local_path.path_join(path).simplify_path();
                relative = true;
            }

            if (p_renames.has(path))
                path = p_renames[path];

            const String full_path = path;
            if (relative)
                path = StringUtils::path_to(local_path, path);

            output.write_unicode_string(type);
            output.write_unicode_string(path);
            output.write_u64(_get_resource_id_for_path(full_path));
        }
    }

    const int64_t size_delta = output.tell() - input.tell();

    // Internal Resources
    const uint32_t internal_resource_size = input.read_u32();
    output.write_u32(internal_resource_size);
    for (uint32_t i = 0; i < internal_resource_size; i++)
    {
        output.write_unicode_string(input.read_unicode_string());
        output.write_u64(input.read_u64() + size_delta);
    }

    // Remainder of the file
    uint8_t b = input.read_u8();
    while (!input.is_eof())
    {
        output.write_u8(b);
        b = input.read_u8();
    }

    const String depren_file = vformat("%s.depren", p_path);

    const Ref<FileAccess> output_file = FileAccess::open_compressed(depren_file, FileAccess::WRITE);
    ERR_FAIL_COND_V_MSG(output_file.is_null(), ERR_FILE_CORRUPT, vformat("Cannot create file '%s'.", depren_file));
    output_file->store_buffer(output.get_as_bytes());

    return output_file->get_error() == OK ? OK : ERR_CANT_CREATE;
}


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
#include "orchestration/serialization/binary/binary_parser.h"

#include "common/string_utils.h"
#include "orchestration/orchestration.h"
#include "orchestration/serialization/binary/binary_format.h"
#include "orchestration/serialization/format.h"

#include <godot_cpp/classes/packet_peer_udp.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>

bool OrchestrationBinaryParser::_is_cached(const String& p_path) {
    return ResourceLoader::get_singleton()->has_cached(p_path);
}

Ref<Resource> OrchestrationBinaryParser::_get_cache_ref(const String& p_path) {
    return ResourceLoader::get_singleton()->get_cached_ref(p_path);
}

String OrchestrationBinaryParser::_read_unicode_string() {
    return OrchestrationBinaryFormat::read_unicode_string(_file);
}

String OrchestrationBinaryParser::_read_string() {
    uint32_t id = _file->get_32();
    if (id & 0x80000000) {
        uint32_t size = id & 0x7FFFFFFF;

        if (size > _string_buffer.size()) {
            _string_buffer.resize(size);
        }
        if (size == 0) {
            return {};
        }

        _file->get_buffer(reinterpret_cast<uint8_t*>(_string_buffer[0]), size);
        return String::utf8(&_string_buffer[0]);
    }

    return _string_map[id];
}

Error OrchestrationBinaryParser::_parse_variant(Variant& r_value) { // NOLINT
    uint32_t variant_type = _file->get_32();

    switch (variant_type) {
        case OrchestrationBinaryFormat::VARIANT_NIL: {
            r_value = Variant();
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_BOOL: {
            r_value = static_cast<bool>(_file->get_32());
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_INT: {
            r_value = static_cast<int>(_file->get_32());
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_INT64: {
            r_value = static_cast<int64_t>(_file->get_64());
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_FLOAT: {
            r_value = _file->get_real();
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_DOUBLE: {
            r_value = _file->get_double();
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_STRING: {
            r_value = _read_unicode_string();
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_RECT2: {
            Rect2 r;
            r.position.x = _file->get_real();
            r.position.y = _file->get_real();
            r.size.x = _file->get_real();
            r.size.y = _file->get_real();
            r_value = r;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_RECT2I: {
            Rect2i r;
            r.position.x = static_cast<int32_t>(_file->get_32());
            r.position.y = static_cast<int32_t>(_file->get_32());
            r.size.x = static_cast<int32_t>(_file->get_32());
            r.size.y = static_cast<int32_t>(_file->get_32());
            r_value = r;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_VECTOR2: {
            Vector2 v;
            v.x = _file->get_real();
            v.y = _file->get_real();
            r_value = v;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_VECTOR2I: {
            Vector2i v;
            v.x = static_cast<int32_t>(_file->get_32());
            v.y = static_cast<int32_t>(_file->get_32());
            r_value = v;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_VECTOR3: {
            Vector3 v;
            v.x = _file->get_real();
            v.y = _file->get_real();
            v.z = _file->get_real();
            r_value = v;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_VECTOR3I: {
            Vector3i v;
            v.x = static_cast<int32_t>(_file->get_32());
            v.y = static_cast<int32_t>(_file->get_32());
            v.z = static_cast<int32_t>(_file->get_32());
            r_value = v;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_VECTOR4: {
            Vector4 v;
            v.x = _file->get_real();
            v.y = _file->get_real();
            v.z = _file->get_real();
            v.w = _file->get_real();
            r_value = v;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_VECTOR4I: {
            Vector4i v;
            v.x = static_cast<int32_t>(_file->get_32());
            v.y = static_cast<int32_t>(_file->get_32());
            v.z = static_cast<int32_t>(_file->get_32());
            v.w = static_cast<int32_t>(_file->get_32());
            r_value = v;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_PLANE: {
            Plane p;
            p.normal.x = _file->get_real();
            p.normal.y = _file->get_real();
            p.normal.z = _file->get_real();
            p.d = _file->get_real();
            r_value = p;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_QUATERNION: {
            Quaternion q;
            q.x = _file->get_real();
            q.y = _file->get_real();
            q.z = _file->get_real();
            q.w = _file->get_real();
            r_value = q;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_AABB: {
            AABB aabb;
            aabb.position.x = _file->get_real();
            aabb.position.y = _file->get_real();
            aabb.position.z = _file->get_real();
            aabb.size.x = _file->get_real();
            aabb.size.y = _file->get_real();
            aabb.size.z = _file->get_real();
            r_value = aabb;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_TRANSFORM2D: {
            Transform2D t;
            t.columns[0].x = _file->get_real();
            t.columns[0].y = _file->get_real();
            t.columns[1].x = _file->get_real();
            t.columns[1].y = _file->get_real();
            t.columns[2].x = _file->get_real();
            t.columns[2].y = _file->get_real();
            r_value = t;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_BASIS: {
            Basis basis;
            basis.rows[0].x = _file->get_real();
            basis.rows[0].y = _file->get_real();
            basis.rows[0].z = _file->get_real();
            basis.rows[1].x = _file->get_real();
            basis.rows[1].y = _file->get_real();
            basis.rows[1].z = _file->get_real();
            basis.rows[2].x = _file->get_real();
            basis.rows[2].y = _file->get_real();
            basis.rows[2].z = _file->get_real();
            r_value = basis;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_TRANSFORM3D: {
            Transform3D t;
            t.basis.rows[0].x = _file->get_real();
            t.basis.rows[0].y = _file->get_real();
            t.basis.rows[0].z = _file->get_real();
            t.basis.rows[1].x = _file->get_real();
            t.basis.rows[1].y = _file->get_real();
            t.basis.rows[1].z = _file->get_real();
            t.basis.rows[2].x = _file->get_real();
            t.basis.rows[2].y = _file->get_real();
            t.basis.rows[2].z = _file->get_real();
            t.origin.x = _file->get_real();
            t.origin.y = _file->get_real();
            t.origin.z = _file->get_real();
            r_value = t;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_PROJECTION: {
            Projection p;
            p.columns[0].x = _file->get_real();
            p.columns[0].y = _file->get_real();
            p.columns[0].z = _file->get_real();
            p.columns[0].w = _file->get_real();
            p.columns[1].x = _file->get_real();
            p.columns[1].y = _file->get_real();
            p.columns[1].z = _file->get_real();
            p.columns[1].w = _file->get_real();
            p.columns[2].x = _file->get_real();
            p.columns[2].y = _file->get_real();
            p.columns[2].z = _file->get_real();
            p.columns[2].w = _file->get_real();
            p.columns[3].x = _file->get_real();
            p.columns[3].y = _file->get_real();
            p.columns[3].z = _file->get_real();
            p.columns[3].w = _file->get_real();
            r_value = p;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_COLOR: {
            // Colors should always be in single-precision.
            Color color;
            color.r = _file->get_float();
            color.g = _file->get_float();
            color.b = _file->get_float();
            color.a = _file->get_float();
            r_value = color;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_STRING_NAME: {
            r_value = StringName(_read_unicode_string());
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_NODE_PATH: {
            [[maybe_unused]] bool absolute;
            Array names;
            Array subnames;

            int name_count = _file->get_16();
            int subname_count = _file->get_16();
            absolute = subname_count & 0x8000;
            subname_count &= 0x7FFF;

            for (int i = 0; i < name_count; i++) {
                names.push_back(_read_string());
            }

            for (int i = 0; i < subname_count; i++) {
                subnames.push_back(_read_string());
            }

            if (subname_count > 0) {
                ERR_FAIL_V_MSG(ERR_PARSE_ERROR, "Node paths cannot be read currently.");
            }

            r_value = NodePath(StringUtils::join("/", names));
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_RID: {
            r_value = _file->get_32();
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_OBJECT: {
            const uint32_t obj_type = _file->get_32();
            switch (obj_type) {
                case OrchestrationBinaryFormat::OBJECT_EMPTY: {
                    // nothing else to do
                    break;
                }
                case OrchestrationBinaryFormat::OBJECT_INTERNAL_RESOURCE: {
                    uint32_t index = _file->get_32();

                    String path = _path + "::" + itos(index);
                    if (!_internal_index_cache.has(path)) {
                        PackedStringArray known_names;
                        for (const KeyValue<String, Ref<Resource>>& E : _internal_index_cache) {
                            known_names.push_back(E.key);
                        }
                        WARN_PRINT("Couldn't load resource (no cache): " + path + "; known: " + StringUtils::join(",", known_names));
                        r_value = Variant();
                    } else {
                        r_value = _internal_index_cache[path];
                    }
                    break;
                }
                case OrchestrationBinaryFormat::OBJECT_EXTERNAL_RESOURCE: {
                    String ext_type = _read_unicode_string();
                    String path = _read_unicode_string();
                    if (!path.contains("://") && path.is_relative_path()) {
                        // Path is relative to file being loaded, so convert to resource path
                        path = ProjectSettings::get_singleton()->localize_path(_path.get_base_dir().path_join(path));
                    }

                    if (_remaps.has(path)) {
                        path = _remaps[path];
                    }

                    const Ref<Resource> res = ResourceLoader::get_singleton()->load(path, ext_type,
                        static_cast<ResourceLoader::CacheMode>(_cache_mode_for_external));

                    if (res.is_null()) {
                        WARN_PRINT(String("Couldn't load resource: " + path).utf8().get_data());
                    }

                    r_value = res;
                    break;
                }
                case OrchestrationBinaryFormat::OBJECT_EXTERNAL_RESOURCE_INDEX: {
                    // A new file format, refers to an index in the external list
                    int32_t err_index = _file->get_32();
                    if (err_index < 0 || err_index >= _external_resources.size()) {
                        WARN_PRINT("Broken external resource! (index out of size)");
                        r_value = Variant();
                    } else {
                        // todo: support load tokens
                        Ref<Resource> res = ResourceLoader::get_singleton()->load(_external_resources[err_index].path, _external_resources[err_index].type);
                        if (res.is_null()) {
                            ERR_FAIL_V_MSG(ERR_FILE_MISSING_DEPENDENCIES, "Cannot load dependency: " + _external_resources[err_index].path + ".");
                        }
                        r_value = res;
                    }
                    break;
                }
                default: {
                    ERR_FAIL_V(ERR_FILE_CORRUPT);
                }
            }
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_CALLABLE:
        case OrchestrationBinaryFormat::VARIANT_SIGNAL: {
            // No data is stored, return an empty Variant
            r_value = Variant();
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_DICTIONARY: {
            uint32_t size = _file->get_32();
            size &= 0x7FFFFFFF;  // last bit means shared

            Dictionary dict;
            for (uint32_t i = 0; i < size; i++) {
                Variant key;
                Error err = _parse_variant(key);
                ERR_FAIL_COND_V_MSG(err, ERR_FILE_CORRUPT, "Error when trying to parse dictionary variant key");

                Variant value;
                err = _parse_variant(value);
                ERR_FAIL_COND_V_MSG(err, ERR_FILE_CORRUPT, "Error when trying to parse dictionary variant value");

                dict[key] = value;
            }
            r_value = dict;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_ARRAY: {
            uint32_t size = _file->get_32();
            size &= 0x7FFFFFFF;  // last bit means shared

            Array a;
            a.resize(size);
            for (uint32_t i = 0; i < size; i++) {
                Variant value;
                Error err = _parse_variant(value);
                ERR_FAIL_COND_V_MSG(err, ERR_FILE_CORRUPT, "Error when trying to parse array variant value");
                a[i] = value;
            }
            r_value = a;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_PACKED_BYTE_ARRAY: {
            const uint32_t size = _file->get_32();

            PackedByteArray array;
            array.resize(size);

            uint8_t* data_ptr = array.ptrw();
            _file->get_buffer(data_ptr, size);

            // Advance padding
            const uint32_t extra = 4 - (size % 4); // NOLINT
            if (extra < 4) {
                for (uint32_t j = 0; j < extra; j++) {
                    [[maybe_unused]] uint8_t u = _file->get_8();
                }
            }

            r_value = array;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_PACKED_INT32_ARRAY: {
            const uint32_t size = _file->get_32();

            PackedInt32Array array;
            array.resize(size);

            for (uint32_t i = 0; i < size; i++) {
                array[i] = _file->get_32();
            }

            r_value = array;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_PACKED_INT64_ARRAY: {
            const uint32_t size = _file->get_32();

            PackedInt64Array array;
            array.resize(size);

            for (uint32_t i = 0; i < size; i++) {
                array[i] = _file->get_64();
            }

            r_value = array;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_PACKED_FLOAT32_ARRAY: {
            const uint32_t size = _file->get_32();

            PackedFloat32Array array;
            array.resize(size);

            for (uint32_t i = 0; i < size; i++) {
                array[i] = _file->get_float();
            }

            r_value = array;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_PACKED_FLOAT64_ARRAY: {
            const uint32_t size = _file->get_32();

            PackedFloat64Array array;
            array.resize(size);

            for (uint32_t i = 0; i < size; i++) {
                array[i] = _file->get_double();
            }

            r_value = array;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_PACKED_STRING_ARRAY: {
            const uint32_t size = _file->get_32();

            PackedStringArray array;
            array.resize(size);

            for (uint32_t i = 0; i < size; i++) {
                array[i] = _read_unicode_string();
            }

            r_value = array;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_PACKED_VECTOR2_ARRAY: {
            const uint32_t size = _file->get_32();

            PackedVector2Array array;
            array.resize(size);

            for (uint32_t i = 0; i < size; i++) {
                array[i].x = _file->get_double();
                array[i].y = _file->get_double();
            }

            r_value = array;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_PACKED_VECTOR3_ARRAY: {
            const uint32_t size = _file->get_32();

            PackedVector3Array array;
            array.resize(size);

            for (uint32_t i = 0; i < size; i++) {
                array[i].x = _file->get_double();
                array[i].y = _file->get_double();
                array[i].z = _file->get_double();
            }

            r_value = array;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_PACKED_COLOR_ARRAY: {
            const uint32_t size = _file->get_32();

            PackedColorArray array;
            array.resize(size);

            for (uint32_t i = 0; i < size; i++) {
                array[i].r = _file->get_float();
                array[i].g = _file->get_float();
                array[i].b = _file->get_float();
                array[i].a = _file->get_float();
            }

            r_value = array;
            break;
        }
        case OrchestrationBinaryFormat::VARIANT_PACKED_VECTOR4_ARRAY: {
            const uint32_t size = _file->get_32();

            PackedVector4Array array;
            array.resize(size);

            for (uint32_t i = 0; i < size; i++) {
                array[i].x = _file->get_double();
                array[i].y = _file->get_double();
                array[i].z = _file->get_double();
                array[i].w = _file->get_double();
            }

            r_value = array;
            break;
        }
        default: {
            ERR_FAIL_V(ERR_FILE_CORRUPT);
        }
    }

    return OK;
}

Error OrchestrationBinaryParser::_read_header_block() {
    ERR_FAIL_COND_V(_file.is_null(), ERR_FILE_CANT_READ);
    _file->seek(0);

    uint8_t header[4];
    _file->get_buffer(header, sizeof(header));
    if (header[0] != 'G' || header[1] != 'D' || header[2] != 'O' || header[3] != 'S') {
        _error_text = vformat("Unrecognized resource file: '%s'", _path);
        return ERR_FILE_UNRECOGNIZED;
    }

    // Setup endianness
    bool big_endian = _file->get_32();
    _file->set_big_endian(big_endian != 0);

    // Read whether this file uses single or double precision.
    [[maybe_unused]] bool use_real64 = _file->get_32();

    _version = _file->get_32();
    if (_version > OrchestrationFormat::FORMAT_VERSION) {
        _error_text = vformat("File '%s' cannot be loaded, it uses a format (version %d) that is newer than the current version (%d).",
            _path, _version, OrchestrationFormat::FORMAT_VERSION);
        return ERR_FILE_CANT_READ;
    }

    uint32_t major = _file->get_32();
    uint32_t minor = _file->get_32();
    uint32_t patch = _file->get_32();
    _godot_version = major * 1000000 + minor * 1000 + patch;

    _type = _read_unicode_string();

    if (_version >= 3) {
        _flags = _file->get_32();
        if (_flags & OrchestrationBinaryFormat::FORMAT_FLAG_UIDS) {
            _uid = static_cast<int64_t>(_file->get_64());
        }
        if (_flags & OrchestrationBinaryFormat::FORMAT_FLAG_HAS_SCRIPT_CLASS) {
            _script_class = _read_unicode_string();
        }
        if (_flags & OrchestrationBinaryFormat::FORMAT_FLAG_HAS_ICON_PATH) {
            _icon_path = _read_unicode_string();
        }
    }

    for (uint32_t i = 0; i < OrchestrationBinaryFormat::RESERVED_FIELDS; i++) {
        [[maybe_unused]] uint32_t reserved = _file->get_32();
    }

    _header_block_size = _file->get_position();

    return OK;
}

Error OrchestrationBinaryParser::_read_resource_metadata(bool p_keep_uuid_paths) {
    ERR_FAIL_COND_V_MSG(_header_block_size == 0, ERR_FILE_CANT_READ, "Resource metadata requires first reading the file header");
    _file->seek(_header_block_size);

    _string_map.resize(_file->get_32());
    for (int64_t i = 0; i < _string_map.size(); i++) {
        _string_map.write[i] = _read_unicode_string();
    }

    if (_version >= 3) {
        // When binary format was introduced, it did not write a zero for the external resource count.
        // To remain backward compatible, the version was changed so that older resources can continue
        // to be loaded safely.
        const uint32_t external_count = _file->get_32();
        for (uint32_t i = 0; i < external_count; i++) {
            ExternalResource external;
            external.type = _read_unicode_string();
            external.path = _read_unicode_string();

            if (_flags & OrchestrationBinaryFormat::FORMAT_FLAG_UIDS) {
                external.uid = static_cast<int64_t>(_file->get_64());
            }

            if (!p_keep_uuid_paths && external.uid != ResourceUID::INVALID_ID) {
                if (ResourceUID::get_singleton()->has_id(external.uid)) {
                    // When UID is found and path is valid use it; otherwise fallback to path
                    external.path = ResourceUID::get_singleton()->get_id_path(external.uid);
                } else {
                    const String message = vformat(
                        "%s: In editor resource #%d, invalid UID: %d - using text path instead: %s",
                        _path, i, external.uid, external.path);
                    #ifdef TOOLS_ENABLED
                    if (ResourceLoader::get_singleton()->get_resource_uid(_path) != external.uid) {
                        WARN_PRINT(message);
                    }
                    #else
                    WARN_PRINT(message);
                    #endif
                }
            }

            _external_resources.push_back(external);
        }
    }

    uint32_t internal_count = _file->get_32();
    for (uint32_t i = 0; i < internal_count; i++) {
        InternalResource internal;
        internal.path = _read_unicode_string();
        internal.offset = _file->get_64();
        _internal_resources.push_back(internal);
    }

    _resource_metadata_block_size = _file->get_position() - _header_block_size;

    return _file->eof_reached() ? ERR_FILE_CORRUPT : OK;
}

Error OrchestrationBinaryParser::_load_resource_properties(Ref<Resource>& r_resource, MissingResource* r_missing_resource) {
    uint32_t property_count = _file->get_32();

    Dictionary missing_resource_properties;
    for (uint32_t i = 0; i < property_count; i++) {
        const StringName name = _read_string();
        ERR_FAIL_COND_V(name == StringName(), ERR_FILE_CORRUPT);

        Variant value;
        Error err = _parse_variant(value);
        if (err != OK) {
            return err;
        }

        _set_resource_property(r_resource, r_missing_resource, name, value, missing_resource_properties);
    }

    if (r_missing_resource) {
        r_missing_resource->set_recording_properties(false);
    }

    if (!missing_resource_properties.is_empty()) {
        r_resource->set_meta("_missing_resources", missing_resource_properties);
    }

    return OK;
}

Error OrchestrationBinaryParser::_load() {
    ERR_FAIL_COND_V_MSG(_header_block_size == 0, ERR_FILE_CANT_READ, "Please use _read_header_block first.");
    ERR_FAIL_COND_V_MSG(_resource_metadata_block_size == 0, ERR_FILE_CANT_READ, "Please use _read_resource_metadata first.");
    _file->seek(_header_block_size + _resource_metadata_block_size);

    for (uint32_t i = 0; i < _external_resources.size(); i++) {
        String path = _external_resources[i].path;
        if (_remaps.has(path)) {
            path = _remaps[path];
        }

        if (!path.contains("://") && path.is_relative_path()) {
            path = ProjectSettings::get_singleton()->localize_path(path.get_base_dir().path_join(_external_resources[i].path));
        }

        _external_resources.write[i].path = path;
    }

    uint32_t internal_size = _internal_resources.size();
    for (uint32_t i = 0; i < internal_size; i++) {
        String path;
        String id;

        const bool is_main = i == internal_size - 1;
        if (!is_main) {
            path = _internal_resources[i].path;
            if (path.begins_with("local://")) {
                path = StringUtils::replace_first(path, "local://", "");
                id = path;
                path = _path + "::" + path;
                _internal_resources.write[i].path = path;
            }

            if (ResourceFormatLoader::CACHE_MODE_REUSE == _cache_mode && _is_cached(path)) {
                const Ref<Resource> cached = _get_cache_ref(path);
                if (cached.is_valid()) {
                    _internal_index_cache[path] = cached;
                    continue;
                }
            }
        } else {
            if (ResourceFormatLoader::CACHE_MODE_IGNORE == _cache_mode && !_is_cached(_path)) {
                path = _path;
            }
        }

        // Jump to resource offset block
        _file->seek(_internal_resources[i].offset);

        String resource_type = _read_unicode_string();

        Ref<Resource> resource;
        if (ResourceFormatLoader::CACHE_MODE_REPLACE == _cache_mode && _is_cached(path)) {
            const Ref<Resource> cached = _get_cache_ref(path);
            if (cached->get_class() == resource_type) {
                cached->reset_state();
                _resource = cached;
            }
        }

        MissingResource* missing_resource = nullptr;
        if (resource.is_null()) {

            Variant instance = _instantiate_resource(resource_type);
            Object* object = instance;
            if (!object) {
                if (_is_creating_missing_resources_if_class_unavailable_enabled()) {
                    missing_resource = memnew(MissingResource);
                    missing_resource->set_original_class(resource_type);
                    missing_resource->set_recording_properties(true);
                    object = missing_resource;
                } else {
                    ERR_FAIL_V_MSG(ERR_FILE_CORRUPT, _path + ": Resource of unrecognized type in file: " + resource_type);
                }
            }

            Resource* res = Object::cast_to<Resource>(object);
            if (!res) {
                String object_class = object->get_class();
                memdelete(object);
                ERR_FAIL_V_MSG(ERR_FILE_CORRUPT, _path + ": Object type is not a resource, type is: " + object_class);
            }

            resource = Ref<Resource>(res);
            if (!path.is_empty() && ResourceFormatLoader::CACHE_MODE_IGNORE != _cache_mode) {
                // resource->set_path(path);
            }

            resource->set_scene_unique_id(id);
        }

        if (!is_main) {
            _internal_index_cache[path] = resource;
        }

        Error err = _load_resource_properties(resource, missing_resource);
        if (err != OK) {
            return err;
        }

        _set_resource_edited(resource, false);
        _resource_cache.push_back(resource);

        if (is_main) {
            _file.unref();

            _resource = resource;
            _resource->set_message_translation(_translation_remapped);
            return OK;
        }
    }

    return ERR_FILE_EOF;
}

Error OrchestrationBinaryParser::_open(const Ref<FileAccess>& p_file, bool p_no_resources, bool p_keep_uuid_paths) {
    _file = p_file;

    Error err = _read_header_block();
    if (err != OK) {
        return err;
    }

    if (p_no_resources) {
        return OK;
    }

    return _read_resource_metadata(p_keep_uuid_paths);
}

String OrchestrationBinaryParser::get_resource_script_class(const String& p_path) {
    const Ref<FileAccess> file = FileAccess::open_compressed(p_path, FileAccess::READ);
    ERR_FAIL_COND_V(file.is_null(), {});

    Error err = _open(file, true);
    if (err != OK) {
        return {};
    }

    if (_flags & OrchestrationBinaryFormat::FORMAT_FLAG_HAS_SCRIPT_CLASS) {
        return _script_class;
    }

    return {};
}

int64_t OrchestrationBinaryParser::get_resource_uid(const String& p_path) {
    // When creating a new script, this is called, and the file path won't yet be valid.
    if (!FileAccess::file_exists(p_path)) {
        return ResourceUID::INVALID_ID;
    }

    const Ref<FileAccess> file = FileAccess::open_compressed(p_path, FileAccess::READ);
    if (file.is_null()) {
        return ResourceUID::INVALID_ID;
    }

    Error err = _open(file, true);
    if (err != OK) {
        return ResourceUID::INVALID_ID;
    }

    return _uid;
}

PackedStringArray OrchestrationBinaryParser::get_dependencies(const String& p_path, bool p_add_types) {
    const Ref<FileAccess> file = FileAccess::open_compressed(p_path, FileAccess::READ);
    ERR_FAIL_COND_V(file.is_null(), {});

    Error err = _open(file, false, true);
    if (err != OK) {
        return {};
    }

    PackedStringArray results;
    for (const auto& [path, type, uid] : _external_resources) {
        String uid_text;
        String fallback_path;

        if (uid != ResourceUID::INVALID_ID) {
            uid_text = ResourceUID::get_singleton()->id_to_text(uid);
            fallback_path = path;
        } else {
            uid_text = path;
        }

        if (p_add_types && !type.is_empty()) {
            uid_text += "::" + type;
        }

        if (!fallback_path.is_empty()) {
            if (!p_add_types) {
                uid_text += "::"; // Ensures that path comes third, even when no type
            }
            uid_text += "::" + fallback_path;
        }

        results.push_back(uid_text);
    }
    return results;
}

Error OrchestrationBinaryParser::rename_dependencies(const String& p_path, const Dictionary& p_renames) {
    const Ref<FileAccess> file = FileAccess::open_compressed(p_path, FileAccess::READ);
    ERR_FAIL_COND_V(file.is_null(), ERR_FILE_CANT_READ);

    Ref<FileAccess> fw = FileAccess::open_compressed(vformat("%s.depren", p_path), FileAccess::WRITE);
    ERR_FAIL_COND_V(fw.is_null(), ERR_FILE_CANT_WRITE);

    String local_path = p_path.get_base_dir();

    uint8_t header[4];
    file->get_buffer(header, 4);
    fw->store_buffer(header, 4);

    bool big_endian = file->get_32();
    bool use_real64 = file->get_32();
    fw->store_32(big_endian);
    fw->store_32(use_real64);

    file->set_big_endian(big_endian != 0);
    fw->set_big_endian(big_endian != 0);

    // Format
    uint32_t format = file->get_32();
    if (format > OrchestrationFormat::FORMAT_VERSION) {
        ERR_FAIL_V_MSG(ERR_FILE_UNRECOGNIZED,
            vformat("File '%s' cannot be loaded as it uses a format version (%d) that is newer than version %d",
                local_path, format));
    }

    // Godot version
    fw->store_32(file->get_32());
    fw->store_32(file->get_32());
    fw->store_32(file->get_32());

    // Resource type
    OrchestrationBinaryFormat::save_unicode_string(fw, OrchestrationBinaryFormat::read_unicode_string(file));

    // Flags
    bool using_uids = false;
    uint32_t flags = file->get_32();
    fw->store_32(flags);
    if (flags & OrchestrationBinaryFormat::FORMAT_FLAG_UIDS) {
        fw->store_64(file->get_64());
        using_uids = true;
    }
    if (flags & OrchestrationBinaryFormat::FORMAT_FLAG_HAS_SCRIPT_CLASS) {
        OrchestrationBinaryFormat::save_unicode_string(fw, OrchestrationBinaryFormat::read_unicode_string(file));
    }
    if (flags & OrchestrationBinaryFormat::FORMAT_FLAG_HAS_ICON_PATH) {
        OrchestrationBinaryFormat::save_unicode_string(fw, OrchestrationBinaryFormat::read_unicode_string(file));
    }

    // Reserved files
    for (uint32_t i = 0; i < OrchestrationBinaryFormat::RESERVED_FIELDS; i++) {
        fw->store_32(file->get_32());
    }

    uint32_t string_table_size = file->get_32();
    fw->store_32(string_table_size);
    for (uint32_t i = 0; i < string_table_size; i++) {
        OrchestrationBinaryFormat::save_unicode_string(fw, _read_unicode_string());
    }

    // External Resources
    uint32_t external_resource_count = file->get_32();
    file->store_32(external_resource_count);
    for (uint32_t i = 0; i < external_resource_count; i++) {
        String type = OrchestrationBinaryFormat::read_unicode_string(file);
        String path = OrchestrationBinaryFormat::read_unicode_string(file);
        if (using_uids) {
            int64_t uid = file->get_64();
            if (uid != ResourceUID::INVALID_ID) {
                if (ResourceUID::get_singleton()->has_id(uid)) {
                    path = ResourceUID::get_singleton()->get_id_path(uid);
                }
            }
        }

        bool relative = false;
        if (!path.begins_with("res://")) {
            path = local_path.path_join(path).simplify_path();
            relative = true;
        }

        if (p_renames.has(path)) {
            path = p_renames[path];
        }

        String full_path = path;
        if (relative) {
            path = StringUtils::path_to_file(local_path, path);
        }

        OrchestrationBinaryFormat::save_unicode_string(fw, type);
        OrchestrationBinaryFormat::save_unicode_string(fw, path);

        if (using_uids) {
            int64_t uid = ResourceSaver::get_singleton()->get_resource_id_for_path(full_path);
            fw->store_64(static_cast<uint64_t>(uid));
        }
    }

    // At this point the remainder of the file is the same; however the offsets for the internal
    // resources need to be adjusted based on this delta.
    const int64_t delta = static_cast<int64_t>(fw->get_position()) - static_cast<int64_t>(file->get_position());

    uint32_t internal_resource_count = file->get_32();
    file->store_32(internal_resource_count);
    for (uint32_t i = 0; i < internal_resource_count; i++) {
        const String path = OrchestrationBinaryFormat::read_unicode_string(file);
        uint64_t offset = file->get_64();
        OrchestrationBinaryFormat::save_unicode_string(fw, path);
        fw->store_64(offset + delta);
    }

    // Remainder of the file
    uint8_t b = file->get_8();
    while (!file->eof_reached()) {
        fw->store_8(b);
        b = file->get_8();
    }

    const bool all_ok = fw->get_error() == OK;
    if (!all_ok) {
        return ERR_CANT_CREATE;
    }

    return OK;
}

PackedStringArray OrchestrationBinaryParser::get_classes_used(const String& p_path) {
    const Ref<FileAccess> file = FileAccess::open_compressed(p_path, FileAccess::READ);
    ERR_FAIL_COND_V(file.is_null(), {});

    Error err = _open(file, false, true);
    if (err != OK) {
        return {};
    }

    PackedStringArray results;
    for (const auto& [path, offset] : _internal_resources) {
        file->seek(offset);

        const String class_name = _read_unicode_string();
        if (!file->get_error() && !class_name.is_empty() && ClassDB::class_exists(class_name)) {
            results.push_back(class_name);
        }
    }

    return results;
}

Variant OrchestrationBinaryParser::load(const String& p_path) {
    const Ref<FileAccess> file = FileAccess::open_compressed(p_path, FileAccess::READ);
    ERR_FAIL_COND_V(file.is_null(), {});

    _path = p_path;
    _cache_mode = ResourceFormatLoader::CACHE_MODE_REPLACE;

    Error err = _open(file);
    if (err != OK) {
        return {};
    }

    err = _load();
    if (err != OK) {
        return {};
    }

    Ref<Orchestration> orchestration = _resource;
    if (orchestration.is_valid()) {
        orchestration->_script_path = p_path;
        orchestration->post_initialize();
    }

    return _resource;
}
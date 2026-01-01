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
#include "orchestration/serialization/binary/binary_serializer.h"

#include "common/dictionary_utils.h"
#include "common/string_utils.h"
#include "orchestration/orchestration.h"
#include "orchestration/serialization/binary/binary_format.h"
#include "orchestration/serialization/binary/binary_parser.h"
#include "orchestration/serialization/format.h"
#include "script/script.h"
#include "script/script_server.h"
#include "script/serialization/format_defs.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/resource_saver.hpp>

uint32_t OrchestrationBinarySerializer::_get_string_index(const String& p_value) {
    const StringName sname = p_value;
    if (_string_map.has(sname)) {
        return _string_map[sname];
    }

    const uint32_t index = _strings.size();
    _string_map[sname] = index;
    _strings.push_back(p_value);

    return index;
}

void OrchestrationBinarySerializer::_save_unicode_string(const String& p_value, bool p_bit_on_length) {
    OrchestrationBinaryFormat::save_unicode_string(_file, p_value, p_bit_on_length);
}

void OrchestrationBinarySerializer::_write_variant(const Variant& p_value, HashMap<Ref<Resource>, uint32_t>& p_resource_map, const PropertyInfo& p_hint) { // NOLINT
    switch (p_value.get_type()) {
        case Variant::NIL: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_NIL);
            // Do not store anything for null values
            break;
        }
        case Variant::BOOL: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_BOOL);
            _file->store_32(p_value ? 1 : 0);
            break;
        }
        case Variant::INT: {
            int64_t val = p_value;
            if (val > 0x7FFFFFFF || val < -static_cast<int64_t>(0x80000000)) {
                _file->store_32(OrchestrationBinaryFormat::VARIANT_INT64);
                _file->store_64(val);
            } else {
                _file->store_32(OrchestrationBinaryFormat::VARIANT_INT);
                _file->store_32(p_value);
            }
            break;
        }
        case Variant::FLOAT: {
            double d = p_value;
            float fl = static_cast<float>(d);
            if (static_cast<double>(fl) != d) {
                _file->store_32(OrchestrationBinaryFormat::VARIANT_DOUBLE);
                _file->store_double(d);
            } else {
                _file->store_32(OrchestrationBinaryFormat::VARIANT_FLOAT);
                _file->store_float(fl);
            }
            break;
        }
        case Variant::STRING: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_STRING);
            _save_unicode_string(p_value);
            break;
        }
        case Variant::RECT2: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_RECT2);

            Rect2 val = p_value;
            _file->store_real(val.position.x);
            _file->store_real(val.position.y);
            _file->store_real(val.size.x);
            _file->store_real(val.size.y);
            break;
        }
        case Variant::RECT2I: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_RECT2I);

            Rect2i val = p_value;
            _file->store_32(val.position.x);
            _file->store_32(val.position.y);
            _file->store_32(val.size.x);
            _file->store_32(val.size.y);
            break;
        }
        case Variant::VECTOR2: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_VECTOR2);

            Vector2 val = p_value;
            _file->store_real(val.x);
            _file->store_real(val.y);
            break;
        }
        case Variant::VECTOR2I: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_VECTOR2I);

            Vector2i val = p_value;
            _file->store_32(val.x);
            _file->store_32(val.y);
            break;
        }
        case Variant::VECTOR3: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_VECTOR3);

            Vector3 val = p_value;
            _file->store_real(val.x);
            _file->store_real(val.y);
            _file->store_real(val.z);
            break;
        }
        case Variant::VECTOR3I: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_VECTOR3I);

            Vector3i val = p_value;
            _file->store_32(val.x);
            _file->store_32(val.y);
            _file->store_32(val.z);
            break;
        }
        case Variant::VECTOR4: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_VECTOR4);

            Vector4 val = p_value;
            _file->store_real(val.x);
            _file->store_real(val.y);
            _file->store_real(val.z);
            _file->store_real(val.w);
            break;
        }
        case Variant::VECTOR4I: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_VECTOR4I);
            
            Vector4i val = p_value;
            _file->store_32(val.x);
            _file->store_32(val.y);
            _file->store_32(val.z);
            _file->store_32(val.w);
            break;
        }
        case Variant::PLANE: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_PLANE);
            
            Plane val = p_value;
            _file->store_real(val.normal.x);
            _file->store_real(val.normal.y);
            _file->store_real(val.normal.z);
            _file->store_real(val.d);
            break;
        }
        case Variant::QUATERNION: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_QUATERNION);
            
            Quaternion val = p_value;
            _file->store_real(val.x);
            _file->store_real(val.y);
            _file->store_real(val.z);
            _file->store_real(val.w);
            break;
        }
        case Variant::AABB: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_AABB);
            
            AABB val = p_value;
            _file->store_real(val.position.x);
            _file->store_real(val.position.y);
            _file->store_real(val.position.z);
            _file->store_real(val.size.x);
            _file->store_real(val.size.y);
            _file->store_real(val.size.z);
            break;
        }
        case Variant::TRANSFORM2D: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_TRANSFORM2D);
            
            Transform2D val = p_value;
            _file->store_real(val.columns[0].x);
            _file->store_real(val.columns[0].y);
            _file->store_real(val.columns[1].x);
            _file->store_real(val.columns[1].y);
            _file->store_real(val.columns[2].x);
            _file->store_real(val.columns[2].y);
            break;
        }
        case Variant::BASIS: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_BASIS);
            
            Basis val = p_value;
            _file->store_real(val.rows[0].x);
            _file->store_real(val.rows[0].y);
            _file->store_real(val.rows[0].z);
            _file->store_real(val.rows[1].x);
            _file->store_real(val.rows[1].y);
            _file->store_real(val.rows[1].z);
            _file->store_real(val.rows[2].x);
            _file->store_real(val.rows[2].y);
            _file->store_real(val.rows[2].z);
            break;
        }
        case Variant::TRANSFORM3D: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_TRANSFORM3D);
            
            Transform3D val = p_value;
            _file->store_real(val.basis.rows[0].x);
            _file->store_real(val.basis.rows[0].y);
            _file->store_real(val.basis.rows[0].z);
            _file->store_real(val.basis.rows[1].x);
            _file->store_real(val.basis.rows[1].y);
            _file->store_real(val.basis.rows[1].z);
            _file->store_real(val.basis.rows[2].x);
            _file->store_real(val.basis.rows[2].y);
            _file->store_real(val.basis.rows[2].z);
            _file->store_real(val.origin.x);
            _file->store_real(val.origin.y);
            _file->store_real(val.origin.z);
            break;
        }
        case Variant::PROJECTION: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_PROJECTION);
            
            Projection val = p_value;
            _file->store_real(val.columns[0].x);
            _file->store_real(val.columns[0].y);
            _file->store_real(val.columns[0].z);
            _file->store_real(val.columns[0].w);
            _file->store_real(val.columns[1].x);
            _file->store_real(val.columns[1].y);
            _file->store_real(val.columns[1].z);
            _file->store_real(val.columns[1].w);
            _file->store_real(val.columns[2].x);
            _file->store_real(val.columns[2].y);
            _file->store_real(val.columns[2].z);
            _file->store_real(val.columns[2].w);
            _file->store_real(val.columns[3].x);
            _file->store_real(val.columns[3].y);
            _file->store_real(val.columns[3].z);
            _file->store_real(val.columns[3].w);
            break;
        }
        case Variant::COLOR: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_COLOR);
            
            Color val = p_value;
            // Color are always floats
            _file->store_float(val.r);
            _file->store_float(val.g);
            _file->store_float(val.b);
            _file->store_float(val.a);
            break;
        }
        case Variant::STRING_NAME: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_STRING_NAME);
            _save_unicode_string(p_value);
            break;
        }
        case Variant::NODE_PATH: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_NODE_PATH);

            NodePath np = p_value;
            _file->store_16(np.get_name_count());

            uint16_t snc = np.get_subname_count();
            if (np.is_absolute()) {
                snc |= 0x8000;
            }

            _file->store_16(snc);
            for (int i = 0; i < np.get_name_count(); i++) {
                if (_string_map.has(np.get_name(i))) {
                    _file->store_32(_string_map[np.get_name(i)]);
                } else {
                    _save_unicode_string(np.get_name(i), true);
                }
            }
            for (int i = 0; i < np.get_subname_count(); i++) {
                if (_string_map.has(np.get_subname(i))) {
                    _file->store_32(_string_map[np.get_subname(i)]);
                } else {
                    _save_unicode_string(np.get_subname(i), true);
                }
            }
            break;
        }
        case Variant::RID: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_RID);

            WARN_PRINT("Cannot save RIDs (resource identifiers)");
            RID val = p_value;
            _file->store_32(val.get_id());
            break;
        }
        case Variant::OBJECT: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_OBJECT);

            Ref<Resource> res = p_value;
            if (res.is_null() || res->get_meta("_skip_save_", false)) {
                // Object is empty
                _file->store_32(OrchestrationBinaryFormat::OBJECT_EMPTY);
                return;
            }

            if (!_is_resource_built_in(res)) {
                _file->store_32(OrchestrationBinaryFormat::OBJECT_EXTERNAL_RESOURCE_INDEX);
                _file->store_32(_external_resources[res]);
            } else {
                if (!p_resource_map.has(res)) {
                    _file->store_32(OrchestrationBinaryFormat::OBJECT_EMPTY);
                    ERR_FAIL_MSG("Resource was not pre-cached, most likely a circular resource problem.");
                }
                _file->store_32(OrchestrationBinaryFormat::OBJECT_INTERNAL_RESOURCE);
                _file->store_32(p_resource_map[res]);
            }
            break;
        }
        case Variant::CALLABLE: {
            // There is no way to serialize a callable, only type is written.
            _file->store_32(OrchestrationBinaryFormat::VARIANT_CALLABLE);
            break;
        }
        case Variant::SIGNAL: {
            // There is no way to serialize signals, only type is written.
            _file->store_32(OrchestrationBinaryFormat::VARIANT_SIGNAL);
            break;
        }
        case Variant::DICTIONARY: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_DICTIONARY);

            Dictionary d = p_value;
            _file->store_32(static_cast<uint32_t>(d.size()));

            Array keys = d.keys();
            for (int i = 0; i < keys.size(); i++) {
                _write_variant(keys[i], p_resource_map);
                _write_variant(d[keys[i]], p_resource_map);
            }
            break;
        }
        case Variant::ARRAY: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_ARRAY);

            Array array = p_value;
            _file->store_32(array.size());

            for (int i = 0; i < array.size(); i++) {
                _write_variant(array[i], p_resource_map);
            }
            break;
        }
        case Variant::PACKED_BYTE_ARRAY: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_PACKED_BYTE_ARRAY);

            PackedByteArray array = p_value;
            const uint32_t size = array.size();
            _file->store_32(size);

            const uint8_t* data = array.ptr();
            _file->store_buffer(data, size);

            // Pad buffer to 32 bytes
            const uint32_t extra = 4 - (size % 4); // NOLINT
            if (extra < 4) {
                for (uint32_t i = 0; i < extra; i++) {
                    _file->store_8(0);
                }
            }
            break;
        }
        case Variant::PACKED_INT32_ARRAY: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_PACKED_INT32_ARRAY);

            PackedInt32Array array = p_value;
            const uint32_t size = array.size();
            _file->store_32(size);

            for (uint32_t i = 0; i < size; i++) {
                _file->store_32(array[i]);
            }
            break;
        }
        case Variant::PACKED_INT64_ARRAY: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_PACKED_INT64_ARRAY);

            PackedInt64Array array = p_value;
            const uint32_t size = array.size();
            _file->store_32(size);

            for (uint32_t i = 0; i < size; i++) {
                _file->store_64(array[i]);
            }
            break;
        }
        case Variant::PACKED_FLOAT32_ARRAY: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_PACKED_FLOAT32_ARRAY);

            PackedFloat32Array array = p_value;
            const uint32_t size = array.size();
            _file->store_32(size);

            for (uint32_t i = 0; i < size; i++) {
                _file->store_float(array[i]);
            }
            break;
        }
        case Variant::PACKED_FLOAT64_ARRAY: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_PACKED_FLOAT64_ARRAY);

            PackedFloat64Array array = p_value;
            const uint32_t size = array.size();
            _file->store_32(size);

            for (uint32_t i = 0; i < size; i++) {
                _file->store_double(array[i]);
            }
            break;
        }
        case Variant::PACKED_STRING_ARRAY: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_PACKED_STRING_ARRAY);

            PackedStringArray array = p_value;
            const uint32_t size = array.size();
            _file->store_32(size);

            for (uint32_t i = 0; i < size; i++) {
                _save_unicode_string(array[i]);
            }
            break;
        }
        case Variant::PACKED_VECTOR2_ARRAY: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_PACKED_VECTOR2_ARRAY);

            PackedVector2Array array = p_value;
            const uint32_t size = array.size();
            _file->store_32(size);

            for (uint32_t i = 0; i < size; i++) {
                _file->store_double(array[i].x);
                _file->store_double(array[i].y);
            }
            break;
        }
        case Variant::PACKED_VECTOR3_ARRAY: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_PACKED_VECTOR3_ARRAY);

            PackedVector3Array array = p_value;
            const uint32_t size = array.size();
            _file->store_32(size);

            for (uint32_t i = 0; i < size; i++) {
                _file->store_double(array[i].x);
                _file->store_double(array[i].y);
                _file->store_double(array[i].z);
            }
            break;
        }
        case Variant::PACKED_COLOR_ARRAY: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_PACKED_COLOR_ARRAY);

            PackedColorArray array = p_value;
            const uint32_t size = array.size();
            _file->store_32(size);

            for (uint32_t i = 0; i < size; i++) {
                _file->store_float(array[i].r);
                _file->store_float(array[i].g);
                _file->store_float(array[i].b);
                _file->store_float(array[i].a);
            }
            break;
        }
        case Variant::PACKED_VECTOR4_ARRAY: {
            _file->store_32(OrchestrationBinaryFormat::VARIANT_PACKED_VECTOR4_ARRAY);

            PackedVector4Array array = p_value;
            const uint32_t size = array.size();
            _file->store_32(size);

            for (uint32_t i = 0; i < size; i++) {
                _file->store_double(array[i].x);
                _file->store_double(array[i].y);
                _file->store_double(array[i].z);
                _file->store_double(array[i].w);
            }
            break;
        }
        default: {
            ERR_FAIL_MSG(vformat("Unable to serialize property type %s with name %s", p_value.get_type(), p_hint.name));
        }    
    }
}

void OrchestrationBinarySerializer::_decode_and_set_flags(const String& p_path, uint32_t p_flags) {
    OrchestrationSerializer::_decode_and_set_flags(p_path, p_flags);
    _big_endian = p_flags & ResourceSaver::FLAG_SAVE_BIG_ENDIAN;
}

void OrchestrationBinarySerializer::_find_resources_node_path(const NodePath& p_node_path, bool p_main) {
    for (int i = 0; i < p_node_path.get_name_count(); i++) {
        _get_string_index(p_node_path.get_name(i));
    }
    for (int i = 0; i < p_node_path.get_subname_count(); i++) {
        _get_string_index(p_node_path.get_subname(i));
    }
}

void OrchestrationBinarySerializer::_find_resources_object(const Variant& p_variant, bool p_main) { // NOLINT
    const Ref<Resource> resource = p_variant;
    if (resource.is_null() || resource->get_meta("_skip_save_", false) || _external_resources.has(resource)) {
        return;
    }
    _find_resources_resource(resource, p_main);
}

void OrchestrationBinarySerializer::_find_resources_resource(const Ref<Resource>& p_resource, bool p_main) { // NOLINT
    if (!p_main && !_bundle_resources && !_is_resource_built_in(p_resource)) {
        if (p_resource->get_path() == _path) {
            ERR_PRINT(vformat("(Circular reference to resource being saved found: %s will be null next time its loaded.", _path));
            return;
        }

        // Use a numeric ID as base to sort in a natural order before saving.
        // This increase the chances of thread loading to fetch them first.
        const uint32_t index = _external_resources.size();
        _external_resources[p_resource] = index;
        return;
    }

    if (_resource_set.has(p_resource)) {
        return;
    }

    _find_resources_resource_properties(p_resource, p_main);
}

PackedStringArray OrchestrationBinarySerializer::get_recognized_extensions(const Ref<Resource>& p_resource) {
    return recognize(p_resource) ? Array::make(ORCHESTRATOR_SCRIPT_EXTENSION) : Array();
}

bool OrchestrationBinarySerializer::recognize(const Ref<Resource>& p_resource) {
    return p_resource.is_valid() && p_resource->get_class() == Orchestration::get_class_static();
}

Error OrchestrationBinarySerializer::set_uid(const String& p_path, int64_t p_uid) {
    Ref<FileAccess> file = FileAccess::open_compressed(p_path, FileAccess::READ);
    ERR_FAIL_COND_V_MSG(file.is_null(), ERR_CANT_OPEN, "Cannot open file " + p_path);

    Ref<FileAccess> fw = FileAccess::open_compressed(vformat("%s.uidren", p_path), FileAccess::WRITE);
    ERR_FAIL_COND_V_MSG(fw.is_null(), ERR_FILE_CANT_WRITE, "Cannot open file " + p_path + ".uidren");

    String local_path = p_path.get_base_dir();

    uint8_t header[4];
    file->get_buffer(header, sizeof(header));
    fw->store_buffer(header, sizeof(header));

    // Endianness
    uint32_t big_endian = file->get_32();
    file->set_big_endian(big_endian == 1);
    fw->store_32(big_endian);
    fw->set_big_endian(big_endian == 1);

    // Precision
    fw->store_32(file->get_32());

    // Serialize the file format version
    uint32_t version = file->get_32();
    if (version > OrchestrationFormat::FORMAT_VERSION) {
        ERR_FAIL_V_MSG(ERR_FILE_UNRECOGNIZED, "File cannot be loaded as it was saved with a newer version of OScript format.");
    }
    fw->store_32(version);

    // Serialize the version of Godot this file was saved with
    fw->store_32(file->get_32());
    fw->store_32(file->get_32());
    fw->store_32(file->get_32());

    // Store the resource type
    // The incoming resource is an Orchestration but for backward compatibility it should be saved
    // as an OScript object, so default to that here.
    OrchestrationBinaryFormat::save_unicode_string(fw, OrchestrationBinaryFormat::read_unicode_string(file));

    // Write flags
    // Binary format will always include UIDs
    String global_name;
    uint32_t flags = file->get_32();
    fw->store_32(flags);

    if (flags & OrchestrationBinaryFormat::FORMAT_FLAG_UIDS) {
        [[maybe_unused]] int64_t old_uid = file->get_64(); // Skip existing UID
        fw->store_32(p_uid);
    }

    if (flags & OrchestrationBinaryFormat::FORMAT_FLAG_HAS_SCRIPT_CLASS) {
        OrchestrationBinaryFormat::save_unicode_string(fw, OrchestrationBinaryFormat::read_unicode_string(file));
    }

    uint8_t b = file->get_8();
    while (!file->eof_reached()) {
        fw->store_8(b);
        b = file->get_8();
    }

    bool all_ok = fw->get_error() == OK;
    if (!all_ok) {
        return ERR_CANT_CREATE;
    }

    file.unref();
    fw.unref();

    Ref<DirAccess> dir = DirAccess::open("res://");
    dir->remove(p_path);
    dir->rename(vformat("%s.uidren", p_path), p_path);

    return OK;
}

bool OrchestrationBinarySerializer::recognize_path(const Ref<Resource>& p_resource, const String& p_path) {
    return p_path.get_extension().naturalnocasecmp_to(ORCHESTRATOR_SCRIPT_EXTENSION) == 0;
}

Error OrchestrationBinarySerializer::save(const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags) {
    const Ref<Orchestration> orchestration = p_resource;
    ERR_FAIL_COND_V_MSG(!orchestration.is_valid(), ERR_INVALID_PARAMETER, "Resource is not an orchestration");

    const Ref<FileAccess> file = FileAccess::open_compressed(p_path, FileAccess::WRITE);
    ERR_FAIL_COND_V_MSG(file.is_null(), ERR_FILE_CANT_WRITE, "Cannot write file '" + p_path + "'.");
    _file = file;

    _decode_and_set_flags(p_path, p_flags);

    // Walk the resource and gather all resource details
    _find_resources(p_resource, true);

    // Header magic
    static constexpr uint8_t header[4] = { 'G', 'D', 'O', 'S' };
    _file->store_buffer(header, sizeof(header));

    // Store whether file is in big/little endian
    if (_big_endian) {
        _file->store_32(1);
        _file->set_big_endian(true);
    } else {
        _file->store_32(0);
    }

    // Real uses float/double
    // For now this is set to 0 = false
    _file->store_32(0);

    // Serialize the file format version
    _file->store_32(OrchestrationFormat::FORMAT_VERSION);

    // Serialize the version of Godot this file was saved with
    _file->store_32(GODOT_VERSION_MAJOR);
    _file->store_32(GODOT_VERSION_MINOR);
    _file->store_32(GODOT_VERSION_PATCH);

    if (_file->get_error() != OK && _file->get_error() != ERR_FILE_EOF) {
        return ERR_CANT_CREATE;
    }

    // Store the resource type
    // The incoming resource is an Orchestration but for backward compatibility it should be saved
    // as an OScript object, so default to that here.
    _save_unicode_string(OScript::get_class_static());

    // Write flags
    // Binary format will always include UIDs
    uint32_t flags = OrchestrationBinaryFormat::FORMAT_FLAG_UIDS;

    String global_name = orchestration->get_global_name();
    if (!global_name.is_empty()) {
        flags |= OrchestrationBinaryFormat::FORMAT_FLAG_HAS_SCRIPT_CLASS;
    }

    _file->store_32(flags);

    if (flags & OrchestrationBinaryFormat::FORMAT_FLAG_UIDS) {
        int64_t uid = _get_resource_id_for_path(p_path, true);
        _file->store_64(uid);
    }
    if (!global_name.is_empty()) {
        _save_unicode_string(global_name);
    }

    // Leaving some buffer here for future expansion if we need additional values.
    for (uint32_t i = 0; i < OrchestrationBinaryFormat::RESERVED_FIELDS; i++) {
        _file->store_32(0);
    }

    const Dictionary missing_resource_properties = p_resource->get_meta("_missing_resources", Dictionary());

    List<ResourceInfo> res_infos;
    for (const Ref<Resource>& E : _saved_resources) {
        auto& [type, properties] = res_infos.push_back(ResourceInfo())->get();
        type = _resource_get_class(E);

        const TypedArray<Dictionary> property_list = E->get_property_list();
        for (uint32_t i = 0; i < property_list.size(); i++) {
            const PropertyInfo pi = DictionaryUtils::to_property(property_list[i]);
            if (_skip_editor && pi.name.begins_with("__editor")) {
                continue;
            }

            if (pi.name.match("metadata/_missing_resources")) {
                continue;
            }

            if (pi.usage & PROPERTY_USAGE_STORAGE) {
                Property property;
                property.index = _get_string_index(pi.name);

                if (pi.usage & PROPERTY_USAGE_RESOURCE_NOT_PERSISTENT) {
                    NonPersistentKey key;
                    key.base = E;
                    key.property = pi.name;

                    if (_non_persistent_map.has(key)) {
                        property.value = _non_persistent_map[key];
                    }
                } else {
                    property.value = E->get(pi.name);
                }

                if (property.info.type == Variant::OBJECT && missing_resource_properties.has(pi.name)) {
                    const Ref<Resource> res = property.value;
                    if (res.is_null()) {
                        property.value = missing_resource_properties[pi.name];
                    }
                }

                Variant default_value = _class_get_property_default_value(E->get_class(), pi.name);
                if (default_value.get_type() != Variant::NIL) {
                    bool valid = false;
                    Variant result = false;
                    Variant::evaluate(Variant::OP_EQUAL, property.value, default_value, result, valid);
                    if (valid && result) {
                        continue;
                    }
                }

                property.info = pi;
                properties.push_back(property);
            }
        }
    }

    // Serialize the string table
    // The string table maintains a collection of all unique names shared across classes
    _file->store_32(_strings.size());
    for (const StringName& E : _strings) {
        _save_unicode_string(E);
    }

    // Sort all external resources
    Vector<Ref<Resource>> external_resource_save_order;
    external_resource_save_order.resize(_external_resources.size());
    for (const KeyValue<Ref<Resource>, uint32_t>& E : _external_resources) {
        external_resource_save_order.write[E.value] = E.key;
    }

    // Serialize any external resources
    String base_dir = p_path.get_base_dir();
    _file->store_32(_external_resources.size());
    for (const Ref<Resource>& E : external_resource_save_order) {
        _save_unicode_string(E->get_class());

        String path = E->get_path();
        path = _relative_paths ? StringUtils::path_to_file(base_dir, path) : path;
        _save_unicode_string(path);

        int64_t uid = _get_resource_id_for_path(E->get_path(), false);
        _file->store_64(uid);
    }

    // Iterate all internal resources and collect used unique scene ids.
    // For resources that have collisions, first one visited wins; others reassigned.
    HashSet<String> used_unique_ids;
    #if GODOT_VERSION >= 0x040300
    for (const Ref<Resource>& E : _saved_resources) {
        if (_is_resource_built_in(E)) {
            if (!E->get_scene_unique_id().is_empty()) {
                if (used_unique_ids.has(E->get_scene_unique_id())) {
                    E->set_scene_unique_id("");
                } else {
                    used_unique_ids.insert(E->get_scene_unique_id());
                }
            }
        }
    }
    #endif

    // Store the number of internal resources
    _file->store_32(_saved_resources.size());

    // Serialize internal resource names and/or paths with offset placeholders
    uint32_t resource_index = 0;
    HashMap<Ref<Resource>, uint32_t> resource_map;
    Vector<uint64_t> offsets;
    for (const Ref<Resource>& E: _saved_resources) {
        #if GODOT_VERSION >= 0x040300
        if (_is_resource_built_in(E)) {
            bool generated;
            String uid_text = _create_resource_uid(E, used_unique_ids, generated);
            if (generated) {
                E->set_scene_unique_id(uid_text);
                used_unique_ids.insert(uid_text);
            }
            _save_unicode_string("local://" + itos(resource_index));
            if (_take_over_paths) {
                E->set_path(vformat("%s::%s", p_path, E->get_scene_unique_id()));
            }
            _set_resource_edited(E, false);
        } else {
            _save_unicode_string(E->get_path());
        }
        #else
        // All internal resources are written as "local://[index]"
        // This allows renaming and moving of files without impacting the data
        // When the file is loaded the "local://" prefix is replaced with the resource path
        _save_unique_string("local://" + itos(resource_index));
        #endif

        // Temporarily store an empty offset "0", and track it in the map
        // These will be serialized later
        offsets.push_back(_file->get_position());
        _file->store_64(0);
        resource_map[E] = resource_index++;
    }

    // Serialize ResourceInfo type with property index and values
    // Records the offset for each resource
    Vector<uint64_t> offset_table;
    for (const auto& [type, properties] : res_infos) {
        offset_table.push_back(_file->get_position());
        _save_unicode_string(type);

        _file->store_32(properties.size());
        for (const auto& [index, info, value] : properties) {
            _file->store_32(index);
            _write_variant(value, resource_map, info);
        }
    }

    // Now take the offset table created from the ResourceInfo blocks and serialize
    // them into the placeholders created two steps prior.
    for (int32_t i = 0; i < offset_table.size(); i++) {
        _file->seek(offsets[i]);
        _file->store_64(offset_table[i]);
    }

    // Return to end of the file to write end magic term.
    _file->seek_end();
    _file->store_buffer(reinterpret_cast<const uint8_t*>("GDOS"), 4);

    if (_file->get_error() != OK && _file->get_error() != ERR_FILE_EOF) {
        return ERR_CANT_CREATE;
    }

    #ifdef TOOLS_ENABLED
    if (orchestration.is_valid()) {
        orchestration->set_edited(false);
    }
    #endif

    return OK;
}
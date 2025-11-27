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
#include "orchestration/io/orchestration_serializer_binary.h"

#include "common/dictionary_utils.h"
#include "common/string_utils.h"
#include "common/version.h"
#include "editor/plugins/orchestrator_editor_plugin.h"
#include "orchestration/io/orchestration_format.h"
#include "orchestration_format_binary.h"
#include "script/script.h"
#include "script/script_server.h"

#include <godot_cpp/classes/missing_resource.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>

int OrchestrationBinarySerializer::_get_string_index(const String& p_value)
{
    int64_t index = _string_map.find(p_value);
    if (index != -1)
        return index;

    _string_map.append(p_value);
    return _string_map.size() - 1;
}

void OrchestrationBinarySerializer::_write_variant(OrchestrationByteStream& p_stream, const Variant& p_value, HashMap<Ref<Resource>, int>& p_resource_map, HashMap<Ref<Resource>, int>& p_ext_resources)
{
    switch (p_value.get_type())
    {
        case Variant::NIL:
        {
            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_NIL);
            break;
        }
        case Variant::BOOL:
        {
            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_BOOL);
            p_stream.write_u32(p_value.operator bool());
            break;
        }
        case Variant::INT:
        {
            const int64_t value = p_value;
            if (value > 0x7FFFFFFF || value < -(int64_t) 0x80000000)
            {
                p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_INT64);
                p_stream.write_u64(value);
            }
            else
            {
                p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_INT);
                p_stream.write_u32(p_value);
            }
            break;
        }
        case Variant::FLOAT:
        {
            const double value = p_value;
            const float fvalue = value;

            if (static_cast<double>(fvalue) != value)
            {
                p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_DOUBLE);
                p_stream.write_double(value);
            }
            else
            {
                p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_FLOAT);
                p_stream.write_float(fvalue);
            }
            break;
        }
        case Variant::STRING:
        {
            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_STRING);
            p_stream.write_unicode_string(p_value);
            break;
        }
        case Variant::RECT2I:
        {
            const Rect2i value = p_value;

            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_RECT2I);
            p_stream.write_u32(value.position.x);
            p_stream.write_u32(value.position.y);
            p_stream.write_u32(value.size.x);
            p_stream.write_u32(value.size.y);
            break;
        }
        case Variant::VECTOR2:
        {
            const Vector2 value = p_value;

            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_VECTOR2);
            p_stream.write_real(value.x);
            p_stream.write_real(value.y);
            break;
        }
        case Variant::VECTOR2I:
        {
            const Vector2i value = p_value;

            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_VECTOR2I);
            p_stream.write_u32(value.x);
            p_stream.write_u32(value.y);
            break;
        }
        case Variant::VECTOR3:
        {
            const Vector3 value = p_value;

            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_VECTOR3);
            p_stream.write_real(value.x);
            p_stream.write_real(value.y);
            p_stream.write_real(value.z);
            break;
        }
        case Variant::VECTOR3I:
        {
            const Vector3i value = p_value;

            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_VECTOR3I);
            p_stream.write_u32(value.x);
            p_stream.write_u32(value.y);
            p_stream.write_u32(value.z);
            break;
        }
        case Variant::VECTOR4:
        {
            const Vector4 value = p_value;

            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_VECTOR4);
            p_stream.write_real(value.x);
            p_stream.write_real(value.y);
            p_stream.write_real(value.z);
            p_stream.write_real(value.w);
            break;
        }
        case Variant::VECTOR4I:
        {
            const Vector4i value = p_value;

            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_VECTOR4I);
            p_stream.write_u32(value.x);
            p_stream.write_u32(value.y);
            p_stream.write_u32(value.z);
            p_stream.write_u32(value.w);
            break;
        }
        case Variant::PLANE:
        {
            const Plane value = p_value;

            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_PLANE);
            p_stream.write_real(value.normal.x);
            p_stream.write_real(value.normal.y);
            p_stream.write_real(value.normal.z);
            p_stream.write_real(value.d);
            break;
        }
        case Variant::QUATERNION:
        {
            const Quaternion value = p_value;

            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_QUATERNION);
            p_stream.write_real(value.x);
            p_stream.write_real(value.y);
            p_stream.write_real(value.z);
            p_stream.write_real(value.w);
            break;
        }
        case Variant::AABB:
        {
            const AABB value = p_value;

            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_AABB);
            p_stream.write_real(value.position.x);
            p_stream.write_real(value.position.y);
            p_stream.write_real(value.position.z);
            p_stream.write_real(value.size.x);
            p_stream.write_real(value.size.y);
            p_stream.write_real(value.size.z);
            break;
        }
        case Variant::TRANSFORM2D:
        {
            const Transform2D value = p_value;

            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_TRANSFORM2D);
            p_stream.write_real(value.columns[0].x);
            p_stream.write_real(value.columns[0].y);
            p_stream.write_real(value.columns[1].x);
            p_stream.write_real(value.columns[1].y);
            p_stream.write_real(value.columns[2].x);
            p_stream.write_real(value.columns[2].y);
            break;
        }
        case Variant::BASIS:
        {
            const Basis value = p_value;

            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_BASIS);
            p_stream.write_real(value.rows[0].x);
            p_stream.write_real(value.rows[0].y);
            p_stream.write_real(value.rows[0].z);
            p_stream.write_real(value.rows[1].x);
            p_stream.write_real(value.rows[1].y);
            p_stream.write_real(value.rows[1].z);
            p_stream.write_real(value.rows[2].x);
            p_stream.write_real(value.rows[2].y);
            p_stream.write_real(value.rows[2].z);
            break;
        }
        case Variant::TRANSFORM3D:
        {
            const Transform3D value = p_value;

            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_TRANSFORM3D);
            p_stream.write_real(value.basis.rows[0].x);
            p_stream.write_real(value.basis.rows[0].y);
            p_stream.write_real(value.basis.rows[0].z);
            p_stream.write_real(value.basis.rows[1].x);
            p_stream.write_real(value.basis.rows[1].y);
            p_stream.write_real(value.basis.rows[1].z);
            p_stream.write_real(value.basis.rows[2].x);
            p_stream.write_real(value.basis.rows[2].y);
            p_stream.write_real(value.basis.rows[2].z);
            p_stream.write_real(value.origin.x);
            p_stream.write_real(value.origin.y);
            p_stream.write_real(value.origin.z);
            break;
        }
        case Variant::PROJECTION:
        {
            const Projection value = p_value;

            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_PROJECTION);
            p_stream.write_real(value.columns[0].x);
            p_stream.write_real(value.columns[0].y);
            p_stream.write_real(value.columns[0].z);
            p_stream.write_real(value.columns[0].w);
            p_stream.write_real(value.columns[1].x);
            p_stream.write_real(value.columns[1].y);
            p_stream.write_real(value.columns[1].z);
            p_stream.write_real(value.columns[1].w);
            p_stream.write_real(value.columns[2].x);
            p_stream.write_real(value.columns[2].y);
            p_stream.write_real(value.columns[2].z);
            p_stream.write_real(value.columns[2].w);
            p_stream.write_real(value.columns[3].x);
            p_stream.write_real(value.columns[3].y);
            p_stream.write_real(value.columns[3].z);
            p_stream.write_real(value.columns[3].w);
            break;
        }
        case Variant::COLOR:
        {
            const Color value = p_value;

            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_COLOR);
            p_stream.write_float(value.r);
            p_stream.write_float(value.g);
            p_stream.write_float(value.b);
            p_stream.write_float(value.a);
            break;
        }
        case Variant::STRING_NAME:
        {
            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_STRING_NAME);
            p_stream.write_unicode_string(p_value);
            break;
        }
        case Variant::NODE_PATH:
        {
            const NodePath np = p_value;

            uint16_t snc = np.get_subname_count();
            if (np.is_absolute())
                snc |= 0x8000;

            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_NODE_PATH);
            p_stream.write_u16(np.get_name_count());
            p_stream.write_u16(snc);

            for (int i = 0; i < np.get_name_count(); i++)
            {
                const int64_t index = _string_map.find(np.get_name(i));
                if (index == -1)
                    p_stream.write_unicode_string(np.get_name(i), true);
                else
                    p_stream.write_u32(index);
            }

            for (int i = 0; i < np.get_subname_count(); i++)
            {
                const int64_t index = _string_map.find(np.get_subname(i));
                if (index == -1)
                    p_stream.write_unicode_string(np.get_subname(i), true);
                else
                    p_stream.write_u32(index);
            }
            break;
        }
        case Variant::RID:
        {
            WARN_PRINT("Cannot save RIDs (resource identifiers)");

            const RID value = p_value;

            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_RID);
            p_stream.write_u32(value.get_id());
            break;
        }
        case Variant::OBJECT:
        {
            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_OBJECT);

            const Ref<Resource> res = p_value;
            if (res.is_null() || res->get_meta("_skip_save_", false))
            {
                // Object is empty
                p_stream.write_u32(OrchestrationBinaryFormat::OBJECT_EMPTY);
                return;
            }

            if (!_is_built_in_resource(res))
            {
                p_stream.write_u32(OrchestrationBinaryFormat::OBJECT_EXTERNAL_RESOURCE_INDEX);
                p_stream.write_u32(p_ext_resources[res]);
            }
            else
            {
                if (!p_resource_map.has(res))
                {
                    p_stream.write_u32(OrchestrationBinaryFormat::OBJECT_EMPTY);
                    ERR_FAIL_MSG("Resource was not pre-cached, most likely a circular resource problem.");
                }

                p_stream.write_u32(OrchestrationBinaryFormat::OBJECT_INTERNAL_RESOURCE);
                p_stream.write_u32(p_resource_map[res]);
            }
            break;
        }
        case Variant::CALLABLE:
        {
            // There is no way to serialize a callable, only type is written.
            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_CALLABLE);
            break;
        }
        case Variant::SIGNAL:
        {
            // There is no way to serialize signals, only type is written.
            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_SIGNAL);
            break;
        }
        case Variant::DICTIONARY:
        {
            const Dictionary value = p_value;

            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_DICTIONARY);
            p_stream.write_u32(static_cast<uint32_t>(value.size()));

            Array keys = value.keys();
            for (int i = 0; i < keys.size(); i++)
            {
                _write_variant(p_stream, keys[i], p_resource_map, p_ext_resources);
                _write_variant(p_stream, value[keys[i]], p_resource_map, p_ext_resources);
            }
            break;
        }
        case Variant::ARRAY:
        {
            const Array array = p_value;

            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_ARRAY);
            p_stream.write_u32(array.size());

            for (int i = 0; i < array.size(); i++)
                _write_variant(p_stream, array[i], p_resource_map, p_ext_resources);

            break;
        }
        case Variant::PACKED_BYTE_ARRAY:
        {
            const PackedByteArray array = p_value;
            const uint8_t*data = array.ptr();
            const int size = static_cast<int>(array.size());

            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_PACKED_BYTE_ARRAY);
            p_stream.write_u32(size);
            p_stream.write_buffer(data, size);

            // pad to 32-bytes
            const int extra = 4 - (size % 4);
            if (extra < 4 )
            {
                for (int i = 0; i < extra; i++)
                    p_stream.write_u8(0);
            }

            break;
        }
        case Variant::PACKED_INT32_ARRAY:
        {
            const PackedInt32Array array = p_value;
            const int size = array.size();

            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_PACKED_INT32_ARRAY);
            p_stream.write_u32(size);

            for (int i = 0; i < size; i++)
                p_stream.write_u32(array[i]);

            break;
        }
        case Variant::PACKED_INT64_ARRAY:
        {
            const PackedInt64Array array = p_value;
            const int size = array.size();

            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_PACKED_INT64_ARRAY);
            p_stream.write_u32(size);

            for (int i = 0; i < size; i++)
                p_stream.write_u64(array[i]);

            break;
        }
        case Variant::PACKED_FLOAT32_ARRAY:
        {
            const PackedFloat32Array array = p_value;
            const int size = array.size();

            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_PACKED_FLOAT32_ARRAY);
            p_stream.write_u32(size);

            for (int i = 0; i < size; i++)
                p_stream.write_float(array[i]);

            break;
        }
        case Variant::PACKED_FLOAT64_ARRAY:
        {
            const PackedFloat64Array array = p_value;
            const int size = array.size();

            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_PACKED_FLOAT64_ARRAY);
            p_stream.write_u32(size);

            for (int i = 0; i < size; i++)
                p_stream.write_double(array[i]);

            break;
        }
        case Variant::PACKED_STRING_ARRAY:
        {
            const PackedStringArray array = p_value;
            const int size = array.size();

            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_PACKED_STRING_ARRAY);
            p_stream.write_u32(size);

            for (int i = 0; i < size; i++)
                p_stream.write_unicode_string(array[i]);

            break;
        }
        case Variant::PACKED_VECTOR2_ARRAY:
        {
            const PackedVector2Array array = p_value;
            const int size = array.size();

            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_PACKED_VECTOR2_ARRAY);
            p_stream.write_u32(size);

            for (int i = 0; i < size; i++)
            {
                p_stream.write_double(array[i].x);
                p_stream.write_double(array[i].y);
            }

            break;
        }
        case Variant::PACKED_VECTOR3_ARRAY:
        {
            const PackedVector3Array array = p_value;
            const int size = array.size();

            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_PACKED_VECTOR3_ARRAY);
            p_stream.write_u32(size);

            for (int i = 0; i < size; i++)
            {
                p_stream.write_double(array[i].x);
                p_stream.write_double(array[i].y);
                p_stream.write_double(array[i].z);
            }

            break;
        }
        case Variant::PACKED_COLOR_ARRAY:
        {
            const PackedColorArray array = p_value;
            const int size = array.size();

            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_PACKED_COLOR_ARRAY);
            p_stream.write_u32(size);

            for (int i = 0; i < size; i++)
            {
                p_stream.write_float(array[i].r);
                p_stream.write_float(array[i].g);
                p_stream.write_float(array[i].b);
                p_stream.write_float(array[i].a);
            }

            break;
        }
        case Variant::PACKED_VECTOR4_ARRAY:
        {
            const PackedVector4Array array = p_value;
            const int size = array.size();

            p_stream.write_u32(OrchestrationBinaryFormat::VARIANT_PACKED_VECTOR4_ARRAY);
            p_stream.write_u32(size);

            for (int i = 0; i < size; i++)
            {
                p_stream.write_double(array[i].x);
                p_stream.write_double(array[i].y);
                p_stream.write_double(array[i].z);
                p_stream.write_double(array[i].w);
            }
            break;
        }
        default:
        {
            ERR_FAIL_MSG(vformat("Unable to serialize property type %s", p_value.get_type()));
        }
    }
}

bool OrchestrationBinarySerializer::_is_resource_gatherable(const Ref<Resource>& p_resource, bool p_main)
{
    if (p_resource.is_null() || _ext_resources.has(p_resource) || p_resource->get_meta(StringName("_skip_save_"), false))
        return false;

    if (!p_main && !_bundle_resources && !_is_built_in_resource(p_resource))
    {
        if (p_resource->get_path() == _path)
        {
            ERR_PRINT(vformat(
                "Circular references to resource being saved found: '%s' will be null next time its loaded.",
                _local_path));
            return false;
        }

        int index = _ext_resources.size();
        _ext_resources[p_resource] = index;
        return false;
    }

    return !_resource_set.has(p_resource);
}

void OrchestrationBinarySerializer::_gather_node_path(const NodePath& p_path, bool p_main)
{
    for (int64_t i = 0; i < p_path.get_name_count(); i++)
        _get_string_index(p_path.get_name(i));

    for (int64_t i = 0; i < p_path.get_subname_count(); i++)
        _get_string_index(p_path.get_subname(i));
}

Variant OrchestrationBinarySerializer::serialize(const Ref<Orchestration>& p_orchestration, const String& p_path, uint32_t p_flags)
{
    _relative_paths = p_flags & ResourceSaver::FLAG_RELATIVE_PATHS;
    _skip_editor = p_flags & ResourceSaver::FLAG_OMIT_EDITOR_PROPERTIES;
    _bundle_resources = p_flags & ResourceSaver::FLAG_BUNDLE_RESOURCES;
    _take_over_paths = p_flags & ResourceSaver::FLAG_REPLACE_SUBRESOURCE_PATHS;

    if (!p_path.begins_with("res://"))
        _take_over_paths = false;

    _local_path = p_path.get_base_dir();
    _path = ProjectSettings::get_singleton()->localize_path(p_path);

    _gather_resources(p_orchestration, true);

    OrchestrationByteStream stream;

    static const uint8_t header[4] = { 'G', 'D', 'O', 'S' };
    stream.write_buffer(header, 4);

    const bool big_endian = p_flags & ResourceSaver::FLAG_SAVE_BIG_ENDIAN;
    stream.write_u32(big_endian ? 1 : 0);
    stream.set_big_endian(big_endian);

    const bool use_real64 = false;
    stream.write_u32(use_real64 ? 1 : 0);

    stream.write_u32(OrchestrationFormat::FORMAT_VERSION);

    stream.write_u32(GODOT_VERSION_MAJOR);
    stream.write_u32(GODOT_VERSION_MINOR);
    stream.write_u32(GODOT_VERSION_PATCH);

    stream.write_unicode_string(p_orchestration->get_class());

    // Always force the use of UIDs
    uint32_t format_flags = OrchestrationBinaryFormat::FORMAT_FLAG_UIDS;

    String script_class;
    const Ref<Script> script = p_orchestration->get_script();
    if (script.is_valid())
    {
        script_class = ScriptServer::get_global_name(script);
        if (!script_class.is_empty())
            format_flags |= OrchestrationBinaryFormat::FORMAT_FLAG_HAS_SCRIPT_CLASS;
    }

    stream.write_u32(format_flags);

    if (format_flags & OrchestrationBinaryFormat::FORMAT_FLAG_UIDS)
    {
        int64_t uid = _get_resource_id_for_path(p_path, true);
        stream.write_u64(uid);
    }

    if (format_flags & OrchestrationBinaryFormat::FORMAT_FLAG_HAS_SCRIPT_CLASS)
        stream.write_unicode_string(script_class);

    for (uint32_t i = 0; i < OrchestrationBinaryFormat::NUM_RESERVED_FIELDS; i++)
        stream.write_u32(0);

    List<ResourceInfo> resources;
    Dictionary missing_properties = p_orchestration->get_meta("_missing_resources", Dictionary());
    for (const Ref<Resource>& E : _saved_resources)
    {
        ResourceInfo& ri = resources.push_back(ResourceInfo())->get();
        ri.type = _get_resource_class(E);

        TypedArray<Dictionary> properties = E->get_property_list();
        for (int64_t i = 0; i < properties.size(); i++)
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
                    property.value = E->get(F.name);

                if (property.info.type == Variant::OBJECT && missing_properties.has(F.name))
                {
                    const Ref<Resource> res = property.value;
                    if (res.is_null())
                        property.value = missing_properties[F.name];
                }

                Variant default_value = _get_class_property_default(E->get_class(), F.name);
                if (default_value.get_type() != Variant::NIL)
                {
                    bool valid;
                    Variant result;
                    Variant::evaluate(Variant::OP_EQUAL, property.value, default_value, result, valid);
                    if (valid && result.operator bool())
                        continue;
                }

                property.info = F;
                ri.properties.push_back(property);
            }
        }
    }

    // String table
    stream.write_u32(_string_map.size());
    for (uint32_t i = 0; i < _string_map.size(); i++)
        stream.write_unicode_string(_string_map[i]);

    Vector<Ref<Resource>> save_order;
    save_order.resize(_ext_resources.size());

    // Store external resources
    stream.write_u32(_ext_resources.size());
    for (const KeyValue<Ref<Resource>, int>& E : _ext_resources)
        save_order.write[E.value] = E.key;

    for (int64_t i = 0; i < save_order.size(); i++)
    {
        stream.write_unicode_string(save_order[i]->get_class());

        String path = save_order[i]->get_path();
        path = _relative_paths ? StringUtils::path_to_file(_local_path, path) : path;

        stream.write_unicode_string(path);

        int64_t uid = _get_resource_id_for_path(save_order[i]->get_path(), false);
        stream.write_u64(uid);
    }

    stream.write_u32(_saved_resources.size());

    HashSet<String> used_unique_ids;

    #if GODOT_VERSION >= 0x040300
    for (const Ref<Resource>& resource : _saved_resources)
    {
        if (_is_built_in_resource(resource))
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

    int index = 0;
    HashMap<Ref<Resource>, int> resource_map;
    Vector<uint64_t> offsets;

    for (const Ref<Resource>& resource : _saved_resources)
    {
        #if GODOT_VERSION >= 0x040300
        if (_is_built_in_resource(resource))
        {
            if (resource->get_scene_unique_id().is_empty())
            {
                String new_id;
                while(true)
                {
                    new_id = _get_resource_class(resource) + "_" + Resource::generate_scene_unique_id();
                    if (!used_unique_ids.has(new_id))
                        break;
                }
                resource->set_scene_unique_id(new_id);
                used_unique_ids.insert(new_id);
            }

            stream.write_unicode_string("local://" + itos(index));
            if (_take_over_paths)
                resource->set_path(vformat("%s::%s", p_path, resource->get_scene_unique_id()));

            _set_resource_edited(resource, false);
        }
        else
            stream.write_unicode_string(resource->get_path());

        #else
        // All internal resources are written as "local://[index]"
        // This allows renaming and moving of files without impacting the data.
        //
        // When the file is loaded, the "local://" prefix is replaced with the resource path,
        // and "::" to handle uniquness within the Editor.
        stream.write_unicode_string("local://" + itos(index));
        #endif

        // Save position reference and write placeholder, populating offset table later
        offsets.push_back(stream.tell());
        stream.write_u64(0);
        resource_map[resource] = index++;
    }

    Vector<uint64_t> offset_table;
    for (const ResourceInfo& ri : resources)
    {
        offset_table.push_back(stream.tell());
        stream.write_unicode_string(ri.type);

        stream.write_u32(ri.properties.size());
        for (const Property& property : ri.properties)
        {
            stream.write_u32(property.name_index);
            _write_variant(stream, property.value, resource_map, _ext_resources);
        }
    }

    for (int i = 0; i < offset_table.size(); i++)
    {
        stream.seek(offsets[i]);
        stream.write_u64(offset_table[i]);
    }

    stream.seek(stream.size());
    stream.write_buffer(reinterpret_cast<const uint8_t*>("GDOS"), 4);

    return stream.get_as_bytes();
}

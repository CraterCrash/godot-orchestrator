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
#include "format_loader_instance.h"

#include "common/logger.h"
#include "common/string_utils.h"
#include "script/script.h"

#include <godot_cpp/classes/missing_resource.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/time.hpp>

bool OScriptResourceLoaderInstance::is_cached(const String& p_path)
{
    return ResourceLoader::get_singleton()->has_cached(p_path);
}

String OScriptResourceLoaderInstance::_read_unicode_string()
{
    int len = f->get_32();

    if (len == 0)
        return String();

    if (len > string_buffer.size())
        string_buffer.resize(len);

    f->get_buffer((uint8_t*)&string_buffer[0], len);

    String result;
    result.parse_utf8(&string_buffer[0]);
    return result;
}

String OScriptResourceLoaderInstance::_read_string()
{
    uint32_t id = f->get_32();
    if (id & 0x80000000)
    {
        uint32_t len = id & 0x7FFFFFFF;
        if ((int)len > string_buffer.size())
            string_buffer.resize(len);
        if (len == 0)
            return StringName();

        f->get_buffer((uint8_t*)&string_buffer[0], len);

        String result;
        result.parse_utf8(&string_buffer[0]);
        return result;
    }
    return string_map[id];
}

Error OScriptResourceLoaderInstance::_parse_variant(Variant& r_val)
{
    uint32_t variant_type = f->get_32();
    switch (variant_type)
    {
        case VARIANT_NIL:
        {
            r_val = Variant();
            break;
        }
        case VARIANT_BOOL:
        {
            r_val = bool(f->get_32());
            break;
        }
        case VARIANT_INT:
        {
            r_val = int(f->get_32());
            break;
        }
        case VARIANT_INT64:
        {
            r_val = int64_t(f->get_64());
            break;
        }
        case VARIANT_FLOAT:
        {
            r_val = f->get_real();
            break;
        }
        case VARIANT_DOUBLE:
        {
            r_val = f->get_double();
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
            v.position.x = f->get_real();
            v.position.y = f->get_real();
            v.size.x = f->get_real();
            v.size.y = f->get_real();
            r_val = v;
            break;
        }
        case VARIANT_RECT2I:
        {
            Rect2i v;
            v.position.x = f->get_32();
            v.position.y = f->get_32();
            v.size.x = f->get_32();
            v.size.y = f->get_32();
            r_val = v;
            break;
        }
        case VARIANT_VECTOR2:
        {
            Vector2 v;
            v.x = f->get_real();
            v.y = f->get_real();
            r_val = v;
            break;
        }
        case VARIANT_VECTOR2I:
        {
            Vector2i v;
            v.x = f->get_32();
            v.y = f->get_32();
            r_val = v;
            break;
        }
        case VARIANT_VECTOR3:
        {
            Vector3 v;
            v.x = f->get_real();
            v.y = f->get_real();
            v.z = f->get_real();
            r_val = v;
            break;
        }
        case VARIANT_VECTOR3I:
        {
            Vector3i v;
            v.x = f->get_32();
            v.y = f->get_32();
            v.z = f->get_32();
            r_val = v;
            break;
        }
        case VARIANT_VECTOR4:
        {
            Vector4 v;
            v.x = f->get_real();
            v.y = f->get_real();
            v.z = f->get_real();
            v.w = f->get_real();
            r_val = v;
            break;
        }
        case VARIANT_VECTOR4I:
        {
            Vector4i v;
            v.x = f->get_32();
            v.y = f->get_32();
            v.z = f->get_32();
            v.w = f->get_32();
            r_val = v;
            break;
        }
        case VARIANT_PLANE:
        {
            Plane v;
            v.normal.x = f->get_real();
            v.normal.y = f->get_real();
            v.normal.z = f->get_real();
            v.d = f->get_real();
            r_val = v;
            break;
        }
        case VARIANT_QUATERNION:
        {
            Quaternion v;
            v.x = f->get_real();
            v.y = f->get_real();
            v.z = f->get_real();
            v.w = f->get_real();
            r_val = v;
            break;
        }
        case VARIANT_AABB:
        {
            AABB v;
            v.position.x = f->get_real();
            v.position.y = f->get_real();
            v.position.z = f->get_real();
            v.size.x = f->get_real();
            v.size.y = f->get_real();
            v.size.z = f->get_real();
            r_val = v;
            break;
        }
        case VARIANT_TRANSFORM2D:
        {
            Transform2D v;
            v.columns[0].x = f->get_real();
            v.columns[0].y = f->get_real();
            v.columns[1].x = f->get_real();
            v.columns[1].y = f->get_real();
            v.columns[2].x = f->get_real();
            v.columns[2].y = f->get_real();
            r_val = v;
            break;
        }
        case VARIANT_BASIS:
        {
            Basis v;
            v.rows[0].x = f->get_real();
            v.rows[0].y = f->get_real();
            v.rows[0].z = f->get_real();
            v.rows[1].x = f->get_real();
            v.rows[1].y = f->get_real();
            v.rows[1].z = f->get_real();
            v.rows[2].x = f->get_real();
            v.rows[2].y = f->get_real();
            v.rows[2].z = f->get_real();
            r_val = v;
            break;
        }
        case VARIANT_TRANSFORM3D:
        {
            Transform3D v;
            v.basis.rows[0].x = f->get_real();
            v.basis.rows[0].y = f->get_real();
            v.basis.rows[0].z = f->get_real();
            v.basis.rows[1].x = f->get_real();
            v.basis.rows[1].y = f->get_real();
            v.basis.rows[1].z = f->get_real();
            v.basis.rows[2].x = f->get_real();
            v.basis.rows[2].y = f->get_real();
            v.basis.rows[2].z = f->get_real();
            v.origin.x = f->get_real();
            v.origin.y = f->get_real();
            v.origin.z = f->get_real();
            r_val = v;
            break;
        }
        case VARIANT_PROJECTION:
        {
            Projection v;
            v.columns[0].x = f->get_real();
            v.columns[0].y = f->get_real();
            v.columns[0].z = f->get_real();
            v.columns[0].w = f->get_real();
            v.columns[1].x = f->get_real();
            v.columns[1].y = f->get_real();
            v.columns[1].z = f->get_real();
            v.columns[1].w = f->get_real();
            v.columns[2].x = f->get_real();
            v.columns[2].y = f->get_real();
            v.columns[2].z = f->get_real();
            v.columns[2].w = f->get_real();
            v.columns[3].x = f->get_real();
            v.columns[3].y = f->get_real();
            v.columns[3].z = f->get_real();
            v.columns[3].w = f->get_real();
            r_val = v;
            break;
        }
        case VARIANT_COLOR:
        {
            // Colors should always be in single-precision.
            Color v;
            v.r = f->get_float();
            v.g = f->get_float();
            v.b = f->get_float();
            v.a = f->get_float();
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

            int name_count = f->get_16();
            int subname_count = f->get_16();
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
            r_val = f->get_32();
            break;
        }
        case VARIANT_OBJECT:
        {
            uint32_t obj_type = f->get_32();
            switch (obj_type)
            {
                case OBJECT_EMPTY:
                {
                    // nothing else to do
                    break;
                }
                case OBJECT_INTERNAL_RESOURCE:
                {
                    uint32_t index = f->get_32();

                    String path = res_path + "::" + itos(index);
                    if (!internal_index_cache.has(path))
                    {
                        PackedStringArray known_names;
                        for (const KeyValue<String, Ref<Resource>>& E : internal_index_cache)
                            known_names.push_back(E.key);

                        WARN_PRINT("Couldn't load resource (no cache): " + path
                                   + "; known: " + StringUtils::join(",", known_names));
                        r_val = Variant();
                    }
                    else
                    {
                        r_val = internal_index_cache[path];
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
            r_val = Callable();
            break;
        }
        case VARIANT_SIGNAL:
        {
            r_val = Signal();
            break;
        }
        case VARIANT_DICTIONARY:
        {
            uint32_t size = f->get_32();
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
            uint32_t size = f->get_32();
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
        default:
            ERR_FAIL_V(ERR_FILE_CORRUPT);
    }

    return OK;
}

Error OScriptResourceLoaderInstance::load(const Ref<FileAccess>& p_file)
{
    f = p_file;

    Logger::debug("Loading OrchestratorScript ", local_path);
    Logger::debug("\tFile Size : ", f->get_length(), " bytes");

    const uint64_t file_time = FileAccess::get_modified_time(f->get_path());
    Logger::debug("\tFile Time : ", Time::get_singleton()->get_datetime_string_from_unix_time(file_time));

    uint8_t header[4];
    f->get_buffer(header, 4);
    if (header[0] != 'G' || header[1] != 'D' || header[2] != 'O' || header[3] != 'S')
    {
        f.unref();
        ERR_FAIL_V_MSG(ERR_FILE_UNRECOGNIZED, "Unrecognized resource file: '" + local_path + "'");
    }

    bool big_endian;
    [[maybe_unused]] bool use_real64;

    big_endian = f->get_32();
    use_real64 = f->get_32();

    // Read big endian if saved as big endian format
    f->set_big_endian(big_endian != 0);

    version = f->get_32();
    Logger::debug("\tFormat    : Version ", version);

    uint32_t major = f->get_32();
    uint32_t minor = f->get_32();
    uint32_t patch = f->get_32();
    godot_version = major * 1000000 + minor * 1000 + patch;
    Logger::debug("\tGodot Ver : ", major, ".", minor, ".", patch, " (", godot_version, ")");

    // Read the resource type
    type = _read_unicode_string();

    // Skip over the reserved fields section
    for (int i = 0; i < RESERVED_FIELDS; i++)
        f->get_32();

    // Read the string table
    uint32_t string_table_size = f->get_32();
    string_map.resize(string_table_size);
    for (uint32_t i = 0; i < string_table_size; i++)
    {
        StringName sn = _read_unicode_string();
        string_map.write[i] = sn;
    }

    // Add loading external resources metadata here if needed

    // Load internal resource metadata
    uint32_t internal_resources_size = f->get_32();
    for (uint32_t i = 0; i < internal_resources_size; i++)
    {
        InternalResource ir;
        ir.path = _read_unicode_string();
        ir.offset = f->get_64();
        internal_resources.push_back(ir);
    }

    if (f->eof_reached())
    {
        f.unref();
        ERR_FAIL_V_MSG(ERR_FILE_CORRUPT, "Premature end of the file (EOF): '" + local_path + "'");
    }

    // Add loading external resources if desired here.

    for (int i = 0; i < internal_resources.size(); i++)
    {
        bool main = i == (internal_resources.size() - 1);

        String path;
        String id;

        if (!main)
        {
            path = internal_resources[i].path;

            if (path.begins_with("local://"))
            {
                path = StringUtils::replace_first(path, "local://", "");
                id = path;
                path = res_path + "::" + path;
                internal_resources.write[i].path = path;  // update path
            }
        }
        else
        {
            if (cache_mode == ResourceFormatLoader::CACHE_MODE_IGNORE && !is_cached(res_path))
                path = res_path;
        }

        // Jump to the internal resource offset
        uint64_t offset = internal_resources[i].offset;
        f->seek(offset);

        // Read the internal resource type name
        String t = _read_unicode_string();

        Ref<Resource> res;

        MissingResource* missing_resource = nullptr;
        if (res.is_null())
        {
            // Resource not cached
            Variant v = ClassDB::instantiate(t);
            Object* obj = v;
            if (!obj)
            {
                ERR_FAIL_V_MSG(ERR_FILE_CORRUPT, local_path + ": Resource of unrecognized type in file: " + t + ".");
            }

            Resource* r = Object::cast_to<Resource>(obj);
            if (!r)
            {
                String obj_class = obj->get_class();
                memdelete(obj);
                ERR_FAIL_V_MSG(ERR_FILE_CORRUPT, local_path + ": Resource type in resource field not a resource, type is: "
                                                     + obj_class + ".");
            }

            res = Ref<Resource>(r);
            if (!path.is_empty() && cache_mode != ResourceFormatLoader::CACHE_MODE_IGNORE)
            {
                // If we got here it's because the resource with the same path with different types.
                // In this case, we would want to ideally replace it.
                r->set_path(path);
            }
        }

        if (!main)
            internal_index_cache[path] = res;

        // Read properties
        Dictionary missing_resource_properties;
        int num_properties = f->get_32();
        for (int j = 0; j < num_properties; ++j)
        {
            StringName name = _read_string();
            if (name == StringName())
            {
                ERR_FAIL_V(ERR_FILE_CORRUPT);
            }

            Variant value;

            Error result = _parse_variant(value);
            if (result != OK)
                return result;

            bool set_valid = true;
            if (value.get_type() == Variant::OBJECT && missing_resource != nullptr)
            {
                // If the property being set is missing a resource (and the parent is not),
                // then setting it will likely not work, save it as metadata instead.
                Ref<MissingResource> mr = value;
                if (mr.is_valid())
                {
                    missing_resource_properties[name] = mr;
                    set_valid = false;
                }
            }

            if (value.get_type() == Variant::ARRAY)
            {
                Array set_array = value;
                Variant get_value = res->get(name);
                if (get_value.get_type() == Variant::ARRAY)
                {
                    Array get_array = get_value;
                    if (!set_array.is_same_typed(get_array))
                    {
                        value = Array(set_array, get_array.get_typed_builtin(), get_array.get_typed_class_name(),
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

        resource_cache.push_back(res);

        if (main)
        {
            f.unref();
            resource = res;
            resource->set_message_translation(translation_remapped);

            Ref<OScript> script = resource;
            if (script.is_valid())
            {
                script->post_initialize();

                // When user enables debugging, we dump some metadata about the script here.
                // There will also be some script metadata printed earlier on about the file name,
                // the format and version used by the script file, etc.
                if (Logger::get_level() >= Logger::DEBUG)
                {
                    const Vector<Ref<OScriptGraph>> graphs = script->get_graphs();
                    const Vector<Ref<OScriptVariable>> variables = script->get_variables();
                    const PackedStringArray function_names = script->get_function_names();
                    const PackedStringArray custom_signal_names = script->get_custom_signal_names();
                    const Vector<Ref<OScriptNode>> nodes = script->get_nodes();
                    const RBSet<OScriptConnection>& connections = script->get_connections();

                    Logger::debug("\tBase Type : ", script->get_base_type());
                    Logger::debug("\tGraphs    : ", graphs.size());
                    Logger::debug("\tVariables : ", variables.size());
                    Logger::debug("\tFunctions : ", function_names.size());
                    Logger::debug("\tSignals   : ", custom_signal_names.size());
                    Logger::debug("\tNodes     : ", nodes.size());
                    Logger::debug("\tWires     : ", connections.size());
                }
            }
            return OK;
        }
    }

    return ERR_FILE_EOF;
}

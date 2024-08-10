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
#ifndef ORCHESTRATOR_SCRIPT_SERIALIZATION_INSTANCE_H
#define ORCHESTRATOR_SCRIPT_SERIALIZATION_INSTANCE_H

#include "godot_cpp/classes/ref.hpp"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/resource.hpp>

using namespace godot;

/// Base class for all Orchestrator resource format instances
class OScriptResourceFormatInstance
{
protected:
    /// Get the resource unique ID for a given resource path
    /// @param p_path the resource path
    /// @param p_generate whether to generate a UID if none found
    /// @return the unique ID or <code>ResourceUID::INVALID_ID</code> if not found and not generated
    int64_t _get_resource_id_for_path(const String& p_path, bool p_generate = false);

    /// Checks if the resource is considered built-in
    /// @param p_resource the resource
    /// @return <code>true</code> if the resource is built-in, <code>false</code> otherwise
    bool _is_resource_built_in(const Ref<Resource>& p_resource) const;

public:
    static uint32_t FORMAT_VERSION;
    static uint32_t RESERVED_FIELDS;

    enum
    {
        // numbering must be different from variant, in case new variant types are added
        // (variant must be always contiguous for jump table optimization)
        VARIANT_NIL = 1,
        VARIANT_BOOL = 2,
        VARIANT_INT = 3,
        VARIANT_FLOAT = 4,
        VARIANT_STRING = 5,
        VARIANT_VECTOR2 = 10,
        VARIANT_RECT2 = 11,
        VARIANT_VECTOR3 = 12,
        VARIANT_PLANE = 13,
        VARIANT_QUATERNION = 14,
        VARIANT_AABB = 15,
        VARIANT_BASIS = 16,
        VARIANT_TRANSFORM3D = 17,
        VARIANT_TRANSFORM2D = 18,
        VARIANT_COLOR = 20,
        VARIANT_NODE_PATH = 22,
        VARIANT_RID = 23,
        VARIANT_OBJECT = 24,
        VARIANT_INPUT_EVENT = 25,
        VARIANT_DICTIONARY = 26,
        VARIANT_ARRAY = 30,
        VARIANT_PACKED_BYTE_ARRAY = 31,
        VARIANT_PACKED_INT32_ARRAY = 32,
        VARIANT_PACKED_FLOAT32_ARRAY = 33,
        VARIANT_PACKED_STRING_ARRAY = 34,
        VARIANT_PACKED_VECTOR3_ARRAY = 35,
        VARIANT_PACKED_COLOR_ARRAY = 36,
        VARIANT_PACKED_VECTOR2_ARRAY = 37,
        VARIANT_INT64 = 40,
        VARIANT_DOUBLE = 41,
        VARIANT_CALLABLE = 42,
        VARIANT_SIGNAL = 43,
        VARIANT_STRING_NAME = 44,
        VARIANT_VECTOR2I = 45,
        VARIANT_RECT2I = 46,
        VARIANT_VECTOR3I = 47,
        VARIANT_PACKED_INT64_ARRAY = 48,
        VARIANT_PACKED_FLOAT64_ARRAY = 49,
        VARIANT_VECTOR4 = 50,
        VARIANT_VECTOR4I = 51,
        VARIANT_PROJECTION = 52,

        // Other static values
        OBJECT_EMPTY = 0,
        OBJECT_EXTERNAL_RESOURCE = 1,
        OBJECT_INTERNAL_RESOURCE = 2,
        OBJECT_EXTERNAL_RESOURCE_INDEX = 3,
    };

    virtual ~OScriptResourceFormatInstance() = default;
};

/// A common class for binary-based resource format instances
class OScriptResourceBinaryFormatInstance : public OScriptResourceFormatInstance
{
protected:
    enum
    {
        // FORMAT_FLAG_NAMED_SCENE_IDS = 1, - Should not be applicable
        FORMAT_FLAG_UIDS = 2,
        // FORMAT_FLAG_REAL_T_IS_DOUBLE = 4, - Not yet possible, FileAccess does not expose this
        FORMAT_FLAG_HAS_SCRIPT_CLASS = 8,
    };

    /// Reads a unicode string from the given file
    /// @param p_file the file reference
    /// @return the unicode string
    String _read_unicode_string(const Ref<FileAccess>& p_file);

    /// Save the specified string in the given file in unicode format.
    /// @param p_file the file reference
    /// @param p_value the string to be stored
    /// @param p_bit_on_length ??
    void _save_unicode_string(const Ref<FileAccess>& p_file, const String& p_value, bool p_bit_on_length = false);
};

/// A common class for text-based resource format instances
class OScriptResourceTextFormatInstance : public OScriptResourceFormatInstance
{
protected:
    String _create_start_tag(const String& p_resource_class, const String& p_script_class, uint32_t p_load_steps, uint32_t p_version, int64_t p_uid);
    String _create_ext_resource_tag(const String& p_type, const String& p_path, const String& p_id, bool p_newline = true);
};

#endif  // ORCHESTRATOR_SCRIPT_SERIALIZATION_INSTANCE_H
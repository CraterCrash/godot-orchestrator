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

#include <godot_cpp/classes/object.hpp>

class OScriptResourceFormatInstance
{
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
        VARIANT_PACKED_VECTOR4_ARRAY = 53,

        // Other static values
        OBJECT_EMPTY = 0,
        OBJECT_EXTERNAL_RESOURCE = 1,
        OBJECT_INTERNAL_RESOURCE = 2,
        OBJECT_EXTERNAL_RESOURCE_INDEX = 3,
    };
};

#endif  // ORCHESTRATOR_SCRIPT_SERIALIZATION_INSTANCE_H
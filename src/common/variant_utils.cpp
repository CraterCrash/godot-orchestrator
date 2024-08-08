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
#include "variant_utils.h"

#include <godot_cpp/variant/utility_functions.hpp>

namespace VariantUtils
{
    String get_type_name_article(Variant::Type p_type, bool p_nil_as_any)
    {
        switch (p_type)
        {
            case Variant::INT:
            case Variant::ARRAY:
            case Variant::OBJECT:
            case Variant::AABB:
                return "an";
            case Variant::NIL:
                return p_nil_as_any ? "an" : "a";
            default:
                return "a";
        }
    }

    String get_friendly_type_name(Variant::Type p_type, bool p_nil_as_any)
    {
        switch (p_type)
        {
            case Variant::INT:
                return "Integer";
            case Variant::BOOL:
                return "Boolean";
            case Variant::RECT2:
                return "Rect2";
            case Variant::RECT2I:
                return "Rect2i";
            case Variant::VECTOR2:
                return "Vector2";
            case Variant::VECTOR2I:
                return "Vector2i";
            case Variant::VECTOR3:
                return "Vector3";
            case Variant::VECTOR3I:
                return "Vector3i";
            case Variant::VECTOR4:
                return "Vector4";
            case Variant::VECTOR4I:
                return "Vector4i";
            case Variant::TRANSFORM2D:
                return "Transform2D";
            case Variant::TRANSFORM3D:
                return "Transform3D";
            case Variant::STRING_NAME:
                return "String Name";
            case Variant::NODE_PATH:
                return "NodePath";
            case Variant::PACKED_BYTE_ARRAY:
                return "Packed Byte Array";
            case Variant::PACKED_INT32_ARRAY:
                return "Packed Int32 Array";
            case Variant::PACKED_INT64_ARRAY:
                return "Packed Int64 Array";
            case Variant::PACKED_FLOAT32_ARRAY:
                return "Packed Float32 Array";
            case Variant::PACKED_FLOAT64_ARRAY:
                return "Packed Float64 Array";
            case Variant::PACKED_STRING_ARRAY:
                return "Packed String Array";
            case Variant::PACKED_VECTOR2_ARRAY:
                return "Packed Vector2 Array";
            case Variant::PACKED_VECTOR3_ARRAY:
                return "Packed Vector3 Array";
            case Variant::PACKED_COLOR_ARRAY:
                return "Packed Color Array";
            default:
            {
                String name = Variant::get_type_name(p_type);
                return p_nil_as_any && name.match("Nil") ? "Any" : name;
            }
        }
    }

    String to_enum_list(bool p_include_any)
    {
        String list = p_include_any ? "Any" : "";
        for (int i = 1; i < Variant::VARIANT_MAX; i++)
        {
            if (!list.is_empty())
                list += ",";

            list += Variant::get_type_name(Variant::Type(i));
        }
        return list;
    }

    Variant::Type to_type(int p_type)
    {
        return Variant::Type(int(p_type));
    }

    Variant make_default(Variant::Type p_type)
    {
        // Explicitly avoid "<null>" strings
        if (p_type == Variant::STRING)
            return String("");

        return UtilityFunctions::type_convert(Variant(), p_type);
    }

    Variant convert(const Variant& p_value, Variant::Type p_target_type)
    {
        if (Variant::can_convert(p_value.get_type(), p_target_type))
            return UtilityFunctions::type_convert(p_value, p_target_type);

        const Variant::Type type = p_value.get_type();

        if (p_target_type == Variant::BOOL)
        {
            if (type == Variant::VECTOR2 || type == Variant::VECTOR2I)
                return p_value.operator Vector2().x;

            if (type == Variant::VECTOR3 || type == Variant::VECTOR3I)
                return p_value.operator Vector3().x;

            if (type == Variant::VECTOR4 || type == Variant::VECTOR4I)
                return p_value.operator Vector4().x;

            if (type == Variant::INT || type == Variant::FLOAT)
                return p_value.operator int64_t();

            if (type == Variant::STRING || type == Variant::STRING_NAME)
            {
                const String value = p_value;
                return value.to_lower().match("true") || value.strip_edges() == "1";
            }
        }

        if (p_target_type == Variant::INT || p_target_type == Variant::FLOAT)
        {
            if (type == Variant::VECTOR2 || type == Variant::VECTOR2I)
                return p_value.operator Vector2().x;

            if (type == Variant::VECTOR3 || type == Variant::VECTOR3I)
                return p_value.operator Vector3().x;

            if (type == Variant::VECTOR4 || type == Variant::VECTOR4I)
                return p_value.operator Vector4().x;

            if (type == Variant::STRING || type == Variant::STRING_NAME)
            {
                String value = p_value;
                if (value.begins_with("(") && value.ends_with(")"))
                {
                    value = value.substr(1, value.length() - 1);
                    convert(value.split(",")[0], p_target_type);
                }
                else if (value.to_lower().match("true"))
                    return convert(true, p_target_type);
                else if (value.strip_edges().match("1"))
                    return convert(true, p_target_type);
            }
        }

        if (p_target_type == Variant::VECTOR2 || p_target_type == Variant::VECTOR2I)
        {
            if (type == Variant::BOOL || type == Variant::INT || type == Variant::FLOAT)
                return Vector2(p_value, p_value);

            if (type == Variant::STRING || type == Variant::STRING_NAME)
            {
                String value = p_value;
                if (value.begins_with("(") && value.ends_with(")"))
                {
                    value = value.substr(1, value.length() - 1);

                    Vector2 result;
                    const PackedStringArray parts = value.split(",");
                    for (int i = 0; i < parts.size() && i < 2; i++)
                        result[i] = parts[i].to_float();
                    return result;
                }
                else if (value.to_lower().match("true"))
                    return convert(true, p_target_type);
                else if (value.strip_edges().match("1"))
                    return convert(true, p_target_type);
            }

            if (type == Variant::VECTOR3 || type == Variant::VECTOR3I)
            {
                const Vector3 v3 = p_value;
                return Vector2(v3.x, v3.y);
            }

            if (type == Variant::VECTOR4 || type == Variant::VECTOR4I)
            {
                const Vector4 v4 = p_value;
                return Vector2(v4.x, v4.y);
            }
        }

        if (p_target_type == Variant::VECTOR3 || p_target_type == Variant::VECTOR3I)
        {
            if (type == Variant::INT || type == Variant::FLOAT || type == Variant::BOOL)
                return Vector3(p_value, p_value, p_value);

            if (type == Variant::STRING || type == Variant::STRING_NAME)
            {
                String value = p_value;
                if (value.begins_with("(") && value.ends_with(")"))
                {
                    value = value.substr(1, value.length() - 1);

                    Vector3 result;
                    const PackedStringArray parts = value.split(",");
                    for (int i = 0; i < parts.size() && i < 3; i++)
                        result[i] = parts[i].to_float();
                    return result;
                }
                else if (value.to_lower().match("true"))
                    return convert(true, p_target_type);
                else if (value.strip_edges().match("1"))
                    return convert(true, p_target_type);
            }

            if (type == Variant::VECTOR2 || type == Variant::VECTOR2I)
            {
                const Vector2 v2 = p_value;
                return Vector3(v2.x, v2.y, 0);
            }

            if (type == Variant::VECTOR4 || type == Variant::VECTOR4I)
            {
                const Vector4 v4 = p_value;
                return Vector3(v4.x, v4.y, v4.z);
            }
        }

        if (p_target_type == Variant::VECTOR4 || p_target_type == Variant::VECTOR4I)
        {
            if (type == Variant::INT || type == Variant::FLOAT || type == Variant::BOOL)
                return Vector4(p_value, p_value, p_value, p_value);

            if (type == Variant::STRING || type == Variant::STRING_NAME)
            {
                String value = p_value;
                if (value.begins_with("(") && value.ends_with(")"))
                {
                    value = value.substr(1, value.length() - 1);

                    Vector4 result;
                    const PackedStringArray parts = value.split(",");
                    for (int i = 0; i < parts.size() && i < 4; i++)
                        result[i] = parts[i].to_float();
                    return result;
                }
                else if (value.to_lower().match("true"))
                    return convert(true, p_target_type);
                else if (value.strip_edges().match("1"))
                    return convert(true, p_target_type);
            }

            if (type == Variant::VECTOR2 || type == Variant::VECTOR2I)
            {
                const Vector2 v2 = p_value;
                return Vector4(v2.x, v2.y, 0, 0);
            }

            if (type == Variant::VECTOR3 || type == Variant::VECTOR3I)
            {
                const Vector3 v3 = p_value;
                return Vector4(v3.x, v3.y, v3.z, 0);
            }
        }

        if (p_target_type == Variant::STRING_NAME)
            return StringName(convert(p_value, Variant::STRING));

        if (p_value.get_type() == Variant::STRING_NAME)
            return convert(String(p_value), p_target_type);

        return make_default(p_target_type);
    }

    bool evaluate(Variant::Operator p_operator, const Variant& p_left, const Variant& p_right, Variant& r_value)
    {
        bool valid = true;
        Variant::evaluate(p_operator, p_left, p_right, r_value, valid);
        return valid;
    }
}
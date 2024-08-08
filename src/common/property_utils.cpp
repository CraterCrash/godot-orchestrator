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
#include "common/property_utils.h"

#include "godot_cpp/templates/hash_map.hpp"
#include "string_utils.h"

namespace PropertyUtils
{
    static HashMap<uint32_t, String> get_property_usage_name_map()
    {
        HashMap<uint32_t, String> names;
        names[PROPERTY_USAGE_NONE] = "None";
        names[PROPERTY_USAGE_STORAGE] = "Storage";
        names[PROPERTY_USAGE_EDITOR] = "Editor";
        names[PROPERTY_USAGE_CLASS_IS_BITFIELD] ="ClassIsBitfield";
        names[PROPERTY_USAGE_CLASS_IS_ENUM] = "ClassIsEnum";
        names[PROPERTY_USAGE_NIL_IS_VARIANT] = "NilIsVariant";
        names[PROPERTY_USAGE_DEFAULT] = "Default";
        return names;
    }

    bool are_equal(const PropertyInfo& p_left, const PropertyInfo& p_right)
    {
        return p_left.type == p_right.type &&
            p_left.hint == p_right.hint &&
            p_left.hint_string == p_right.hint_string &&
            p_left.usage == p_right.usage &&
            p_left.class_name == p_right.class_name;
    }

    PropertyInfo as(const String& p_name, const PropertyInfo& p_property)
    {
        PropertyInfo new_property = p_property;
        new_property.name = p_name;
        return new_property;
    }

    PropertyInfo make_exec(const String& p_name)
    {
        return make_typed(p_name, Variant::NIL);
    }

    PropertyInfo make_variant(const String& p_name)
    {
        return { Variant::NIL, p_name, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT };
    }

    PropertyInfo make_object(const String& p_name, const String& p_class_name)
    {
        return { Variant::OBJECT, p_name, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT, p_class_name };
    }

    PropertyInfo make_file(const String& p_name, const String& p_filters)
    {
        return { Variant::STRING, p_name, PROPERTY_HINT_FILE, p_filters, PROPERTY_USAGE_DEFAULT };
    }

    PropertyInfo make_typed(const String& p_name, Variant::Type p_type, bool p_variant_on_nil)
    {
        if (p_variant_on_nil && p_type == Variant::NIL)
            return make_variant(p_name);

        return { p_type, p_name, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT };
    }

    PropertyInfo make_multiline(const String& p_name)
    {
        return { Variant::STRING, p_name, PROPERTY_HINT_MULTILINE_TEXT, "", PROPERTY_USAGE_DEFAULT };
    }

    PropertyInfo make_enum_class(const String& p_name, const String& p_class_name)
    {
        return { Variant::INT, p_name, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_ENUM, p_class_name };
    }

    PropertyInfo make_class_enum(const String& p_name, const String& p_class_name, const String& p_enum_name)
    {
        return make_enum_class(p_name, vformat("%s.%s", p_class_name, p_enum_name));
    }

    bool is_nil_no_variant(const PropertyInfo& p_property)
    {
        return is_nil(p_property) && !(p_property.usage & PROPERTY_USAGE_NIL_IS_VARIANT);
    }

    bool is_passed_by_reference(const PropertyInfo& p_property)
    {
        switch (p_property.type)
        {
            // These are all types that are explicitly passed by reference, always
            case Variant::OBJECT:
            case Variant::PACKED_INT32_ARRAY:
            case Variant::PACKED_INT64_ARRAY:
            case Variant::PACKED_BYTE_ARRAY:
            case Variant::PACKED_COLOR_ARRAY:
            case Variant::PACKED_FLOAT32_ARRAY:
            case Variant::PACKED_FLOAT64_ARRAY:
            case Variant::PACKED_STRING_ARRAY:
            case Variant::PACKED_VECTOR2_ARRAY:
            case Variant::PACKED_VECTOR3_ARRAY:
            #if GODOT_VERSION >= 0x040300
            case Variant::PACKED_VECTOR4_ARRAY:
            #endif
            case Variant::ARRAY:
            case Variant::DICTIONARY:
                return true;

            // Handle cases where pass by value/reference varies
            default:
                // Variant is always passed by reference
                return is_variant(p_property);
        }
    }

    String get_property_type_name(const PropertyInfo& p_property)
    {
        if (is_variant(p_property))
            return "Variant";

        if (is_enum(p_property) || is_bitfield(p_property))
            return "Enum";

        if (is_class(p_property))
            return p_property.class_name;

        return Variant::get_type_name(p_property.type);
    }

    String get_variant_type_name(const PropertyInfo& p_property)
    {
        if (is_variant(p_property))
            return "Variant";

        if (p_property.type == Variant::OBJECT)
            return "MiniObject";

        return Variant::get_type_name(p_property.type);
    }

    String usage_to_string(uint32_t p_usage)
    {
        static HashMap<uint32_t, String> usage_names = get_property_usage_name_map();

        PackedStringArray values;
        for (const KeyValue<uint32_t, String>& E : usage_names)
            if (E.key & p_usage)
                values.push_back(E.value);

        return StringUtils::join(", ", values);
    }
}
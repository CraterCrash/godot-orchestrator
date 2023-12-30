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
            case Variant::STRING_NAME:
                return "String Name";
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
}
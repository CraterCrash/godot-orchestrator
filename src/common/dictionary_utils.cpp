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
#include "dictionary_utils.h"

#include "memory_utils.h"

#include <godot_cpp/variant/utility_functions.hpp>

namespace DictionaryUtils
{
    bool is_property_equal(const PropertyInfo& p_left, const PropertyInfo& p_right)
    {
        return p_left.name == p_right.name &&
                p_left.type == p_right.type &&
                p_left.hint == p_right.hint &&
                p_left.class_name == p_right.class_name &&
                p_left.hint_string == p_right.hint_string &&
                p_left.usage == p_right.usage;
    }

    PropertyInfo to_property(const Dictionary& p_dict)
    {
        PropertyInfo pi;
        pi.hint = PROPERTY_HINT_NONE;
        pi.usage = PROPERTY_USAGE_DEFAULT;

        if (p_dict.has("name"))
            pi.name = p_dict["name"];
        if (p_dict.has("type"))
            pi.type = Variant::Type(int(p_dict["type"]));
        if (p_dict.has("class_name"))
            pi.class_name = p_dict["class_name"];
        if (p_dict.has("hint"))
            pi.hint = p_dict["hint"];
        if (p_dict.has("hint_string"))
            pi.hint_string = p_dict["hint_string"];

        // Fixes godot-cpp bug where PropertyInfo usage was being serialized incorrectly.
        if (p_dict.has("usage"))
        {
            uint32_t usage = p_dict["usage"];
            if (usage != 7)
                pi.usage = usage;
        }

        return pi;
    }

    Dictionary from_property(const PropertyInfo& p_property, bool p_use_minimal)
    {
        Dictionary dict;

        if (!p_use_minimal || !p_property.name.is_empty())
            dict["name"] = p_property.name;

        if (!p_use_minimal || p_property.type != Variant::NIL)
            dict["type"] = p_property.type;

        if (!p_use_minimal || !p_property.class_name.is_empty())
            dict["class_name"] = p_property.class_name;

        if (!p_use_minimal || p_property.hint != PROPERTY_HINT_NONE)
            dict["hint"] = p_property.hint;

        if (!p_use_minimal || !p_property.hint_string.is_empty())
            dict["hint_string"] = p_property.hint_string;

        // Fixes godot-cpp bug where PropertyInfo usage was being serialized incorrectly.
        uint32_t usage = p_property.usage;
        if (usage == 7)
            usage = PROPERTY_USAGE_DEFAULT;

        if (!p_use_minimal || usage != PROPERTY_USAGE_DEFAULT)
            dict["usage"] = usage;

        return dict;
    }

    GDExtensionPropertyInfo to_extension_property(const Dictionary& p_dict)
    {
        PropertyInfo pi = to_property(p_dict);

        GDExtensionPropertyInfo gpi;
        gpi.class_name = MemoryUtils::memnew_stringname(pi.class_name);
        gpi.name = MemoryUtils::memnew_stringname(pi.name);
        gpi.type = GDExtensionVariantType(pi.type);
        gpi.hint = pi.hint;
        gpi.hint_string = MemoryUtils::memnew_string(pi.hint_string);
        gpi.usage = pi.usage;
        return gpi;
    }

    MethodInfo to_method(const Dictionary& p_dict)
    {
        MethodInfo mi;
        if (p_dict.has("name"))
            mi.name = p_dict["name"];
        if (p_dict.has("return"))
            mi.return_val = to_property(p_dict["return"]);
        if (p_dict.has("flags"))
            mi.flags = p_dict["flags"];

        if (p_dict.has("args"))
        {
            Array array = p_dict["args"];
            for (int i = 0; i < array.size(); i++)
                mi.arguments.push_back(to_property(array[i]));
        }

        if (p_dict.has("default_args"))
        {
            Array array = p_dict["default_args"];
            for (int i = 0; i < array.size(); i++)
                mi.default_arguments.push_back(array[i]);
        }

        return mi;
    }

    Dictionary from_method(const MethodInfo& p_method, bool p_use_minimal)
    {
        PropertyInfo empty_property;
        empty_property.usage = PROPERTY_USAGE_DEFAULT;
        empty_property.hint = PROPERTY_HINT_NONE;

        Dictionary dict;
        dict["name"] = p_method.name;

        if (!p_use_minimal || !is_property_equal(p_method.return_val, empty_property))
            dict["return"] = from_property(p_method.return_val, p_use_minimal);

        if (!p_use_minimal || p_method.flags != METHOD_FLAGS_DEFAULT)
            dict["flags"] = p_method.flags;

        if (!p_use_minimal || !p_method.default_arguments.empty())
        {
            Array default_args;
            for (const Variant& default_argument : p_method.default_arguments)
                default_args.push_back(default_argument);
            dict["default_args"] = default_args;
        }

        if (!p_use_minimal || !p_method.arguments.empty())
        {
            Array args;
            for (const PropertyInfo& argument : p_method.arguments)
                args.push_back(from_property(argument, p_use_minimal));
            dict["args"] = args;
        }

        return dict;
    }

    Dictionary of(std::initializer_list<std::pair<Variant, Variant>>&& p_values)
    {
        Dictionary result;
        for (auto&& element : p_values)
        {
            auto&& [key, val] = element;
            result[key] = val;
        }
        return result;
    }

    struct PropertyInfoNameSort
    {
        _FORCE_INLINE_ bool operator()(const PropertyInfo& a, const PropertyInfo& b) const
        {
            return String(a.name) < String(b.name);
        }
    };

    List<PropertyInfo> to_properties(const TypedArray<Dictionary>& p_array, bool p_sorted)
    {
        List<PropertyInfo> properties;

        uint64_t array_size = p_array.size();
        for (uint64_t i = 0; i < array_size; i++)
        {
            const PropertyInfo pi = to_property(p_array[i]);
            properties.push_back(pi);
        }

        if (p_sorted)
            properties.sort_custom<PropertyInfoNameSort>();

        return properties;
    }

    Array from_properties(const Vector<PropertyInfo>& p_properties)
    {
        TypedArray<Dictionary> result;
        for (const PropertyInfo& property : p_properties)
            result.push_back(from_property(property));

        return result;
    }
}
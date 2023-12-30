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

namespace DictionaryUtils
{
    PropertyInfo to_property(const Dictionary& p_dict)
    {
        PropertyInfo pi;
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
        if (p_dict.has("usage"))
            pi.usage = p_dict["usage"];
        return pi;
    }

    Dictionary from_property(const PropertyInfo& p_property)
    {
        Dictionary dict;
        dict["name"] = p_property.name;
        dict["type"] = p_property.type;
        dict["class_name"] = p_property.class_name;
        dict["hint"] = p_property.hint;
        dict["hint_string"] = p_property.hint_string;
        dict["usage"] = p_property.usage;
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

    Dictionary from_method(const MethodInfo& p_method)
    {
        Dictionary dict;
        dict["name"] = p_method.name;
        dict["return"] = from_property(p_method.return_val);
        dict["flags"] = p_method.flags;

        Array default_args;
        for (const Variant& default_argument : p_method.default_arguments)
            default_args.push_back(default_argument);
        dict["default_args"] = default_args;

        Array args;
        for (const PropertyInfo& argument : p_method.arguments)
            args.push_back(from_property(argument));
        dict["args"] = args;

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
}
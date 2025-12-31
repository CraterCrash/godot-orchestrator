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
#ifndef ORCHESTRATOR_CORE_GODOT_DOCDATA_H
#define ORCHESTRATOR_CORE_GODOT_DOCDATA_H

#include <godot_cpp/templates/hash_map.hpp>

using namespace godot;

class DocData {
public:
    struct ArgumentDoc {
        String name;
        String type;
        String enumeration;
        bool is_bitfield = false;
        String default_value;

        bool operator<(const ArgumentDoc& p_doc) const;
        static ArgumentDoc from_dict(const Dictionary& p_dict);
        static Dictionary to_dict(const ArgumentDoc& p_doc);
    };

    struct MethodDoc {
        String name;
        String return_type;
        String return_enum;
        bool return_is_bitfield = false;
        String qualifiers;
        String description;
        bool is_deprecated = false;
        String deprecated_message;
        bool is_experimental = false;
        String experimental_message;
        Vector<ArgumentDoc> arguments;
        ArgumentDoc rest_argument;
        Vector<int> errors_returned;
        String keywords;

        bool operator<(const MethodDoc& p_doc) const;
        static MethodDoc from_dict(const Dictionary& p_dict);
        static Dictionary to_dict(const MethodDoc& p_doc);
    };

    struct ConstantDoc {
        String name;
        String value;
        bool is_value_valid = false;
        String type;
        String enumeration;
        bool is_bitfield = false;
        String description;
        bool is_deprecated = false;
        String deprecated_message;
        bool is_experimental = false;
        String experimental_message;
        String keywords;

        bool operator<(const ConstantDoc& p_doc) const;
        static ConstantDoc from_dict(const Dictionary& p_dict);
        static Dictionary to_dict(const ConstantDoc& p_doc);
    };

    struct PropertyDoc {
        String name;
        String type;
        String enumeration;
        bool is_bitfield = false;
        String description;
        String setter, getter;
        String default_value;
        bool overridden = false;
        String overrides;
        bool is_deprecated = false;
        String deprecated_message;
        bool is_experimental = false;
        String experimental_message;
        String keywords;

        bool operator<(const PropertyDoc& p_doc) const;
        static PropertyDoc from_dict(const Dictionary& p_dict);
        static Dictionary to_dict(const PropertyDoc& p_doc);
    };

    struct ThemeItemDoc {
        String name;
        String type;
        String data_type;
        String description;
        bool is_deprecated = false;
        String deprecated_message;
        bool is_experimental = false;
        String experimental_message;
        String default_value;
        String keywords;

        bool operator<(const ThemeItemDoc& p_doc) const;
        static ThemeItemDoc from_dict(const Dictionary& p_dict);
        static Dictionary to_dict(const ThemeItemDoc& p_doc);
    };

    struct TutorialDoc {
        String link;
        String title;

        static TutorialDoc from_dict(const Dictionary& p_dict);
        static Dictionary to_dict(const TutorialDoc& p_doc);
    };

    struct EnumDoc {
        String description;
        bool is_deprecated = false;
        String deprecated_message;
        bool is_experimental = false;
        String experimental_message;

        static EnumDoc from_dict(const Dictionary& p_dict);
        static Dictionary to_dict(const EnumDoc& p_doc);
    };

    struct ClassDoc {
        String name;
        String inherits;
        String brief_description;
        String description;
        String keywords;
        Vector<TutorialDoc> tutorials;
        Vector<MethodDoc> constructors;
        Vector<MethodDoc> methods;
        Vector<MethodDoc> operators;
        Vector<MethodDoc> signals;
        Vector<ConstantDoc> constants;
        HashMap<String, EnumDoc> enums;
        Vector<PropertyDoc> properties;
        Vector<MethodDoc> annotations;
        Vector<ThemeItemDoc> theme_properties;
        bool is_deprecated = false;
        String deprecated_message;
        bool is_experimental = false;
        String experimental_message;
        bool is_script_doc = false;
        String script_path;

        bool operator<(const ClassDoc& p_doc) const;
        static ClassDoc from_dict(const Dictionary& p_dict);
        static Dictionary to_dict(const ClassDoc& p_doc);
    };
};

#endif // ORCHESTRATOR_CORE_GODOT_DOCDATA_H
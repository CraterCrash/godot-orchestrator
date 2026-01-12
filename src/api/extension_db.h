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
#ifndef ORCHESTRATOR_EXTENSION_DB_H
#define ORCHESTRATOR_EXTENSION_DB_H

#include "common/variant_operators.h"

#include <godot_cpp/core/method_bind.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

namespace godot {

    struct EnumValue {
            StringName name;
            StringName friendly_name;
            int value = 0;
        };

        struct EnumInfo {
            StringName name;
            bool is_bitfield = false;
            Vector<EnumValue> values;
        };

        struct FunctionInfo {
            MethodInfo method;
            StringName category;
            int64_t hash = 0;
            String description;

            _FORCE_INLINE_ bool is_vararg() const { return method.flags & METHOD_FLAG_VARARG; }
        };

        struct OperatorInfo {
            VariantOperators::Code op = VariantOperators::OP_EQUAL;
            StringName code;
            StringName name;
            Variant::Type left_type = Variant::NIL;
            StringName left_type_name;
            Variant::Type right_type = Variant::NIL;
            StringName right_type_name;
            Variant::Type return_type = Variant::NIL;
        };

        struct ConstructorInfo {
            Vector<PropertyInfo> arguments;
        };

        struct ConstantInfo {
            StringName name;
            Variant::Type type = Variant::NIL;
            Variant value;
        };

        struct BuiltInType {
            StringName name;
            Variant::Type type = Variant::NIL;
            bool keyed = false;
            bool has_destructor = false;
            Vector<OperatorInfo> operators;
            Vector<ConstructorInfo> constructors;
            Vector<PropertyInfo> properties;
            Vector<ConstantInfo> constants;
            Vector<EnumInfo> enums;
            Variant::Type index_returning_type = Variant::NIL;
            HashMap<StringName, FunctionInfo> methods;

            Vector<MethodInfo> get_method_list() const;
        };

        struct ClassMethodInfo {
            MethodInfo method;
            int64_t hash = 0;
            String description;
        };

        struct ClassPropertyInfo {
            PropertyInfo property;
            String getter;
            String setter;
            String description;
        };

        struct ClassSignalInfo {
            MethodInfo method;
            String description;
        };

        struct ClassInfo {
            StringName name;
            bool ref_counted = false;
            bool instantiable = false;
            StringName parent_class;
            StringName api_type;
            Vector<StringName> bitfield_enums;
            HashMap<StringName, ClassMethodInfo> methods;
            HashMap<StringName, ClassPropertyInfo> properties;
            HashMap<StringName, ClassSignalInfo> signals;
            String brief_description;
            String description;
            ClassInfo* parent = nullptr;
        };

    class ExtensionDB {
        static ExtensionDB* _singleton;

        HashMap<StringName, Variant::Type> variant_name_to_type;
        HashMap<StringName, StringName> operator_names;
        HashMap<StringName, VariantOperators::Code> operator_codes;

        HashMap<StringName, ConstantInfo> math_constants;
        PackedStringArray math_constant_names;

        HashMap<StringName, BuiltInType> builtin_types;
        HashMap<Variant::Type, StringName> builtin_types_to_name;

        HashMap<StringName, EnumInfo> global_enums;
        PackedStringArray global_enum_names;
        PackedStringArray global_enum_value_names;

        HashMap<StringName, FunctionInfo> utility_functions;

        HashMap<StringName, ClassInfo> classes;

        static String _resolve_enum_prefix(const Vector<EnumValue>& p_enum_values);
        static bool _is_enum_values_upper_cased(const EnumInfo& p_enumeration);
        static void _sanitize(EnumInfo& p_enumeration);

        Variant::Type _resolve_variant_type_from_name(const String& p_name);

        String _resolve_operator_name(const String& p_name);
        VariantOperators::Code _resolve_operator_type(const String& p_name);

        static int32_t _resolve_method_flags(const Dictionary& p_method);
        PropertyInfo _resolve_type_to_property(const String& p_type, const String& p_name = String());
        PropertyInfo _resolve_method_return(const Dictionary& p_method);
        PropertyInfo _resolve_method_argument(const Dictionary& p_argument);
        static Variant _resolve_method_argument_default(const Dictionary& p_argument);

        void _decompress_and_load(); // NOLINT - generated dynamically
        void _load(const PackedByteArray& p_data);

        void _load_builtin_types(const Dictionary& p_data);
        void _load_global_enumerations(const Dictionary& p_data);
        void _load_utility_functions(const Dictionary& p_data);
        void _load_classes(const Dictionary& p_data);

    public:
        // Built-in Types
        static bool is_builtin_type(const StringName& p_type_name);
        static Vector<BuiltInType> get_builtin_types();
        static BuiltInType get_builtin_type(const StringName& p_type_name);
        static BuiltInType get_builtin_type(Variant::Type p_type);

        // Global Enumerations
        static PackedStringArray get_global_enum_names();
        static PackedStringArray get_global_enum_value_names();
        static EnumInfo get_global_enum(const StringName& p_enum_name);
        static EnumInfo get_global_enum_by_value(const StringName& p_enum_name);
        static EnumValue get_global_enum_value(const StringName& p_enum_value_name);

        // Math constants
        static PackedStringArray get_math_constant_names();
        static ConstantInfo get_math_constant(const StringName& p_constant_name);

        // Utility Functions
        static bool is_utility_function(const StringName& p_method_name);
        static Vector<FunctionInfo> get_utility_functions();
        static FunctionInfo get_utility_function(const StringName& p_name);

        // Classes
        static bool is_class_enum_bitfield(const StringName& p_class_name, const StringName& p_enum_name);
        static PackedStringArray get_class_static_function_names(const StringName& p_class_name);
        static bool get_class_method_info(const StringName& p_class_name, const StringName& p_method_name, MethodInfo& r_info, bool p_no_inheritance = false);
        static MethodBind* get_method(const StringName& p_class_name, const StringName& p_method_name);

        ExtensionDB();
        ~ExtensionDB();
    };
}

#endif // ORCHESTRATOR_EXTENSION_DB_H
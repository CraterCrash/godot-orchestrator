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
#ifndef ORCHESTRATOR_CORE_GODOT_VARIANT_H
#define ORCHESTRATOR_CORE_GODOT_VARIANT_H

#include <vector>

#include <godot_cpp/core/object.hpp>

using namespace godot;

/// Creates a Godot Variant array (vector) of values from a variadic list of arguments
template <typename... VarArgs>
std::vector<Variant> varray(VarArgs... p_args) {
    std::vector<Variant> values;
    const Variant args[sizeof...(p_args) + 1] = { p_args..., Variant() };

    const uint32_t arg_count = sizeof...(p_args);
    if (arg_count > 0) {
        values.resize(arg_count);
        for (uint32_t index = 0; index < arg_count; index++) {
            values[index] = args[index];
        }
    }

    return values;
}

namespace GDE {
    struct Variant {
        using ConstVariantPtrs = const godot::Variant**;
        using Type = godot::Variant::Type;

        Variant() = delete;

        enum UtilityFunctionType {
            UTILITY_FUNC_TYPE_MATH,
            UTILITY_FUNC_TYPE_RANDOM,
            UTILITY_FUNC_TYPE_GENERAL
        };

        struct StringLikeVariantOrder {
            static bool compare(const godot::Variant& p_lhs, const godot::Variant& p_rhs);
            _ALWAYS_INLINE_ bool operator()(const godot::Variant& p_lhs, const godot::Variant& p_rhs) const {
                return compare(p_lhs, p_rhs);
            }
        };

        // This should remain aligned with godot/core/variant/variant.h
        struct StringLikeVariantComparator {
            static bool compare(const godot::Variant& p_lhs, const godot::Variant& p_rhs);
        };

        _ALWAYS_INLINE_ static Type as_type(int p_type) { return static_cast<Type>(p_type); }

        static bool is_null(const godot::Variant& p_value);
        static bool is_read_only(const godot::Variant& p_value);
        static bool is_ref_counted(const godot::Variant& p_value);
        static bool is_type_shared(Type p_type);

        static godot::Variant evaluate(godot::Variant::Operator p_operator, const godot::Variant& p_left, const godot::Variant& p_right, bool& r_valid);
        static godot::Variant evaluate(godot::Variant::Operator p_operator, const godot::Variant& p_left, const godot::Variant& p_right);
        static StringName get_operator_name(godot::Variant::Operator p_operator);
        static Type get_operator_return_type(godot::Variant::Operator p_operator, Type p_left, Type p_right);
        static GDExtensionPtrOperatorEvaluator get_validated_operator_evaluator(godot::Variant::Operator p_operator, Type p_left, Type p_right);

        static GDExtensionCallError construct(Type p_type, godot::Variant& r_value, ConstVariantPtrs p_args, int p_arg_count);
        static void construct(Type p_type, godot::Variant& r_value, ConstVariantPtrs p_args, int p_arg_count, GDExtensionCallError& r_error);

        static Vector<MethodInfo> get_constructor_list(Type p_type);
        static Vector<MethodInfo> get_method_list(const godot::Variant& p_value);
        static Vector<PropertyInfo> get_property_list(const godot::Variant& p_value);

        static Type get_member_type(Type p_type, const StringName& p_name);

        static GDExtensionPtrSetter get_member_validated_setter(Type p_type, const StringName& p_name);
        static GDExtensionPtrGetter get_member_validated_getter(Type p_type, const StringName& p_name);

        static GDExtensionPtrIndexedSetter get_member_validated_indexed_setter(Type p_type);
        static GDExtensionPtrIndexedGetter get_member_validated_indexed_getter(Type p_type);

        static GDExtensionPtrKeyedSetter get_member_validated_keyed_setter(Type p_type);
        static GDExtensionPtrKeyedGetter get_member_validated_keyed_getter(Type p_type);
        
        static bool has_constant(Type p_type, const StringName& p_value);
        static godot::Variant get_constant_value(Type p_type, const StringName& p_constant_name, bool* r_valid = nullptr);

        static bool has_enum(Type p_type, const StringName& p_enum_name);
        static int get_enum_value(Type p_type, const StringName& p_enum_name, const StringName& p_enumeration, bool* r_valid = nullptr);
        static Vector<StringName> get_enumerations_for_enum(Type p_type, const StringName& p_enum_name);
        static StringName get_enum_for_enumeration(Type p_type, const StringName& p_enumeration);

        static bool has_builtin_method(Type p_type, const StringName& p_name);
        static bool has_builtin_method_return_value(Type p_type, const StringName& p_name);
        static MethodInfo get_builtin_method_info(Type p_type, const StringName& p_name);
        static MethodInfo get_builtin_method(Type p_type, const StringName& p_name); // consolidate?
        static int64_t get_builtin_method_hash(Type p_type, const StringName& p_name);

        static bool has_utility_function(const StringName& p_function);
        static bool has_utility_function_return_value(const StringName& p_function);
        static bool call_utility_function(const StringName& p_function, godot::Variant& r_value, ConstVariantPtrs p_args, int p_arg_count, GDExtensionCallError& r_error, String& r_reason);
        static bool call_utility_function(const StringName& p_function, godot::Variant& r_value, ConstVariantPtrs p_args, int p_arg_count, GDExtensionCallError& r_error);
        static MethodInfo get_utility_function_method_info(const StringName& p_function);
        static int get_utility_function_argument_count(const StringName& p_function);
        static Type get_utility_function_return_type(const StringName& p_function);
        static UtilityFunctionType get_utility_function_type(const StringName& p_function);

        static Object* get_validated_object_with_check(const godot::Variant& p_value, bool& r_previously_freed);

        static String get_call_error_text(const StringName& p_method, ConstVariantPtrs p_args, int p_arg_count, const GDExtensionCallError& r_error);
        static String get_call_error_text(Object* p_base, const StringName& p_method, ConstVariantPtrs p_args, int p_arg_count, const GDExtensionCallError& r_error);
    };
}

#endif // ORCHESTRATOR_CORE_GODOT_VARIANT_H

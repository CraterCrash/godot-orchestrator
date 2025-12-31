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
#include "core/godot/variant/variant.h"

#include "api/extension_db.h"
#include "common/dictionary_utils.h"
#include "common/method_utils.h"
#include "common/resource_utils.h"
#include "common/string_utils.h"
#include "orchestration/serialization/text/variant_parser.h"

#include <godot_cpp/classes/expression.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/variant_internal.hpp>

template <typename T> T cast_to(const Variant& p_value) {
    return T(p_value);
}

template <typename L, typename R>
        constexpr int64_t str_compare(const L *l_ptr, const R *r_ptr) {
    while (true) {
        const char32_t l = *l_ptr;
        const char32_t r = *r_ptr;

        if (l == 0 || l != r) {
            return static_cast<int64_t>(l) - static_cast<int64_t>(r);
        }

        l_ptr++;
        r_ptr++;
    }
}

struct AlphCompare {
    template <typename LT, typename RT>
    _FORCE_INLINE_ bool operator()(const LT &l, const RT &r) const {
        return compare(l, r);
    }
    _FORCE_INLINE_ static bool compare(const StringName &l, const StringName &r) {
        const String lhs = l;
        const String rhs = r;
        return str_compare(lhs.utf8().get_data(), rhs.utf8().get_data()) < 0;
    }
    _FORCE_INLINE_ static bool compare(const String &l, const StringName &r) {
        const String rhs = r;
        return str_compare(l.utf8().get_data(), rhs.utf8().get_data()) < 0;
    }
    _FORCE_INLINE_ static bool compare(const StringName &l, const String &r) {
        const String lhs = l;
        return str_compare(lhs.utf8().get_data(), r.utf8().get_data()) < 0;
    }
    _FORCE_INLINE_ static bool compare(const String &l, const String &r) {
        return str_compare(l.utf8().get_data(), r.utf8().get_data()) < 0;
    }
};

bool GDE::Variant::StringLikeVariantOrder::compare(const godot::Variant &p_lhs, const godot::Variant &p_rhs) {
    if (p_lhs.get_type() == godot::Variant::STRING) {
        const String& lhs = *VariantInternal::get_string(&p_lhs);
        if (p_rhs.get_type() == godot::Variant::STRING) {
            return AlphCompare::compare(lhs, *VariantInternal::get_string(&p_rhs));
        } else if (p_rhs.get_type() == godot::Variant::STRING_NAME) {
            return AlphCompare::compare(lhs, *VariantInternal::get_string_name(&p_rhs));
        }
    } else if (p_lhs.get_type() == godot::Variant::STRING_NAME) {
        const StringName& lhs = *VariantInternal::get_string_name(&p_lhs);
        if (p_rhs.get_type() == godot::Variant::STRING) {
            return AlphCompare::compare(lhs, *VariantInternal::get_string(&p_rhs));
        } else if (p_rhs.get_type() == godot::Variant::STRING_NAME) {
            return AlphCompare::compare(lhs, *VariantInternal::get_string_name(&p_rhs));
        }
    }

    return p_lhs < p_rhs;
}

bool GDE::Variant::StringLikeVariantComparator::compare(const godot::Variant& p_lhs, const godot::Variant& p_rhs) {
    if (p_lhs.hash_compare(p_rhs)) {
        return true;
    }
    if (p_lhs.get_type() == godot::Variant::STRING && p_rhs.get_type() == godot::Variant::STRING_NAME) {
        return *VariantInternal::get_string(&p_lhs) == *VariantInternal::get_string_name(&p_rhs);
    }
    if (p_lhs.get_type() == godot::Variant::STRING_NAME && p_rhs.get_type() == godot::Variant::STRING) {
        return *VariantInternal::get_string_name(&p_lhs) == *VariantInternal::get_string(&p_rhs);
    }
    return false;
}

bool GDE::Variant::is_null(const godot::Variant& p_value) {
    if (p_value.get_type() == godot::Variant::OBJECT) {
        return cast_to<Object*>(p_value) != nullptr;
    } else {
        return true;
    }
}

bool GDE::Variant::is_read_only(const godot::Variant& p_value) {
    switch (p_value.get_type()) {
        case godot::Variant::ARRAY:
            return reinterpret_cast<const Array&>(p_value).is_read_only();
        case godot::Variant::DICTIONARY:
            return reinterpret_cast<const Dictionary&>(p_value).is_read_only();
        default:
            return false;
    }
}

bool GDE::Variant::is_ref_counted(const godot::Variant& p_value) {
    if (p_value.get_type() == godot::Variant::OBJECT) {
        Object* object = cast_to<Object*>(p_value);
        return ObjectID(object->get_instance_id()).is_ref_counted();
    }
    return false;
}

bool GDE::Variant::is_type_shared(Type p_type) {
    // take from variant.cpp
    switch (p_type) {
        case godot::Variant::OBJECT:
        case godot::Variant::ARRAY:
        case godot::Variant::DICTIONARY:
            return true;
        default:
            return false;
    }
}

Variant GDE::Variant::evaluate(godot::Variant::Operator p_operator, const godot::Variant& p_left, const godot::Variant& p_right, bool& r_valid) {
    r_valid = false;
    godot::Variant result;
    godot::Variant::evaluate(p_operator, p_left, p_right, result, r_valid);
    return result;
}

bool GDE::Variant::evaluate(godot::Variant::Operator p_operator, const godot::Variant& p_left, const godot::Variant& p_right) {
    bool r_valid = false;
    evaluate(p_operator, p_left, p_right, r_valid);
    return r_valid;
}

StringName GDE::Variant::get_operator_name(godot::Variant::Operator p_operator) {
    static const char *_op_names[godot::Variant::OP_MAX] = {
        "==",
        "!=",
        "<",
        "<=",
        ">",
        ">=",
        "+",
        "-",
        "*",
        "/",
        "unary-",
        "unary+",
        "%",
        "**",
        "<<",
        ">>",
        "&",
        "|",
        "^",
        "~",
        "and",
        "or",
        "xor",
        "not",
        "in"
    };

    ERR_FAIL_INDEX_V(p_operator, godot::Variant::OP_MAX, "");
    return _op_names[p_operator];
}

GDE::Variant::Type GDE::Variant::get_operator_return_type(godot::Variant::Operator p_operator, Type p_left, Type p_right) {
    ERR_FAIL_INDEX_V(p_operator, godot::Variant::OP_MAX, godot::Variant::NIL);
    ERR_FAIL_INDEX_V(p_left, godot::Variant::VARIANT_MAX, godot::Variant::NIL);

    const BuiltInType built_in_type = ExtensionDB::get_builtin_type(p_left);
    for (const OperatorInfo& info : built_in_type.operators) {
        if (info.left_type == p_left && info.right_type == p_right && VariantOperators::to_engine(info.op) == p_operator) {
            return info.return_type;
        }
    }

    ERR_FAIL_V_MSG(godot::Variant::NIL, "Failed to resolve return type for operator mapping");
}

GDExtensionPtrOperatorEvaluator GDE::Variant::get_validated_operator_evaluator(godot::Variant::Operator p_operator, Type p_left, Type p_right) {
    GDExtensionVariantOperator op;
    switch (p_operator) {
        case godot::Variant::Operator::OP_EQUAL:
            op = GDEXTENSION_VARIANT_OP_EQUAL;
            break;
        case godot::Variant::Operator::OP_NOT_EQUAL:
            op = GDEXTENSION_VARIANT_OP_NOT_EQUAL;
            break;
        case godot::Variant::Operator::OP_LESS:
            op = GDEXTENSION_VARIANT_OP_LESS;
            break;
        case godot::Variant::Operator::OP_LESS_EQUAL:
            op = GDEXTENSION_VARIANT_OP_LESS_EQUAL;
            break;
        case godot::Variant::Operator::OP_GREATER:
            op = GDEXTENSION_VARIANT_OP_GREATER;
            break;
        case godot::Variant::Operator::OP_GREATER_EQUAL:
            op = GDEXTENSION_VARIANT_OP_GREATER_EQUAL;
            break;
        case godot::Variant::Operator::OP_ADD:
            op = GDEXTENSION_VARIANT_OP_ADD;
            break;
        case godot::Variant::Operator::OP_SUBTRACT:
            op = GDEXTENSION_VARIANT_OP_SUBTRACT;
            break;
        case godot::Variant::Operator::OP_MULTIPLY:
            op = GDEXTENSION_VARIANT_OP_MULTIPLY;
            break;
        case godot::Variant::Operator::OP_DIVIDE:
            op = GDEXTENSION_VARIANT_OP_DIVIDE;
            break;
        case godot::Variant::Operator::OP_NEGATE:
            op = GDEXTENSION_VARIANT_OP_NEGATE;
            break;
        case godot::Variant::Operator::OP_POSITIVE:
            op = GDEXTENSION_VARIANT_OP_POSITIVE;
            break;
        case godot::Variant::Operator::OP_MODULE:
            op = GDEXTENSION_VARIANT_OP_MODULE;
            break;
        case godot::Variant::Operator::OP_POWER:
            op = GDEXTENSION_VARIANT_OP_POWER;
            break;
        case godot::Variant::Operator::OP_SHIFT_LEFT:
            op = GDEXTENSION_VARIANT_OP_SHIFT_LEFT;
            break;
        case godot::Variant::Operator::OP_SHIFT_RIGHT:
            op = GDEXTENSION_VARIANT_OP_SHIFT_RIGHT;
            break;
        case godot::Variant::Operator::OP_BIT_AND:
            op = GDEXTENSION_VARIANT_OP_BIT_AND;
            break;
        case godot::Variant::Operator::OP_BIT_OR:
            op = GDEXTENSION_VARIANT_OP_BIT_OR;
            break;
        case godot::Variant::Operator::OP_BIT_XOR:
            op = GDEXTENSION_VARIANT_OP_BIT_XOR;
            break;
        case godot::Variant::Operator::OP_BIT_NEGATE:
            op = GDEXTENSION_VARIANT_OP_BIT_NEGATE;
            break;
        case godot::Variant::Operator::OP_AND:
            op = GDEXTENSION_VARIANT_OP_AND;
            break;
        case godot::Variant::Operator::OP_OR:
            op = GDEXTENSION_VARIANT_OP_OR;
            break;
        case godot::Variant::Operator::OP_XOR:
            op = GDEXTENSION_VARIANT_OP_XOR;
            break;
        case godot::Variant::Operator::OP_NOT:
            op = GDEXTENSION_VARIANT_OP_NOT;
            break;
        case godot::Variant::Operator::OP_IN:
            op = GDEXTENSION_VARIANT_OP_IN;
            break;
        case godot::Variant::Operator::OP_MAX:
            op = GDEXTENSION_VARIANT_OP_MAX;
            break;
        default:
            ERR_FAIL_V_MSG(nullptr, "Failed to map Variant operator: " + itos(p_operator));
    }

    return internal::gdextension_interface_variant_get_ptr_operator_evaluator(
        op,
        static_cast<GDExtensionVariantType>(p_left),
        static_cast<GDExtensionVariantType>(p_right));
}

GDExtensionCallError GDE::Variant::construct(Type p_type, godot::Variant& r_value, ConstVariantPtrs p_args, int p_arg_count) {
    GDExtensionCallError error;
    internal::gdextension_interface_variant_construct(
        static_cast<GDExtensionVariantType>(p_type),
        &r_value,
        reinterpret_cast<const GDExtensionConstVariantPtr*>(p_args),
        p_arg_count,
        &error);
    return error;
}

void GDE::Variant::construct(Type p_type, godot::Variant& r_value, ConstVariantPtrs p_args, int p_arg_count, GDExtensionCallError& r_error) {
    r_error = construct(p_type, r_value, p_args, p_arg_count);
}

Vector<MethodInfo> GDE::Variant::get_constructor_list(Type p_type) {
    ERR_FAIL_INDEX_V(p_type, godot::Variant::VARIANT_MAX, {});

    Vector<MethodInfo> constructors;
    const BuiltInType type = ExtensionDB::get_builtin_type(p_type);
    for (const ConstructorInfo& info : type.constructors) {
        MethodInfo method;
        method.return_val.type = p_type;
        method.name = godot::Variant::get_type_name(p_type);
        for (const PropertyInfo& argument : info.arguments) {
            method.arguments.push_back(argument);
        }
        constructors.push_back(method);
    }

    return constructors;
}

Vector<MethodInfo> GDE::Variant::get_method_list(const godot::Variant& p_value) {
    if (p_value.get_type() == godot::Variant::OBJECT) {
        Object *object = p_value.get_validated_object();
        if (object) {
            Vector<MethodInfo> methods;
            const TypedArray<Dictionary> method_list = object->get_method_list();
            for (const godot::Variant& entry : method_list) {
                methods.push_back(DictionaryUtils::to_method(entry));
            }
            return methods;
        }
        return {};
    }

    return ExtensionDB::get_builtin_type(p_value.get_type()).methods;
}

Vector<PropertyInfo> GDE::Variant::get_property_list(const godot::Variant& p_value) {
    if (p_value.get_type() == godot::Variant::DICTIONARY) {
        Vector<PropertyInfo> properties;
        const Dictionary& dict = p_value;
        for (const godot::Variant& key : dict.keys()) {
            if (key.get_type() == godot::Variant::STRING){
                properties.push_back(PropertyInfo(dict.get(key, godot::Variant()).get_type(), key));
            }
        }
        return properties;
    } else if (p_value.get_type() == godot::Variant::OBJECT) {
        Vector<PropertyInfo> properties;
        Object* object = p_value.get_validated_object();
        ERR_FAIL_NULL_V(object, properties);

        const TypedArray<Dictionary> property_list = object->get_property_list();
        for (const godot::Variant& entry : property_list) {
            properties.push_back(DictionaryUtils::to_property(entry));
        }
        return properties;
    }

    return ExtensionDB::get_builtin_type(p_value.get_type()).properties;
}

GDE::Variant::Type GDE::Variant::get_member_type(Type p_type, const StringName& p_name) {
    BuiltInType built_in_type = ExtensionDB::get_builtin_type(p_type);
    for (const PropertyInfo& property : built_in_type.properties) {
        if (property.name == p_name) {
            return property.type;
        }
    }
    ERR_FAIL_V_MSG(godot::Variant::NIL, "Failed to resolve member type for " + p_name);
}

GDExtensionPtrSetter GDE::Variant::get_member_validated_setter(Type p_type, const StringName& p_name) {
    return internal::gdextension_interface_variant_get_ptr_setter(static_cast<GDExtensionVariantType>(p_type), &p_name);
}

GDExtensionPtrGetter GDE::Variant::get_member_validated_getter(Type p_type, const StringName& p_name) {
    return internal::gdextension_interface_variant_get_ptr_getter(static_cast<GDExtensionVariantType>(p_type), &p_name);
}

GDExtensionPtrIndexedSetter GDE::Variant::get_member_validated_indexed_setter(Type p_type) {
    return internal::gdextension_interface_variant_get_ptr_indexed_setter(static_cast<GDExtensionVariantType>(p_type));
}

GDExtensionPtrIndexedGetter GDE::Variant::get_member_validated_indexed_getter(Type p_type) {
    return internal::gdextension_interface_variant_get_ptr_indexed_getter(static_cast<GDExtensionVariantType>(p_type));
}

GDExtensionPtrKeyedSetter GDE::Variant::get_member_validated_keyed_setter(Type p_type) {
    return internal::gdextension_interface_variant_get_ptr_keyed_setter(static_cast<GDExtensionVariantType>(p_type));
}

GDExtensionPtrKeyedGetter GDE::Variant::get_member_validated_keyed_getter(Type p_type) {
    return internal::gdextension_interface_variant_get_ptr_keyed_getter(static_cast<GDExtensionVariantType>(p_type));
}

bool GDE::Variant::has_constant(Type p_type, const StringName& p_value) {
    ERR_FAIL_INDEX_V(p_type, godot::Variant::VARIANT_MAX, false);

    const BuiltInType type = ExtensionDB::get_builtin_type(p_type);
    for (const ConstantInfo& ci : type.constants) {
        if (ci.name == p_value) {
            return true;
        }
    }
    return false;
}

Variant GDE::Variant::get_constant_value(Type p_type, const StringName& p_constant_name, bool* r_valid) {
    if (r_valid) {
        *r_valid = false;
    }

    ERR_FAIL_INDEX_V(p_type, godot::Variant::VARIANT_MAX, 0);

    const BuiltInType type = ExtensionDB::get_builtin_type(p_type);
    for (const ConstantInfo& ci : type.constants) {
        if (ci.name == p_constant_name) {
            if (r_valid) {
                *r_valid = true;
            }
            return ci.value;
        }
    }

    return -1;
}

bool GDE::Variant::has_enum(Type p_type, const StringName& p_enum_name) {
    ERR_FAIL_INDEX_V(p_type, godot::Variant::VARIANT_MAX, false);

    const BuiltInType type = ExtensionDB::get_builtin_type(p_type);
    for (const EnumInfo& ei : type.enums) {
        if (ei.name == p_enum_name) {
            return true;
        }
    }
    return false;
}

int GDE::Variant::get_enum_value(Type p_type, const StringName& p_enum_name, const StringName& p_enumeration, bool* r_valid) {
    if (r_valid) {
        *r_valid = false;
    }

    ERR_FAIL_INDEX_V(p_type, godot::Variant::VARIANT_MAX, -1);

    const BuiltInType type = ExtensionDB::get_builtin_type(p_type);
    for (const EnumInfo& ei : type.enums) {
        if (ei.name == p_enum_name) {
            for (const EnumValue& value : ei.values) {
                if (value.name == p_enumeration) {
                    if (r_valid) {
                        *r_valid = true;
                    }
                    return value.value;
                }
            }
        }
    }

    return -1;
}

Vector<StringName> GDE::Variant::get_enumerations_for_enum(Type p_type, const StringName& p_enum_name) {
    ERR_FAIL_INDEX_V(p_type, godot::Variant::VARIANT_MAX, {});

    Vector<StringName> results;
    const BuiltInType type = ExtensionDB::get_builtin_type(p_type);
    for (const EnumInfo& ei : type.enums) {
        if (ei.name == p_enum_name) {
            for (const EnumValue& value : ei.values) {
                results.push_back(value.name);
            }
        }
    }
    return results;
}

StringName GDE::Variant::get_enum_for_enumeration(Type p_type, const StringName& p_enumeration) {
    ERR_FAIL_INDEX_V(p_type, godot::Variant::VARIANT_MAX, {});

    const BuiltInType type = ExtensionDB::get_builtin_type(p_type);
    for (const EnumInfo& ei : type.enums) {
        for (const EnumValue& value : ei.values) {
            if (value.name == p_enumeration) {
                return ei.name;
            }
        }
    }

    return {};
}

bool GDE::Variant::has_builtin_method(Type p_type, const StringName& p_name) {
    return ExtensionDB::get_builtin_type(p_type).method_hashes.has(p_name);
}

bool GDE::Variant::has_builtin_method_return_value(Type p_type, const StringName& p_name) {
    const BuiltInType built_in_type = ExtensionDB::get_builtin_type(p_type);
    for (const MethodInfo& method : built_in_type.methods) {
        if (method.name == p_name) {
            return MethodUtils::has_return_value(method);
        }
    }
    return false;
}

MethodInfo GDE::Variant::get_builtin_method_info(Type p_type, const StringName& p_name) {
    const BuiltInType built_in_type = ExtensionDB::get_builtin_type(p_type);
    for (const MethodInfo& mi : built_in_type.methods) {
        if (mi.name == p_name) {
            return mi;
        }
    }
    return {};
}

MethodInfo GDE::Variant::get_builtin_method(Type p_type, const StringName& p_name) {
    return get_builtin_method_info(p_type, p_name);
}

int64_t GDE::Variant::get_builtin_method_hash(Type p_type, const StringName& p_name) {
    const BuiltInType built_in_type = ExtensionDB::get_builtin_type(p_type);
    if (built_in_type.method_hashes.has(p_name)) {
        return built_in_type.method_hashes[p_name];
    }
    return 0;
}

bool GDE::Variant::has_utility_function(const StringName& p_function) {
    return ExtensionDB::get_function_names().has(p_function);
}

bool GDE::Variant::has_utility_function_return_value(const StringName& p_function) {
    if (has_utility_function(p_function)) {
        return MethodUtils::has_return_value(ExtensionDB::get_function(p_function).return_val);
    }
    return false;
}

bool GDE::Variant::call_utility_function(const StringName& p_function, godot::Variant& r_value, ConstVariantPtrs p_args, int p_arg_count, GDExtensionCallError& r_error, String& r_reason) {
    Array args;
    PackedStringArray arg_names;
    for (int i = 0; i < p_arg_count; i++) {
        arg_names.push_back(vformat("x%d", i));
        args.push_back(*p_args[i]);
    }

    const String expression = vformat("%s(%s)", p_function, StringUtils::join(",", arg_names));
    Ref<Expression> parser;
    parser.instantiate();

    Error err = parser->parse(expression, arg_names);
    if (err != OK) {
        r_error.error = GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT;
        r_reason = vformat(R"*(Error calling utility function "%s()": %s)*", p_function, parser->get_error_text());
        return false;
    }

    godot::Variant result = parser->execute(args);
    if (parser->has_execute_failed()) {
        r_error.error = GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT;
        r_reason = vformat(R"*(Error executing utility function "%s()": %s)*", p_function, parser->get_error_text());
        return false;
    }

    r_error.error = GDEXTENSION_CALL_OK;
    r_value = result;
    return true;
}

bool GDE::Variant::call_utility_function(const StringName& p_function, godot::Variant& r_value, ConstVariantPtrs p_args, int p_arg_count, GDExtensionCallError& r_error) {
    String reason;
    return call_utility_function(p_function, r_value, p_args, p_arg_count, r_error, reason);
}

MethodInfo GDE::Variant::get_utility_function_method_info(const StringName& p_function) {
    ERR_FAIL_COND_V(!has_utility_function(p_function), {});

    const FunctionInfo& fi = ExtensionDB::get_function(p_function);

    MethodInfo info(p_function);
    if (MethodUtils::has_return_value(fi.return_val)) {
        info.return_val.type = fi.return_val.type;
        if (info.return_val.type == godot::Variant::NIL) {
            info.return_val.usage |= PROPERTY_USAGE_NIL_IS_VARIANT;
        }
    }

    if (fi.is_vararg) {
        info.flags |= METHOD_FLAG_VARARG;
    } else {
        // This intentionally copies minimal data during OScriptAnalyzer
        for (int i = 0; i < fi.arguments.size(); i++) {
            const PropertyInfo& pi = fi.arguments[i];
            PropertyInfo property;
            #ifdef DEBUG_ENABLED
            property.name = pi.name;
            #else
            property.name = "arg" + itos(i + 1);
            #endif
            property.type = pi.type;
            info.arguments.push_back(property);
        }
    }

    return info;
}

int GDE::Variant::get_utility_function_argument_count(const StringName& p_function) {
    ERR_FAIL_COND_V(!has_utility_function(p_function), 0);
    return ExtensionDB::get_function(p_function).arguments.size();
}

GDE::Variant::Type GDE::Variant::get_utility_function_return_type(const StringName& p_function) {
    if (has_utility_function(p_function)) {
        return ExtensionDB::get_function(p_function).return_val.type;
    }
    return godot::Variant::NIL;
}

GDE::Variant::UtilityFunctionType GDE::Variant::get_utility_function_type(const StringName& p_function) {
    ERR_FAIL_COND_V(!has_utility_function(p_function), UTILITY_FUNC_TYPE_GENERAL);

    const FunctionInfo& fi = ExtensionDB::get_function(p_function);
    if (fi.category == StringName("math")) {
        return UTILITY_FUNC_TYPE_MATH;
    } else if (fi.category == StringName("random")) {
        return UTILITY_FUNC_TYPE_RANDOM;
    } else if (fi.category == StringName("general")) {
        return UTILITY_FUNC_TYPE_GENERAL;
    }

    ERR_FAIL_V_MSG(UTILITY_FUNC_TYPE_GENERAL, "Unknown function category: " + fi.category);
}

Object* GDE::Variant::get_validated_object_with_check(const godot::Variant& p_value, bool& r_previously_freed) {
    if (p_value.get_type() == godot::Variant::OBJECT) {
        Object* instance = p_value.get_validated_object();
        r_previously_freed = !instance && ObjectID(p_value) != ObjectID();
        return instance;
    } else {
        r_previously_freed = false;
        return nullptr;
    }
}

String GDE::Variant::get_call_error_text(const StringName& p_method, ConstVariantPtrs p_args, int p_arg_count, const GDExtensionCallError& r_error) {
    return get_call_error_text(nullptr, p_method, p_args, p_arg_count, r_error);
}

String GDE::Variant::get_call_error_text(Object* p_base, const StringName& p_method, ConstVariantPtrs p_args, int p_arg_count, const GDExtensionCallError& r_error) {
    String err_text;

    if (r_error.error == GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT) {
        const int error_arg = r_error.argument;
        const String expected_type = godot::Variant::get_type_name(static_cast<Type>(r_error.expected));
        if (p_args) {
            const String arg_type = godot::Variant::get_type_name(p_args[error_arg]->get_type());
            err_text = "Cannot convert argument " + itos(error_arg + 1) + " from " + arg_type + " to " + expected_type;
        } else {
            err_text = "Cannot convert argument " + itos(error_arg + 1) + " from [missing argptr, type unknown] to " + expected_type;
        }
    } else if (r_error.error == GDEXTENSION_CALL_ERROR_TOO_MANY_ARGUMENTS) {
        err_text = "Method expected " + itos(r_error.expected) + " argument(s), but called with " + itos(p_arg_count);
    } else if (r_error.error == GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS) {
        err_text = "Method expected " + itos(r_error.expected) + " argument(s), but called with " + itos(p_arg_count);
    } else if (r_error.error == GDEXTENSION_CALL_ERROR_INVALID_METHOD) {
        err_text = "Method not found";
    } else if (r_error.error == GDEXTENSION_CALL_ERROR_INSTANCE_IS_NULL) {
        err_text = "Instance is null";
    } else if (r_error.error == GDEXTENSION_CALL_ERROR_METHOD_NOT_CONST) {
        err_text = "Method not const in const instance";
    } else if (r_error.error == GDEXTENSION_CALL_OK) {
        return "Call OK";
    }

    String base_text;
    if (p_base) {
        base_text = p_base->get_class();
        Ref<Resource> script = p_base->get_script();
        if (script.is_valid() && ResourceUtils::is_file(script->get_path())) {
            base_text += "(" + script->get_path().get_file() + ")";
        }
        base_text += "::";
    }
    return "'" + base_text + String(p_method) + "': " + err_text;
}
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
#include "api/extension_db.h"

#include "core/godot/gdextension_compat.h"

#include <godot_cpp/classes/json.hpp>

#define REGISTER_MATH_CONSTANT(m_name, m_type, m_value) {           \
        math_constants[m_name] = { m_name, m_type, m_value };       \
        math_constant_names.push_back(m_name);                      \
    }

#define REGISTER_OPERATOR(m_code, m_name, m_type) {                 \
        operator_names[m_code] = m_name;                            \
        operator_codes[m_code] = m_type;                            \
    }

ExtensionDB* ExtensionDB::_singleton = nullptr;

Vector<MethodInfo> BuiltInType::get_method_list() const {
    Vector<MethodInfo> method_list;
    for (const KeyValue<StringName, FunctionInfo>& E : methods) {
        method_list.push_back(E.value.method);
    }
    return method_list;
}

String ExtensionDB::_resolve_enum_prefix(const Vector<EnumValue>& p_enum_values) {
    if (p_enum_values.size() == 0) {
        return {};
    }

    String prefix = p_enum_values[0].name;
    // Some Godot enums are prefixed with a trailing underscore, those are our target.
    if (!prefix.contains("_")) {
        return {};
    }

    for (const EnumValue& value : p_enum_values) {
        while (value.name.find(prefix) != 0) {
            prefix = prefix.substr(0, prefix.length() - 1);
            if (prefix.is_empty()) {
                return {};
            }
        }
    }
    return prefix;
}

bool ExtensionDB::_is_enum_values_upper_cased(const EnumInfo& p_enumeration) {
    return p_enumeration.name.match("EulerOrder");
}

void ExtensionDB::_sanitize(EnumInfo& p_enumeration) {
    const bool is_key = p_enumeration.name.match("Key");
    const bool is_error = p_enumeration.name.match("Error");
    const bool is_method_flags = p_enumeration.name.match("MethodFlags");
    const bool is_upper = _is_enum_values_upper_cased(p_enumeration);

    const String prefix = _resolve_enum_prefix(p_enumeration.values);
    for (EnumValue& value : p_enumeration.values) {
        value.friendly_name = value.name.replace(prefix, "").capitalize();

        // Handle unique fix-ups for enum friendly names
        if (is_key && value.friendly_name.begins_with("Kp ")) {
            value.friendly_name = value.friendly_name.substr(3, value.friendly_name.length()) + " (Keypad)";
        }
        else if(is_key && value.friendly_name.begins_with("F ")) {
            value.friendly_name = value.friendly_name.replace(" ", "");
        }
        else if (is_error && value.friendly_name.begins_with("Err ")) {
            value.friendly_name = value.friendly_name.substr(4, value.friendly_name.length());
        }
        else if (is_method_flags && value.name.match("METHOD_FLAGS_DEFAULT")) {
            value.friendly_name = ""; // forces it to be skipped by some nodes (same as normal)
        }
        if (is_upper) {
            value.friendly_name = value.friendly_name.to_upper();
        }
    }
}

Variant::Type ExtensionDB::_resolve_variant_type_from_name(const String& p_name) {
    const Variant::Type* type = variant_name_to_type.getptr(p_name);
    return type ? *type : Variant::NIL;
}

String ExtensionDB::_resolve_operator_name(const String& p_name) {
    const StringName* name = operator_names.getptr(p_name);
    return name ? *name : "Unknown";
}

VariantOperators::Code ExtensionDB::_resolve_operator_type(const String& p_name) {
    const VariantOperators::Code* code = operator_codes.getptr(p_name);
    return code ? *code : VariantOperators::OP_ADD;
}

int32_t ExtensionDB::_resolve_method_flags(const Dictionary& p_method) {
    int32_t flags = METHOD_FLAG_NORMAL;
    if (p_method.get("is_const", false)) {
        flags |= METHOD_FLAG_CONST;
    }
    if (p_method.get("is_static", false)) {
        flags |= METHOD_FLAG_STATIC;
    }
    if (p_method.get("is_vararg", false)) {
        flags |= METHOD_FLAG_VARARG;
    }
    if (p_method.get("is_required", false)) {
        flags |= METHOD_FLAG_VIRTUAL_REQUIRED;
    }
    if (p_method.get("is_virtual", false)) {
        flags |= METHOD_FLAG_VIRTUAL;
    }
    return flags;
}

PropertyInfo ExtensionDB::_resolve_type_to_property(const String& p_type, const String& p_name) {
    if (p_type.begins_with("enum::")) {
        const PackedStringArray parts = p_type.split("::", false);
        if (parts[1].find(".") != -1) {
            return { Variant::INT, p_name, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_ENUM, parts[1] };
        } else {
            return { Variant::INT, p_name, PROPERTY_HINT_ENUM, parts[1] };
        }
    }

    if (p_type.begins_with("bitfield::")) {
        const PackedStringArray parts = p_type.split("::", false);
        if (parts[1].find(".") != -1) {
            return { Variant::INT, p_name, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_BITFIELD, parts[1] };
        } else {
            return { Variant::INT, p_name, PROPERTY_HINT_FLAGS, parts[1] };
        }
    }

    if (p_type.begins_with("typedarray::")) {
        const PackedStringArray parts = p_type.split("::", false);
        return { Variant::ARRAY, p_name, PROPERTY_HINT_ARRAY_TYPE, parts[1], PROPERTY_USAGE_DEFAULT };
    }

    if (p_type == "Variant") {
        return { Variant::NIL, p_name, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT };
    }

    if (p_type.is_empty()) {
        return { Variant::NIL, p_name };
    }

    const Variant::Type* type = variant_name_to_type.getptr(p_type);
    if (type) {
        return { *type, p_name };
    }

    // Class type
    return { Variant::OBJECT, p_name, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT, p_type };
}

PropertyInfo ExtensionDB::_resolve_method_return(const Dictionary& p_method) {
    Dictionary return_value = p_method.get("return_value", Dictionary());

    String return_type = return_value.get("type", "");
    if (return_type.is_empty()) {
        return_type = p_method.get("return_type", "");
    }

    return _resolve_type_to_property(return_type);
}

PropertyInfo ExtensionDB::_resolve_method_argument(const Dictionary& p_argument) {
    return _resolve_type_to_property(p_argument["type"], p_argument["name"]);
}

Variant ExtensionDB::_resolve_method_argument_default(const Dictionary& p_argument) {
    const String default_value = p_argument.get("default_value", "");
    if (default_value == "[]") {
        return Array();
    }

    if (default_value == "{}") {
        return Dictionary();
    }

    if (default_value == "null") {
        return {};
    }

    if (default_value.begins_with("Array[") && default_value.ends_with("]([])")) {
        int64_t start_pos = default_value.find("[") + 1;
        int64_t end_pos = default_value.find("]");
        Array array;
        array.set_typed(Variant::OBJECT, default_value.substr(start_pos, end_pos - start_pos), Variant());
        return array;
    }

    return UtilityFunctions::str_to_var(default_value);
}

void ExtensionDB::_load(const PackedByteArray& p_data) {
    const Dictionary api_data = JSON::parse_string(p_data.get_string_from_utf8());
    ERR_FAIL_COND_MSG(api_data.is_empty(), "Failed to load Orchestrator API data.");

    // Prime variant name to type mappings
    for (uint32_t i = 0; i < Variant::VARIANT_MAX; i++) {
        Variant::Type type = static_cast<Variant::Type>(i);
        variant_name_to_type[Variant::get_type_name(type)] = type;
    }
    variant_name_to_type["Variant"] = Variant::NIL;

    REGISTER_OPERATOR("==", "Equal", VariantOperators::OP_EQUAL);
    REGISTER_OPERATOR("!=", "Not Equal", VariantOperators::OP_NOT_EQUAL);
    REGISTER_OPERATOR("<", "Less-than", VariantOperators::OP_LESS);
    REGISTER_OPERATOR("<=", "Less-than or Equal", VariantOperators::OP_LESS_EQUAL);
    REGISTER_OPERATOR(">", "Greater-than", VariantOperators::OP_GREATER);
    REGISTER_OPERATOR(">=", "Greater-than or Equal", VariantOperators::OP_GREATER_EQUAL);
    REGISTER_OPERATOR("+", "Addition", VariantOperators::OP_ADD);
    REGISTER_OPERATOR("-", "Subtract", VariantOperators::OP_SUBTRACT);
    REGISTER_OPERATOR("*", "Multiply", VariantOperators::OP_MULTIPLY);
    REGISTER_OPERATOR("/", "Division", VariantOperators::OP_DIVIDE);
    REGISTER_OPERATOR("unary-", "Unary- or Negate", VariantOperators::OP_NEGATE);
    REGISTER_OPERATOR("unary+", "Unary+", VariantOperators::OP_POSITIVE);
    REGISTER_OPERATOR("%", "Modulo", VariantOperators::OP_MODULE);
    REGISTER_OPERATOR("**", "Power", VariantOperators::OP_POWER);
    REGISTER_OPERATOR("<<", "Shift Left", VariantOperators::OP_SHIFT_LEFT);
    REGISTER_OPERATOR(">>", "Shift Right", VariantOperators::OP_SHIFT_RIGHT);
    REGISTER_OPERATOR("&", "Bitwise And", VariantOperators::OP_BIT_AND);
    REGISTER_OPERATOR("|", "Bitwise Or", VariantOperators::OP_BIT_OR);
    REGISTER_OPERATOR("^", "Bitwise Xor", VariantOperators::OP_BIT_XOR);
    REGISTER_OPERATOR("~", "Bitwise Negate", VariantOperators::OP_BIT_NEGATE);
    REGISTER_OPERATOR("and", "And", VariantOperators::OP_AND);
    REGISTER_OPERATOR("or", "Or", VariantOperators::OP_OR);
    REGISTER_OPERATOR("xor", "Xor", VariantOperators::OP_XOR);
    REGISTER_OPERATOR("not", "Not", VariantOperators::OP_NOT);
    REGISTER_OPERATOR("in", "In", VariantOperators::OP_IN);

    REGISTER_MATH_CONSTANT("One", Variant::FLOAT, 1.0);
    REGISTER_MATH_CONSTANT("PI", Variant::FLOAT, Math_PI);
    REGISTER_MATH_CONSTANT("PI/2", Variant::FLOAT, Math_PI * 0.5);
    REGISTER_MATH_CONSTANT("LN(2)", Variant::FLOAT, Math_LN2);
    REGISTER_MATH_CONSTANT("TAU", Variant::FLOAT, Math_TAU);
    REGISTER_MATH_CONSTANT("E", Variant::FLOAT, Math_E);
    REGISTER_MATH_CONSTANT("Sqrt1/2", Variant::FLOAT, Math_SQRT12);
    REGISTER_MATH_CONSTANT("Sqrt2", Variant::FLOAT, Math_SQRT2);
    REGISTER_MATH_CONSTANT("INF", Variant::FLOAT, Math_INF);
    REGISTER_MATH_CONSTANT("NAN", Variant::FLOAT, Math_NAN);

    _load_builtin_types(api_data);
    _load_global_enumerations(api_data);
    _load_utility_functions(api_data);
    _load_classes(api_data);
}

void ExtensionDB::_load_builtin_types(const Dictionary& p_data) {
    Array types = p_data.get("builtin_classes", Array());
    for (uint32_t i = 0; i < types.size(); i++) {
        const Dictionary& type_data = types[i];

        BuiltInType type;
        type.name = type_data["name"];
        type.type = _resolve_variant_type_from_name(type.name);
        type.keyed = type_data.get("is_keyed", false);
        type.has_destructor = type_data.get("has_destructor", false);
        type.index_returning_type = _resolve_variant_type_from_name(type_data.get("indexing_return_type", "Nil"));

        const Array operators = type_data.get("operators", Array());
        for (uint32_t j = 0; j < operators.size(); j++) {
            const Dictionary& data = operators[j];
            const String op_name = data["name"];

            OperatorInfo oi;
            oi.name = _resolve_operator_name(op_name);
            oi.op = _resolve_operator_type(op_name);
            oi.code = op_name;
            oi.left_type = type.type;
            oi.left_type_name = type.name;
            oi.right_type = _resolve_variant_type_from_name(data.get("right_type", "Nil"));
            oi.right_type_name = data.get("right_type", "");
            oi.return_type = _resolve_variant_type_from_name(data["return_type"]);

            type.operators.push_back(oi);
        }

        const Array ctors = type_data.get("constructors", Array());
        for (uint32_t j = 0; j < ctors.size(); j++) {
            const Dictionary& data = ctors[j];

            ConstructorInfo ci;
            const Array arguments = data.get("arguments", Array());
            for (uint32_t k = 0; k < arguments.size(); k++) {
                const Dictionary& arg_data = arguments[k];

                PropertyInfo argument;
                argument.type = _resolve_variant_type_from_name(arg_data["type"]);
                argument.name = arg_data["name"];
                ci.arguments.push_back(argument);
            }

            type.constructors.push_back(ci);
        }

        const Array members = type_data.get("members", Array());
        for (uint32_t j = 0; j < members.size(); j++) {
            const Dictionary& data = members[j];

            PropertyInfo pi;
            pi.type = _resolve_variant_type_from_name(data["type"]);
            pi.name = data["name"];

            type.properties.push_back(pi);
        }

        const Array constants = type_data.get("constants", Array());
        for (uint32_t j = 0; j < constants.size(); j++) {
            const Dictionary& data = constants[j];

            ConstantInfo ci;
            ci.name = data["name"];
            ci.type = _resolve_variant_type_from_name(data["type"]);
            ci.value = UtilityFunctions::str_to_var(data["value"]);
            type.constants.push_back(ci);
        }

        const Array enumerations = type_data.get("enums", Array());
        for (uint32_t j = 0; j < enumerations.size(); j++) {
            const Dictionary& data = enumerations[j];

            EnumInfo ei;
            ei.is_bitfield = data.get("is_bitfield", false);

            const Array values = data.get("values", Array());
            for (uint32_t k = 0; k < values.size(); k++) {
                const Dictionary& value = values[k];

                EnumValue ev;
                ev.name = value["name"];
                ev.value = value["value"];

                ei.values.push_back(ev);
            }

            _sanitize(ei);
            type.enums.push_back(ei);
        }

        const Array methods = type_data.get("methods", Array());
        for (uint32_t j = 0; j < methods.size(); j++) {
            const Dictionary& data = methods[j];

            FunctionInfo fi;
            fi.method.name = data["name"];
            fi.method.flags = _resolve_method_flags(data);
            fi.method.return_val = _resolve_method_return(data);
            fi.hash = data["hash"];

            const Array args = data.get("arguments", Array());
            for (uint32_t k = 0; k < args.size(); k++) {
                fi.method.arguments.push_back(_resolve_method_argument(args[k]));
            }

            type.methods[fi.method.name] = fi;
        }

        builtin_types[type.name] = type;
        builtin_types_to_name[type.type] = type.name;
    }
}

void ExtensionDB::_load_global_enumerations(const Dictionary& p_data) {
    const Array global_enumerations = p_data.get("global_enums", Array());
    for (uint32_t i = 0; i < global_enumerations.size(); i++) {
        const Dictionary& data = global_enumerations[i];

        EnumInfo ei;
        ei.name = data["name"];
        ei.is_bitfield = data.get("is_bitfield", false);

        const Array values = data.get("values", Array());
        for (uint32_t j = 0; j < values.size(); j++) {
            const Dictionary& value = values[j];

            EnumValue ev;
            ev.name = value["name"];
            ev.value = value["value"];
            ei.values.push_back(ev);
        }

        _sanitize(ei);

        global_enums[ei.name] = ei;
        global_enum_names.push_back(ei.name);
        for (const EnumValue& ev : ei.values) {
            global_enum_value_names.push_back(ev.name);
        }
    }
}

void ExtensionDB::_load_utility_functions(const Dictionary& p_data) {
    const Array funcs = p_data.get("utility_functions", Array());
    for (uint32_t i = 0; i < funcs.size(); i++) {
        const Dictionary& data = funcs[i];

        FunctionInfo fi;
        fi.method.name = data["name"];
        fi.method.flags = _resolve_method_flags(data);
        fi.method.return_val = _resolve_method_return(data);
        fi.category = data["category"];
        fi.hash = data["hash"];

        const Array args = data.get("arguments", Array());
        for (uint32_t j = 0; j < args.size(); j++) {
            fi.method.arguments.push_back(_resolve_method_argument(args[j]));
        }

        utility_functions[fi.method.name] = fi;
    }
}

void ExtensionDB::_load_classes(const Dictionary& p_data) {
    const Array api_classes = p_data.get("classes", Array());
    for (uint32_t i = 0; i < api_classes.size(); i++) {
        const Dictionary& data = api_classes[i];

        ClassInfo ci;
        ci.name = data["name"];
        ci.ref_counted = data.get("is_refcounted", false);
        ci.instantiable = data.get("is_instantiable", false);
        ci.parent_class = data.get("inherits", "");
        ci.api_type = data.get("api_type", "");
        ci.brief_description = data.get("brief_description", "");
        ci.description = data.get("description", "");

        const Array methods = data.get("methods", Array());
        for (uint32_t j = 0; j < methods.size(); j++) {
            const Dictionary& method_data = methods[j];

            ClassMethodInfo cmi;
            cmi.method.name = method_data["name"];
            cmi.method.flags = _resolve_method_flags(method_data);
            cmi.method.return_val = _resolve_method_return(method_data);
            cmi.hash = method_data["hash"];
            cmi.description = method_data.get("description", "");

            const Array args = method_data.get("arguments", Array());
            for (uint32_t k = 0; k < args.size(); k++) {
                const Dictionary& arg_data = args[k];
                cmi.method.arguments.push_back(_resolve_method_argument(arg_data));
                if (arg_data.has("default_value")) {
                    cmi.method.default_arguments.push_back(_resolve_method_argument_default(arg_data));
                }
            }

            ci.methods[cmi.method.name] = cmi;
        }

        const Array properties = data.get("properties", Array());
        for (uint32_t j = 0; j < properties.size(); j++) {
            const Dictionary& prop_data = properties[j];

            ClassPropertyInfo cpi;
            cpi.property = _resolve_method_argument(prop_data);
            cpi.getter = prop_data.get("getter", "");
            cpi.setter = prop_data.get("setter", "");
            cpi.description = prop_data.get("description", "");

            ci.properties[cpi.property.name] = cpi;
        }

        const Array signals = data.get("signals", Array());
        for (uint32_t j = 0; j < signals.size(); j++) {
            const Dictionary& signal_data = signals[j];

            ClassSignalInfo csi;
            csi.method.name = signal_data["name"];
            csi.description = signal_data.get("description", "");
            const Array args = signal_data.get("arguments", Array());
            for (uint32_t k = 0; k < args.size(); k++) {
                const Dictionary& arg_data = args[k];
                csi.method.arguments.push_back(_resolve_method_argument(arg_data));
            }

            ci.signals[csi.method.name] = csi;
        }

        const Array class_enums = data.get("enums", Array());
        for (uint32_t j = 0; j < class_enums.size(); j++) {
            const Dictionary& enum_data = class_enums[j];
            if (enum_data.get("is_bitfield", false)) {
                ci.bitfield_enums.push_back(enum_data["name"]);
            }
        }

        classes[ci.name] = ci;
    }

    // Iterate classes and link inheritance pointers
    for (KeyValue<StringName, ClassInfo>& E : classes) {
        if (!E.value.parent_class.is_empty()) {
            E.value.parent = classes.getptr(E.value.parent_class);
        }
    }
}

bool ExtensionDB::is_builtin_type(const StringName& p_type_name) {
    return _singleton->builtin_types.has(p_type_name);
}

Vector<BuiltInType> ExtensionDB::get_builtin_types() {
    Vector<BuiltInType> types;
    for (const KeyValue<StringName, BuiltInType>& E : _singleton->builtin_types) {
        types.push_back(E.value);
    }
    return types;
}

BuiltInType ExtensionDB::get_builtin_type(const StringName& p_type_name) {
    return _singleton->builtin_types[p_type_name];
}

BuiltInType ExtensionDB::get_builtin_type(Variant::Type p_type) {
    return _singleton->builtin_types[_singleton->builtin_types_to_name[p_type]];
}

PackedStringArray ExtensionDB::get_global_enum_names() {
    return _singleton->global_enum_names;
}

PackedStringArray ExtensionDB::get_global_enum_value_names() {
    return _singleton->global_enum_value_names;
}

EnumInfo ExtensionDB::get_global_enum(const StringName& p_enum_name) {
    return _singleton->global_enums[p_enum_name];
}

EnumInfo ExtensionDB::get_global_enum_by_value(const StringName& p_enum_name) {
    for (const KeyValue<StringName, EnumInfo>& E : _singleton->global_enums) {
        for (const EnumValue& value : E.value.values) {
            if (value.name == p_enum_name) {
                return E.value;
            }
        }
    }
    return {};
}

EnumValue ExtensionDB::get_global_enum_value(const StringName& p_enum_value_name) {
    for (const KeyValue<StringName, EnumInfo>& E : _singleton->global_enums) {
        for (const EnumValue& value : E.value.values) {
            if (value.name == p_enum_value_name) {
                return value;
            }
        }
    }
    return {};
}

PackedStringArray ExtensionDB::get_math_constant_names() {
    return _singleton->math_constant_names;
}

ConstantInfo ExtensionDB::get_math_constant(const StringName& p_constant_name) {
    return _singleton->math_constants[p_constant_name];
}

bool ExtensionDB::is_utility_function(const StringName& p_method_name) {
    return _singleton->utility_functions.has(p_method_name);
}

Vector<FunctionInfo> ExtensionDB::get_utility_functions() {
    Vector<FunctionInfo> functions;
    for (const KeyValue<StringName, FunctionInfo>& E : _singleton->utility_functions) {
        functions.push_back(E.value);
    }
    return functions;
}

FunctionInfo ExtensionDB::get_utility_function(const StringName& p_name) {
    return _singleton->utility_functions[p_name];
}

bool ExtensionDB::is_class_enum_bitfield(const StringName& p_class_name, const StringName& p_enum_name) {
    if (_singleton->classes.has(p_class_name)) {
        return _singleton->classes[p_class_name].bitfield_enums.has(p_enum_name);
    }
    return false;
}

PackedStringArray ExtensionDB::get_class_static_function_names(const StringName& p_class_name) {
    const ClassInfo* clazz = _singleton->classes.getptr(p_class_name);
    if (!clazz) {
        return {};
    }

    PackedStringArray names;
    for (const KeyValue<StringName, ClassMethodInfo>& E : clazz->methods) {
        if (E.value.method.flags & METHOD_FLAG_STATIC) {
            names.push_back(E.key);
        }
    }
    return names;
}

bool ExtensionDB::get_class_method_info(const StringName& p_class_name, const StringName& p_method_name, MethodInfo& r_info, bool p_no_inheritance) {
    const ClassInfo* clazz = _singleton->classes.getptr(p_class_name);
    while (clazz) {
        const ClassMethodInfo* method = clazz->methods.getptr(p_method_name);
        if (method) {
            r_info = method->method;
            return true;
        }

        if (p_no_inheritance) {
            break;
        }

        clazz = clazz->parent;
    }

    if (ClassDB::class_has_method(p_class_name, p_method_name, p_no_inheritance)) {
        ERR_PRINT(vformat("Bug: ExtensionDB failed to locate %s.%s, but ClassDB says it exists.", p_class_name, p_method_name));
    }

    return false;
}

MethodBind* ExtensionDB::get_method(const StringName& p_class_name, const StringName& p_method_name) {
    const ClassInfo* clazz = _singleton->classes.getptr(p_class_name);
    while (clazz) {
        const ClassMethodInfo* method = clazz->methods.getptr(p_method_name);
        if (method) {
            return const_cast<MethodBind*>(static_cast<const MethodBind*>(
                GDE_INTERFACE(classdb_get_method_bind)(
                    p_class_name._native_ptr(), p_method_name._native_ptr(), method->hash)));
        }
        clazz = clazz->parent;
    }
    return nullptr;
}

ExtensionDB::ExtensionDB() {
    _singleton = this;
    _decompress_and_load();
}

ExtensionDB::~ExtensionDB() {
    _singleton = nullptr;
}
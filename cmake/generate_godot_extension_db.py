## This file is part of the Godot Orchestrator project.
##
## Copyright (c) 2023-present Crater Crash Studios LLC and its contributors.
##
## Licensed under the Apache License, Version 2.0 (the "License");
## you may not use this file except in compliance with the License.
## You may obtain a copy of the License at
##
##		http://www.apache.org/licenses/LICENSE-2.0
##
## Unless required by applicable law or agreed to in writing, software
## distributed under the License is distributed on an "AS IS" BASIS,
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
## See the License for the specific language governing permissions and
## limitations under the License.
##

import json
import sys

# Global variables
NS = "godot"
DB = "ExtensionDB::_singleton->"
INDENT = 0


def get_variant_type(variant_type):
    if variant_type == "Variant" or variant_type == "Nil":
        return "Variant::NIL"
    elif variant_type == "bool":
        return "Variant::BOOL"
    elif variant_type == "int":
        return "Variant::INT"
    elif variant_type == "float":
        return "Variant::FLOAT"
    elif variant_type == "Plane":
        return "Variant::PLANE"
    elif variant_type == "AABB":
        return "Variant::AABB"
    elif variant_type == "Quaternion":
        return "Variant::QUATERNION"
    elif variant_type == "Basis":
        return "Variant::BASIS"
    elif variant_type == "Projection":
        return "Variant::PROJECTION"
    elif variant_type == "Transform2D":
        return "Variant::TRANSFORM2D"
    elif variant_type == "Transform3D":
        return "Variant::TRANSFORM3D"
    elif variant_type == "Rect2":
        return "Variant::RECT2"
    elif variant_type == "Rect2i":
        return "Variant::RECT2I"
    elif variant_type == "Vector2":
        return "Variant::VECTOR2"
    elif variant_type == "Vector2i":
        return "Variant::VECTOR2I"
    elif variant_type == "Vector3":
        return "Variant::VECTOR3"
    elif variant_type == "Vector3i":
        return "Variant::VECTOR3I"
    elif variant_type == "Vector4":
        return "Variant::VECTOR4"
    elif variant_type == "Vector4i":
        return "Variant::VECTOR4I"
    elif variant_type == "Color":
        return "Variant::COLOR"
    elif variant_type == "NodePath":
        return "Variant::NODE_PATH"
    elif variant_type == "String":
        return "Variant::STRING"
    elif variant_type == "Array":
        return "Variant::ARRAY"
    elif variant_type == "Dictionary":
        return "Variant::DICTIONARY"
    elif variant_type == "StringName":
        return "Variant::STRING_NAME"
    elif variant_type == "RID":
        return "Variant::RID"
    elif variant_type == "Callable":
        return "Variant::CALLABLE"
    elif variant_type == "Signal":
        return "Variant::SIGNAL"
    elif variant_type == "Object":
        return "Variant::OBJECT"
    elif variant_type == "PackedByteArray":
        return "Variant::PACKED_BYTE_ARRAY"
    elif variant_type == "PackedInt32Array":
        return "Variant::PACKED_INT32_ARRAY"
    elif variant_type == "PackedInt64Array":
        return "Variant::PACKED_INT64_ARRAY"
    elif variant_type == "PackedFloat32Array":
        return "Variant::PACKED_FLOAT32_ARRAY"
    elif variant_type == "PackedFloat64Array":
        return "Variant::PACKED_FLOAT64_ARRAY"
    elif variant_type == "PackedStringArray":
        return "Variant::PACKED_STRING_ARRAY"
    elif variant_type == "PackedVector2Array":
        return "Variant::PACKED_VECTOR2_ARRAY"
    elif variant_type == "PackedVector3Array":
        return "Variant::PACKED_VECTOR3_ARRAY"
    elif variant_type == "PackedColorArray":
        return "Variant::PACKED_COLOR_ARRAY"
    return variant_type

def get_operator_type(op_type):
    if op_type == "==":
        return "VariantOperators::OP_EQUAL"
    elif op_type == "!=":
        return "VariantOperators::OP_NOT_EQUAL"
    elif op_type == "<":
        return "VariantOperators::OP_LESS"
    elif op_type == "<=":
        return "VariantOperators::OP_LESS_EQUAL"
    elif op_type == ">":
        return "VariantOperators::OP_GREATER"
    elif op_type == ">=":
        return "VariantOperators::OP_GREATER_EQUAL"
    elif op_type == "+":
        return "VariantOperators::OP_ADD"
    elif op_type == "-":
        return "VariantOperators::OP_SUBTRACT"
    elif op_type == "*":
        return "VariantOperators::OP_MULTIPLY"
    elif op_type == "/":
        return "VariantOperators::OP_DIVIDE"
    elif op_type == "unary-":
        return "VariantOperators::OP_NEGATE"
    elif op_type == "unary+":
        return "VariantOperators::OP_POSITIVE"
    elif op_type == "%":
        return "VariantOperators::OP_MODULE"
    # Start where GodotCPP and Godot Engine differ
    elif op_type == "**":
        return "VariantOperators::OP_POWER"
    elif op_type == "<<":
        return "VariantOperators::OP_SHIFT_LEFT"
    elif op_type == ">>":
        return "VariantOperators::OP_SHIFT_RIGHT"
    elif op_type == "&":
        return "VariantOperators::OP_BIT_AND"
    elif op_type == "|":
        return "VariantOperators::OP_BIT_OR"
    elif op_type == "^":
        return "VariantOperators::OP_BIT_XOR"
    elif op_type == "~":
        return "VariantOperators::OP_BIT_NEGATE"
    elif op_type == "and":
        return "VariantOperators::OP_AND"
    elif op_type == "or":
        return "VariantOperators::OP_OR"
    elif op_type == "xor":
        return "VariantOperators::OP_XOR"
    elif op_type == "not":
        return "VariantOperators::OP_NOT"
    elif op_type == "in":
        return "VariantOperators::OP_IN"
    # Should never happen
    return "VariantOperators::OP_MAX"


def get_operator_name(op_type):
    if op_type == "==":
        return "Equal"
    elif op_type == "!=":
        return "Not Equal"
    elif op_type == "<":
        return "Less-than"
    elif op_type == "<=":
        return "Less-than or Equal"
    elif op_type == ">":
        return "Greater-than"
    elif op_type == ">=":
        return "Greater-than or Equal"
    elif op_type == "+":
        return "Addition"
    elif op_type == "-":
        return "Subtract"
    elif op_type == "*":
        return "Multiply"
    elif op_type == "/":
        return "Division"
    elif op_type == "unary-":
        return "Unary- or Negate"
    elif op_type == "unary+":
        return "Unary+"
    elif op_type == "%":
        return "Module"
    elif op_type == "**":
        return "Power"
    elif op_type == "<<":
        return "Shift Left"
    elif op_type == ">>":
        return "Shift Right"
    elif op_type == "&":
        return "Bitwise And"
    elif op_type == "|":
        return "Bitwise Or"
    elif op_type == "^":
        return "Bitwise Xor"
    elif op_type == "~":
        return "Bitwise Negate"
    elif op_type == "and":
        return "And"
    elif op_type == "or":
        return "Or"
    elif op_type == "xor":
        return "Xor"
    elif op_type == "not":
        return "Not"
    elif op_type == "in":
        return "In"
    return op_type

def indent_push():
    global INDENT
    INDENT = INDENT + 1


def indent_pop():
    global INDENT
    INDENT = INDENT - 1


def sanitize_constant(cname, cvalue):
    if cvalue.startswith("Vector"):
        cvalue = cvalue.replace("inf", "INFINITY")
    elif cvalue.startswith("Projection"):
        if cname == "IDENTITY":
            cvalue = "Projection(Vector4(1, 0, 0, 0), Vector4(0, 1, 0, 0), Vector4(0, 0, 1, 0), Vector4(0, 0, 0, 1))"
        elif cname == "ZERO":
            cvalue = "Projection(Vector4(0, 0, 0, 0), Vector4(0, 0, 0, 0), Vector4(0, 0, 0, 0), Vector4(0, 0, 0, 0))"
    return cvalue


def quote(text):
    return "\"" + text + "\""


def print_indent(text):
    tab_str = '\t' * INDENT
    print(tab_str + text)


def print_struct(name, elements, methods = None):
    print_indent("struct " + name)
    print_indent("{")
    indent_push()
    for element in elements:
        print_indent(element + ";")
    if not methods is None:
        print_indent("")
        for method in methods:
            print_indent(method + ";")
    indent_pop()
    print_indent("};")
    print_indent("")


# Creates all the struct data types
def create_structs():
    indent_push()

    print_indent("/// Describes a mapping between an enum name and value")
    print_struct("EnumValue", ["StringName name",
                               "StringName friendly_name",
                               "int value{ 0 }"])

    print_indent("/// Describes a definition of an Enumeration type")
    print_struct("EnumInfo", ["StringName name",
                              "bool is_bitfield{ false }",
                              "Vector<EnumValue> values"])

    print_indent("/// Describes a function")
    print_struct("FunctionInfo", ["StringName name",
                                  "PropertyInfo return_val",
                                  "StringName category",
                                  "bool is_vararg{ false }",
                                  "Vector<PropertyInfo> arguments"])

    print_indent("/// Describes an operator for a Godot type")
    print_struct("OperatorInfo", ["VariantOperators::Code op{ VariantOperators::OP_EQUAL }",
                                  "StringName code",
                                  "StringName name",
                                  "Variant::Type left_type{ Variant::NIL }",
                                  "StringName left_type_name",
                                  "Variant::Type right_type{ Variant::NIL }",
                                  "StringName right_type_name",
                                  "Variant::Type return_type{ Variant::NIL }"])

    print_indent("/// Describes a constructor definition")
    print_struct("ConstructorInfo", ["Vector<PropertyInfo> arguments"])

    print_indent("/// Describes a Constant definition")
    print_struct("ConstantInfo", ["StringName name",
                                  "Variant::Type type{ Variant::NIL }",
                                  "Variant value"])

    print_indent("/// Builtin Godot Type details")
    print_struct("BuiltInType", ["StringName name",
                                 "Variant::Type type{ Variant::NIL }",
                                 "bool keyed{ false }",
                                 "bool has_destructor{ false }",
                                 "Vector<OperatorInfo> operators",
                                 "Vector<ConstructorInfo> constructors",
                                 "Vector<MethodInfo> methods",
                                 "Vector<PropertyInfo> properties",
                                 "Vector<ConstantInfo> constants",
                                 "Vector<EnumInfo> enums",
                                 "Variant::Type index_returning_type{ Variant::NIL }"])

    print_indent("/// Describes a Godot Class")
    print_struct("ClassInfo", ["StringName name",
                               "Vector<StringName> bitfield_enums",
                               "HashMap<StringName, int64_t> static_function_hashes"])
    indent_pop()


def create_loader_header():
    # NS::internal::ExtensionDBBuilder
    print_indent("namespace internal")
    print_indent("{")
    indent_push()
    print_indent("/// Populates the contents of the ExtensionDB singleton database")
    print_indent("class ExtensionDBLoader")
    print_indent("{")
    indent_push()
    print_indent("/// Populates Math Constants")
    print_indent("void prime_math_constants();")
    print_indent("")
    print_indent("/// Populates Global Enumerations")
    print_indent("void prime_global_enumerations();")
    print_indent("")
    print_indent("/// Populates Builtin Data Types")
    print_indent("void prime_builtin_classes();")
    print_indent("")
    print_indent("/// Populates Utility Functions")
    print_indent("void prime_utility_functions();")
    print_indent("")
    print_indent("/// Populate class details")
    print_indent("void prime_class_details();")
    indent_pop()
    print_indent("")
    print_indent("public:")
    indent_push()
    print_indent("/// Populates the ExtensionDB")
    print_indent("void prime();")
    indent_pop()
    print_indent("};")
    indent_pop()
    print_indent("}")
    print_indent("")


def create_db_header():
    # NS::ExtensionDB
    print_indent("/// A simple database that exposes GDExtension and Godot details")
    print_indent("/// This is intended to supplement ClassDB, which does not expose all details to GDExtension.")
    print_indent("class ExtensionDB")
    print_indent("{")
    indent_push()
    print_indent("friend class internal::ExtensionDBLoader;")
    print_indent("static ExtensionDB* _singleton;")
    print_indent("")
    print_indent("HashMap<Variant::Type, StringName> _builtin_types_to_name;")
    print_indent("HashMap<StringName, BuiltInType> _builtin_types;")
    print_indent("PackedStringArray _builtin_type_names;")
    print_indent("")
    print_indent("PackedStringArray _global_enum_names;")
    print_indent("PackedStringArray _global_enum_value_names;")
    print_indent("HashMap<StringName, EnumInfo> _global_enums;")
    print_indent("")
    print_indent("PackedStringArray _math_constant_names;")
    print_indent("HashMap<StringName, ConstantInfo> _math_constants;")
    print_indent("")
    print_indent("PackedStringArray _function_names;")
    print_indent("HashMap<StringName, FunctionInfo> _functions;")
    print_indent("")
    print_indent("HashMap<StringName, ClassInfo> _classes;")
    print_indent("")
    indent_pop()
    print_indent("public:")
    indent_push()
    print_indent("ExtensionDB();")
    print_indent("~ExtensionDB();")
    print_indent("")
    print_indent("static PackedStringArray get_builtin_type_names();")
    print_indent("static BuiltInType get_builtin_type(const StringName& p_type_name);")
    print_indent("static BuiltInType get_builtin_type(Variant::Type p_type);")
    print_indent("")
    print_indent("static PackedStringArray get_global_enum_names();")
    print_indent("static PackedStringArray get_global_enum_value_names();")
    print_indent("static EnumInfo get_global_enum(const StringName& p_enum_name);")
    print_indent("static EnumInfo get_global_enum_by_value(const StringName& p_name);")
    print_indent("static EnumValue get_global_enum_value(const StringName& p_enum_value_name);")
    print_indent("")
    print_indent("static PackedStringArray get_math_constant_names();")
    print_indent("static ConstantInfo get_math_constant(const StringName& p_constant_name);")
    print_indent("")
    print_indent("static PackedStringArray get_function_names();")
    print_indent("static FunctionInfo get_function(const StringName& p_name);")
    print_indent("")
    print_indent("static bool is_class_enum_bitfield(const StringName& p_class_name, const String& p_enum_name);")
    print_indent("")
    print_indent("static PackedStringArray get_static_function_names(const StringName& p_class_name);")
    print_indent("static int64_t get_static_function_hash(const StringName& p_class_name, const StringName& p_function_name);")
    indent_pop()
    print_indent("};")
    print_indent("")


def create_hpp():
    # NS
    print_indent("namespace " + NS)
    print_indent("{")
    create_structs()
    indent_push()
    create_loader_header()
    create_db_header()
    indent_pop()
    print_indent("}")


def write_math_constant(name, value):
    qname = quote(name)
    print_indent(DB + "_math_constant_names.push_back(" + qname + ");")
    print_indent(DB + "_math_constants[" + qname + "] = { " + qname + ", Variant::FLOAT, " + value + " };")


# Outputs the Math Constants to the CPP
def write_math_constants():
    indent_push()
    # Math Constants
    print_indent("// Math Constants")
    write_math_constant("One", "1.0")
    write_math_constant("PI", "Math_PI")
    write_math_constant("PI/2", "Math_PI * 0.5")
    write_math_constant("LN(2)", "Math_LN2")
    write_math_constant("TAU", "Math_TAU")
    write_math_constant("E", "Math_E")
    write_math_constant("Sqrt1/2", "Math_SQRT12")
    write_math_constant("Sqrt2", "Math_SQRT2")
    write_math_constant("INF", "Math_INF")
    write_math_constant("NAN", "Math_NAN")
    indent_pop()


def write_global_enums(enums):
    indent_push()
    print_indent("// Global enumerations")
    for enum in enums:
        print_indent("{")
        indent_push()
        print_indent("EnumInfo ei;")
        print_indent("ei.name = " + quote(enum["name"]) + ";")
        print_indent("ei.is_bitfield = " + str(enum["is_bitfield"]).lower() + ";")
        for ev in enum["values"]:
            print_indent("ei.values.push_back({ " + quote(ev["name"]) + ", \"\", " + str(ev["value"]) + " });")
        print_indent("_sanitize_enum(ei);")
        print_indent(DB + "_global_enums[" + quote(enum["name"]) + "] = ei;")
        print_indent(DB + "_global_enum_names.push_back(" + quote(enum["name"]) + ");")
        print_indent("for (const EnumValue& v : ei.values)")
        indent_push()
        print_indent(DB + "_global_enum_value_names.push_back(v.name);")
        indent_pop()
        indent_pop()
        print_indent("}")
    indent_pop()

def write_builtin_type_operators(godot_type):
    if 'operators' in godot_type:
        for operator in godot_type["operators"]:
            op_id = get_operator_type(operator["name"])
            op_name = quote(get_operator_name(operator["name"]))
            op_code = quote(operator["name"])
            op_left_type = get_variant_type(godot_type["name"])
            op_left_type_name = quote(godot_type["name"])
            op_right_type = "Variant::NIL";
            op_return_type = get_variant_type(operator["return_type"])
            op_right_type_name = quote("")
            if 'right_type' in operator:
                op_right_type = get_variant_type(operator["right_type"])
                op_right_type_name = quote(operator["right_type"])
            print_indent("type.operators.push_back({ " + op_id + ", " + op_code + ", " + op_name + ", " +
                         op_left_type + ", " + op_left_type_name + ", " +
                         op_right_type + ", " + op_right_type_name + ", " +
                         op_return_type + " });")


def write_builtin_type_constructors(godot_type):
    if 'constructors' in godot_type:
        for ctor in godot_type["constructors"]:
            args = ""
            if 'arguments' in ctor:
                for arg in ctor["arguments"]:
                    variant_type = get_variant_type(arg["type"])
                    if len(args) > 0:
                        args = args + ", "
                    args = args + "PropertyInfo(" + variant_type + ", " + quote(arg["name"]) + ")"
            print_indent("type.constructors.push_back({ { " + args + " } });")

def write_builtin_type_members(godot_type):
    if 'members' in godot_type:
        for member in godot_type["members"]:
            variant_type = get_variant_type(member["type"])
            print_indent("type.properties.push_back({ " + variant_type + ", " + quote(member["name"]) + " });")


def write_builtin_type_constants(godot_type):
    if 'constants' in godot_type:
        for constant in godot_type["constants"]:
            cn = quote(constant["name"])
            ct = get_variant_type(constant["type"])
            cv = sanitize_constant(constant["name"], constant["value"])
            print_indent("type.constants.push_back({ " + cn + ", " + ct + ", " + cv + " });")


def write_builtin_type_enums(godot_type):
    if 'enums' in godot_type:
        for enum in godot_type["enums"]:
            enum_values = ""
            bitfield = "false"
            if 'is_bitfield' in enum:
                bitfield = str(enum["is_bitfield"]).lower()
            if 'values' in enum:
                for value in enum["values"]:
                    if len(enum_values) > 0:
                        enum_values = enum_values + ", "
                    enum_values = enum_values + "{ " + quote(value["name"]) + ", \"\", " + str(value["value"]) + " }"
            print_indent("type.enums.push_back({ " + quote(enum["name"]) + ", " + bitfield + ", { " + enum_values + " } });")
        print_indent("_sanitize_enums(type.enums);")


def get_method_flags(method):
    flags = "METHOD_FLAG_NORMAL"
    if 'is_const' in method and method["is_const"]:
        flags += " | METHOD_FLAG_CONST"
    if 'is_static' in method and method["is_static"]:
        flags += " | METHOD_FLAG_STATIC"
    if 'is_vararg' in method and method["is_vararg"]:
        flags += " | METHOD_FLAG_VARARG"
    return flags


def get_method_return_type(method):
    if 'return_type' in method:
        return get_variant_type(method["return_type"])
    else:
        return "Variant::NIL"


def write_builtin_type_methods(godot_type):
    if 'methods' in godot_type:
        for method in godot_type["methods"]:
            args = ""
            if 'arguments' in method:
                for arg in method["arguments"]:
                    if len(args) > 0:
                        args += ", "
                    args += "{ " + get_variant_type(arg["type"]) + ", " + quote(arg["name"])
                    if arg["type"] == "Variant":
                        args += ", PROPERTY_HINT_NONE, \"\", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT"
                    args += " }"

            nil_is_variant = ""
            if "return_type" in method and method["return_type"] == "Variant":
                nil_is_variant = ", true"

            print_indent("type.methods.push_back(_make_method(" + quote(method["name"]) + ", " + get_method_flags(method) + ", " + get_method_return_type(method) + ", { " + args + " }" + nil_is_variant + "));")


def write_builtin_types(types):
    indent_push()
    print_indent("// Builtin Data Types")
    for godot_type in types:
        index_return_type = "Variant::NIL"
        if 'indexing_return_type' in godot_type:
            index_return_type = get_variant_type(godot_type["indexing_return_type"])
        print_indent("{")
        indent_push()
        print_indent("BuiltInType type;")
        print_indent("type.name = " + quote(godot_type["name"]) + ";")
        print_indent("type.type = " + get_variant_type(godot_type["name"]) + ";")
        print_indent("type.keyed = " + str(godot_type["is_keyed"]).lower() + ";")
        print_indent("type.has_destructor = " + str(godot_type["has_destructor"]).lower() + ";")
        print_indent("type.index_returning_type = " + index_return_type + ";")
        write_builtin_type_operators(godot_type)
        write_builtin_type_constructors(godot_type)
        write_builtin_type_members(godot_type)
        write_builtin_type_constants(godot_type)
        write_builtin_type_enums(godot_type)
        write_builtin_type_methods(godot_type)
        print_indent(DB + "_builtin_types[" + quote(godot_type["name"]) + "] = type;")
        print_indent(DB + "_builtin_types_to_name[" + get_variant_type(godot_type["name"]) + "] = " + quote(godot_type["name"]) + ";")
        print_indent(DB + "_builtin_type_names.push_back(" + quote(godot_type["name"]) + ");")
        indent_pop()
        print_indent("}")
    indent_pop()


def write_utility_functions(functions):
    indent_push()
    print_indent("// Utility Functions")
    for func in functions:
        print_indent("{")
        indent_push()
        print_indent("FunctionInfo fi;")
        print_indent("fi.name = " + quote(func["name"]) + ";")
        print_indent("fi.category = " + quote(func["category"]) + ";")
        if 'return_type' in func:
            if func["return_type"] == "Variant":
                print_indent("fi.return_val = PropertyInfo(Variant::NIL, \"\", PROPERTY_HINT_NONE, \"\", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT);")
            else:
                print_indent("fi.return_val = PropertyInfo(" + get_variant_type(func["return_type"]) + ", \"\");")
        else:
            print_indent("fi.return_val = PropertyInfo(Variant::NIL, \"\");")
        print_indent("fi.is_vararg = " + str(func["is_vararg"]).lower() + ";")
        if 'arguments' in func:
            for arg in func["arguments"]:
                if arg["type"] == "Variant":
                    print_indent("fi.arguments.push_back({ " + get_variant_type(arg["type"]) + ", " + quote(arg["name"]) + ", PROPERTY_HINT_NONE, \"\", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT });")
                else:
                    print_indent("fi.arguments.push_back({ " + get_variant_type(arg["type"]) + ", " + quote(arg["name"]) + " });")
        print_indent(DB + "_functions[" + quote(func["name"]) + "] = fi;")
        print_indent(DB + "_function_names.push_back(" + quote(func["name"]) + ");")
        indent_pop()
        print_indent("}")
    indent_pop()

def write_class_details(classes):
    indent_push()
    print_indent("// Class details")
    print_indent("// This currently only loads classes that have bitfield enums; use ClassDB otherwise.")
    print_indent("// Can eventually be replaced by: https://github.com/godotengine/godot/pull/90368")
    for clazz in classes:
        class_output = False
        if 'methods' in clazz:
            for method in clazz["methods"]:
                if method["is_static"]:
                    if not class_output:
                        print_indent("")
                        print_indent("// " + clazz["name"])
                        print_indent(DB + "_classes[" + quote(clazz["name"]) + "].name = " + quote(clazz["name"]) + ";")
                        class_output = True
                    print_indent(DB + "_classes[" + quote(clazz["name"]) + "].static_function_hashes[" + quote(method["name"]) + "] = " + str(method["hash"]) + ";")

        enum_names = []
        if 'enums' in clazz:
            for enum in clazz["enums"]:
                if enum["is_bitfield"]:
                    enum_names.append(enum["name"])
        if enum_names:
            if not class_output:
                print_indent("")
                print_indent("// " + clazz["name"])
                print_indent(DB + "_classes[" + quote(clazz["name"]) + "].name = " + quote(clazz["name"]) + ";")
            for enum_name in enum_names:
                print_indent(DB + "_classes[" + quote(clazz["name"]) + "].bitfield_enums.push_back(" + quote(enum_name) + ");")


    indent_pop()

def create_cpp():
    # Load the data
    data = json.load(file)

    indent_push()
    indent_push()

    print_indent("void ExtensionDBLoader::prime_math_constants()")
    print_indent("{")
    write_math_constants()
    print_indent("}")
    print_indent("")

    print_indent("void ExtensionDBLoader::prime_global_enumerations()")
    print_indent("{")
    write_global_enums(data["global_enums"])
    print_indent("}")
    print_indent("")

    print_indent("void ExtensionDBLoader::prime_builtin_classes()")
    print_indent("{")
    write_builtin_types(data["builtin_classes"])
    print_indent("}")
    print_indent("")

    print_indent("void ExtensionDBLoader::prime_utility_functions()")
    print_indent("{")
    write_utility_functions(data["utility_functions"])
    print_indent("}")
    print_indent("")

    print_indent("void ExtensionDBLoader::prime_class_details()")
    print_indent("{")
    write_class_details(data["classes"])
    print_indent("}")
    print_indent("")

    print_indent("void ExtensionDBLoader::prime()")
    print_indent("{")
    indent_push()
    print_indent("prime_math_constants();")
    print_indent("prime_global_enumerations();")
    print_indent("prime_builtin_classes();")
    print_indent("prime_utility_functions();")
    print_indent("prime_class_details();")
    indent_pop()
    print_indent("}")


with open(sys.argv[1], 'r') as file:
    # Create the C++ header file
    create_hpp()

    # Used by the CMAKE utility to separate the output of the HPP and CPP
    # Do not change this
    print_indent("//##")

    create_cpp()

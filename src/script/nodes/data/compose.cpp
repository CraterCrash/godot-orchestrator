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
#include "compose.h"

#include "api/extension_db.h"
#include "common/dictionary_utils.h"
#include "common/property_utils.h"
#include "common/string_utils.h"
#include "common/variant_utils.h"

#include <godot_cpp/classes/expression.hpp>

OScriptNodeCompose::TypeMap OScriptNodeCompose::_type_components;

class OScriptNodeComposeInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeCompose);
    Array _components;
    Variant::Type _target_type{ Variant::NIL };

public:
    int step(OScriptExecutionContext& p_context) override
    {
        if (_target_type != Variant::NIL)
        {
            Variant output = VariantUtils::make_default(_target_type);
            for (int i = 0; i < _components.size(); i++)
                output.set(_components[i], p_context.get_input(i));

            p_context.set_output(0, output);
        }
        else
            p_context.set_output(0, Variant());

        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class OScriptNodeComposeFromInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeComposeFrom);
    Variant::Type _target_type{ Variant::NIL };
    Vector<Variant::Type> _constructor_arg_types;

    Variant _convert(Variant::Type p_expected_type, const Variant& p_value) const
    {
        switch (p_expected_type)
        {
            case Variant::BOOL:
                return p_value.operator bool();
            case Variant::VECTOR2:
                return p_value.operator Vector2();
            case Variant::VECTOR2I:
                return p_value.operator Vector2i();
            case Variant::VECTOR3:
                return p_value.operator Vector3();
            case Variant::VECTOR3I:
                return p_value.operator Vector3i();
            case Variant::VECTOR4:
                return p_value.operator Vector4();
            case Variant::VECTOR4I:
                return p_value.operator Vector4i();
            case Variant::QUATERNION:
                return p_value.operator Quaternion();
            default:
                return p_value;
        }
    }

    String _render(Variant::Type p_expected_type, const Variant& p_value) const
    {
        const Variant converted_value = _convert(p_expected_type, p_value);
        switch (p_expected_type)
        {
            case Variant::VECTOR2:
                return vformat("Vector2%s", converted_value);
            case Variant::VECTOR2I:
                return vformat("Vector2i%s", converted_value);
            case Variant::VECTOR3:
                return vformat("Vector3%s", converted_value);
            case Variant::VECTOR3I:
                return vformat("Vector3i%s", converted_value);
            case Variant::VECTOR4:
                return vformat("Vector4%s", converted_value);
            case Variant::VECTOR4I:
                return vformat("Vector4i%s", converted_value);
            case Variant::QUATERNION:
                return vformat("Quaternion%s", converted_value);
            case Variant::COLOR:
                return vformat("Color%s", converted_value);
            case Variant::NODE_PATH:
            case Variant::STRING:
            case Variant::STRING_NAME:
                return vformat("\"%s\"", converted_value);
            default:
                return vformat("%s", converted_value);
        }
    }

    String _get_argument_list(OScriptExecutionContext& p_context) const
    {
        PackedStringArray args;
        for (int i = 0; i < _constructor_arg_types.size(); i++)
        {
            Variant arg_value = p_context.get_input(i);
            args.push_back(_render(_constructor_arg_types[i], arg_value));
        }
        return StringUtils::join(", ", args);
    }

public:
    int step(OScriptExecutionContext& p_context) override
    {
        if (_target_type != Variant::NIL)
        {
            if (_constructor_arg_types.is_empty())
            {
                Variant output = VariantUtils::make_default(_target_type);
                p_context.set_output(0, output);
            }
            else
            {
                switch (_target_type)
                {
                    case Variant::BOOL:
                        p_context.set_output(0, VariantUtils::cast_to<bool>(p_context.get_input(0)));
                        break;
                    case Variant::INT:
                        p_context.set_output(0, VariantUtils::cast_to<int>(p_context.get_input(0)));
                        break;
                    case Variant::FLOAT:
                        p_context.set_output(0, VariantUtils::cast_to<float>(p_context.get_input(0)));
                        break;
                    case Variant::STRING:
                        p_context.set_output(0, VariantUtils::cast_to<String>(p_context.get_input(0)));
                        break;
                    case Variant::STRING_NAME:
                        p_context.set_output(0, VariantUtils::cast_to<StringName>(p_context.get_input(0)));
                        break;
                    case Variant::CALLABLE:
                    {
                        Object* callable_object = p_context.get_input(0);
                        if (!callable_object)
                            callable_object = p_context.get_owner();

                        Callable c(callable_object, p_context.get_input(1));
                        p_context.set_output(0, c);
                        break;
                    }
                    default:
                    {
                        const String type_name = Variant::get_type_name(_target_type);
                        String ctor_expression = vformat("%s(%s)", type_name, _get_argument_list(p_context));

                        Ref<Expression> expression;
                        expression.instantiate();

                        if (expression->parse(ctor_expression) != Error::OK)
                        {
                            p_context.set_error(GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT,
                                                "Failed to parse expression: " + ctor_expression);
                            return -1 | STEP_FLAG_END;
                        }

                        Variant result = expression->execute();
                        if (expression->has_execute_failed())
                        {
                            p_context.set_error(GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT,
                                                "Failed to evaluate expression: " + ctor_expression);
                            return -1 | STEP_FLAG_END;
                        }

                        p_context.set_output(0, result);
                        break;
                    }
                }
            }
        }
        else
            p_context.set_output(0, Variant());

        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeCompose::_bind_methods()
{
    // Populate the type components
    for (const String& type_name : ExtensionDB::get_builtin_type_names())
    {
        const BuiltInType type = ExtensionDB::get_builtin_type(type_name);
        if (!type.properties.is_empty())
        {
            Array properties;
            for (const PropertyInfo& pi : type.properties)
                properties.push_back(pi.name);

            // Color exposes a variety of additional properties, we only concern ourselves
            // with the R,G,B,A properties and not R8,G8,B8,A8 nor H, S, or V.
            if (type.type == Variant::COLOR)
                properties.resize(4);

            // Plane exposes not only the X,Y,Z and distance but also the normal.
            // We want to express planes only via X, Y, Z, and distance.
            else if (type.type == Variant::PLANE)
                properties.resize(4);

            // AABB exposes position, size, and end.
            // We only want to express AABB via position and size only.
            else if (type.type == Variant::AABB)
                properties.resize(2);

            _type_components[type.type] = properties;
        }
    }
}

void OScriptNodeCompose::post_initialize()
{
    // Clone this from the output pin
    _type = find_pin("value", PD_Output)->get_type();
    super::post_initialize();
}

void OScriptNodeCompose::allocate_default_pins()
{
    Variant default_value = VariantUtils::make_default(_type);

    const Array& components = _type_components[_type];
    for (int i = 0; i < components.size(); i++)
    {
        const Variant bit = default_value.get(components[i]);
        create_pin(PD_Input, PT_Data, PropertyUtils::make_typed(components[i], bit.get_type()));
    }

    // This is the pin that will be constructed from its types
    create_pin(PD_Output, PT_Data, PropertyUtils::make_typed("value", _type));
}

String OScriptNodeCompose::get_tooltip_text() const
{
    if (_type != Variant::NIL)
    {
        const String type_name = VariantUtils::get_friendly_type_name(_type);
        const String components = StringUtils::join(", ", _type_components[_type]);
        return vformat("Make a %s from %s", type_name, components);
    }
    return "Construct a Godot built-in type, optionally from its sub-components.";
}

String OScriptNodeCompose::get_node_title() const
{
    return "Make " + VariantUtils::get_friendly_type_name(_type);
}

String OScriptNodeCompose::get_icon() const
{
    return "Instance";
}

OScriptNodeInstance* OScriptNodeCompose::instantiate()
{
    OScriptNodeComposeInstance* i = memnew(OScriptNodeComposeInstance);
    i->_node = this;
    i->_target_type = _type;
    i->_components = _type_components[_type];
    return i;
}

void OScriptNodeCompose::initialize(const OScriptNodeInitContext& p_context)
{
    ERR_FAIL_COND_MSG(!p_context.user_data, "A Compose node requires custom data");

    const Dictionary& data = p_context.user_data.value();
    ERR_FAIL_COND_MSG(!data.has("type"), "Cannot properly initialize compose node, no type specified.");

    _type = VariantUtils::to_type(data["type"]);
    super::initialize(p_context);
}

bool OScriptNodeCompose::is_supported(Variant::Type p_type)
{
    switch (p_type)
    {
        // These types are handled by OScriptNodeComposeFrom
        case Variant::AABB:
        case Variant::BASIS:
        case Variant::COLOR:
        case Variant::PLANE:
        case Variant::PROJECTION:
        case Variant::QUATERNION:
        case Variant::RECT2:
        case Variant::RECT2I:
        case Variant::TRANSFORM2D:
        case Variant::TRANSFORM3D:
        case Variant::VECTOR2:
        case Variant::VECTOR2I:
        case Variant::VECTOR3:
        case Variant::VECTOR3I:
        case Variant::VECTOR4:
        case Variant::VECTOR4I:
            return false;
        default:
            return true;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeComposeFrom::_bind_methods()
{
}

void OScriptNodeComposeFrom::post_initialize()
{
    // Clone this from the output pin
    _type = find_pin("value", PD_Output)->get_type();

    for (const Ref<OScriptNodePin>& pin : find_pins(PD_Input))
    {
        if (!pin->is_execution())
            _constructor_args.push_back(PropertyInfo(pin->get_type(), pin->get_pin_name()));
    }

    super::post_initialize();
}

void OScriptNodeComposeFrom::allocate_default_pins()
{
    for (int i = 0; i < _constructor_args.size(); i++)
    {
        const PropertyInfo& property = _constructor_args[i];
        if (property.name.is_empty())
            create_pin(PD_Input, PT_Data, PropertyUtils::as("arg" + itos(i), property));
        else
            create_pin(PD_Input, PT_Data, property);
    }

    // This is the pin that will be constructed from its types
    create_pin(PD_Output, PT_Data, PropertyUtils::make_typed("value", _type));
}

String OScriptNodeComposeFrom::get_tooltip_text() const
{
    if (_type != Variant::NIL)
    {
        const String type_name = VariantUtils::get_friendly_type_name(_type);

        PackedStringArray component_names;
        for (const PropertyInfo& property : _constructor_args)
            component_names.push_back(VariantUtils::get_friendly_type_name(property.type));

        const String components = StringUtils::join(" and ", component_names);
        return vformat("Construct a %s from %s", type_name, components);
    }
    return "Construct a Godot built-in type, optionally from its sub-components.";
}

String OScriptNodeComposeFrom::get_node_title() const
{
    return "Make " + VariantUtils::get_friendly_type_name(_type);
}

String OScriptNodeComposeFrom::get_icon() const
{
    return "Instance";
}

OScriptNodeInstance* OScriptNodeComposeFrom::instantiate()
{
    OScriptNodeComposeFromInstance* i = memnew(OScriptNodeComposeFromInstance);
    i->_node = this;
    i->_target_type = _type;

    Vector<Variant::Type> arg_types;
    for (const PropertyInfo& pi : _constructor_args)
        arg_types.push_back(pi.type);

    i->_constructor_arg_types = arg_types;
    return i;
}

void OScriptNodeComposeFrom::initialize(const OScriptNodeInitContext& p_context)
{
    ERR_FAIL_COND_MSG(!p_context.user_data, "A ComposeFrom node requires custom data");

    const Dictionary& data = p_context.user_data.value();
    ERR_FAIL_COND_MSG(!data.has("type"), "Cannot properly initialize compose from node, no type specified.");

    _type = VariantUtils::to_type(data["type"]);
    if (data.has("constructor_args"))
    {
        const Array constructor_types = data["constructor_args"];
        for (int i = 0; i < constructor_types.size(); i++)
        {
            const PropertyInfo pi = DictionaryUtils::to_property(constructor_types[i]);
            _constructor_args.push_back(pi);
        }
    }

    super::initialize(p_context);
}

bool OScriptNodeComposeFrom::is_supported(Variant::Type p_type, const Vector<PropertyInfo>& p_args)
{
    switch (p_type)
    {
        case Variant::NIL: // Unnecessary
        case Variant::ARRAY: // Makes use of custom MakeArray
        case Variant::DICTIONARY: // Makes use of custom MakeDictionary
        case Variant::RID: // Not necessary
        case Variant::SIGNAL: // Not necessary
            return false;
        case Variant::PACKED_BYTE_ARRAY:
        case Variant::PACKED_STRING_ARRAY:
        case Variant::PACKED_COLOR_ARRAY:
        case Variant::PACKED_FLOAT32_ARRAY:
        case Variant::PACKED_FLOAT64_ARRAY:
        case Variant::PACKED_INT32_ARRAY:
        case Variant::PACKED_INT64_ARRAY:
        case Variant::PACKED_VECTOR2_ARRAY:
        case Variant::PACKED_VECTOR3_ARRAY:
        case Variant::AABB:
        case Variant::BASIS:
        case Variant::CALLABLE:
        case Variant::PLANE:
        case Variant::TRANSFORM2D:
            // Single argument constructors with same types, ignore them.
            if (p_args.size() == 1 && p_args[0].type == p_type)
                return false;
            break;
        case Variant::TRANSFORM3D:
            // Single argument constructors with same types, ignore them.
            if (p_args.size() == 1 && (p_args[0].type == p_type || p_args[0].type == Variant::PROJECTION))
                return false;
            if (p_args.size() == 2 && p_args[0].type == Variant::BASIS && p_args[1].type == Variant::VECTOR3)
                return false;
            break;
        case Variant::RECT2:
            // Single argument constructors with same types, ignore them.
            if (p_args.size() == 1 && (p_args[0].type == p_type || p_args[0].type == Variant::RECT2I))
                return false;
            break;
        case Variant::RECT2I:
            // Single argument constructors with same types, ignore them.
            if (p_args.size() == 1 && (p_args[0].type == p_type || p_args[0].type == Variant::RECT2))
                return false;
            break;
        case Variant::PROJECTION:
            // Single argument constructors with same types, ignore them.
            if (p_args.size() == 1 && (p_args[0].type == p_type || p_args[0].type == Variant::TRANSFORM3D))
                return false;
            break;
        case Variant::QUATERNION:
            if (p_args.size() == 1 && p_args[0].type == Variant::BASIS)
                return false;
            break;
        default:
            break;
    }

    return true;
}
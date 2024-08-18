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
#include "script/variable.h"

#include "api/extension_db.h"
#include "common/property_utils.h"
#include "common/string_utils.h"
#include "common/variant_utils.h"
#include "script/script.h"

PropertyInfo _parse_classification(const String& p_classification)
{
    if (p_classification.begins_with("type:"))
    {
        // Basic types
        const String type_name = p_classification.substr(p_classification.find(":") + 1);
        for (int i = 0; i < Variant::VARIANT_MAX; i++)
        {
            const Variant::Type type = VariantUtils::to_type(i);
            if (Variant::get_type_name(type).match(type_name))
            {
                PropertyInfo property;
                property.type = type;
                property.hint = PROPERTY_HINT_NONE;
                property.hint_string = "";
                property.class_name = "";
                property.usage = PROPERTY_USAGE_SCRIPT_VARIABLE;

                switch (type)
                {
                    case Variant::CALLABLE:
                    case Variant::SIGNAL:
                    case Variant::RID:
                        property.usage |= PROPERTY_USAGE_STORAGE;
                        break;
                    case Variant::NIL:
                        property.usage |= (PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_NIL_IS_VARIANT);
                        break;
                    default:
                        property.usage |= PROPERTY_USAGE_DEFAULT;
                }

                return property;
            }
        }
    }
    else if (p_classification.begins_with("enum:") || p_classification.begins_with("bitfield:"))
    {
        // Enum and Bitfields
        const String name = p_classification.substr(p_classification.find(":") + 1);
        const EnumInfo& info = ExtensionDB::get_global_enum(name);
        if (!info.values.is_empty())
        {
            PackedStringArray hints;
            for (int index = 0; index < info.values.size(); ++index)
                hints.push_back(info.values[index].name);

            const bool is_bitfield = p_classification.begins_with("bitfield:");

            PropertyInfo property;
            property.type = Variant::INT;
            property.hint_string = StringUtils::join(",", hints);
            property.class_name = name;
            property.usage = PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_SCRIPT_VARIABLE;

            if (is_bitfield)
            {
                property.hint = PROPERTY_HINT_FLAGS;
                property.usage |= PROPERTY_USAGE_CLASS_IS_BITFIELD;
            }
            else
            {
                property.hint = PROPERTY_HINT_ENUM;
                property.usage |= PROPERTY_USAGE_CLASS_IS_ENUM;
            }

            return property;
        }
    }
    else if (p_classification.begins_with("class:"))
    {
        // Classes
        const String class_name = p_classification.substr(p_classification.find(":") + 1);

        PropertyInfo property;
        property.type = Variant::OBJECT;
        property.hint = PROPERTY_USAGE_NONE;
        property.hint_string = "";
        property.class_name = class_name;
        property.usage = PROPERTY_USAGE_NO_EDITOR;

        if (ClassDB::is_parent_class(class_name, "Resource"))
        {
            property.hint = PROPERTY_HINT_RESOURCE_TYPE;
            property.hint_string = class_name;
            property.class_name = "";
            property.usage = PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_SCRIPT_VARIABLE;
        }
        else if (ClassDB::is_parent_class(class_name, "Node"))
        {
            property.hint = PROPERTY_HINT_NODE_TYPE;
            property.hint_string = class_name;
            property.class_name = class_name;
            property.usage = PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_SCRIPT_VARIABLE;
        }

        return property;
    }
    else if (p_classification.begins_with("class_enum:") || p_classification.begins_with("class_bitfield:"))
    {
        const String class_enum_name = p_classification.substr(p_classification.find(":") + 1);
        const String class_name = class_enum_name.substr(0, class_enum_name.find("."));
        const String enum_name = class_enum_name.substr(class_enum_name.find(".") + 1);
        const String hint_string = StringUtils::join(",", ClassDB::class_get_enum_constants(class_name, enum_name, true));

        PropertyInfo property;
        property.type = Variant::INT;
        property.hint_string = hint_string;
        property.class_name = class_enum_name;

        if (p_classification.begins_with("class_bitfield:"))
        {
            property.hint = PROPERTY_HINT_FLAGS;
            property.usage = PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_BITFIELD | PROPERTY_USAGE_SCRIPT_VARIABLE;
        }
        else
        {
            property.hint = PROPERTY_HINT_ENUM;
            property.usage = PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_ENUM | PROPERTY_USAGE_SCRIPT_VARIABLE;
        }

        return property;
    }

    PropertyInfo property;
    property.type = Variant::NIL;
    property.hint = PROPERTY_HINT_NONE;
    property.hint_string = "";
    property.class_name = "";
    property.usage = PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_SCRIPT_VARIABLE;

    return property;
}

void OScriptVariableBase::_validate_property(PropertyInfo& p_property) const
{
    if (p_property.name.match("default_value"))
    {
        p_property.type = _info.type;
        p_property.class_name = _info.class_name;
        p_property.hint = _info.hint;
        p_property.hint_string = _info.hint_string;

        switch (_info.hint)
        {
            case PROPERTY_HINT_NODE_TYPE:
                p_property.usage = PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_SCRIPT_VARIABLE;
                break;
            default:
                p_property.usage = _info.usage;
            break;
        };
    }
    else if (p_property.name.match("constant"))
    {
        if (_supports_constants())
            p_property.usage = PROPERTY_USAGE_DEFAULT;
        else
            p_property.usage = PROPERTY_USAGE_NONE;
    }
    else if (p_property.name.match("exported"))
    {
        if (_supports_exported())
        {
            p_property.usage = PROPERTY_USAGE_DEFAULT;
            if (!_exportable)
                p_property.usage |= PROPERTY_USAGE_READ_ONLY;
        }
        else
            p_property.usage = PROPERTY_USAGE_NONE;
    }
    else if (p_property.name.match("type"))
    {
        if (_supports_legacy_type())
            p_property.usage = PROPERTY_USAGE_STORAGE;
        else
            p_property.usage = PROPERTY_USAGE_NONE;
    }
    else if (p_property.name.match("value_list"))
    {
        if (_classification.begins_with("custom_"))
        {
            const bool is_enum = (_info.hint & PROPERTY_HINT_ENUM) || (_info.usage & PROPERTY_USAGE_CLASS_IS_ENUM);
            const bool is_bitfield = (_info.hint & PROPERTY_HINT_FLAGS) || (_info.usage & PROPERTY_USAGE_CLASS_IS_BITFIELD);
            p_property.usage = (is_enum || is_bitfield) ? PROPERTY_USAGE_DEFAULT : PROPERTY_USAGE_NO_EDITOR;
        }
        else
            p_property.usage = PROPERTY_USAGE_NO_EDITOR;
    }
}

bool OScriptVariableBase::_property_can_revert(const StringName& p_name) const
{
    static Array properties = Array::make("name", "category", "exported", "classification", "default_value", "description", "constant");
    return properties.has(p_name);
}

bool OScriptVariableBase::_property_get_revert(const StringName& p_name, Variant& r_property)
{
    if (p_name.match("name"))
    {
        r_property = _info.name;
        return true;
    }
    else if (p_name.match("category"))
    {
        r_property = "Default";
        return true;
    }
    else if (p_name.match("classification"))
    {
        r_property = "type:bool";
        return true;
    }
    else if (p_name.match("default_value"))
    {
        r_property = VariantUtils::make_default(_info.type);
        return true;
    }
    else if (p_name.match("description"))
    {
        r_property = "";
        return true;
    }
    else if (p_name.match("exported") || p_name.match("constant"))
    {
        r_property = false;
        return true;
    }
    return false;
}

void OScriptVariableBase::_set_variable_type(Variant::Type p_type)
{
    if (_info.type != p_type)
    {
        _info.type = p_type;

        if (_default_value.get_type() != _info.type)
            set_default_value(VariantUtils::make_default(_info.type));

        notify_property_list_changed();
        emit_changed();
    }
}

bool OScriptVariableBase::_is_exportable_type(const PropertyInfo& p_property) const
{
    // If the variable doesn't support the export feature or is a constant, cannot be exported
    if (!_supports_exported() || is_constant())
        return false;

    switch (p_property.type)
    {
        // These are all not exportable
        case Variant::CALLABLE:
        case Variant::SIGNAL:
        case Variant::RID:
            return false;

        // Object has specific use cases
        case Variant::OBJECT:
        {
            const String& hint = p_property.hint_string;
            if (hint.is_empty() || (!ClassDB::is_parent_class(hint, "Node") && !ClassDB::is_parent_class(hint, "Resource")))
                return false;

            break;
        }

        default:
            break;
    }

    return true;
}

void OScriptVariableBase::post_initialize()
{
    if (_classification.is_empty())
    {
        if (_supports_exported())
            _exportable = _is_exportable_type(_info);

        _classification = vformat("type:%s", Variant::get_type_name(_info.type));
    }
}

bool OScriptVariableBase::is_grouped_by_category() const
{
    return !_category.is_empty() && !_category.to_lower().match("default") && !_category.to_lower().match("none");
}

String OScriptVariableBase::get_variable_type_name() const
{
    return PropertyUtils::get_property_type_name(_info);
}

void OScriptVariableBase::set_variable_name(const String& p_name)
{
    if (!_info.name.match(p_name))
    {
        _info.name = p_name;
        emit_changed();
    }
}

void OScriptVariableBase::set_description(const String& p_description)
{
    if (!_description.match(p_description))
    {
        _description = p_description;
        emit_changed();
    }
}

void OScriptVariableBase::set_category(const String& p_category)
{
    if (!_category.match(p_category))
    {
        _category = p_category;
        emit_changed();
    }
}

void OScriptVariableBase::set_default_value(const Variant& p_value)
{
    if (_default_value != p_value)
    {
        _default_value = p_value;
        emit_changed();
    }
}

void OScriptVariableBase::set_classification(const String& p_classification)
{
    if (!_classification.match(p_classification) && p_classification.contains(":"))
    {
        _classification = p_classification;

        const PropertyInfo property = _parse_classification(_classification);
        if (property.hint == PROPERTY_USAGE_NONE && property.hint_string.is_empty() && property.class_name.is_empty())
        {
            // Basic type
            if (_info.type != property.type)
                set_default_value(VariantUtils::convert(get_default_value(), property.type));
        }

        _info.type = property.type;
        _info.hint = property.hint;
        _info.hint_string = property.hint_string;
        _info.class_name = property.class_name;
        _info.usage = property.usage;

        _exportable = _is_exportable_type(_info);

        notify_property_list_changed();
        emit_changed();
    }
}

void OScriptVariableBase::set_custom_value_list(const String& p_custom_value_list)
{
    if (!_custom_value_list.match(p_custom_value_list))
    {
        _custom_value_list = p_custom_value_list;
        emit_changed();
    }
}

void OScriptVariableBase::set_constant(bool p_constant)
{
    if (_constant != p_constant)
    {
        ERR_FAIL_COND_MSG(!_supports_constants(), "Variable does not support the constant feature");

        _constant = p_constant;

        _exportable = _is_exportable_type(_info);
        if (!_exportable && _constant)
            _exported = false;

        notify_property_list_changed();
        emit_changed();
    }
}

void OScriptVariableBase::set_exported(bool p_exported)
{
    if (_exported != p_exported)
    {
        ERR_FAIL_COND_MSG(!_supports_exported(), "Variable does not support the exported feature.");

        _exported = p_exported;
        emit_changed();
    }
}

bool OScriptVariableBase::supports_validated_getter() const
{
    return _info.type == Variant::OBJECT;
}

void OScriptVariableBase::_bind_methods()
{
    // Name is read-only in the inspector
    // Renaming of variables should be done via the component panel only.
    ClassDB::bind_method(D_METHOD("set_variable_name", "name"), &OScriptVariableBase::set_variable_name);
    ClassDB::bind_method(D_METHOD("get_variable_name"), &OScriptVariableBase::get_variable_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY), "set_variable_name", "get_variable_name");

    ClassDB::bind_method(D_METHOD("set_category", "category"), &OScriptVariableBase::set_category);
    ClassDB::bind_method(D_METHOD("get_category"), &OScriptVariableBase::get_category);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "category"), "set_category", "get_category");

    ClassDB::bind_method(D_METHOD("set_constant", "constant"), &OScriptVariableBase::set_constant);
    ClassDB::bind_method(D_METHOD("is_constant"), &OScriptVariableBase::is_constant);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "constant"), "set_constant", "is_constant");

    ClassDB::bind_method(D_METHOD("set_exported", "exported"), &OScriptVariableBase::set_exported);
    ClassDB::bind_method(D_METHOD("is_exported"), &OScriptVariableBase::is_exported);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "exported"), "set_exported", "is_exported");

    ClassDB::bind_method(D_METHOD("set_classification", "classification"), &OScriptVariableBase::set_classification);
    ClassDB::bind_method(D_METHOD("get_classification"), &OScriptVariableBase::get_classification);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "classification"), "set_classification", "get_classification");

    const String types = VariantUtils::to_enum_list();
    ClassDB::bind_method(D_METHOD("_set_variable_type", "type"), &OScriptVariableBase::_set_variable_type);
    ClassDB::bind_method(D_METHOD("_get_variable_type"), &OScriptVariableBase::_get_variable_type);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "type", PROPERTY_HINT_ENUM, types, PROPERTY_USAGE_STORAGE), "_set_variable_type", "_get_variable_type");

    ClassDB::bind_method(D_METHOD("set_default_value", "value"), &OScriptVariableBase::set_default_value);
    ClassDB::bind_method(D_METHOD("get_default_value"), &OScriptVariableBase::get_default_value);
    ADD_PROPERTY(PropertyInfo(Variant::NIL, "default_value", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "set_default_value", "get_default_value");

    ClassDB::bind_method(D_METHOD("set_custom_value_list", "value_list"), &OScriptVariableBase::set_custom_value_list);
    ClassDB::bind_method(D_METHOD("get_custom_value_list"), &OScriptVariableBase::get_custom_value_list);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "value_list", PROPERTY_HINT_MULTILINE_TEXT), "set_custom_value_list", "get_custom_value_list");

    ClassDB::bind_method(D_METHOD("set_description", "description"), &OScriptVariableBase::set_description);
    ClassDB::bind_method(D_METHOD("get_description"), &OScriptVariableBase::get_description);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "description", PROPERTY_HINT_MULTILINE_TEXT), "set_description", "get_description");
}

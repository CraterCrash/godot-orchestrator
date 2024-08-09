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
#include "script/variable.h"

#include "api/extension_db.h"
#include "common/property_utils.h"
#include "common/string_utils.h"
#include "common/variant_utils.h"
#include "script/script.h"

void OScriptVariable::_bind_methods()
{
    // This is read-only to avoid name changes in the inspector, which creates cache issues with the owning script
    ClassDB::bind_method(D_METHOD("set_variable_name", "name"), &OScriptVariable::set_variable_name);
    ClassDB::bind_method(D_METHOD("get_variable_name"), &OScriptVariable::get_variable_name);
    ADD_PROPERTY(
        PropertyInfo(Variant::STRING, "name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY),
        "set_variable_name", "get_variable_name");

    ClassDB::bind_method(D_METHOD("set_category", "category"), &OScriptVariable::set_category);
    ClassDB::bind_method(D_METHOD("get_category"), &OScriptVariable::get_category);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "category"), "set_category", "get_category");

    ClassDB::bind_method(D_METHOD("set_exported", "exported"), &OScriptVariable::set_exported);
    ClassDB::bind_method(D_METHOD("is_exported"), &OScriptVariable::is_exported);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "exported"), "set_exported", "is_exported");

    ClassDB::bind_method(D_METHOD("set_classification", "classification"), &OScriptVariable::set_classification);
    ClassDB::bind_method(D_METHOD("get_classification"), &OScriptVariable::get_classification);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "classification"), "set_classification", "get_classification");

    const String types = VariantUtils::to_enum_list();
    ClassDB::bind_method(D_METHOD("set_variable_type", "type"), &OScriptVariable::set_variable_type);
    ClassDB::bind_method(D_METHOD("get_variable_type"), &OScriptVariable::get_variable_type);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "type", PROPERTY_HINT_ENUM, types, PROPERTY_USAGE_STORAGE), "set_variable_type", "get_variable_type");

    ClassDB::bind_method(D_METHOD("set_default_value", "value"), &OScriptVariable::set_default_value);
    ClassDB::bind_method(D_METHOD("get_default_value"), &OScriptVariable::get_default_value);
    ADD_PROPERTY(PropertyInfo(Variant::NIL, "default_value", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT),
                 "set_default_value", "get_default_value");

    ClassDB::bind_method(D_METHOD("set_custom_value_list", "value_list"), &OScriptVariable::set_custom_value_list);
    ClassDB::bind_method(D_METHOD("get_custom_value_list"), &OScriptVariable::get_custom_value_list);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "value_list", PROPERTY_HINT_MULTILINE_TEXT), "set_custom_value_list", "get_custom_value_list");

    ClassDB::bind_method(D_METHOD("set_description", "description"), &OScriptVariable::set_description);
    ClassDB::bind_method(D_METHOD("get_description"), &OScriptVariable::get_description);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "description", PROPERTY_HINT_MULTILINE_TEXT), "set_description",
                 "get_description");
}

void OScriptVariable::_validate_property(PropertyInfo& p_property) const
{
    if (p_property.name.match("default_value"))
    {
        p_property.type = _info.type;
        p_property.class_name = _info.class_name;
        p_property.hint = _info.hint;
        p_property.hint_string = _info.hint_string;
        p_property.usage = _info.hint == PROPERTY_HINT_NODE_TYPE
            ? PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_SCRIPT_VARIABLE
            : _info.usage;
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
    else if (p_property.name.match("exported"))
    {
        if (_exportable)
            p_property.usage &= ~PROPERTY_USAGE_READ_ONLY;
        else
            p_property.usage |= PROPERTY_USAGE_READ_ONLY;
    }
}

bool OScriptVariable::_property_can_revert(const StringName& p_name) const
{
    static Array properties = Array::make("name", "category", "exported", "classification", "default_value", "description");
    return properties.has(p_name);
}

bool OScriptVariable::_property_get_revert(const StringName& p_name, Variant& r_property)
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
    else if (p_name.match("exported"))
    {
        r_property = false;
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
    return false;
}

bool OScriptVariable::_is_exportable_type(const PropertyInfo& p_property) const
{
    switch (p_property.type)
    {
        // These are all not exportable
        case Variant::CALLABLE:
        case Variant::SIGNAL:
        case Variant::RID:
            return false;

        // Object has specific circumstances depending on hint string
        case Variant::OBJECT:
        {
            if (p_property.hint_string.is_empty())
                return false;

            if (!ClassDB::is_parent_class(p_property.hint_string, "Node")
                    && !ClassDB::is_parent_class(p_property.hint_string, "Resource"))
                return false;

            break;
        }

        default:
            break;
    }
    return true;
}

bool OScriptVariable::_convert_default_value(Variant::Type p_new_type)
{
    set_default_value(VariantUtils::convert(get_default_value(), p_new_type));
    return true;
}

OScriptVariable::OScriptVariable()
{
    _info.type = Variant::NIL;
}

void OScriptVariable::post_initialize()
{
    if (_classification.is_empty())
    {
        // Prepares the OScriptVariable for the variable system v2 using classifications
        _exportable = _is_exportable_type(_info);
        _classification = vformat("type:%s", Variant::get_type_name(_info.type));
    }
}

Orchestration* OScriptVariable::get_orchestration() const
{
    return _orchestration;
}

void OScriptVariable::set_variable_name(const String& p_name)
{
    if (!_info.name.match(p_name))
    {
        _info.name = p_name;
        emit_changed();
    }
}

bool OScriptVariable::is_grouped_by_category() const
{
    return !_category.is_empty() && !_category.to_lower().match("default") && !_category.to_lower().match("none");
}

void OScriptVariable::set_category(const String& p_category)
{
    if (!_category.match(p_category))
    {
        _category = p_category;
        emit_changed();
    }
}

void OScriptVariable::set_classification(const String& p_classification)
{
    if (!_classification.match(p_classification))
    {
        _classification = p_classification;

        if (_classification.contains(":"))
        {
            if (_classification.begins_with("type:"))
            {
                // basic types
                const String type_name = _classification.substr(_classification.find(":") + 1);
                for (int i = 0; i < Variant::VARIANT_MAX; i++)
                {
                    const Variant::Type type = VariantUtils::to_type(i);
                    if (Variant::get_type_name(type).match(type_name))
                    {
                        if (_info.type != type)
                            _convert_default_value(type);

                        _info.type = type;
                        _info.hint = PROPERTY_HINT_NONE;
                        _info.hint_string = "";
                        _info.class_name = "";

                        // These cannot have default values
                        if (type == Variant::CALLABLE || type == Variant::SIGNAL || type == Variant::RID || type == Variant::NIL)
                            _info.usage = PROPERTY_USAGE_STORAGE;
                        else
                            _info.usage = PROPERTY_USAGE_DEFAULT;

                        if (type == Variant::NIL)
                            _info.usage |= PROPERTY_USAGE_NIL_IS_VARIANT;

                        break;
                    }
                }
            }
            else if (_classification.begins_with("enum:") || _classification.begins_with("bitfield:"))
            {
                // enum/bitfields
                const String name = _classification.substr(_classification.find(":") + 1);
                const EnumInfo& enum_info = ExtensionDB::get_global_enum(name);
                if (!enum_info.values.is_empty())
                {
                    PackedStringArray hints;
                    for (int i = 0; i < enum_info.values.size(); i++)
                        hints.push_back(enum_info.values[i].name);

                    _info.type = Variant::INT;
                    _info.hint = _classification.begins_with("bitfield:") ? PROPERTY_HINT_FLAGS : PROPERTY_HINT_ENUM;
                    _info.hint_string = StringUtils::join(",", hints);
                    _info.class_name = name;
                    if (_classification.begins_with("bitfield:"))
                        _info.usage = PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_BITFIELD;
                    else
                        _info.usage = PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_ENUM;
                }
            }
            else if (_classification.begins_with("class:"))
            {
                // class type
                const String class_name = _classification.substr(_classification.find(":") + 1);
                if (ClassDB::is_parent_class(class_name, "Resource"))
                {
                    _info.type = Variant::OBJECT;
                    _info.hint = PROPERTY_HINT_RESOURCE_TYPE;
                    _info.hint_string = class_name;
                    _info.class_name = "";
                    _info.usage = PROPERTY_USAGE_DEFAULT;
                }
                else if (ClassDB::is_parent_class(class_name, "Node"))
                {
                    _info.type = Variant::OBJECT;
                    _info.hint = PROPERTY_HINT_NODE_TYPE;
                    _info.hint_string = class_name;
                    _info.class_name = class_name;
                    _info.usage = PROPERTY_USAGE_DEFAULT;
                }
                else
                {
                    _info.type = Variant::OBJECT;
                    _info.hint = PROPERTY_HINT_NONE;
                    _info.hint_string = "";
                    _info.class_name = class_name;
                    _info.usage = PROPERTY_USAGE_NO_EDITOR;
                }
            }
            else if (_classification.begins_with("class_enum:") || _classification.begins_with("class_bitfield:"))
            {
                const String class_enum_name = _classification.substr(_classification.find(":") + 1);
                const String class_name = class_enum_name.substr(0, class_enum_name.find("."));
                const String enum_name = class_enum_name.substr(class_enum_name.find(".") + 1);
                const String hint_string = StringUtils::join(",", ClassDB::class_get_enum_constants(class_name, enum_name, true));
                const bool bitfield = _classification.begins_with("class_bitfield:");

                _info.type = Variant::INT;
                _info.hint = bitfield ? PROPERTY_HINT_FLAGS : PROPERTY_HINT_ENUM;
                _info.hint_string = hint_string;
                _info.class_name = class_enum_name;
                _info.usage = PROPERTY_USAGE_DEFAULT | (bitfield ? PROPERTY_USAGE_CLASS_IS_BITFIELD : PROPERTY_USAGE_CLASS_IS_ENUM);
            }
            else if (_classification.begins_with("custom_enum:") || _classification.begins_with("custom_bitfield:"))
            {
                const bool bitfield = _classification.begins_with("custom_bitfield:");
                _info.type = Variant::INT;
                _info.hint = bitfield ? PROPERTY_HINT_FLAGS : PROPERTY_HINT_ENUM;
                _info.hint_string = "";
                _info.class_name = "";
                _info.usage = PROPERTY_USAGE_NO_EDITOR;
            }
        }
        else
        {
            _info.type = Variant::NIL;
            _info.hint = PROPERTY_HINT_NONE;
            _info.hint_string = "";
            _info.class_name = "";
            _info.usage = PROPERTY_USAGE_NO_EDITOR; // no default value
        }

        _exportable = _is_exportable_type(_info);
        _info.usage |= PROPERTY_USAGE_SCRIPT_VARIABLE;

        notify_property_list_changed();
        emit_changed();
    }
}

void OScriptVariable::set_custom_value_list(const String& p_value_list)
{
    if (!_value_list.match(p_value_list))
    {
        _value_list = p_value_list;
        emit_changed();
    }
}

void OScriptVariable::set_variable_type(const Variant::Type p_type)
{
    if (_info.type != p_type)
    {
        _info.type = p_type;

        if (_default_value.get_type() != _info.type)
            set_default_value(VariantUtils::make_default(_info.type));

        emit_changed();
        notify_property_list_changed();
    }
}

String OScriptVariable::get_variable_type_name() const
{
    return PropertyUtils::get_property_type_name(_info);
}

void OScriptVariable::set_description(const String& p_description)
{
    if (_description != p_description)
    {
        _description = p_description;
    }
}

void OScriptVariable::set_exported(bool p_exported)
{
    if (_exported != p_exported)
    {
        _exported = p_exported;
        emit_changed();
    }
}

void OScriptVariable::set_default_value(const Variant& p_default_value)
{
    if (_default_value != p_default_value)
    {
        _default_value = p_default_value;
        emit_changed();
    }
}

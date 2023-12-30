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
#include "variable.h"

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

    const String types = VariantUtils::to_enum_list();
    ClassDB::bind_method(D_METHOD("set_variable_type", "type"), &OScriptVariable::set_variable_type);
    ClassDB::bind_method(D_METHOD("get_variable_type"), &OScriptVariable::get_variable_type);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "type", PROPERTY_HINT_ENUM, types), "set_variable_type", "get_variable_type");

    ClassDB::bind_method(D_METHOD("set_default_value", "value"), &OScriptVariable::set_default_value);
    ClassDB::bind_method(D_METHOD("get_default_value"), &OScriptVariable::get_default_value);
    ADD_PROPERTY(PropertyInfo(Variant::NIL, "default_value", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT),
                 "set_default_value", "get_default_value");

    ClassDB::bind_method(D_METHOD("set_exported", "exported"), &OScriptVariable::set_exported);
    ClassDB::bind_method(D_METHOD("is_exported"), &OScriptVariable::is_exported);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "exported"), "set_exported", "is_exported");

    ClassDB::bind_method(D_METHOD("set_description", "description"), &OScriptVariable::set_description);
    ClassDB::bind_method(D_METHOD("get_description"), &OScriptVariable::get_description);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "description", PROPERTY_HINT_MULTILINE_TEXT), "set_description",
                 "get_description");
}

void OScriptVariable::_validate_property(PropertyInfo& p_property) const
{
    if (p_property.name.match("default_value"))
        p_property.type = get_variable_type();
}

OScriptVariable::OScriptVariable()
{
    _info.type = Variant::BOOL;
}

Ref<OScript> OScriptVariable::get_owning_script() const
{
    return _script;
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

void OScriptVariable::set_variable_type(const Variant::Type p_type)
{
    if (_info.type != p_type)
    {
        _info.type = p_type;

        if (_default_value.get_type() != _info.type)
            _default_value = VariantUtils::make_default(_info.type);

        emit_changed();
        notify_property_list_changed();
    }
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

Ref<OScriptVariable> OScriptVariable::create(OScript* p_script, const PropertyInfo& p_property)
{
    Ref<OScriptVariable> variable(memnew(OScriptVariable));
    variable->_info = p_property;
    variable->_script = p_script;
    variable->_default_value = VariantUtils::make_default(p_property.type);
    variable->_category = "Default";

    return variable;
}

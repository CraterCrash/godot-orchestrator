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
#include "script/script_docdata.h"

Dictionary OScriptDocData::_create_property_documentation(const PropertyInfo& p_property)
{
    Dictionary property;
    property["name"] = p_property.name;
    property["type"] = p_property.type == Variant::NIL ? "Variant" : Variant::get_type_name(p_property.type);
    return property;
}

TypedArray<Dictionary> OScriptDocData::_get_method_arguments_documentation(const std::vector<PropertyInfo>& p_properties)
{
    TypedArray<Dictionary> data;
    for (const PropertyInfo& property : p_properties)
        data.push_back(_create_property_documentation(property));

    return data;
}

String OScriptDocData::_get_method_return_type(const MethodInfo& p_method)
{
    if (p_method.return_val.type == Variant::NIL)
        return p_method.return_val.usage & PROPERTY_USAGE_NIL_IS_VARIANT ? "Variant" : "void";

    return Variant::get_type_name(p_method.return_val.type);
}

Dictionary OScriptDocData::_method_info_documentation(const MethodInfo& p_method, const String& p_description)
{
    Dictionary data;
    data["name"] = p_method.name;
    data["description"] = p_description;
    data["return_type"] = _get_method_return_type(p_method);
    data["arguments"] = _get_method_arguments_documentation(p_method.arguments);
    return data;
}

TypedArray<Dictionary> OScriptDocData::_create_properties_documentation(const Ref<OScript>& p_script)
{
    TypedArray<Dictionary> data;
    for (const Ref<OScriptVariable>& variable : p_script->get_variables())
    {
        Dictionary property_data = _create_property_documentation(variable->get_info());
        property_data["description"] = variable->get_description();

        data.push_back(property_data);
    }
    return data;
}

TypedArray<Dictionary> OScriptDocData::_create_signals_documentation(const Ref<OScript>& p_script)
{
    TypedArray<Dictionary> data;
    for (const Ref<OScriptSignal>& signal : p_script->get_custom_signals())
    {
        const MethodInfo& mi = signal->get_method_info();
        data.push_back(_method_info_documentation(mi, signal->get("description")));
    }
    return data;
}

TypedArray<Dictionary> OScriptDocData::_create_functions_documentation(const Ref<OScript>& p_script)
{
    TypedArray<Dictionary> data;
    for (const Ref<OScriptFunction>& function : p_script->get_functions())
    {
        const MethodInfo& mi = function->get_method_info();
        data.push_back(_method_info_documentation(mi, function->get("description")));
    }
    return data;
}

TypedArray<Dictionary> OScriptDocData::create_documentation(const Ref<OScript>& p_script)
{
    Dictionary data;
    data["name"] = vformat("\"%s\"", p_script->get_path().replace("res://", ""));
    data["inherits"] = p_script->get_base_type();
    data["brief_description"] = p_script->get_brief_description();
    data["description"] = p_script->get_description();
    data["methods"] = _create_functions_documentation(p_script);
    data["signals"] = _create_signals_documentation(p_script);
    data["properties"] = _create_properties_documentation(p_script);
    data["is_deprecated"] = false;
    data["is_experimental"] = false;
    data["is_script_doc"] = true;
    data["script_path"] = p_script->get_path();

    // We currently only support 1 class per Orchestration
    return Array::make(data);
}

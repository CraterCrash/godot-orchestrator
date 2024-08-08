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
#include "script/signals.h"

#include "common/dictionary_utils.h"
#include "common/variant_utils.h"
#include "script/script.h"

void OScriptSignal::_get_property_list(List<PropertyInfo>* r_list) const
{
    static String types = VariantUtils::to_enum_list();

    // Properties that are serialized (not visible in Editor)
    r_list->push_back(PropertyInfo(Variant::DICTIONARY, "method", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));

    // Editor-only properties
    r_list->push_back(PropertyInfo(Variant::STRING, "signal_name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY));
    r_list->push_back(PropertyInfo(Variant::STRING, "Inputs", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_CATEGORY));
    r_list->push_back(PropertyInfo(Variant::DICTIONARY, "inputs", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR));
}

bool OScriptSignal::_get(const StringName &p_name, Variant &r_value)
{
    if (p_name.match("method"))
    {
        r_value = DictionaryUtils::from_method(_method, true);
        return true;
    }
    else if (p_name.match("signal_name"))
    {
        r_value = _method.name;
        return true;
    }
    else if (p_name.match("inputs"))
    {
        TypedArray<Dictionary> properties;
        for (const PropertyInfo& property : _method.arguments)
            properties.push_back(DictionaryUtils::from_property(property));

        r_value = properties;
        return true;
    }
    return false;
}

bool OScriptSignal::_set(const StringName &p_name, const Variant &p_value)
{
    if (p_name.match("method"))
    {
        _method = DictionaryUtils::to_method(p_value);
        emit_changed();
        return true;
    }
    else if (p_name.match("signal_name"))
    {
        _method.name = p_value;
        emit_changed();
        return true;
    }
    else if (p_name.match("inputs"))
    {
        TypedArray<Dictionary> properties = p_value;
        const bool refresh_required = _method.arguments.size() != size_t(properties.size());

        _method.arguments.resize(properties.size());
        for (int index = 0; index < properties.size(); ++index)
            _method.arguments[index] = DictionaryUtils::to_property(properties[index]);

        emit_changed();

        if (refresh_required)
            notify_property_list_changed();

        return true;
    }
    return false;
}

Orchestration* OScriptSignal::get_orchestration() const
{
    return _orchestration;
}

const StringName& OScriptSignal::get_signal_name() const
{
    return _method.name;
}

void OScriptSignal::rename(const StringName &p_new_name)
{
    if (_method.name != p_new_name)
    {
        _method.name = p_new_name;
        emit_changed();
    }
}

const MethodInfo& OScriptSignal::get_method_info() const
{
    return _method;
}

size_t OScriptSignal::get_argument_count() const
{
    return _method.arguments.size();
}


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
#include "signals.h"

#include "common/dictionary_utils.h"
#include "common/variant_utils.h"
#include "script/script.h"

void OScriptSignal::_get_property_list(List<PropertyInfo>* r_list) const
{
    static String types = VariantUtils::to_enum_list();

    // Properties that are serialized (not visible in Editor)
    r_list->push_back(PropertyInfo(Variant::DICTIONARY, "method", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));

    // Editor-only properties
    int32_t read_only = PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY;
    r_list->push_back(PropertyInfo(Variant::STRING, "signal_name", PROPERTY_HINT_NONE, "", read_only));
    r_list->push_back(PropertyInfo(Variant::INT, "argument_count", PROPERTY_HINT_RANGE, "0,32", PROPERTY_USAGE_EDITOR));
    for (size_t i = 1; i <= _method.arguments.size(); i++)
    {
        r_list->push_back(PropertyInfo(Variant::INT, "argument_" + itos(i) + "/type", PROPERTY_HINT_ENUM, types,
                                       PROPERTY_USAGE_EDITOR));
        r_list->push_back(PropertyInfo(Variant::STRING, "argument_" + itos(i) + "/name", PROPERTY_HINT_NONE, "",
                                       PROPERTY_USAGE_EDITOR));
    }
}

bool OScriptSignal::_get(const StringName &p_name, Variant &r_value)
{
    if (p_name.match("method"))
    {
        r_value = DictionaryUtils::from_method(_method);
        return true;
    }
    else if (p_name.match("signal_name"))
    {
        r_value = _method.name;
        return true;
    }
    else if (p_name.match("argument_count"))
    {
        r_value = static_cast<int64_t>(_method.arguments.size());
        return true;
    }
    else if (p_name.begins_with("argument_"))
    {
        const size_t index = p_name.get_slicec('_', 1).get_slicec('/', 0).to_int() - 1;
        ERR_FAIL_INDEX_V(index, _method.arguments.size(), false);

        const String what = p_name.get_slicec('/', 1);
        if (what == "type")
        {
            r_value = _method.arguments[index].type;
            return true;
        }
        else if (what == "name")
        {
            r_value = _method.arguments[index].name;
            return true;
        }
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
    else if (p_name.match("argument_count"))
    {
        if (resize_argument_list(static_cast<int64_t>(p_value)))
            notify_property_list_changed();
        return true;
    }
    else if (p_name.begins_with("argument_"))
    {
        const size_t index = p_name.get_slicec('_', 1).get_slicec('/', 0).to_int() - 1;
        ERR_FAIL_INDEX_V(index, _method.arguments.size(), false);

        const String what = p_name.get_slicec('/', 1);
        if (what == "type")
        {
            set_argument_type(index, VariantUtils::to_type(p_value));
            return true;
        }
        else if (what == "name")
        {
            set_argument_name(index, p_value);
            return true;
        }
    }
    return false;
}

Ref<OScript> OScriptSignal::get_owning_script() const
{
    return _script;
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

bool OScriptSignal::resize_argument_list(size_t p_new_size)
{
    bool result = false;

    const size_t current_size = get_argument_count();
    if (p_new_size > current_size)
    {
        _method.arguments.resize(p_new_size);
        for (size_t i = current_size; i < p_new_size; i++)
        {
            _method.arguments[i].name = "arg" + itos(i + 1);
            _method.arguments[i].type = Variant::NIL;
        }
        result = true;
    }
    else if (p_new_size < current_size)
    {
        _method.arguments.resize(p_new_size);
        result = true;
    }

    if (result)
        emit_changed();

    return result;
}

void OScriptSignal::set_argument_type(size_t p_index, Variant::Type p_type)
{
    if (_method.arguments.size() > p_index)
    {
        _method.arguments[p_index].type = p_type;
        emit_changed();
    }
}

void OScriptSignal::set_argument_name(size_t p_index, const StringName& p_name)
{
    if (_method.arguments.size() > p_index)
    {
        _method.arguments[p_index].name = p_name;
        emit_changed();
    }
}

Ref<OScriptSignal> OScriptSignal::create(OScript* p_script, const MethodInfo &p_method)
{
    Ref<OScriptSignal> signal(memnew(OScriptSignal));
    signal->_method = p_method;
    signal->_script = p_script;
    return signal;
}
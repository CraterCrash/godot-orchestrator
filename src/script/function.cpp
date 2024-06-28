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
#include "script/function.h"

#include "common/dictionary_utils.h"
#include "common/method_utils.h"
#include "nodes/functions/function_result.h"
#include "script/script.h"

void OScriptFunction::_get_property_list(List<PropertyInfo> *r_list) const
{
    r_list->push_back(PropertyInfo(Variant::STRING, "guid"));
    r_list->push_back(PropertyInfo(Variant::DICTIONARY, "method"));
    r_list->push_back(PropertyInfo(Variant::BOOL, "user_defined"));
    r_list->push_back(PropertyInfo(Variant::INT, "id"));
}

bool OScriptFunction::_get(const StringName &p_name, Variant &r_value)
{
    if (p_name.match("guid"))
    {
        r_value = _guid.to_string();
        return true;
    }
    else if (p_name.match("method"))
    {
        r_value = DictionaryUtils::from_method(_method, true);
        return true;
    }
    else if (p_name.match("id"))
    {
        r_value = _owning_node_id;
        return true;
    }
    else if (p_name.match("user_defined"))
    {
        r_value = _user_defined;
        return true;
    }
    return false;
}

bool OScriptFunction::_set(const StringName &p_name, const Variant &p_value)
{
    bool result = false;
    if (p_name.match("guid"))
    {
        _guid = Guid(p_value);
        result = true;
    }
    else if (p_name.match("method"))
    {
        _method = DictionaryUtils::to_method(p_value);
        _returns_value = MethodUtils::has_return_value(_method);

        // Cleanup the argument usage flags that were constructed incorrectly due to godot-cpp bug
        for (PropertyInfo& argument : _method.arguments)
        {
            if (argument.usage == 7)
                argument.usage = PROPERTY_USAGE_DEFAULT;
        }

        // Cleanup return value usage flags that were constructed incorrectly due to godot-cpp bug
        if (_method.return_val.usage == 7)
            _method.return_val.usage = PROPERTY_USAGE_DEFAULT;

        result = true;
    }
    else if (p_name.match("id"))
    {
        _owning_node_id = p_value;
        result = true;
    }
    else if (p_name.match("user_defined"))
    {
        _user_defined = p_value;
        result = true;
    }

    if (result)
        emit_changed();

    return result;
}

const StringName& OScriptFunction::get_function_name() const
{
    return _method.name;
}

bool OScriptFunction::can_be_renamed() const
{
    return _user_defined;
}

void OScriptFunction::rename(const StringName &p_new_name)
{
    if (can_be_renamed() && (_method.name != p_new_name))
    {
        _method.name = p_new_name;
        emit_changed();
    }
}

const Guid& OScriptFunction::get_guid() const
{
    return _guid;
}

const MethodInfo& OScriptFunction::get_method_info() const
{
    return _method;
}

bool OScriptFunction::is_user_defined() const
{
    return _user_defined;
}

Orchestration* OScriptFunction::get_orchestration() const
{
    return _orchestration;
}

int OScriptFunction::get_owning_node_id() const
{
    return _owning_node_id;
}

Ref<OScriptNode> OScriptFunction::get_owning_node() const
{
    return _orchestration->get_node(_owning_node_id);
}

Ref<OScriptNode> OScriptFunction::get_return_node() const
{
    const Vector<Ref<OScriptNode>> nodes = get_return_nodes();
    return nodes.is_empty() ? Ref<OScriptNode>() : nodes[0];
}

Vector<Ref<OScriptNode>> OScriptFunction::get_return_nodes() const
{
    Vector<Ref<OScriptNode>> results;

    const Ref<OScriptGraph> graph = get_function_graph();
    if (graph.is_valid())
    {
        for (const Ref<OScriptNode>& node : graph->get_nodes())
        {
            const Ref<OScriptNodeFunctionResult> result = node;
            if (result.is_valid())
                results.push_back(result);
        }
    }
    return results;
}

Ref<OScriptGraph> OScriptFunction::get_function_graph() const
{
    if (_orchestration->has_graph(get_function_name()))
        return _orchestration->get_graph(get_function_name());

    return {};
}

Dictionary OScriptFunction::to_dict() const
{
    Dictionary result = DictionaryUtils::from_method(_method);
    result["_oscript_guid"] = _guid.to_string();
    result["_oscript_owning_node_id"] = _owning_node_id;
    return result;
}

size_t OScriptFunction::get_argument_count() const
{
    return _method.arguments.size();
}

bool OScriptFunction::resize_argument_list(size_t p_new_size)
{
    bool result = false;
    if (_user_defined)
    {
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
    }

    if (result)
        emit_changed();

    return result;
}

void OScriptFunction::set_argument_type(size_t p_index, Variant::Type p_type)
{
    if (_method.arguments.size() > p_index && _user_defined)
    {
        _method.arguments[p_index].type = p_type;
        emit_changed();
    }
}

void OScriptFunction::set_argument_name(size_t p_index, const StringName& p_name)
{
    if (_method.arguments.size() > p_index && _user_defined)
    {
        _method.arguments[p_index].name = p_name;
        emit_changed();
    }
}

bool OScriptFunction::has_return_type() const
{
    return _returns_value;
}

Variant::Type OScriptFunction::get_return_type() const
{
    return _method.return_val.type;
}

void OScriptFunction::set_return_type(Variant::Type p_type)
{
    if (_user_defined && _method.return_val.type != p_type)
    {
        if (_returns_value)
            MethodUtils::set_return_value_type(_method, p_type);
        else
            MethodUtils::set_no_return_value(_method);

        emit_changed();
    }
}

void OScriptFunction::set_has_return_value(bool p_has_return_value)
{
    if (_returns_value != p_has_return_value)
    {
        if (p_has_return_value)
            MethodUtils::set_return_value(_method);
        else
            MethodUtils::set_no_return_value(_method);

        _returns_value = p_has_return_value;
        emit_changed();
    }
}

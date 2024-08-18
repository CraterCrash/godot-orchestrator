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
#include "variable.h"

void OScriptNodeVariableBase::_get_property_list(List<PropertyInfo>* r_list) const
{
    r_list->push_back(PropertyInfo(Variant::STRING, "variable_name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
}

bool OScriptNodeVariableBase::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("variable_name"))
    {
        r_value = _variable_name;
        return true;
    }
    return false;
}

bool OScriptNodeVariableBase::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("variable_name"))
    {
        _variable_name = p_value;
        return true;
    }
    return false;
}

void OScriptNodeVariableBase::initialize(const OScriptNodeInitContext& p_context)
{
    ERR_FAIL_COND_MSG(!p_context.variable_name, "Failed to initialize Variable without a variable name");
    _variable_name = p_context.variable_name.value();

    _lookup_and_set_variable(_variable_name);

    super::initialize(p_context);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeScriptVariableBase::_lookup_and_set_variable(const StringName& p_variable_name)
{
    Ref<OScriptVariable> variable = get_orchestration()->get_variable(p_variable_name);
    if (variable.is_valid())
    {
        _variable = variable;
        if (_is_in_editor())
            _variable->connect("changed", callable_mp(this, &OScriptNodeScriptVariableBase::_on_variable_changed));
    }
}

void OScriptNodeScriptVariableBase::_on_variable_changed()
{
    if (_variable.is_valid())
    {
        _variable_name = _variable->get_variable_name();
        reconstruct_node();

        // This must be triggered after reconstruction
        _variable_changed();
    }
}

void OScriptNodeScriptVariableBase::post_initialize()
{
    if (!_variable_name.is_empty())
        _lookup_and_set_variable(_variable_name);

    super::post_initialize();
}

void OScriptNodeScriptVariableBase::validate_node_during_build(BuildLog& p_log) const
{
    const Ref<OScriptVariable> variable = get_orchestration()->get_variable(_variable_name);
    if (!variable.is_valid())
        p_log.error(this, "Variable is no longer defined.");

    super::validate_node_during_build(p_log);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeLocalVariableBase::_get_property_list(List<PropertyInfo>* r_list) const
{
    r_list->push_back(PropertyInfo(Variant::STRING, "guid", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
}

bool OScriptNodeLocalVariableBase::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("guid"))
    {
        r_value = _function_guid.to_string();
        return true;
    }
    return false;
}

bool OScriptNodeLocalVariableBase::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("guid"))
    {
        _function_guid = Guid(p_value);
        return true;
    }
    return false;
}

void OScriptNodeLocalVariableBase::_lookup_and_set_variable(const StringName& p_variable_name)
{
    const Ref<OScriptFunction> function = get_function();
    if (!function.is_valid())
        return;

    const Ref<OScriptLocalVariable> variable = function->get_local_variable(p_variable_name);
    if (!variable.is_valid())
        return;

    _variable = variable;

    if (_is_in_editor())
        _variable->connect("changed", callable_mp(this, &OScriptNodeLocalVariableBase::_on_variable_changed));
}

void OScriptNodeLocalVariableBase::_on_variable_changed()
{
    if (_variable.is_valid())
    {
        _variable_name = _variable->get_variable_name();
        reconstruct_node();

        // This must be triggered after reconstruction
        _variable_changed();
    }
}

void OScriptNodeLocalVariableBase::post_initialize()
{
    _lookup_and_set_variable(_variable_name);

    super::post_initialize();
}

void OScriptNodeLocalVariableBase::initialize(const OScriptNodeInitContext& p_context)
{
    ERR_FAIL_COND_MSG(!p_context.user_data, "Must supply user data to create a local variable node.");
    ERR_FAIL_COND_MSG(!p_context.user_data.value().has("function_guid"), "Must have a function guid.");

    _function_guid = Guid(p_context.user_data.value()["function_guid"]);

    super::initialize(p_context);
}

void OScriptNodeLocalVariableBase::validate_node_during_build(BuildLog& p_log) const
{
    if (!_function_guid.is_valid())
        p_log.error(this, "Function reference is invalid.");

    if (_function_guid.is_valid())
    {
        const Ref<OScriptFunction> function = get_function();
        if (!function.is_valid())
            p_log.error(this, "Function is no longer defined.");

        if (function.is_valid())
        {
            const Ref<OScriptLocalVariable> variable = function->get_local_variable(_variable_name);
            if (!variable.is_valid())
                p_log.error(this, "Local variable is no longer defined.");
        }
    }

    super::validate_node_during_build(p_log);
}

Ref<OScriptFunction> OScriptNodeLocalVariableBase::get_function() const
{
    if (!_function_guid.is_valid())
        return nullptr;

    return get_orchestration()->find_function(_function_guid);
}

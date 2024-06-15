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
#include "call_script_function.h"

void OScriptNodeCallScriptFunction::_on_function_changed()
{
    if (_function.is_valid())
    {
        _reference.name = _function->get_function_name();
        _reference.method = _function->get_method_info();
        _reference.return_type = _function->get_return_type();
    }
    reconstruct_node();
}

void OScriptNodeCallScriptFunction::post_initialize()
{
    if (_reference.guid.is_valid())
    {
        _function = get_orchestration()->find_function(_reference.guid);
        if (_function.is_valid())
        {
            _reference.name = _function->get_function_name();
            _reference.return_type = _function->get_return_type();
            _reference.method = _function->get_method_info();
            _function_flags.set_flag(FF_IS_SELF);
            if (_is_in_editor())
            {
                Callable callable = callable_mp(this, &OScriptNodeCallScriptFunction::_on_function_changed);
                if (!_function->is_connected("changed", callable))
                    _function->connect("changed", callable);
            }
        }
    }
    super::post_initialize();
}

void OScriptNodeCallScriptFunction::post_placed_new_node()
{
    super::post_placed_new_node();
    if (_function.is_valid() && _is_in_editor())
    {
        Callable callable = callable_mp(this, &OScriptNodeCallScriptFunction::_on_function_changed);
        if (!_function->is_connected("changed", callable))
            _function->connect("changed", callable);
    }
}

String OScriptNodeCallScriptFunction::get_tooltip_text() const
{
    return vformat("Target is %s", get_orchestration()->get_base_type());
}

String OScriptNodeCallScriptFunction::get_node_title() const
{
    if (_function.is_valid())
        return vformat("%s", _function->get_function_name().capitalize());

    return super::get_node_title();
}

Object* OScriptNodeCallScriptFunction::get_jump_target_for_double_click() const
{
    if (_function.is_valid())
        return _function.ptr();

    return super::get_jump_target_for_double_click();
}

bool OScriptNodeCallScriptFunction::can_jump_to_definition() const
{
    return get_jump_target_for_double_click() != nullptr;
}

void OScriptNodeCallScriptFunction::validate_node_during_build(BuildLog& p_log) const
{
    super::validate_node_during_build(p_log);

    if (!_function.is_valid())
        p_log.error("There is no function instance defined.");
}

bool OScriptNodeCallScriptFunction::can_inspect_node_properties() const
{
    if (_function.is_valid() && !_function->get_function_name().is_empty())
    {
        if (get_orchestration()->has_graph(_function->get_function_name()))
            return true;
    }
    return false;
}

void OScriptNodeCallScriptFunction::initialize(const OScriptNodeInitContext& p_context)
{
    ERR_FAIL_COND_MSG(!p_context.method, "Failed to initialize CallScriptFunction without a MethodInfo");

    const MethodInfo& mi = p_context.method.value();
    _function = get_orchestration()->find_function(mi.name);
    if (_function.is_valid())
    {
        _reference.guid = _function->get_guid();
        _reference.name = _function->get_function_name();
        _reference.method = _function->get_method_info();
        _reference.return_type = _function->get_return_type();
        _function_flags.set_flag(FF_IS_SELF);
    }

    super::initialize(p_context);
}
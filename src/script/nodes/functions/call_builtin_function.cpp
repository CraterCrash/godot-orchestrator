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
#include "call_builtin_function.h"

#include "common/dictionary_utils.h"
#include "common/method_utils.h"

OScriptNodeCallBuiltinFunction::OScriptNodeCallBuiltinFunction()
{
    _flags = ScriptNodeFlags::CATALOGABLE;
}

bool OScriptNodeCallBuiltinFunction::_has_execution_pins(const MethodInfo& p_method) const
{
    return !MethodUtils::has_return_value(p_method);
}

void OScriptNodeCallBuiltinFunction::post_initialize()
{
    _reference.name = _reference.method.name;
    _reference.return_type = _reference.method.return_val.type;

    super::post_initialize();
}

String OScriptNodeCallBuiltinFunction::get_tooltip_text() const
{
    if (!_reference.method.name.is_empty())
        return vformat("Calls the built-in Godot function '%s'", _reference.method.name);
    else
        return "Calls the specified built-in Godot function";
}

String OScriptNodeCallBuiltinFunction::get_node_title() const
{
    return vformat("Call %s", _reference.method.name.capitalize());
}

void OScriptNodeCallBuiltinFunction::initialize(const OScriptNodeInitContext& p_context)
{
    ERR_FAIL_COND_MSG(!p_context.user_data, "Failed to initialize BuiltInFunction without data");

    const Dictionary& data = p_context.user_data.value();
    ERR_FAIL_COND_MSG(!data.has("name"), "MethodInfo is incomplete.");

    const MethodInfo mi = DictionaryUtils::to_method(data);
    _reference.method = mi;
    _reference.name = _reference.method.name;
    _reference.return_type = _reference.method.return_val.type;

    _function_flags = FunctionFlags::FF_PURE;
    _set_function_flags(_reference.method);

    super::initialize(p_context);
}

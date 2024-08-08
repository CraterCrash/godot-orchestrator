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
#include "call_builtin_function.h"

#include "common/dictionary_utils.h"
#include "common/method_utils.h"
#include "common/version.h"

OScriptNodeCallBuiltinFunction::OScriptNodeCallBuiltinFunction()
{
    _flags = ScriptNodeFlags::CATALOGABLE;
    _function_flags = FunctionFlags::FF_PURE;
}

bool OScriptNodeCallBuiltinFunction::_has_execution_pins(const MethodInfo& p_method) const
{
    return !MethodUtils::has_return_value(p_method);
}

String OScriptNodeCallBuiltinFunction::get_tooltip_text() const
{
    if (!_reference.method.name.is_empty())
        return vformat("Calls the built-in Godot function '%s'", _reference.method.name);

    return "Calls the specified built-in Godot function";
}

String OScriptNodeCallBuiltinFunction::get_node_title() const
{
    return vformat("%s", _reference.method.name.capitalize());
}

String OScriptNodeCallBuiltinFunction::get_help_topic() const
{
    #if GODOT_VERSION >= 0x040300
    return vformat("class_method:@GlobalScope:%s", _reference.method.name);
    #else
    return super::get_help_topic();
    #endif
}

void OScriptNodeCallBuiltinFunction::initialize(const OScriptNodeInitContext& p_context)
{
    ERR_FAIL_COND_MSG(!p_context.user_data, "Failed to initialize BuiltInFunction without data");

    const Dictionary& data = p_context.user_data.value();
    ERR_FAIL_COND_MSG(!data.has("name"), "MethodInfo is incomplete.");

    const MethodInfo mi = DictionaryUtils::to_method(data);
    _reference.method = mi;

    _set_function_flags(_reference.method);

    super::initialize(p_context);
}

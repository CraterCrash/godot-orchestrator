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
#ifndef ORCHESTRATOR_SCRIPT_NODE_CALL_BUILTIN_FUNCTION_H
#define ORCHESTRATOR_SCRIPT_NODE_CALL_BUILTIN_FUNCTION_H

#include "call_function.h"

/// Supports calling a built-in Godot function
class OScriptNodeCallBuiltinFunction : public OScriptNodeCallFunction
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeCallBuiltinFunction, OScriptNodeCallFunction);

    static void _bind_methods() {}

protected:

    //~ Begin OScriptNodeCallFunction Interface
    bool _has_execution_pins(const MethodInfo& p_method) const override;
    //~ End OScriptNodeCallFunction Interface

public:
    OScriptNodeCallBuiltinFunction();

    //~ Begin OScriptNode Interface
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "pure_function_call"; }
    String get_help_topic() const override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    //~ End OScriptNode Interface

};

#endif  // ORCHESTRATOR_SCRIPT_NODE_CALL_BUILTIN_FUNCTION_H

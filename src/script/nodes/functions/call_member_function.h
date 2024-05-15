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
#ifndef ORCHESTRATOR_SCRIPT_NODE_CALL_MEMBER_FUNCTION_H
#define ORCHESTRATOR_SCRIPT_NODE_CALL_MEMBER_FUNCTION_H

#include "call_function.h"

/// An implementation of OrchestratorScript CallFunction node that calls a method
/// on a Godot object.
class OScriptNodeCallMemberFunction : public OScriptNodeCallFunction
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeCallMemberFunction, OScriptNodeCallFunction);

    static void _bind_methods() {}

public:
    OScriptNodeCallMemberFunction();

    //~ Begin OScriptNode Interface
    void post_initialize() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "function_call"; }
    void initialize(const OScriptNodeInitContext& p_context) override;
    void on_pin_connected(const Ref<OScriptNodePin>& p_pin) override;
    void on_pin_disconnected(const Ref<OScriptNodePin>& p_pin) override;
    //~ End OScriptNode Interface

    /// Get the target function class
    /// @return the target function class
    String get_target_class() const { return _reference.target_class_name; }

    /// Get the Godot function reference
    /// @return the function reference
    const MethodInfo& get_function() const { return _reference.method; }

};

#endif  // ORCHESTRATOR_SCRIPT_NODE_CALL_MEMBER_FUNCTION_H

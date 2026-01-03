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
#ifndef ORCHESTRATOR_SCRIPT_NODE_CALL_SCRIPT_FUNCTION_H
#define ORCHESTRATOR_SCRIPT_NODE_CALL_SCRIPT_FUNCTION_H

#include "call_function.h"

/// An implementation of OrchestratorScript CallFunction node that calls functions
/// that are defined as a part of an Orchestration script.
class OScriptNodeCallScriptFunction : public OScriptNodeCallFunction {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeCallScriptFunction, OScriptNodeCallFunction);

    Ref<OScriptFunction> _function;

protected:
    static void _bind_methods() {}

    /// Called when the script function is modified
    void _on_function_changed();

    //~ Begin OScriptNodeCallFunction Interface
    bool _is_method_info_serialized() const override { return false; }
    bool _use_argument_class_name() const override { return false; }
    int get_argument_count() const override;
    //~ End OScriptNodeCallFunction Interface

public:

    //~ Begin OScriptNode Interface
    void post_initialize() override;
    void post_placed_new_node() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "orchestration_function_call"; }
    Object* get_jump_target_for_double_click() const override;
    bool can_jump_to_definition() const override;
    void validate_node_during_build(BuildLog& p_log) const override;
    bool can_inspect_node_properties() const override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    //~ End OScriptNode Interface

    //~ Begin OScriptNodeCallFunction Interface
    MethodInfo get_method_info() override;
    //~ End OScriptNodeCallFunction Interface

    /// Get the function reference
    /// @return the script function reference the node calls
    Ref<OScriptFunction> get_function() { return _function; }
};

#endif  // ORCHESTRATOR_SCRIPT_NODE_CALL_SCRIPT_FUNCTION_H

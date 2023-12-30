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
#ifndef ORCHESTRATOR_SCRIPT_NODE_FUNCTION_RESULT_H
#define ORCHESTRATOR_SCRIPT_NODE_FUNCTION_RESULT_H

#include "function_terminator.h"

/// Handles the result of a function call.
///
/// A function definition within a function graph can optionally return values, and
/// when a value is returned, the graph can maintain only one function result node
/// which acts as the end terminator for the function's control flow.
///
/// In UE, when adding return values to the function definition, if a return node
/// does not exist, a new return node will be created and automatically linked to
/// the entry node if the entry node's execution pin is not yet connected.
///
/// By removing all output parameters for the function call, the return node will
/// automatically be removed from the function graph.
///
class OScriptNodeFunctionResult : public OScriptNodeFunctionTerminator
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeFunctionResult, OScriptNodeFunctionTerminator);

public:
    //~ Begin OScriptNode Interface
    void pre_remove() override;
    void allocate_default_pins() override;
    String get_node_title() const override;
    String get_tooltip_text() const override;
    bool validate_node_during_build() const override;
    bool draw_node_as_exit() const override { return true; }
    bool is_compatible_with_graph(const Ref<OScriptGraph>& p_graph) const override;
    void post_placed_new_node() override;
    bool can_user_delete_node() const override;
    OScriptNodeInstance* instantiate(OScriptInstance* p_instance) override;
    //~ End OScriptNode Interface

    OScriptNodeFunctionResult();
};

#endif  // ORCHESTRATOR_SCRIPT_NODE_FUNCTION_RESULT_H

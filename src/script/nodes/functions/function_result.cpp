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
#include "script/nodes/functions/function_result.h"

#include "common/property_utils.h"
#include "script/nodes/flow_control/sequence.h"

void OScriptNodeFunctionResult::pre_remove() {
    // When this node is removed, clear the function's return value if this is the last return node
    Ref<OScriptFunction> function = get_function();
    if (function.is_valid()) {
        Vector<Ref<OScriptNode>> return_nodes = function->get_return_nodes();
        if (return_nodes.size() == 1) {
            _function->set_has_return_value(false);
        }
    }
}

void OScriptNodeFunctionResult::allocate_default_pins() {
    create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"));

    Ref<OScriptFunction> function = get_function();
    if (function.is_valid()) {
        create_pins_for_function_entry_exit(function, false);
    }
    super::allocate_default_pins();
}

String OScriptNodeFunctionResult::get_node_title() const {
    return "Return Node";
}

String OScriptNodeFunctionResult::get_tooltip_text() const {
    return "The node terminates the function's execution and returns any output values.";
}

bool OScriptNodeFunctionResult::is_compatible_with_graph(const Ref<OScriptGraph>& p_graph) const {
    return p_graph.is_valid() && p_graph->get_flags().has_flag(OScriptGraph::GF_FUNCTION);
}

void OScriptNodeFunctionResult::post_placed_new_node() {
    const Ref<OScriptGraph> graph = get_owning_graph();
    if (graph.is_valid() && graph->get_flags().has_flag(OScriptGraph::GF_FUNCTION)) {
        // There is only ever 1 function node in a function graph and the function node cannot
        // be deleted by the user, and so we can safely look that up on the graph's metadata.
        const Vector<Ref<OScriptFunction>> functions = graph->get_functions();
        if (!functions.is_empty()) {
            _function = functions[0];
            _guid = _function->get_guid();
            reconstruct_node();
        }
    }
    super::post_placed_new_node();
}

bool OScriptNodeFunctionResult::can_user_delete_node() const {
    Ref<OScriptFunction> function = get_function();
    if (function.is_valid() && !function->is_user_defined()) {
        return false;
    }
    return true;
}

OScriptNodeFunctionResult::OScriptNodeFunctionResult() {
    _flags.set_flag(CATALOGABLE);
}
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
#include "function_result.h"

#include "common/property_utils.h"

class OScriptNodeFunctionResultInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeFunctionResult);
    bool _has_return{ false };

public:

    int get_working_memory_size() const override { return 1; }

    int step(OScriptExecutionContext& p_context) override
    {
        if (_has_return)
        {
            p_context.set_working_memory(0, p_context.get_input(0));
            return STEP_FLAG_END;
        }
        else
        {
            p_context.set_working_memory(0, Variant());
            return 0;
        }
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeFunctionResult::pre_remove()
{
    // When this node is removed, clear the function's return value if this is the last return node
    Ref<OScriptFunction> function = get_function();
    if (function.is_valid())
    {
        Vector<Ref<OScriptNode>> return_nodes = function->get_return_nodes();
        if (return_nodes.size() == 1)
            _function->set_has_return_value(false);
    }
}

void OScriptNodeFunctionResult::allocate_default_pins()
{
    create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"));

    Ref<OScriptFunction> function = get_function();
    if (function.is_valid())
        create_pins_for_function_entry_exit(function, false);

    super::allocate_default_pins();
}

String OScriptNodeFunctionResult::get_node_title() const
{
    return "Return Node";
}

String OScriptNodeFunctionResult::get_tooltip_text() const
{
    return "The node terminates the function's execution and returns any output values.";
}

void OScriptNodeFunctionResult::validate_node_during_build(BuildLog& p_log) const
{
    super::validate_node_during_build(p_log);

    const Ref<OScriptFunction> function = get_function();
    if (function.is_valid())
    {
        const String function_name = function->get_function_name();
        for (const Ref<OScriptNodePin>& pin : get_all_pins())
        {
            // Check hidden first because those are not assigned cached pin indices
            if (!pin->is_hidden() && !pin->has_any_connections())
            {
                if (pin->get_property_info().type >= Variant::RID)
                    p_log.error(this, pin, "Requires a connection.");
            }
        }

        // Nothing stops a user from adding multiple return nodes to a function graph
        // The control flow connection validation should only be called once per function
        if (function->get_return_node().ptr() == this)
        {
            // Collect all nodes that participate in the function graph
            HashMap<int, Ref<OScriptNode>> graph_nodes;
            for (const Ref<OScriptNode>& node : function->get_function_graph()->get_nodes())
                graph_nodes[node->get_id()] = node;

            // Collect all control flow connections in the function graph
            RBSet<OScriptConnection> control_flows;
            for (const OScriptConnection& E : function->get_orchestration()->get_connections())
            {
                if (graph_nodes.has(E.from_node))
                {
                    const Ref<OScriptNode> node = graph_nodes[E.from_node];
                    for (const Ref<OScriptNodePin>& output : node->find_pins(PD_Output))
                    {
                        if (output.is_valid() && output->is_execution() && E.from_port == output->get_pin_index())
                            control_flows.insert(E);
                    }
                }
            }

            RBSet<uint64_t> skipped;
            List<uint64_t> seen;

            seen.push_back(function->get_owning_node_id()); // starting node
            seen.push_back(_id); // return node

            // Traverse from the starting function node
            List<int> queue;
            queue.push_back(function->get_owning_node_id());
            while(!queue.is_empty())
            {
                const int current_id = queue.front()->get();

                for (const OScriptConnection& E : control_flows)
                {
                    const Ref<OScriptNode> source = graph_nodes[E.from_node];
                    if (source.is_valid())
                    {
                        if (source->is_loop_port(E.from_port))
                            skipped.insert(E.to_node);
                    }

                    if (skipped.has(E.from_node))
                        skipped.insert(E.to_node);

                    if (E.from_node == current_id && !seen.find(E.to_node))
                    {
                        queue.push_back(E.to_node);
                        seen.push_back(E.to_node);
                    }
                }
                queue.pop_front();
            }

            for (uint64_t node_id : seen)
            {
                if (skipped.has(node_id))
                    continue;

                const Ref<OScriptNode> node = graph_nodes[node_id];
                if (node.is_valid())
                {
                    for (const Ref<OScriptNodePin>& output : node->find_pins(PD_Output))
                    {
                        if (output.is_valid() && output->is_execution() && !output->has_any_connections() && !node->is_loop_port(output->get_pin_index()))
                            p_log.error(node.ptr(), output, "This pin should be connected to the return node.");
                    }
                }
            }
        }
    }
}

bool OScriptNodeFunctionResult::is_compatible_with_graph(const Ref<OScriptGraph>& p_graph) const
{
    return p_graph.is_valid() && p_graph->get_flags().has_flag(OScriptGraph::GF_FUNCTION);
}

void OScriptNodeFunctionResult::post_placed_new_node()
{
    const Ref<OScriptGraph> graph = get_owning_graph();
    if (graph.is_valid() && graph->get_flags().has_flag(OScriptGraph::GF_FUNCTION))
    {
        // There is only ever 1 function node in a function graph and the function node cannot
        // be deleted by the user, and so we can safely look that up on the graph's metadata.
        const Vector<Ref<OScriptFunction>> functions = graph->get_functions();
        if (!functions.is_empty())
        {
            _function = functions[0];
            _guid = _function->get_guid();
            reconstruct_node();
        }
    }

    super::post_placed_new_node();
}

bool OScriptNodeFunctionResult::can_user_delete_node() const
{
    Ref<OScriptFunction> function = get_function();
    if (function.is_valid() && !function->is_user_defined())
        return false;

    return true;
}

OScriptNodeInstance* OScriptNodeFunctionResult::instantiate()
{
    OScriptNodeFunctionResultInstance* i = memnew(OScriptNodeFunctionResultInstance);
    i->_node = this;
    i->_has_return = _function->has_return_type();
    return i;
}

OScriptNodeFunctionResult::OScriptNodeFunctionResult()
{
    _flags.set_flag(ScriptNodeFlags::CATALOGABLE);
}
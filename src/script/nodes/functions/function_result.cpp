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
#include "function_result.h"

class OScriptNodeFunctionResultInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeFunctionResult);
    bool _has_return{ false };

public:

    int get_working_memory_size() const override { return 1; }

    int step(OScriptNodeExecutionContext& p_context) override
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
    // When this node is removed, clear the function's return value
    Ref<OScriptFunction> function = get_function();
    if (function.is_valid())
        _function->set_return_type(Variant::NIL);
}

void OScriptNodeFunctionResult::allocate_default_pins()
{
    create_pin(PD_Input, "ExecIn")->set_flags(OScriptNodePin::Flags::EXECUTION);

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

bool OScriptNodeFunctionResult::validate_node_during_build() const
{
    if (!super::validate_node_during_build())
        return false;

    Ref<OScriptFunction> function = get_function();
    if (function.is_valid())
    {
        const String function_name = function->get_function_name();
        for (const Ref<OScriptNodePin>& pin : get_all_pins())
        {
            // Check hidden first because those are not assigned cached pin indices
            if (!pin->get_flags().has_flag(OScriptNodePin::Flags::HIDDEN) && !pin->has_any_connections())
            {
                ERR_PRINT("There is no connection to function " + function_name + " output pin " + pin->get_pin_name());
                return false;
            }
        }
    }
    return true;
}

bool OScriptNodeFunctionResult::is_compatible_with_graph(const Ref<OScriptGraph>& p_graph) const
{
    bool is_function_graph = p_graph.is_valid() && p_graph->get_flags().has_flag(OScriptGraph::GF_FUNCTION);
    if (is_function_graph)
    {
        TypedArray<int> node_ids = p_graph->get_nodes();
        for (int i = 0; i < node_ids.size(); i++)
        {
            Ref<OScriptNode> node = p_graph->get_owning_script()->get_node(node_ids[i]);
            if (node.is_valid())
            {
                Ref<OScriptNodeFunctionResult> result = node;
                if (result.is_valid())
                    return false;
            }
        }
        return true;
    }
    return false;
}

void OScriptNodeFunctionResult::post_placed_new_node()
{
    Ref<OScriptGraph> graph = get_owning_script()->find_graph(this);
    if (graph.is_valid() && graph->get_flags().has_flag(OScriptGraph::GF_FUNCTION))
    {
        // There is only ever 1 function node in a function graph and the function node cannot
        // be deleted by the user, and so we can safely look that up on the graph's metadata.
        int function_node_id = graph->get_functions()[0];
        Ref<OScriptNodeFunctionTerminator> func = get_owning_script()->get_node(function_node_id);
        if (func.is_valid())
        {
            _function = func->get_function();
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

OScriptNodeInstance* OScriptNodeFunctionResult::instantiate(OScriptInstance* p_instance)
{
    OScriptNodeFunctionResultInstance* i = memnew(OScriptNodeFunctionResultInstance);
    i->_node = this;
    i->_instance = p_instance;
    i->_has_return = _function->has_return_type();
    return i;
}

OScriptNodeFunctionResult::OScriptNodeFunctionResult()
{
    _flags.set_flag(ScriptNodeFlags::CATALOGABLE);
}
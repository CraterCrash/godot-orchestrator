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
#include "graph_node_spawner.h"

#include "common/callable_lambda.h"
#include "common/method_utils.h"
#include "graph_edit.h"
#include "graph_node_pin.h"
#include "script/nodes/script_nodes.h"

void OrchestratorGraphNodeSpawner::execute(OrchestratorGraphEdit* p_graph, const Vector2& p_position)
{
    ERR_PRINT("Spawner '" + get_class() + "' does not support placing nodes.");
}

bool OrchestratorGraphNodeSpawner::_has_all_filter_keywords(const Vector<String>& p_keywords, const PackedStringArray& p_values)
{
    bool has_all = true;
    for (const String& keyword : p_keywords)
    {
        bool found = false;
        for (const String& value : p_values)
        {
            if (value.contains(keyword))
            {
                found = true;
                break;
            }
        }
        if (!found)
            has_all = false;
    }
    return has_all;
}

bool OrchestratorGraphNodeSpawner::is_filtered(const OrchestratorGraphActionFilter& p_filter, const OrchestratorGraphActionSpec& p_spec)
{
    // Check whether any user-entered values filter the action text
    if (!p_filter.keywords.is_empty())
    {
        if (_has_all_filter_keywords(p_filter.keywords, p_spec.keywords.to_lower().split(",")))
            return false;

        if (_has_all_filter_keywords(p_filter.keywords, p_spec.text.to_lower().replace("_", " ").split(" ")))
            return false;

        return true;
    }

    // No data entered, don't apply any filters
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool OrchestratorGraphNodeSpawnerProperty::is_filtered(const OrchestratorGraphActionFilter& p_filter,
                                                 const OrchestratorGraphActionSpec& p_spec)
{
    if (p_filter.flags & OrchestratorGraphActionFilter::Filter_RejectProperties)
        return true;

    // Only expose properties that are visible to the editor and are exposed as script variables
    if (!(_property.usage & PROPERTY_USAGE_EDITOR) && !(_property.usage & PROPERTY_USAGE_SCRIPT_VARIABLE))
        return true;

    return OrchestratorGraphNodeSpawner::is_filtered(p_filter, p_spec);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OrchestratorGraphNodeSpawnerPropertyGet::execute(OrchestratorGraphEdit* p_graph, const Vector2& p_position)
{
    OScriptNodeInitContext context;
    context.property = _property;

    if (!_node_path.is_empty())
        context.node_path = _node_path;
    else if (!_target_classes.is_empty())
        context.class_name = _target_classes[0];

    p_graph->spawn_node<OScriptNodePropertyGet>(context, p_position);
}

bool OrchestratorGraphNodeSpawnerPropertyGet::is_filtered(const OrchestratorGraphActionFilter& p_filter,
                                                    const OrchestratorGraphActionSpec& p_spec)
{
    if (p_filter.context_sensitive && !p_filter.context.pins.is_empty())
    {
        // PropertyGet nodes return a specific type, and so if a pin is provided,
        // it must be an input node and have a compatible type.
        bool found = false;
        for (OrchestratorGraphNodePin* pin : p_filter.context.pins)
        {
            if (pin->is_input() && pin->get_value_type() == _property.type)
            {
                found = true;
                break;
            }
        }
        if (!found)
            return true;
    }

    return OrchestratorGraphNodeSpawnerProperty::is_filtered(p_filter, p_spec);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OrchestratorGraphNodeSpawnerPropertySet::execute(OrchestratorGraphEdit* p_graph, const Vector2& p_position)
{
    OScriptNodeInitContext context;
    context.property = _property;

    if (!_node_path.is_empty())
        context.node_path = _node_path;
    else if (!_target_classes.is_empty())
        context.class_name = _target_classes[0];

    p_graph->spawn_node<OScriptNodePropertySet>(context, p_position,
        callable_mp_lambda(this, [&, this](const Ref<OScriptNodePropertySet>& p_node) {
            if (p_node.is_valid() && _default_value)
                p_node->set_default_value(_default_value);
        }));
}

bool OrchestratorGraphNodeSpawnerPropertySet::is_filtered(const OrchestratorGraphActionFilter& p_filter,
                                                    const OrchestratorGraphActionSpec& p_spec)
{
    if (p_filter.context_sensitive && !p_filter.context.pins.is_empty())
    {
        // PropertySet nodes accept a specific type, and so if a pin is provided,
        // it must be an output node and have a compatible type.
        bool found = false;
        for (OrchestratorGraphNodePin* pin : p_filter.context.pins)
        {
            if (pin->is_output() && pin->get_value_type() == _property.type)
            {
                found = true;
                break;
            }
        }
        if (!found)
            return true;
    }

    return OrchestratorGraphNodeSpawnerProperty::is_filtered(p_filter, p_spec);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OrchestratorGraphNodeSpawnerCallMemberFunction::execute(OrchestratorGraphEdit* p_graph, const Vector2& p_position)
{
    OScriptNodeInitContext context;
    context.method = _method;
    context.class_name = _class_name;

    p_graph->spawn_node<OScriptNodeCallMemberFunction>(context, p_position);
}

bool OrchestratorGraphNodeSpawnerCallMemberFunction::is_filtered(const OrchestratorGraphActionFilter& p_filter,
                                                           const OrchestratorGraphActionSpec& p_spec)
{
    bool reject_methods = p_filter.flags & OrchestratorGraphActionFilter::Filter_RejectMethods;
    bool reject_virtual = p_filter.flags & OrchestratorGraphActionFilter::Filter_RejectVirtualMethods;

    // Simply do not add these virtual overrides to the UI
    // Users should always use the non-virtual "get" and "set methods only
    if (_method.name.match("_get") || _method.name.match("_set"))
        return true;

    if (reject_methods && reject_virtual)
        return true;

    if (reject_virtual && (_method.flags & METHOD_FLAG_VIRTUAL))
        return true;

    if (reject_methods && (!(_method.flags & METHOD_FLAG_VIRTUAL)))
        return true;

    if (p_filter.context_sensitive)
    {
        bool args_filtered = false;
        bool return_filtered = false;

        for (OrchestratorGraphNodePin* pin : p_filter.context.pins)
        {
            if (pin->is_output())
            {
                bool found = false;
                for (const PropertyInfo& property : _method.arguments)
                {
                    if (property.type == pin->get_value_type())
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                    args_filtered = true;
            }
            else
            {
                // If the method returns nothing but the pin is an output, this triggers
                // a unique state where we reject the method all together.
                if (!MethodUtils::has_return_value(_method))
                    return_filtered = true;

                if (!return_filtered && _method.return_val.type != pin->get_value_type())
                    return_filtered = true;
            }
        }

        if (args_filtered || return_filtered)
            return true;
    }

    return OrchestratorGraphNodeSpawner::is_filtered(p_filter, p_spec);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OrchestratorGraphNodeSpawnerCallScriptFunction::execute(OrchestratorGraphEdit* p_graph, const Vector2& p_position)
{
    OScriptNodeInitContext context;
    context.method = _method;

    p_graph->spawn_node<OScriptNodeCallScriptFunction>(context, p_position);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OrchestratorGraphNodeSpawnerEvent::execute(OrchestratorGraphEdit* p_graph, const Vector2& p_position)
{
    OScriptNodeInitContext context;
    context.method = _method;

    p_graph->spawn_node<OScriptNodeEvent>(context, p_position);
}

bool OrchestratorGraphNodeSpawnerEvent::is_filtered(const OrchestratorGraphActionFilter& p_filter, const OrchestratorGraphActionSpec& p_spec)
{
    if (p_filter.flags & OrchestratorGraphActionFilter::Filter_RejectEvents)
        return true;

    // Can only define the event function once
    if (Orchestration* orchestration = p_filter.get_orchestration())
    {
        if (orchestration->has_function(_method.name))
            return true;
    }

    bool reject_methods = p_filter.flags & OrchestratorGraphActionFilter::Filter_RejectMethods;
    bool reject_virtual = p_filter.flags & OrchestratorGraphActionFilter::Filter_RejectVirtualMethods;

    // Simply do not add these virtual overrides to the UI
    // Users should always use the non-virtual "get" and "set methods only
    if (_method.name.match("_get") || _method.name.match("_set"))
        return true;

    if (reject_methods && reject_virtual)
        return true;

    if (reject_virtual && (_method.flags & METHOD_FLAG_VIRTUAL))
        return true;

    if (reject_methods && (!(_method.flags & METHOD_FLAG_VIRTUAL)))
        return true;

    if (p_filter.context_sensitive)
    {
        bool args_filtered = false;
        for (OrchestratorGraphNodePin* pin : p_filter.context.pins)
        {
            if (pin->is_input())
            {
                bool found = false;
                for (const PropertyInfo& property : _method.arguments)
                {
                    if (property.type == pin->get_value_type())
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                    args_filtered = true;
            }
        }

        if (args_filtered)
            return true;
    }

    return OrchestratorGraphNodeSpawner::is_filtered(p_filter, p_spec);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OrchestratorGraphNodeSpawnerEmitMemberSignal::execute(OrchestratorGraphEdit* p_graph, const Vector2& p_position)
{
    // todo: encode as class_name?
    Dictionary data;
    data["target_class"] = _class_name;

    OScriptNodeInitContext context;
    context.method = _method;
    context.user_data = data;

    p_graph->spawn_node<OScriptNodeEmitMemberSignal>(context, p_position);
}

bool OrchestratorGraphNodeSpawnerEmitMemberSignal::is_filtered(const OrchestratorGraphActionFilter& p_filter,
                                                               const OrchestratorGraphActionSpec& p_spec)
{
    if (p_filter.flags & OrchestratorGraphActionFilter::Filter_RejectSignals)
        return true;

    return OrchestratorGraphNodeSpawnerCallMemberFunction::is_filtered(p_filter, p_spec);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OrchestratorGraphNodeSpawnerEmitSignal::execute(OrchestratorGraphEdit* p_graph, const Vector2& p_position)
{
    if (!p_graph->get_orchestration()->has_custom_signal(_method.name))
        return;

    OScriptNodeInitContext context;
    context.method = _method;

    p_graph->spawn_node<OScriptNodeEmitSignal>(context, p_position);
}

bool OrchestratorGraphNodeSpawnerEmitSignal::is_filtered(const OrchestratorGraphActionFilter& p_filter,
                                                   const OrchestratorGraphActionSpec& p_spec)
{
    if (p_filter.flags & OrchestratorGraphActionFilter::Filter_RejectSignals)
        return true;

    if (p_filter.context_sensitive && p_filter.target_type != Variant::NIL && !p_filter.context.pins.is_empty())
    {
        const OrchestratorGraphNodePin* pin = p_filter.context.pins[0];
        if (pin && pin->is_output())
        {
            if (Orchestration* orchestration = p_filter.get_orchestration())
            {
                Ref<OScriptSignal> signal = orchestration->find_custom_signal(_method.name);
                if (signal.is_valid() && signal->get_argument_count() > 0)
                {
                    bool filtered = true;
                    for (const PropertyInfo& property : signal->get_method_info().arguments)
                    {
                        if (property.type == p_filter.target_type)
                        {
                            filtered = false;
                            break;
                        }
                    }
                    if (filtered)
                        return true;
                }
            }
        }
    }
    return OrchestratorGraphNodeSpawnerCallMemberFunction::is_filtered(p_filter, p_spec);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool OrchestratorGraphNodeSpawnerVariable::is_filtered(const OrchestratorGraphActionFilter& p_filter,
                                                 const OrchestratorGraphActionSpec& p_spec)
{
    if (p_filter.flags & OrchestratorGraphActionFilter::Filter_RejectVariables)
        return true;

    return OrchestratorGraphNodeSpawner::is_filtered(p_filter, p_spec);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OrchestratorGraphNodeSpawnerVariableGet::execute(OrchestratorGraphEdit* p_graph, const Vector2& p_position)
{
    OScriptNodeInitContext context;
    context.variable_name = _variable_name;

    Dictionary data;
    data["validation"] = _validation;
    context.user_data = data;

    p_graph->spawn_node<OScriptNodeVariableGet>(context, p_position);
}

bool OrchestratorGraphNodeSpawnerVariableGet::is_filtered(const OrchestratorGraphActionFilter& p_filter,
                                                          const OrchestratorGraphActionSpec& p_spec)
{
    if (p_filter.context_sensitive && p_filter.target_type != Variant::NIL && !p_filter.context.pins.is_empty())
    {
        if (Orchestration* orchestration = p_filter.get_orchestration())
        {
            const Ref<OScriptVariable> variable = orchestration->get_variable(_variable_name);
            if (variable.is_valid() && variable->get_variable_type() != p_filter.target_type)
                return true;
        }
    }
    return OrchestratorGraphNodeSpawnerVariable::is_filtered(p_filter, p_spec);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OrchestratorGraphNodeSpawnerVariableSet::execute(OrchestratorGraphEdit* p_graph, const Vector2& p_position)
{
    OScriptNodeInitContext context;
    context.variable_name = _variable_name;

    p_graph->spawn_node<OScriptNodeVariableSet>(context, p_position);
}

bool OrchestratorGraphNodeSpawnerVariableSet::is_filtered(const OrchestratorGraphActionFilter& p_filter,
                                                          const OrchestratorGraphActionSpec& p_spec)
{
    if (p_filter.context_sensitive && p_filter.target_type != Variant::NIL && !p_filter.context.pins.is_empty())
    {
        const Ref<OScriptVariable> variable = p_filter.get_orchestration()->get_variable(_variable_name);
        if (variable.is_valid() && variable->get_variable_type() != p_filter.target_type)
            return true;
    }
    return OrchestratorGraphNodeSpawnerVariable::is_filtered(p_filter, p_spec);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OrchestratorGraphNodeSpawnerScriptNode::execute(OrchestratorGraphEdit* p_graph, const Vector2& p_position)
{
    OScriptNodeInitContext context;
    context.user_data = _data;

    p_graph->spawn_node(_node_name, context, p_position);
}

bool OrchestratorGraphNodeSpawnerScriptNode::is_filtered(const OrchestratorGraphActionFilter& p_filter,
                                                   const OrchestratorGraphActionSpec& p_spec)
{
    if (p_filter.flags & OrchestratorGraphActionFilter::Filter_RejectScriptNodes)
        return true;

    if (!p_spec.graph_compatible)
        return true;

    // If the target type is set and there is a pin, try to contextualize the drag node types
    // by comparing the node's input/output pins with the specified type.
    if (p_filter.context_sensitive && p_filter.target_type != Variant::NIL && !p_filter.context.pins.is_empty())
    {
        const OrchestratorGraphNodePin* pin = p_filter.context.pins[0];
        if (pin)
        {
            const EPinDirection direction = pin->is_input() ? PD_Output : PD_Input;

            bool filtered = true;
            for (const Ref<OScriptNodePin>& node_pin : _node->find_pins(direction))
            {
                if (node_pin->get_type() == p_filter.target_type)
                {
                    filtered = false;
                    break;
                }
            }
            if (filtered)
                return filtered;
        }
    }

    return OrchestratorGraphNodeSpawner::is_filtered(p_filter, p_spec);
}
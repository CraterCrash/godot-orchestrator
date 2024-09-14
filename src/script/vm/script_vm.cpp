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
#include "script/vm/script_vm.h"

#include "common/settings.h"
#include "orchestration/orchestration.h"
#include "script/instances/node_instance.h"
#include "script/nodes/variables/local_variable.h"
#include "script/nodes/variables/variable_get.h"
#include "script/nodes/variables/variable_set.h"
#include "script/vm/script_state.h"

#include <godot_cpp/classes/engine_debugger.hpp>

static int get_exec_pin_index_of_port(const Ref<OScriptNode>& p_node, int p_port, EPinDirection p_direction)
{
    int exec_index{ 0 };
    int port_index{ 0 };

    for (const Ref<OScriptNodePin>& pin : p_node->find_pins(p_direction))
    {
        if (port_index == p_port)
            break;

        port_index++;
        if (pin->is_execution())
            exec_index++;
    }

    return exec_index;
}

static int get_data_pin_index_of_port(const Ref<OScriptNode>& p_node, int p_port, EPinDirection p_direction)
{
    int data_index{ 0 };
    int port_index{ 0 };

    for (const Ref<OScriptNodePin>& pin : p_node->find_pins(p_direction))
    {
        if (port_index == p_port)
            break;

        port_index++;
        if (!pin->is_execution())
            data_index++;
    }

    return data_index;
}

static Ref<OScriptNodePin> get_data_pin_at_count_index(const Ref<OScriptNode>& p_node, int p_index, EPinDirection p_direction)
{
    int node_index = 0;
    for (const Ref<OScriptNodePin>& pin : p_node->find_pins(p_direction))
    {
        if (pin->is_execution())
            continue;

        if (node_index == p_index)
            return pin;

        node_index++;
    }

    return nullptr;
}

void OScriptVirtualMachine::_set_unassigned_inputs(const Ref<OScriptNode>& p_node, const OScriptNodeInstance* p_instance, Function& r_function)
{
    for (int i = 0; i < p_instance->data_input_pin_count; i++)
    {
        // If the input pin is bound with a value of -1, it is unassigned.
        if (p_instance->input_pins[i] == -1)
        {
            // Place the pin's effective default value on the default value list
            const Ref<OScriptNodePin> pin = get_data_pin_at_count_index(p_node, i, PD_Input);
            if (!pin.is_valid())
                continue;

            // Default values should be passed on the stack, assign a stack position for the value
            int stack_pos = r_function.max_stack++;
            p_instance->input_default_stack_pos[i] = stack_pos;

            // Rather than duplicate default values for each node, reuse existing ones if possible.
            int lookup_index = _default_values.find(pin->get_effective_default_value());
            if (lookup_index != -1)
            {
                // Default value already exists, assign the stack position
                p_instance->input_pins[i] = lookup_index | OScriptNodeInstance::INPUT_DEFAULT_VALUE_BIT;
            }
            else
            {
                // Default value doesn't exist, create a new entry
                p_instance->input_pins[i] = _default_values.size() | OScriptNodeInstance::INPUT_DEFAULT_VALUE_BIT;
                _default_values.push_back(pin->get_effective_default_value());
            }
        }
    }
}

void OScriptVirtualMachine::_set_unassigned_outputs(const Ref<OScriptNode>& p_node, const OScriptNodeInstance* p_instance, int p_stack_pos)
{
    for (int i = 0; i < p_instance->data_output_pin_count; i++)
    {
        // If the output pin is bound with a value of-1, it is unassigned
        if (p_instance->output_pins[i] == -1)
            p_instance->output_pins[i] = p_stack_pos;
    }
}

void OScriptVirtualMachine::_get_execution_path(Orchestration* p_orchestration, int p_node_id, RBSet<OScriptConnection>& r_connections, RBSet<int>& r_execution_path)
{
    // Capture all execution output pins
    RBSet<OScriptConnection> exec_connections;
    for (const OScriptConnection& E : p_orchestration->get_connections())
    {
        const Ref<OScriptNodePin>& pin = p_orchestration->get_node(E.from_node)->find_pin(E.from_port, PD_Output);
        if (pin.is_valid() && pin->is_execution())
            exec_connections.insert(E);
    }

    // Add starting node to the execution path
    r_execution_path.insert(p_node_id);

    // Traverse nodes to build the execution path & connections traversed
    List<int> queue;
    queue.push_back(p_node_id);
    while (!queue.is_empty())
    {
        for (const OScriptConnection& E : exec_connections)
        {
            if (queue.front()->get() == E.from_node && !r_execution_path.has(E.to_node))
            {
                queue.push_back(E.to_node);
                r_execution_path.insert(E.to_node);
            }
            if (queue.front()->get() == E.from_node && !r_connections.has(E))
                r_connections.insert(E);
        }
        queue.pop_front();
    }
}

void OScriptVirtualMachine::_get_data_connection_lookup(Orchestration* p_orchestration, HashMap<int, HashMap<int, Pair<int, int>>>& r_lookup)
{
    for (const OScriptConnection& E : p_orchestration->get_connections())
    {
        const Ref<OScriptNode> source = p_orchestration->get_node(E.from_node);
        const Ref<OScriptNode> target = p_orchestration->get_node(E.to_node);
        if (source.is_valid() && target.is_valid())
        {
            const Ref<OScriptNodePin>& pin = source->find_pin(E.from_port, PD_Output);
            if (!pin.is_valid() || pin->is_execution())
                continue;

            int source_index = get_data_pin_index_of_port(source, E.from_port, PD_Output);
            int target_index = get_data_pin_index_of_port(target, E.to_port, PD_Input);

            r_lookup[E.to_node][target_index] = Pair<int, int>(E.from_node, source_index);
        }
    }
}

bool OScriptVirtualMachine::_create_node_instance_pins(const Ref<OScriptNode>& p_node, OScriptNodeInstance* p_instance)
{
    for (const Ref<OScriptNodePin>& pin : p_node->get_all_pins())
    {
        // Currently we ignore hidden pins.
        // Ideally long-term, this logic should allow hidden pins to be used for internal purposes.
        if (pin->is_hidden())
            continue;

        switch (pin->get_direction())
        {
            case PD_Input:
            {
                p_instance->input_pin_count++;
                if (pin->is_execution())
                    p_instance->execution_input_pin_count++;
                break;
            }
            case PD_Output:
            {
                p_instance->output_pin_count++;
                if (pin->is_execution())
                    p_instance->execution_output_pin_count++;
                break;
            }
            default:
            {
                ERR_PRINT("An unexpected pin direction found: " + itos(pin->get_direction()));
                return false;
            }
        }
    }

    // Calculate the data pin counts
    p_instance->data_input_pin_count = p_instance->input_pin_count - p_instance->execution_input_pin_count;
    p_instance->data_output_pin_count = p_instance->output_pin_count - p_instance->execution_output_pin_count;

    if (p_instance->data_input_pin_count)
    {
        // Create the input array
        // Each defaults to -1, and if left as -1 triggers default value serialization
        p_instance->input_pins = memnew_arr(int, p_instance->data_input_pin_count);
        p_instance->input_default_stack_pos = memnew_arr(int, p_instance->data_input_pin_count);
        for (int i = 0; i < p_instance->data_input_pin_count; i++)
        {
            p_instance->input_pins[i] = -1;
            p_instance->input_default_stack_pos[i] = -1;
        }
    }

    if (p_instance->data_output_pin_count)
    {
        // Create the output array
        // Each defaults to -1, and if left as -1 triggers serialization to trash
        p_instance->output_pins = memnew_arr(int, p_instance->data_output_pin_count);
        for (int i = 0; i < p_instance->data_output_pin_count; i++)
            p_instance->output_pins[i] = -1;
    }

    if (p_instance->execution_output_pin_count)
    {
        // Create the output execution instance references array
        // The values are initialized as null and if it remains null, execution ends
        p_instance->execution_outputs = memnew_arr(OScriptNodeInstance*, p_instance->execution_output_pin_count);
        p_instance->execution_output_pins = memnew_arr(int, p_instance->execution_output_pin_count);
        for (int i = 0; i < p_instance->execution_output_pin_count; i++)
        {
            p_instance->execution_outputs[i] = nullptr;
            p_instance->execution_output_pins[i] = -1;
        }
    }

    return true;
}

bool OScriptVirtualMachine::_create_node_instance(Orchestration* p_orchestration, int p_node_id, Function& r_function, HashMap<String, int>& r_lv_indices)
{
    const Ref<OScriptNode>& node = p_orchestration->get_node(p_node_id);
    if (!node.is_valid())
        return false;

    OScriptNodeInstance* instance = node->instantiate();
    ERR_FAIL_COND_V_MSG(!instance, false, "Failed to create node instance for node ID " + itos(p_node_id));

    instance->_base = node.ptr();
    instance->id = p_node_id;
    instance->execution_index = r_function.node_count++;
    instance->pass_index = -1;

    if (!_create_node_instance_pins(node, instance))
        return false;

    const Ref<OScriptNodeLocalVariable> lv_node = node;
    const Ref<OScriptNodeAssignLocalVariable> assign_lv_node = node;
    if (lv_node.is_valid() || assign_lv_node.is_valid())
    {
        // For local variables and assignments to local variables, assign a pointer into the stack where
        // these nodes can share data that changes made via the assignment can then be propagated when
        // other nodes retrieve the value from the local variable. This is done by using the indices map
        // to track the stack position.
        //
        // Local variables in Orchestrator are not assigned names, instead the local variable node will
        // auto-generate a unique GUID. This GUID is used as the basis for both the assignment and the
        // retrieval process in the map table so that assignment refers to a local variable and can
        // share the same memory offset in the stack.
        const String name = assign_lv_node.is_valid() ? assign_lv_node->get_variable_guid() : lv_node->get_variable_guid();
        if (!r_lv_indices.has(name))
        {
            // First time this has been seen
            r_lv_indices[name] = r_function.max_stack;
            r_function.max_stack++;
        }
        // Assign working memory reference
        instance->working_memory_index = r_lv_indices[name];
    }
    else if (instance->get_working_memory_size())
    {
        // Node has working memory, create stack entry for this
        instance->working_memory_index = r_function.max_stack;
        r_function.max_stack += instance->get_working_memory_size();
    }
    else
    {
        // Node does not require working memory
        instance->working_memory_index = -1;
    }

    // Recalculate the max input/output pins
    _max_inputs = Math::max(_max_inputs, instance->data_input_pin_count);
    _max_outputs = Math::max(_max_outputs, instance->data_output_pin_count);

    // Register the node
    _nodes[p_node_id] = instance;

    return true;
}

bool OScriptVirtualMachine::_build_function_node_graph(const Ref<OScriptFunction>& p_function, Function& r_function, HashMap<String, int>& r_lv_indices)
{
    RBSet<int> execution_path;
    RBSet<OScriptConnection> exec_pins;
    _get_execution_path(p_function->get_orchestration(), r_function.node, exec_pins, execution_path);

    HashMap<int, HashMap<int, Pair<int, int>>> data_conn_lookup;
    _get_data_connection_lookup(p_function->get_orchestration(), data_conn_lookup);

    // Create a data pin processing queue
    List<int> data_pin_queue;
    for (const int E : execution_path)
        data_pin_queue.push_back(E);

    // Iterate the data pin processing queue and create the data pin connection list
    RBSet<OScriptConnection> data_pins;
    while (!data_pin_queue.is_empty())
    {
        const int key = data_pin_queue.front()->get();
        for (const KeyValue<int, Pair<int, int>> & E : data_conn_lookup[key])
        {
            OScriptConnection C;
            C.from_node = E.value.first;
            C.from_port = E.value.second;
            C.to_node = key;
            C.to_port = E.key;

            data_pins.insert(C);
            data_pin_queue.push_back(C.from_node);
            execution_path.insert(C.from_node);
        }
        data_pin_queue.pop_front();
    }

    // Step 1
    // Iterate the execution path and construct the node instances
    for (const int E : execution_path)
    {
        if (!_create_node_instance(p_function->get_orchestration(), E, r_function, r_lv_indices))
        {
            OScriptLanguage::get_singleton()->debug_break_parse(
                p_function->get_orchestration()->get_self()->get_path(),
                0,
                "Failed to create function node instance for node with ID " + itos(E) + " for function " + p_function->get_function_name());

            return false;
        }
    }

    // Step 2
    // Create the data connections
    for (const OScriptConnection& E : data_pins)
    {
        ERR_CONTINUE(!_nodes.has(E.from_node));
        ERR_CONTINUE(!_nodes.has(E.to_node));

        OScriptNodeInstance* source = _nodes[E.from_node];
        OScriptNodeInstance* target = _nodes[E.to_node];

        ERR_CONTINUE(E.from_port > source->output_pin_count);
        ERR_CONTINUE(E.to_port > target->input_pin_count);

        // If source output pin is -1, value can be assigned.
        // If the output pin is not -1, another node is already connected and it should be ignored.
        if (source->output_pins[E.from_port] == -1)
        {
            const int stack_pos = r_function.max_stack++;
            source->output_pins[E.from_port] = stack_pos;
        }

        // When the node has no output execution pins, the node instance step method should be called
        // If this is a dependency node, it should be added as a dependency of the target node.
        if (source->execution_output_pin_count == 0 && target->dependencies.find(source) == -1)
        {
            if (source->pass_index == -1)
            {
                source->pass_index = r_function.pass_stack_size;
                r_function.pass_stack_size++;
            }
            target->dependencies.push_back(source);
        }

        // Read from the stack position
        target->input_pins[E.to_port] = source->output_pins[E.from_port];
    }

    // Assign trash position
    r_function.trash_pos = r_function.max_stack++;

    // Step 3
    // Create the execution connections
    for (const OScriptConnection& E : exec_pins)
    {
        ERR_CONTINUE(!_nodes.has(E.from_node));
        ERR_CONTINUE(!_nodes.has(E.to_node));

        OScriptNodeInstance* source = _nodes[E.from_node];
        OScriptNodeInstance* target = _nodes[E.to_node];

        ERR_CONTINUE(E.from_port >= source->output_pin_count);

        const int source_pin_index = get_exec_pin_index_of_port(source->get_base_node(), E.from_port, PD_Output);
        const int target_pin_index = get_exec_pin_index_of_port(target->get_base_node(), E.to_port, PD_Input);

        source->execution_outputs[source_pin_index] = target;
        source->execution_output_pins[source_pin_index] = target_pin_index;
    }

    // Step 4
    // Handle unassigned input pins by assigning default values, if applicable.
    // Handle unassigned output pins, connecting to trash position on stack.
    for (const int E : execution_path)
    {
        ERR_CONTINUE(!_nodes.has(E));

        const Ref<OScriptNode>& node = p_function->get_orchestration()->get_node(E);
        const OScriptNodeInstance* instance = _nodes[E];

        _set_unassigned_inputs(node, instance, r_function);
        _set_unassigned_outputs(node, instance, r_function.trash_pos);
    }

    return true;
}

void OScriptVirtualMachine::_resolve_inputs(OScriptExecutionContext& p_context, OScriptNodeInstance* p_instance, Function* p_function)
{
    // For the initial node of the function, copy the stack to the inputs
    if (p_context._current_node_id == p_context._initial_node_id)
    {
        p_context._copy_stack_to_inputs(p_function->argument_count);
        return;
    }

    // If node has dependencies, resolve those
    if (!p_instance->dependencies.is_empty())
    {
        OScriptNodeInstance** deps = p_instance->dependencies.ptrw();

        const int dep_count = p_instance->dependencies.size();
        for (int i = 0; i < dep_count; i++)
        {
            _dependency_step(p_context, deps[i], &p_instance);
            if (p_context.has_error())
            {
                p_context._current_node_id = p_instance->id;
                break;
            }
        }

        if (p_context.has_error())
            return;
    }

    // Prepare inputs
    for (int i = 0; i < p_instance->data_input_pin_count; i++)
    {
        const int index = p_instance->input_pins[i] & OScriptNodeInstance::INPUT_MASK;
        if (p_instance->input_pins[i] & OScriptNodeInstance::INPUT_DEFAULT_VALUE_BIT)
            p_context._set_input_from_default_value(i, p_instance->input_default_stack_pos[i], _default_values[index]);
        else
            p_context._copy_stack_to_input(index, i);
    }
}

void OScriptVirtualMachine::_copy_stack_to_node_outputs(OScriptExecutionContext& p_context, OScriptNodeInstance* p_instance)
{
    for (int i = 0; i < p_instance->data_output_pin_count; i++)
        p_context._copy_stack_to_output(p_instance->output_pins[i], i);
}

void OScriptVirtualMachine::_resolve_step_mode(OScriptExecutionContext& p_context, bool& r_resume)
{
    if (r_resume)
    {
        p_context.set_step_mode(OScriptNodeInstance::STEP_MODE_RESUME);
        r_resume = false;
    }
    else if (p_context._has_flow_stack_bit(OScriptNodeInstance::FLOW_STACK_PUSHED_BIT))
    {
        // Node had a flow stack bit pushed, so re-execute the node a subsequent time
        p_context.set_step_mode(OScriptNodeInstance::STEP_MODE_CONTINUE);
    }
    else
    {
        // Start from the beginning
        p_context.set_step_mode(OScriptNodeInstance::STEP_MODE_BEGIN);
    }
}

void OScriptVirtualMachine::_dependency_step(OScriptExecutionContext& p_context, OScriptNodeInstance* p_instance, OScriptNodeInstance** r_error_node)
{
    ERR_FAIL_COND(p_instance->pass_index == -1);

    if (p_context._get_pass_at(p_instance->pass_index) == p_context.get_passes())
        return;

    p_context._add_current_pass(p_instance->pass_index);

    if (!p_instance->dependencies.is_empty())
    {
        OScriptNodeInstance** deps = p_instance->dependencies.ptrw();

        const int deps_count = p_instance->dependencies.size();
        for (int i = 0; i < deps_count; i++)
        {
            _dependency_step(p_context, deps[i], r_error_node);
            if (p_context.has_error())
                return;
        }
    }

    // Set step details for the dependency node
    p_context._set_current_node_working_memory(p_instance->get_working_memory_size());

    // Set the inputs for the dependency node
    for (int i = 0; i < p_instance->data_input_pin_count; i++)
    {
        const int index = p_instance->input_pins[i] & OScriptNodeInstance::INPUT_MASK;
        if (p_instance->input_pins[i] & OScriptNodeInstance::INPUT_DEFAULT_VALUE_BIT)
            p_context._set_input_from_default_value(i, p_instance->input_default_stack_pos[i], _default_values[index]);
        else
            p_context._copy_stack_to_input(index, i);
    }

    _copy_stack_to_node_outputs(p_context, p_instance);

    p_context.set_working_memory(p_instance->working_memory_index);

    // Execute the dependency node's step
    _execute_step(p_context, p_instance);

    if (p_context.has_error())
        *r_error_node = p_instance;
}

int OScriptVirtualMachine::_execute_step(OScriptExecutionContext& p_context, OScriptNodeInstance* p_instance)
{
    // In the case of dependency steps, adjust current node id
    int current_node_id = p_instance->get_id();
    if (p_context.get_current_node() != current_node_id)
        p_context._current_node_id = p_instance->get_id();

    // Setup step details
    p_context._set_current_node_working_memory(p_instance->get_working_memory_size());

    #if GODOT_VERSION >= 0x040300
    EngineDebugger* debugger = EngineDebugger::get_singleton();
    if (debugger && debugger->is_active())
    {
        Orchestration* orchestration = p_instance->get_base_node()->get_orchestration();

        bool do_break = false;
        const int node_id = p_instance->get_base_node()->get_id();

        if (debugger->get_lines_left() > 0)
        {
            if (debugger->get_depth() <= 0)
                debugger->set_lines_left(debugger->get_lines_left() - 1);
            if (debugger->get_lines_left() <= 0)
                do_break = true;
        }

        if (!do_break && debugger->is_breakpoint(node_id, orchestration->get_self()->get_path()))
            do_break = true;

        if (do_break && !debugger->is_skipping_breakpoints())
            OScriptLanguage::get_singleton()->debug_break("Breakpoint: Before Node " + itos(node_id) + " executes.", true);

        debugger->line_poll();
    }
    #endif

    // Reset node
    p_context._current_node_id = current_node_id;

    // Execute
    return p_instance->step(p_context);
}

OScriptNodeInstance* OScriptVirtualMachine::_resolve_next_node(OScriptExecutionContext& p_context, OScriptNodeInstance* p_instance, int p_result, int p_next_node_id)
{
    if ((p_result == p_next_node_id || p_result & OScriptNodeInstance::STEP_FLAG_PUSH_STACK_BIT) && p_instance->execution_output_pin_count)
    {
        if (p_next_node_id >= 0 && p_next_node_id < p_instance->execution_output_pin_count)
            return p_instance->execution_outputs[p_next_node_id];

        // No exit bit was set and node has an execution output
        p_context.set_error(
            vformat("Node %s: %d returned an invalid execution pin output %d",
                p_instance->get_base_node()->get_class(),
                p_instance->get_id(),
                p_next_node_id));
    }

    return nullptr;
}

int OScriptVirtualMachine::_resolve_next_node_port(OScriptNodeInstance* p_instance, int p_next_node_id, OScriptNodeInstance* p_next)
{
    if (p_instance && p_next)
    {
        if (p_next_node_id >= 0 && p_next_node_id < p_instance->execution_output_pin_count)
            return p_instance->execution_output_pins[p_next_node_id];
    }
    return -1;
}

void OScriptVirtualMachine::_set_node_flow_execution_state(OScriptExecutionContext& p_context, OScriptNodeInstance* p_instance, int p_result)
{
    if (p_result & OScriptNodeInstance::STEP_FLAG_PUSH_STACK_BIT)
    {
        p_context._set_flow_stack_bit(OScriptNodeInstance::FLOW_STACK_PUSHED_BIT);
        p_context._set_node_execution_state(p_instance->execution_index, true);
    }
    else
        p_context._set_node_execution_state(p_instance->execution_index, false);
}

void OScriptVirtualMachine::_report_error(OScriptExecutionContext& p_context, OScriptNodeInstance* p_instance, const StringName& p_method)
{
    const String err_file = _script->get_path();
    const String err_func = p_method;
    const int err_line = p_context.get_current_node();

    String error_str = p_context.get_error_reason();
    if (error_str.is_empty())
    {
        switch (p_context.get_error().error)
        {
            case GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT:
            {
                error_str = "Invalid argument detected.";
                break;
            }
            case GDEXTENSION_CALL_ERROR_TOO_MANY_ARGUMENTS:
            {
                error_str = "Too many arguments detected.";
                break;
            }
            case GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS:
            {
                error_str = "Too few arguments detected.";
                break;
            }
            case GDEXTENSION_CALL_ERROR_INVALID_METHOD:
            {
                error_str = vformat("An unexpected error happened inside the '%s' method.", p_method);
                break;
            }
            case GDEXTENSION_CALL_ERROR_METHOD_NOT_CONST:
            {
                error_str = vformat("The method '%s' is not const, but called in a const instance.", p_method);
                break;
            }
            case GDEXTENSION_CALL_ERROR_INSTANCE_IS_NULL:
            {
                error_str = vformat("Method '%s' exception detected for a null instance", p_method);
                break;
            }
            default:
            {
                error_str = vformat("An unexpected error inside method '%s'.", p_method);
                break;
            }
        }
    }

    if (!OScriptLanguage::get_singleton()->debug_break(error_str, false))
        _err_print_error(err_func.utf8().get_data(), err_file.utf8().get_data(), err_line, error_str.utf8().get_data());
}

void OScriptVirtualMachine::_call_method_internal(const StringName& p_method, OScriptExecutionContext* p_context, bool p_resume, OScriptNodeInstance* p_instance, Function* p_function, Variant& r_return, GDExtensionCallError& r_err)
{
    OScriptExecutionContext& context = *p_context;

    // Initialize the context
    context._instance = this;
    context._initial_node_id = p_function->node;
    context._step_mode = OScriptNodeInstance::StepMode::STEP_MODE_BEGIN;
    context._error = &r_err;
    context._current_node_id = p_function->node;
    context._function = p_function;

    OScriptNodeInstance* node = p_instance;
    int node_port = 0; // always assumes 0 for now

    #if GODOT_VERSION >= 0x040300
    if (EngineDebugger::get_singleton()->is_active())
        OScriptLanguage::get_singleton()->function_entry(&p_method, p_context);
    #endif

    while (node)
    {
        // Track current node details
        context._current_node_id = node->get_id();
        context._current_node_port = node_port;

        // Keep track of the number of iterations in the flow
        context._passes++;

        // Resolve inputs
        _resolve_inputs(context, node, p_function);
        if (context.has_error())
        {
            ERR_PRINT(vformat("Script call error #%d with Node %d: %s",
                context.get_error().error,
                context.get_current_node(),
                context.get_error_reason()));

            r_return.clear();
            break;
        }

        // Initialize working memory
        // This must be set after input resolution as dependency chains will adjust this
        context.set_working_memory(node->working_memory_index);

        // Setup outputs
        _copy_stack_to_node_outputs(context, node);

        // Evaluate the step mode based on the resume state
        _resolve_step_mode(context, p_resume);

        // Clear errors, if any exist
        context.clear_error();

        // Execute the step
        // If the step failed with an error, we should break the loop immediately.
        const int result = _execute_step(context, node);
        if (context.has_error())
            break;

        if (result & OScriptNodeInstance::STEP_FLAG_YIELD)
        {
            // The node requested a yield without working memory.
            // This is invalid and we should immediately terminate the function call in this use case.
            if (node->get_working_memory_size() == 0)
            {
                context.set_error("Execution yielded without any working memory");
                break;
            }

            // Attempt to create the yield state
            // If the state failed to be allocated, terminate the function call in this use case.
            Ref<OScriptState> state = context.get_working_memory();
            if (!state.is_valid())
            {
                context.set_error("Execution yield failed to create memory state");
                break;
            }

            const int stack_size = context.get_metadata().get_stack_size();

            state->_instance_id = _owner->get_instance_id();
            state->_script_id = _script->get_instance_id();
            state->_instance = this;
            state->_script_instance = p_context->_script_instance;
            state->_function = p_method;
            state->_working_memory_index = node->working_memory_index;
            state->_variant_stack_size = p_function->max_stack;
            state->_node = node;
            state->_func_ptr = p_function;
            state->_flow_stack_pos = context._get_flow_stack_position();
            state->_pass = context.get_passes();
            state->_stack_info = context.get_metadata();
            state->_stack.resize(stack_size);
            memcpy(state->_stack.ptrw(), context._get_stack(), stack_size);

            context.clear_error();
            r_return = state;

            #if GODOT_VERSION >= 0x040300
            if (EngineDebugger::get_singleton()->is_active())
                OScriptLanguage::get_singleton()->function_exit(&p_method, p_context);
            #endif

            return;
        }

        // Check whether the function exited or ended
        if (result & OScriptNodeInstance::STEP_FLAG_END)
        {
            if (node->get_working_memory_size() > 0)
                r_return = context.get_working_memory();
            else
                context.set_error("Return value should be assigned to node's working memory");
            break;
        }

        #if GODOT_VERSION >= 0x040300
        if (EngineDebugger::get_singleton()->is_active())
        {
            bool do_break = false;

            const int node_id = node->get_base_node()->get_id();
            const String path = node->get_base_node()->get_orchestration()->get_self()->get_path();
            if (EngineDebugger::get_singleton()->is_breakpoint(node_id, path))
                do_break = true;

            if (do_break && !EngineDebugger::get_singleton()->is_skipping_breakpoints())
                OScriptLanguage::get_singleton()->debug_break("Breakpoint: After Node " + itos(node_id) + " has executed.", true);

            EngineDebugger::get_singleton()->line_poll();
        }
        #endif

        // Calculate the output node
        const int next_node_id = result & OScriptNodeInstance::STEP_MASK;

        // Resolve the next node instance
        OScriptNodeInstance* next = _resolve_next_node(context, node, result, next_node_id);
        if (context.has_error())
            break;

        // Resolve the next node's port index
        const int next_port = _resolve_next_node_port(node, next_node_id, next);

        if (context._has_flow_stack())
        {
            // Update flow stack
            context._set_flow_stack(context.get_current_node());
            _set_node_flow_execution_state(context, node, result);

            if (result & OScriptNodeInstance::STEP_FLAG_GO_BACK_BIT)
            {
                // When flow position is less-than or equal to 0, cannot go back, so exit
                if (context._get_flow_stack_position() <= 0)
                    break;

                context._decrement_flow_stack_position();
                node = _nodes[context._get_flow_stack_value() & OScriptNodeInstance::FLOW_STACK_MASK];
                node_port = 0; // ?
            }
            else if (next)
            {
                // There is a next node, check if its executed previously
                if (context.has_node_executed(next->execution_index))
                {
                    // Enter a node that is in the middle of doing its sequence (pushed stack)
                    // operation from the front because each node has working memory. This can
                    // not really do a sub-sequence as a result, the sequence will be restarted
                    // and the stack will roll back to find where this node started.
                    bool found = false;
                    for (int i = context._get_flow_stack_position(); i >= 0; i--)
                    {
                        if ((context._get_flow_stack_value(i) & OScriptNodeInstance::FLOW_STACK_MASK) == next->get_id())
                        {
                            // Roll back and remove
                            context._set_flow_stack_position(i);
                            context._set_flow_stack(next->get_id());
                            context._set_node_execution_state(next->execution_index, false);
                            found = true;
                        }
                    }
                    if (!found)
                    {
                        context.set_error("Found execution bit but not the node in the stack.");
                        break;
                    }

                    // Set the current node as the next
                    node = next;
                    node_port = next_port;
                }
                else
                {
                    // Check for overflow
                    if (context._get_flow_stack_position() + 1 >= context._get_flow_stack_size())
                    {
                        context.set_error("Stack overflow");
                        break;
                    }

                    // Set the current node as the next
                    node = next;
                    node_port = next_port;

                    context._increment_flow_stack_position();
                    context._set_flow_stack(node->get_id());
                }
            }
            else
            {
                // No next node, try and go back in stack and push bit.
                bool found = false;
                for (int i = context._get_flow_stack_position(); i >= 0; i--)
                {
                    int flow_stack_value = context._get_flow_stack_value(i);
                    if (flow_stack_value & OScriptNodeInstance::FLOW_STACK_PUSHED_BIT)
                    {
                        node = _nodes[flow_stack_value & OScriptNodeInstance::FLOW_STACK_MASK];
                        context._set_flow_stack_position(i);
                        found = true;
                        break;
                    }
                }

                // Could not find pushed stack bit, exit
                if (!found)
                    break;
            }
        }
        else
        {
            // Stackless, advance to next node
            node = next;
            node_port = next_port;
        }
    }

    // If there are errors, report the error
    if (context.has_error())
        _report_error(context, node, p_method);

    #if GODOT_VERSION >= 0x040300
    if (EngineDebugger::get_singleton()->is_active())
        OScriptLanguage::get_singleton()->function_exit(&p_method, p_context);
    #endif

    // Cleanup
    context._cleanup();
}

bool OScriptVirtualMachine::register_variable(const Ref<OScriptVariable>& p_variable)
{
    ERR_FAIL_COND_V_MSG(!p_variable.is_valid(), false, "Cannot register a variable that is invalid");
    ERR_FAIL_COND_V_MSG(_variables.has(p_variable->get_variable_name()), false, "A variable is defined with the name: " + p_variable->get_variable_name());

    Variable variable;
    variable.exported = p_variable->is_exported();
    variable.value= p_variable->get_default_value();
    variable.type = p_variable->get_variable_type();

    _variables[p_variable->get_variable_name()] = variable;

    return true;
}

bool OScriptVirtualMachine::has_variable(const StringName& p_name) const
{
    return _variables.has(p_name);
}

OScriptVirtualMachine::Variable* OScriptVirtualMachine::get_variable(const StringName& p_name) const
{
    const HashMap<StringName, Variable>::ConstIterator E = _variables.find(p_name);
    return E ? const_cast<Variable*>(&E->value) : nullptr;
}

bool OScriptVirtualMachine::get_variable(const StringName& p_name, Variant& r_value) const
{
    if (!_variables.has(p_name))
        return false;

    const Variable& variable = _variables[p_name];
    r_value = variable.value;

    return true;
}

bool OScriptVirtualMachine::set_variable(const StringName& p_name, const Variant& p_value)
{
    if (!_variables.has(p_name))
        return false;

    Variable& variable = _variables[p_name];
    variable.value = p_value;

    return true;
}

bool OScriptVirtualMachine::has_signal(const StringName& p_name) const
{
    return _script->has_script_signal(p_name);
}

Variant OScriptVirtualMachine::get_signal(const StringName& p_name)
{
    ERR_FAIL_COND_V_MSG(!_script->has_script_signal(p_name), Variant(), "No signal with name '" + p_name + "' found.");
    return Signal(get_owner(), p_name);
}

bool OScriptVirtualMachine::register_function(const Ref<OScriptFunction>& p_function)
{
    ERR_FAIL_COND_V_MSG(!p_function.is_valid(), false, "Cannot register function that is invalid.");

    Function function;
    function.node = p_function->get_owning_node_id();
    function.argument_count = p_function->get_argument_count();
    function.max_stack = function.argument_count;
    function.flow_stack_size = 256;

    if (function.node < 0)
    {
        OScriptLanguage::get_singleton()->debug_break_parse(
            p_function->get_orchestration()->get_self()->get_path(),
            0,
            "No start node was defined for function " + p_function->get_function_name());
        return false;
    }

    Ref<OScriptNode> node = p_function->get_owning_node();
    if (!node.is_valid())
    {
        OScriptLanguage::get_singleton()->debug_break_parse(
            p_function->get_orchestration()->get_self()->get_path(),
            0,
            "Unable to locate function start node in graph with ID: " + itos(function.node));
        return false;
    }

    // Calculate the maximum number of input arguments based on the function definition.
    _max_inputs = Math::max(_max_inputs, function.argument_count);

    // Populate the function's local variables
    for (const Ref<OScriptLocalVariable>& local : p_function->get_local_variables())
        function._variables[local->get_variable_name()] = local->get_default_value();

    // Initialize the function's node graph
    HashMap<String, int> local_variable_indices;
    _build_function_node_graph(p_function, function, local_variable_indices);

    // Register function
    _functions[p_function->get_function_name()] = function;
    return true;
}

void OScriptVirtualMachine::call_method(OScriptInstance* p_instance, const StringName& p_method, const Variant* const* p_args, GDExtensionInt p_arg_count, Variant* r_return, GDExtensionCallError* r_err)
{
    ERR_FAIL_COND_MSG(!r_err, "No error code argument provided.");

    r_err->error = GDEXTENSION_CALL_OK;

    // Check whether the method is defined as part of the Orchestration.
    // This means that there will be a function defined in the function map.
    const HashMap<StringName, Function>::Iterator E = _functions.find(p_method);
    if (!E)
    {
        // Method was not found, return invalid method
        r_err->error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;
        *r_return = Variant();
        return;
    }

    // Check whether the function has a node instance associated with it
    // These are assigned lazily.
    Function* F = &E->value;
    if (!F->instance)
    {
        // Lookup the node that offers the function.
        HashMap<int, OScriptNodeInstance*>::Iterator N = _nodes.find(F->node);
        if (!N)
        {
            // No node found
            r_err->error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;
            ERR_FAIL_MSG("Unable to locate node for method '" + p_method + "' with node id " + itos(F->node));
        }

        // Lazily assign the instance reference
        F->instance = N->value;
    }

    if (F->max_stack > _max_call_stack)
    {
        ERR_FAIL_MSG("Unable to call function, call stack exceeds " + itos(_max_call_stack));
    }

    // Setup the execution stack
    OScriptExecutionStackInfo si;
    si.max_stack_size = F->max_stack;
    si.node_count = F->node_count;
    si.max_inputs = _max_inputs;
    si.max_outputs = _max_outputs;
    si.flow_size = F->flow_stack_size;
    si.pass_size = F->pass_stack_size;

    const int stack_size = si.get_stack_size();
    void* stack = alloca(stack_size);
    memset(stack, 0, stack_size);

    OScriptExecutionContext context(si, stack, 0, 0);
    context._initialize_variant_stack();
    context._push_node_onto_flow_stack(F->node);
    context._push_arguments(p_args, static_cast<int>(p_arg_count));
    context._script_instance = p_instance;

    // Dispatch to the internal handler
    _call_method_internal(p_method, &context, false, F->instance, F, *r_return, *r_err);
}

OScriptVirtualMachine::OScriptVirtualMachine()
{
    _max_call_stack = OrchestratorSettings::get_singleton()->get_setting("settings/runtime/max_call_stack");
}

OScriptVirtualMachine::~OScriptVirtualMachine()
{
    for (const KeyValue<int, OScriptNodeInstance*>& E : _nodes)
        memdelete(E.value);

    _nodes.clear();
}

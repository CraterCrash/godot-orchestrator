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
#include "execution_context.h"

#include "script/instances/node_instance.h"
#include "script/node.h"

// todo:    p_node_id and p_flow_stack_size are driven by OScriptInstance::Function
//          can we refactor and only pass the function reference by chance?

OScriptNodeExecutionContext::OScriptNodeExecutionContext(const Ref<OScriptExecutionStack>& p_stack, int p_node_id, int p_passes,
                                                         int p_flow_stack_position, GDExtensionCallError* p_error)
    : _execution_stack(p_stack)
    , _initial_node_id(p_node_id)
    , _current_node_id(p_node_id)
    , _passes(p_passes)
    , _step_mode(OScriptNodeInstance::StepMode::STEP_MODE_BEGIN)
    , _error(p_error)
    , _flow_stack_position(p_flow_stack_position)
{
}

void OScriptNodeExecutionContext::set_error(GDExtensionCallErrorType p_type, const String& p_reason)
{
    _error->error = p_type;
    _error_reason = p_reason;
}

void OScriptNodeExecutionContext::set_invalid_argument(OScriptNodeInstance* p_instance, int p_argument_index, Variant::Type p_type, Variant::Type p_expected_type)
{
    _error->error = GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT;
    _error->argument = p_argument_index;
    _error->expected = p_expected_type;
    _error_reason = p_instance->get_base_node()->get_class() + ":" + itos(p_instance->get_id()) + " type " + Variant::get_type_name(p_type);
}

void OScriptNodeExecutionContext::clear_error()
{
    if (_error->error != GDEXTENSION_CALL_OK)
    {
        _error->error = GDEXTENSION_CALL_OK;
        _error_reason = "";
    }
}

Variant OScriptNodeExecutionContext::get_working_memory(int p_index)
{
#if _DEBUG
    ERR_FAIL_COND_V_MSG(p_index >= _current_node_working_memory, _empty,
                        "Working memory index " + itos(p_index) + " is out of bounds for node #" + itos(_current_node_id));
#endif
    static Variant empty = Variant();
    return has_working_memory() ? _working_memory[p_index] : empty;
}

void OScriptNodeExecutionContext::set_working_memory(int p_index)
{
    if (p_index >= 0)
    {
        Variant* variant_stack = _execution_stack->_variant_stack;
        _working_memory = &(variant_stack[p_index]);
        return;
    }
    _working_memory = nullptr;
}

void OScriptNodeExecutionContext::set_working_memory(int p_index, const Variant& p_value)
{
#if _DEBUG
    ERR_FAIL_COND_MSG(p_index >= _current_node_working_memory,
                      "Working memory index " + itos(p_index) + " is out of bounds for node #" + itos(_current_node_id));
#endif
    _working_memory[p_index] = p_value;
}

void OScriptNodeExecutionContext::cleanup()
{
    _execution_stack->cleanup_variant_stack();
}

Variant& OScriptNodeExecutionContext::get_input(int p_index)
{
#ifdef _DEBUG
    ERR_FAIL_COND_V_MSG(p_index >= _current_node_inputs, _empty,
                      "Input index " + itos(p_index) + " out of bounds processing node #" + itos(_current_node_id));
#endif
    return *_execution_stack->_inputs[p_index];
}

const Variant** OScriptNodeExecutionContext::get_input_ptr()
{
    return const_cast<const Variant**>(_execution_stack->_inputs);
}

void OScriptNodeExecutionContext::set_input(int p_index, const Variant* p_value)
{
#ifdef _DEBUG
    ERR_FAIL_COND_MSG(p_index >= _current_node_inputs,
                      "Input index " + itos(p_index) + " out of bounds processing node #" + itos(_current_node_id));
#endif
    _execution_stack->_inputs[p_index] = const_cast<Variant*>(p_value);
}

Variant& OScriptNodeExecutionContext::get_output(int p_index)
{
#ifdef _DEBUG
    ERR_FAIL_COND_V_MSG(p_index >= _current_node_outputs, _empty,
                      "Output index " + itos(p_index) + " out of bounds processing node #" + itos(_current_node_id));
#endif
    return *_execution_stack->_outputs[p_index];
}

bool OScriptNodeExecutionContext::set_output(int p_index, const Variant& p_value)
{
#ifdef _DEBUG
    ERR_FAIL_COND_V_MSG(p_index >= _current_node_outputs, false,
                        "Output index " + itos(p_index) + " out of bounds processing node #" + itos(_current_node_id));
#endif
    *_execution_stack->_outputs[p_index] = p_value;
    return true;
}

bool OScriptNodeExecutionContext::set_output(int p_index, Variant* p_value)
{
#ifdef _DEBUG
    ERR_FAIL_COND_V_MSG(p_index >= _current_node_outputs, false,
                        "Output index " + itos(p_index) + " out of bounds processing node #" + itos(_current_node_id));
#endif
    *_execution_stack->_outputs[p_index] = *p_value;
    return true;
}

void OScriptNodeExecutionContext::copy_inputs_to_outputs(int p_elements)
{
    for (int i = 0; i < p_elements; i++)
        _execution_stack->_outputs[i] = _execution_stack->_inputs[i];
}

void OScriptNodeExecutionContext::copy_input_to_output(size_t p_input_index, size_t p_output_index)
{
    *_execution_stack->_outputs[p_output_index] = *_execution_stack->_inputs[p_input_index];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OScriptExecutionContext::OScriptExecutionContext(const Ref<OScriptExecutionStack>& p_stack, int p_node_id, int p_passes,
                                                 int p_flow_stack_position, GDExtensionCallError* p_err)
    : OScriptNodeExecutionContext(p_stack, p_node_id, p_passes, p_flow_stack_position, p_err)
{
}
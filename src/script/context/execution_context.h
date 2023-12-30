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
#ifndef ORCHESTRATOR_EXECUTION_CONTEXT_H
#define ORCHESTRATOR_EXECUTION_CONTEXT_H

#include "execution_stack.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/templates/vector.hpp>

using namespace godot;

/// Forward declaration
class OScriptInstance;
class OScriptNodeInstance;

/// Represents the node execution context, an object that is passed between an OrchestratorScript's nodes
/// that contains information about the current execution of the script, such as inputs, outputs,
/// stack values and other important bits.
class OScriptNodeExecutionContext
{
protected:
    Ref<OScriptExecutionStack> _execution_stack;  //! The underlying execution stack
    int _initial_node_id{ -1 };                   //! The initial executing function node unique id
    int _current_node_id{ -1 };                   //! The current executing node's unique id
    int _current_node_port{ -1 };                 //! The current executing node's port that triggered the step
    int _passes{ 0 };                             //! The number of passes
    int _step_mode{ 0 };                          //! OScriptNodeInstance::StepMode
    GDExtensionCallError* _error;                 //! Reference to the call error object
    String _error_reason;                         //! The error reason
    int _flow_stack_position{ 0 };                //! THe flow stack position
    Variant* _working_memory{ nullptr };          //! The node's working memory buffer
    Variant _empty;                               //! Empty response
    int _current_node_working_memory{ 0 };        //! Current execution node's max working memory count
    int _current_node_inputs{ 0 };                //! Current execution node's max input count
    int _current_node_outputs{ 0 };               //! Current execution node's max output count

public:
    OScriptNodeExecutionContext(const Ref<OScriptExecutionStack>& p_stack, int p_node_id, int p_passes,
                                int p_flow_stack_position, GDExtensionCallError* p_err);

    /// Get the current node port
    /// @return the current node port that control flow entered through.
    int get_current_node_port() const { return _current_node_port; }

    /// Get the current step mode
    /// @return the current step mode
    int get_step_mode() const { return _step_mode; }

    /// Set the current step mode
    /// @param p_step_mode the new step mode to be set
    void set_step_mode(int p_step_mode) { _step_mode = p_step_mode; }

    /// Check whether the context has reported any error.
    /// @return true if an error has been reported, false otherwise
    _FORCE_INLINE_ bool has_error() const { return _error->error != GDEXTENSION_CALL_OK; }

    /// Get the extension call error code
    /// @return a reference to the GDExtensionCallError object
    GDExtensionCallError& get_error() { return *_error; }

    /// Get the error reason message
    /// @return the reason message for the error
    String get_error_reason() const { return _error_reason; }

    /// Set an execution error
    /// @param p_type the error type to be reported
    /// @param p_reason an error reason to be reported
    void set_error(GDExtensionCallErrorType p_type, const String& p_reason = String());

    void set_invalid_argument(OScriptNodeInstance* p_instance, int p_argument_index, Variant::Type p_type,
                              Variant::Type p_expected_type);

    /// Helper method to clear any error condition in the context
    void clear_error();

    _FORCE_INLINE_ bool has_working_memory() const { return _working_memory != nullptr; }
    Variant get_working_memory(int p_index = 0);
    void set_working_memory(int p_index);
    void set_working_memory(int p_index, const Variant& p_value);

    /// Cleanup the specified number of objects on the variant stack
    void cleanup();

    /// Get the input at the specified index from the execution stack
    /// @param p_index the stack index where to read the input value from
    /// @return reference to the input value on the stack
    Variant& get_input(int p_index);

    /// Get a pointer to the start of the input argument stack.
    /// @return a pointer
    const Variant** get_input_ptr();

    /// @brief Set the input value at a given index
    /// @param p_index the input stack index
    /// @param p_value the value to set
    void set_input(int p_index, const Variant* p_value);

    /// @brief Get the output value at a given index
    /// @param p_index the output stack index
    /// @return the value
    Variant& get_output(int p_index);

    /// Set the output value at a given index
    /// @param p_index the output stack index
    /// @param p_value the value to set
    /// @param true if the output was set, false otherwise
    bool set_output(int p_index, const Variant& p_value);

    /// Set the output value at a given index
    /// @param p_index the output stack index
    /// @param p_value the value to set
    /// @return true if the output was set, false otherwise
    bool set_output(int p_index, Variant* p_value);

    /// Copies the specified number of elements from the input to output stack
    /// @param p_elements number of elements to copy
    void copy_inputs_to_outputs(int p_elements);

    /// Copies a specific input at the given index to the output index
    /// @param p_input_index the input stack index to copy a value from
    /// @param p_output_index the output stack index to copy a value to
    void copy_input_to_output(size_t p_input_index, size_t p_output_index);
};

/// Represents the top-layer or script-level execution context
class OScriptExecutionContext : public OScriptNodeExecutionContext
{
public:
    OScriptExecutionContext(const Ref<OScriptExecutionStack>& p_stack, int p_node_id, int p_passes, int p_flow_stack_position,
                            GDExtensionCallError* p_err);

    /// Get the current pass count
    /// @return the number of passes performed
    int get_passes() const { return _passes; }

    /// Increments the number of execution passes performed
    void increment_passes() { _passes++; }

    /// Checks whether the current node is the initial function node
    /// @return true if we are executing the first initial node, false otherwise
    bool is_initial_node() const { return _current_node_id == _initial_node_id; }

    /// Get the current executing node unique id
    /// @return the current executing node's unique id
    int get_current_node() const { return _current_node_id; }

    /// Set the current executing node unique id
    /// @param p_node_id the node's unique id
    void set_current_node(int p_node_id) { _current_node_id = p_node_id; }

    void set_current_node_port(int p_node_port) { _current_node_port = p_node_port; }

    void set_current_node_working_memory(int p_working_memory) { _current_node_working_memory = p_working_memory; }
    void set_current_node_inputs(int p_inputs) { _current_node_inputs = p_inputs; }
    void set_current_node_outputs(int p_outputs) { _current_node_outputs = p_outputs; }

    /// Check whether a specific node has executed
    /// @param p_index the node's execution index into the execution stack
    /// @return true if the node has executed, false otherwise
    _FORCE_INLINE_ bool has_node_executed(int p_index) const { return _execution_stack->_execution_bits[p_index]; }

    /// Set whether a node has executed
    /// @param p_index the node's execution index in the execution stack
    /// @param p_state true if it has executed, false if it has not
    _FORCE_INLINE_ void set_node_execution_state(int p_index, bool p_state)
    {
        _execution_stack->_execution_bits[p_index] = p_state;
    }

    // Flow Stack API
    _FORCE_INLINE_ bool has_flow_stack() const { return _execution_stack->_flow != nullptr; }
    _FORCE_INLINE_ int get_flow_stack_size() const { return _execution_stack->_info.flow_size; }
    _FORCE_INLINE_ int get_flow_stack_value(int p_index) const { return _execution_stack->_flow[p_index]; }
    _FORCE_INLINE_ int get_flow_stack_position() const { return _flow_stack_position; }
    _FORCE_INLINE_ void increment_flow_stack_position() { _flow_stack_position++; }
    _FORCE_INLINE_ void decrement_flow_stack_position() { _flow_stack_position--; }
    _FORCE_INLINE_ void set_flow_stack_position(int p_index) { _flow_stack_position = p_index; }

    // Flow Stack API - operates on the current position
    _FORCE_INLINE_ bool has_flow_stack_bit(int p_bit)
    {
        return has_flow_stack() && (_execution_stack->_flow[_flow_stack_position] & p_bit);
    }
    _FORCE_INLINE_ void set_flow_stack_bit(int p_bit) { _execution_stack->_flow[_flow_stack_position] |= p_bit; }
    _FORCE_INLINE_ void set_flow_stack(int p_node_id) { _execution_stack->_flow[_flow_stack_position] = p_node_id; }
    _FORCE_INLINE_ int get_flow_stack_value() const { return get_flow_stack_value(_flow_stack_position); }

    // Used by the _dependency_step method
    _FORCE_INLINE_ int get_pass_at(int p_index) { return _execution_stack->_pass[p_index]; }
    _FORCE_INLINE_ void add_current_pass(int p_index) { _execution_stack->_pass[p_index] = _passes; }

    _FORCE_INLINE_ Variant* get_variant_stack() { return _execution_stack->_variant_stack; }

    /// Copies the specified number of elements from the top of the stack as inputs
    /// @param elements number of elements to copy
    _FORCE_INLINE_ void copy_stack_to_inputs(int p_elements)
    {
        for (int i = 0; i < p_elements; i++)
            _execution_stack->_inputs[i] = &_execution_stack->_variant_stack[i];
    }

    _FORCE_INLINE_ void set_input_from_default_value(int p_index, const Variant& p_default_value)
    {
        _execution_stack->_inputs[p_index] = const_cast<Variant*>(&p_default_value);
    }

    /// Copies the stack element at the specified offset to the input stack
    /// @param stack_offset stack offset to copy from
    /// @param input_offset input offset to write to
    _FORCE_INLINE_ void copy_stack_to_input(int stack_offset, int input_offset)
    {
        _execution_stack->_inputs[input_offset] = &_execution_stack->_variant_stack[stack_offset];
    }

    /// Copies the stack element at the specified offset to the output stack
    /// @param stack_offset stack offset to copy from
    /// @param output_offset output offset to write to
    _FORCE_INLINE_ void copy_stack_to_output(int stack_offset, int output_offset)
    {
        _execution_stack->_outputs[output_offset] = &_execution_stack->_variant_stack[stack_offset];
    }
};

#endif  // ORCHESTRATOR_EXECUTION_CONTEXT_H
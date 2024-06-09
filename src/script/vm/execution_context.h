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
#ifndef ORCHESTRATOR_SCRIPT_EXECUTION_CONTEXT_H
#define ORCHESTRATOR_SCRIPT_EXECUTION_CONTEXT_H

#include <godot_cpp/variant/variant.hpp>

using namespace godot;

/// Forward declarations
class OScriptNodeInstance;
class OScriptState;
class OScriptVirtualMachine;

/// Defines the metadata details about the execution stack's layout
///
/// The execution stack used by Orchestrator represents all the state, including the curent inputs,
/// outputs, flow control, data and execution passes, and working memory. The stack structure is
/// as follows:
///
/// @code
/// +-------------------------------+
/// | variants     [max_stack_size] |
/// | executions   [node_count]     |
/// | inputs       [max_inputs]     |
/// | outputs      [max_outputs]    |
/// | flow stack   [flow_size]      |
/// | pass stack   [pass_size]      |
/// +-------------------------------+
/// @endcode
///
struct OScriptExecutionStackInfo
{
    int max_stack_size{ 0 };  //! Max stack size
    int node_count{ 0 };      //! Number of nodes
    int max_inputs{ 0 };      //! Maximum number of inputs
    int max_outputs{ 0 };     //! Maximum number of outputs
    int flow_size{ 0 };       //! Flow stack size
    int pass_size{ 0 };       //! Pass stack size

    /// Get the calculated stack size based on the metadata
    /// @return the calculated stack size
    int get_stack_size() const;
};

/// The main script execution context which manages the execution state, stack and other runtime details.
class OScriptExecutionContext
{
    friend class OScriptState;
    friend class OScriptVirtualMachine;

protected:
    OScriptExecutionStackInfo _info;              //! The execution stack metadata
    void* _stack{ nullptr };                      //! The allocated stack
    Variant* _variant_stack{ nullptr };           //! The variant stack for storing values
    bool* _execution_stack{ nullptr };            //! The node execution stack
    Variant** _inputs{ nullptr };                 //! The input values for the current step
    Variant** _outputs{ nullptr };                //! The output values for the current step
    int* _flow_stack{ nullptr };                  //! The flow stack
    int* _pass_stack{ nullptr };                  //! The node pass stack

    OScriptVirtualMachine* _instance{ nullptr };  //! The virtual machine instance

    int _initial_node_id{ -1 };                   //! Initial starting node ID
    int _current_node_id{ -1 };                   //! The current executing node ID
    int _current_node_port{ -1 };                 //! The current executing node impulse port
    int _passes{ 0 };                             //! The current number of passes
    int _step_mode{ 0 };                          //! The current step mode
    int _flow_stack_position{ 0 };                //! The current flow stack position
    int _current_node_working_memory{ 0 };        //! The current node working memory position
    Variant* _working_memory{ nullptr };          //! The working memory

    GDExtensionCallError* _error{ nullptr };      //! The call error reference
    String _error_reason;                         //! The error reason

    /// Initialize the variant stack using memnew_placement
    void _initialize_variant_stack();

    /// Get the stack pointer
    /// @return the stack pointer
    const void* _get_stack() const { return _stack; }

    /// Sets the current node's working memory index
    /// @param p_index the working memory index
    void _set_current_node_working_memory(int p_index) { _current_node_working_memory = p_index; }

    /// Sets whether the specified node has been executed
    /// @param p_index the execution stack index to mutate
    /// @param p_state whether the node has executed
    _FORCE_INLINE_ void _set_node_execution_state(int p_index, bool p_state) { _execution_stack[p_index] = p_state; }

    /// Copy the specified number of variants from the top of the stack as inputs
    /// @param p_count the number of variants to copy
    _FORCE_INLINE_ void _copy_stack_to_inputs(int p_count)
    {
        for (int i = 0; i < p_count; i++)
            _inputs[i] = &_variant_stack[i];
    }

    /// Set the input value at the specified index to the default value
    /// @param p_index the input stack index to mutate
    /// @param p_value the default value to use
    _FORCE_INLINE_ void _set_input_from_default_value(int p_index, const Variant& p_value) { _inputs[p_index] = const_cast<Variant*>(&p_value); }

    /// Copies a variant stack value to the input stack at the given indices
    /// @param p_stack_index the variant stack index to copy from
    /// @param p_input_index the input stack index to write to
    _FORCE_INLINE_ void _copy_stack_to_input(int p_stack_index, int p_input_index) { _inputs[p_input_index] = &_variant_stack[p_stack_index]; }

    /// Copies a variant stack value to the output stack at the given indices
    /// @param p_stack_index the variant stack index to copy from
    /// @param p_output_index the output stack index to write to
    _FORCE_INLINE_ void _copy_stack_to_output(int p_stack_index, int p_output_index) { _outputs[p_output_index] = &_variant_stack[p_stack_index]; }

    /// Pushes the arguments onto the variant stack
    /// @param p_args the arguments to push
    /// @param p_count the number of arguments
    _FORCE_INLINE_ void _push_arguments(const Variant* const* p_args, int p_count)
    {
        for (int i = 0; i < p_count; i++)
            _variant_stack[i] = *p_args[i];
    }

    //~ Begin Flow Stack Interface
    _FORCE_INLINE_ bool _has_flow_stack() const { return _flow_stack != nullptr; }
    _FORCE_INLINE_ int _get_flow_stack_size() const { return _info.flow_size; }
    _FORCE_INLINE_ int _get_flow_stack_value(int p_index) const { return _flow_stack[p_index]; }
    _FORCE_INLINE_ int _get_flow_stack_position() const { return _flow_stack_position;}
    _FORCE_INLINE_ void _increment_flow_stack_position() { _flow_stack_position++; }
    _FORCE_INLINE_ void _decrement_flow_stack_position() { _flow_stack_position--; }
    _FORCE_INLINE_ void _set_flow_stack_position(int p_index) { _flow_stack_position = p_index; }
    _FORCE_INLINE_ void _push_node_onto_flow_stack(int p_node_id) { if (_flow_stack) { _flow_stack[0] = p_node_id; } }
    //~ End Flow Stack Interface

    //~ Begin Flow Stack Bit Interface
    _FORCE_INLINE_ bool _has_flow_stack_bit(int p_bit) { return _has_flow_stack() && _flow_stack[_flow_stack_position] & p_bit; }
    _FORCE_INLINE_ void _set_flow_stack_bit(int p_bit) { _flow_stack[_flow_stack_position] |= p_bit; }
    _FORCE_INLINE_ void _set_flow_stack(int p_node_id) { _flow_stack[_flow_stack_position] = p_node_id; }
    _FORCE_INLINE_ int _get_flow_stack_value() const { return _get_flow_stack_value(_flow_stack_position);}
    //~ End Flow Stack Bit Interface

    //~ Begin Pass Interface
    _FORCE_INLINE_ int _get_pass_at(int p_index) { return _pass_stack[p_index]; }
    _FORCE_INLINE_ void _add_current_pass(int p_index) { _pass_stack[p_index] = _passes; }
    //~ End Pass Interface

    /// Cleans up the variant stack
    void _cleanup();

    /// Cleanup the variant stack
    /// @param p_info the stack information
    /// @param p_stack the variant stack to cleanup
    static void _cleanup_stack(OScriptExecutionStackInfo& p_info, const Variant* p_stack);

public:
    /// Get the current runtime virtual machine reference.
    /// @return the owning virtual machine
    _FORCE_INLINE_ OScriptVirtualMachine* get_runtime() { return _instance; }

    /// Gets the owner object, typically the owner of the virtual machine.
    /// @return the owner object, should never be <code>null</code>.
    Object* get_owner();

    /// Get the execution stack metadata
    /// @return the metadata
    OScriptExecutionStackInfo get_metadata() const { return _info; }

    /// Get the current execution step mode
    /// @return the current step mode
    int get_step_mode() const { return _step_mode; }

    /// Sets the current step mode
    /// @param p_step_mode the step mode
    void set_step_mode(int p_step_mode) { _step_mode = p_step_mode; }

    /// Get the current pass stack count
    /// @return the pass count
    int get_passes() const { return _passes; }

    /// Gets the current executing node unique ID
    /// @return the current node unique ID
    int get_current_node() const { return _current_node_id; }

    /// Get the current node port that received the impulse
    /// @return the current node port with the impulse
    _FORCE_INLINE_ int get_current_node_port() const { return _current_node_port; }

    /// Checks whether the specified node has been executed
    /// @param p_index the node's execution index
    /// @return true if the node has been executed, false otherwise
    _FORCE_INLINE_ bool has_node_executed(int p_index) const { return _execution_stack[p_index]; }

    //~ Begin Error Interface
    _FORCE_INLINE_ bool has_error() const { return _error && _error->error != GDEXTENSION_CALL_OK; }
    GDExtensionCallError& get_error() { return *_error; }
    String get_error_reason() const { return _error_reason; }
    void set_error(GDExtensionCallErrorType p_type, const String& p_reason = String());
    void set_invalid_argument(OScriptNodeInstance* p_instance, int p_index, Variant::Type p_type, Variant::Type p_expected_type);
    void clear_error();
    //~ End Error Interface

    //~ Begin Working Memory Interface
    _FORCE_INLINE_ bool has_working_memory() const { return _working_memory != nullptr; }
    _FORCE_INLINE_ Variant get_working_memory(int p_index = 0) { static Variant empty = Variant(); return has_working_memory() ? _working_memory[p_index] : empty; }
    _FORCE_INLINE_ void set_working_memory(int p_index) { _working_memory = p_index >= 0 ? &(_variant_stack[p_index]) : nullptr; }
    _FORCE_INLINE_ void set_working_memory(int p_index, const Variant& p_value){ _working_memory[p_index] = p_value; }
    //~ End Working Memory Interface

    //~ Begin Inputs Interface
    _FORCE_INLINE_ Variant& get_input(int p_index) { return *_inputs[p_index]; }
    _FORCE_INLINE_ const Variant** get_input_ptr() { return const_cast<const Variant**>(_inputs); }
    _FORCE_INLINE_ void set_input(int p_index, const Variant* p_value) { _inputs[p_index] = const_cast<Variant*>(p_value); }
    //~ End Inputs Interface

    /// Copies the specified number of variants from the input to the output stack.
    /// @param p_count the number of variants to copy
    void copy_inputs_to_outputs(int p_count);

    /// Copies the specified input stack variant at the given index to the output stack at the given index.
    /// @param p_input_index the input index to copy from
    /// @param p_output_index the output index to copy to
    void copy_input_to_output(size_t p_input_index, size_t p_output_index);

    //~ Begin Outputs Interface
    _FORCE_INLINE_ Variant& get_output(int p_index) { return *_outputs[p_index]; }
    _FORCE_INLINE_ bool set_output(int p_index, const Variant& p_value) { *_outputs[p_index] = p_value; return true; }
    _FORCE_INLINE_ bool set_output(int p_index, Variant* p_value) { *_outputs[p_index] = *p_value; return true; }
    //~ End Outputs Interface

    /// Construct the execution context
    /// @param p_stack_info the stack information
    /// @param p_stack the stack pointer
    /// @param p_flow_position the flow stack position
    /// @param p_passes the pass count
    OScriptExecutionContext(OScriptExecutionStackInfo p_stack_info, void* p_stack, int p_flow_position, int p_passes);
};

#endif  // ORCHESTRATOR_SCRIPT_EXECUTION_CONTEXT_H
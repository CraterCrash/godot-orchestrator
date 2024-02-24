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
#ifndef ORCHESTRATOR_EXECUTION_STACK_H
#define ORCHESTRATOR_EXECUTION_STACK_H

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

/// An information object used to create an execution stack
struct OScriptExecutionStackInfo
{
    int max_stack_size{ 0 };
    int node_count{ 0 };
    int max_inputs{ 0 };
    int max_outputs{ 0 };
    int flow_size{ 0 };
    int pass_size{ 0 };

    int get_stack_size() const;
};

/// The execution stack used by the OrchestratorScript. The stack represents all the state, including
/// the current inputs, outputs, flow control, pass data, execution data, and working memory. The
/// stack structure is as follows:
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
///
class OScriptExecutionStack
{
    friend class OScriptNodeExecutionContext;
    friend class OScriptExecutionContext;

    OScriptExecutionStack() = default;

    OScriptExecutionStackInfo _info;      //! The execution stack metadata
    void* _stack{ nullptr };              //! The allocated memory block

    // Defines pointers into the stack for quick reference
    Variant* _variant_stack{ nullptr };
    bool* _execution_bits{ nullptr };
    Variant** _inputs{ nullptr };
    Variant** _outputs{ nullptr };
    int* _flow{ nullptr };
    int* _pass{ nullptr };
    bool _allocated{ false };

public:
    /// Constructor
    /// @param p_stack_info the stack construction metadata object
    /// @param p_stack an existing stack
    /// @param p_init whether the stack needs initialization
    /// @param p_allocated whether the stack should be deallocated
    OScriptExecutionStack(const OScriptExecutionStackInfo& p_stack_info, void* p_stack, bool p_init, bool p_allocated = true);

    /// ~Destructor
    ~OScriptExecutionStack();

    /// Get the pointer to the underlying stack buffer
    void* get_stack_ptr() const { return _stack; }

    /// Get the metadata details about the stack's sizes and construction details
    /// @return the execution stack information
    const OScriptExecutionStackInfo& get_metadata() const { return _info; }

    /// Cleanup the variant stack
    void cleanup_variant_stack();

    /// Cleanup the specified variant stack
    /// @param p_info the execution stack innformation
    /// @param p_stack the stack to clean up
    static void cleanup_variant_stack(const OScriptExecutionStackInfo& p_info, Variant* p_stack);

    /// @brief Push a node onto the graph flow stack
    /// @param p_node_id the node id
    void push_node_onto_flow_stack(int p_node_id);

    /// @brief Push the provided arguments onto the stack
    /// @param p_args the arguments
    /// @param p_count number of arguments
    void push_arguments(const Variant* const* p_args, int p_count);

    /// Dump the contents of the execution stack to the console.
    void dump();

    /// Dump the contents of the variant stack to the console.
    void dump_variant_stack();

    /// Dump the contents of the input stack to the console.
    void dump_input_stack();

    /// Dump the contents of the output stack to the console.
    void dump_output_stack();

};

#endif  // ORCHESTRATOR_EXECUTION_STACK_H
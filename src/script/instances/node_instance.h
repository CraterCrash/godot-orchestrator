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
#ifndef ORCHESTRATOR_SCRIPT_NODE_INSTANCE_H
#define ORCHESTRATOR_SCRIPT_NODE_INSTANCE_H

#include "script/context/execution_context.h"
#include "script_instance.h"

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/templates/vector.hpp>

using namespace godot;

/// Forward declarations
class OScriptNode;

/// The runtime instance of an OScriptNode object.
///
/// When an Orchestration (OrchestratorScript) is loaded and prepares to run, each node in the
/// graph construct an OScriptNodeInstance, which acts as the runtime data holder for the step
/// that specific node is to execute during the lifetime of the script instance.
///
/// This class is not exposed to Godot intentionally, but does derive from godot::Object as to
/// be able to use certain Godot features such as emit_signal and connect for various runtime
/// node behaviors.
///
class OScriptNodeInstance : public Object
{
    friend class OScriptInstance;

public:
    /// Defines the different modes for the call to the step method.
    enum StepMode
    {
        STEP_MODE_BEGIN,     //! Start from the beginning
        STEP_MODE_CONTINUE,  //! Continue execution of the step where it last left off
        STEP_MODE_RESUME     //! Resume from await
    };

    /// Defines the different ways to handle input
    enum InputMask
    {
        INPUT_SHIFT = 1 << 24,
        INPUT_MASK = INPUT_SHIFT - 1,
        INPUT_DEFAULT_VALUE_BIT = INPUT_SHIFT,
    };

    /// Defines different step result mask types
    enum StepResultMask
    {
        STEP_SHIFT = 1 << 24,
        STEP_MASK = STEP_SHIFT - 1,                  //! Step result mask
        STEP_FLAG_PUSH_STACK_BIT = STEP_SHIFT,       //! Push node back onto the execution stack (call again)
        STEP_FLAG_GO_BACK_BIT = STEP_SHIFT << 1,     //! Go back to previous node
        STEP_FLAG_NO_ADVANCE = STEP_SHIFT << 2,      //! Don't advance past this node
        STEP_FLAG_END = STEP_SHIFT << 3,             //! Return from function call
        STEP_FLAG_YIELD = STEP_SHIFT << 4,           //! Yield
        FLOW_STACK_PUSHED_BIT = 1 << 30,             //! Must come back here at end of sequence
        FLOW_STACK_MASK = FLOW_STACK_PUSHED_BIT - 1  //! Flow stack mask
    };

protected:
    OScriptNode* _base{ nullptr };                       //! The node this runtime instance represents
    int id{ 0 };                                         //! The node's unique identifier
    int execution_index{ 0 };                            //! The execution index
    OScriptNodeInstance** execution_outputs{ nullptr };  //! The outputs
    int* execution_output_pins{ nullptr };               //! The execution output pins
    int execution_output_pin_count{ 0 };                 //! The number of execution output pins
    int execution_input_pin_count{ 0 };                  //! The number of execution input pins
    Vector<OScriptNodeInstance*> dependencies;           //! List of node instance dependencies for this node
    int* input_pins{ nullptr };                          //! Input pins
    int input_pin_count{ 0 };                            //! Input pin count
    int* output_pins{ nullptr };                         //! Output pins
    int output_pin_count{ 0 };                           //! Output pin count
    int working_memory_index{ 0 };                       //! Number of working memory slots
    int pass_index{ 0 };                                 //! The pass index
    int data_input_pin_count{ 0 };
    int data_output_pin_count{ 0 };

public:
    /// Get the node instance's node unique id
    int get_id();

    /// Get the working memory size needed during runtime.
    /// @return the number of variants to allocate on the stack for working memory
    virtual int get_working_memory_size() const { return 0; }

    /// Get the node this runtime instance represents
    /// @return the non-runtime node
    Ref<OScriptNode> get_base_node();

    /// Executes a single step for this node during a frame
    /// @param execution context
    /// @return the output port and bits
    virtual int step(OScriptNodeExecutionContext& p_context) = 0;
};

#endif  // ORCHESTRATOR_SCRIPT_NODE_INSTANCE_H
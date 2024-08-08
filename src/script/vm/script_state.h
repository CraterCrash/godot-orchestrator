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
#ifndef ORCHESTRATOR_SCRIPT_STATE_H
#define ORCHESTRATOR_SCRIPT_STATE_H

#include "script/vm/execution_context.h"
#include "script/vm/script_vm.h"

#include <godot_cpp/classes/ref_counted.hpp>

using namespace godot;

/// A state object that stores runtime state.
///
/// During the execution of an Orchestration, a node may request a yield/await at any point, allowing the
/// plug-in to return execution control directly back to the Godot engine. This state class will use the
/// Godot signal system to trigger when the yield/await has finished, allowing the plug-in to resume the
/// script's execution.
///
/// To facilitate this activity, this state class stores the execution stack details and other metadata,
/// restoring this state when the yield/await signal is triggered.
///
class OScriptState : public RefCounted
{
    friend class OScriptVirtualMachine;

    GDCLASS(OScriptState, RefCounted);
    static void _bind_methods();

protected:
    ObjectID _instance_id;                                  //! Script intance object ID
    ObjectID _script_id;                                    //! Script intance ID
    OScriptInstance* _script_instance{ nullptr };           //! Script instance
    OScriptVirtualMachine* _instance{ nullptr };            //! Virtual machine runtime (owned by script instance)
    OScriptVirtualMachine::Function* _func_ptr{ nullptr };  //! Executing function pointer
    OScriptNodeInstance* _node{ nullptr };                  //! Node that requested the yield/await
    StringName _function;                                   //! The function name that was being executed
    Vector<uint8_t> _stack;                                 //! The execution stack buffer
    OScriptExecutionStackInfo _stack_info;                  //! The execution stack details
    int _working_memory_index{ 0 };                         //! The working memory position
    int _variant_stack_size{ 0 };                           //! The variant stsack size
    int _flow_stack_pos{ 0 };                               //! The flow stack position
    int _pass{ 0 };                                         //! The current number of passes

    /// The signal callback, which is dispatched when the resume signal is triggered.
    /// @param p_args the signal argument list, if any
    /// @param p_argcount the number of arguments, or 0 if no arguments supplied
    /// @param r_err the callback error to return after the callback is processed
    void _signal_callback(const Variant** p_args, GDExtensionInt p_argcount, GDExtensionCallError& r_err);

    /// Call the method
    /// @param r_error the error code
    /// @return the result of the execution
    Variant _call_method(GDExtensionCallError& r_error);

public:
    /// Connect to a specific signal, binding the optional values. When the signal is triggered,
    /// the <code>_signal_callback</code> method will be notified.
    /// @param p_object the object to bind
    /// @param p_signal the name of the signal to bind
    /// @param p_bindings the optional array of signal arguments
    void connect_to_signal(Object* p_object, const String& p_signal, const Array& p_bindings);

    /// Checks whether the script state is considered valid.
    /// @return true if the state is valid, false otherwise
    bool is_valid() const;

    /// Resumes the script state without the use of a signal.
    /// @param p_args the argument bindings to resume with
    /// @return the output value after resuming the execution based on the provided state
    Variant resume(const Array& p_args);

    /// Destructor
    ~OScriptState() override;
};

#endif  // ORCHESTRATOR_SCRIPT_STATE_H
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
#ifndef ORCHESTRATOR_SCRIPT_VIRTUAL_MACHINE_H
#define ORCHESTRATOR_SCRIPT_VIRTUAL_MACHINE_H

#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/rb_set.hpp>
#include <godot_cpp/templates/vector.hpp>

using namespace godot;

/// Forward declarations
class Orchestration;
class OScriptCompileContext;
class OScriptExecutionContext;
class OScriptExecutionStack;
class OScriptFunction;
class OScriptInstance;
class OScriptLanguage;
class OScriptNode;
class OScriptNodeInstance;
class OScriptVariable;
class OScriptState;

struct OScriptConnection;

/// The runtime virtual machine for Orchestrations
class OScriptVirtualMachine
{
    friend class OScriptLanguage;
    friend class OScriptState;

public:
    /// Defines details about a function
    struct Function
    {
        int node{ 0 };                             //! Functions starting node ID
        int max_stack{ 0 };                        //! Maximum value stack size
        int trash_pos{ 0 };                        //! Function's trash position in the stack
        int flow_stack_size{ 0 };                  //! Flow stack size
        int pass_stack_size{ 0 };                  //! Pass stack size
        int node_count{ 0 };                       //! Number of nodes in the function's graph
        int argument_count{ 0 };                   //! Number of function arguments
        HashMap<StringName, Variant> _variables;   //! Function local variables
        OScriptNodeInstance* instance{ nullptr };  //! Cached instance of the node that starts this function
    };

    /// Defines details about a variable
    struct Variable
    {
        Variant value;       //! The variable's current value
        bool exported;       //! Publically accessible, exported
        Variant::Type type;  //! Variable type
    };

protected:

    Object* _owner{ nullptr };                  //! The owner
    Ref<Script> _script;                        //! The script
    HashMap<StringName, Variable> _variables;   //! Defined Variables
    HashMap<StringName, Function> _functions;   //! Defined functions
    HashMap<int, OScriptNodeInstance*> _nodes;  //! Nodes
    Vector<Variant> _default_values;            //! Default values
    int _max_inputs{ 0 };                       //! Maximum number of input arguments
    int _max_outputs{ 0 };                      //! Maximum number of output arguments
    int _max_call_stack{ 0 };                   //! Maximum call stack

    /// Sets unassigned inputs on the specified node, if any exist.
    /// @param p_node the script node
    /// @param p_instance the node instance
    /// @param r_function the function declaration
    void _set_unassigned_inputs(const Ref<OScriptNode>& p_node, const OScriptNodeInstance* p_instance, Function& r_function);

    /// Sets unassigned outputs on the specified node, if any exist.
    /// @param p_node the script node
    /// @param p_instance the node instance
    /// @param p_stack_pos the trash stack position
    void _set_unassigned_outputs(const Ref<OScriptNode>& p_node, const OScriptNodeInstance* p_instance, int p_stack_pos);

    /// Build the execution path for a given orchestration starting a specified node
    /// @param p_orchestration the orchestration
    /// @param p_node_id the starting node unique ID
    /// @param r_connections the execution connections traversed
    /// @param r_execution_path the execution path in linear execution order
    void _get_execution_path(Orchestration* p_orchestration, int p_node_id, RBSet<OScriptConnection>& r_connections, RBSet<int>& r_execution_path);

    /// Capture the data connection lookup
    /// @param p_orchestration the orchestration
    /// @param r_lookup the lookup map
    void _get_data_connection_lookup(Orchestration* p_orchestration, HashMap<int, HashMap<int, Pair<int, int>>>& r_lookup);

    /// Create node instance pins
    /// @param p_node the script node
    /// @param p_instance the node instance
    /// @return true if the pins are created successfully, false otherwise
    bool _create_node_instance_pins(const Ref<OScriptNode>& p_node, OScriptNodeInstance* p_instance);

    /// Create a node instance
    /// @param p_orchestration the orchestration
    /// @param p_node_id the node ID
    /// @param r_function the function declaration
    /// @param r_lv_indices the constructe local variable indices
    /// @return true if the node instance was created successfully, false otherwise
    bool _create_node_instance(Orchestration* p_orchestration, int p_node_id, Function& r_function, HashMap<String, int>& r_lv_indices);

    /// Build the function's node graph
    /// @param p_function the script function
    /// @param r_function the function declaration
    /// @param r_lv_indices the constructed local variable indices
    /// @return true if the node graph was built successfully, false otherwise
    bool _build_function_node_graph(const Ref<OScriptFunction>& p_function, Function& r_function, HashMap<String, int>& r_lv_indices);

    /// Resolve the inputs for a function method
    /// @param p_context the execution context
    /// @param p_instance the node intance
    /// @param p_function the function call
    void _resolve_inputs(OScriptExecutionContext& p_context, OScriptNodeInstance* p_instance, Function* p_function);

    /// Copy the stack to the node's outputs
    /// @param p_context the execution context
    /// @param p_instance the node instance
    void _copy_stack_to_node_outputs(OScriptExecutionContext& p_context, OScriptNodeInstance* p_instance);

    /// Resolves the step mode
    /// @param p_context the execution context
    /// @param r_resume the resume flag
    void _resolve_step_mode(OScriptExecutionContext& p_context, bool& r_resume);

    /// Execute all dependency node steps.
    /// @param p_context the execution context
    /// @param p_instance the node instance
    /// @param r_error_node the node that caused the dependency error, if any
    void _dependency_step(OScriptExecutionContext& p_context, OScriptNodeInstance* p_instance, OScriptNodeInstance** r_error_node);

    /// Execute the node instance's step function
    /// @param p_context the execution context
    /// @param p_instance the node instance to step
    /// @return the step result
    int _execute_step(OScriptExecutionContext& p_context, OScriptNodeInstance* p_instance);

    /// Resolve the next node instance to step
    /// @param p_context the execution context
    /// @param p_instance the previously executed node instance
    /// @param p_result the result of the previous node execution
    /// @param p_next_node_id the next node id to execute
    /// @return the next node instance to step/execute, can be <code>null</code> if not found.
    OScriptNodeInstance* _resolve_next_node(OScriptExecutionContext& p_context, OScriptNodeInstance* p_instance, int p_result, int p_next_node_id);

    /// Resolves the next node port
    /// @param p_instance the node instance that previously executed
    /// @param p_next_node_id the next executing node id
    /// @param p_next the next executing node
    /// @return the next node's port that will receive the impulse
    int _resolve_next_node_port(OScriptNodeInstance* p_instance, int p_next_node_id, OScriptNodeInstance* p_next);

    /// Set the node's flow execution state
    /// @param p_context the execution context
    /// @param p_instance the node instance
    /// @param p_result the previous step's result
    void _set_node_flow_execution_state(OScriptExecutionContext& p_context, OScriptNodeInstance* p_instance, int p_result);

    /// Reports the error from the context
    /// @param p_context the execution context
    /// @param p_instance the executing node instance
    /// @param p_method the method that was called
    void _report_error(OScriptExecutionContext& p_context, OScriptNodeInstance* p_instance, const StringName& p_method);

    /// Executes the specified method
    /// @param p_method the method to execute
    /// @param p_context the execution context
    /// @param p_resume whether to resume from a prior yield
    /// @param p_instance the node instance
    /// @param p_function the function being executed
    /// @param r_return the return value, if applicable
    /// @param r_err the return error code
    void _call_method_internal(const StringName& p_method, OScriptExecutionContext* p_context, bool p_resume, OScriptNodeInstance* p_instance, Function* p_function, Variant& r_return, GDExtensionCallError& r_err);

public:
    /// Get the owner of the virtual machine
    /// @return the owner
    Object* get_owner() const { return _owner; }

    /// Set the virtual machine owner
    /// @param p_owner the owner of the virtual machine
    void set_owner(Object* p_owner) { _owner = p_owner; }

    /// Set the script instance
    /// @param p_script the script instance
    void set_script(const Ref<Script>& p_script) { _script = p_script; }

    /// Register a variable
    /// @param p_variable the variable to be registered
    /// @return true if the variable was registered successfully, false otherwise
    bool register_variable(const Ref<OScriptVariable>& p_variable);

    /// Gets the variable by name
    /// @param p_name the variable name
    /// @return the variable if found, otherwise returns null
    Variable* get_variable(const StringName& p_name) const;

    /// Get the value of a variable
    /// @param p_name the variable name
    /// @param r_value the return value
    /// @return true if the variable was found, false otherwise
    bool get_variable(const StringName& p_name, Variant& r_value) const;

    /// Set the value of a variable
    /// @param p_name the variable name
    /// @param p_value the value of the variable to set
    /// @return true if the value was set, false otherwise
    bool set_variable(const StringName& p_name, const Variant& p_value);

    /// Register a function
    /// @param p_function the function to be registered
    /// @return true if the function was registered successfully, false othrewise
    bool register_function(const Ref<OScriptFunction>& p_function);

    /// Executes or calls the specified method
    /// @param p_instance the script instance that made the call
    /// @param p_method the method name to run
    /// @param p_args the method arguments
    /// @param p_arg_count the number of method arguments
    /// @param r_return the return value, if applicable
    /// @param r_err the return code, if applicable
    void call_method(OScriptInstance* p_instance, const StringName& p_method, const Variant* const* p_args, GDExtensionInt p_arg_count, Variant* r_return, GDExtensionCallError* r_err);

    /// Constructs the virtual machine
    OScriptVirtualMachine();

    /// Destroys the virtual machine
    ~OScriptVirtualMachine();
};

#endif  // ORCHESTRATOR_SCRIPT_VIRTUAL_MACHINE_H

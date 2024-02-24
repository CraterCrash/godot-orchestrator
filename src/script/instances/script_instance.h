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
#ifndef ORCHESTRATOR_SCRIPT_INSTANCE_H
#define ORCHESTRATOR_SCRIPT_INSTANCE_H

#include "instance_base.h"
#include "node_instance.h"

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/templates/hash_map.hpp>

using namespace godot;

/// Forward declarations
class OScriptLanguage;
class OScript;
class OScriptNode;
class OScriptState;

/// The runtime instance of an OScript object.
///
/// When an Orchestration (OrchestratorScript) is loaded and prepares to run, the script creates
/// an instance of an OScriptInstance, which maintains the runtime state of the executing script
/// object.
///
/// This instance type represents the game instance, the one that does not run within the editor
/// but instead runs when the scene is running outside the editor's scope.
///
class OScriptInstance : public OScriptInstanceBase
{
    friend class OScriptState;

    /// Function attributes
    struct Function
    {
        int node{ 0 };                             //! The function's node unique identifier
        int max_stack{ 0 };                        //! The function's maximum value stack size
        int trash_pos{ 0 };                        //! The position in the stack for writing to trash
        int flow_stack_size{ 0 };                  //! The function's flow stack size
        int pass_stack_size{ 0 };                  //! The function's pass stack size
        int node_count{ 0 };                       //! The node count?
        int argument_count{ 0 };                   //! The number of arguments for the function call.
        OScriptNodeInstance* instance{ nullptr };  //! Cached instance of the node that starts the function
    };

    Ref<OScript> _script;                       //! The script this instance represents
    Object* _owner{ nullptr };                  //! The owning object of the script
    OScriptLanguage* _language{ nullptr };      //! The language the script represents
    HashMap<StringName, Variant> _variables;    //! Variables defined in the script
    HashMap<StringName, bool> _exported_vars;   //! Exported variables
    HashMap<StringName, Function> _functions;   //! Functions defined in the script
    HashMap<int, OScriptNodeInstance*> _nodes;  //! Map of all the script node instances
    Vector<Variant> _default_values;            //! Registered default values
    int _max_input_args{ 0 };                   //! Maximum number of input arguments
    int _max_output_args{ 0 };                  //! Maximum number of output arguments
    int _max_call_stack{ 0 };                   //! cached configured maximum function call stack

public:
    /// Defines details about the script instance to be passed to Godot
    static const GDExtensionScriptInstanceInfo2 INSTANCE_INFO;

    /// Create an OScriptInstance object
    /// @param p_script the orchestrator script this instance represents
    /// @param p_language the language object
    /// @param p_owner the owner of the script instance
    OScriptInstance(const Ref<OScript>& p_script, OScriptLanguage* p_language, Object* p_owner);

    /// OScriptInstance destructor
    ~OScriptInstance() override;

    //~ Begin OScriptInstanceBase Interface
    bool set(const StringName& p_name, const Variant& p_value, PropertyError* r_err) override;
    bool get(const StringName& p_name, Variant& p_value, PropertyError* r_err) override;
    GDExtensionPropertyInfo* get_property_list(uint32_t* r_count) override;
    Variant::Type get_property_type(const StringName& p_name, bool* r_is_valid) const override;
    bool has_method(const StringName& p_name) const override;
    Object* get_owner() const override;
    Ref<OScript> get_script() const override;
    ScriptLanguage* get_language() const override;
    bool is_placeholder() const override;
    //~ End OScriptInstanceBase Interface

    //~ Begin ScriptInstanceInfo2 Interface
    bool property_can_revert(const StringName& p_name);
    bool property_get_revert(const StringName& p_name, Variant* r_ret);
    void call(const StringName& p_method, const Variant* const* p_args, GDExtensionInt p_arg_count, Variant* r_return,
              GDExtensionCallError* r_err);
    void notification(int32_t p_what, bool p_reversed);
    void to_string(GDExtensionBool* r_is_valid, String* r_out);
    //~ End ScriptInstanceInfo2 Interface

    /// Get the base node/object type the script is based on
    /// @return the base type of the script
    String get_base_type() const;

    /// Set the base node/object type
    /// @param p_base_type the base type the script instance is based on
    void set_base_type(const String& p_base_type);

    /// Get a script defined variable
    /// @param p_name the variable name
    /// @param r_value the variable's value, only applicable if the method returns true
    /// @return true if the variable is found; false otherwise
    bool get_variable(const StringName& p_name, Variant& r_value) const;

    /// Set a script defined variable's value.
    /// @param p_name the variable to be set
    /// @param p_value the value to set for the variable
    /// @return true if the variable is set, false otherwise
    bool set_variable(const StringName& p_name, const Variant& p_value);

    /// Helper to lookup an OScriptInstance from a Godot Object reference
    /// @param p_object the godot object to find a script instance about
    /// @return the orchestrator script instance if found; null otherwise
    static OScriptInstance* from_object(GDExtensionObjectPtr p_object);

private:
    /// Initialize the instance's variable state from the script
    /// @param p_script the orchestrator script
    void _initialize_variables(const Ref<OScript>& p_script);

    /// Initialize the instance's function and node graph state from the script
    /// @param p_script the orchestrator script
    void _initialize_functions(const Ref<OScript>& p_script);

    /// Initializes the function's directed acyclic graph (DAG).
    /// @param p_script the orchestrator script
    /// @param p_function the function the graph is constructed for
    /// @param p_lv_indices local variable indices
    /// @return true if the graph is created successfully, false otherwise
    bool _initialize_function_node_graph(const Ref<OScript>& p_script, Function& p_function,
                                         HashMap<String, int>& p_lv_indices);

    /// Creates the node instance for the specified node
    /// @param p_script the orchestrator script
    /// @param p_node_id the orchestrator script node unique id
    /// @param p_function the function the graph is constructed for
    /// @param p_lv_indices the local variable indices
    /// @return true if the instance is created, false otherwise
    bool _create_node_instance(const Ref<OScript>& p_script, int p_node_id, Function& p_function,
                               HashMap<String, int>& p_lv_indices);

    /// Sets all unassigned input pins to their default values
    /// @param p_node the orchestrator script node reference
    /// @param p_instance the instance the pins are being set for
    void _set_unassigned_inputs(const Ref<OScriptNode>& p_node, const OScriptNodeInstance* p_instance);

    /// The internal call method handler. This method is separated from the call function as it can
    /// be called multiple times, particularly in the case where a node may have resumed from a
    /// prior await operation or other stack-based operations.
    ///
    /// @param p_method the method to call if the method has not yet started
    /// @param p_stack the execution stack, should not be {@code null}
    /// @param p_flow_pos the position in the flow stack, typically 0
    /// @param p_passes the number of execution passes ran, typically 0
    /// @param p_resume_yield whether the method call is resuming from a prior yield
    /// @param p_node the current node in the graph that should be called
    /// @param p_function the executing function instance, should not be {@code null}
    /// @param r_err the output error code when an error is encountered
    /// @return the result value, if applicable or null
    Variant _call_internal(const StringName& p_method, OScriptExecutionStack* p_stack, int p_flow_pos,
                           int p_passes, bool p_resume_yield, OScriptNodeInstance* p_node, Function* p_function,
                           GDExtensionCallError& r_err);

    /// Resolve the inputs for an node that is about to be executed.
    /// @param p_context the execution context
    /// @param p_node the node to resolve inputs for
    /// @param p_function the currently executing function reference
    void _resolve_inputs(OScriptExecutionContext& p_context, OScriptNodeInstance* p_node, Function* p_function);

    /// Handles reporting the error from the context
    /// @param p_context the execution context
    /// @param p_node the currently executing node
    /// @param p_method the currently executing outer function
    void _report_error(OScriptExecutionContext& p_context, OScriptNodeInstance* p_node, const StringName& p_method);

    /// Nodes in the execution graph may have input links that are not part of the initial execution
    /// chain for that node, such as a calculation that prepares a static input value for a node. In
    /// these dependency use cases, this method executes those dependency chains and prepares the
    /// value on the execution stack for the node that requires these dependencies.
    ///
    /// @param p_context the execution context
    /// @param p_node the node step to execute
    /// @param r_err the output error code
    void _dependency_step(OScriptExecutionContext& p_context, OScriptNodeInstance* p_node, OScriptNodeInstance** r_err);

    /// Resolve the node execution step mode
    /// @param p_context the execution context
    /// @param p_resume_yield whether the context is resuming from yield
    static void _resolve_step_mode(OScriptExecutionContext& p_context, bool& p_resume_yield);

    /// Sets all unassigned output pins to the trash position
    /// @param p_script the orchestrator script
    /// @param p_function the function reference
    static void _set_unassigned_outputs(const OScriptNodeInstance* p_instance, const Function& p_function);

    /// Creates the node instance pins.
    /// @param p_node the orchestrator script node reference
    /// @param p_instance the instance the pins are created for
    /// @return true if the pins are created successfully, false otherwise
    static bool _create_node_instance_pins(const Ref<OScriptNode>& p_node, OScriptNodeInstance* p_instance);

    /// Resolve the next execution node
    /// @param p_context the execution context
    /// @param p_node the currently executing node
    /// @param p_result the previous step's result value
    /// @param p_output the previous step's requested output node identifier
    /// @return the next node instance to execute, may be null if no node was found
    static OScriptNodeInstance* _resolve_next_node(OScriptExecutionContext& p_context, OScriptNodeInstance* p_node,
                                                   int p_result, int p_output);

    static int _resolve_next_node_port(OScriptNodeExecutionContext& p_context, OScriptNodeInstance* p_node,
                                       int p_result, int p_output, OScriptNodeInstance* p_next);

    /// Sets the node's flow and execution state
    /// @param p_context the execution context
    /// @param p_node the currently executing node
    /// @param p_result the previous step's result value
    static void _set_node_flow_execution_state(OScriptExecutionContext& p_context, OScriptNodeInstance* p_node,
                                               int p_result);

    /// Executes the node's step.
    ///
    /// @param p_context the execution context.
    /// @param p_node the node to execute
    /// @return the step's output
    static int _execute_step(OScriptExecutionContext& p_context, OScriptNodeInstance* p_node);

    /**
     * Copy output pin values from the execution stack.
     * @param p_context execution context
     * @param p_node the node
     */
    static void _copy_stack_to_node_outputs(OScriptExecutionContext& p_context, OScriptNodeInstance* p_node);
};

/// A simple state object that is recorded when a node requests a yield/await operation and for
/// control to be returned back to the engine. In this case, this object stores the execution
/// stack details and other metadata so that it can safely be restored when the script returns
/// from the yield/await operation, which is triggered via a signal.
class OScriptState : public RefCounted
{
    friend class OScriptInstance;
    GDCLASS(OScriptState, RefCounted);

    static void _bind_methods();

    ObjectID instance_id;                            //! The script instance object id
    ObjectID script_id;                              //! The script instance id
    OScriptInstance* instance{ nullptr };            //! The OScript runtime instance object
    StringName function;                             //! The function/method being executed
    Vector<uint8_t> stack;                           //! The execution stack
    OScriptExecutionStackInfo stack_info;            //! The stack information
    int working_mem_index{ 0 };                      //! The current position in the working memory
    int variant_stack_size{ 0 };                     //! The current variant stack size
    OScriptNodeInstance* node{ nullptr };            //! The current OScriptNode runtime instance object
    int flow_stack_pos{ 0 };                         //! The current flow stack position
    int pass{ 0 };                                   //! The current number of passes
    OScriptInstance::Function* func_ptr{ nullptr };  //! The executing function pointer

    /// The signal callback, which is dispatched when the resume signal is raised by
    /// the engine for the OScript to resume execution.
    /// @param p_args the array arguments
    void _signal_callback(const Array& p_args);

public:
    ~OScriptState() override;

    /// Requests that the script state perform a signal request on the specified object to the
    /// specified signal, binding the specified values. When the signal is then fired, the
    /// method "_signal_callback" method is called.
    ///
    /// @param p_object the object to bind the signal to
    /// @param p_signal the signal to listen about
    /// @param p_binds the argument bindings
    void connect_to_signal(Object* p_object, const String& p_signal, Array p_binds);

    /// Check whether the script state is valid
    /// @return true if the state is valid, false otherwise
    bool is_valid() const;

    /// Resumes the script state without using a signal.
    /// @param p_args the argument bindings to resume with
    /// @return the output value or null of there is no output
    Variant resume(const Array& p_args);
};

#endif  // ORCHESTRATOR_SCRIPT_INSTANCE_H
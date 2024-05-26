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
#ifndef ORCHESTRATOR_SCRIPT_H
#define ORCHESTRATOR_SCRIPT_H

#include "context/execution_stack.h"
#include "function.h"
#include "graph.h"
#include "instances/instance_base.h"
#include "language.h"
#include "node.h"
#include "signals.h"
#include "variable.h"

#include <gdextension_interface.h>
#include <godot_cpp/classes/script_extension.hpp>
#include <godot_cpp/classes/script_language.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/rb_set.hpp>

using namespace godot;

/// Forward declarations
class OScriptInstance;
class OScriptPlaceHolderInstance;

/// Defines a connection between two nodes and their respective ports
///
/// This is defined standalone as a connection will be re-used by OScript libraries, which
/// will contain reusable functions and graph macros as importable libraries.
///
struct OScriptConnection
{
    union
    {
        struct
        {
            // Allows for 24 million nodes, each with 255 ports per script.
            uint64_t from_node : 24;
            uint64_t from_port : 8;
            uint64_t to_node   : 24;
            uint64_t to_port   : 8;
        };
        uint64_t id = 0;
    };

    // Default constructor that assigns all bits to zero.
    OScriptConnection() : from_node(0), from_port(0), to_node(0), to_port(0) {}

    // Constructor for explicit id
    explicit OScriptConnection(uint64_t p_id) : id(p_id) {}

    // Needed for associative containers, i.e. RBSet
    bool operator<(const OScriptConnection& p_connection) const { return id < p_connection.id; }

    //~ Begin Serialization Helpers
    static OScriptConnection from_dict(const Dictionary& p_dict);
    Dictionary to_dict() const;
    //~ End Serialization Helpers

    /// Return whether this connection is linked to the specified node id
    /// @param p_id the node id
    /// @return true if the connection is linked to the node on either end, false otherwise
    bool is_linked_to(uint64_t p_id) const { return from_node == p_id || to_node == p_id; }

    /// Get the connection as a formatted string
    String to_string() const { return vformat("%d:%d to %d:%d", from_node, from_port, to_node, to_port); }
};

/// Defines the script extension for Orchestrations.
///
/// An orchestration is a visual-script like graph of nodes that allows to build code visually.
/// These graphs are stored as a script that can be attached to any scene tree node and this is
/// the base class that offers that functionality.
///
class OScript : public ScriptExtension
{
    friend class OScriptInstance;
    GDCLASS(OScript, ScriptExtension);

private:
    bool _tool{ false };                                   //! Is this script marked as a tool script
    bool _valid{ false };                                  //! Determines whether the script is currently valid
    bool _edited{ false };                                 //! Tracks whether the script has been edited
    bool _initialized{ false };                            //! Whether the script has been initialized
    bool _placeholder_fallback_enabled{ false };           //! Deals with placeholders
    StringName _base_type;                                 //! Base type of the script
    OScriptLanguage* _language{ nullptr };                 //! The script language
    RBSet<OScriptConnection> _connections;                 //! All connections
    HashMap<int, Ref<OScriptNode>> _nodes;                 //! All nodes
    HashMap<StringName, Ref<OScriptFunction>> _functions;  //! All functions
    HashMap<StringName, Ref<OScriptVariable>> _variables;  //! All variables
    HashMap<StringName, Ref<OScriptSignal>> _signals;      //! All signals
    HashMap<StringName, Ref<OScriptGraph>> _graphs;        //! All logical graphs (UI only)

    // these are mutable because they're modified within const function callbacks
    mutable HashMap<Object*, OScriptInstance*> _instances;
    mutable HashMap<uint64_t, OScriptPlaceHolderInstance*> _placeholders;

protected:
    // Godot bindings
    static void _bind_methods();

    //~ Begin Serialization API
    TypedArray<OScriptNode> _get_nodes() const;
    void _set_nodes(const TypedArray<OScriptNode>& p_nodes);
    TypedArray<int> _get_connections() const;
    void _set_connections(const TypedArray<int>& p_connections);
    TypedArray<OScriptGraph> _get_graphs() const;
    void _set_graphs(const TypedArray<OScriptGraph>& p_graphs);
    TypedArray<OScriptFunction> _get_functions() const;
    void _set_functions(const TypedArray<OScriptFunction>& p_functions);
    TypedArray<OScriptVariable> _get_variables() const;
    void _set_variables(const TypedArray<OScriptVariable>& p_variables);
    TypedArray<OScriptSignal> _get_signals() const;
    void _set_signals(const TypedArray<OScriptSignal>& p_signals);
    //~ End Serialization API

    /// Get all node ids for a given node type.
    /// @tparam T the node type
    /// @return list of node ids
    template<typename T> List<int> _get_node_type_node_ids()
    {
        List<int> ids;
        for (const KeyValue<int, Ref<OScriptNode>>& E : _nodes)
        {
            if (T* ptr = Object::cast_to<T>(E.value.ptr()))
                ids.push_back(ptr->get_id());
        }
        return ids;
    }

public:
    OScript();

    //~ ScriptExtension overrides
    bool _editor_can_reload_from_file() override;
    void* _placeholder_instance_create(Object* p_object) const override;
    void _placeholder_erased(void* p_placeholder) override;
    bool _is_placeholder_fallback_enabled() const override;
    bool placeholder_has(Object* p_object) const;
    void* _instance_create(Object* p_object) const override;
    bool _instance_has(Object* p_object) const override;
    bool _can_instantiate() const override;
    Ref<Script> _get_base_script() const override;
    bool _inherits_script(const Ref<Script>& p_script) const override;
    StringName _get_global_name() const override;
    StringName _get_instance_base_type() const override;
    bool _has_source_code() const override;
    String _get_source_code() const override;
    void _set_source_code(const String& p_code) override;
    Error _reload(bool p_keep_state) override;
    TypedArray<Dictionary> _get_documentation() const override;
    bool _has_static_method(const StringName& p_method) const override;
    bool _has_method(const StringName& p_method) const override;
    Dictionary _get_method_info(const StringName& p_method) const override;
    TypedArray<Dictionary> _get_script_method_list() const override;
    TypedArray<Dictionary> _get_script_property_list() const override;
    bool _is_tool() const override;
    bool _is_valid() const override;
    ScriptLanguage* _get_language() const override;
    bool _has_script_signal(const StringName& p_signal) const override;
    TypedArray<Dictionary> _get_script_signal_list() const override;
    bool _has_property_default_value(const StringName& p_property) const override;
    Variant _get_property_default_value(const StringName& p_property) const override;
    void _update_exports() override;
    int32_t _get_member_line(const StringName& p_member) const override;
    Dictionary _get_constants() const override;
    TypedArray<StringName> _get_members() const override;
    Variant _get_rpc_config() const override;
    String _get_class_icon_path() const override;
    //~ End ScriptExtension overrides

    /// Performs post load steps after the script has been initialized.
    void post_initialize();

    /// Validates and builds the script
    /// @return true if successful; false otherwise
    bool validate_and_build();

    /// Get the underlying script's language
    /// @return the script language instance
    ScriptLanguage* get_language() const { return _get_language(); }

    /// Get the base type for this script
    /// @return base type name
    StringName get_base_type() const;

    /// Set the base type for this script
    /// @param p_base_type the base type
    void set_base_type(const StringName& p_base_type);

    /// Returns whether the script has been modified
    /// @return true if script modified, false otherwise
    bool is_edited() const;

    /// Get the next available function id
    /// @return function id
    int get_available_id() const;

    /// Check whether the script is marked as a tool script
    /// @return true if the script is marked as tool-mode, false otherwise
    bool get_tool() const { return _is_tool(); }

    /// Set whether the script operates in tool-mode
    /// @param p_tool true sets the script to tool mode, false does not
    void set_tool(bool p_tool) { _tool = p_tool; }

    // Node API

    /// Check whether a specific node with the given unique id exists
    /// @param p_node_id the node id to lookup
    /// @return true if a node is registered with the id, false otherwise
    bool has_node(int p_node_id) const;

    /// Adds the given node to the specified node graph.
    /// @param p_graph the node graph to register the node with
    /// @param p_node the node to be added
    void add_node(const Ref<OScriptGraph>& p_graph, const Ref<OScriptNode>& p_node);

    /// Removes the node by it's unique node identifier
    /// @param p_node_id the node id to remove
    void remove_node(int p_node_id);

    /// Get a node by its unique node identifier
    /// @param p_node_id the node id
    /// @return the node reference if found, an invalid reference if not found
    Ref<OScriptNode> get_node(int p_node_id) const;

    /// Get an immutable list of all nodes registered with this script
    /// @return a list of all node references, may be empty if no nodes are registered
    Vector<Ref<OScriptNode>> get_nodes() const;

    // Connections API

    /// Get the underlying connection set maintained by the script
    /// @return an immutable set of the script's connections
    const RBSet<OScriptConnection>& get_connections() const;

    /// Connect a given source node and port with a target node and port.
    /// @param p_source_id the source node id
    /// @param p_source_port the source node port
    /// @param p_target_id the target node id
    /// @param p_target_port the target node port
    void connect_nodes(int p_source_id, int p_source_port, int p_target_id, int p_target_port);

    /// Disconnect a given source node and port from a target node and port.
    /// @param p_source_id the source node id
    /// @param p_source_port the source node port
    /// @param p_target_id the target node id
    /// @param p_target_port the target node port
    void disconnect_nodes(int p_source_id, int p_source_port, int p_target_id, int p_target_port);

    /// Get a list of pins connected to the specified pin.
    /// @param p_pin the node to get connected pins for
    /// @return list of pin references connected to the argument pin, may be empty
    Vector<Ref<OScriptNodePin>> get_connections(const OScriptNodePin* p_pin) const;

    /// Adjusts the connections associated with the specified node
    /// @param p_node the node
    /// @param p_offset the offset position to relink from
    /// @param p_adjustment the adjustment to make
    /// @param p_dir the direction, defaults to PD_MAX to adjust both input and outputs
    void adjust_connections(const OScriptNode* p_node, int p_offset, int p_adjustment, EPinDirection p_dir = PD_MAX);

    // Graph API

    /// Check whether a graph with the specified name is defined.
    /// @param p_name the graph name
    /// @return true if a graph exists with the name, false otherwise
    bool has_graph(const StringName& p_name) const;

    /// Create a graph with the specified name and flags
    /// @param p_name the graph name
    /// @param p_flags the flags
    /// @return the newly created graph reference or an invalid reference on failure
    Ref<OScriptGraph> create_graph(const StringName& p_name, int p_flags = 0);

    /// Removes a graph by the given name
    /// @param p_name the graph name
    void remove_graph(const StringName& p_name);

    /// Get the graph by name
    /// @param p_name the graph name to lookup
    /// @return the graph reference or an invalid reference if the graph is not found
    Ref<OScriptGraph> get_graph(const StringName& p_name) const;

    /// Get the graph that owns the specified node
    /// @param p_node the node to get its owning graph for
    /// @return the graph reference or an invalid reference if the graph is not found
    Ref<OScriptGraph> find_graph(const Ref<OScriptNode>& p_node);

    /// Rename a graph
    /// @param p_old_name the old graph name
    /// @param p_new_name the new graph name
    void rename_graph(const StringName& p_old_name, const StringName& p_new_name);

    /// Get an immutable list of graphs defined in this script
    /// @return a list of graph references, never empty as there is at least 1 default graph defined
    Vector<Ref<OScriptGraph>> get_graphs() const;

    // Functions API

    /// Check whether a function with the specified name is defined
    /// @param p_name the function name
    /// @return true if the function is defined, false otherwise
    bool has_function(const StringName& p_name) const;

    /// Create a new function in the script
    /// @param p_method the function's method information data structure
    /// @param p_node_id the script node that owns the function declaration
    /// @param p_user_defined whether the function is user-defined or a built-in Godot function
    /// @return a reference to the function or an invalid reference on failure
    Ref<OScriptFunction> create_function(const MethodInfo& p_method, int p_node_id, bool p_user_defined = false);

    /// Remove a function by name
    /// @param p_name the function name
    void remove_function(const StringName& p_name);

    /// Find a function by its name
    /// @param p_name the function name
    /// @return a reference to the function or an invalid reference if not found
    Ref<OScriptFunction> find_function(const StringName& p_name);

    /// Find a function by its globally unique identifier (guid)
    /// @param p_guid the function's guid
    /// @return a reference to the function or an invalid reference if not found
    Ref<OScriptFunction> find_function(const Guid& p_guid);

    /// Renames a function
    /// @param p_old_name the old function name
    /// @param p_new_name the new function name
    void rename_function(const StringName& p_old_name, const String& p_new_name);

    /// Get a list of all defined function names
    /// @return a packed string array of all defined function names
    PackedStringArray get_function_names() const;

    /// Get the owning node unique id for a given function by name
    /// @param p_name the function name
    /// @return the owning node id or -1 if not found
    int get_function_node_id(const StringName& p_name) const;

    // Variables API

    /// Check whether a variable with the specified name is defined
    /// @param p_name the variable name
    /// @return true if a variable is defined with the given name, false otherwise
    bool has_variable(const StringName& p_name) const;

    /// Creates a variable in the script
    /// @param p_name the variable name
    /// @param p_type the variable type, defaults to Variant::NIL
    /// @return the newly created variable reference or an invalid reference on failure
    Ref<OScriptVariable> create_variable(const StringName& p_name, Variant::Type p_type = Variant::NIL);

    /// Removes a variable by the given name
    /// @param p_name the variable name to lookup and remove
    void remove_variable(const StringName& p_name);

    /// Get a variable by name
    /// @param p_name the variable name
    /// @return the variable reference if found, an invalid reference if not found
    Ref<OScriptVariable> get_variable(const StringName& p_name);

    /// Renames a variable definition
    /// @param p_old_name the old variable name
    /// @param p_new_name the new variable name
    void rename_variable(const StringName& p_old_name, const StringName& p_new_name);

    /// Get an immutable list of all variables defined with this script
    /// @return a list of all variable references, may be empty if node variables are defined
    Vector<Ref<OScriptVariable>> get_variables() const;

    /// Get the name of all defined variables
    /// @return a PackedStringArray of all variable names
    PackedStringArray get_variable_names() const;

    /// Checks whether a given variable with the specified name can be removed.
    /// @param p_name the variable name
    /// @return true if the variable can be removed, false otherwise
    bool can_remove_variable(const StringName& p_name) const;

    // Signals API

    /// Check whether a signal with the specified name is defined
    /// @param p_name the signal name
    /// @return true if a signal is defined with the given name, false otherwise
    bool has_custom_signal(const StringName& p_name) const;

    /// Creates a signal in the script
    /// @param p_name the signal name
    /// @return the newly created signal reference or an invalid reference on failure
    Ref<OScriptSignal> create_custom_signal(const StringName& p_name);

    /// Removes a signal by the given name
    /// @param p_name the signal name to lookup and remove
    void remove_custom_signal(const StringName& p_name);

    /// Get a signal by name, printing an error if not found
    /// @param p_name the signal name
    /// @return the signal reference if found, an invalid reference if not found
    Ref<OScriptSignal> get_custom_signal(const StringName& p_name);

    /// Finds a signal by name, printing no error if not found
    /// @param p_name the signal name
    /// @return the signal reference if found, an invalid reference if not found
    Ref<OScriptSignal> find_custom_signal(const StringName& p_name);

    /// Renames a signal definition
    /// @param p_old_name the old signal name
    /// @param p_new_name the new signal name
    void rename_custom_user_signal(const StringName& p_old_name, const StringName& p_new_name);

    /// Get the name of all defined signal
    /// @return a PackedStringArray of all signal names
    PackedStringArray get_custom_signal_names() const;

    /// Checks whether a given signal with the specified name can be removed.
    /// @param p_name the signal name
    /// @return true if the signal can be removed, false otherwise
    bool can_remove_custom_signal(const StringName& p_name) const;

private:
    /// Mark the script as modified.
    /// @param p_edited set to true to mark as modified
    void set_edited(bool p_edited);

    /// Update export placeholders
    /// @param r_err the output error
    /// @param p_recursive whether called recursively
    /// @param p_instance the script instance, should never be null
    bool _update_exports_placeholder(bool* r_err, bool p_recursive, OScriptInstance* p_instance) const;

    /// Updates the placeholders
    void _update_placeholders();
};

void register_script_classes();

#endif  // ORCHESTRATOR_SCRIPT_H
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
#ifndef ORCHESTRATOR_ORCHESTRATION_H
#define ORCHESTRATOR_ORCHESTRATION_H

#include "orchestration/build_log.h"
#include "script/connection.h"
#include "script/function.h"
#include "script/graph.h"
#include "script/node.h"
#include "script/signals.h"
#include "script/variable.h"

#include <godot_cpp/templates/rb_set.hpp>

using namespace godot;

/// Defines different types of orchestrations
enum OrchestrationType
{
    OT_Script  //! An orchestration that acts as a Godot script
};

VARIANT_ENUM_CAST(OrchestrationType);

/// The common contract for different types of Orchestration resources.
///
/// Different resource types can implement this interface in order to act like an Orchestration.  This
/// permits Godot Script types or other Resource types (such as libraries) to act like an Orchestration
/// to share common behavior without needing to duplicate behavior.
///
/// This also allows the existing Orchestrator file format to be used without changing the data layout.
///
/// todo: rename OSciptXxX sub-resources to OrchestrationXXX sub-resources
///
class Orchestration
{
    friend class OScriptGraph;

protected:
    OrchestrationType _type;                               //! The orchestration type
    bool _initialized{ false };                            //! Whether the orchestration is initialized
    bool _edited{ false };                                 //! Tracks whether the orchestration has been edited
    StringName _base_type;                                 //! The base type of the orchestration
    RBSet<OScriptConnection> _connections;                 //! The connections between nodes in the orchestration
    HashMap<int, Ref<OScriptNode>> _nodes;                 //! Map of all nodes within this orchestration
    HashMap<StringName, Ref<OScriptFunction>> _functions;  //! Map of all orchestration functions
    HashMap<StringName, Ref<OScriptVariable>> _variables;  //! Map of all orchestration variables
    HashMap<StringName, Ref<OScriptSignal>> _signals;      //! Map of all user-defined signals
    HashMap<StringName, Ref<OScriptGraph>> _graphs;        //! Map of all defined graphs
    Resource* _self;                                       //! Reference to the outer resource type
    String _brief_description;                             //! The brief description
    String _description;                                   //! The description

    //~ Begin Serialization Interface
    TypedArray<OScriptNode> _get_nodes_internal() const;
    void _set_nodes_internal(const TypedArray<OScriptNode>& p_nodes);
    TypedArray<int> _get_connections_internal() const;
    void _set_connections_internal(const TypedArray<int>& p_connections);
    TypedArray<OScriptGraph> _get_graphs_internal() const;
    void _set_graphs_internal(const TypedArray<OScriptGraph>& p_graphs);
    TypedArray<OScriptFunction> _get_functions_internal() const;
    void _set_functions_internal(const TypedArray<OScriptFunction>& p_functions);
    TypedArray<OScriptVariable> _get_variables_internal() const;
    void _set_variables_internal(const TypedArray<OScriptVariable>& p_variables);
    TypedArray<OScriptSignal> _get_signals_internal() const;
    void _set_signals_internal(const TypedArray<OScriptSignal>& p_signals);
    //~ End Serialization Interface

    /// Fixes any potential orphan nodes in the script
    /// @note This should generally not be an issue, except during development, but its a great sanity check
    virtual void _fix_orphans();

    /// Get whether there are any instances of this orchestration
    /// @return true if there are existing instances, false otherwise
    virtual bool _has_instances() const { return false; }

    /// Update the placeholders
    virtual void _update_placeholders() { }

    //~ Begin Internal Connection API
    void _connect_nodes(int p_source_id, int p_source_port, int p_target_id, int p_target_port);
    void _disconnect_nodes(int p_source_id, int p_source_port, int p_target_id, int p_target_port);
    //~ End Internal Connection API

    /// Get all unique node ids for a specific node type
    /// @tparam T the node type
    /// @return a list of node unique IDs
    template <typename T>
    List<int> _get_node_type_node_ids()
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
    /// Get the orchestration type
    /// @return the orchestration type
    OrchestrationType get_type() const { return _type; }

    /// Get the base type of the orchestration
    /// @return base class type of the orchestration
    StringName get_base_type() const;

    /// Set the base class type
    /// @param p_base_type the base type class name
    void set_base_type(const StringName& p_base_type);

    /// Get whether the orchestration runs in tool-mode
    /// @return true if the orchestration runs in tool-mode in the editor, false otherwise
    virtual bool get_tool() const { return false; }

    /// Set whether the orchestration runs in tool-mode
    /// @param p_tool true to run in the editor in tool-mode, false to run only at run-time
    virtual void set_tool(bool p_tool) { }

    /// Get a pointer to the underlying owning resource of the orchestration
    /// @return the owning resource, use with caution
    virtual Ref<Resource> get_self() const { return _self; }

    /// Get a pointer to this orchestration
    /// @return the orchestration
    virtual Orchestration* get_orchestration() { return this; }

    /// Get the next available node unique ID
    /// @return the next node unique ID
    int get_available_id() const;

    /// Check whether the orchestration is edited
    /// @return true if the orchestration was edited, false otherwise
    bool is_edited() const { return _edited; }

    /// Sets the orchestration as edited
    /// @param p_edited true if the orchestration was edited, false otherwise
    void set_edited(bool p_edited);

    /// Performs post initialization/load steps
    virtual void post_initialize();

    /// Validtes and the builds the orchestration
    /// @param p_log the build log
    virtual void validate_and_build(BuildLog& p_log);

    /// Get the brief description for the orchestration
    /// @return the brief description
    virtual String get_brief_description() const { return _brief_description; }

    /// Set the brief description
    /// @param p_description the brief description
    virtual void set_brief_description(const String& p_description);

    /// Get the description for the orchestration
    /// @return the description
    virtual String get_description() const { return _description; }

    /// Set the description for the orchestration
    /// @param p_description the description
    virtual void set_description(const String& p_description);

    //~ Begin Node Interface
    void add_node(const Ref<OScriptGraph>& p_graph, const Ref<OScriptNode>& p_node);
    void remove_node(int p_node_id);
    /// @deprecated use OScriptGraph::has_node
    Ref<OScriptNode> get_node(int p_node_id) const;
    Vector<Ref<OScriptNode>> get_nodes() const;
    //~ End Node Interface

    //~ Begin Connection Interface
    const RBSet<OScriptConnection>& get_connections() const;
    /// @deprecated use OScriptGraph::unlink
    /// @note this method was left because OScriptNodePin needs this due to an order of operations issue
    void disconnect_nodes(int p_source_id, int p_source_port, int p_target_id, int p_target_port);
    Vector<Ref<OScriptNodePin>> get_connections(const OScriptNodePin* p_pin) const;
    void adjust_connections(const OScriptNode* p_node, int p_offset, int p_adjustment, EPinDirection p_dir = PD_MAX);
    //~ End Connection Interface

    //~ Begin Graph Interface
    bool has_graph(const StringName& p_name) const;
    Ref<OScriptGraph> create_graph(const StringName& p_name, int p_flags = 0);
    void remove_graph(const StringName& p_name);
    Ref<OScriptGraph> get_graph(const StringName& p_name) const;
    Ref<OScriptGraph> find_graph(const StringName& p_name) const;
    Ref<OScriptGraph> find_graph(const Ref<OScriptNode>& p_node);
    void rename_graph(const StringName& p_old_name, const StringName& p_new_name);
    Vector<Ref<OScriptGraph>> get_graphs() const;
    //~ End Graph Interface

    //~ Begin Function API
    bool has_function(const StringName& p_name) const;
    Ref<OScriptFunction> create_function(const MethodInfo& p_method, int p_node_id, bool p_user_defined = false);
    void remove_function(const StringName& p_name);
    Ref<OScriptFunction> find_function(const StringName& p_name) const;
    Ref<OScriptFunction> find_function(const Guid& p_guid) const;
    void rename_function(const StringName& p_old_name, const StringName& p_new_name);
    PackedStringArray get_function_names() const;
    int get_function_node_id(const StringName& p_name) const;
    Vector<Ref<OScriptFunction>> get_functions() const;
    //~ End Function Interface

    //~ Begin Variable Interface
    bool has_variable(const StringName& p_name) const;
    Ref<OScriptVariable> create_variable(const StringName& p_name, Variant::Type p_type = Variant::NIL);
    void remove_variable(const StringName& p_name);
    Ref<OScriptVariable> get_variable(const StringName& p_name);
    void rename_variable(const StringName& p_old_name, const StringName& p_new_name);
    Vector<Ref<OScriptVariable>> get_variables() const;
    PackedStringArray get_variable_names() const;
    bool can_remove_variable(const StringName& p_name) const;
    //~ End Variable Interface

    //~ Begin Signals Interface
    bool has_custom_signal(const StringName& p_name) const;
    Ref<OScriptSignal> create_custom_signal(const StringName& p_name);
    void remove_custom_signal(const StringName& p_name);
    Ref<OScriptSignal> get_custom_signal(const StringName& p_name);
    Ref<OScriptSignal> find_custom_signal(const StringName& p_name) const;
    void rename_custom_user_signal(const StringName& p_old_name, const StringName& p_new_name);
    Vector<Ref<OScriptSignal>> get_custom_signals() const;
    PackedStringArray get_custom_signal_names() const;
    bool can_remove_custom_signal(const StringName& p_name) const;
    //~ End Signals Interface

    /// Constructs the orchestration for the specified object
    /// @param p_self the owner object
    /// @param p_type the orchestration type
    explicit Orchestration(Resource* p_self, OrchestrationType p_type);

    /// Destructor
    virtual ~Orchestration() = default;
};

#endif  // ORCHESTRATOR_ORCHESTRATION_H
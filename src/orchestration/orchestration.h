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
#ifndef ORCHESTRATOR_ORCHESTRATION_NEW_H
#define ORCHESTRATOR_ORCHESTRATION_NEW_H

#include "script/connection.h"
#include "script/function.h"
#include "script/graph.h"
#include "script/node.h"
#include "script/signals.h"
#include "script/variable.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/rb_set.hpp>

using namespace godot;

/// Forward declarations
class OrchestrationTextParser;
class OrchestrationTextSerializer;
class OScript;

/// An abstract base class for future expansion
class AbstractOrchestration : public Resource
{
    // todo: consider using RefCounted rather than Resource
    GDCLASS(AbstractOrchestration, Resource);

protected:
    static void _bind_methods() {}

    ~AbstractOrchestration() override = default;
};

/// Defines an <code>Orchestration</code> resource.
class Orchestration : public AbstractOrchestration
{
    friend class OScriptGraph;
    friend class OrchestrationTextSerializer;
    friend class OrchestrationTextParser;

    GDCLASS(Orchestration, AbstractOrchestration);

    StringName _base_type;
    bool _tool{ false };
    bool _edited{ false };
    bool _initialized{ false };
    uint32_t _version{ 0 };

    HashMap<int, Ref<OScriptNode>> _nodes;
    HashMap<StringName, Ref<OScriptGraph>> _graphs;
    HashMap<StringName, Ref<OScriptFunction>> _functions;
    HashMap<StringName, Ref<OScriptVariable>> _variables;
    HashMap<StringName, Ref<OScriptSignal>> _signals;
    RBSet<OScriptConnection> _connections;
    OScript* _script; // cannot be a reference

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

    //~ Begin Internal Connection API
    void _connect_nodes(int p_source_id, int p_source_port, int p_target_id, int p_target_port);
    void _disconnect_nodes(int p_source_id, int p_source_port, int p_target_id, int p_target_port);
    //~ End Internal Connection API

    void _fix_orphans();

protected:
    static void _bind_methods();

public:

    int get_available_id() const;

    Ref<OScript> get_self();
    void set_self(const Ref<OScript>& p_script);

    bool is_edited() const { return _edited; }
    void set_edited(bool p_edited);

    StringName get_base_type() const;
    void set_base_type(const StringName& p_base_type);

    bool get_tool() const;
    void set_tool(bool p_tool);

    //~ Begin Graphs Interface
    TypedArray<OScriptGraph> get_graphs_serialized() const;
    void set_graphs_serialized(const TypedArray<OScriptGraph>& p_nodes);
    bool has_graph(const StringName& p_name) const;
    Ref<OScriptGraph> create_graph(const StringName& p_name, int p_flags = 0);
    void remove_graph(const StringName& p_name);
    Ref<OScriptGraph> get_graph(const StringName& p_name) const;
    Ref<OScriptGraph> find_graph(const StringName& p_name) const;
    Ref<OScriptGraph> find_graph(const Ref<OScriptNode>& p_node);
    bool rename_graph(const StringName& p_old_name, const StringName& p_new_name);
    Vector<Ref<OScriptGraph>> get_graphs() const;
    //~ End Graphs Interface

    //~ Begin Nodes Interface
    TypedArray<OScriptNode> get_nodes_serialized() const;
    void set_nodes_serialized(const TypedArray<OScriptNode>& p_nodes);
    void add_node(const Ref<OScriptGraph>& p_graph, const Ref<OScriptNode>& p_node);
    void remove_node(int p_node_id);
    /// @deprecated use OScriptGraph::has_node
    Ref<OScriptNode> get_node(int p_node_id) const;
    Vector<Ref<OScriptNode>> get_nodes() const;
    //~ End Nodes Interface

    //~ Begin Connections Interface
    TypedArray<int> get_connections_serialized() const;
    void set_connections_serialized(const TypedArray<int>& p_connections);
    const RBSet<OScriptConnection>& get_connections() const;
    /// @deprecated use OScriptGraph::unlink
    /// @note this method was left because OScriptNodePin needs this due to an order of operations issue
    void disconnect_nodes(int p_source_id, int p_source_port, int p_target_id, int p_target_port);
    Vector<Ref<OScriptNodePin>> get_connections(const OScriptNodePin* p_pin) const;
    void adjust_connections(const OScriptNode* p_node, int p_offset, int p_adjustment, EPinDirection p_dir = PD_MAX);
    //~ End Connection Interface

    //~ Begin Functions Interface
    TypedArray<OScriptFunction> get_functions_serialized() const;
    void set_functions_serialized(const TypedArray<OScriptFunction>& p_functions);
    bool has_function(const StringName& p_name) const;
    Ref<OScriptFunction> create_function(const MethodInfo& p_method, int p_node_id, bool p_user_defined = false);
    Ref<OScriptFunction> duplicate_function(const StringName& p_name, bool p_include_code);
    void remove_function(const StringName& p_name);
    Ref<OScriptFunction> find_function(const StringName& p_name) const;
    Ref<OScriptFunction> find_function(const Guid& p_guid) const;
    bool rename_function(const StringName& p_old_name, const StringName& p_new_name);
    PackedStringArray get_function_names() const;
    int get_function_node_id(const StringName& p_name) const;
    Vector<Ref<OScriptFunction>> get_functions() const;
    //~ End Functions Interface

    //~ Begin Variables Interface
    TypedArray<OScriptVariable> get_variables_serialized() const;
    void set_variables_serialized(const TypedArray<OScriptVariable>& p_variables);
    bool has_variable(const StringName& p_name) const;
    Ref<OScriptVariable> create_variable(const StringName& p_name, Variant::Type p_type = Variant::NIL);
    Ref<OScriptVariable> duplicate_variable(const StringName& p_name);
    void remove_variable(const StringName& p_name);
    Ref<OScriptVariable> get_variable(const StringName& p_name);
    bool rename_variable(const StringName& p_old_name, const StringName& p_new_name);
    Vector<Ref<OScriptVariable>> get_variables() const;
    PackedStringArray get_variable_names() const;
    bool can_remove_variable(const StringName& p_name) const;
    Ref<OScriptVariable> promote_to_variable(const Ref<OScriptNodePin>& p_pin);
    //~ End Variables Interface

    //~ Begin Signals Interface
    TypedArray<OScriptSignal> get_signals_serialized() const;
    void set_signals_serialized(const TypedArray<OScriptSignal>& p_signals);
    bool has_custom_signal(const StringName& p_name) const;
    Ref<OScriptSignal> create_custom_signal(const StringName& p_name);
    void remove_custom_signal(const StringName& p_name);
    Ref<OScriptSignal> get_custom_signal(const StringName& p_name);
    Ref<OScriptSignal> find_custom_signal(const StringName& p_name) const;
    bool rename_custom_user_signal(const StringName& p_old_name, const StringName& p_new_name);
    Vector<Ref<OScriptSignal>> get_custom_signals() const;
    PackedStringArray get_custom_signal_names() const;
    bool can_remove_custom_signal(const StringName& p_name) const;
    //~ End Signals Interface

    void post_initialize();
    void validate_and_build(BuildLog& p_log);

    Orchestration();
    ~Orchestration() override;
};

#endif // ORCHESTRATOR_ORCHESTRATION_NEW_H
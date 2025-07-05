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
#include "orchestration/orchestration.h"

#include "common/method_utils.h"
#include "common/name_utils.h"
#include "common/variant_utils.h"
#include "script/node.h"
#include "script/nodes/functions/call_script_function.h"
#include "script/nodes/functions/function_entry.h"
#include "script/nodes/functions/function_result.h"
#include "script/nodes/signals/emit_member_signal.h"
#include "script/nodes/signals/emit_signal.h"
#include "script/nodes/variables/variable.h"
#include "script/variable.h"

#include <godot_cpp/classes/os.hpp>

TypedArray<OScriptNode> Orchestration::_get_nodes_internal() const
{
    Array r_out;
    for (const KeyValue<int, Ref<OScriptNode>>& E : _nodes)
        r_out.push_back(E.value);

    return r_out;
}

void Orchestration::_set_nodes_internal(const TypedArray<OScriptNode>& p_nodes)
{
    _nodes.clear();
    for (int i = 0; i < p_nodes.size(); i++)
    {
        Ref<OScriptNode> node = p_nodes[i];
        node->_orchestration = this;
        _nodes[node->get_id()] = node;
    }
}

TypedArray<int> Orchestration::_get_connections_internal() const
{
    Array connections;
    for (const OScriptConnection& E : _connections)
    {
        connections.push_back(E.from_node);
        connections.push_back(E.from_port);
        connections.push_back(E.to_node);
        connections.push_back(E.to_port);
    }
    return connections;
}

void Orchestration::_set_connections_internal(const TypedArray<int>& p_connections)
{
    _connections.clear();
    for (int i = 0; i < p_connections.size(); i += 4)
    {
        OScriptConnection connection;
        connection.from_node = p_connections[i];
        connection.from_port = p_connections[i + 1];
        connection.to_node = p_connections[i + 2];
        connection.to_port = p_connections[i + 3];

        _connections.insert(connection);
    }
}

TypedArray<OScriptGraph> Orchestration::_get_graphs_internal() const
{
    TypedArray<OScriptGraph> graphs;
    for (const KeyValue<StringName, Ref<OScriptGraph>>& E : _graphs)
        graphs.push_back(E.value);
    return graphs;
}

void Orchestration::_set_graphs_internal(const TypedArray<OScriptGraph>& p_graphs)
{
    for (int i = 0; i < p_graphs.size(); i++)
    {
        Ref<OScriptGraph> graph = p_graphs[i];
        graph->_orchestration = this;
        _graphs[graph->get_graph_name()] = graph;
    }
}

TypedArray<OScriptFunction> Orchestration::_get_functions_internal() const
{
    TypedArray<OScriptFunction> functions;
    for (const KeyValue<StringName, Ref<OScriptFunction>>& E : _functions)
        functions.push_back(E.value);
    return functions;
}

void Orchestration::_set_functions_internal(const TypedArray<OScriptFunction>& p_functions)
{
    _functions.clear();
    for (int i = 0; i < p_functions.size(); i++)
    {
        Ref<OScriptFunction> function = p_functions[i];
        function->_orchestration = this;
        _functions[function->get_function_name()] = function;
    }
}

TypedArray<OScriptVariable> Orchestration::_get_variables_internal() const
{
    TypedArray<OScriptVariable> variables;
    for (const KeyValue<StringName, Ref<OScriptVariable>>& E : _variables)
        variables.push_back(E.value);
    return variables;
}

void Orchestration::_set_variables_internal(const TypedArray<OScriptVariable>& p_variables)
{
    _variables.clear();
    for (int i = 0; i < p_variables.size(); i++)
    {
        Ref<OScriptVariable> variable = p_variables[i];
        variable->_orchestration = this;
        _variables[variable->get_variable_name()] = variable;
    }
}

TypedArray<OScriptSignal> Orchestration::_get_signals_internal() const
{
    TypedArray<OScriptSignal> signals;
    for (const KeyValue<StringName, Ref<OScriptSignal>>& E : _signals)
        signals.push_back(E.value);
    return signals;
}

void Orchestration::_set_signals_internal(const TypedArray<OScriptSignal>& p_signals)
{
    _signals.clear();
    for (int i = 0; i < p_signals.size(); i++)
    {
        Ref<OScriptSignal> signal = p_signals[i];
        signal->_orchestration = this;
        _signals[signal->get_signal_name()] = signal;
    }
}

void Orchestration::_fix_orphans()
{
    // Iterate nodes and check orphan status
    for (const KeyValue<int, Ref<OScriptNode>>& E : _nodes)
    {
        bool orphaned = true;
        for (const KeyValue<StringName, Ref<OScriptGraph>>& G : _graphs)
        {
            if (G.value->has_node(E.key))
            {
                orphaned = false;
                break;
            }
        }

        if (!orphaned)
            continue;

        // If a node is orphaned but a connection exists to re-add it back to the graph, do it
        for (const OScriptConnection& C : _connections)
        {
            if (C.is_linked_to(E.key))
            {
                for (const KeyValue<StringName, Ref<OScriptGraph>>& G : _graphs)
                {
                    if (G.value->has_node(C.to_node) || G.value->has_node(C.from_node))
                    {
                        WARN_PRINT("Adding orphaned node " + itos(E.key) + " back to graph " + G.value->get_graph_name());
                        G.value->add_node(E.value);
                        orphaned = false;
                        break;
                    }
                }

                if (!orphaned)
                    break;
            }
        }

        if (!orphaned)
            continue;

        WARN_PRINT(vformat("Removed orphan node %d (%s) from script %s.", E.key, E.value->get_class(), get_self()->get_path()));
        _nodes.erase(E.key);
    }

    {
        RBSet<OScriptConnection> removals;
        for (const OScriptConnection& C : _connections)
        {
            if (!_nodes.has(C.from_node) || !_nodes.has(C.to_node))
                removals.insert(C);
        }
        for (const OScriptConnection C : removals)
        {
            String extra = "";
            if (OS::get_singleton()->has_feature("editor"))
                extra += " Please save orchestration '" + get_self()->get_path() + "' to apply changes.";

            WARN_PRINT(vformat("Removing orphan connection for " + C.to_string() + ", either the source or target node no longer exists." + extra));

            _connections.erase(C);
        }
    }
}

void Orchestration::_connect_nodes(int p_source_id, int p_source_port, int p_target_id, int p_target_port)
{
    ERR_FAIL_COND_MSG(_has_instances(), "Cannot connect nodes, instances exist.");

    OScriptConnection connection;
    connection.from_node = p_source_id;
    connection.from_port = p_source_port;
    connection.to_node = p_target_id;
    connection.to_port = p_target_port;

    ERR_FAIL_COND_MSG(_connections.has(connection), "A connection already exists: " + connection.to_string());
    _connections.insert(connection);

    _self->emit_signal("connections_changed", "connect_nodes");
}

void Orchestration::_disconnect_nodes(int p_source_id, int p_source_port, int p_target_id, int p_target_port)
{
    ERR_FAIL_COND_MSG(_has_instances(), "Cannot disconnect nodes, instances exist.");

    OScriptConnection connection;
    connection.from_node = p_source_id;
    connection.from_port = p_source_port;
    connection.to_node = p_target_id;
    connection.to_port = p_target_port;

    ERR_FAIL_COND_MSG(!_connections.has(connection), "Cannot remove non-existant connection: " + connection.to_string());
    _connections.erase(connection);

    // Clean-up graph knots for the connection
    for (const KeyValue<StringName, Ref<OScriptGraph>>& E : _graphs)
    {
        if (E.value->has_node(p_source_id) || E.value->has_node(p_target_id))
            E.value->remove_connection_knot(connection.id);
    }

    _self->emit_signal("connections_changed", "disconnect_nodes");
}

StringName Orchestration::get_base_type() const
{
    return _base_type;
}

void Orchestration::set_base_type(const StringName& p_base_type)
{
    if (!_base_type.match(p_base_type))
    {
        _base_type = p_base_type;
        _self->emit_changed();
    }
}

int Orchestration::get_available_id() const
{
    // We should eventually consider a better strategy for node unique ids to deal with
    // scripts that are constantly modified with new nodes added and removed.

    int max = -1;
    for (const KeyValue<int, Ref<OScriptNode>>& E : _nodes)
        max = Math::max(E.key, max);

    return (max + 1);
}

void Orchestration::set_edited(bool p_edited)
{
    if (_edited != p_edited)
    {
        _edited = p_edited;
        if (_edited)
            _self->emit_changed();
    }
}

void Orchestration::post_initialize()
{
    // Initialize variables
    for (const KeyValue<StringName, Ref<OScriptVariable>>& E : _variables)
        E.value->post_initialize();

    // Initialize nodes
    for (const KeyValue<int, Ref<OScriptNode>>& E : _nodes)
        E.value->post_initialize();

    // Initialize graphs
    for (const KeyValue<StringName, Ref<OScriptGraph>>& G : _graphs)
        G.value->post_initialize();

    _fix_orphans();

    // Check if upgrades are required
    if (_version < OScriptResourceFormatInstance::FORMAT_VERSION)
    {
        // Upgrade nodes that require it
        for (const KeyValue<int, Ref<OScriptNode>>& E : _nodes)
            E.value->_upgrade(_version, OScriptResourceFormatInstance::FORMAT_VERSION);

        _version = OScriptResourceFormatInstance::FORMAT_VERSION;
    }

    _initialized = true;
}

void Orchestration::validate_and_build(BuildLog& p_log)
{
    // Sanity check
    _fix_orphans();

    for (const KeyValue<int, Ref<OScriptNode>>& E : _nodes)
        E.value->validate_node_during_build(p_log);
}

void Orchestration::add_node(const Ref<OScriptGraph>& p_graph, const Ref<OScriptNode>& p_node)
{
    ERR_FAIL_COND_MSG(_has_instances(), "Cannot add node, instances exist.");
    ERR_FAIL_COND(p_node.is_null());
    ERR_FAIL_COND(_nodes.has(p_node->get_id()));

    // Validate the node details
    p_node->_orchestration = this;
    p_node->_validate_input_default_values();

    // Register the node with the script
    _nodes[p_node->get_id()] = p_node;

    // Register the node with the graph
    p_graph->add_node(p_node);
}

void Orchestration::remove_node(int p_node_id)
{
    ERR_FAIL_COND_MSG(_has_instances(), "Cannot remove node, instances exist.");
    ERR_FAIL_COND(!_nodes.has(p_node_id));

    const Ref<OScriptNode> node = _nodes[p_node_id];
    node->pre_remove();

    // Check whether the node represents a function and if so, remove the function
    for (const KeyValue<StringName, Ref<OScriptFunction>>& E : _functions)
    {
        if (E.value->get_owning_node_id() == p_node_id)
        {
            _functions.erase(E.key);
            break;
        }
    }

    for (const Ref<OScriptNodePin>& pin : node->get_all_pins())
        pin->unlink_all(true);

    List<OScriptConnection> removals;
    for (const OScriptConnection& connection : _connections)
    {
        if (connection.is_linked_to(p_node_id))
            removals.push_back(connection);
    }

    if (!removals.is_empty())
    {
        ERR_PRINT("Node still has remaining connects, cleaning them up");
        while (!removals.is_empty())
        {
            _connections.erase(removals.front()->get());
            removals.pop_front();
        }

    }

    for (const KeyValue<StringName, Ref<OScriptGraph>>& E : _graphs)
        E.value->remove_node(node);

    _nodes.erase(p_node_id);
}

Ref<OScriptNode> Orchestration::get_node(int p_node_id) const
{
    ERR_FAIL_COND_V_MSG(!_nodes.has(p_node_id), nullptr, "No node exists with the specified ID: " + itos(p_node_id));
    return _nodes[p_node_id];
}

Vector<Ref<OScriptNode>> Orchestration::get_nodes() const
{
    Vector<Ref<OScriptNode>> results;
    for (const KeyValue<int, Ref<OScriptNode>>& E : _nodes)
        results.push_back(E.value);
    return results;
}

const RBSet<OScriptConnection>& Orchestration::get_connections() const
{
    return _connections;
}

void Orchestration::disconnect_nodes(int p_source_id, int p_source_port, int p_target_id, int p_target_port)
{
    _disconnect_nodes(p_source_id, p_source_port, p_target_id, p_target_port);
}

Vector<Ref<OScriptNodePin>> Orchestration::get_connections(const OScriptNodePin* p_pin) const
{
    // todo: consider caching pin connections in each pin for performance reasons
    if (!p_pin || p_pin->is_hidden())
        return {};

    const OScriptNode* node = p_pin->get_owning_node();
    if (!node)
        return {};

    const bool input = p_pin->is_input();
    const int node_id = node->get_id();

    Vector<Ref<OScriptNodePin>> results;
    for (const OScriptConnection& E : _connections)
    {
        if (input && E.to_node == node_id && E.to_port == p_pin->get_pin_index())
        {
            const Ref<OScriptNode> other = get_node(E.from_node);
            if (other.is_valid())
            {
                const Ref<OScriptNodePin> other_pin = other->find_pin(E.from_port, PD_Output);
                if (other_pin.is_valid())
                    results.push_back(other_pin);
            }
        }
        else if (!input && E.from_node == node_id && E.from_port == p_pin->get_pin_index())
        {
            const Ref<OScriptNode> other = get_node(E.to_node);
            if (other.is_valid())
            {
                const Ref<OScriptNodePin> other_pin = other->find_pin(E.to_port, PD_Input);
                if (other_pin.is_valid())
                    results.push_back(other_pin);
            }
        }
    }
    return results;
}

void Orchestration::adjust_connections(const OScriptNode* p_node, int p_offset, int p_adjustment, EPinDirection p_dir)
{
    struct ConnectionData
    {
        OScriptConnection existing;
        OScriptConnection mutated;
    };

    // NOTE:
    // The RBSet maintains search criteria order based on the calculation of Connection::operator< and
    // when we modify the port adjustments here, that invalidates the criteria, which will lead to a
    // situation where the tree's internal state no longer matches the object state, causing functions
    // such as "has" to fail to locate an entry.
    //
    // Now we could simply recreate the _connections RBSet by copying the old into a new instance, but
    // this is highly inefficient when the connection set maintains a number of objects in a large
    // script, so instead we'll cache the data-set specific to the mutation and adjust only those. It
    // should, in theory, be overall less impactful to the data structure in large graphs.
    List<ConnectionData> data;
    for (const OScriptConnection& E : _connections)
    {
        if (p_dir != PD_Output && E.to_node == p_node->get_id() && E.to_port >= p_offset)
        {
            ConnectionData cd;
            cd.existing = E;
            cd.mutated = E;
            cd.mutated.to_port += p_adjustment;
            data.push_back(cd);
        }
        if (p_dir != PD_Input && E.from_node == p_node->get_id() && E.from_port >= p_offset)
        {
            ConnectionData cd;
            cd.existing = E;
            cd.mutated = E;
            cd.mutated.from_port += p_adjustment;
            data.push_back(cd);
        }
    }

    // Now that the data set has been cached, the next phase must be done in 2 steps
    // First remove the old entries from the RBSet
    for (const ConnectionData& cd : data)
        _connections.erase(cd.existing);

    // Next add the new entries to the RBSet
    for (const ConnectionData& cd : data)
        _connections.insert(cd.mutated);

    _self->emit_signal("connections_changed", "adjust_connections");
}

bool Orchestration::has_graph(const StringName& p_name) const
{
    return _graphs.has(p_name);
}

Ref<OScriptGraph> Orchestration::create_graph(const StringName& p_name, int p_flags)
{
    ERR_FAIL_COND_V_MSG(has_graph(p_name), nullptr, "A graph with that name already exists: " + p_name);
    ERR_FAIL_COND_V_MSG(p_name.is_empty(), nullptr, "A name is required to create a graph.");
    ERR_FAIL_COND_V_MSG(!p_name.is_valid_identifier(), nullptr, "The name is not a valid graph name.");

    Ref<OScriptGraph> graph(memnew(OScriptGraph));
    graph->_orchestration = this;
    graph->set_graph_name(p_name);
    graph->set_flags(p_flags);

    _graphs[p_name] = graph;

    // _self->emit_signal("graphs_changed");

    return graph;
}

void Orchestration::remove_graph(const StringName& p_name)
{
    ERR_FAIL_COND_MSG(!has_graph(p_name), "No graph exists with the specified name: " + p_name);

    if (get_type() == OrchestrationType::OT_Script)
    {
        if (p_name.match("EventGraph"))
        {
            ERR_PRINT("The 'EventGraph' graph cannot be removed.");
            return;
        }
    }

    const Ref<OScriptGraph> graph = get_graph(p_name);
    graph->remove_all_nodes();

    // Remove the graph
    _graphs.erase(p_name);
}

Ref<OScriptGraph> Orchestration::get_graph(const StringName& p_name) const
{
    ERR_FAIL_COND_V_MSG(!has_graph(p_name), nullptr, "No graph exists with the specified name: " + p_name);
    return _graphs[p_name];
}

Ref<OScriptGraph> Orchestration::find_graph(const StringName& p_name) const
{
    return has_graph(p_name) ? _graphs[p_name] : nullptr;
}

Ref<OScriptGraph> Orchestration::find_graph(const Ref<OScriptNode>& p_node)
{
    ERR_FAIL_COND_V_MSG(!p_node.is_valid(), nullptr, "Cannot find a graph when the node reference is invalid.");

    for (const KeyValue<StringName, Ref<OScriptGraph>>& E : _graphs)
    {
        if (E.value->has_node(p_node->get_id()))
            return E.value;
    }

    ERR_FAIL_V_MSG(nullptr, "No graph contains the node with the unique ID: " + itos(p_node->get_id()));
}

bool Orchestration::rename_graph(const StringName& p_old_name, const StringName& p_new_name)
{
    ERR_FAIL_COND_V_MSG(!has_graph(p_old_name), false, "No graph exists with the old name: " + p_old_name);
    ERR_FAIL_COND_V_MSG(has_graph(p_new_name), false, "A graph already exists with the new name: " + p_new_name);
    ERR_FAIL_COND_V_MSG(!p_new_name.is_valid_identifier(), false, "The new graph name is not a valid.");

    UtilityFunctions::print("Create Graph with name [", p_old_name, "] to [", p_new_name, "]");

    Ref<OScriptGraph> graph = get_graph(p_old_name);
    if (!graph.is_valid())
        return false;

    graph->set_graph_name(p_new_name);

    _graphs[p_new_name] = graph;
    _graphs.erase(p_old_name);
    return true;
}

Vector<Ref<OScriptGraph>> Orchestration::get_graphs() const
{
    Vector<Ref<OScriptGraph>> results;
    for (const KeyValue<StringName, Ref<OScriptGraph>>& E : _graphs)
        results.push_back(E.value);
    return results;
}

bool Orchestration::has_function(const StringName& p_name) const
{
    return _functions.has(p_name);
}

Ref<OScriptFunction> Orchestration::create_function(const MethodInfo& p_method, int p_node_id, bool p_user_defined)
{
    ERR_FAIL_COND_V_MSG(_has_instances(), nullptr, "Cannot create functions, instances exist.");
    ERR_FAIL_COND_V_MSG(!String(p_method.name).is_valid_identifier(), nullptr, "Invalid function name: " + p_method.name);
    ERR_FAIL_COND_V_MSG(_functions.has(p_method.name), nullptr, "A function already exists with the name: " + p_method.name);
    ERR_FAIL_COND_V_MSG(_variables.has(p_method.name), nullptr, "A variable already exists with the name: " + p_method.name);
    ERR_FAIL_COND_V_MSG(_signals.has(p_method.name), nullptr, "A signal already exists with the name: " + p_method.name);

    Ref<OScriptFunction> function(memnew(OScriptFunction));
    function->_orchestration = this;
    function->_guid = Guid::create_guid();
    function->_method = p_method;
    function->_owning_node_id = p_node_id;
    function->_user_defined = p_user_defined;
    function->_returns_value = MethodUtils::has_return_value(p_method);

    _functions[p_method.name] = function;

    _self->emit_signal("functions_changed");

    return function;
}

Ref<OScriptFunction> Orchestration::duplicate_function(const StringName& p_name, bool p_include_code)
{
    ERR_FAIL_COND_V_MSG(_has_instances(), nullptr, "Cannot duplicate functions, instances exist.");
    ERR_FAIL_COND_V_MSG(!has_function(p_name), nullptr, "No function exists with the name: " + p_name);

    Ref<OScriptGraph> old_graph = find_graph(p_name);
    Ref<OScriptFunction> old_function = find_function(p_name);
    Ref<OScriptFunction> new_function;

    // make a unique name for the new function
    String new_name = NameUtils::create_unique_name(p_name, get_function_names());

    // make a graph
    Ref<OScriptGraph> new_graph = create_graph(new_name, OScriptGraph::GF_FUNCTION | OScriptGraph::GF_DEFAULT);


    // duplicate each node, make a lookup table that maps old node IDs to new node IDs
    HashMap<int,int> node_id_map;
    bool has_result = false;

    // new entry and result nodes (only needed later if we don't include code)
    Ref<OScriptNodeFunctionEntry> new_entry;
    Ref<OScriptNodeFunctionResult> new_result;

    for (const Ref<OScriptNode>& old_node : old_graph->get_nodes())
    {
        // if we don't include the code skip anything that is not a function entry node
        // or a function return node
        Ref<OScriptNodeFunctionEntry> old_entry = old_node;
        Ref<OScriptNodeFunctionResult> old_result = old_node;
        if (!p_include_code && !old_entry.is_valid() && !old_result.is_valid()) {
            // skip this node
            continue;
        }

        // if we don't include code, we only duplicate ONE result node
        if (!p_include_code && old_result.is_valid())
        {
            if (has_result)
            {
                continue; // skip this node, we already have a result node
            }
            has_result = true; // we will only have one result node
        }

        // duplicate the node including its resources
        Ref<OScriptNode> new_node = old_graph->duplicate_node(old_node->get_id(), {}, true);
        // and move it over to the new graph
        old_graph->move_node_to(new_node, new_graph);


        node_id_map[old_node->get_id()] = new_node->get_id();

        // if this node was the function entry node, we need to update function reference
        if (!new_entry.is_valid())
        {
            // this is an implicit cast, if the new_node is not a function entry node,
            // new_entry will remain invalid
            new_entry = new_node;
            if (new_entry.is_valid())
            {
                OScriptNodeInitContext context;
                MethodInfo mi = old_function->get_method_info();
                mi.name = new_name;
                context.method = mi;
                new_entry->initialize(context);
                new_function = new_entry->get_function();
            }
        }

        // take note of the result node (unless we already have one)
        // only needed when we don't include code
        if (!p_include_code && !new_result.is_valid())
        {
            // this is an implicit cast, if the new_node is not a result node,
            // new_result will remain invalid
            new_result = new_node;
        }
    }

    // now restore connections
    if (p_include_code)
    {
        // if we include code, we need to restore all connections
        for (const OScriptConnection& old_connection : old_graph->get_connections())
        {
            int source_id = node_id_map[static_cast<int>(old_connection.from_node)];
            int target_id = node_id_map[static_cast<int>(old_connection.to_node)];
            int source_port = old_connection.from_port;
            int target_port = old_connection.to_port;
            new_graph->link(source_id, source_port, target_id, target_port);
        }
    }
    else
    {
        // otherwise we just connect the entry node to the result node (if we had a result node)
        if (new_entry.is_valid() && new_result.is_valid())
        {
            // get first the output pin of the entry node that is an execution pin
            Ref<OScriptNodePin> entry_execution_pin;
            for (const Ref<OScriptNodePin>& pin : new_entry->find_pins(PD_Output))
            {
                if (pin->is_execution())
                {
                    entry_execution_pin = pin;
                    break;
                }
            }
            Ref<OScriptNodePin> result_execution_pin;
            // get the fist input pin of the result node that is an execution pin
            for (const Ref<OScriptNodePin>& pin : new_result->find_pins(PD_Input))
            {
                if (pin->is_execution())
                {
                    result_execution_pin = pin;
                    break;
                }
            }


            // connect the entry node to the result node
            if (entry_execution_pin.is_valid() && result_execution_pin.is_valid())
            {
                // link the entry execution pin to the result execution pin
                new_graph->link(
                    new_entry->get_id(),
                    entry_execution_pin->get_pin_index(),
                    new_result->get_id(),
                    result_execution_pin->get_pin_index());
            }

            // and move the result node close to the entry node
            // this doesn't work too well on HDPI displays, but it is better than nothing
            new_result->set_position(new_entry->get_position() + Vector2(250, 0));
        }
    }

    return new_function;
}

void Orchestration::remove_function(const StringName& p_name)
{
    ERR_FAIL_COND_MSG(_has_instances(), "Cannot remove functions, instances exist.");
    ERR_FAIL_COND_MSG(!_functions.has(p_name), "Cannot remove function that does not exist with name: " + p_name);
    {
        Ref<OScriptFunction> function = _functions[p_name];
        ERR_FAIL_COND_MSG(!function.is_valid(), "Function found with name '" + p_name + "', but its not valid.");

        // Check if the function has a graph (user-defined functions do)
        if (_graphs.has(p_name))
        {
            Ref<OScriptGraph> graph = _graphs[p_name];
            if (graph->get_flags().has_flag(OScriptGraph::GF_FUNCTION))
                remove_graph(graph->get_graph_name());
        }

        const List<int> node_ids = _get_node_type_node_ids<OScriptNodeCallScriptFunction>();
        for (int node_id : node_ids)
        {
            const Ref<OScriptNodeCallScriptFunction> call_function = get_node(node_id);
            const Ref<OScriptFunction> called_function = call_function->get_function();
            if (call_function.is_valid() && called_function->get_function_name().match(function->get_function_name()))
                remove_node(node_id);
        }

        // Find the node for this function and remove it
        if (_nodes.has(function->get_owning_node_id()))
            remove_node(function->get_owning_node_id());

        // Let the editor handle node removal
        _functions.erase(p_name);

        _self->emit_signal("functions_changed");
    }
    _self->emit_changed();
}

Ref<OScriptFunction> Orchestration::find_function(const StringName& p_name) const
{
    for (const KeyValue<StringName, Ref<OScriptFunction>>& E : _functions)
    {
        if (E.value->get_function_name().match(p_name))
            return E.value;
    }
    return nullptr;
}

Ref<OScriptFunction> Orchestration::find_function(const Guid& p_guid) const
{
    for (const KeyValue<StringName, Ref<OScriptFunction>>& E : _functions)
    {
        if (E.value->get_guid() == p_guid)
            return E.value;
    }
    return nullptr;
}

bool Orchestration::rename_function(const StringName& p_old_name, const StringName& p_new_name)
{
    // Ignore if the old/new names are the same
    if (p_old_name == p_new_name)
        return false;

    ERR_FAIL_COND_V_MSG(_has_instances(), false, "Cannot rename function, instances exist.");
    ERR_FAIL_COND_V_MSG(!has_function(p_old_name), false, "Cannot rename, no function found with old name: " + p_old_name);
    ERR_FAIL_COND_V_MSG(has_function(p_new_name), false, "Cannot rename, a function already exists with new name: " + p_new_name);
    ERR_FAIL_COND_V_MSG(!String(p_new_name).is_valid_identifier(), false, "New function name is invalid: " + p_new_name);
    ERR_FAIL_COND_V_MSG(has_variable(p_new_name), false, "Cannot rename function, a variable with name already exists: " + p_new_name);
    ERR_FAIL_COND_V_MSG(has_custom_signal(p_new_name), false, "Cannot rename function, a signal with the name already exists: " + p_new_name);

    const Ref<OScriptFunction> function = _functions[p_old_name];
    if (!function.is_valid() || !function->can_be_renamed())
        return false;

    // Rename function graph, if found
    const Ref<OScriptGraph> function_graph = find_graph(p_old_name);
    if (function_graph.is_valid() && function_graph->get_flags().has_flag(OScriptGraph::GraphFlags::GF_FUNCTION))
    {
        if (!rename_graph(p_old_name, p_new_name))
            return false;
    }

    function->rename(p_new_name);

    _functions.erase(p_old_name);
    _functions[p_new_name] = function;

    _self->emit_signal("functions_changed");
    return true;
}

PackedStringArray Orchestration::get_function_names() const
{
    PackedStringArray names;
    for (const KeyValue<StringName, Ref<OScriptFunction>>& E : _functions)
        names.push_back(E.key);
    return names;
}

int Orchestration::get_function_node_id(const StringName& p_name) const
{
    ERR_FAIL_COND_V(!_functions.has(p_name), -1);
    return _functions[p_name]->get_owning_node_id();
}

Vector<Ref<OScriptFunction>> Orchestration::get_functions() const
{
    Vector<Ref<OScriptFunction>> results;
    for (const KeyValue<StringName, Ref<OScriptFunction>>& E : _functions)
        results.push_back(E.value);
    return results;
}

bool Orchestration::has_variable(const StringName& p_name) const
{
    return _variables.has(p_name);
}

Ref<OScriptVariable> Orchestration::create_variable(const StringName& p_name, Variant::Type p_type)
{
    ERR_FAIL_COND_V_MSG(_has_instances(), nullptr, "Cannot create variables, instances exist.");
    ERR_FAIL_COND_V_MSG(!String(p_name).is_valid_identifier(), nullptr, "Cannot create variable, invalid name: " + p_name);
    ERR_FAIL_COND_V_MSG(has_variable(p_name), nullptr, "A variable with that name already exists: " + p_name);

    PropertyInfo property;
    property.name = p_name;
    property.type = p_type;

    Ref<OScriptVariable> variable(memnew(OScriptVariable));
    variable->_orchestration = this;
    variable->_info = property;
    variable->_default_value = VariantUtils::make_default(property.type);
    variable->_category = "Default";
    variable->_classification = "type:" + Variant::get_type_name(property.type);
    variable->_info.type = property.type;
    variable->_info.hint = PROPERTY_HINT_NONE;
    variable->_info.hint_string = "";
    variable->_info.class_name = "";
    variable->_info.usage = PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_NIL_IS_VARIANT;
    _variables[p_name] = variable;

    #ifdef TOOLS_ENABLED
    _update_placeholders();
    #endif

    _self->emit_signal("variables_changed");

    return variable;
}

Ref<OScriptVariable> Orchestration::duplicate_variable(const StringName& p_name)
{
    ERR_FAIL_COND_V_MSG(_has_instances(), {}, "Cannot duplicate variables, instances exist.");
    ERR_FAIL_COND_V_MSG(!has_variable(p_name), {}, "Cannot duplicate variable that does not exist: " + p_name);

    Ref<OScriptVariable> old_variable = get_variable(p_name);

    String new_name = NameUtils::create_unique_name(p_name, get_variable_names());

    Ref<OScriptVariable> new_variable = create_variable(new_name, old_variable->get_variable_type());
    ERR_FAIL_COND_V_MSG(!new_variable.is_valid(), {}, "Failed to create a new variable with name: " + new_name);
    new_variable->copy_persistent_state(old_variable);

    return new_variable;
}

void Orchestration::remove_variable(const StringName& p_name)
{
    ERR_FAIL_COND_MSG(!has_variable(p_name), "Cannot remove a variable that does not exist: " + p_name);

    const List<int> node_ids = _get_node_type_node_ids<OScriptNodeVariable>();
    for (int node_id : node_ids)
    {
        const Ref<OScriptNodeVariable> variable = get_node(node_id);
        if (variable.is_valid() && variable->get_variable()->get_variable_name().match(p_name))
            remove_node(node_id);
    }

    _variables.erase(p_name);

    _self->emit_signal("variables_changed");
    _self->emit_changed();
    _self->notify_property_list_changed();

    #ifdef TOOLS_ENABLED
    _update_placeholders();
    #endif
}

Ref<OScriptVariable> Orchestration::get_variable(const StringName& p_name)
{
    return has_variable(p_name) ? _variables[p_name] : nullptr;
}

bool Orchestration::rename_variable(const StringName& p_old_name, const StringName& p_new_name)
{
    if (p_old_name == p_new_name)
        return false;

    ERR_FAIL_COND_V_MSG(_has_instances(), false, "Cannot rename variable, instances exist.");
    ERR_FAIL_COND_V_MSG(!has_variable(p_old_name), false, "Cannot rename, no variable exists with the old name: " + p_old_name);
    ERR_FAIL_COND_V_MSG(has_variable(p_new_name), false, "Cannot rename, a variable already exists with the new name: " + p_new_name);
    ERR_FAIL_COND_V_MSG(!String(p_new_name).is_valid_identifier(), false, "Cannot rename, variable name is not valid: " + p_new_name);

    const Ref<OScriptVariable> variable = _variables[p_old_name];
    variable->set_variable_name(p_new_name);

    _variables[p_new_name] = variable;
    _variables.erase(p_old_name);

    _self->emit_signal("variables_changed");
    _self->emit_changed();
    _self->notify_property_list_changed();

    #ifdef TOOLS_ENABLED
    _update_placeholders();
    #endif

    return true;
}

Vector<Ref<OScriptVariable>> Orchestration::get_variables() const
{
    Vector<Ref<OScriptVariable>> results;
    for (const KeyValue<StringName, Ref<OScriptVariable>>& E : _variables)
        results.push_back(E.value);
    return results;
}

PackedStringArray Orchestration::get_variable_names() const
{
    PackedStringArray names;
    for (const KeyValue<StringName, Ref<OScriptVariable>>& E : _variables)
        names.push_back(E.key);
    return names;
}

bool Orchestration::can_remove_variable(const StringName& p_name) const
{
    for (const KeyValue<int, Ref<OScriptNode>>& E : _nodes)
    {
        Ref<OScriptNodeVariable> variable = E.value;
        if (variable.is_valid() && variable->get_variable()->get_variable_name().match(p_name))
            return false;
    }
    return true;
}

Ref<OScriptVariable> Orchestration::promote_to_variable(const Ref<OScriptNodePin>& p_pin)
{
    int index = 0;
    String name = vformat("%s_%d", p_pin->get_pin_name(), index++);
    while (has_variable(name))
        name = vformat("%s_%d", p_pin->get_pin_name(), index++);

    Ref<OScriptVariable> variable = create_variable(name);
    if (variable.is_valid())
    {
        ClassificationParser parser;
        if (parser.parse(p_pin->get_property_info()))
            variable->set_classification(parser.get_classification());

        variable->set_default_value(p_pin->get_effective_default_value());

        variable->emit_changed();
        variable->notify_property_list_changed();

        _self->emit_signal("variables_changed");
    }

    return variable;
}

bool Orchestration::has_custom_signal(const StringName& p_name) const
{
    return _signals.has(p_name);
}

Ref<OScriptSignal> Orchestration::create_custom_signal(const StringName& p_name)
{
    ERR_FAIL_COND_V_MSG(has_custom_signal(p_name), nullptr, "A custom signal already exists with the name: " + p_name);
    ERR_FAIL_COND_V_MSG(!p_name.is_valid_identifier(), nullptr, "The name is not a valid signal name.");

    MethodInfo method;
    method.name = p_name;
    // Fixed by https://github.com/godotengine/godot-cpp/pull/1440
    method.return_val.usage = PROPERTY_USAGE_DEFAULT;

    Ref<OScriptSignal> signal(memnew(OScriptSignal));
    signal->_orchestration = this;
    signal->_method = method;

    _signals[p_name] = signal;

    _self->emit_signal("signals_changed");

    return signal;
}

void Orchestration::remove_custom_signal(const StringName& p_name)
{
    ERR_FAIL_COND_MSG(!has_custom_signal(p_name), "No signal exists with the name: " + p_name);

    const List<int> node_ids = _get_node_type_node_ids<OScriptNodeEmitSignal>();
    for (int node_id : node_ids)
    {
        const Ref<OScriptNodeEmitSignal> signal = get_node(node_id);
        if (signal.is_valid() && signal->get_signal()->get_signal_name().match(p_name))
            remove_node(node_id);
    }

    _signals.erase(p_name);

    _self->emit_signal("signals_changed");
}

Ref<OScriptSignal> Orchestration::get_custom_signal(const StringName& p_name)
{
    ERR_FAIL_COND_V_MSG(!has_custom_signal(p_name), nullptr, "No custom signal exists with name " + p_name);
    return _signals[p_name];
}

Ref<OScriptSignal> Orchestration::find_custom_signal(const StringName& p_name) const
{
    return has_custom_signal(p_name) ? _signals[p_name] : nullptr;
}

bool Orchestration::rename_custom_user_signal(const StringName& p_old_name, const StringName& p_new_name)
{
    if (p_old_name == p_new_name)
        return false;

    ERR_FAIL_COND_V_MSG(_has_instances(), false, "Cannot rename custom signal, instances exist.");
    ERR_FAIL_COND_V_MSG(!has_custom_signal(p_old_name), false, "No custom signal exists with the old name: " + p_old_name);
    ERR_FAIL_COND_V_MSG(has_custom_signal(p_new_name), false, "A custom signal already exists with the new name: " + p_new_name);
    ERR_FAIL_COND_V_MSG(!String(p_new_name).is_valid_identifier(), false, "The custom signal name is invalid: " + p_new_name);

    const Ref<OScriptSignal> signal = find_custom_signal(p_old_name);
    signal->rename(p_new_name);

    _signals[p_new_name] = signal;
    _signals.erase(p_old_name);

    _self->emit_signal("signals_changed");
    _self->emit_changed();
    _self->notify_property_list_changed();

    #ifdef TOOLS_ENABLED
    _update_placeholders();
    #endif

    return true;
}

Vector<Ref<OScriptSignal>> Orchestration::get_custom_signals() const
{
    Vector<Ref<OScriptSignal>> results;
    for (const KeyValue<StringName, Ref<OScriptSignal>>& E : _signals)
        results.push_back(E.value);
    return results;
}

PackedStringArray Orchestration::get_custom_signal_names() const
{
    PackedStringArray names;
    for (const KeyValue<StringName, Ref<OScriptSignal>>& E : _signals)
        names.push_back(E.key);
    return names;
}

bool Orchestration::can_remove_custom_signal(const StringName& p_name) const
{
    for (const KeyValue<int, Ref<OScriptNode>>& E : _nodes)
    {
        const Ref<OScriptNodeEmitSignal> emit_signal_node = E.value;
        if (emit_signal_node.is_valid() && emit_signal_node->get_signal()->get_signal_name().match(p_name))
            return false;
    }
    return true;
}

Orchestration::Orchestration(Resource* p_self, OrchestrationType p_type)
    : _type(p_type)
    , _base_type("Object")
    , _self(p_self)
{
}

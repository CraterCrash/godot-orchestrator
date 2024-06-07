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
#include "orchestration/orchestration.h"

#include "common/variant_utils.h"
#include "script/node.h"
#include "script/nodes/functions/call_script_function.h"
#include "script/nodes/signals/emit_member_signal.h"
#include "script/nodes/signals/emit_signal.h"
#include "script/nodes/variables/variable.h"
#include "script/variable.h"

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

        WARN_PRINT(vformat("Removed orphan node %d from script %s.", E.key, get_self()->get_path()));
        _nodes.erase(E.key);
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

    _initialized = true;
}

void Orchestration::validate_and_build(BuildLog& p_log)
{
    // Sanity check
    _fix_orphans();

    for (const KeyValue<int, Ref<OScriptNode>>& E : _nodes)
    {
        p_log.set_current_node(E.value);

        E.value->validate_node_during_build(p_log);
        p_log.set_current_node({});
    }
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
    if (!p_pin || p_pin->get_flags().has_flag(OScriptNodePin::Flags::HIDDEN))
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

void Orchestration::rename_graph(const StringName& p_old_name, const StringName& p_new_name)
{
    ERR_FAIL_COND_MSG(!has_graph(p_old_name), "No graph exists with the old name: " + p_old_name);
    ERR_FAIL_COND_MSG(has_graph(p_new_name), "A graph already exists with the new name: " + p_new_name);

    Ref<OScriptGraph> graph = get_graph(p_old_name);
    graph->set_graph_name(p_new_name);

    _graphs[p_new_name] = graph;
    _graphs.erase(p_old_name);
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

    _functions[p_method.name] = function;

    _self->emit_signal("functions_changed");

    return function;
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

void Orchestration::rename_function(const StringName& p_old_name, const StringName& p_new_name)
{
    // Ignore if the old/new names are the same
    if (p_old_name == p_new_name)
        return;

    ERR_FAIL_COND_MSG(_has_instances(), "Cannot rename function, instances exist.");
    ERR_FAIL_COND_MSG(!has_function(p_old_name), "Cannot rename, no function found with old name: " + p_old_name);
    ERR_FAIL_COND_MSG(has_function(p_new_name), "Cannot rename, a function already exists with new name: " + p_new_name);
    ERR_FAIL_COND_MSG(!String(p_new_name).is_valid_identifier(), "New function name is invalid: " + p_new_name);
    ERR_FAIL_COND_MSG(has_variable(p_new_name), "Cannot rename function, a variable with name already exists: " + p_new_name);
    ERR_FAIL_COND_MSG(has_custom_signal(p_new_name), "Cannot rename function, a signal with the name already exists: " + p_new_name);

    const Ref<OScriptFunction> function = _functions[p_old_name];
    function->rename(p_new_name);

    _functions.erase(p_old_name);
    _functions[p_new_name] = function;

    // Rename function graph, if found
    const Ref<OScriptGraph> function_graph = find_graph(p_old_name);
    if (function_graph.is_valid() && function_graph->get_flags().has_flag(OScriptGraph::GraphFlags::GF_FUNCTION))
        rename_graph(p_old_name, p_new_name);

    _self->emit_signal("functions_changed");
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
    variable->_info.usage = PROPERTY_USAGE_STORAGE;
    _variables[p_name] = variable;

    #ifdef TOOLS_ENABLED
    _update_placeholders();
    #endif

    _self->emit_signal("variables_changed");

    return variable;
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

void Orchestration::rename_variable(const StringName& p_old_name, const StringName& p_new_name)
{
    if (p_old_name == p_new_name)
        return;

    ERR_FAIL_COND_MSG(_has_instances(), "Cannot rename variable, instances exist.");
    ERR_FAIL_COND_MSG(!has_variable(p_old_name), "Cannot rename, no variable exists with the old name: " + p_old_name);
    ERR_FAIL_COND_MSG(has_variable(p_new_name), "Cannot rename, a variable already exists with the new name: " + p_new_name);
    ERR_FAIL_COND_MSG(!String(p_new_name).is_valid_identifier(), "Cannot rename, variable name is not valid: " + p_new_name);

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

bool Orchestration::has_custom_signal(const StringName& p_name) const
{
    return _signals.has(p_name);
}

Ref<OScriptSignal> Orchestration::create_custom_signal(const StringName& p_name)
{
    ERR_FAIL_COND_V_MSG(has_custom_signal(p_name), nullptr, "A custom signal already exists with the name: " + p_name);

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

void Orchestration::rename_custom_user_signal(const StringName& p_old_name, const StringName& p_new_name)
{
    if (p_old_name == p_new_name)
        return;

    ERR_FAIL_COND_MSG(_has_instances(), "Cannot rename custom signal, instances exist.");
    ERR_FAIL_COND_MSG(!has_custom_signal(p_old_name), "No custom signal exists with the old name: " + p_old_name);
    ERR_FAIL_COND_MSG(has_custom_signal(p_new_name), "A custom signal already exists with the new name: " + p_new_name);
    ERR_FAIL_COND_MSG(!String(p_new_name).is_valid_identifier(), "The custom signal name is invalid: " + p_new_name);

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

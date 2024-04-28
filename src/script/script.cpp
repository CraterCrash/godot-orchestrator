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
#include "script.h"

#include "instances/script_instance.h"
#include "instances/script_instance_placeholder.h"
#include "nodes/script_nodes.h"
#include "resource/format_loader.h"
#include "resource/format_saver.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/mutex_lock.hpp>
#include <godot_cpp/templates/rb_set.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

OScript::OScript()
    : _valid(true)
    , _base_type("Object")
    , _language(OScriptLanguage::get_singleton())
{
}

void OScript::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_base_type", "p_base_type"), &OScript::set_base_type);
    ClassDB::bind_method(D_METHOD("get_base_type"), &OScript::get_base_type);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "base_type", PROPERTY_HINT_TYPE_STRING, "Node"), "set_base_type",
                 "get_base_type");

    // Purposely hidden until tested
    ClassDB::bind_method(D_METHOD("set_tool", "p_tool"), &OScript::set_tool);
    ClassDB::bind_method(D_METHOD("get_tool"), &OScript::get_tool);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "tool", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_tool",
                 "get_tool");

    ClassDB::bind_method(D_METHOD("_set_variables", "variables"), &OScript::_set_variables);
    ClassDB::bind_method(D_METHOD("_get_variables"), &OScript::_get_variables);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "variables", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE),
                 "_set_variables", "_get_variables");

    ClassDB::bind_method(D_METHOD("_set_functions", "functions"), &OScript::_set_functions);
    ClassDB::bind_method(D_METHOD("_get_functions"), &OScript::_get_functions);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "functions", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE),
                 "_set_functions", "_get_functions");

    ClassDB::bind_method(D_METHOD("_set_signals", "signals"), &OScript::_set_signals);
    ClassDB::bind_method(D_METHOD("_get_signals"), &OScript::_get_signals);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "signals", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE),
                 "_set_signals", "_get_signals");

    ClassDB::bind_method(D_METHOD("_set_connections", "connections"), &OScript::_set_connections);
    ClassDB::bind_method(D_METHOD("_get_connections"), &OScript::_get_connections);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "connections", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE),
                 "_set_connections", "_get_connections");

    ClassDB::bind_method(D_METHOD("_set_nodes", "nodes"), &OScript::_set_nodes);
    ClassDB::bind_method(D_METHOD("_get_nodes"), &OScript::_get_nodes);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "nodes", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "_set_nodes",
                 "_get_nodes");

    ClassDB::bind_method(D_METHOD("_set_graphs", "graphs"), &OScript::_set_graphs);
    ClassDB::bind_method(D_METHOD("_get_graphs"), &OScript::_get_graphs);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "graphs", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "_set_graphs",
                 "_get_graphs");

    ADD_SIGNAL(MethodInfo("connections_changed", PropertyInfo(Variant::STRING, "caller")));
    ADD_SIGNAL(MethodInfo("functions_changed"));
    ADD_SIGNAL(MethodInfo("variables_changed"));
    ADD_SIGNAL(MethodInfo("signals_changed"));
}

void OScript::post_initialize()
{
    for (const KeyValue<StringName, Ref<OScriptVariable>>& E : _variables)
        E.value->post_initialize();

    for (const KeyValue<int, Ref<OScriptNode>>& E : _nodes)
        E.value->post_initialize();

    _initialized = true;
}

/// Serialization //////////////////////////////////////////////////////////////////////////////////////////////////////

TypedArray<OScriptNode> OScript::_get_nodes() const
{
    Array r_out;
    for (const KeyValue<int, Ref<OScriptNode>>& E : _nodes)
        r_out.push_back(E.value);

    return r_out;
}

void OScript::_set_nodes(const TypedArray<OScriptNode>& p_nodes)
{
    _nodes.clear();
    for (int i = 0; i < p_nodes.size(); i++)
    {
        Ref<OScriptNode> node = p_nodes[i];
        node->set_owning_script(this);
        _nodes[node->get_id()] = node;
    }
}

TypedArray<int> OScript::_get_connections() const
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

void OScript::_set_connections(const TypedArray<int>& p_connections)
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

TypedArray<OScriptGraph> OScript::_get_graphs() const
{
    TypedArray<OScriptGraph> graphs;
    for (const KeyValue<StringName, Ref<OScriptGraph>>& E : _graphs)
        graphs.push_back(E.value);
    return graphs;
}

void OScript::_set_graphs(const TypedArray<OScriptGraph>& p_graphs)
{
    for (int i = 0; i < p_graphs.size(); i++)
    {
        Ref<OScriptGraph> graph = p_graphs[i];
        graph->set_owning_script(this);
        _graphs[graph->get_graph_name()] = graph;
    }
}

TypedArray<OScriptFunction> OScript::_get_functions() const
{
    TypedArray<OScriptFunction> functions;
    for (const KeyValue<StringName, Ref<OScriptFunction>>& E : _functions)
        functions.push_back(E.value);
    return functions;
}

void OScript::_set_functions(const TypedArray<OScriptFunction>& p_functions)
{
    _functions.clear();
    for (int i = 0; i < p_functions.size(); i++)
    {
        Ref<OScriptFunction> function = p_functions[i];
        function->_script = this;
        _functions[function->get_function_name()] = function;
    }
}

TypedArray<OScriptVariable> OScript::_get_variables() const
{
    TypedArray<OScriptVariable> variables;
    for (const KeyValue<StringName, Ref<OScriptVariable>>& E : _variables)
        variables.push_back(E.value);
    return variables;
}

void OScript::_set_variables(const TypedArray<OScriptVariable>& p_variables)
{
    _variables.clear();
    for (int i = 0; i < p_variables.size(); i++)
    {
        Ref<OScriptVariable> variable = p_variables[i];
        _variables[variable->get_variable_name()] = variable;
    }
}

TypedArray<OScriptSignal> OScript::_get_signals() const
{
    TypedArray<OScriptSignal> signals;
    for (const KeyValue<StringName, Ref<OScriptSignal>>& E : _signals)
        signals.push_back(E.value);
    return signals;
}

void OScript::_set_signals(const TypedArray<OScriptSignal>& p_signals)
{
    _signals.clear();
    for (int i = 0; i < p_signals.size(); i++)
    {
        Ref<OScriptSignal> signal = p_signals[i];
        _signals[signal->get_signal_name()] = signal;
    }
}

/// ScriptExtension ////////////////////////////////////////////////////////////////////////////////////////////////////

bool OScript::_editor_can_reload_from_file()
{
    return true;
}

void* OScript::_placeholder_instance_create(Object* p_object) const
{
#ifdef TOOLS_ENABLED
    Ref<Script> script(this);
    OScriptPlaceHolderInstance* psi = memnew(OScriptPlaceHolderInstance(script, p_object));
    {
        MutexLock lock(*_language->lock.ptr());
        _placeholders[p_object->get_instance_id()] = psi;
    }
    return internal::gdextension_interface_script_instance_create2(&OScriptPlaceHolderInstance::INSTANCE_INFO, psi);
#else
    return nullptr;
#endif
}

void OScript::_placeholder_erased(void* p_placeholder)
{
    for (const KeyValue<uint64_t, OScriptPlaceHolderInstance*>& E : _placeholders)
    {
        if (E.value == p_placeholder)
        {
            _placeholders.erase(E.key);
            break;
        }
    }
}

bool OScript::_is_placeholder_fallback_enabled() const
{
    return _placeholder_fallback_enabled;
}

bool OScript::placeholder_has(Object* p_object) const
{
    return _placeholders.has(p_object->get_instance_id());
}

void* OScript::_instance_create(Object* p_object) const
{
    OScriptInstance* si = memnew(OScriptInstance(Ref<Script>(this), _language, p_object));
    {
        MutexLock lock(*_language->lock.ptr());
        _instances[p_object] = si;
    }

    void* godot_inst = internal::gdextension_interface_script_instance_create2(&OScriptInstance::INSTANCE_INFO, si);

    // Dispatch the "Init Event" if its wired
    if (has_function("_init"))
    {
        Variant result;
        GDExtensionCallError err;
        si->call("_init", nullptr, 0, &result, &err);
    }

    return godot_inst;
}

bool OScript::_instance_has(Object* p_object) const
{
    return _instances.has(p_object);
}

bool OScript::_can_instantiate() const
{
    bool editor = Engine::get_singleton()->is_editor_hint();
    // Built-in script languages check if scripting is enabled OR if this is a tool script
    // Scripting is disabled by default in the editor
    return _valid && (_is_tool() || !editor);
}

Ref<Script> OScript::_get_base_script() const
{
    // No inheritance

    // Base in this case infers that a script inherits from another script, not that your script
    // inherits from a super type, such as Node.
    return {};
}

bool OScript::_inherits_script(const Ref<Script>& p_script) const
{
    // No inheritance
    return false;
}

StringName OScript::_get_global_name() const
{
    return "Orchestration";
}

StringName OScript::_get_instance_base_type() const
{
    return _base_type;
}

bool OScript::_has_source_code() const
{
    // No source
    return false;
}

String OScript::_get_source_code() const
{
    return {};
}

void OScript::_set_source_code(const String& p_code)
{
}

Error OScript::_reload(bool p_keep_state)
{
    // todo: need to find a way to reload the script when requested
    _valid = true;
    return OK;
}

TypedArray<Dictionary> OScript::_get_documentation() const
{
    // todo:    see how to generate it from the script/node contents
    //          see doc_data & script_language_extension
    return {};
}

bool OScript::_has_static_method(const StringName& p_method) const
{
    // Currently we don't support static methods
    return false;
}

bool OScript::_has_method(const StringName& p_method) const
{
    return _functions.has(p_method);
}

Dictionary OScript::_get_method_info(const StringName& p_method) const
{
    return {};
}

TypedArray<Dictionary> OScript::_get_script_method_list() const
{
    TypedArray<Dictionary> results;
    for (const KeyValue<StringName, Ref<OScriptFunction>>& E : _functions)
        results.push_back(E.value->to_dict());

    return results;
}

TypedArray<Dictionary> OScript::_get_script_property_list() const
{
    return {};
}

bool OScript::_is_tool() const
{
    return _tool;
}

bool OScript::_is_valid() const
{
    return _valid;
}

ScriptLanguage* OScript::_get_language() const
{
    return _language;
}

bool OScript::_has_script_signal(const StringName& p_signal) const
{
    return has_custom_signal(p_signal);
}

#include "common/dictionary_utils.h"

TypedArray<Dictionary> OScript::_get_script_signal_list() const
{
    TypedArray<Dictionary> list;

    for (const KeyValue<StringName, Ref<OScriptSignal>>& E : _signals)
        list.push_back(DictionaryUtils::from_method(E.value->get_method_info()));

    return list;
}

bool OScript::_has_property_default_value(const StringName& p_property) const
{
    for (const KeyValue<StringName, Ref<OScriptVariable>>& E : _variables)
    {
        if (E.key.match(p_property))
        {
            if (E.value->get_default_value().get_type() != Variant::NIL)
                return true;
        }
    }
    return false;
}

Variant OScript::_get_property_default_value(const StringName& p_property) const
{
    for (const KeyValue<StringName, Ref<OScriptVariable>>& E : _variables)
        if (E.key.match(p_property))
            return E.value->get_default_value();

    return {};
}

void OScript::_update_exports()
{
}

int32_t OScript::_get_member_line(const StringName& p_member) const
{
    return -1;
}

Dictionary OScript::_get_constants() const
{
    return {};
}

TypedArray<StringName> OScript::_get_members() const
{
    return {};
}

Variant OScript::_get_rpc_config() const
{
    // Gather a dictionary of all RPC calls defined
    return Dictionary();
}

String OScript::_get_class_icon_path() const
{
    return {};
}

/// OScript ////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool OScript::validate_and_build()
{
    for (const KeyValue<int, Ref<OScriptNode>>& E : _nodes)
    {
        if (!E.value->validate_node_during_build())
        {
            ERR_PRINT(vformat("Script %s failed to validate node %s with ID %d",
                              get_path(),
                              E.value->get_class(),
                              E.value->get_id()));
            return false;
        }
    }
    return true;
}

StringName OScript::get_base_type() const
{
    return _base_type;
}

void OScript::set_base_type(const StringName& p_base_type)
{
    if (!_base_type.match(p_base_type))
    {
        _base_type = p_base_type;
        emit_changed();
    }
}

bool OScript::is_edited() const
{
    return _edited;
}

int OScript::get_available_id() const
{
    // We should eventually consider a better strategy for node unique ids to deal with
    // scripts that are constantly modified with new nodes added and removed.
    int max = -1;
    for (const KeyValue<int, Ref<OScriptNode>>& E : _nodes)
        if (E.key > max)
            max = E.key;
    return (max + 1);
}

/// Node API ///////////////////////////////////////////////////////////////////////////////////////////////////////////

bool OScript::has_node(int p_node_id) const
{
    return _nodes.has(p_node_id);
}

void OScript::add_node(const Ref<OScriptGraph>& p_graph, const Ref<OScriptNode>& p_node)
{
    ERR_FAIL_COND(_instances.size());
    ERR_FAIL_COND(p_node.is_null());
    ERR_FAIL_COND(_nodes.has(p_node->get_id()));

    // Validate the node details
    p_node->set_owning_script(this);
    p_node->_validate_input_default_values();

    // Register the node with the script
    _nodes[p_node->get_id()] = p_node;

    // Register the node with the graph
    p_graph->add_node(p_node->get_id());
}

void OScript::remove_node(int p_node_id)
{
    ERR_FAIL_COND(!_instances.is_empty());
    ERR_FAIL_COND(!_nodes.has(p_node_id));

    _nodes[p_node_id]->pre_remove();

    // Check whether the node represents a function and if so, remove the function
    for (const KeyValue<StringName, Ref<OScriptFunction>>& E : _functions)
    {
        if (E.value->get_owning_node_id() == p_node_id)
        {
            _functions.erase(E.key);
            break;
        }
    }

    Ref<OScriptNode> node = _nodes[p_node_id];
    for (const Ref<OScriptNodePin>& pin : node->get_all_pins())
        pin->unlink_all(true);

    List<OScriptConnection> removals;
    for (const OScriptConnection& connection : _connections)
        if (connection.from_node == p_node_id || connection.to_node == p_node_id)
            removals.push_back(connection);

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
        E.value->remove_node(p_node_id);

    _nodes.erase(p_node_id);
}

Ref<OScriptNode> OScript::get_node(int p_node_id) const
{
    ERR_FAIL_COND_V_MSG(!_nodes.has(p_node_id), {}, "Failed to find node with id " + itos(p_node_id)
                                                        + ". Script has " + itos(_nodes.size()) + " nodes.");
    return _nodes[p_node_id];
}

Vector<Ref<OScriptNode>> OScript::get_nodes() const
{
    Vector<Ref<OScriptNode>> results;
    for (const KeyValue<int, Ref<OScriptNode>>& E : _nodes)
        results.push_back(E.value);
    return results;
}

/// Connection API /////////////////////////////////////////////////////////////////////////////////////////////////////

const RBSet<OScriptConnection>& OScript::get_connections() const
{
    return _connections;
}

void OScript::connect_nodes(int p_source_id, int p_source_port, int p_target_id, int p_target_port)
{
    ERR_FAIL_COND(_instances.size());

    OScriptConnection connection;
    connection.from_node = p_source_id;
    connection.from_port = p_source_port;
    connection.to_node = p_target_id;
    connection.to_port = p_target_port;

    ERR_FAIL_COND(_connections.has(connection));
    _connections.insert(connection);

    emit_signal("connections_changed", "connect_nodes");
}

void OScript::disconnect_nodes(int p_source_id, int p_source_port, int p_target_id, int p_target_port)
{
    OScriptConnection connection;
    connection.from_node = p_source_id;
    connection.from_port = p_source_port;
    connection.to_node = p_target_id;
    connection.to_port = p_target_port;

    ERR_FAIL_COND(!_connections.has(connection));
    _connections.erase(connection);

    emit_signal("connections_changed", "disconnect_nodes");
}

Vector<Ref<OScriptNodePin>> OScript::get_connections(const OScriptNodePin* p_pin) const
{
    Vector<Ref<OScriptNodePin>> result;
    if (OScriptNode* node = p_pin->get_owning_node())
    {
        const bool is_input = p_pin->is_input();
        const int node_id = node->get_id();

        // NOTE:
        // Do not cache get pin index above as this call will fail upon immediate placement
        // of new nodes because the pin index has not yet been cached. We need to find a
        // better solution for the indices.
        for (const OScriptConnection& E : _connections)
        {
            if (is_input && E.to_node == node_id && E.to_port == p_pin->get_pin_index())
            {
                Ref<OScriptNode> other = get_node(E.from_node);
                if (other.is_valid())
                {
                    Ref<OScriptNodePin> other_pin = other->find_pin(E.from_port, PD_Output);
                    if (other_pin.is_valid())
                        result.push_back(other_pin);
                }
            }
            else if (!is_input && E.from_node == node_id && E.from_port == p_pin->get_pin_index())
            {
                Ref<OScriptNode> other = get_node(E.to_node);
                if (other.is_valid())
                {
                    Ref<OScriptNodePin> other_pin = other->find_pin(E.to_port, PD_Input);
                    if (other_pin.is_valid())
                        result.push_back(other_pin);
                }
            }
        }
    }
    return result;
}

void OScript::adjust_connections(const OScriptNode* p_node, int p_offset, int p_adjustment, EPinDirection p_dir)
{
    printf("adjust_connections - %d %d\n", p_offset, p_adjustment);

    // NOTE:
    // The RBSet maintains search criteria order based on the calculation of Connection::operator< and
    // when we modify the port adjustments here, that invalidates the criteria, which will lead to a
    // situation where the tree's internal state no longer matches the object state, causing functions
    // such as "has" to fail to locate an entry.
    //
    // Now we could simply recreate the _connections RBSet by copying the old into a new instance, but
    // this is highly inefficient when the connection set maintains a number of objects in a large
    // script, so instead we'll cache the data-set specific to the mutation and adjust only those. It
    // should, in theory, be overall less impact to the data structure in large graphs.

    struct ConnectionData
    {
        OScriptConnection existing;
        OScriptConnection mutated;
    };

    List<ConnectionData> data;
    for (OScriptConnection& E : _connections)
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

    emit_signal("connections_changed", "adjust_connections");
}

/// Graph API //////////////////////////////////////////////////////////////////////////////////////////////////////////

bool OScript::has_graph(const StringName& p_name) const
{
    return _graphs.has(p_name);
}

Ref<OScriptGraph> OScript::create_graph(const StringName& p_name, int p_flags)
{
    ERR_FAIL_COND_V_MSG(has_graph(p_name), Ref<OScriptGraph>(), "A graph with that name already exists: " + p_name);
    ERR_FAIL_COND_V_MSG(p_name.is_empty(), Ref<OScriptGraph>(), "Cannot create a graph with an empty name.");

    Ref<OScriptGraph> graph(memnew(OScriptGraph));
    graph->set_owning_script(this);
    graph->set_graph_name(p_name);
    graph->set_flags(p_flags);

    _graphs[p_name] = graph;

    return graph;
}

void OScript::remove_graph(const StringName& p_name)
{
    ERR_FAIL_COND_MSG(!has_graph(p_name), "There is no graph with that name: " + p_name);

    // The default "EventGraph" cannot be removed.
    if (p_name.match("EventGraph"))
    {
        ERR_PRINT("The event graph '" + p_name + "' cannot be removed.");
        return;
    }

    Ref<OScriptGraph> graph = get_graph(p_name);

    // For each node in the graph, remove it from the script
    TypedArray<int> nodes = graph->get_nodes();
    for (int i = 0; i < nodes.size(); i++)
    {
        const int node_id = nodes[i];
        remove_node(node_id);
    }

    // Clear the graph's node references
    nodes.clear();
    graph->set_nodes(nodes);

    // Remove the graph
    _graphs.erase(p_name);
}

Ref<OScriptGraph> OScript::get_graph(const StringName& p_name) const
{
    ERR_FAIL_COND_V_MSG(!has_graph(p_name), Ref<OScriptGraph>(), "There is no graph with name " + p_name);
    return _graphs[p_name];
}

Ref<OScriptGraph> OScript::find_graph(const Ref<OScriptNode>& p_node)
{
    for (const KeyValue<StringName, Ref<OScriptGraph>>& E : _graphs)
        if (E.value->has_node(p_node->get_id()))
            return E.value;

    ERR_FAIL_V_MSG({}, "Failed to find node " + itos(p_node->get_id()) + " in any graph.");
}

void OScript::rename_graph(const StringName& p_old_name, const StringName& p_new_name)
{
    ERR_FAIL_COND_MSG(!has_graph(p_old_name), "There is no graph with the name: " + p_old_name);
    ERR_FAIL_COND_MSG(has_graph(p_new_name), "A graph already exists with the name: " + p_new_name);

    Ref<OScriptGraph> graph = get_graph(p_old_name);
    graph->set_graph_name(p_new_name);

    // Rename the entry in the hashmap
    _graphs[p_new_name] = _graphs[p_old_name];
    _graphs.erase(p_old_name);
}

Vector<Ref<OScriptGraph>> OScript::get_graphs() const
{
    Vector<Ref<OScriptGraph>> result;
    for (const KeyValue<StringName, Ref<OScriptGraph>>& E : _graphs)
        result.push_back(E.value);
    return result;
}

/// Functions API //////////////////////////////////////////////////////////////////////////////////////////////////////

bool OScript::has_function(const StringName& p_name) const
{
    return _functions.has(p_name);
}

Ref<OScriptFunction> OScript::create_function(const MethodInfo& p_method, int p_node_id, bool p_user_defined)
{
    ERR_FAIL_COND_V(_instances.size(), {});
    ERR_FAIL_COND_V(!String(p_method.name).is_valid_identifier(), {});
    ERR_FAIL_COND_V(_functions.has(p_method.name), {});
    ERR_FAIL_COND_V(_variables.has(p_method.name), {});
    ERR_FAIL_COND_V(_signals.has(p_method.name), {});

    Ref<OScriptFunction> function = OScriptFunction::create(this, p_method);
    function->_owning_node_id = p_node_id;
    function->_user_defined = p_user_defined;

    _functions[p_method.name] = function;
    emit_signal("functions_changed");

    return function;
}

void OScript::remove_function(const StringName& p_name)
{
    ERR_FAIL_COND(_instances.size());
    ERR_FAIL_COND(!_functions.has(p_name));
    {
        Ref<OScriptFunction> function = _functions[p_name];

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
            Ref<OScriptNodeCallScriptFunction> call_function = get_node(node_id);
            Ref<OScriptFunction> called_function = call_function->get_function();
            if (call_function.is_valid() && called_function->get_function_name().match(function->get_function_name()))
                remove_node(node_id);
        }

        // Find the node for this function and remove it
        if (has_node(function->get_owning_node_id()))
            remove_node(function->get_owning_node_id());

        // Let the editor handle node removal
        _functions.erase(p_name);

        emit_signal("functions_changed");
    }
    emit_changed();
}

Ref<OScriptFunction> OScript::find_function(const StringName& p_name)
{
    for (const KeyValue<StringName, Ref<OScriptFunction>>& E : _functions)
        if (E.value->get_function_name() == p_name)
            return E.value;

    return {};
}

Ref<OScriptFunction> OScript::find_function(const Guid& p_guid)
{
    for (const KeyValue<StringName, Ref<OScriptFunction>>& E : _functions)
        if (E.value->get_guid() == p_guid)
            return E.value;

    return {};
}

void OScript::rename_function(const StringName& p_old_name, const String& p_new_name)
{
    if (p_new_name == p_old_name)
        return;

    ERR_FAIL_COND(_instances.size());
    ERR_FAIL_COND(!_functions.has(p_old_name));
    ERR_FAIL_COND(!String(p_new_name).is_valid_identifier());
    ERR_FAIL_COND(_functions.has(p_new_name));
    ERR_FAIL_COND(_variables.has(p_new_name));
    ERR_FAIL_COND(_signals.has(p_new_name));

    // Update function list
    Ref<OScriptFunction> function = _functions[p_old_name];
    function->rename(p_new_name);

    _functions.erase(p_old_name);
    _functions[p_new_name] = function;

    if (_graphs.has(p_old_name))
    {
        Ref<OScriptGraph> graph = _graphs[p_old_name];
        if (graph->get_flags().has_flag(OScriptGraph::GF_FUNCTION))
            rename_graph(p_old_name, p_new_name);
    }

    emit_signal("functions_changed");
}

PackedStringArray OScript::get_function_names() const
{
    PackedStringArray psa;
    for (const KeyValue<StringName, Ref<OScriptFunction>>& E : _functions)
        psa.push_back(E.key);
    return psa;
}

int OScript::get_function_node_id(const StringName& p_name) const
{
    ERR_FAIL_COND_V(!_functions.has(p_name), -1);
    return _functions[p_name]->get_owning_node_id();
}

/// Variables API //////////////////////////////////////////////////////////////////////////////////////////////////////

bool OScript::has_variable(const StringName& p_name) const
{
    return _variables.has(p_name);
}

Ref<OScriptVariable> OScript::create_variable(const StringName& p_name, Variant::Type p_type)
{
    ERR_FAIL_COND_V(_instances.size(), Ref<OScriptVariable>());
    ERR_FAIL_COND_V(!String(p_name).is_valid_identifier(), Ref<OScriptVariable>());
    ERR_FAIL_COND_V(_variables.has(p_name), Ref<OScriptVariable>());

    PropertyInfo property;
    property.name = p_name;
    property.type = p_type;

    Ref<OScriptVariable> variable = OScriptVariable::create(this, property);
    _variables[p_name] = variable;

#ifdef TOOLS_ENABLED
    _update_placeholders();
#endif
    emit_signal("variables_changed");

    return variable;
}

void OScript::remove_variable(const StringName& p_name)
{
    ERR_FAIL_COND(!_variables.has(p_name));

    const List<int> node_ids = _get_node_type_node_ids<OScriptNodeVariable>();
    for (int node_id : node_ids)
    {
        Ref<OScriptNodeVariable> variable = get_node(node_id);
        if (variable.is_valid() && variable->get_variable()->get_variable_name().match(p_name))
            remove_node(node_id);
    }

    _variables.erase(p_name);

    emit_signal("variables_changed");

    emit_changed();
    notify_property_list_changed();

#ifdef TOOLS_ENABLED
    _update_placeholders();
#endif
}

Ref<OScriptVariable> OScript::get_variable(const StringName& p_name)
{
    if (_variables.has(p_name))
        return _variables[p_name];
    return {};
}

void OScript::rename_variable(const StringName& p_old_name, const StringName& p_new_name)
{
    ERR_FAIL_COND(_instances.size());
    ERR_FAIL_COND(!_variables.has(p_old_name));

    if (p_new_name == p_old_name)
        return;

    ERR_FAIL_COND(!String(p_new_name).is_valid_identifier());
    ERR_FAIL_COND(_variables.has(p_new_name));

    _variables[p_new_name] = _variables[p_old_name];
    _variables[p_new_name]->set_variable_name(p_new_name);

    _variables.erase(p_old_name);

    emit_signal("variables_changed");

    emit_changed();
    notify_property_list_changed();

#ifdef TOOLS_ENABLED
    _update_placeholders();
#endif
}

Vector<Ref<OScriptVariable>> OScript::get_variables() const
{
    Vector<Ref<OScriptVariable>> result;
    for (const KeyValue<StringName, Ref<OScriptVariable>>& E : _variables)
        result.push_back(E.value);
    return result;
}

PackedStringArray OScript::get_variable_names() const
{
    PackedStringArray psa;
    for (const KeyValue<StringName, Ref<OScriptVariable>>& E : _variables)
        psa.push_back(E.key);
    return psa;
}

bool OScript::can_remove_variable(const StringName& p_name) const
{
    for (const KeyValue<int, Ref<OScriptNode>>& E : _nodes)
    {
        Ref<OScriptNodeVariable> variable = E.value;
        if (variable.is_valid())
        {
            if (variable->get_variable()->get_variable_name().match(p_name))
                return false;
        }
    }
    return true;
}

/// Signals API ////////////////////////////////////////////////////////////////////////////////////////////////////////

bool OScript::has_custom_signal(const StringName& p_name) const
{
    return _signals.has(p_name);
}

Ref<OScriptSignal> OScript::create_custom_signal(const StringName& p_name)
{
    ERR_FAIL_COND_V_MSG(_signals.has(p_name), Ref<OScriptSignal>(), "Signal already defined with name " + p_name);

    MethodInfo method;
    method.name = p_name;
    // Fixed by https://github.com/godotengine/godot-cpp/pull/1440
    method.return_val.usage = PROPERTY_USAGE_DEFAULT;

    Ref<OScriptSignal> signal = OScriptSignal::create(this, method);
    _signals[p_name] = signal;

    emit_signal("signals_changed");

    return signal;
}

void OScript::remove_custom_signal(const StringName& p_name)
{
    ERR_FAIL_COND_MSG(!_signals.has(p_name), "No signal exists with name " + p_name);

    const List<int> node_ids = _get_node_type_node_ids<OScriptNodeEmitSignal>();
    for (int node_id : node_ids)
    {
        Ref<OScriptNodeEmitSignal> emit_signal = get_node(node_id);
        if (emit_signal.is_valid() && emit_signal->get_signal()->get_signal_name().match(p_name))
            remove_node(node_id);
    }

    _signals.erase(p_name);

    emit_signal("signals_changed");
}

Ref<OScriptSignal> OScript::get_custom_signal(const StringName& p_name)
{
    ERR_FAIL_COND_V_MSG(!_signals.has(p_name), Ref<OScriptSignal>(), "No signal with name " + p_name);
    return _signals[p_name];
}

Ref<OScriptSignal> OScript::find_custom_signal(const StringName& p_name)
{
    if (_signals.has(p_name))
        return _signals[p_name];

    return {};
}

void OScript::rename_custom_user_signal(const StringName& p_old_name, const StringName& p_new_name)
{
    ERR_FAIL_COND(_instances.size());
    ERR_FAIL_COND(!_signals.has(p_old_name));

    if (p_new_name == p_old_name)
        return;

    ERR_FAIL_COND(!String(p_new_name).is_valid_identifier());
    ERR_FAIL_COND(_signals.has(p_new_name));

    _signals[p_new_name] = _signals[p_old_name];
    _signals[p_new_name]->rename(p_new_name);

    _signals.erase(p_old_name);

    emit_signal("signals_changed");

    emit_changed();
    notify_property_list_changed();

#ifdef TOOLS_ENABLED
    _update_placeholders();
#endif
}

PackedStringArray OScript::get_custom_signal_names() const
{
    PackedStringArray psa;
    for (const KeyValue<StringName, Ref<OScriptSignal>>& E : _signals)
        psa.push_back(E.key);
    return psa;
}

bool OScript::can_remove_custom_signal(const StringName& p_name) const
{
    for (const KeyValue<int, Ref<OScriptNode>>& E : _nodes)
    {
        Ref<OScriptNodeEmitSignal> emit_signal = E.value;
        if (emit_signal.is_valid())
        {
            if (emit_signal->get_signal()->get_signal_name().match(p_name))
                return false;
        }
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Internal API

void OScript::set_edited(bool p_edited)
{
    _edited = p_edited;
    emit_changed();
}

bool OScript::_update_exports_placeholder(bool* r_err, bool p_recursive_call, OScriptInstance* p_instance) const
{
#ifdef TOOLS_ENABLED
    return true;
#else
    return false;
#endif
}

void OScript::_update_placeholders()
{
}

void register_script_classes()
{
    // Abstracts first
    ORCHESTRATOR_REGISTER_ABSTRACT_NODE_CLASS(OScriptNode)

    ORCHESTRATOR_REGISTER_INTERNAL_CLASS(OScriptTargetObject)
    ORCHESTRATOR_REGISTER_INTERNAL_CLASS(OScriptNodePin)
    ORCHESTRATOR_REGISTER_INTERNAL_CLASS(OScriptResourceLoader)
    ORCHESTRATOR_REGISTER_INTERNAL_CLASS(OScriptResourceSaver)
    ORCHESTRATOR_REGISTER_INTERNAL_CLASS(OScriptLanguage)
    ORCHESTRATOR_REGISTER_INTERNAL_CLASS(OScriptGraph)
    ORCHESTRATOR_REGISTER_INTERNAL_CLASS(OScriptFunction)
    ORCHESTRATOR_REGISTER_INTERNAL_CLASS(OScriptVariable)
    ORCHESTRATOR_REGISTER_INTERNAL_CLASS(OScriptSignal)
    ORCHESTRATOR_REGISTER_INTERNAL_CLASS(OScriptState)
    ORCHESTRATOR_REGISTER_INTERNAL_CLASS(OScriptAction)

    // Purposely public
    ORCHESTRATOR_REGISTER_CLASS(OScript)
}
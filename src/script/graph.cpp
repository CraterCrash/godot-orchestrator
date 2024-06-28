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
#include "script/graph.h"

#include "script/nodes/functions/event.h"
#include "script/nodes/functions/function_entry.h"
#include "script/nodes/functions/function_terminator.h"
#include "script/script.h"

#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/templates/hash_set.hpp>

void OScriptGraph::_bind_methods()
{
    BIND_BITFIELD_FLAG(GraphFlags::GF_NONE)
    BIND_BITFIELD_FLAG(GraphFlags::GF_RENAMABLE)
    BIND_BITFIELD_FLAG(GraphFlags::GF_DELETABLE)
    BIND_BITFIELD_FLAG(GraphFlags::GF_EVENT)
    BIND_BITFIELD_FLAG(GraphFlags::GF_FUNCTION)
    BIND_BITFIELD_FLAG(GraphFlags::GF_DEFAULT)

    ClassDB::bind_method(D_METHOD("set_graph_name", "graph_name"), &OScriptGraph::set_graph_name);
    ClassDB::bind_method(D_METHOD("get_graph_name"), &OScriptGraph::get_graph_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "graph_name"), "set_graph_name", "get_graph_name");

    ClassDB::bind_method(D_METHOD("set_viewport_offset", "offset"), &OScriptGraph::set_viewport_offset);
    ClassDB::bind_method(D_METHOD("get_viewport_offset"), &OScriptGraph::get_viewport_offset);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "offset"), "set_viewport_offset", "get_viewport_offset");

    ClassDB::bind_method(D_METHOD("set_viewport_zoom", "zoom"), &OScriptGraph::set_viewport_zoom);
    ClassDB::bind_method(D_METHOD("get_viewport_zoom"), &OScriptGraph::get_viewport_zoom);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "zoom"), "set_viewport_zoom", "get_viewport_zoom");

    ClassDB::bind_method(D_METHOD("set_flags", "flags"), &OScriptGraph::set_flags);
    ClassDB::bind_method(D_METHOD("get_flags"), &OScriptGraph::get_flags);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "flags"), "set_flags", "get_flags");

    ClassDB::bind_method(D_METHOD("_set_nodes", "nodes"), &OScriptGraph::_set_nodes);
    ClassDB::bind_method(D_METHOD("_get_nodes"), &OScriptGraph::_get_nodes);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "nodes"), "_set_nodes", "_get_nodes");

    ClassDB::bind_method(D_METHOD("_set_functions", "functions"), &OScriptGraph::_set_functions);
    ClassDB::bind_method(D_METHOD("_get_functions"), &OScriptGraph::_get_functions);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "functions"), "_set_functions", "_get_functions");

    ClassDB::bind_method(D_METHOD("_set_knots", "knots"), &OScriptGraph::_set_knots);
    ClassDB::bind_method(D_METHOD("_get_knots"), &OScriptGraph::_get_knots);
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "knots"), "_set_knots", "_get_knots");

    ADD_SIGNAL(MethodInfo("node_added", PropertyInfo(Variant::INT, "node_id")));
    ADD_SIGNAL(MethodInfo("node_removed", PropertyInfo(Variant::INT, "node_id")));
    ADD_SIGNAL(MethodInfo("knots_updated"));
}

TypedArray<int> OScriptGraph::_get_nodes() const
{
    TypedArray<int> nodes;
    for (const int &node_id : _nodes)
        nodes.push_back(node_id);
    return nodes;
}

void OScriptGraph::_set_nodes(const TypedArray<int>& p_nodes)
{
    _nodes.clear();
    for (int i = 0; i < p_nodes.size(); i++)
        _nodes.insert(p_nodes[i]);
    emit_changed();
}

TypedArray<Dictionary> OScriptGraph::_get_knots() const
{
    TypedArray<Dictionary> results;
    for (const KeyValue<uint64_t, PackedVector2Array>& E : _knots)
    {
        Dictionary data;
        data["id"] = E.key;
        data["points"] = E.value;
        results.push_back(data);
    }
    return results;
}

void OScriptGraph::_set_knots(const TypedArray<Dictionary>& p_knots)
{
    _knots.clear();
    for (int i = 0; i < p_knots.size(); i++)
    {
        const Dictionary& data = p_knots[i];
        const uint64_t id = data["id"];
        const PackedVector2Array points = data["points"];
        _knots[id] = points;
    }
}

TypedArray<int> OScriptGraph::_get_functions() const
{
    TypedArray<int> functions;
    for (const int &function_node_id : _functions)
        functions.push_back(function_node_id);
    return functions;
}

void OScriptGraph::_set_functions(const TypedArray<int>& p_functions)
{
    _functions.clear();
    for (int i = 0; i < p_functions.size(); i++)
        _functions.insert(p_functions[i]);

    emit_changed();
}

void OScriptGraph::_initialize_node(const Ref<OScriptNode>& p_node, const OScriptNodeInitContext& p_context, const Vector2& p_position)
{
    p_node->initialize(p_context);

    if (p_position != Vector2())
        p_node->set_position(p_position);

    _orchestration->add_node(this, p_node);

    p_node->post_placed_new_node();

    add_node(p_node);
}

void OScriptGraph::_remove_node(int p_node_id)
{
    _nodes.erase(p_node_id);

    HashSet<uint64_t> connection_ids;
    for (const KeyValue<uint64_t, PackedVector2Array>& E : _knots)
    {
        OScriptConnection C(E.key);
        if (C.is_linked_to(p_node_id))
            connection_ids.insert(C.id);
    }

    if (!connection_ids.is_empty())
    {
        for (const uint64_t& connection_id : connection_ids)
            _knots.erase(connection_id);
        emit_signal("knots_updated");
    }

    if (_functions.has(p_node_id))
        _functions.erase(p_node_id);

    emit_signal("node_removed", p_node_id);
}

void OScriptGraph::post_initialize()
{
    // OScriptNodeEvent nodes were not being registered with OScriptGraph properly for overrides
    for (const int node_id : _nodes)
    {
        Ref<OScriptNodeEvent> event = _orchestration->get_node(node_id);
        if (event.is_valid() && !_functions.has(node_id))
        {
            WARN_PRINT(vformat(
                "Script '%s': Migrating registration in graph %s for node ID %d.",
                _orchestration->get_self()->get_path(), get_graph_name(), node_id));

            _functions.insert(node_id);
        }
    }

    // Fixup and function node references that are invalid
    RBSet<int>::Iterator iterator = _functions.begin();
    while (iterator != _functions.end())
    {
        const int function_id = *iterator;
        if (!_orchestration->_nodes.has(function_id))
        {
            WARN_PRINT(vformat(
                "Script '%s': Removed orphan function reference found in graph %s for node ID %d.",
                _orchestration->get_self()->get_path(), get_graph_name(), function_id));

            _functions.erase(function_id);
        }
        ++iterator;
    }
}

Orchestration* OScriptGraph::get_orchestration() const
{
    return _orchestration;
}

StringName OScriptGraph::get_graph_name() const
{
    return _name;
}

void OScriptGraph::set_graph_name(const StringName& p_name)
{
    if (!_name.match(p_name))
    {
        _name = p_name;
        emit_changed();
    }
}

Vector2 OScriptGraph::get_viewport_offset() const
{
    return _offset;
}

void OScriptGraph::set_viewport_offset(const Vector2& p_offset)
{
    if (_offset != p_offset)
    {
        _offset = p_offset;
        emit_changed();
    }
}

double OScriptGraph::get_viewport_zoom() const
{
    return _zoom;
}

void OScriptGraph::set_viewport_zoom(const double p_zoom)
{
    if (!UtilityFunctions::is_equal_approx(_zoom, p_zoom))
    {
        _zoom = p_zoom;
        emit_changed();
    }
}

BitField<OScriptGraph::GraphFlags> OScriptGraph::get_flags() const
{
    return _flags;
}

void OScriptGraph::set_flags(BitField<GraphFlags> p_flags)
{
    if (_flags != p_flags)
    {
        _flags = p_flags;
        emit_changed();
    }
}

void OScriptGraph::sanitize_nodes()
{
    List<int> remove_queue;
    for (const int node_id : _nodes)
    {
        const Ref<OScriptNode> node = _orchestration->get_node(node_id);
        if (!node.is_valid())
        {
            ERR_PRINT(
                vformat("Graph %s has node with id %d, but node is not found in the script metadata.",
                    get_graph_name(), node_id));
            remove_queue.push_back(node_id);
        }
    }

    while (!remove_queue.is_empty())
    {
        _nodes.erase(remove_queue.front()->get());
        remove_queue.pop_front();
    }
}

Vector<Ref<OScriptNode>> OScriptGraph::get_nodes() const
{
    List<int> removals;

    Vector<Ref<OScriptNode>> nodes;
    for (const int node_id : _nodes)
    {
        Ref<OScriptNode> node = _orchestration->get_node(node_id);
        if (node.is_valid())
            nodes.push_back(_orchestration->get_node(node_id));
    }
    return nodes;
}

RBSet<OScriptConnection> OScriptGraph::get_connections() const
{
    RBSet<OScriptConnection> connections;
    for (const OScriptConnection& E : _orchestration->get_connections())
    {
        if (_nodes.has(E.from_node) || _nodes.has(E.to_node))
            connections.insert(E);
    }
    return connections;
}

void OScriptGraph::link(int p_source_id, int p_source_port, int p_target_id, int p_target_port)
{
    _orchestration->_connect_nodes(p_source_id, p_source_port, p_target_id, p_target_port);
}

void OScriptGraph::unlink(int p_source_id, int p_source_port, int p_target_id, int p_target_port)
{
    _orchestration->_disconnect_nodes(p_source_id, p_source_port, p_target_id, p_target_port);
}

Ref<OScriptNode> OScriptGraph::get_node(int p_node_id) const
{
    return _orchestration->get_node(p_node_id);
}

void OScriptGraph::add_node(const Ref<OScriptNode>& p_node)
{
    _nodes.insert(p_node->get_id());

    // script_view#_create_new_function
    const Ref<OScriptNodeFunctionEntry> function_entry = p_node;
    if (function_entry.is_valid() && _flags.has_flag(GF_FUNCTION) && !_functions.has(p_node->get_id()))
        _functions.insert(p_node->get_id());

    // script_view#_add_callback
    const Ref<OScriptNodeEvent> event_entry = p_node;
    if (event_entry.is_valid() && _flags.has_flag(GF_EVENT) && !_functions.has(p_node->get_id()))
        _functions.insert(p_node->get_id());

    emit_signal("node_added", p_node->get_id());
}

void OScriptGraph::remove_node(const Ref<OScriptNode>& p_node)
{
    _remove_node(p_node->get_id());
}

void OScriptGraph::remove_all_nodes()
{
    while (!_nodes.is_empty())
    {
        // todo: handle this better
        const int node_id = _nodes.front()->get();
        _remove_node(node_id);
        _orchestration->remove_node(node_id);
    }
}

void OScriptGraph::move_node_to(const Ref<OScriptNode>& p_node, const Ref<OScriptGraph>& p_target)
{
    remove_node(p_node);
    p_target->add_node(p_node);
}

Ref<OScriptNode> OScriptGraph::duplicate_node(int p_node_id, const Vector2& p_delta, bool p_duplicate_resources)
{
    const Ref<OScriptNode> node = get_node(p_node_id);
    if (!node.is_valid())
    {
        ERR_PRINT("Cannot duplicate node with id " + itos(p_node_id));
        return nullptr;
    }

    // Duplicate node
    Ref<OScriptNode> duplicate = node->duplicate(p_duplicate_resources);

    // Initialize the duplicate node
    // While the ID and Position are obvious, the other two maybe are not.
    // Setting OScript is necessary because the OScript pointer is not stored/persisted.
    // Additionally, there are other node properties that are maybe references to other objects or data
    // that is post-processed after placement but before rendering, and this allows this step to handle
    // that cleanly.
    duplicate->_orchestration = _orchestration;
    duplicate->set_id(_orchestration->get_available_id());
    duplicate->set_position(node->get_position() + p_delta);
    duplicate->post_initialize();

    _orchestration->add_node(this, duplicate);
    duplicate->post_placed_new_node();

    return duplicate;
}

Ref<OScriptNode> OScriptGraph::paste_node(const Ref<OScriptNode>& p_node, const Vector2& p_position)
{
    p_node->_orchestration = _orchestration;
    p_node->set_id(_orchestration->get_available_id());
    p_node->set_position(p_position);
    p_node->post_initialize();

    _orchestration->add_node(this, p_node);
    p_node->post_placed_new_node();

    return p_node;
}

Vector<Ref<OScriptFunction>> OScriptGraph::get_functions() const
{
    Vector<Ref<OScriptFunction>> functions;
    for (int function_id : _functions)
    {
        Ref<OScriptNodeFunctionTerminator> term = _orchestration->get_node(function_id);
        if (term.is_valid() && !functions.has(term->get_function()))
            functions.push_back(term->get_function());
    }
    return functions;
}

const HashMap<uint64_t, PackedVector2Array>& OScriptGraph::get_knots() const
{
    return _knots;
}

void OScriptGraph::set_knots(const HashMap<uint64_t, PackedVector2Array>& p_knots)
{
    _knots = p_knots;
    emit_signal("knots_updated");
}

void OScriptGraph::remove_connection_knot(uint64_t p_connection_id)
{
    _knots.erase(p_connection_id);
    emit_signal("knots_updated");
}

Ref<OScriptNode> OScriptGraph::create_node(const StringName& p_type, const OScriptNodeInitContext& p_context, const Vector2& p_position)
{
    const Ref<OScriptNode> spawned = OScriptLanguage::get_singleton()->create_node_from_name(p_type, _orchestration);
    if (spawned.is_valid())
        _initialize_node(spawned, p_context, p_position);

    return spawned;
}

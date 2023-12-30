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
#include "graph.h"

#include <godot_cpp/variant/utility_functions.hpp>

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

    ClassDB::bind_method(D_METHOD("set_nodes", "nodes"), &OScriptGraph::set_nodes);
    ClassDB::bind_method(D_METHOD("get_nodes"), &OScriptGraph::get_nodes);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "nodes"), "set_nodes", "get_nodes");

    ClassDB::bind_method(D_METHOD("set_functions", "functions"), &OScriptGraph::set_functions);
    ClassDB::bind_method(D_METHOD("get_functions"), &OScriptGraph::get_functions);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "functions"), "set_functions", "get_functions");

    ADD_SIGNAL(MethodInfo("node_added", PropertyInfo(Variant::INT, "node_id")));
    ADD_SIGNAL(MethodInfo("node_removed", PropertyInfo(Variant::INT, "node_id")));
}

OScript* OScriptGraph::get_owning_script() const
{
    return _script;
}

void OScriptGraph::set_owning_script(OScript* p_script)
{
    _script = p_script;
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

TypedArray<int> OScriptGraph::get_nodes() const
{
    TypedArray<int> nodes;
    for (const int &node_id : _nodes)
        nodes.push_back(node_id);
    return nodes;
}

void OScriptGraph::set_nodes(const TypedArray<int>& p_nodes)
{
    _nodes.clear();
    for (int i = 0; i < p_nodes.size(); i++)
        _nodes.insert(p_nodes[i]);

    emit_changed();
}

void OScriptGraph::add_node(int p_node_id)
{
    _nodes.insert(p_node_id);
    emit_signal("node_added", p_node_id);
}

void OScriptGraph::remove_node(int p_node_id)
{
    _nodes.erase(p_node_id);
    emit_signal("node_removed", p_node_id);
}

TypedArray<int> OScriptGraph::get_functions() const
{
    TypedArray<int> functions;
    for (const int &function_node_id : _functions)
        functions.push_back(function_node_id);
    return functions;
}

void OScriptGraph::set_functions(const TypedArray<int>& p_functions)
{
    _functions.clear();
    for (int i = 0; i < p_functions.size(); i++)
        _functions.insert(p_functions[i]);

    emit_changed();
}

void OScriptGraph::add_function(int p_node_id)
{
    _functions.insert(p_node_id);
}

void OScriptGraph::remove_function(int p_node_id)
{
    _functions.erase(p_node_id);
}

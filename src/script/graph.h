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
#ifndef ORCHESTRATOR_SCRIPT_GRAPH_H
#define ORCHESTRATOR_SCRIPT_GRAPH_H

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/templates/rb_set.hpp>

using namespace godot;

/// Forward declarations
class OScript;

/// An Orchestration maintains a collection of OScriptGraph instances, which act as a visual
/// collection of nodes. In other words, this is used solely by the UI for representing any
/// logical group of nodes and is not used in the runtime of a script at all.
///
/// Therefore, at runtime, no OScriptInstance or OScriptNodeInstance should make any claim
/// or attempt to use any OScriptGraph object at all.
///
class OScriptGraph : public Resource
{
    GDCLASS(OScriptGraph, Resource);

    static void _bind_methods();

public:
    enum GraphFlags
    {
        GF_NONE = 0,                              //! None, should never be used technically
        GF_RENAMABLE = 1 << 1,                    //! The graph name can be changed
        GF_DELETABLE = 1 << 2,                    //! The graph can be deleted
        GF_EVENT = 1 << 3,                        //! The graph represents an event graph
        GF_FUNCTION = 1 << 4,                     //! The graph represents a free function
        GF_DEFAULT = GF_RENAMABLE | GF_DELETABLE  //! Default flags
    };

private:
    OScript* _script{ nullptr };       //! Owning script
    StringName _name;                  //! Unique name for this graph
    Vector2 _offset;                   //! Viewport offset
    double _zoom{ 1.f };               //! Viewport zoom
    BitField<GraphFlags> _flags{ 0 };  //! Flags
    RBSet<int> _nodes;                 //! Set of node ids that participate in this graph
    RBSet<int> _functions;             //! Set of node ids that represent entry points or functions

public:

    /// Get the owning Orchestrator script
    /// @return the orchestrator script
    OScript* get_owning_script() const;

    /// Set the script that owns this node
    /// @param p_script the owning script
    void set_owning_script(OScript* p_script);

    /// Get the graph name
    /// @return the graph name
    StringName get_graph_name() const;

    /// Set the graph name
    /// @param p_name the graph name
    void set_graph_name(const StringName& p_name);

    /// Get the graph's current viewport offset
    Vector2 get_viewport_offset() const;

    /// Set the graph's viewport offset
    /// @param p_offset the viewport offset
    void set_viewport_offset(const Vector2& p_offset);

    /// Get the viewport zoom
    /// @return the viewport zoom
    double get_viewport_zoom() const;

    /// Set the viewport zoom
    /// @param p_zoom the viewport zoom
    void set_viewport_zoom(double p_zoom);

    /// Get the flags associated with the graph
    /// @return a bit field of flags
    BitField<GraphFlags> get_flags() const;

    /// Set the flags associated with the graph
    /// @param p_flags bit field of flags
    void set_flags(BitField<GraphFlags> p_flags);

    /// Check whether the specified node participates in this graph
    /// @param p_node_id the node unique id to check
    /// @return true if the node participates in this graph, false otherwise
    bool has_node(int p_node_id) const { return _nodes.has(p_node_id); }

    /// Add a new node to this specific graph
    /// @param p_node_id the node unique id
    void add_node(int p_node_id);

    /// Remove a node from the graph
    /// @param p_node_id the node unique id
    void remove_node(int p_node_id);

    /// Get an array of nodes that participate in this graph, used for serialization.
    /// @return an array of node ids
    TypedArray<int> get_nodes() const;

    /// Sets the nodes that participates in this graph as an array, used for serialization.
    /// @param p_nodes array of nodes that are part of this graph
    void set_nodes(const TypedArray<int>& p_nodes);

    /// Check whether the specified function is contained within this graph
    /// @param p_node_id the node unique id
    /// @return true if the function is in this graph, false otherwise
    bool has_function(int p_node_id) const { return _functions.has(p_node_id); }

    /// Add a new function to this specific graph
    /// @param p_node_id the function node id
    void add_function(int p_node_id);

    /// Remove a function from the graph
    /// @param p_node_id the function node id
    void remove_function(int p_node_id);

    /// Get an array of function nodes that participate in this graph, used for serialization.
    /// @return an array of function node ids
    TypedArray<int> get_functions() const;

    /// Sets the function nodes that particpate in this graph, used for serialization.
    /// @param p_functions array of function node ids that are part of this graph
    void set_functions(const TypedArray<int>& p_functions);

};

VARIANT_BITFIELD_CAST(OScriptGraph::GraphFlags)

#endif  // ORCHESTRATOR_SCRIPT_GRAPH_H

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
#ifndef ORCHESTRATOR_SCRIPT_GRAPH_H
#define ORCHESTRATOR_SCRIPT_GRAPH_H

#include "script/connection.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/rb_set.hpp>

using namespace godot;

/// Forward declarations
class Orchestration;
class OScriptFunction;
class OScriptNode;

struct OScriptNodeInitContext;

/// An Orchestration maintains a collection of OScriptGraph instances, which act as a visual
/// collection of nodes. In other words, this is used solely by the UI for representing any
/// logical group of nodes and is not used in the runtime of a script at all.
///
/// Therefore, at runtime, no OScriptInstance or OScriptNodeInstance should make any claim
/// or attempt to use any OScriptGraph object at all.
///
class OScriptGraph : public Resource
{
    friend class Orchestration;

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
    Orchestration* _orchestration{ nullptr };      //! Owning orchestration
    StringName _name;                              //! Unique name for this graph
    Vector2 _offset;                               //! Viewport offset
    double _zoom{ 1.f };                           //! Viewport zoom
    BitField<GraphFlags> _flags{ 0 };              //! Flags
    RBSet<int> _nodes;                             //! Set of node ids that participate in this graph
    RBSet<int> _functions;                         //! Set of node ids that represent entry points or functions
    HashMap<uint64_t, PackedVector2Array> _knots;  //! Knots for each graph connection

    //~ Begin Serialization
    TypedArray<int> _get_nodes() const;
    void _set_nodes(const TypedArray<int>& p_nodes);
    TypedArray<Dictionary> _get_knots() const;
    void _set_knots(const TypedArray<Dictionary>& p_knots);
    TypedArray<int> _get_functions() const;
    void _set_functions(const TypedArray<int>& p_functions);
    //~ End Serialization

    /// Initializes the node in this graph
    /// @param p_node the node to be initialized
    /// @param p_context the context details to initialize the node with
    /// @param p_position the position of the node
    void _initialize_node(const Ref<OScriptNode>& p_node, const OScriptNodeInitContext& p_context, const Vector2& p_position);

    /// Removes a node from this graph
    /// @param p_node_id the node id to be removed
    void _remove_node(int p_node_id);

    /// Constructor
    /// Intentionally protected, graphs created via an Orchestration
    OScriptGraph() = default;

public:
    /// Performs post resource initialization.
    /// This is used to align and fix-up state across versions.
    void post_initialize();

    /// Get the owning orchestration
    /// @return the owning orchestration
    Orchestration* get_orchestration() const;

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

    /// Get all connections within this graph
    /// @return the graph connections
    RBSet<OScriptConnection> get_connections() const;

    /// Links two ports between a source and target node within this graph
    /// @param p_source_id the source node id
    /// @param p_source_port the source node port
    /// @param p_target_id the target node id
    /// @param p_target_port the target node port
    void link(int p_source_id, int p_source_port, int p_target_id, int p_target_port);

    /// Unlinks two ports between a source and target node within the graph
    /// @param p_source_id the source node id
    /// @param p_source_port the source node port
    /// @param p_target_id the target node id
    /// @param p_target_port the target node port
    void unlink(int p_source_id, int p_source_port, int p_target_id, int p_target_port);

    /// Check whether the specified node participates in this graph
    /// @param p_node_id the node unique id to check
    /// @return true if the node participates in this graph, false otherwise
    bool has_node(int p_node_id) const { return _nodes.has(p_node_id); }

    /// Lookup a node in the graph by its unique ID
    /// @param p_node_id the node id
    /// @return the node reference if found, or an invalid reference if not found
    Ref<OScriptNode> get_node(int p_node_id) const;

    /// Add a new node to this specific graph
    /// @param p_node the node
    void add_node(const Ref<OScriptNode>& p_node);

    /// Remove a node from the graph
    /// @param p_node the node
    void remove_node(const Ref<OScriptNode>& p_node);

    /// Removes all nodes from the graph
    void remove_all_nodes();

    /// Move the specified node to another graph
    /// @param p_node the node in this graph that is to be moved
    /// @param p_target the target graph
    void move_node_to(const Ref<OScriptNode>& p_node, const Ref<OScriptGraph>& p_target);

    /// Duplicate the specified node
    /// @param p_node_id the node to duplicate
    /// @param p_delta the position to add to the existing node's position
    /// @param p_duplicate_resources whether to duplicate sub-resources, defaults to false
    /// @return the duplicated node reference
    Ref<OScriptNode> duplicate_node(int p_node_id, const Vector2& p_delta, bool p_duplicate_resources = false);

    /// Pastes node into this graph
    /// @param p_node the node to be pasted
    /// @param p_position the position to be placed
    /// @return the pasted node
    Ref<OScriptNode> paste_node(const Ref<OScriptNode>& p_node, const Vector2& p_position);

    /// Sanitize the nodes array
    void sanitize_nodes();

    /// Get an array of nodes that participate in this graph
    /// @return an array of nodes
    Vector<Ref<OScriptNode>> get_nodes() const;

    /// Get an array of all functions that participate in this graph
    /// @return an array of functions
    Vector<Ref<OScriptFunction>> get_functions() const;

    /// Get an immutable map of knots for this graph's connections.
    /// @return knot map
    const HashMap<uint64_t, PackedVector2Array>& get_knots() const;

    /// Sets the knot map for this graph's connections
    /// @param p_knots the knot map
    void set_knots(const HashMap<uint64_t, PackedVector2Array>& p_knots);

    /// Remove connection knots for connection
    void remove_connection_knot(uint64_t p_connection_id);

    /// Create a new node within this graph
    /// @tparam T the node type
    /// @param p_context node initialization context
    /// @param p_position the position to place the node, only used if the value isn't <code>Vector2(0,0)</code>.
    /// @return the newly created node refereence, or an invalid reference if the creation failed
    template<typename T>
    Ref<T> create_node(const OScriptNodeInitContext& p_context, const Vector2& p_position = Vector2())
    {
        return create_node(T::get_class_static(), p_context, p_position);
    }

    /// Create a new node within this graph by type
    /// @param p_type the node type to spawn
    /// @param p_context node initialization context
    /// @param p_position the position to place the node
    /// @return the newly spawned node reference, of an invalid reference if the spawn failed
    Ref<OScriptNode> create_node(const StringName& p_type, const OScriptNodeInitContext& p_context, const Vector2& p_position = Vector2());
};

VARIANT_BITFIELD_CAST(OScriptGraph::GraphFlags)

#endif  // ORCHESTRATOR_SCRIPT_GRAPH_H

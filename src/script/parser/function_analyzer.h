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
#ifndef ORCHESTRATOR_SCRIPT_PARSER_GRAPH_ANALYZER_H
#define ORCHESTRATOR_SCRIPT_PARSER_GRAPH_ANALYZER_H

#include "script/function.h"
#include "script/node_pin.h"

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>

using namespace godot;

// Forward declarations
class OScriptNode;

using NodeId = int;
using PinId = int;

// Uniquely identifies a specific node/pin pair
struct OScriptNodePinId {
    NodeId node;
    PinId pin;

    bool operator==(const OScriptNodePinId& p_other) const {
        return node == p_other.node && pin == p_other.pin;
    }
};

struct OScriptNodePinIdHasher {
    static uint32_t hash(const OScriptNodePinId& p_key) {
        uint32_t h = 5381;
        h = hash_murmur3_one_64(p_key.node, h);
        h = hash_murmur3_one_64(p_key.pin, h);
        return h;
    }
};

struct OScriptNetKey {
    NodeId node;
    PinId pin;

    bool operator==(const OScriptNetKey& p_other) const {
        return node == p_other.node && pin == p_other.pin;
    }
};

struct OScriptNetKeyHasher {
    static uint32_t hash(const OScriptNetKey& p_key) {
        uint32_t h = 5381;
        h = hash_murmur3_one_64(p_key.node, h);
        h = hash_murmur3_one_64(p_key.pin, h);
        return h;
    }
};

typedef HashSet<OScriptNodePinId, OScriptNodePinIdHasher> OScriptNodePinSet;

struct OScriptFunctionInfo {
    template <typename T>
    using OScriptNetKeyMap = HashMap<OScriptNetKey, T, OScriptNetKeyHasher>;

    NodeId entry_node_id = -1;

    // All nodes in the graph
    HashSet<NodeId> graph_nodes;

    // Control flow
    HashMap<NodeId, NodeId> loop_body_start_nodes;                  //! Loop Node Body First Node Id
    HashMap<NodeId, StringName> loop_break_variables;               //! Per loop's break variable name
    HashMap<NodeId, OScriptNodePinSet> loop_break_sources;          //! Node ids that feed back into break pins
    OScriptNodePinSet loop_break_targets;                           //! A set of all break pin targets
    HashMap<NodeId, bool> is_loop_node;                             //! Quick lookup if node id is a loop
    HashMap<NodeId, HashSet<NodeId>> node_to_enclosing_loop;        //! Only includes control flow nodes to loop node
    HashMap<NodeId, HashSet<NodeId>> nodes_in_loop_body;            //! Only includes loop node to list of control nodes
    HashMap<NodeId, bool> has_nested_loops;
    HashMap<NodeId, bool> is_branch_node;                           //! quick lookup if node id is a branch
    HashSet<NodeId> unreachable_nodes;                              //! Nodes with no incoming control flow
    HashSet<NodeId> dead_end_nodes;                                 //! Nodes with no outgoing control flow
    HashMap<NodeId, NodeId> incoming_control_flow_count;            //! merge point detection
    HashMap<NodeId, StringName> local_variables;                    //! Local function-scoped variable declarations

    enum class DivergenceType {
        ConditionalBranch,      //! Branch if/else
        TypeCast,               //! Successful cast or failed
        Switch,                 //! Switch statements
        LoopBreak,              //! Loop body versus break path
    };

    HashMap<NodeId, DivergenceType> node_divergence_type;           //! NodeId -> what kind of divergence it is

    // Maintains a collection of node mappings where the key represents where the path diverges and the value
    // represents the closest node in the graph where the paths converge.
    HashMap<NodeId, NodeId> divergence_to_merge_point;              //! NodeId -> where paths reconverge
    HashMap<NodeId, OScriptNodePinId> divergence_to_merge_pins;

    // Maintains a collection of path start nodes for any node that has multiple control flow outputs.
    // For example, a branch node would have one or two nodes in the value depending on whether the
    // convergence happened immediately or if there are two unique paths prior to convergence.
    HashMap<NodeId, HashSet<NodeId>> divergence_paths;

    // Data Flow
    HashMap<NodeId, HashSet<NodeId>> node_data_dependencies;        //! Set of node ids that the key depends on
    HashMap<NodeId, bool> has_data_dependencies;                    //! Lookup if a given node has data dependencies

    // Net Registration
    OScriptNetKeyMap<StringName> net_variable_allocation;
    OScriptNetKeyMap<HashSet<NodeId>> net_consumers;
    OScriptNetKeyMap<OScriptNetKey> net_pin_consumers;
    OScriptNetKeyMap<NodeId> net_producers;

    // Execution
    Vector<NodeId> linear_execution_list;

    _FORCE_INLINE_ bool is_break_source(NodeId p_node, PinId p_pin, NodeId p_loop) const {
        return loop_break_sources.has(p_loop) ? loop_break_sources[p_loop].has({ p_node , p_pin }) : false;
    }

    _FORCE_INLINE_ bool is_reachable(NodeId p_node) const {
        return !unreachable_nodes.has(p_node);
    }

    // _FORCE_INLINE_ StringName get_variable_for_net(const OScriptNetKey& p_id) const {
    //     return net_variable_allocation.has(p_id) ? net_variable_allocation[p_id] : "@temp";
    // }

    _FORCE_INLINE_ const HashSet<NodeId>* get_net_consumers(const OScriptNetKey& p_id) const {
        return net_consumers.getptr(p_id);
    }

    _FORCE_INLINE_ NodeId get_net_producer(const OScriptNetKey& p_id) const {
        const OScriptNetKeyMap<NodeId>::ConstIterator E = net_producers.find(p_id);
        return E ? E->value : -1;
    }

    String to_string();
};

/// This class performs a pre-pass analysis on the <code>OScriptFunction</code> graph.
///
/// The purpose of this class is to populate an <code>OScriptFunctionInfo</code> object, that holds
/// pre-pass metadata about the <code>OScriptFunction</code> graph traversal.
///
class OScriptFunctionAnalyzer {
public:
    struct AnalyzerWarning {
        NodeId node;
        String message;
    };

    struct AnalyzerError {
        NodeId node;
        String message;
    };

private:
    struct Context {
        uint64_t next_net_id = 1;
        Ref<OScriptFunction> function;
        Ref<OScriptNode> entry_node;
        OScriptFunctionInfo info;

        Ref<OScriptNode> get_node_by_id(NodeId p_node_id);
        uint64_t get_next_net_id() { return next_net_id++; }
    };

    List<AnalyzerWarning> warnings;
    List<AnalyzerError> errors;

    void _build_linear_execution_list(Context& p_context, bool p_data_dependencies = false);
    void _register_incoming_nets(Context& p_context, const Ref<OScriptNodePin>& p_pin, NodeId p_node_id);
    void _register_nets(Context& p_context);

    void _populate_divergence_paths(Context& p_context, NodeId p_divergence_node_id);
    void _find_merge_point(Context& p_context, NodeId p_divergence_node_id);
    void _find_merge_point_by_pin(Context& p_context, NodeId p_divergence_node_id);
    HashSet<NodeId> _get_all_reachable_nodes(Context& p_context, NodeId p_from_node_id);
    OScriptNodePinSet _get_all_reachable_pins(Context& p_context, const OScriptNodePinId& p_id);
    void _collect_data_dependencies(const Ref<OScriptNode>& p_node, OScriptNodePinSet& p_dependencies);
    void _collect_data_dependencies(const Ref<OScriptNode>& p_node, HashSet<NodeId>& p_dependencies);

    void _collect_graph_nodes(Context& p_context);
    void _collect_node_types(Context& p_context);
    void _analyze_loop_breaks(Context& p_context);
    void _analyze_data_dependencies(Context& p_context);
    void _analyze_nesting(Context& p_context);
    void _detect_control_flow_issues(Context& p_context);
    void _detect_divergence_points(Context& p_context);
    void _validate(const Context& p_context);

public:
    OScriptFunctionInfo analyze_function(const Ref<OScriptFunction>& p_function);

    bool has_warnings() const { return !warnings.is_empty(); }
    const List<AnalyzerWarning>& get_warnings() const { return warnings; }

    bool has_errors() const { return !errors.is_empty(); }
    const List<AnalyzerError>& get_errors() const { return errors; }
};

#endif // ORCHESTRATOR_SCRIPT_PARSER_GRAPH_ANALYZER_H
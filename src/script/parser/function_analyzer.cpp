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
#include "script/parser/function_analyzer.h"

#include "common/string_utils.h"
#include "core/godot/variant/array.h"
#include "script/node.h"
#include "script/nodes/script_nodes.h"

#include <functional>

static bool is_for_loop_node(const Ref<OScriptNode>& p_node) {
    return p_node->is_type<OScriptNodeForLoop>() || p_node->is_type<OScriptNodeForEach>();
}

static bool is_entry_node(const Ref<OScriptNode>& p_node) {
    return p_node->is_type<OScriptNodeFunctionEntry>() || p_node->is_type<OScriptNodeEvent>();
}

static bool is_return_node(const Ref<OScriptNode>& p_node) {
    return p_node->is_type<OScriptNodeFunctionResult>();
}

static Vector<Ref<OScriptNode>> get_control_flow_successors(const Ref<OScriptNode>& p_node) {
    Vector<Ref<OScriptNode>> successors;
    for (const Ref<OScriptNodePin>& output : p_node->find_pins(PD_Output)) {
        if (output.is_valid() && output->is_execution() && output->has_any_connections()) {
            successors.push_back(output->get_connection()->get_owning_node());
        }
    }
    return successors;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OScriptFunctionInfo

String OScriptFunctionInfo::to_string() {
    String result = String("-").repeat(120);
    result += vformat("\nEntry Node Id: %d", entry_node_id);
    result += vformat("\nGraph Nodes: %s", StringUtils::join(", ", GDE::Array::from_container(graph_nodes)));

    result += "\nLoop Body Start Nodes:";
    for (const KeyValue<NodeId, NodeId>& E : loop_body_start_nodes) {
        result += vformat("\n\t%d: %d", E.key, E.value);
    }

    result += "\nLoop Break Sources:";
    for (const KeyValue<NodeId, OScriptNodePinSet>& E : loop_break_sources) {
        String node_pin_set = "[";
        for (OScriptNodePinSet::Iterator I = E.value.begin(); I != E.value.end();) {
            node_pin_set += vformat("{ %d, %d }", I->node, I->pin);
            ++I;
            if (I != E.value.end()) {
                node_pin_set += ", ";
            }
        }
        node_pin_set += "]";
        result += vformat("\n\t%d: %s", E.key, node_pin_set);
    }

    result += "\nLoop Break Variables:";
    for (const KeyValue<NodeId, StringName>& E : loop_break_variables) {
        result += vformat("\n\t%d: %s", E.key, E.value);
    }

    result += "\nLoop Nodes:";
    for (const KeyValue<NodeId, bool>& E : is_loop_node) {
        result += vformat("\n\t%d: %s", E.key, E.value);
    }

    result += "\nNode To Enclosing Loop:";
    for (const KeyValue<NodeId, HashSet<NodeId>>& E : node_to_enclosing_loop) {
        result += vformat("\n\t%d <-> %s", E.key, StringUtils::join(", ", GDE::Array::from_container(E.value)));
    }

    result += "\nNodes In Loop Body:";
    for (const KeyValue<NodeId, HashSet<NodeId>>& E : nodes_in_loop_body) {
        result += vformat("\n\t%d: %s", E.key, StringUtils::join(", ", GDE::Array::from_container(E.value)));
    }

    result += "\nNested Loops:";
    for (const KeyValue<NodeId, bool>& E : has_nested_loops) {
        result += vformat("\n\t%d <-> %s", E.key, E.value);
    }

    result += "\nBranch Nodes:";
    for (const KeyValue<NodeId, bool>& E : is_branch_node) {
        result += vformat("\n\t%d <-> %s", E.key, E.value);
    }

    result += vformat("\nUnreachable Nodes: %s", StringUtils::join(", ", GDE::Array::from_container(unreachable_nodes)));
    result += vformat("\nDead-end Nodes: %s", StringUtils::join(", ", GDE::Array::from_container(dead_end_nodes)));

    result += "\nIncoming Control Flow Count";
    for (const KeyValue<NodeId, NodeId>& E : incoming_control_flow_count) {
        result += vformat("\n\t%d: %s", E.key, E.value);
    }

    result += "\nNode Divergence Types:";
    for (const KeyValue<NodeId, DivergenceType>& E : node_divergence_type) {
        result += vformat("\n\t%d: %s", E.key, static_cast<uint64_t>(E.value));
    }

    result += "\nNode Divergence Merge Point:";
    for (const KeyValue<NodeId, NodeId>& E : divergence_to_merge_point) {
        result += vformat("\n\t%d: %s", E.key, E.value);
    }

    result += "\nNode Divergence Merge Point Pins:";
    for (const KeyValue<NodeId, OScriptNodePinId>& E : divergence_to_merge_pins) {
        result += vformat("\n\t%d: { %d, %d }", E.key, E.value.node, E.value.pin);
    }

    result += "\nNode Divergence Paths:";
    for (const KeyValue<NodeId, HashSet<NodeId>>& E : divergence_paths) {
        result += vformat("\n\t%d: %s", E.key, StringUtils::join(", ", GDE::Array::from_container(E.value)));
    }

    result += "\nNode Data Dependencies:";
    for (const KeyValue<NodeId, HashSet<NodeId>>& E : node_data_dependencies) {
        result += vformat("\n\t%d: %s", E.key, StringUtils::join(", ", GDE::Array::from_container(E.value)));
    }

    result += "\nHas Data Dependencies:";
    for (const KeyValue<NodeId, bool>& E : has_data_dependencies) {
        result += vformat("\n\t%d <-> %s", E.key, E.value);
    }

    result += "\nLinear Execution List: " + StringUtils::join(", ", GDE::Array::from_container(linear_execution_list));

    result += "\nNet Variable Allocations:";
    for (const auto& E : net_variable_allocation) {
        result += vformat("\n\t{ %d, %d } -> %s", E.key.node, E.key.pin, E.value);
    }

    result += "\nNet Producers:";
    for (const auto& E : net_producers) {
        result += vformat("\n\t{ %d, %d } = %d", E.key.node, E.key.pin, E.value);
    }

    result += "\nNet Consumers:";
    for (const auto& E : net_consumers) {
        result += vformat("\n\t{ %d, %d } = %s", E.key.node, E.key.pin, StringUtils::join(", ", GDE::Array::from_container(E.value)));
    }

    result += "\nNet Consumers (Node/Pin Pairs):";
    for (const auto& E : net_pin_consumers) {
        result += vformat("\n\t{ %d, %d } = { %d, %d }", E.key.node, E.key.pin, E.value.node, E.value.pin);
    }

    result += "\nLocal Variables:";
    for (const auto& E : local_variables) {
        result += vformat("\n\t%d: %s", E.key, E.value);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OScriptFunctionAnalyzer

Ref<OScriptNode> OScriptFunctionAnalyzer::Context::get_node_by_id(NodeId p_node_id) {
    const Ref<OScriptGraph> graph = function->get_graph();
    if (graph.is_valid()) {
        return graph->get_node(p_node_id);
    }
    return {};
}

void OScriptFunctionAnalyzer::_build_linear_execution_list(Context& p_context) {
    HashMap<Ref<OScriptNode>, int> node_degrees;
    HashSet<Ref<OScriptNode>> all_nodes;

    for (NodeId node_id : p_context.info.graph_nodes) {
        const Ref<OScriptNode> node = p_context.get_node_by_id(node_id);
        if (node.is_valid()) {
            all_nodes.insert(node);
            node_degrees[node] = 0;
        }
    }

    // Count incoming control flow edges
    for (const Ref<OScriptNode>& node : all_nodes) {
        for (const Ref<OScriptNodePin>& input : node->find_pins(PD_Input)) {
            if (input.is_valid() && input->is_execution()) {
                node_degrees[node] += input->get_connections().size();
            }
        }
    }

    List<Ref<OScriptNode>> queue;
    for (const Ref<OScriptNode>& node : all_nodes) {
        if (node_degrees[node] == 0) {
            queue.push_back(node);
        }
    }

    while (!queue.is_empty()) {
        const Ref<OScriptNode> node = queue.front()->get();
        queue.pop_front();

        p_context.info.linear_execution_list.push_back(node->get_id());

        // Decrement for successors
        for (const Ref<OScriptNode>& successor : get_control_flow_successors(node)) {
            node_degrees[successor]--;
            if (node_degrees[successor] == 0) {
                queue.push_back(successor);
            }
        }
    }
}

void OScriptFunctionAnalyzer::_register_incoming_nets(Context& p_context, const Ref<OScriptNodePin>& p_pin, NodeId p_node_id) {
    if (!p_pin.is_valid() || !p_pin->is_input()) {
        // Can't register for invalid pins or non-inputs
        return;
    }

    for (const Ref<OScriptNodePin>& source_pin : p_pin->get_connections()) {
        const Ref<OScriptNode> source_node = source_pin->get_owning_node();
        if (!source_node.is_valid()) {
            continue;
        }

        OScriptNetKey key = { source_node->get_id(), source_pin->get_pin_index() };
        if (!p_context.info.net_variable_allocation.has(key)) {
            if (const Ref<OScriptNodeFunctionEntry>& entry = source_node; entry.is_valid()) {
                p_context.info.net_variable_allocation[key] = source_pin->get_pin_name();
            } else if (const Ref<OScriptNodeLocalVariable>& local_var = source_node; local_var.is_valid()) {
                String variable_name = local_var->get_variable_name();
                if (variable_name.is_empty()) {
                    const uint64_t net_id = p_context.get_next_net_id();
                    variable_name = vformat("@ltemp%d", net_id);
                }
                p_context.info.net_variable_allocation[key] = variable_name;

            } else if (const Ref<OScriptNodeVariable>& var = source_node; var.is_valid()) {
                p_context.info.net_variable_allocation[key] = var->get_variable()->get_variable_name();
            } else {
                const uint64_t net_id = p_context.get_next_net_id();
                p_context.info.net_variable_allocation[key] = vformat("@temp%d", net_id);
            }
        }

        // Register producer
        p_context.info.net_producers[key] = source_node->get_id();

        // Register consumer and inverse consumer mappings
        const OScriptNetKey inverse_key = { p_pin->get_owning_node()->get_id(), p_pin->get_pin_index() };
        p_context.info.net_consumers[key].insert(p_node_id);
        p_context.info.net_pin_consumers[inverse_key] = key;
    }
}

void OScriptFunctionAnalyzer::_register_nets(Context& p_context) {
    HashSet<NodeId> visited;

    std::function <void(const Ref<OScriptNode>&)> visit = [&](const Ref<OScriptNode>& current) {
        const NodeId node_id = current->get_id();
        if (visited.has(node_id)) {
            return;
        }

        visited.insert(node_id);

        for (const Ref<OScriptNodePin>& input : current->find_pins(PD_Input)) {
            if (input.is_valid() && !input->is_execution()) {
                _register_incoming_nets(p_context, input, node_id);

                // Visit nodes that feed this node for pure nodes
                for (const Ref<OScriptNodePin>& source_pin : input->get_connections()) {
                    const Ref<OScriptNode> source = source_pin->get_owning_node();
                    if (source.is_valid()) {
                        visit(source);
                    }
                }
            }
        }

        // Traverse control flow
        for (const Ref<OScriptNode>& successor : get_control_flow_successors(current)) {
            visit(successor);
        }
    };

    visit(p_context.entry_node);
}

void OScriptFunctionAnalyzer::_populate_divergence_paths(Context& p_context, NodeId p_divergence_node_id) {
    const Ref<OScriptNode> node = p_context.get_node_by_id(p_divergence_node_id);
    if (!node.is_valid()) {
        return;
    }

    // Collect all immediate successors (the diverging paths)
    const Vector<Ref<OScriptNode>> successors = get_control_flow_successors(node);
    if (successors.size() > 1) {
        for (const Ref<OScriptNode>& successor : successors) {
            p_context.info.divergence_paths[p_divergence_node_id].insert(successor->get_id());
        }
    }
}

void OScriptFunctionAnalyzer::_find_merge_point(Context& p_context, NodeId p_divergence_node_id) {
    const HashSet<NodeId>& paths = p_context.info.divergence_paths[p_divergence_node_id];
    if (paths.is_empty()) {
        // This should never happen
        return;
    }

    // Check if both paths immediately converge
    if (paths.size() == 1) {
        // During later analysis, if the convergence path has only one node, the branch logic could be
        // flattened, particularly in the case of an if/else node path
        p_context.info.divergence_to_merge_point[p_divergence_node_id] = *paths.begin();
        return;
    }

    const int k = (int)paths.size();

    if (k <= 64) {
        // Pass O(n + e)
        // Single forward pass over the pre-built topological order using bitmasks. Each bit i
        // represents "reachable from path i". The first node in topological order where all k
        // bits are set is the merge point (closest node reached by every path).
        const uint64_t all_bits = (k < 64) ? ((1ULL << k) - 1) : ~0ULL;
        HashMap<NodeId, uint64_t> reach_mask;

        int i = 0;
        for (NodeId path_start : paths) {
            uint64_t* m = reach_mask.getptr(path_start);
            if (m) {
                *m |= (1ULL << i);
            } else {
                reach_mask[path_start] = (1ULL << i);
            }
            i++;
        }

        for (NodeId node_id : p_context.info.linear_execution_list) {
            const uint64_t* mask_ptr = reach_mask.getptr(node_id);
            if (!mask_ptr || *mask_ptr == 0) {
                continue;
            }

            const uint64_t mask = *mask_ptr;
            if (mask == all_bits) {
                p_context.info.divergence_to_merge_point[p_divergence_node_id] = node_id;
                return;
            }

            const Ref<OScriptNode> node = p_context.get_node_by_id(node_id);
            if (!node.is_valid()) {
                continue;
            }

            for (const Ref<OScriptNode>& successor : get_control_flow_successors(node)) {
                const NodeId succ_id = successor->get_id();
                uint64_t* succ_mask = reach_mask.getptr(succ_id);
                if (succ_mask) {
                    *succ_mask |= mask;
                } else {
                    reach_mask[succ_id] = mask;
                }
            }
        }

        // No merge point found (e.g., one or more paths end without reconverging)
        return;
    }

    // Fallback for k > 64 paths (should virtually never happens in practice)
    HashSet<NodeId> common_reachable;
    bool first = true;

    for (NodeId path_start : paths) {
        HashSet<NodeId> reachable_from_path = _get_all_reachable_nodes(p_context, path_start);
        if (first) {
            common_reachable = reachable_from_path;
            first = false;
        } else {
            // Intersect: Keep only nodes reachable from ALL paths
            HashSet<NodeId> intersection;
            for (NodeId node_id : common_reachable) {
                if (reachable_from_path.has(node_id)) {
                    intersection.insert(node_id);
                }
            }
            common_reachable = intersection;
        }
    }

    // The merge point is the first common node (closest to the divergence)
    if (common_reachable.size() > 0) {
        NodeId merge_point = *common_reachable.begin();
        p_context.info.divergence_to_merge_point[p_divergence_node_id] = merge_point;
    }
}

void OScriptFunctionAnalyzer::_find_merge_point_by_pin(Context& p_context, NodeId p_divergence_node_id) {
    // The merge node is already known from _find_merge_point(); derive the merge pin from it
    // instead of running a separate O(k*n) pin-level DFS + intersection.
    const HashMap<NodeId, NodeId>::ConstIterator merge_it = p_context.info.divergence_to_merge_point.find(p_divergence_node_id);
    if (!merge_it) {
        return;
    }

    const NodeId merge_node_id = merge_it->value;
    const Ref<OScriptNode> merge_node = p_context.get_node_by_id(merge_node_id);
    if (!merge_node.is_valid()) {
        return;
    }

    // Check whether the merge is within the loop body or not.
    // This is useful so that the logic below can safely include or exclude loop break pins
    bool divergence_is_in_loop_body = p_context.info.nodes_in_loop_body.has(merge_node_id)
        && p_context.info.nodes_in_loop_body[merge_node_id].has(p_divergence_node_id);

    // Find the first connected execution input pin on the merge node: the convergence pin.
    // For loop nodes with a break pin, skip the break pin unless the divergence originates
    // from within the loop body (where breaking out is valid).
    for (const Ref<OScriptNodePin>& input : merge_node->find_pins(PD_Input)) {
        if (input.is_valid() && input->is_execution() && input->has_any_connections()
            && (divergence_is_in_loop_body || !merge_node->is_loop_break_pin(input))) {
            p_context.info.divergence_to_merge_pins[p_divergence_node_id] = { merge_node_id, input->get_pin_index() };
            return;
        }
    }

    // Fallback: take the first execution pin (no connections found above).
    for (const Ref<OScriptNodePin>& input : merge_node->find_pins(PD_Input)) {
        if (input.is_valid() && input->is_execution()
            && (divergence_is_in_loop_body || !merge_node->is_loop_break_pin(input))) {
            p_context.info.divergence_to_merge_pins[p_divergence_node_id] = { merge_node_id, input->get_pin_index() };
            return;
        }
    }
}

HashSet<NodeId> OScriptFunctionAnalyzer::_get_all_reachable_nodes(Context& p_context, NodeId p_from_node_id) {
    HashSet<NodeId> reachable;
    HashSet<NodeId> visited;

    std::function <void(NodeId)> DFS = [&](NodeId node_id) {
        if (visited.has(node_id)) {
            return;
        }

        visited.insert(node_id);
        reachable.insert(node_id);

        const Ref<OScriptNode> node = p_context.get_node_by_id(node_id);
        if (!node.is_valid()) {
            return;
        }

        for (const Ref<OScriptNode>& successor : get_control_flow_successors(node)) {
            DFS(successor->get_id());
        }
    };

    DFS(p_from_node_id);
    return reachable;
}

OScriptNodePinSet OScriptFunctionAnalyzer::_get_all_reachable_pins(Context& p_context, const OScriptNodePinId& p_id) {
    OScriptNodePinSet reachable;
    OScriptNodePinSet visited;

    std::function <void(OScriptNodePinId)> DFS = [&](const OScriptNodePinId& id) {
        if (visited.has(id)) {
            return;
        }

        visited.insert(id);
        reachable.insert(id);

        const Ref<OScriptNode> node = p_context.get_node_by_id(id.node);
        if (!node.is_valid()) {
            return;
        }

        for (const Ref<OScriptNodePin>& output_pin : node->find_pins(PD_Output)) {
            if (output_pin->is_execution() && output_pin->has_any_connections()) {
                const Ref<OScriptNodePin> target_pin = output_pin->get_connection();
                DFS({ target_pin->get_owning_node()->get_id(), target_pin->get_pin_index() });
            }
        }
    };

    DFS(p_id);
    return reachable;
}

void OScriptFunctionAnalyzer::_collect_data_dependencies(const Ref<OScriptNode>& p_node, OScriptNodePinSet& p_dependencies) {
    // Recursively collect all nodes this node depends on via data pins
    for (const Ref<OScriptNodePin>& input : p_node->find_pins(PD_Input)) {
        if (input.is_valid() && !input->is_execution() && input->has_any_connections()) {
            const Ref<OScriptNode> source_node = input->get_connection()->get_owning_node();
            p_dependencies.insert({ source_node->get_id(), input->get_pin_index() });
            _collect_data_dependencies(source_node, p_dependencies);
        }
    }
}

void OScriptFunctionAnalyzer::_collect_data_dependencies(const Ref<OScriptNode>& p_node, HashSet<NodeId>& p_dependencies) {
    // Recursively collect all nodes this node depends on via data pins
    for (const Ref<OScriptNodePin>& input : p_node->find_pins(PD_Input)) {
        if (input.is_valid() && !input->is_execution() && input->has_any_connections()) {
            const Ref<OScriptNode> source_node = input->get_connection()->get_owning_node();
            p_dependencies.insert(source_node->get_id());
            _collect_data_dependencies(source_node, p_dependencies);
        }
    }
}

void OScriptFunctionAnalyzer::_collect_graph_nodes(Context& p_context) {
    // Record all nodes within the function's owning graph.
    // For event graphs, this will include all nodes
    const Ref<OScriptGraph> owning_graph = p_context.function->get_graph();
    if (owning_graph.is_valid()) {
        for (const Ref<OScriptNode>& graph_node : owning_graph->get_nodes()) {
            p_context.info.graph_nodes.insert(graph_node->get_id());
        }
    }
}

void OScriptFunctionAnalyzer::_analyze_combined(Context& p_context) {
    HashSet<NodeId> visited;
    HashSet<NodeId> type_visited;
    HashSet<NodeId> reachable;
    HashMap<NodeId, uint64_t> incoming_edge_count;
    List<NodeId> loop_stack;
    OScriptFunctionInfo& info = p_context.info;

    // Classify node types and traverse backward through data pins.
    // Uses a separate visited set so the main control-flow pass can still visit all nodes.
    std::function<void(const Ref<OScriptNode>&)> visit_types = [&](const Ref<OScriptNode>& current) {
        const NodeId node_id = current->get_id();
        if (type_visited.has(node_id)) {
            return;
        }
        type_visited.insert(node_id);

        if (is_for_loop_node(current)) {
            info.is_loop_node[node_id] = true;
            const Ref<OScriptNodePin> body_pin = current->find_pin("loop_body", PD_Output);
            if (body_pin.is_valid() && body_pin->has_any_connections()) {
                const Ref<OScriptNode> target = body_pin->get_connection()->get_owning_node();
                info.loop_body_start_nodes[node_id] = target->get_id();
            }
        } else if (const Ref<OScriptNodeBranch>& branch = current; branch.is_valid()) {
            info.is_branch_node[node_id] = true;
        } else if (const Ref<OScriptNodeLocalVariable>& local_variable = current; local_variable.is_valid()) {
            info.local_variables[node_id] = local_variable->get_variable_name();
        }

        for (const Ref<OScriptNodePin>& input : current->find_pins(PD_Input)) {
            if (input.is_valid() && input->has_any_connections()) {
                for (const Ref<OScriptNodePin>& source : input->get_connections()) {
                    const Ref<OScriptNode> owner = source->get_owning_node();
                    if (owner.is_valid()) {
                        visit_types(owner);
                    }
                }
            }
        }
    };

    // Combined forward-control DFS: handles _detect_control_flow_issues, _analyze_data_dependencies,
    // _detect_divergence_points, and _analyze_nesting in a single traversal.
    std::function<void(const Ref<OScriptNode>&)> visit = [&](const Ref<OScriptNode>& current) {
        const NodeId node_id = current->get_id();

        // On revisit (back-edge within a loop body): still record loop membership for nesting.
        if (visited.has(node_id)) {
            if (loop_stack.size() > 0) {
                const NodeId enclosing_loop_id = loop_stack.back()->get();
                info.node_to_enclosing_loop[node_id].insert(enclosing_loop_id);
                info.nodes_in_loop_body[enclosing_loop_id].insert(node_id);
            }
            return;
        }

        visited.insert(node_id);
        reachable.insert(node_id);

        // --- _collect_node_types: classify this node and its data predecessors ---
        visit_types(current);

        // --- _analyze_data_dependencies ---
        _collect_data_dependencies(current, info.node_data_dependencies[node_id]);
        if (info.node_data_dependencies[node_id].size() > 0) {
            info.has_data_dependencies[node_id] = true;
        }

        // --- _detect_divergence_points ---
        if (current->is_type<OScriptNodeBranch>()) {
            info.node_divergence_type[node_id] = OScriptFunctionInfo::DivergenceType::ConditionalBranch;
            _populate_divergence_paths(p_context, node_id);
            _find_merge_point(p_context, node_id);
            _find_merge_point_by_pin(p_context, node_id);
        } else if (current->is_type<OScriptNodeTypeCast>()) {
            info.node_divergence_type[node_id] = OScriptFunctionInfo::DivergenceType::TypeCast;
            _populate_divergence_paths(p_context, node_id);
            _find_merge_point(p_context, node_id);
            _find_merge_point_by_pin(p_context, node_id);
        } else if (is_for_loop_node(current)) {
            info.node_divergence_type[node_id] = OScriptFunctionInfo::DivergenceType::LoopBreak;
            const Ref<OScriptNodePin> abort_pin = current->find_pin("aborted", PD_Output);
            if (abort_pin.is_valid()) {
                for (const Ref<OScriptNodePin>& target_pin : abort_pin->get_connections()) {
                    info.divergence_paths[node_id].insert(target_pin->get_owning_node()->get_id());
                }
            }
            const Ref<OScriptNodePin> completed_pin = current->find_pin("completed", PD_Output);
            if (completed_pin.is_valid()) {
                for (const Ref<OScriptNodePin>& target_pin : completed_pin->get_connections()) {
                    info.divergence_paths[node_id].insert(target_pin->get_owning_node()->get_id());
                }
            }
            _find_merge_point(p_context, node_id);
            _find_merge_point_by_pin(p_context, node_id);
        } else if (current->is_type<OScriptNodeChance>()) {
            info.node_divergence_type[node_id] = OScriptFunctionInfo::DivergenceType::ConditionalBranch;
            _populate_divergence_paths(p_context, node_id);
            _find_merge_point(p_context, node_id);
            _find_merge_point_by_pin(p_context, node_id);
        } else if (current->is_type<OScriptNodeSwitchEnum>() || current->is_type<OScriptNodeSwitchInteger>() || current->is_type<OScriptNodeSwitchString>()) {
            info.node_divergence_type[node_id] = OScriptFunctionInfo::DivergenceType::Switch;
            _populate_divergence_paths(p_context, node_id);
            _find_merge_point(p_context, node_id);
            _find_merge_point_by_pin(p_context, node_id);
        } else if (current->is_type<OScriptNodeRandom>()) {
            info.node_divergence_type[node_id] = OScriptFunctionInfo::DivergenceType::ConditionalBranch;
            _populate_divergence_paths(p_context, node_id);
            _find_merge_point(p_context, node_id);
            _find_merge_point_by_pin(p_context, node_id);
        } else if (current->is_type<OScriptNodeDialogueMessage>()) {
            info.node_divergence_type[node_id] = OScriptFunctionInfo::DivergenceType::ConditionalBranch;
            _populate_divergence_paths(p_context, node_id);
            _find_merge_point(p_context, node_id);
            _find_merge_point_by_pin(p_context, node_id);
        }
        // todo: what about OScriptNodeSwitch?

        // --- _analyze_nesting: record enclosing loop for this node ---
        if (loop_stack.size() > 0) {
            const NodeId enclosing_loop_id = loop_stack.back()->get();
            info.node_to_enclosing_loop[node_id].insert(enclosing_loop_id);
            info.nodes_in_loop_body[enclosing_loop_id].insert(node_id);
        }

        // --- _analyze_nesting: push loop node onto stack ---
        if (info.is_loop_node.has(node_id)) {
            if (loop_stack.size() > 0) {
                info.has_nested_loops[loop_stack.back()->get()] = true;
            }
            loop_stack.push_back(node_id);
        }

        // Compute successors once: used by dead-end detection, incoming-edge counting, and recursion.
        const Vector<Ref<OScriptNode>> successors = get_control_flow_successors(current);

        // --- _detect_control_flow_issues: dead-end detection ---
        if (successors.is_empty() && !is_entry_node(current) && !is_return_node(current)) {
            info.dead_end_nodes.insert(node_id);
        }

        for (const Ref<OScriptNode>& successor : successors) {
            // --- _detect_control_flow_issues: count incoming control-flow edges ---
            incoming_edge_count[successor->get_id()]++;

            // --- _analyze_nesting: loop nodes only recurse into their body, not break/completion ---
            if (info.is_loop_node.has(node_id)) {
                const NodeId body_start = info.loop_body_start_nodes[node_id];
                if (successor->get_id() == body_start) {
                    visit(successor);
                }
            } else {
                visit(successor);
            }
        }

        // --- _analyze_nesting: pop loop stack after all successors are visited ---
        if (info.is_loop_node.has(node_id)) {
            loop_stack.pop_back();
        }
    };

    visit(p_context.entry_node);

    // Derive unreachable nodes as the complement of the reachable set within the full graph.
    for (NodeId node_id : info.graph_nodes) {
        if (!reachable.has(node_id)) {
            info.unreachable_nodes.insert(node_id);
        }
    }

    // Store incoming-edge counts only for actual merge points (more than one incoming edge).
    for (const KeyValue<NodeId, uint64_t>& E : incoming_edge_count) {
        if (E.value > 1) {
            info.incoming_control_flow_count[E.key] = E.value;
        }
    }
}

void OScriptFunctionAnalyzer::_analyze_loop_breaks(Context& p_context) {
    HashSet<NodeId> visited;
    OScriptFunctionInfo& info = p_context.info;

    std::function <void(const Ref<OScriptNode>&)> visit = [&](const Ref<OScriptNode>& current) {
        const NodeId node_id = current->get_id();
        if (visited.has(node_id)) {
            return;
        }

        visited.insert(node_id);

        if (is_for_loop_node(current)) {
            NodeId loop_id = node_id;

            // Find all nodes that connect to break pin
            Ref<OScriptNodePin> break_pin = current->find_pin("break", PD_Input);
            if (break_pin.is_valid()) {
                info.loop_break_targets.insert({ loop_id, break_pin->get_pin_index() });

                const Vector<Ref<OScriptNodePin>> break_inputs = break_pin->get_connections();
                for (const Ref<OScriptNodePin>& input : break_inputs) {
                    const Ref<OScriptNode> input_node = input->get_owning_node();
                    if (info.unreachable_nodes.has(input_node->get_id())) {
                        errors.push_back({ node_id, vformat("Node %d connects to for loop %d break pint but it isn't reachable.", input_node->get_id(), node_id) });
                    } else {
                        if (!info.loop_break_variables.has(loop_id)) {
                            info.loop_break_variables[loop_id] = vformat("for_loop_%d_break", loop_id);
                        }

                        info.loop_break_sources[loop_id].insert({ input_node->get_id(), input->get_pin_index() });
                        // Also collect all data dependencies of the break source
                        _collect_data_dependencies(input_node, info.loop_break_sources[loop_id]);
                    }
                }
            }
        }

        for (const Ref<OScriptNode>& successor : get_control_flow_successors(current)) {
            visit(successor);
        }
    };

    visit(p_context.entry_node);
}

void OScriptFunctionAnalyzer::_validate(const Context& p_context) {
    warnings.clear();
    errors.clear();

    const String function_name = p_context.function->get_function_name();
    const OScriptFunctionInfo& info = p_context.info;

    if (!info.unreachable_nodes.is_empty()) {
        warnings.push_back({ -1, vformat("Function %s has %d unreachable nodes", function_name, info.unreachable_nodes.size()) });
        for (NodeId node_id : info.unreachable_nodes) {
            warnings.push_back({ node_id, vformat("Node %d cannot be reached", node_id) });
        }
    }

    if (!info.dead_end_nodes.is_empty()) {
        warnings.push_back({ -1, vformat("Function %s has %d dead-end nodes", function_name, info.dead_end_nodes.size()) });
        for (NodeId node_id : info.dead_end_nodes) {
            warnings.push_back({ node_id, vformat("Node %d is considered a dead-end", node_id) });
        }
    }

    for (const KeyValue<NodeId, OScriptNodePinSet>& E : info.loop_break_sources) {
        for (OScriptNodePinId break_node_id : E.value) {
            if (!info.nodes_in_loop_body[E.key].has(break_node_id.node)) {
                errors.push_back({ break_node_id.node, vformat("Break node %d is outside loop body", break_node_id.node) });
            }
        }
    }
}

OScriptFunctionInfo OScriptFunctionAnalyzer::analyze_function(const Ref<OScriptFunction>& p_function) {
    // Setup analysis context
    Context context;
    context.function = p_function;
    context.entry_node = p_function->get_owning_node();
    context.info.entry_node_id = context.entry_node->get_id();

    _collect_graph_nodes(context);
    _build_linear_execution_list(context);

    // Combined single-pass analysis: node types, control-flow issues, data dependencies,
    // divergence points, and nesting are all resolved in one DFS traversal.
    _analyze_combined(context);
    // Loop-break analysis runs after the combined pass because it needs unreachable_nodes.
    _analyze_loop_breaks(context);
    _validate(context);

    // Generate variable allocation network analysis
    _register_nets(context);

    return context.info;
}

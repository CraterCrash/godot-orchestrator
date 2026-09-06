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
#include "editor/graph/graph_clipboard.h"

#include "common/dictionary_utils.h"
#include "common/method_utils.h"
#include "common/property_utils.h"
#include "orchestration/nodes/call_function.h"
#include "orchestration/nodes/comment.h"
#include "orchestration/nodes/emit_signal.h"
#include "orchestration/nodes/event.h"
#include "orchestration/nodes/operator_node.h"
#include "orchestration/nodes/variables.h"
#include "orchestration/orchestration.h"

OrchestratorEditorGraphClipboard::Buffer OrchestratorEditorGraphClipboard::_buffer;

bool OrchestratorEditorGraphClipboard::Buffer::is_empty() const {
    return nodes.is_empty();
}

void OrchestratorEditorGraphClipboard::Buffer::clear() {
    nodes.clear();
    connections.clear();
    variables.clear();
    functions.clear();
    events.clear();
    signals.clear();
}

bool OrchestratorEditorGraphClipboard::ClipboardResult::had_skipped_nodes() const {
    return !skipped_functions.is_empty() || !skipped_events.is_empty() || !skipped_variables.is_empty() || !skipped_signals.is_empty();
}

OrchestratorEditorGraphClipboard::ClipboardResult OrchestratorEditorGraphClipboard::copy(
    const Vector<Ref<OrchestrationGraphNode>>& p_nodes, const Ref<OrchestrationGraph>& p_source) {

    clear();

    ClipboardResult result;
    HashSet<uint64_t> node_ids;
    for (const Ref<OrchestrationGraphNode>& script_node : p_nodes) {
        ERR_CONTINUE(script_node.is_null());

        // A node whose referenced function, variable, or signal no longer resolves cannot be reproduced on paste,
        // so it is left out of the buffer rather than dereferenced.
        Ref<OScriptFunction> event_function;
        Ref<OScriptFunction> called_function;
        Ref<OScriptVariable> variable;
        Ref<OScriptSignal> signal;

        if (const Ref<OScriptNodeEvent>& event = script_node; event.is_valid()) {
            event_function = event->get_function();
            ERR_CONTINUE_MSG(event_function.is_null(), vformat("Cannot copy event node %d; its function no longer exists.", script_node->get_id()));
        } else if (const Ref<OScriptNodeCallScriptFunction>& call_script = script_node; call_script.is_valid()) {
            called_function = call_script->get_function();
            ERR_CONTINUE_MSG(called_function.is_null(), vformat("Cannot copy call function node %d; its function no longer exists.", script_node->get_id()));
        }

        if (const Ref<OScriptNodeVariable>& variable_node = script_node; variable_node.is_valid()) {
            variable = variable_node->get_variable();
            ERR_CONTINUE_MSG(variable.is_null(), vformat("Cannot copy variable node %d; its variable no longer exists.", script_node->get_id()));
        }

        if (const Ref<OScriptNodeEmitSignal>& signal_node = script_node; signal_node.is_valid()) {
            signal = signal_node->get_signal();
            ERR_CONTINUE_MSG(signal.is_null(), vformat("Cannot copy emit signal node %d; its signal no longer exists.", script_node->get_id()));
        }

        CopyItem item;
        item.id = script_node->get_id();
        item.position = script_node->get_position();
        item.node = p_source->copy_node(item.id, true);

        node_ids.insert(item.id);
        result.added_nodes.insert(item.id);
        _buffer.nodes.push_back(item);

        if (event_function.is_valid()) {
            _buffer.events[event_function->get_function_name()] = event_function->duplicate();
        }

        if (called_function.is_valid()) {
            _buffer.functions[called_function->get_function_name()] = called_function->duplicate();
        }

        if (variable.is_valid()) {
            _buffer.variables[variable->get_variable_name()] = variable->duplicate();
        }

        if (signal.is_valid()) {
            _buffer.signals[signal->get_signal_name()] = signal->duplicate();
        }
    }

    for (const OScriptConnection& C : p_source->get_orchestration()->get_connections()) {
        if (node_ids.has(C.from_node) && node_ids.has(C.to_node)) {
            _buffer.connections.push_back(C.id);
        }
    }

    return result;
}

OrchestratorEditorGraphClipboard::ClipboardResult OrchestratorEditorGraphClipboard::paste(
    const Ref<OrchestrationGraph>& p_target, const Vector2& p_offset, bool p_snapping_enabled, int p_snapping_distance) {

    ClipboardResult result;

    // Pass 1 - Verify Functions
    for (const KeyValue<StringName, Ref<OScriptFunction>>& E : _buffer.functions) {
        const Ref<OScriptFunction> target_function = p_target->get_orchestration()->find_function(E.key);
        if (!target_function.is_valid()) {
            const Ref<OScriptFunction> function = p_target->get_orchestration()->create_function(
                E.value->get_method_info(), E.value->is_user_defined());
            if (!function.is_valid()) {
                result.skipped_functions[E.key] = "Failed to create function.";
            }
        } else if (!MethodUtils::has_same_signature(E.value->get_method_info(), target_function->get_method_info())) {
            result.skipped_functions[E.key] = "Function signatures do not match.";
        }
    }

    // Pass 2 - Verify Events
    for (const KeyValue<StringName, Ref<OScriptFunction>>& E : _buffer.events) {
        const Ref<OScriptFunction> target_function = p_target->get_orchestration()->find_function(E.key);
        if (target_function.is_valid()) {
            if (!MethodUtils::has_same_signature(E.value->get_method_info(), target_function->get_method_info())) {
                result.skipped_events[E.key] = "Event function signatures do not match.";
            }
        }
    }

    // Pass 3 - Create missing variables
    for (const KeyValue<StringName, Ref<OScriptVariable>>& E : _buffer.variables) {
        const Ref<OScriptVariable> target_variable = p_target->get_orchestration()->get_variable(E.key);
        if (target_variable.is_null()) {
            const Ref<OScriptVariable> variable = p_target->get_orchestration()->create_variable(E.key);
            ERR_CONTINUE(!variable.is_valid());
            variable->copy_persistent_state(E.value);
        } else if (!PropertyUtils::are_equal(E.value->get_info(), target_variable->get_info())) {
            result.skipped_variables[E.key] = "Variable declarations do not match.";
        }
    }

    // Pass 4 - Create missing signals
    for (const KeyValue<StringName, Ref<OScriptSignal>>& E : _buffer.signals) {
        const Ref<OScriptSignal> target_signal = p_target->get_orchestration()->find_custom_signal(E.key);
        if (target_signal.is_null()) {
            const Ref<OScriptSignal> signal = p_target->get_orchestration()->create_custom_signal(E.key);
            ERR_CONTINUE(!signal.is_valid());
            signal->copy_persistent_state(E.value);
        } else if (!MethodUtils::has_same_signature(E.value->get_method_info(), target_signal->get_method_info())) {
            result.skipped_signals[E.key] = "Signal signatures do not match.";
        }
    }

    // Pass 5 - Compute Paste Offset
    Vector2 offset = p_offset;
    if (!_buffer.nodes.is_empty()) {
        offset -= _buffer.nodes.get(0).position;
    }

    if (p_snapping_enabled) {
        offset = offset.snapped(Vector2(p_snapping_distance, p_snapping_distance));
    }

    // Pass 6 - Create Nodes
    HashMap<uint64_t, uint64_t> connection_remap;
    for (const CopyItem& item : _buffer.nodes) {

        Ref<OrchestrationGraphNode> new_node;
        const Ref<OrchestrationGraphNode>& node = item.node;

        const Ref<OScriptNodeEvent> event = node;
        if (event.is_valid()) {
            const Ref<OScriptFunction> event_function = event->get_function();
            if (!event_function.is_valid() || result.skipped_events.has(event_function->get_function_name())) {
                continue;
            }

            // Event nodes can only be placed inside event graphs
            if (!p_target->get_flags().has_flag(OrchestrationGraph::GF_EVENT)) {
                result.skipped_events[event_function->get_function_name()] = "Cannot paste event nodes into non-event graphs.";
                continue;
            }

            const Ref<OScriptFunction> target_function = p_target->get_orchestration()->find_function(event_function->get_function_name());
            if (target_function.is_valid()) {
                result.skipped_events[event_function->get_function_name()] = "An event node already exists with the same name.";
                continue;
            }

            OScriptNodeInitContext context;
            context.method = event_function->get_method_info();
            context.user_data = DictionaryUtils::of({ { "user_defined", event_function->is_user_defined() } });

            // This creates the node and its matching event function signature
            // Event nodes require this special handling versus the pasting.
            new_node = p_target->create_node<OScriptNodeEvent>(context, item.position + offset);

        } else {
            // Call-script-function nodes need their function GUID remapped to the target orchestration.
            // If the function doesn't exist (or has a different signature) in the target, skip the node.
            const Ref<OScriptNodeCallScriptFunction> call_script_func = node;
            if (call_script_func.is_valid()) {
                const Ref<OScriptFunction> called_function = call_script_func->get_function();
                if (called_function.is_null()) {
                    continue;
                }

                const StringName function_name = called_function->get_function_name();
                if (result.skipped_functions.has(function_name)) {
                    continue;
                }

                const Ref<OScriptFunction> target_function = p_target->get_orchestration()->find_function(function_name);
                if (!target_function.is_valid()) {
                    continue;
                }

                call_script_func->set("guid", target_function->get_guid().to_string());
            }

            // Variable and signal nodes whose referenced resource was incompatible are skipped.
            const Ref<OScriptNodeVariable> variable_node = node;
            if (variable_node.is_valid()) {
                const Ref<OScriptVariable> variable = variable_node->get_variable();
                if (variable.is_null() || result.skipped_variables.has(variable->get_variable_name())) {
                    continue;
                }
            }

            const Ref<OScriptNodeEmitSignal> signal_node = node;
            if (signal_node.is_valid()) {
                const Ref<OScriptSignal> signal = signal_node->get_signal();
                if (signal.is_null() || result.skipped_signals.has(signal->get_signal_name())) {
                    continue;
                }
            }

            new_node = p_target->paste_node(node, item.position + offset);
        }

        ERR_CONTINUE(!new_node.is_valid());
        connection_remap[item.id] = new_node->get_id();
        result.added_nodes.insert(new_node->get_id());
    }

    // Pass 7 - Create connections
    for (const uint64_t id : _buffer.connections) {
        const OScriptConnection C(id);
        const uint64_t source_node = connection_remap[C.from_node];
        const uint64_t target_node = connection_remap[C.to_node];

        if (result.added_nodes.has(source_node) && result.added_nodes.has(target_node)) {
            p_target->link(source_node, C.from_port, target_node, C.to_port);
        }
    }

    // Pass 8 - Restore pin types if pasted PromotableOperator nodes have connections
    for (const CopyItem& item : _buffer.nodes) {
        const int new_node_id = connection_remap[item.node->get_id()];
        const Ref<OrchestrationGraphNode> new_node = p_target->get_orchestration()->get_node(new_node_id);
        OScriptNodePromotableOperator::copy_pin_types(item.node, new_node);
    }

    // Pass 9 - Pasted comments still reference the copied nodes' original ids
    _remap_comment_attachments(p_target, result.added_nodes, connection_remap);

    return result;
}

OrchestratorEditorGraphClipboard::ClipboardResult OrchestratorEditorGraphClipboard::duplicate(
    const Vector<Ref<OrchestrationGraphNode>>& p_nodes, const Ref<OrchestrationGraph>& p_graph, const Vector2& p_offset) {

    ClipboardResult result;
    HashMap<uint64_t, uint64_t> connection_remap;

    for (const Ref<OrchestrationGraphNode>& node : p_nodes) {
        ERR_CONTINUE(node.is_null());

        const Ref<OrchestrationGraphNode> new_node = p_graph->duplicate_node(node->get_id(), p_offset, true);
        ERR_CONTINUE(!new_node.is_valid());

        connection_remap[node->get_id()] = new_node->get_id();
        result.added_nodes.insert(new_node->get_id());
    }

    for (const OScriptConnection& C : p_graph->get_orchestration()->get_connections()) {
        if (connection_remap.has(C.from_node) && connection_remap.has(C.to_node)) {
            uint64_t source_node = connection_remap[C.from_node];
            uint64_t target_node = connection_remap[C.to_node];
            p_graph->link(source_node, C.from_port, target_node, C.to_port);
        }
    }

    // Makes sure that if a PromotableOperator node has any connections that are duplicated, the pin types
    // from the original node are restored.
    for (const Ref<OrchestrationGraphNode>& node : p_nodes) {
        if (node.is_null() || !connection_remap.has(node->get_id())) {
            continue;
        }

        const int new_node_id = connection_remap[node->get_id()];
        const Ref<OrchestrationGraphNode> new_node = p_graph->get_orchestration()->get_node(new_node_id);
        OScriptNodePromotableOperator::copy_pin_types(node, new_node);
    }

    _remap_comment_attachments(p_graph, result.added_nodes, connection_remap);

    return result;
}

void OrchestratorEditorGraphClipboard::clear() {
    _buffer.clear();
}

void OrchestratorEditorGraphClipboard::_remap_comment_attachments(const Ref<OrchestrationGraph>& p_graph,
    const HashSet<uint64_t>& p_node_ids, const HashMap<uint64_t, uint64_t>& p_remap) {

    for (const uint64_t node_id : p_node_ids) {
        const Ref<OScriptNodeComment> comment = p_graph->get_orchestration()->get_node(node_id);
        if (comment.is_valid()) {
            comment->remap_attached_nodes(p_remap);
        }
    }
}
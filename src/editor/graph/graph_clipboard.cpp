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
#include "common/resource_utils.h"
#include "common/version.h"
#include "orchestration/nodes/call_function.h"
#include "orchestration/nodes/comment.h"
#include "orchestration/nodes/emit_signal.h"
#include "orchestration/nodes/event.h"
#include "orchestration/nodes/operator_node.h"
#include "orchestration/nodes/variables.h"
#include "orchestration/orchestration.h"
#include "orchestration/serialization/format.h"

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace {
    constexpr const char* PAYLOAD_MAGIC = "orchestrator/clipboard";
    constexpr const char* PAYLOAD_HEADER = "; orchestrator/clipboard";
}

OrchestratorEditorGraphClipboard::Buffer* OrchestratorEditorGraphClipboard::_buffer = nullptr;

bool OrchestratorEditorGraphClipboard::ClipboardResult::had_skipped_nodes() const {
    return !skipped_functions.is_empty() || !skipped_events.is_empty() || !skipped_variables.is_empty() || !skipped_signals.is_empty();
}

void OrchestratorEditorGraphClipboard::_write_payload(const Dictionary& p_payload) {
    if (!_buffer) {
        _buffer = memnew(Buffer);
    }

    _buffer->payload = p_payload;
    _buffer->text = String(PAYLOAD_HEADER) + "\n" + UtilityFunctions::var_to_str(p_payload);

    if (DisplayServer* display_server = DisplayServer::get_singleton()) {
        display_server->clipboard_set(_buffer->text);
    }
}

bool OrchestratorEditorGraphClipboard::_read_payload(Dictionary& r_payload) {
    DisplayServer* display_server = DisplayServer::get_singleton();
    const String text = display_server ? display_server->clipboard_get() : String();

    // Nothing on the OS clipboard, or no OS clipboard at all, falls back to what this editor last copied
    if (text.is_empty()) {
        if (_buffer && !_buffer->payload.is_empty()) {
            r_payload = _buffer->payload;
            return true;
        }
        return false;
    }

    // Something other than a payload is on the clipboard, there is nothing to paste
    if (!text.begins_with(PAYLOAD_HEADER)) {
        return false;
    }

    // Text this editor produced needs no parsing
    if (_buffer && text == _buffer->text) {
        r_payload = _buffer->payload;
        return true;
    }

    // The header is a parser comment, so the text is parsed as a whole
    const Variant parsed = UtilityFunctions::str_to_var(text);
    ERR_FAIL_COND_V_MSG(parsed.get_type() != Variant::DICTIONARY, false, "The clipboard payload could not be parsed.");

    const Dictionary payload = parsed;
    ERR_FAIL_COND_V_MSG(String(payload.get("magic", String())) != PAYLOAD_MAGIC, false, "The clipboard text is not an Orchestrator payload.");

    const Dictionary graph = payload.get("graph", Dictionary());
    const uint32_t format = graph.get("format", OrchestrationFormat::FORMAT_VERSION);
    ERR_FAIL_COND_V_MSG(format > OrchestrationFormat::FORMAT_VERSION, false,
        vformat("The clipboard payload was created by a newer version of Orchestrator (%s).", String(payload.get("plugin", String()))));

    r_payload = payload;
    return true;
}

void OrchestratorEditorGraphClipboard::_apply_properties(const Ref<Resource>& p_resource, const Dictionary& p_properties, const Vector<StringName>& p_excluded) {
    const Array keys = p_properties.keys();
    for (int i = 0; i < keys.size(); i++) {
        const StringName key = keys[i];
        if (!p_excluded.has(key)) {
            p_resource->set(key, p_properties[key]);
        }
    }
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

OrchestratorEditorGraphClipboard::ClipboardResult OrchestratorEditorGraphClipboard::copy(
    const Vector<Ref<OrchestrationGraphNode>>& p_nodes, const Ref<OrchestrationGraph>& p_source) {

    clear();

    ClipboardResult result;
    Dictionary functions;
    Dictionary events;
    Dictionary variables;
    Dictionary signals;
    Vector<int> node_ids;

    for (const Ref<OrchestrationGraphNode>& script_node : p_nodes) {
        ERR_CONTINUE(script_node.is_null());

        // A node whose referenced function, variable, or signal no longer resolves cannot be reproduced on paste,
        // so it is left out of the payload rather than dereferenced.
        if (const Ref<OScriptNodeEvent>& event = script_node; event.is_valid()) {
            const Ref<OScriptFunction> function = event->get_function();
            ERR_CONTINUE_MSG(function.is_null(), vformat("Cannot copy event node %d; its function no longer exists.", script_node->get_id()));

            // The persisted guid links the exported node back to this declaration on paste
            events[function->get_function_name()] = ResourceUtils::get_storage_properties(function);
        } else if (const Ref<OScriptNodeCallScriptFunction>& call_script = script_node; call_script.is_valid()) {
            const Ref<OScriptFunction> function = call_script->get_function();
            ERR_CONTINUE_MSG(function.is_null(), vformat("Cannot copy call function node %d; its function no longer exists.", script_node->get_id()));

            functions[function->get_function_name()] = ResourceUtils::get_storage_properties(function);
        }

        if (const Ref<OScriptNodeVariable>& variable_node = script_node; variable_node.is_valid()) {
            const Ref<OScriptVariable> variable = variable_node->get_variable();
            ERR_CONTINUE_MSG(variable.is_null(), vformat("Cannot copy variable node %d; its variable no longer exists.", script_node->get_id()));

            variables[variable->get_variable_name()] = ResourceUtils::get_storage_properties(variable);
        }

        if (const Ref<OScriptNodeEmitSignal>& signal_node = script_node; signal_node.is_valid()) {
            const Ref<OScriptSignal> signal = signal_node->get_signal();
            ERR_CONTINUE_MSG(signal.is_null(), vformat("Cannot copy emit signal node %d; its signal no longer exists.", script_node->get_id()));

            signals[signal->get_signal_name()] = ResourceUtils::get_storage_properties(signal);
        }

        node_ids.push_back(script_node->get_id());
        result.added_nodes.insert(script_node->get_id());
    }

    Dictionary payload;
    payload["magic"] = PAYLOAD_MAGIC;
    payload["plugin"] = VERSION_FULL_BUILD;
    payload["graph"] = p_source->export_nodes(node_ids);
    payload["functions"] = functions;
    payload["events"] = events;
    payload["variables"] = variables;
    payload["signals"] = signals;

    _write_payload(payload);

    return result;
}

OrchestratorEditorGraphClipboard::ClipboardResult OrchestratorEditorGraphClipboard::paste(
    const Ref<OrchestrationGraph>& p_target, const Vector2& p_offset, bool p_snapping_enabled, int p_snapping_distance) {

    ClipboardResult result;

    Dictionary payload;
    if (!_read_payload(payload)) {
        return result;
    }

    Orchestration* orchestration = p_target->get_orchestration();
    ERR_FAIL_NULL_V(orchestration, result);

    // Properties that identify a resource within its source orchestration, or that the target
    // consumes when it creates the resource. Everything else in a declaration is applied as-is.
    const Vector<StringName> function_identity = { "guid", "id", "method", "user_defined" };
    const Vector<StringName> variable_identity = { "name" };
    const Vector<StringName> signal_identity = { "signal_name" };

    // Pass 1 - Verify Functions
    const Dictionary functions = payload.get("functions", Dictionary());
    const Array function_names = functions.keys();
    for (int i = 0; i < function_names.size(); i++) {
        const StringName name = function_names[i];
        const Dictionary declaration = functions[name];
        const MethodInfo method = DictionaryUtils::to_method(declaration.get("method", Dictionary()));

        const Ref<OScriptFunction> target_function = orchestration->find_function(name);
        if (!target_function.is_valid()) {
            const bool user_defined = declaration.get("user_defined", false);
            const Ref<OScriptFunction> function = orchestration->create_function(method, user_defined);
            if (!function.is_valid()) {
                result.skipped_functions[name] = "Failed to create function.";
            } else {
                _apply_properties(function, declaration, function_identity);
            }
        } else if (!MethodUtils::has_same_signature(method, target_function->get_method_info())) {
            result.skipped_functions[name] = "Function signatures do not match.";
        }
    }

    // Pass 2 - Verify Events
    HashMap<String, StringName> event_names;
    const Dictionary events = payload.get("events", Dictionary());
    const Array event_keys = events.keys();
    for (int i = 0; i < event_keys.size(); i++) {
        const StringName name = event_keys[i];
        const Dictionary declaration = events[name];
        event_names[declaration.get("guid", String())] = name;

        const Ref<OScriptFunction> target_function = orchestration->find_function(name);
        if (target_function.is_valid()) {
            const MethodInfo method = DictionaryUtils::to_method(declaration.get("method", Dictionary()));
            if (!MethodUtils::has_same_signature(method, target_function->get_method_info())) {
                result.skipped_events[name] = "Event function signatures do not match.";
            }
        }
    }

    // Pass 3 - Create missing variables
    const Dictionary variables = payload.get("variables", Dictionary());
    const Array variable_names = variables.keys();
    for (int i = 0; i < variable_names.size(); i++) {
        const StringName name = variable_names[i];
        const Dictionary properties = variables[name];

        const Ref<OScriptVariable> target_variable = orchestration->get_variable(name);
        if (target_variable.is_null()) {
            const Ref<OScriptVariable> variable = orchestration->create_variable(name);
            ERR_CONTINUE(!variable.is_valid());
            _apply_properties(variable, properties, variable_identity);
        } else if (!PropertyUtils::are_equal(DictionaryUtils::to_property(properties.get("info", Dictionary())), target_variable->get_info())) {
            result.skipped_variables[name] = "Variable declarations do not match.";
        }
    }

    // Pass 4 - Create missing signals
    const Dictionary signals = payload.get("signals", Dictionary());
    const Array signal_names = signals.keys();
    for (int i = 0; i < signal_names.size(); i++) {
        const StringName name = signal_names[i];
        const Dictionary properties = signals[name];

        const Ref<OScriptSignal> target_signal = orchestration->find_custom_signal(name);
        if (target_signal.is_null()) {
            const Ref<OScriptSignal> signal = orchestration->create_custom_signal(name);
            ERR_CONTINUE(!signal.is_valid());
            _apply_properties(signal, properties, signal_identity);
        } else if (!MethodUtils::has_same_signature(DictionaryUtils::to_method(properties.get("method", Dictionary())), target_signal->get_method_info())) {
            result.skipped_signals[name] = "Signal signatures do not match.";
        }
    }

    // Pass 5 - Compute Paste Offset
    // The graph data is worked on as a deep copy, the reference rewrites below must not alter the payload
    const Dictionary graph = Dictionary(payload.get("graph", Dictionary())).duplicate(true);
    const Array entries = graph.get("nodes", Array());

    Vector2 offset = p_offset;
    if (!entries.is_empty()) {
        const Dictionary first = entries[0];
        const Dictionary properties = first.get("properties", Dictionary());
        offset -= Vector2(properties.get("position", Vector2()));
    }

    if (p_snapping_enabled) {
        offset = offset.snapped(Vector2(p_snapping_distance, p_snapping_distance));
    }

    // Pass 6 - Resolve references against the target and create event nodes
    HashMap<uint64_t, uint64_t> remap;
    HashSet<int> skipped;
    for (int i = 0; i < entries.size(); i++) {
        const Dictionary entry = entries[i];
        const int id = entry.get("id", -1);
        const String class_name = entry.get("class", String());
        Dictionary properties = entry.get("properties", Dictionary());

        if (ClassDB::is_parent_class(class_name, OScriptNodeEvent::get_class_static())) {
            const String guid = properties.get("function_id", String());
            if (!event_names.has(guid)) {
                skipped.insert(id);
                continue;
            }

            const StringName name = event_names[guid];
            if (result.skipped_events.has(name)) {
                skipped.insert(id);
                continue;
            }

            // Event nodes can only be placed inside event graphs
            if (!p_target->get_flags().has_flag(OrchestrationGraph::GF_EVENT)) {
                result.skipped_events[name] = "Cannot paste event nodes into non-event graphs.";
                skipped.insert(id);
                continue;
            }

            if (orchestration->find_function(name).is_valid()) {
                result.skipped_events[name] = "An event node already exists with the same name.";
                skipped.insert(id);
                continue;
            }

            // This creates the node and its matching event function signature, so the event is seeded
            // into the remap rather than imported as data.
            const Dictionary declaration = events[name];

            OScriptNodeInitContext context;
            context.method = DictionaryUtils::to_method(declaration.get("method", Dictionary()));
            context.user_data = DictionaryUtils::of({ { "user_defined", declaration.get("user_defined", false) } });

            const Vector2 position = Vector2(properties.get("position", Vector2())) + offset;
            const Ref<OScriptNode> node = p_target->create_node<OScriptNodeEvent>(context, position);
            if (!node.is_valid()) {
                skipped.insert(id);
                continue;
            }

            // The node created the function, carry over the rest of its declaration
            const Ref<OScriptFunction> function = orchestration->find_function(name);
            if (function.is_valid()) {
                _apply_properties(function, declaration, function_identity);
            }

            remap[id] = node->get_id();
            continue;
        }

        if (ClassDB::is_parent_class(class_name, OScriptNodeCallScriptFunction::get_class_static())) {
            // The function GUID belongs to the source orchestration and is rewritten to the target's function.
            // If the function doesn't exist (or has a different signature) in the target, skip the node.
            const StringName name = properties.get("function_name", String());
            const Ref<OScriptFunction> target_function = orchestration->find_function(name);
            if (result.skipped_functions.has(name) || !target_function.is_valid()) {
                skipped.insert(id);
                continue;
            }

            properties["guid"] = target_function->get_guid().to_string();
        } else if (ClassDB::is_parent_class(class_name, OScriptNodeVariable::get_class_static())) {
            const StringName name = properties.get("variable_name", String());
            if (result.skipped_variables.has(name)) {
                skipped.insert(id);
                continue;
            }
        } else if (ClassDB::is_parent_class(class_name, OScriptNodeEmitSignal::get_class_static())) {
            const StringName name = properties.get("signal_name", String());
            if (result.skipped_signals.has(name)) {
                skipped.insert(id);
                continue;
            }
        }
    }

    // Pass 7 - Import nodes, connections, knots, pin types and comment attachments
    p_target->import_nodes(graph, offset, remap, skipped);

    for (const KeyValue<uint64_t, uint64_t>& E : remap) {
        result.added_nodes.insert(E.value);
    }

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
    if (_buffer) {
        _buffer->payload.clear();
        _buffer->text = String();
    }
}

void OrchestratorEditorGraphClipboard::free_resources() {
    if (_buffer) {
        memdelete(_buffer);
        _buffer = nullptr;
    }
}
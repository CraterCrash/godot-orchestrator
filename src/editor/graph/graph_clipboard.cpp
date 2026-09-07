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
#include "common/name_utils.h"
#include "common/property_utils.h"
#include "common/resource_utils.h"
#include "common/version.h"
#include "orchestration/function.h"
#include "orchestration/nodes/call_function.h"
#include "orchestration/nodes/comment.h"
#include "orchestration/nodes/emit_signal.h"
#include "orchestration/nodes/event.h"
#include "orchestration/nodes/function_result.h"
#include "orchestration/nodes/local_variables.h"
#include "orchestration/nodes/operator_node.h"
#include "orchestration/nodes/variables.h"
#include "orchestration/orchestration.h"
#include "orchestration/serialization/format.h"
#include "orchestration/signals.h"
#include "orchestration/variable.h"

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace {
    constexpr const char* PAYLOAD_MAGIC = "orchestrator/clipboard";
    constexpr const char* PAYLOAD_HEADER = "; orchestrator/clipboard";

    // Properties that identify a resource within its source orchestration, or that the target consumes
    // when it creates the resource. Everything else in a declaration is applied as-is. Built on demand:
    // a StringName at static-init time is constructed before the extension is initialized and crashes
    // the plugin on load.
    Vector<StringName> function_identity() { return { "guid", "id", "method", "user_defined", "graph" }; }
    Vector<StringName> variable_identity() { return { "name" }; }
    Vector<StringName> signal_identity() { return { "signal_name" }; }
    Vector<StringName> local_variable_identity() { return { "name" }; }

    // The local variable nodes of the top-level selection; function bodies resolve their own
    Vector<Dictionary> get_graph_node_entries(const Dictionary& p_payload) {
        Vector<Dictionary> result;
        const Dictionary graph = p_payload.get("graph", Dictionary());
        const Array entries = graph.get("nodes", Array());
        for (int i = 0; i < entries.size(); i++) {
            result.push_back(entries[i]);
        }
        return result;
    }
}

OrchestratorEditorGraphClipboard::Buffer* OrchestratorEditorGraphClipboard::_buffer = nullptr;

bool OrchestratorEditorGraphClipboard::ClipboardResult::is_empty() const {
    return added_nodes.is_empty() && added_functions.is_empty() && added_variables.is_empty() && added_signals.is_empty()
        && added_local_variables.is_empty();
}

bool OrchestratorEditorGraphClipboard::ClipboardResult::had_skipped_nodes() const {
    return !skipped_functions.is_empty() || !skipped_events.is_empty() || !skipped_variables.is_empty()
        || !skipped_signals.is_empty() || !skipped_local_variables.is_empty() || !skipped_nodes.is_empty();
}

bool OrchestratorEditorGraphClipboard::ClipboardResult::had_renamed_declarations() const {
    return !renamed_functions.is_empty() || !renamed_variables.is_empty() || !renamed_signals.is_empty()
        || !renamed_local_variables.is_empty();
}

String OrchestratorEditorGraphClipboard::ClipboardResult::get_summary() const {
    String summary;

    if (had_renamed_declarations()) {
        summary += "The following items were pasted under a new name:\n\n";
        for (const KeyValue<StringName, StringName>& E : renamed_functions) {
            summary += vformat("* Function %s was pasted as %s\n", E.key, E.value);
        }
        for (const KeyValue<StringName, StringName>& E : renamed_variables) {
            summary += vformat("* Variable %s was pasted as %s\n", E.key, E.value);
        }
        for (const KeyValue<StringName, StringName>& E : renamed_signals) {
            summary += vformat("* Signal %s was pasted as %s\n", E.key, E.value);
        }
        for (const KeyValue<StringName, StringName>& E : renamed_local_variables) {
            summary += vformat("* Local variable %s was pasted as %s\n", E.key, E.value);
        }
    }

    if (had_skipped_nodes()) {
        if (!summary.is_empty()) {
            summary += "\n";
        }
        summary += "Several nodes were not pasted due to the following reasons:\n\n";
        for (const KeyValue<StringName, String>& E : skipped_functions) {
            summary += vformat("* Function %s: %s\n", E.key, E.value);
        }
        for (const KeyValue<StringName, String>& E : skipped_events) {
            summary += vformat("* Event %s: %s\n", E.key, E.value);
        }
        for (const KeyValue<StringName, String>& E : skipped_variables) {
            summary += vformat("* Variable %s: %s\n", E.key, E.value);
        }
        for (const KeyValue<StringName, String>& E : skipped_signals) {
            summary += vformat("* Signal %s: %s\n", E.key, E.value);
        }
        for (const KeyValue<StringName, String>& E : skipped_local_variables) {
            summary += vformat("* Local variable %s: %s\n", E.key, E.value);
        }
        for (const KeyValue<uint64_t, String>& E : skipped_nodes) {
            summary += vformat("* Node %d: %s\n", E.key, E.value);
        }
    }

    return summary;
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

    const uint32_t format = payload.get("format", OrchestrationFormat::FORMAT_VERSION);
    ERR_FAIL_COND_V_MSG(format > OrchestrationFormat::FORMAT_VERSION, false,
        vformat("The clipboard payload was created by a newer version of Orchestrator (%s).", String(payload.get("plugin", String()))));

    r_payload = payload;
    return true;
}

Dictionary OrchestratorEditorGraphClipboard::_create_payload(const Dictionary& p_functions, const Dictionary& p_events,
    const Dictionary& p_variables, const Dictionary& p_signals, const Dictionary& p_local_variables, const Dictionary& p_graph) {

    Dictionary payload;
    payload["magic"] = PAYLOAD_MAGIC;
    payload["plugin"] = VERSION_FULL_BUILD;
    payload["format"] = OrchestrationFormat::FORMAT_VERSION;
    if (!p_graph.is_empty()) {
        payload["graph"] = p_graph;
    }
    payload["functions"] = p_functions;
    payload["events"] = p_events;
    payload["variables"] = p_variables;
    payload["signals"] = p_signals;
    payload["local_variables"] = p_local_variables;

    return payload;
}

void OrchestratorEditorGraphClipboard::_collect_function_closure(Orchestration* p_source, const StringName& p_name,
    Dictionary& r_functions, Dictionary& r_variables, Dictionary& r_signals) {

    Vector<StringName> worklist;
    worklist.push_back(p_name);

    while (!worklist.is_empty()) {
        const StringName name = worklist[worklist.size() - 1];
        worklist.remove_at(worklist.size() - 1);

        if (r_functions.has(name)) {
            continue;
        }

        const Ref<OScriptFunction> function = p_source->find_function(name);
        if (!function.is_valid()) {
            continue;
        }

        // Only user functions have a body of their own; anything else travels as a declaration
        if (!function->is_user_defined()) {
            r_functions[name] = ResourceUtils::get_storage_properties(function);
            continue;
        }

        const Dictionary data = p_source->export_function(name);
        r_functions[name] = data;

        if (data.has("graph")) {
            _collect_references(p_source, data["graph"], worklist, r_variables, r_signals);
        }
    }
}

void OrchestratorEditorGraphClipboard::_collect_references(Orchestration* p_source, const Dictionary& p_graph,
    Vector<StringName>& r_worklist, Dictionary& r_variables, Dictionary& r_signals) {

    const Array entries = p_graph.get("nodes", Array());
    for (int i = 0; i < entries.size(); i++) {
        const Dictionary entry = entries[i];
        const String class_name = entry.get("class", String());
        const Dictionary properties = entry.get("properties", Dictionary());

        if (ClassDB::is_parent_class(class_name, OScriptNodeCallScriptFunction::get_class_static())) {
            r_worklist.push_back(properties.get("function_name", String()));
        } else if (ClassDB::is_parent_class(class_name, OScriptNodeVariable::get_class_static())) {
            const StringName name = properties.get("variable_name", String());
            if (!r_variables.has(name)) {
                const Ref<OScriptVariable> variable = p_source->get_variable(name);
                if (variable.is_valid()) {
                    r_variables[name] = ResourceUtils::get_storage_properties(variable);
                }
            }
        } else if (ClassDB::is_parent_class(class_name, OScriptNodeEmitSignal::get_class_static())) {
            const StringName name = properties.get("signal_name", String());
            if (!r_signals.has(name)) {
                const Ref<OScriptSignal> signal = p_source->find_custom_signal(name);
                if (signal.is_valid()) {
                    r_signals[name] = ResourceUtils::get_storage_properties(signal);
                }
            }
        }
    }
}

Vector<Dictionary> OrchestratorEditorGraphClipboard::_get_node_entries(const Dictionary& p_payload) {
    Vector<Dictionary> result;

    const Dictionary graph = p_payload.get("graph", Dictionary());
    const Array entries = graph.get("nodes", Array());
    for (int i = 0; i < entries.size(); i++) {
        result.push_back(entries[i]);
    }

    const Dictionary functions = p_payload.get("functions", Dictionary());
    const Array names = functions.keys();
    for (int i = 0; i < names.size(); i++) {
        const Dictionary declaration = functions[names[i]];
        const Dictionary body = declaration.get("graph", Dictionary());
        const Array body_entries = body.get("nodes", Array());
        for (int j = 0; j < body_entries.size(); j++) {
            result.push_back(body_entries[j]);
        }
    }

    return result;
}

String OrchestratorEditorGraphClipboard::_describe_function(const Dictionary& p_declaration) {
    const MethodInfo method = DictionaryUtils::to_method(ResourceUtils::get_storage_property(p_declaration, OScriptFunction::get_class_static(), "method"));
    return MethodUtils::get_signature(method);
}

String OrchestratorEditorGraphClipboard::_describe_variable(const Dictionary& p_declaration) {
    const PropertyInfo info = DictionaryUtils::to_property(ResourceUtils::get_storage_property(p_declaration, OScriptVariable::get_class_static(), "info"));
    return PropertyUtils::get_property_type_name(info);
}

String OrchestratorEditorGraphClipboard::_describe_signal(const Dictionary& p_declaration) {
    const MethodInfo method = DictionaryUtils::to_method(ResourceUtils::get_storage_property(p_declaration, OScriptSignal::get_class_static(), "method"));
    return MethodUtils::get_signature(method);
}

String OrchestratorEditorGraphClipboard::_describe_local_variable(const Dictionary& p_declaration) {
    const PropertyInfo info = DictionaryUtils::to_property(ResourceUtils::get_storage_property(p_declaration, OScriptLocalVariable::get_class_static(), "info"));
    return PropertyUtils::get_property_type_name(info);
}

void OrchestratorEditorGraphClipboard::_rename_function(Dictionary& p_payload, const StringName& p_old_name, const StringName& p_new_name) {
    Dictionary functions = p_payload.get("functions", Dictionary());
    if (!functions.has(p_old_name)) {
        return;
    }

    Dictionary declaration = functions[p_old_name];
    functions.erase(p_old_name);

    Dictionary method = declaration.get("method", Dictionary());
    method["name"] = p_new_name;
    declaration["method"] = method;
    functions[p_new_name] = declaration;

    // Every call to the function, in the selection and in any carried body, follows the new name
    for (const Dictionary& entry : _get_node_entries(p_payload)) {
        const String class_name = entry.get("class", String());
        if (!ClassDB::is_parent_class(class_name, OScriptNodeCallScriptFunction::get_class_static())) {
            continue;
        }

        Dictionary properties = entry.get("properties", Dictionary());
        if (StringName(properties.get("function_name", String())) != p_old_name) {
            continue;
        }

        properties["function_name"] = p_new_name;
        if (properties.has("method")) {
            Dictionary call_method = properties["method"];
            if (call_method.has("name")) {
                call_method["name"] = p_new_name;
            }
        }
    }
}

void OrchestratorEditorGraphClipboard::_rename_variable(Dictionary& p_payload, const StringName& p_old_name, const StringName& p_new_name) {
    Dictionary variables = p_payload.get("variables", Dictionary());
    if (!variables.has(p_old_name)) {
        return;
    }

    Dictionary declaration = variables[p_old_name];
    variables.erase(p_old_name);

    if (declaration.has("name")) {
        declaration["name"] = p_new_name;
    }
    if (declaration.has("info")) {
        Dictionary info = declaration["info"];
        if (info.has("name")) {
            info["name"] = p_new_name;
        }
    }
    variables[p_new_name] = declaration;

    for (const Dictionary& entry : _get_node_entries(p_payload)) {
        const String class_name = entry.get("class", String());
        if (!ClassDB::is_parent_class(class_name, OScriptNodeVariable::get_class_static())) {
            continue;
        }

        Dictionary properties = entry.get("properties", Dictionary());
        if (StringName(properties.get("variable_name", String())) == p_old_name) {
            properties["variable_name"] = p_new_name;
        }
    }
}

void OrchestratorEditorGraphClipboard::_rename_signal(Dictionary& p_payload, const StringName& p_old_name, const StringName& p_new_name) {
    Dictionary signals = p_payload.get("signals", Dictionary());
    if (!signals.has(p_old_name)) {
        return;
    }

    Dictionary declaration = signals[p_old_name];
    signals.erase(p_old_name);

    Dictionary method = declaration.get("method", Dictionary());
    method["name"] = p_new_name;
    declaration["method"] = method;
    if (declaration.has("signal_name")) {
        declaration["signal_name"] = p_new_name;
    }
    signals[p_new_name] = declaration;

    for (const Dictionary& entry : _get_node_entries(p_payload)) {
        const String class_name = entry.get("class", String());
        if (!ClassDB::is_parent_class(class_name, OScriptNodeEmitSignal::get_class_static())) {
            continue;
        }

        Dictionary properties = entry.get("properties", Dictionary());
        if (StringName(properties.get("signal_name", String())) == p_old_name) {
            properties["signal_name"] = p_new_name;
        }
    }
}

void OrchestratorEditorGraphClipboard::_rename_local_variable(Dictionary& p_payload, const StringName& p_old_name, const StringName& p_new_name) {
    Dictionary local_variables = p_payload.get("local_variables", Dictionary());
    if (!local_variables.has(p_old_name)) {
        return;
    }

    Dictionary declaration = local_variables[p_old_name];
    local_variables.erase(p_old_name);

    if (declaration.has("name")) {
        declaration["name"] = p_new_name;
    }
    if (declaration.has("info")) {
        Dictionary info = declaration["info"];
        if (info.has("name")) {
            info["name"] = p_new_name;
        }
    }
    local_variables[p_new_name] = declaration;

    // Only the selection references these locals; carried function bodies use their own declarations
    for (const Dictionary& entry : get_graph_node_entries(p_payload)) {
        const String class_name = entry.get("class", String());
        if (!ClassDB::is_parent_class(class_name, OScriptNodeLocalVariable::get_class_static())) {
            continue;
        }

        Dictionary properties = entry.get("properties", Dictionary());
        if (StringName(properties.get("variable_name", String())) == p_old_name) {
            properties["variable_name"] = p_new_name;
        }
    }
}

void OrchestratorEditorGraphClipboard::_apply_resolutions(Orchestration* p_target, const Ref<OScriptFunction>& p_function,
    Dictionary& p_payload, const Vector<Resolution>& p_resolutions, ClipboardResult& r_result) {

    // Unique names must avoid every identifier in the target, and the names chosen here
    PackedStringArray used = p_target->get_function_names();
    used.append_array(p_target->get_variable_names());
    used.append_array(p_target->get_custom_signal_names());

    // Local variables live in the function's namespace, beside its arguments and other locals
    PackedStringArray used_locals;
    if (p_function.is_valid()) {
        used_locals = p_function->get_local_variable_names();
        for (const PropertyInfo& argument : p_function->get_method_info().arguments) {
            used_locals.push_back(argument.name);
        }
    }

    for (const Resolution& resolution : p_resolutions) {
        if (!resolution.rename) {
            switch (resolution.kind) {
                case Conflict::FUNCTION: {
                    r_result.skipped_functions[resolution.name] = "Skipped by user.";
                    break;
                }
                case Conflict::VARIABLE: {
                    r_result.skipped_variables[resolution.name] = "Skipped by user.";
                    break;
                }
                case Conflict::SIGNAL: {
                    r_result.skipped_signals[resolution.name] = "Skipped by user.";
                    break;
                }
                case Conflict::LOCAL_VARIABLE: {
                    r_result.skipped_local_variables[resolution.name] = "Skipped by user.";
                    break;
                }
            }
            continue;
        }

        if (resolution.kind == Conflict::LOCAL_VARIABLE) {
            const StringName new_name = NameUtils::create_unique_name(resolution.name, used_locals);
            used_locals.push_back(new_name);

            _rename_local_variable(p_payload, resolution.name, new_name);
            r_result.renamed_local_variables[resolution.name] = new_name;
            continue;
        }

        const StringName new_name = NameUtils::create_unique_name(resolution.name, used);
        used.push_back(new_name);

        switch (resolution.kind) {
            case Conflict::FUNCTION: {
                _rename_function(p_payload, resolution.name, new_name);
                r_result.renamed_functions[resolution.name] = new_name;
                break;
            }
            case Conflict::VARIABLE: {
                _rename_variable(p_payload, resolution.name, new_name);
                r_result.renamed_variables[resolution.name] = new_name;
                break;
            }
            case Conflict::SIGNAL: {
                _rename_signal(p_payload, resolution.name, new_name);
                r_result.renamed_signals[resolution.name] = new_name;
                break;
            }
            default: {
                break;
            }
        }
    }
}

void OrchestratorEditorGraphClipboard::_paste_declarations(Orchestration* p_target, const StringName& p_function_name,
    Dictionary& p_payload, const Vector<Resolution>& p_resolutions, ClipboardResult& r_result) {

    const Ref<OScriptFunction> function = p_function_name.is_empty() ? Ref<OScriptFunction>() : p_target->find_function(p_function_name);

    _apply_resolutions(p_target, function, p_payload, p_resolutions, r_result);

    // Variables and signals first, function bodies resolve them by name as their nodes initialize
    const Dictionary variables = p_payload.get("variables", Dictionary());
    const Array variable_names = variables.keys();
    for (int i = 0; i < variable_names.size(); i++) {
        const StringName name = variable_names[i];
        if (r_result.skipped_variables.has(name)) {
            continue;
        }

        const Dictionary properties = variables[name];
        const Ref<OScriptVariable> target_variable = p_target->get_variable(name);
        if (target_variable.is_null()) {
            const Ref<OScriptVariable> variable = p_target->create_variable(name);
            ERR_CONTINUE(!variable.is_valid());
            ResourceUtils::apply_storage_properties(variable, properties, variable_identity());
            r_result.added_variables.insert(name);
        } else if (!PropertyUtils::are_equal(DictionaryUtils::to_property(ResourceUtils::get_storage_property(properties, OScriptVariable::get_class_static(), "info")), target_variable->get_info())) {
            r_result.skipped_variables[name] = "Variable declarations do not match.";
        }
    }

    const Dictionary signals = p_payload.get("signals", Dictionary());
    const Array signal_names = signals.keys();
    for (int i = 0; i < signal_names.size(); i++) {
        const StringName name = signal_names[i];
        if (r_result.skipped_signals.has(name)) {
            continue;
        }

        const Dictionary properties = signals[name];
        const Ref<OScriptSignal> target_signal = p_target->find_custom_signal(name);
        if (target_signal.is_null()) {
            const Ref<OScriptSignal> signal = p_target->create_custom_signal(name);
            ERR_CONTINUE(!signal.is_valid());
            ResourceUtils::apply_storage_properties(signal, properties, signal_identity());
            r_result.added_signals.insert(name);
        } else if (!MethodUtils::has_same_signature(DictionaryUtils::to_method(ResourceUtils::get_storage_property(properties, OScriptSignal::get_class_static(), "method")), target_signal->get_method_info())) {
            r_result.skipped_signals[name] = "Signal signatures do not match.";
        }
    }

    // Local variables land in the function that owns the target graph; without one they cannot be placed
    const Dictionary local_variables = p_payload.get("local_variables", Dictionary());
    const Array local_variable_names = local_variables.keys();
    for (int i = 0; i < local_variable_names.size(); i++) {
        const StringName name = local_variable_names[i];
        if (r_result.skipped_local_variables.has(name)) {
            continue;
        }

        if (!function.is_valid()) {
            r_result.skipped_local_variables[name] = "Local variables can only be pasted into a function graph.";
            continue;
        }

        const Dictionary properties = local_variables[name];
        const PropertyInfo info = DictionaryUtils::to_property(ResourceUtils::get_storage_property(properties, OScriptLocalVariable::get_class_static(), "info"));

        const Ref<OScriptLocalVariable> target_local = function->find_local_variable(name);
        if (target_local.is_null()) {
            if (!function->is_local_variable_name_available(name)) {
                r_result.skipped_local_variables[name] = "A function argument already has that name.";
                continue;
            }

            const Ref<OScriptLocalVariable> local_variable = function->create_local_variable(name, info.type);
            ERR_CONTINUE(!local_variable.is_valid());
            ResourceUtils::apply_storage_properties(local_variable, properties, local_variable_identity());
            r_result.added_local_variables.insert(name);
        } else if (!PropertyUtils::are_equal(info, target_local->get_info())) {
            r_result.skipped_local_variables[name] = "Local variable declarations do not match.";
        }
    }

    // Function declarations next, all of them before any function body, so calls between them can be bound
    const Dictionary functions = p_payload.get("functions", Dictionary());
    const Array function_names = functions.keys();
    for (int i = 0; i < function_names.size(); i++) {
        const StringName name = function_names[i];
        if (r_result.skipped_functions.has(name)) {
            continue;
        }

        const Dictionary declaration = functions[name];
        const Ref<OScriptFunction> target_function = p_target->find_function(name);
        if (!target_function.is_valid()) {
            const Ref<OScriptFunction> function = p_target->import_function(declaration, name);
            if (!function.is_valid()) {
                r_result.skipped_functions[name] = "Failed to create function.";
            } else {
                r_result.added_functions.insert(name);
            }
        } else if (!MethodUtils::has_same_signature(DictionaryUtils::to_method(ResourceUtils::get_storage_property(declaration, OScriptFunction::get_class_static(), "method")), target_function->get_method_info())) {
            r_result.skipped_functions[name] = "Function signatures do not match.";
        }
    }

    // Every function that will exist now does, so call nodes anywhere in the payload can be bound to it
    for (const Dictionary& entry : _get_node_entries(p_payload)) {
        const String class_name = entry.get("class", String());
        if (!ClassDB::is_parent_class(class_name, OScriptNodeCallScriptFunction::get_class_static())) {
            continue;
        }

        Dictionary properties = entry.get("properties", Dictionary());
        const Ref<OScriptFunction> target_function = p_target->find_function(StringName(properties.get("function_name", String())));
        if (target_function.is_valid()) {
            properties["guid"] = target_function->get_guid().to_string();
        }
    }

    // Bodies last, into the functions created above; an existing function keeps its own body
    for (const StringName& name : r_result.added_functions) {
        const Dictionary declaration = functions[name];
        if (declaration.has("graph")) {
            p_target->import_function_body(name, declaration["graph"]);
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
    ERR_FAIL_COND_V(p_source.is_null(), result);

    Orchestration* orchestration = p_source->get_orchestration();
    ERR_FAIL_NULL_V(orchestration, result);

    Dictionary functions;
    Dictionary events;
    Dictionary variables;
    Dictionary signals;
    Dictionary local_variables;
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

            // The called function travels with its body, and everything the body needs
            _collect_function_closure(orchestration, function->get_function_name(), functions, variables, signals);
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

        if (const Ref<OScriptNodeLocalVariable>& local_node = script_node; local_node.is_valid()) {
            const Ref<OScriptLocalVariable> local_variable = local_node->get_variable();
            ERR_CONTINUE_MSG(local_variable.is_null(), vformat("Cannot copy local variable node %d; its local variable no longer exists.", script_node->get_id()));

            local_variables[local_variable->get_variable_name()] = ResourceUtils::get_storage_properties(local_variable);
        }

        node_ids.push_back(script_node->get_id());
        result.added_nodes.insert(script_node->get_id());
    }

    _write_payload(_create_payload(functions, events, variables, signals, local_variables, p_source->export_nodes(node_ids)));

    return result;
}

void OrchestratorEditorGraphClipboard::copy_function(Orchestration* p_source, const StringName& p_name) {
    clear();
    ERR_FAIL_NULL(p_source);

    Dictionary functions;
    Dictionary variables;
    Dictionary signals;
    _collect_function_closure(p_source, p_name, functions, variables, signals);
    ERR_FAIL_COND_MSG(functions.is_empty(), "No function exists with the name: " + p_name);

    _write_payload(_create_payload(functions, Dictionary(), variables, signals));
}

void OrchestratorEditorGraphClipboard::copy_variable(Orchestration* p_source, const StringName& p_name) {
    clear();
    ERR_FAIL_NULL(p_source);

    const Ref<OScriptVariable> variable = p_source->get_variable(p_name);
    ERR_FAIL_COND_MSG(variable.is_null(), "No variable exists with the name: " + p_name);

    Dictionary variables;
    variables[p_name] = ResourceUtils::get_storage_properties(variable);

    _write_payload(_create_payload(Dictionary(), Dictionary(), variables, Dictionary()));
}

void OrchestratorEditorGraphClipboard::copy_signal(Orchestration* p_source, const StringName& p_name) {
    clear();
    ERR_FAIL_NULL(p_source);

    const Ref<OScriptSignal> signal = p_source->find_custom_signal(p_name);
    ERR_FAIL_COND_MSG(signal.is_null(), "No signal exists with the name: " + p_name);

    Dictionary signals;
    signals[p_name] = ResourceUtils::get_storage_properties(signal);

    _write_payload(_create_payload(Dictionary(), Dictionary(), Dictionary(), signals));
}

void OrchestratorEditorGraphClipboard::copy_local_variable(Orchestration* p_source, const StringName& p_function_name, const StringName& p_name) {
    clear();
    ERR_FAIL_NULL(p_source);

    const Ref<OScriptFunction> function = p_source->find_function(p_function_name);
    ERR_FAIL_COND_MSG(function.is_null(), "No function exists with the name: " + p_function_name);

    const Ref<OScriptLocalVariable> local_variable = function->find_local_variable(p_name);
    ERR_FAIL_COND_MSG(local_variable.is_null(), vformat("No local variable exists with the name '%s' in function '%s'.", p_name, p_function_name));

    Dictionary local_variables;
    local_variables[p_name] = ResourceUtils::get_storage_properties(local_variable);

    _write_payload(_create_payload(Dictionary(), Dictionary(), Dictionary(), Dictionary(), local_variables));
}

Vector<OrchestratorEditorGraphClipboard::Conflict> OrchestratorEditorGraphClipboard::plan(Orchestration* p_target, const StringName& p_function_name) {
    Vector<Conflict> conflicts;
    ERR_FAIL_NULL_V(p_target, conflicts);

    Dictionary payload;
    if (!_read_payload(payload)) {
        return conflicts;
    }

    const Dictionary functions = payload.get("functions", Dictionary());
    const Array function_names = functions.keys();
    for (int i = 0; i < function_names.size(); i++) {
        const StringName name = function_names[i];
        const Dictionary declaration = functions[name];

        const Ref<OScriptFunction> target_function = p_target->find_function(name);
        if (target_function.is_valid()) {
            const MethodInfo method = DictionaryUtils::to_method(ResourceUtils::get_storage_property(declaration, OScriptFunction::get_class_static(), "method"));
            if (!MethodUtils::has_same_signature(method, target_function->get_method_info())) {
                conflicts.push_back({ Conflict::FUNCTION, name, _describe_function(declaration), MethodUtils::get_signature(target_function->get_method_info()) });
            }
        }
    }

    const Dictionary variables = payload.get("variables", Dictionary());
    const Array variable_names = variables.keys();
    for (int i = 0; i < variable_names.size(); i++) {
        const StringName name = variable_names[i];
        const Dictionary declaration = variables[name];

        const Ref<OScriptVariable> target_variable = p_target->get_variable(name);
        if (target_variable.is_valid()) {
            const PropertyInfo info = DictionaryUtils::to_property(ResourceUtils::get_storage_property(declaration, OScriptVariable::get_class_static(), "info"));
            if (!PropertyUtils::are_equal(info, target_variable->get_info())) {
                conflicts.push_back({ Conflict::VARIABLE, name, _describe_variable(declaration), PropertyUtils::get_property_type_name(target_variable->get_info()) });
            }
        }
    }

    const Dictionary signals = payload.get("signals", Dictionary());
    const Array signal_names = signals.keys();
    for (int i = 0; i < signal_names.size(); i++) {
        const StringName name = signal_names[i];
        const Dictionary declaration = signals[name];

        const Ref<OScriptSignal> target_signal = p_target->find_custom_signal(name);
        if (target_signal.is_valid()) {
            const MethodInfo method = DictionaryUtils::to_method(ResourceUtils::get_storage_property(declaration, OScriptSignal::get_class_static(), "method"));
            if (!MethodUtils::has_same_signature(method, target_signal->get_method_info())) {
                conflicts.push_back({ Conflict::SIGNAL, name, _describe_signal(declaration), MethodUtils::get_signature(target_signal->get_method_info()) });
            }
        }
    }

    const Ref<OScriptFunction> function = p_function_name.is_empty() ? Ref<OScriptFunction>() : p_target->find_function(p_function_name);
    if (function.is_valid()) {
        const Dictionary local_variables = payload.get("local_variables", Dictionary());
        const Array local_variable_names = local_variables.keys();
        for (int i = 0; i < local_variable_names.size(); i++) {
            const StringName name = local_variable_names[i];
            const Dictionary declaration = local_variables[name];

            const Ref<OScriptLocalVariable> target_local = function->find_local_variable(name);
            if (target_local.is_valid()) {
                const PropertyInfo info = DictionaryUtils::to_property(ResourceUtils::get_storage_property(declaration, OScriptLocalVariable::get_class_static(), "info"));
                if (!PropertyUtils::are_equal(info, target_local->get_info())) {
                    conflicts.push_back({ Conflict::LOCAL_VARIABLE, name, _describe_local_variable(declaration), PropertyUtils::get_property_type_name(target_local->get_info()) });
                }
            }
        }
    }

    return conflicts;
}

OrchestratorEditorGraphClipboard::ClipboardResult OrchestratorEditorGraphClipboard::paste(const Ref<OrchestrationGraph>& p_target,
    const Vector2& p_offset, bool p_snapping_enabled, int p_snapping_distance, const Vector<Resolution>& p_resolutions) {

    ClipboardResult result;
    ERR_FAIL_COND_V(p_target.is_null(), result);

    Dictionary payload;
    if (!_read_payload(payload)) {
        return result;
    }

    Orchestration* orchestration = p_target->get_orchestration();
    ERR_FAIL_NULL_V(orchestration, result);

    // The payload is worked on as a deep copy, renames and reference rewrites must not alter the buffer
    Dictionary working = payload.duplicate(true);

    // Local variables belong to the function whose graph receives the paste
    const bool function_graph = p_target->get_flags().has_flag(OrchestrationGraph::GF_FUNCTION);
    const StringName function_name = function_graph ? p_target->get_graph_name() : StringName();

    _paste_declarations(orchestration, function_name, working, p_resolutions, result);

    // A payload copied from the components panel carries no graph nodes, the declarations were the paste
    const Dictionary graph = working.get("graph", Dictionary());
    const Array entries = graph.get("nodes", Array());
    if (entries.is_empty()) {
        return result;
    }

    // Verify events; their function may already exist in the target with a different signature
    HashMap<String, StringName> event_names;
    const Dictionary events = working.get("events", Dictionary());
    const Array event_keys = events.keys();
    for (int i = 0; i < event_keys.size(); i++) {
        const StringName name = event_keys[i];
        const Dictionary declaration = events[name];
        event_names[declaration.get("guid", String())] = name;

        const Ref<OScriptFunction> target_function = orchestration->find_function(name);
        if (target_function.is_valid()) {
            const MethodInfo method = DictionaryUtils::to_method(ResourceUtils::get_storage_property(declaration, OScriptFunction::get_class_static(), "method"));
            if (!MethodUtils::has_same_signature(method, target_function->get_method_info())) {
                result.skipped_events[name] = "Event function signatures do not match.";
            }
        }
    }

    // Compute the paste offset from the first node
    Vector2 offset = p_offset;
    {
        const Dictionary first = entries[0];
        const Dictionary properties = first.get("properties", Dictionary());
        offset -= Vector2(properties.get("position", Vector2()));
    }

    if (p_snapping_enabled) {
        offset = offset.snapped(Vector2(p_snapping_distance, p_snapping_distance));
    }

    // Resolve node references against the target and create event nodes
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
            context.method = DictionaryUtils::to_method(ResourceUtils::get_storage_property(declaration, OScriptFunction::get_class_static(), "method"));
            context.user_data = DictionaryUtils::of({ { "user_defined", ResourceUtils::get_storage_property(declaration, OScriptFunction::get_class_static(), "user_defined") } });

            const Vector2 position = Vector2(properties.get("position", Vector2())) + offset;
            const Ref<OScriptNode> node = p_target->create_node<OScriptNodeEvent>(context, position);
            if (!node.is_valid()) {
                skipped.insert(id);
                continue;
            }

            // The node created the function, carry over the rest of its declaration
            const Ref<OScriptFunction> function = orchestration->find_function(name);
            if (function.is_valid()) {
                ResourceUtils::apply_storage_properties(function, declaration, function_identity());
            }

            remap[id] = node->get_id();
            continue;
        }

        // A return node rebinds to the function owning the graph it lands in when it is placed, see
        // OScriptNodeFunctionResult::post_placed_new_node, but it cannot exist outside a function graph at all.
        if (ClassDB::is_parent_class(class_name, OScriptNodeFunctionResult::get_class_static())
                && !p_target->get_flags().has_flag(OrchestrationGraph::GF_FUNCTION)) {
            result.skipped_nodes[id] = "Return nodes can only be pasted into function graphs.";
            skipped.insert(id);
            continue;
        }

        // Nodes that reference a declaration the paste could not provide are left out
        if (ClassDB::is_parent_class(class_name, OScriptNodeCallScriptFunction::get_class_static())) {
            const StringName name = properties.get("function_name", String());
            if (result.skipped_functions.has(name) || !orchestration->find_function(name).is_valid()) {
                skipped.insert(id);
                continue;
            }
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
        } else if (ClassDB::is_parent_class(class_name, OScriptNodeLocalVariable::get_class_static())) {
            const StringName name = properties.get("variable_name", String());
            if (!function_graph) {
                result.skipped_nodes[id] = "Local variable nodes can only be pasted into function graphs.";
                skipped.insert(id);
                continue;
            }

            const Ref<OScriptFunction> function = orchestration->find_function(function_name);
            if (result.skipped_local_variables.has(name) || !function.is_valid() || !function->has_local_variable(name)) {
                skipped.insert(id);
                continue;
            }
        }
    }

    // Import nodes, connections, knots, pin types and comment attachments
    p_target->import_nodes(graph, offset, remap, skipped);

    for (const KeyValue<uint64_t, uint64_t>& E : remap) {
        result.added_nodes.insert(E.value);
    }

    return result;
}

OrchestratorEditorGraphClipboard::ClipboardResult OrchestratorEditorGraphClipboard::paste_declarations(Orchestration* p_target,
    const Vector<Resolution>& p_resolutions, const StringName& p_function_name) {

    ClipboardResult result;
    ERR_FAIL_NULL_V(p_target, result);

    Dictionary payload;
    if (!_read_payload(payload)) {
        return result;
    }

    Dictionary working = payload.duplicate(true);
    _paste_declarations(p_target, p_function_name, working, p_resolutions, result);

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
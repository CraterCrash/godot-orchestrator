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
#pragma once

#include "orchestration/graph.h"
#include "orchestration/node.h"

#include <godot_cpp/templates/hash_set.hpp>

/// Forward declarations
class Orchestration;

/// Manages the clipboard state for the graph and component panels.
///
/// The clipboard holds data rather than live nodes, so it has no ties to the orchestration the nodes
/// were copied from. The payload is a dictionary with the following layout:
///
/// <pre>
/// magic       "orchestrator/clipboard"
/// plugin      the plugin version that produced the payload
/// format      the script format version, see OrchestrationFormat
/// graph       exported nodes, see OScriptGraph::export_nodes; absent when only declarations were copied
/// functions   name -> persisted function properties, plus "graph" holding the body of user functions
/// events      name -> persisted function properties, for every event node
/// variables   name -> persisted variable properties
/// signals     name -> persisted signal properties
/// </pre>
///
/// Copying a graph selection walks the closure of every called user function: the bodies of those
/// functions, the functions they call in turn, and the variables and signals any of them reference.
///
/// Declarations carry every persisted property of the resource, see ResourceUtils::get_storage_properties.
/// On paste the properties that identify the resource within its source orchestration are excluded and
/// everything else is applied, so a property added to a resource later is carried without changes here.
///
/// Paste is two steps. <code>plan</code> reports declarations that exist in the target with a different
/// definition, and <code>paste</code> takes the user's resolution for each: paste under a unique name, or
/// skip. Without a resolution a conflicting declaration is skipped. A missing declaration is created and a
/// matching one is reused.
///
/// The payload travels on the OS clipboard as text: a header line followed by the dictionary in
/// <code>var_to_str</code> form. The header begins with <code>;</code>, which the variant parser treats as
/// a comment, so the text round-trips through any text editor and pastes into another editor instance.
/// The OS clipboard is authoritative on paste. The last payload written is kept in memory for headless
/// runs and to skip reparsing text this editor produced.
class OrchestratorEditorGraphClipboard {

public:
    /// A declaration in the payload that exists in the target with a different definition
    struct Conflict {
        enum Kind {
            FUNCTION,
            VARIABLE,
            SIGNAL
        };

        Kind kind;
        StringName name;
        String source;   //! The definition in the payload
        String target;   //! The definition in the target orchestration
    };

    /// The user's decision for a conflict
    struct Resolution {
        Conflict::Kind kind;
        StringName name;
        bool rename = false;   //! Paste under a unique name when true, skip otherwise
    };

    struct ClipboardResult {
        HashSet<uint64_t> added_nodes;
        HashSet<StringName> added_functions;
        HashSet<StringName> added_variables;
        HashSet<StringName> added_signals;
        HashMap<StringName, StringName> renamed_functions;
        HashMap<StringName, StringName> renamed_variables;
        HashMap<StringName, StringName> renamed_signals;
        HashMap<StringName, String> skipped_functions;
        HashMap<StringName, String> skipped_events;
        HashMap<StringName, String> skipped_variables;
        HashMap<StringName, String> skipped_signals;

        /// Whether nothing at all was pasted
        bool is_empty() const;
        bool had_skipped_nodes() const;
        bool had_renamed_declarations() const;

        /// Builds the user-facing summary of renamed and skipped declarations, empty if there are none
        String get_summary() const;
    };

private:
    // Allocated on first use. A static Dictionary is constructed before the extension is initialized
    // and crashes the plugin on load, so the buffer lives behind a pointer.
    struct Buffer {
        Dictionary payload;
        String text;
    };

    static Buffer* _buffer;

    static void _write_payload(const Dictionary& p_payload);
    static bool _read_payload(Dictionary& r_payload);
    static Dictionary _create_payload(const Dictionary& p_functions, const Dictionary& p_events, const Dictionary& p_variables, const Dictionary& p_signals, const Dictionary& p_graph = Dictionary());

    static void _collect_function_closure(Orchestration* p_source, const StringName& p_name, Dictionary& r_functions, Dictionary& r_variables, Dictionary& r_signals);
    static void _collect_references(Orchestration* p_source, const Dictionary& p_graph, Vector<StringName>& r_worklist, Dictionary& r_variables, Dictionary& r_signals);
    static Vector<Dictionary> _get_node_entries(const Dictionary& p_payload);

    static String _describe_function(const Dictionary& p_declaration);
    static String _describe_variable(const Dictionary& p_declaration);
    static String _describe_signal(const Dictionary& p_declaration);

    static void _rename_function(Dictionary& p_payload, const StringName& p_old_name, const StringName& p_new_name);
    static void _rename_variable(Dictionary& p_payload, const StringName& p_old_name, const StringName& p_new_name);
    static void _rename_signal(Dictionary& p_payload, const StringName& p_old_name, const StringName& p_new_name);
    static void _apply_resolutions(Orchestration* p_target, Dictionary& p_payload, const Vector<Resolution>& p_resolutions, ClipboardResult& r_result);

    static void _paste_declarations(Orchestration* p_target, Dictionary& p_payload, const Vector<Resolution>& p_resolutions, ClipboardResult& r_result);
    static void _remap_comment_attachments(const Ref<OrchestrationGraph>& p_graph, const HashSet<uint64_t>& p_node_ids, const HashMap<uint64_t, uint64_t>& p_remap);

public:
    /// Copies a graph selection, with the closure of every user function the selection calls
    ClipboardResult copy(const Vector<Ref<OrchestrationGraphNode>>& p_nodes, const Ref<OrchestrationGraph>& p_source);

    /// Copies a function declaration, its body, and the closure of everything the body references
    void copy_function(Orchestration* p_source, const StringName& p_name);
    /// Copies a variable declaration
    void copy_variable(Orchestration* p_source, const StringName& p_name);
    /// Copies a signal declaration
    void copy_signal(Orchestration* p_source, const StringName& p_name);

    /// Reports the declarations in the clipboard that conflict with the target. An empty result means
    /// paste can proceed without asking the user anything.
    Vector<Conflict> plan(Orchestration* p_target);

    /// Pastes the declarations and then the graph nodes into the target graph
    ClipboardResult paste(const Ref<OrchestrationGraph>& p_target, const Vector2& p_offset, bool p_snapping_enabled, int p_snapping_distance, const Vector<Resolution>& p_resolutions = Vector<Resolution>());
    /// Pastes only the declarations, functions with their bodies, into the target. Graph nodes are ignored.
    ClipboardResult paste_declarations(Orchestration* p_target, const Vector<Resolution>& p_resolutions = Vector<Resolution>());

    ClipboardResult duplicate(const Vector<Ref<OrchestrationGraphNode>>& p_nodes, const Ref<OrchestrationGraph>& p_graph, const Vector2& p_offset);

    /// Clears the in-memory payload. The OS clipboard is left as-is.
    void clear();

    /// Releases the in-memory payload, called when the editor is torn down
    static void free_resources();
};
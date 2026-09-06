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

/// Manages the clipboard state for <code>OrchestratorEditorGraphPanel</code>.
///
/// The clipboard holds data rather than live nodes, so it has no ties to the orchestration the nodes
/// were copied from. The payload is a dictionary with the following layout:
///
/// <pre>
/// magic       "orchestrator/clipboard"
/// plugin      the plugin version that produced the payload
/// graph       exported nodes, see OScriptGraph::export_nodes
/// functions   name -> persisted function properties, for every called script function
/// events      name -> persisted function properties, for every event node
/// variables   name -> persisted variable properties
/// signals     name -> persisted signal properties
/// </pre>
///
/// Declarations carry every persisted property of the resource, see ResourceUtils::get_storage_properties.
/// On paste the properties that identify the resource within its source orchestration are excluded and
/// everything else is applied, so a property added to a resource later is carried without changes here.
///
/// The payload travels on the OS clipboard as text: a header line followed by the dictionary in
/// <code>var_to_str</code> form. The header begins with <code>;</code>, which the variant parser treats as
/// a comment, so the text round-trips through any text editor and pastes into another editor instance.
/// The OS clipboard is authoritative on paste. The last payload written is kept in memory for headless
/// runs and to skip re-parsing text this editor produced.
class OrchestratorEditorGraphClipboard {

    // Allocated on first use. A static Dictionary is constructed before the extension is initialized
    // and crashes the plugin on load, so the buffer lives behind a pointer.
    struct Buffer {
        Dictionary payload;
        String text;
    };

    static Buffer* _buffer;

    static void _write_payload(const Dictionary& p_payload);
    static bool _read_payload(Dictionary& r_payload);
    static Variant _get_declared(const Dictionary& p_properties, const StringName& p_class, const StringName& p_name);
    static void _apply_properties(const Ref<Resource>& p_resource, const Dictionary& p_properties, const Vector<StringName>& p_excluded);
    static void _remap_comment_attachments(const Ref<OrchestrationGraph>& p_graph, const HashSet<uint64_t>& p_node_ids, const HashMap<uint64_t, uint64_t>& p_remap);

public:
    struct ClipboardResult {
        HashSet<uint64_t> added_nodes;
        HashMap<StringName, String> skipped_functions;
        HashMap<StringName, String> skipped_events;
        HashMap<StringName, String> skipped_variables;
        HashMap<StringName, String> skipped_signals;

        bool had_skipped_nodes() const;
    };

    ClipboardResult copy(const Vector<Ref<OrchestrationGraphNode>>& p_nodes, const Ref<OrchestrationGraph>& p_source);
    ClipboardResult paste(const Ref<OrchestrationGraph>& p_target, const Vector2& p_offset, bool p_snapping_enabled, int p_snapping_distance);
    ClipboardResult duplicate(const Vector<Ref<OrchestrationGraphNode>>& p_nodes, const Ref<OrchestrationGraph>& p_graph, const Vector2& p_offset);

    /// Clears the in-memory payload. The OS clipboard is left as-is.
    void clear();

    /// Releases the in-memory payload, called when the editor is torn down
    static void free_resources();
};
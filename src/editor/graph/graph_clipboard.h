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

#include "editor/graph/graph_node.h"
#include "orchestration/node.h"
#include "orchestration/signals.h"
#include "orchestration/variable.h"

#include <godot_cpp/templates/hash_set.hpp>

/// Manages the clipboard state for <code>OrchestratorEditorGraphPanel</code>.
class OrchestratorEditorGraphClipboard {

    struct CopyItem {
        int id;
        Ref<OrchestrationGraphNode> node;
        Vector2 position;
        Vector2 size;
    };

    struct Buffer {
        List<CopyItem> nodes;
        List<uint64_t> connections;
        HashMap<StringName, Ref<OScriptVariable>> variables;
        HashMap<StringName, Ref<OScriptFunction>> functions;
        HashMap<StringName, Ref<OScriptFunction>> events;
        HashMap<StringName, Ref<OScriptSignal>> signals;

        bool is_empty() const;
        void clear();
    };

    static Buffer _buffer;

public:
    struct ClipboardResult {
        HashSet<uint64_t> added_nodes;
        HashMap<StringName, String> skipped_functions;
        HashMap<StringName, String> skipped_events;
        HashMap<StringName, String> skipped_variables;
        HashMap<StringName, String> skipped_signals;

        bool had_skipped_nodes() const;
    };

    ClipboardResult copy(const Vector<OrchestratorEditorGraphNode*>& p_nodes, const Ref<OrchestrationGraph>& p_source);
    ClipboardResult paste(const Ref<OrchestrationGraph>& p_target, const Vector2& p_offset, bool p_snapping_enabled, int p_snapping_distance);
    ClipboardResult duplicate(const Vector<OrchestratorEditorGraphNode*>& p_nodes, const Ref<OrchestrationGraph>& p_graph, const Vector2& p_offset);

    void clear();
};
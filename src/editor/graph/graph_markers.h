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

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/templates/hash_map.hpp>

using namespace godot;

class OrchestratorEditorGraphNode;
class OrchestratorEditorGraphPanel;

/// Helper class that maintains information about specific graph markers. These markers
/// include things like Bookmarks and Breakpoints.
class OrchestratorEditorGraphMarkers : public Object {
    GDCLASS(OrchestratorEditorGraphMarkers, Object);

    OrchestratorEditorGraphPanel* _panel = nullptr;

    HashMap<int, bool> _breakpoint_state;
    PackedInt64Array _breakpoints;
    PackedInt64Array _bookmarks;
    int _breakpoints_index = -1;
    int _bookmarks_index = -1;

    String _get_script_path() const;
    void _set_debugger_breakpoint(int p_node_id, bool p_enabled);

protected:
    static void _bind_methods();

public:

    void initialize(OrchestratorEditorGraphPanel* p_panel);

    //~ Begin Bookmark API
    bool is_bookmarked(const OrchestratorEditorGraphNode* p_node) const;

    void set_bookmarked(OrchestratorEditorGraphNode* p_node, bool p_bookmarked);
    void toggle_bookmark(OrchestratorEditorGraphNode* p_node);

    void goto_next_bookmark();
    void goto_previous_bookmark();
    //~ End Bookmark API

    //~ Begin Breakpoint API
    bool has_breakpoint(int p_node_id) const;
    bool is_breakpoint_enabled(int p_node_id) const;
    bool is_breakpoint(const OrchestratorEditorGraphNode* p_node) const;

    bool get_breakpoint(const OrchestratorEditorGraphNode* p_node);
    void set_breakpoint(OrchestratorEditorGraphNode* p_node, bool p_breakpoint);
    void set_breakpoint_enabled(OrchestratorEditorGraphNode* p_node, bool p_enabled);
    void toggle_breakpoint(OrchestratorEditorGraphNode* p_node);

    void goto_next_breakpoint();
    void goto_previous_breakpoint();

    PackedInt32Array get_breakpoints() const;
    void clear_breakpoints();
    //~ End Breakpoint API

    void save_state(Dictionary& r_state);
    void load_state(const Dictionary& p_state);
};
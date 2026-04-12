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
#include "editor/graph/graph_markers.h"

#include "common/callable_lambda.h"
#include "editor/debugger/script_debugger_plugin.h"
#include "editor/graph/graph_panel.h"
#include "orchestration/orchestration.h"
#include "script/script.h"

String OrchestratorEditorGraphMarkers::_get_script_path() const {
    return _panel->_graph->get_orchestration()->as_script()->get_path();
}

void OrchestratorEditorGraphMarkers::_set_debugger_breakpoint(int p_node_id, bool p_enabled) {
    if (OrchestratorEditorDebuggerPlugin* debugger = OrchestratorEditorDebuggerPlugin::get_singleton()) {
        debugger->set_breakpoint(_get_script_path(), p_node_id, p_enabled);
    }
}

void OrchestratorEditorGraphMarkers::initialize(OrchestratorEditorGraphPanel* p_panel) {
    _panel = p_panel;
}

bool OrchestratorEditorGraphMarkers::is_bookmarked(const OrchestratorEditorGraphNode* p_node) const {
    ERR_FAIL_NULL_V(p_node, false);
    return _bookmarks.has(p_node->get_id());
}

void OrchestratorEditorGraphMarkers::set_bookmarked(OrchestratorEditorGraphNode* p_node, bool p_bookmarked) {
    ERR_FAIL_NULL(p_node);

    const int node_id = p_node->get_id();
    const int index = _bookmarks.find(node_id);

    if (index != -1 && !p_bookmarked) {
        _bookmarks.remove_at(index);
        p_node->notify_bookmarks_changed();
    } else if (index == -1 && p_bookmarked) {
        _bookmarks.push_back(node_id);
        p_node->notify_bookmarks_changed();
    }
}

void OrchestratorEditorGraphMarkers::toggle_bookmark(OrchestratorEditorGraphNode* p_node) {
    set_bookmarked(p_node, !is_bookmarked(p_node));
}

void OrchestratorEditorGraphMarkers::goto_next_bookmark() {
    if (_bookmarks.is_empty()) {
        _bookmarks_index = -1;
        return;
    }

    if (_bookmarks_index >= _bookmarks.size()) {
        _bookmarks_index = -1;
    }

    _bookmarks_index = _bookmarks_index == -1 ? 0 : (_bookmarks_index + 1) % _bookmarks.size();
    _panel->center_node_id(_bookmarks[_bookmarks_index]);
}

void OrchestratorEditorGraphMarkers::goto_previous_bookmark() {
    if (_bookmarks.is_empty()) {
        _bookmarks_index = -1;
        return;
    }

    if (_bookmarks_index >= _bookmarks.size()) {
        _bookmarks_index = -1;
    }

    _bookmarks_index = _bookmarks_index == -1
        ? _bookmarks.size() - 1 : (_bookmarks_index - 1 + _bookmarks.size()) % _bookmarks.size();
    _panel->center_node_id(_bookmarks[_bookmarks_index]);
}

bool OrchestratorEditorGraphMarkers::has_breakpoint(int p_node_id) const {
    return _breakpoints.has(p_node_id);
}

bool OrchestratorEditorGraphMarkers::is_breakpoint_enabled(int p_node_id) const {
    if (has_breakpoint(p_node_id)) {
        return _breakpoint_state[p_node_id];
    }
    return false;
}

bool OrchestratorEditorGraphMarkers::is_breakpoint(const OrchestratorEditorGraphNode* p_node) const {
    ERR_FAIL_NULL_V(p_node, false);
    return has_breakpoint(p_node->get_id());
}

bool OrchestratorEditorGraphMarkers::get_breakpoint(const OrchestratorEditorGraphNode* p_node) {
    ERR_FAIL_NULL_V(p_node, false);
    return _breakpoint_state.has(p_node->get_id()) ? _breakpoint_state[p_node->get_id()] : false;
}

void OrchestratorEditorGraphMarkers::set_breakpoint(OrchestratorEditorGraphNode* p_node, bool p_breakpoint) {
    ERR_FAIL_NULL_MSG(p_node, "Cannot set node breakpoint on an invalid node reference");

    const int id = p_node->get_id();
    if (p_breakpoint) {
        _breakpoint_state[id] = true;
        if (!_breakpoints.has(id)) {
            _breakpoints.push_back(id);
        }
        _panel->emit_signal("breakpoint_added", id);
    } else {
        _breakpoint_state.erase(id);
        const int index = _breakpoints.find(id);
        if (index != -1) {
            _breakpoints.remove_at(index);
        }
        _panel->emit_signal("breakpoint_removed", id);
    }

    _set_debugger_breakpoint(id, p_breakpoint);

    p_node->notify_breakpoints_changed();
}

void OrchestratorEditorGraphMarkers::set_breakpoint_enabled(OrchestratorEditorGraphNode* p_node, bool p_enabled) {
    ERR_FAIL_NULL_MSG(p_node, "Cannot set node breakpoint status on an invalid node reference");

    const int id = p_node->get_id();
    _breakpoint_state[id] = p_enabled;
    _panel->emit_signal("breakpoint_changed", id, p_enabled);

    if (!_breakpoints.has(id)) {
        _breakpoints.push_back(id);
    }

    _set_debugger_breakpoint(id, p_enabled);

    p_node->notify_breakpoints_changed();
}

void OrchestratorEditorGraphMarkers::toggle_breakpoint(OrchestratorEditorGraphNode* p_node) {
    ERR_FAIL_NULL_MSG(p_node, "Cannot toggle node breakpoint on an invalid node reference");

    const int id = p_node->get_id();
    if (!_breakpoint_state.has(id)) {
        _breakpoint_state[id] = true;
        _breakpoints.push_back(id);
        _panel->emit_signal("breakpoint_added", id);
    } else {
        _breakpoint_state.erase(id);
        if (_breakpoints.has(id)) {
            _breakpoints.remove_at(_breakpoints.find(id));
        }
        _panel->emit_signal("breakpoint_removed", id);
    }

    _set_debugger_breakpoint(id, _breakpoints.has(id));

    p_node->notify_breakpoints_changed();
}

void OrchestratorEditorGraphMarkers::goto_next_breakpoint() {
    if (_breakpoints.is_empty()) {
        _breakpoints_index = -1;
        return;
    }

    if (_breakpoints_index >= _breakpoints.size()) {
        _breakpoints_index = -1;
    }

    _breakpoints_index = _breakpoints_index == -1 ? 0 : (_breakpoints_index + 1) % _breakpoints.size();
    _panel->center_node_id(_breakpoints[_breakpoints_index]);
}

void OrchestratorEditorGraphMarkers::goto_previous_breakpoint() {
    if (_breakpoints.is_empty()) {
        _breakpoints_index = -1;
        return;
    }

    if (_breakpoints_index >= _breakpoints.size()) {
        _breakpoints_index = -1;
    }

    _breakpoints_index = _breakpoints_index == -1
        ? _breakpoints.size() - 1 : (_breakpoints_index - 1 + _breakpoints.size()) % _breakpoints.size();
    _panel->center_node_id(_breakpoints[_breakpoints_index]);
}

PackedInt32Array OrchestratorEditorGraphMarkers::get_breakpoints() const {
    PackedInt32Array active_breakpoints;
    for (const KeyValue<int, bool>& E : _breakpoint_state) {
        if (E.value && !active_breakpoints.has(E.key)) {
            active_breakpoints.push_back(E.key);
        }
    }
    return active_breakpoints;
}

void OrchestratorEditorGraphMarkers::clear_breakpoints() {
    while (!_breakpoints.is_empty()) {
        int node_id = _breakpoints[_breakpoints.size() - 1];

        _set_debugger_breakpoint(node_id, false);

        _breakpoints.remove_at(_breakpoints.size() - 1);
        _breakpoint_state.erase(node_id);
    }

    _panel->_queue_panel_refresh();
}

void OrchestratorEditorGraphMarkers::save_state(Dictionary& r_state) {
    Array breakpoints;
    for (const KeyValue<int, bool>& E : _breakpoint_state) {
        Dictionary data;
        data[E.key] = E.value;
        breakpoints.push_back(data);
    }

    r_state["bookmarks"] = _bookmarks;
    r_state["breakpoints"] = breakpoints;
}

void OrchestratorEditorGraphMarkers::load_state(const Dictionary& p_state) {
    ERR_FAIL_NULL(_panel);
    ERR_FAIL_COND(_panel->_graph.is_null());

    _bookmarks = p_state.get("bookmarks", PackedInt64Array());

    Array breakpoints = p_state.get("breakpoints", Array());
    for (int i = 0; i < breakpoints.size(); i++) {
        const Dictionary& data = breakpoints[i];

        const int node_id = data.keys()[0];
        const bool status = data[node_id];

        if (!_panel->_graph->has_node(node_id)) {
            continue;
        }

        _breakpoint_state[node_id] = status;
        _breakpoints.push_back(node_id);
    }

    callable_mp_lambda(this, [this] {
        for (int bookmark : _bookmarks) {
            if (OrchestratorEditorGraphNode* node = _panel->find_node(bookmark)) {
                node->notify_bookmarks_changed();
            }
        }

        for (int breakpoint : _breakpoints) {
            if (OrchestratorEditorGraphNode* node = _panel->find_node(breakpoint)) {
                node->notify_breakpoints_changed();
            }
        }
    }).call_deferred();
}

void OrchestratorEditorGraphMarkers::_bind_methods() {

}
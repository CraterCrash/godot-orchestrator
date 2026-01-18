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
#ifndef ORCHESTRATOR_EDITOR_GRAPH_PANEL_STYLER_H
#define ORCHESTRATOR_EDITOR_GRAPH_PANEL_STYLER_H

#include "script/node_pin.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/ref_counted.hpp>

using namespace godot;

class OrchestratorEditorGraphNode;
class OrchestratorEditorGraphPanel;
class OrchestratorEditorGraphPin;

/// Provides customized styling behavior to an <code>OrchestratorEditorGraphPanel</code> widget.
///
/// This class specifically provides fading techniques when a user begins a drag connection within the panel,
/// and will also highlight connected nodes if that feature is toggled on.
///
class OrchestratorEditorGraphPanelStyler : public RefCounted {
    GDCLASS(OrchestratorEditorGraphPanelStyler, RefCounted);

    OrchestratorEditorGraphPanel* _panel;
    bool _highlight_selected_connections = false;
    bool _last_was_selection = false;

    float _full_opacity = 1.f;
    float _fade_opacity = 0.3f;
    Color _full_modulate = Color(1, 1, 1, 1);
    Color _half_modulate = Color(1, 1, 1, 0.5);

    void _settings_changed();

    void _connection_pin_drag_started(OrchestratorEditorGraphPin* p_drag_pin);
    void _connection_pin_drag_ended();
    void _connections_changed();

    void _node_selected(Node* p_node);
    void _node_deselected(Node* p_node);

    void _highlight_nodes(bool p_selected);

    int _get_all_with_opacity_count(OrchestratorEditorGraphNode* p_node, float p_opacity, EPinDirection p_direction = PD_MAX);

    void _set_node_modulate(OrchestratorEditorGraphNode* p_node, Color p_modulate_color);
    void _set_node_accept_opacity(OrchestratorEditorGraphNode* p_node, float p_opacity, OrchestratorEditorGraphPin* p_pin, EPinDirection p_direction = PD_MAX);
    void _set_node_all_opacity(OrchestratorEditorGraphNode* p_node, float p_opacity, EPinDirection p_direction = PD_MAX);

protected:
    static void _bind_methods();

public:
    void set_graph_panel(OrchestratorEditorGraphPanel* p_panel);

    OrchestratorEditorGraphPanelStyler();
};

#endif // ORCHESTRATOR_EDITOR_GRAPH_PANEL_STYLER_H
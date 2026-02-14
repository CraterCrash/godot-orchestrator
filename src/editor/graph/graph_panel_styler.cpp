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
#include "editor/graph/graph_panel_styler.h"

#include "common/macros.h"
#include "common/settings.h"
#include "core/godot/config/project_settings_cache.h"
#include "editor/graph/graph_panel.h"
#include "editor/graph/graph_pin.h"

void OrchestratorEditorGraphPanelStyler::_settings_changed() {
    _highlight_selected_connections = ORCHESTRATOR_GET("ui/nodes/highlight_selected_connections", false);
}

void OrchestratorEditorGraphPanelStyler::_connection_pin_drag_started(OrchestratorEditorGraphPin* p_drag_pin) {
    GUARD_NULL(p_drag_pin);

    const EPinDirection pin_dir = p_drag_pin->get_direction();
    const EPinDirection opposing_pin_dir = pin_dir == PD_Input ? PD_Output : PD_Input;
    const OrchestratorEditorGraphNode* source = p_drag_pin->get_graph_node();

    _panel->for_each<OrchestratorEditorGraphNode>([&] (OrchestratorEditorGraphNode* node) {
        _set_node_accept_opacity(node, _fade_opacity, p_drag_pin, opposing_pin_dir);
        _set_node_all_opacity(node, _fade_opacity, pin_dir);

        if (_get_all_with_opacity_count(node, _full_opacity, opposing_pin_dir) == 0 && node != source) {
            _set_node_modulate(node, _half_modulate);
        }
    });
}

void OrchestratorEditorGraphPanelStyler::_connection_pin_drag_ended() {
    _panel->for_each<OrchestratorEditorGraphNode>([&] (OrchestratorEditorGraphNode* node) {
        _set_node_modulate(node, _full_modulate);
        _set_node_all_opacity(node, _full_opacity);
    });
}

void OrchestratorEditorGraphPanelStyler::_connections_changed() {
    _highlight_nodes(_last_was_selection);
}

void OrchestratorEditorGraphPanelStyler::_node_selected(Node* p_node) {
    _last_was_selection = true;
    _highlight_nodes(true);
}

void OrchestratorEditorGraphPanelStyler::_node_deselected(Node* p_node) {
    _last_was_selection = false;
    _highlight_nodes(false);
}

void OrchestratorEditorGraphPanelStyler::_highlight_nodes(bool p_selected) {
    if (!_highlight_selected_connections) {
        return;
    }

    const Vector<OrchestratorEditorGraphNode*> selected_nodes = _panel->get_selected<OrchestratorEditorGraphNode>();

    if (selected_nodes.is_empty() && !p_selected) {
        _panel->for_each<OrchestratorEditorGraphNode>([&] (OrchestratorEditorGraphNode* node) {
            _set_node_modulate(node, _full_modulate);
            _set_node_all_opacity(node, _full_opacity);
        });
        return;
    }

    if (!selected_nodes.is_empty() && p_selected) {
        _panel->for_each<OrchestratorEditorGraphNode>([&] (OrchestratorEditorGraphNode* node) {
            _set_node_all_opacity(node, _fade_opacity);
        });
    }

    Vector<OrchestratorEditorGraphNode*> linked_nodes;
    for (OrchestratorEditorGraphNode* node : selected_nodes) {
        const HashSet<OrchestratorEditorGraphNode*> links = _panel->get_connected_nodes(node);
        for (OrchestratorEditorGraphNode* linked_node : links) {
            if (!linked_nodes.has(linked_node)) {
                linked_nodes.push_back(linked_node);
            }
        }
    }

    _panel->for_each<OrchestratorEditorGraphNode>([&] (OrchestratorEditorGraphNode* node) {
        const bool use_full_modulate = selected_nodes.has(node) || linked_nodes.has(node);
        _set_node_modulate(node, use_full_modulate ? _full_modulate : _half_modulate);
    });
}

int OrchestratorEditorGraphPanelStyler::_get_all_with_opacity_count(OrchestratorEditorGraphNode* p_node, float p_opacity, EPinDirection p_direction) {
    int count = 0;

    if (p_direction == PD_Input || p_direction == PD_MAX) {
        for (int i = 0; i < p_node->get_input_port_count(); i++) {
            if (p_node->is_slot_enabled_left(i)) {
                Color color = p_node->get_input_port_color(i);
                if (UtilityFunctions::is_equal_approx(color.a, p_opacity)) {
                    count++;
                }
            }
        }
    }

    if (p_direction == PD_Output || p_direction == PD_MAX) {
        for (int i = 0; i < p_node->get_output_port_count(); i++) {
            if (p_node->is_slot_enabled_right(i)) {
                Color color = p_node->get_output_port_color(i);
                if (UtilityFunctions::is_equal_approx(color.a, p_opacity)) {
                    count++;
                }
            }
        }
    }

    return count;
}

void OrchestratorEditorGraphPanelStyler::_set_node_modulate(OrchestratorEditorGraphNode* p_node, Color p_modulate_color) {
    p_node->set_modulate(p_modulate_color);
}

void OrchestratorEditorGraphPanelStyler::_set_node_accept_opacity(OrchestratorEditorGraphNode* p_node, float p_opacity, OrchestratorEditorGraphPin* p_pin, EPinDirection p_direction) {
    if (p_direction == PD_Input || p_direction == PD_MAX) {
        for (int i = 0; i < p_node->get_input_port_count(); i++) {
            if (p_node->is_slot_enabled_left(i)) {
                OrchestratorEditorGraphPin* pin = p_node->get_input_pin(i);
                if (pin && !_panel->are_pins_compatible(pin, p_pin)) {
                    p_node->set_slot_color_left(i, Color(p_node->get_input_port_color(i), p_opacity));
                }
            }
        }
    }

    if (p_direction == PD_Output || p_direction == PD_MAX) {
        for (int i = 0; i < p_node->get_output_port_count(); i++) {
            if (p_node->is_slot_enabled_right(i)) {
                OrchestratorEditorGraphPin* pin = p_node->get_output_pin(i);
                if (pin && !_panel->are_pins_compatible(p_pin, pin)) {
                    p_node->set_slot_color_right(i, Color(p_node->get_output_port_color(i), p_opacity));
                }
            }
        }
    }
}

void OrchestratorEditorGraphPanelStyler::_set_node_all_opacity(OrchestratorEditorGraphNode* p_node, float p_opacity, EPinDirection p_direction) {
    if (p_direction == PD_Input || p_direction == PD_MAX) {
        for (int i = 0; i < p_node->get_input_port_count(); i++) {
            if (p_node->is_slot_enabled_left(i)) {
                p_node->set_slot_color_left(i, Color(p_node->get_input_port_color(i), p_opacity));
            }
        }
    }

    if (p_direction == PD_Output || p_direction == PD_MAX) {
        for (int i = 0; i < p_node->get_output_port_count(); i++) {
            if (p_node->is_slot_enabled_right(i)) {
                p_node->set_slot_color_right(i, Color(p_node->get_output_port_color(i), p_opacity));
            }
        }
    }
}

void OrchestratorEditorGraphPanelStyler::set_graph_panel(OrchestratorEditorGraphPanel* p_panel) {
    GUARD_NULL(p_panel);
    _panel = p_panel;

    _panel->connect("connection_pin_drag_started", callable_mp_this(_connection_pin_drag_started));
    _panel->connect("connection_pin_drag_ended", callable_mp_this(_connection_pin_drag_ended));
    _panel->connect("node_selected", callable_mp_this(_node_selected));
    _panel->connect("node_deselected", callable_mp_this(_node_deselected));
    _panel->connect("connections_changed", callable_mp_this(_connections_changed));
}

void OrchestratorEditorGraphPanelStyler::_bind_methods() {
}

OrchestratorEditorGraphPanelStyler::OrchestratorEditorGraphPanelStyler() {
    // Updates internal configuration state when project settings change
    OrchestratorProjectSettingsCache::get_singleton()->connect("settings_changed", callable_mp_this(_settings_changed));
    _settings_changed();
}
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
#include "editor/graph/nodes/knot_node.h"

#include "common/macros.h"
#include "common/scene_utils.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

void OrchestratorEditorGraphNodeKnot::_position_offset_changed() {
    // In case the node is on a connection wire that has interpolated colors, this makes
    // sure that the knot's color matches its position on the connection wire.
    _color = _calculate_knot_color();
    queue_redraw();
}

Color OrchestratorEditorGraphNodeKnot::_calculate_knot_color() {
    if (!is_inside_tree()) {
        return _color;
    }

    const OScriptConnection connection(_connection_id);
    const String source_name = vformat("%d", connection.from_node);
    const String target_name = vformat("%d", connection.to_node);

    GraphNode* source = cast_to<GraphNode>(get_parent()->find_child(source_name, false, false));
    GraphNode* target = cast_to<GraphNode>(get_parent()->find_child(target_name, false, false));

    const Color source_color = source->get_output_port_color(static_cast<int32_t>(connection.from_port));
    const Color target_color = target->get_input_port_color(static_cast<int32_t>(connection.to_port));

    // A quick exit if the two have the same colors
    // There is no need to do distance calculation for linear interpolation
    if (source_color == target_color) {
        return source_color;
    }

    const Vector2 source_position = source->get_output_port_position(static_cast<int32_t>(connection.from_port))
        + source->get_position_offset();

    const Vector2 target_position = target->get_input_port_position(static_cast<int32_t>(connection.to_port))
        + target->get_position_offset();

    GraphEdit* parent = cast_to<GraphEdit>(get_parent());
    const PackedVector2Array points = parent->_get_connection_line(source_position, target_position);

    float distance = INFINITY;
    int knot_index = 0;
    for (int i = 0; i < points.size(); i++) {
        float computed_distance = points[i].distance_to(get_position_offset());
        if (computed_distance < distance) {
            knot_index = i;
            distance = computed_distance;
        }
    }

    float total_length = 0;
    float knot_length = 0;

    for (int i = 1; i < points.size(); i++) {
        float segment_length = points[i - 1].distance_to(points[i]);
        total_length += segment_length;

        if (i <= knot_index) {
            knot_length += segment_length;
        }
    }

    float t = 0;
    if (total_length != 0) {
        t = knot_length / total_length;
    }

    return source_color.lerp(target_color, t);
}

void OrchestratorEditorGraphNodeKnot::_gui_input(const Ref<InputEvent>& p_event) {
    const Ref<InputEventMouseButton> mb = p_event;
    if (is_inside_tree() && mb.is_valid() && mb->is_pressed()) {
        if (mb->get_button_index() == MOUSE_BUTTON_LEFT && mb->get_modifiers_mask().has_flag(KEY_MASK_CTRL)) {
            GraphEdit* parent = cast_to<GraphEdit>(get_parent());
            if (parent) {
                parent->emit_signal("delete_nodes_request", Array::make(get_name()));
            }
            accept_event();
            return;
        }
    }

    GraphElement::_gui_input(p_event);
}

bool OrchestratorEditorGraphNodeKnot::_has_point(const Vector2& p_point) const {
    return Rect2(-get_size() / 2.0, get_size()).has_point(p_point);
}

void OrchestratorEditorGraphNodeKnot::set_connection_id(uint64_t p_connection_id) {
    _connection_id = p_connection_id;
}

void OrchestratorEditorGraphNodeKnot::set_guid(const Guid& p_guid) {
    _guid = p_guid;
}

void OrchestratorEditorGraphNodeKnot::set_selected_color(const Color& p_color) {
    _selected_color = p_color;

    if (is_selected()) {
        queue_redraw();
    }
}

void OrchestratorEditorGraphNodeKnot::remove_knots_for_connection(uint64_t p_connection_id) {
    if (_connection_id == p_connection_id) {
        queue_free();
    }
}

void OrchestratorEditorGraphNodeKnot::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_READY: {
            set_tooltip_text(vformat("Connection ID: %d\nGUID: %s", _connection_id, _guid.to_string()));
            break;
        }
        case NOTIFICATION_DRAW: {
            _color = _calculate_knot_color();
            draw_texture(_icon, -get_size() / 2.0, is_selected() ? _selected_color : _color);
            break;
        }
    }
}

void OrchestratorEditorGraphNodeKnot::_bind_methods() {
}

OrchestratorEditorGraphNodeKnot::OrchestratorEditorGraphNodeKnot() {
    set_mouse_filter(MOUSE_FILTER_STOP);

    VBoxContainer* vbox = memnew(VBoxContainer);
    vbox->set_h_size_flags(SIZE_EXPAND_FILL);
    vbox->set_v_size_flags(SIZE_EXPAND_FILL);
    add_child(vbox);

    _icon = SceneUtils::get_editor_icon("GuiGraphNodePort");
    set_custom_minimum_size(_icon.is_valid() ? EDSCALE * _icon->get_size() : Size2(16, 16) * EDSCALE);

    connect("node_selected", callable_mp_cast(this, CanvasItem, queue_redraw));
    connect("node_deselected", callable_mp_cast(this, CanvasItem, queue_redraw));
    connect("position_offset_changed", callable_mp_this(_position_offset_changed));
}
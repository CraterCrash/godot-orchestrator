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
#include "editor/graph/nodes/reroute_graph_node.h"

#include "common/macros.h"
#include "common/scene_utils.h"
#include "core/godot/scene_string_names.h"
#include "editor/editor.h"
#include "editor/graph/graph_pin.h"
#include "editor/graph/graph_pin_factory.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/method_tweener.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/tween.hpp>

bool OrchestratorEditorGraphNodeReroute::_has_point(const Vector2& p_point) const {
    const Vector2 center = get_size() * 0.5f + Vector2(0, -16 * EDSCALE);
    return p_point.distance_to(center) <= 16 * EDSCALE;
}

void OrchestratorEditorGraphNodeReroute::_draw_port(int32_t p_slot_index, const Vector2i& p_position, bool p_left, const Color& p_color) {
    const Color rim_color = is_selected()
        ? SceneUtils::get_editor_color("selected_rim_color", "VSRerouteNode")
        : SceneUtils::get_editor_color("connection_rim_color", "GraphEdit");
    _draw_port2(p_slot_index, p_position, p_left, p_color, rim_color);
}

void OrchestratorEditorGraphNodeReroute::_update_styles() {
    for (int i = 0; i < get_titlebar_hbox()->get_child_count(); i++) {
        Control* control = cast_to<Control>(get_titlebar_hbox()->get_child(i));
        if (control) {
            control->hide();
        }
    }

    set_custom_minimum_size(Size2(0,0));
    set_size(Vector2());
}

void OrchestratorEditorGraphNodeReroute::_create_pin_widgets() {
    // A plain Control (not HBoxContainer) is enough as a reroute has no visible pin widgets.
    // It just needs a child node at slot 0 so GraphNode can anchor the port dots correctly.
    Control* slot_area = memnew(Control);
    slot_area->set_size(Vector2());
    slot_area->set_mouse_filter(MOUSE_FILTER_IGNORE);
    add_child(slot_area);

    OrchestratorEditorGraphPin* left_pin = OrchestratorEditorGraphPinFactory::create_pin_widget(get_graph_node()->find_pin(0, PD_Input));
    left_pin->set_graph_node(this);
    left_pin->hide();
    slot_area->add_child(left_pin);

    OrchestratorEditorGraphPin* right_pin = OrchestratorEditorGraphPinFactory::create_pin_widget(get_graph_node()->find_pin(0, PD_Output));
    right_pin->set_graph_node(this);
    right_pin->hide();
    slot_area->add_child(right_pin);

    const OrchestratorEditorGraphPinSlotInfo left = left_pin->get_slot_info();
    const OrchestratorEditorGraphPinSlotInfo right = right_pin->get_slot_info();

    Slot slot;
    slot.slot = 0;
    slot.row = slot_area;
    slot.left = left_pin;
    slot.right = right_pin;
    _slots[0] = slot;

    set_slot(0, true, left.type, left.color, true, right.type, right.color,
             SceneUtils::get_editor_icon(left.icon), SceneUtils::get_editor_icon(right.icon));
    set_size(Vector2());

    emit_signal("node_pins_changed", this);
}

void OrchestratorEditorGraphNodeReroute::_mouse_entered() {
    const Ref<Tween> tween = create_tween();
    tween->tween_method(callable_mp_this(set_icon_opacity), 0.0, 1.0, FADE_ANIMATION_LENGTH_SECONDS);
}

void OrchestratorEditorGraphNodeReroute::_mouse_exited() {
    const Ref<Tween> tween = create_tween();
    tween->tween_method(callable_mp_this(set_icon_opacity), 1.0, 0.0, FADE_ANIMATION_LENGTH_SECONDS);
}

void OrchestratorEditorGraphNodeReroute::set_icon_opacity(float p_opacity) {
    _icon_opacity = p_opacity;
    queue_redraw();
}

void OrchestratorEditorGraphNodeReroute::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_READY: {
            connect(SceneStringName(mouse_entered), callable_mp_this(_mouse_entered));
            connect(SceneStringName(mouse_exited), callable_mp_this(_mouse_exited));
            break;
        }
        case NOTIFICATION_DRAW: {
            const bool selected = is_selected();
            const Vector2 offset = Vector2(0, -16 * EDSCALE);
            const Color drag_bg_color = get_theme_color("drag_background", "VSRerouteNode");
            draw_circle(get_size() * 0.5 + offset, 16 * EDSCALE, Color(drag_bg_color, selected ? 1 : _icon_opacity), true, -1, true);

            const Ref<Texture2D> icon = SceneUtils::get_editor_icon("ToolMove");
            const Point2 icon_offset = -icon->get_size() * 0.5 + get_size() * 0.5 + offset;
            draw_texture(icon, icon_offset, Color(1, 1, 1, selected ? 1 : _icon_opacity));
            break;
        }
    }
}

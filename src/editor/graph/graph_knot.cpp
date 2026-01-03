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
#include "editor/graph/graph_knot.h"

#include "common/scene_utils.h"
#include "common/settings.h"

#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

void OrchestratorGraphKnot::_connections_changed(const String& p_caller)
{
    if (!_graph->get_orchestration()->get_connections().has(_connection))
        queue_free();
}

void OrchestratorGraphKnot::_position_changed()
{
    _knot->point = get_position_offset();
    emit_signal("knot_position_changed", _knot->point);
}

void OrchestratorGraphKnot::_node_selected()
{
    OrchestratorSettings* os = OrchestratorSettings::get_singleton();
    _draw_color = os->get_setting("ui/graph/knot_selected_color", Color(0.68f, 0.44f, 0.09f));
}

void OrchestratorGraphKnot::_node_deselected()
{
    _draw_color = _color;
}

void OrchestratorGraphKnot::set_graph(const Ref<OScriptGraph>& p_graph)
{
    _graph = p_graph;

    _graph->get_orchestration()->connect(
        "connections_changed", callable_mp(this, &OrchestratorGraphKnot::_connections_changed));
}

void OrchestratorGraphKnot::set_knot(const Ref<OrchestratorKnotPoint>& p_knot)
{
    _knot = p_knot;

    set_position_offset(_knot->point);
}

void OrchestratorGraphKnot::set_color(const Color& p_color)
{
    _color = p_color;

    if (!is_selected())
        _draw_color = _color;
}

void OrchestratorGraphKnot::_gui_input(const Ref<InputEvent>& p_event)
{
    Ref<InputEventMouseButton> mb = p_event;
    if (mb.is_valid() && mb->is_pressed())
    {
        if (mb->get_button_index() == MOUSE_BUTTON_LEFT && mb->get_modifiers_mask().has_flag(KEY_MASK_CTRL))
        {
            OrchestratorGraphEdit* parent = Object::cast_to<OrchestratorGraphEdit>(get_parent());
            if (parent)
            {
                emit_signal("knot_delete_requested", get_name());
                accept_event();
                return;
            }
        }
    }

    GraphElement::_gui_input(p_event);
}

bool OrchestratorGraphKnot::_has_point(const Vector2& p_point) const
{
    // Wrap mouse actions around the top-left point rather than around the snapped GraphElement
    return Rect2(-(get_size() /2), get_size()).has_point(p_point);
}

void OrchestratorGraphKnot::_notification(int p_what)
{
    switch (p_what)
    {
        case NOTIFICATION_ENTER_TREE:
        {
            const double scale = EditorInterface::get_singleton()->get_editor_scale();

            _icon = SceneUtils::get_editor_icon("GuiGraphNodePort");
            set_custom_minimum_size(_icon.is_valid() ? scale * _icon->get_size() : Size2(16, 16) * scale);
            break;
        }
        case NOTIFICATION_READY:
        {
            connect("position_offset_changed", callable_mp(this, &OrchestratorGraphKnot::_position_changed));
            connect("node_selected", callable_mp(this, &OrchestratorGraphKnot::_node_selected));
            connect("node_deselected", callable_mp(this, &OrchestratorGraphKnot::_node_deselected));
            break;
        }
        case NOTIFICATION_DRAW:
        {
            const Vector2 icon_position = -get_size() / 2.0;

            // todo: add rim color
            draw_texture(_icon, icon_position, _draw_color);
            break;
        }
    }
}

void OrchestratorGraphKnot::_bind_methods()
{
    ADD_SIGNAL(MethodInfo("knot_position_changed", PropertyInfo(Variant::VECTOR2, "position")));
    ADD_SIGNAL(MethodInfo("knot_delete_requested", PropertyInfo(Variant::STRING, "name")));
}

OrchestratorGraphKnot::OrchestratorGraphKnot()
{
    set_mouse_filter(MOUSE_FILTER_STOP);

    VBoxContainer* vbox = memnew(VBoxContainer);
    vbox->set_h_size_flags(SIZE_EXPAND_FILL);
    vbox->set_v_size_flags(SIZE_EXPAND_FILL);
    add_child(vbox);
}

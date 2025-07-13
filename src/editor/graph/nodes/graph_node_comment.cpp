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
#include "graph_node_comment.h"

#include "editor/graph/graph_edit.h"

#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>

OrchestratorGraphNodeComment::OrchestratorGraphNodeComment(OrchestratorGraphEdit* p_graph, const Ref<OScriptNodeComment>& p_node)
    : OrchestratorGraphNode(p_graph, p_node)
    , _comment_node(p_node)
{
    // Since _has_point is const, we need to cache this
    _title_hbox = get_titlebar_hbox();

    MarginContainer* container = memnew(MarginContainer);
    container->add_theme_constant_override("margin_top", 4);
    container->add_theme_constant_override("margin_bottom", 4);
    container->add_theme_constant_override("margin_left", 10);
    container->add_theme_constant_override("margin_right", 10);
    add_child(container);

    _label = memnew(Label);
    container->add_child(_label);

    if (_comment_node->is_title_center_aligned())
    {
        HBoxContainer* hbox = get_titlebar_hbox();
        if (Label* control = Object::cast_to<Label>(hbox->get_child(0)))
            control->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
    }
}

void OrchestratorGraphNodeComment::_bind_methods()
{
}

void OrchestratorGraphNodeComment::_update_pins()
{
    if (!_comment_node.is_valid())
        return;

    HorizontalAlignment alignment = HORIZONTAL_ALIGNMENT_LEFT;
    if (_comment_node->is_title_center_aligned())
        alignment = HORIZONTAL_ALIGNMENT_CENTER;

    HBoxContainer* hbox = get_titlebar_hbox();
    for (int i = 0; i < hbox->get_child_count(); i++)
    {
        if (Label* label = Object::cast_to<Label>(hbox->get_child(i)))
        {
            label->set_horizontal_alignment(alignment);
            break;
        }
    }

    Ref<StyleBoxFlat> panel = get_theme_stylebox("panel")->duplicate(true);
    panel->set_bg_color(_comment_node->get_background_color());
    add_theme_stylebox_override("panel", panel);

    Ref<StyleBoxFlat> panel_selected = get_theme_stylebox("panel_selected")->duplicate(true);
    panel_selected->set_bg_color(_comment_node->get_background_color());
    add_theme_stylebox_override("panel_selected", panel_selected);

    const int font_size = _comment_node->get_font_size();
    _label->add_theme_font_size_override("font_size", font_size != 0 ? font_size : 14);
    _label->set_text(_comment_node->get("comments"));
    _label->add_theme_color_override("font_color", _comment_node->get_text_color());
}

void OrchestratorGraphNodeComment::_notification(int p_what)
{
    #if GODOT_VERSION < 0x040202
    OrchestratorGraphNode::_notification(p_what);
    #endif

    if (p_what == NOTIFICATION_READY)
        connect("raise_request", callable_mp(this, &OrchestratorGraphNodeComment::_on_raise_request));
}

bool OrchestratorGraphNodeComment::_has_point(const Vector2& p_point) const
{
    Ref<StyleBox> sb_panel = get_theme_stylebox("panel");
    Ref<StyleBox> sb_titlebar = get_theme_stylebox("titlebar");
    Ref<Texture2D> resizer = get_theme_icon("resizer");

    if (Rect2(get_size() - resizer->get_size(), resizer->get_size()).has_point(p_point))
        return true;

    // Grab titlebar
    int titlebar_height = _title_hbox->get_size().height + sb_titlebar->get_minimum_size().height;
    if (Rect2(0, 0, get_size().width, titlebar_height).has_point(p_point))
        return true;

    // Allow grabbing on all sides of comment
    Rect2 rect = Rect2(0, 0, get_size().width, get_size().height);
    Rect2 no_drag_rect = rect.grow(-16);

    if (rect.has_point(p_point) && !no_drag_rect.has_point(p_point))
        return true;

    return false;
}

void OrchestratorGraphNodeComment::_gui_input(const Ref<InputEvent>& p_event)
{
     Ref<InputEventMouseButton> mb = p_event;
     if (mb.is_valid())
     {
         if (mb->is_double_click() && mb->get_button_index() == MOUSE_BUTTON_LEFT)
         {
             if (is_group_selected())
                 deselect_group();
             else
                 select_group();

             accept_event();
             return;
         }
     }
     return OrchestratorGraphNode::_gui_input(p_event);
}

void OrchestratorGraphNodeComment::_on_raise_request()
{
    // When comment nodes are raised, their order must always be behind the connection layer.
    // This guarantees that connection wires render properly.
    if (OrchestratorGraphEdit* graph_edit = Object::cast_to<OrchestratorGraphEdit>(get_parent()))
    {
        int position = 0;
        for (int index = 0; index < graph_edit->get_child_count(); index++)
        {
            Node* child = graph_edit->get_child(index);

            OrchestratorGraphNodeComment* comment = Object::cast_to<OrchestratorGraphNodeComment>(child);
            if (comment && comment != this)
                graph_edit->call_deferred("move_child", comment, position++);
        }

        graph_edit->call_deferred("move_child", this, position);
        graph_edit->call_deferred("move_child", graph_edit->find_child("_connection_layer", false, false), position + 1);
    }
}

bool OrchestratorGraphNodeComment::is_group_selected()
{
    List<GraphElement*> intersections = get_elements_within_global_rect();
    for (GraphElement* child : intersections)
        if (!child->is_selected())
            return false;
    return true;
}

void OrchestratorGraphNodeComment::select_group()
{
    // Select all child nodes
    List<GraphElement*> intersections = get_elements_within_global_rect();
    for (GraphElement* child : intersections)
        child->set_selected(true);
}

void OrchestratorGraphNodeComment::deselect_group()
{
    // Deselects all child nodes
    List<GraphElement*> intersections = get_elements_within_global_rect();
    for (GraphElement* child : intersections)
        child->set_selected(false);
}
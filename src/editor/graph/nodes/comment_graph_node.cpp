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
#include "editor/graph/nodes/comment_graph_node.h"

#include "common/macros.h"
#include "common/scene_utils.h"
#include "editor/graph/graph_panel.h"
#include "script/nodes/utilities/comment.h"

#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/margin_container.hpp>

void OrchestratorEditorGraphNodeComment::_raise_request()
{
    // Comment nodes must always be behind the connection layer so that the connection lines are drawn
    // atop of the comment node rather than behind it.
    OrchestratorEditorGraphPanel* panel = cast_to<OrchestratorEditorGraphPanel>(get_parent());
    GUARD_NULL(panel);

    int position = 0;
    panel->for_each<OrchestratorEditorGraphNodeComment>([&] (OrchestratorEditorGraphNodeComment *node) {
        if (node != this)
            panel->call_deferred("move_child", node, position++);
    });

    panel->call_deferred("move_child", this, position);
    panel->call_deferred("move_child", panel->get_connection_layer_node(), position + 1);
}

void OrchestratorEditorGraphNodeComment::_gui_input(const Ref<InputEvent>& p_event)
{
    const Ref<InputEventMouseButton> mb = p_event;
    if (mb.is_valid() && mb->is_double_click() && mb->get_button_index() == MOUSE_BUTTON_LEFT)
    {
        bool group_selected = true;
        const Vector<GraphElement*> overlapping_elements = get_overlapping_elements();
        for (GraphElement* element : overlapping_elements)
        {
            if (!element->is_selected())
            {
                group_selected = false;
                break;
            }
        }

        for (GraphElement* element : overlapping_elements)
            element->set_selected(!group_selected);

        accept_event();
        return;
    }

    OrchestratorEditorGraphNode::_gui_input(p_event);
}

bool OrchestratorEditorGraphNodeComment::_has_point(const Vector2& p_point) const
{
    const Ref<StyleBox> panel_sbox = get_theme_stylebox("panel");
    const Ref<StyleBox> titlebar_sbox = get_theme_stylebox("titlebar");
    const Ref<Texture2D> resizer = get_theme_icon("resizer");

    ERR_FAIL_COND_V_MSG(!panel_sbox.is_valid(), false, "Panel stylebox is invalid");
    ERR_FAIL_COND_V_MSG(!titlebar_sbox.is_valid(), false, "Titlebar stylebox is invalid");
    ERR_FAIL_COND_V_MSG(!resizer.is_valid(), false, "Resizer is invalid");

    const Rect2 resizer_area = Rect2(get_size() - resizer->get_size(), resizer->get_size());
    if (resizer_area.has_point(p_point))
        return true;

    const int titlebar_height = _title_hbox->get_size().height + titlebar_sbox->get_minimum_size().height;
    const Rect2 titlebar_area = Rect2(0, 0, get_size().width, titlebar_height);
    if (titlebar_area.has_point(p_point))
        return true;

    const Rect2 area = Rect2(0, 0, get_size().width, get_size().height);
    const Rect2 no_drag_rect = area.grow(-16);
    if (area.has_point(p_point) && !no_drag_rect.has_point(p_point))
        return true;

    return false;
}

void OrchestratorEditorGraphNodeComment::update()
{
    _update_titlebar();

    const Ref<OScriptNodeComment> comment_node = get_graph_node();
    if (comment_node.is_valid())
    {
        const int node_font_size = comment_node->get_font_size();
        const int font_size = node_font_size <= 0 ? 14 : node_font_size;

        if (!_theme_cache.panel.is_valid())
        {
            _theme_cache.panel = get_theme_stylebox("panel")->duplicate();
            ERR_FAIL_COND(!_theme_cache.panel.is_valid());
        }

        if (!_theme_cache.panel_selected.is_valid())
        {
            _theme_cache.panel_selected = get_theme_stylebox("panel_selected")->duplicate();
            ERR_FAIL_COND(!_theme_cache.panel_selected.is_valid());
        }

        _theme_cache.panel->set_bg_color(comment_node->get_background_color());
        _theme_cache.panel_selected->set_bg_color(comment_node->get_background_color());

        begin_bulk_theme_override();
        add_theme_stylebox_override("panel", _theme_cache.panel);
        add_theme_stylebox_override("panel_selected", _theme_cache.panel_selected);
        end_bulk_theme_override();

        _text->add_theme_font_size_override("font_size", font_size);
        _text->set_text(comment_node->get("comments"));
        _text->add_theme_color_override("font_color", comment_node->get_text_color());

        HorizontalAlignment alignment = HORIZONTAL_ALIGNMENT_LEFT;
        if (comment_node->is_title_center_aligned())
            alignment = HORIZONTAL_ALIGNMENT_CENTER;

        for (int i = 0; i < _title_hbox->get_child_count(); i++)
        {
            if (Label* label = cast_to<Label>(_title_hbox->get_child(i)))
            {
                label->set_horizontal_alignment(alignment);
                break;
            }
        }
    }
}

void OrchestratorEditorGraphNodeComment::_bind_methods()
{
}

OrchestratorEditorGraphNodeComment::OrchestratorEditorGraphNodeComment()
{
    // Used in _has_point and its const, so cache
    _title_hbox = get_titlebar_hbox();

    MarginContainer* container = memnew(MarginContainer);
    container->add_theme_constant_override("margin_top", 4);
    container->add_theme_constant_override("margin_bottom", 4);
    container->add_theme_constant_override("margin_left", 10);
    container->add_theme_constant_override("margin_right", 10);
    add_child(container);

    _text = memnew(Label);
    container->add_child(_text);

    connect("raise_request", callable_mp_this(_raise_request));
    connect("ready", callable_mp_this(_raise_request));
}

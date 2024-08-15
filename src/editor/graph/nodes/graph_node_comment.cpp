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

#include "common/macros.h"
#include "common/scene_utils.h"
#include "editor/graph/graph_edit.h"
#include "editor/plugins/orchestrator_editor_plugin.h"
#include "editor/theme/theme_cache.h"

#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/texture_rect.hpp>

OrchestratorGraphNodeComment::OrchestratorGraphNodeComment(OrchestratorGraphEdit* p_graph, const Ref<OScriptNodeComment>& p_node)
    : OrchestratorGraphNode(p_graph, p_node)
    , _comment_node(p_node)
{
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
    ClassDB::bind_method(D_METHOD("raise_request_node_reorder"), &OrchestratorGraphNodeComment::raise_request_node_reorder);
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
    #if GODOT_VERSION < 0x040300
    OrchestratorGraphNode::_notification(p_what);
    #endif

    if (p_what == NOTIFICATION_READY)
        connect("raise_request", callable_mp(this, &OrchestratorGraphNodeComment::_on_raise_request));
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
    // This call must be deferred because the Godot GraphNode implementation raises this node
    // after this method has been called, so we want to guarantee that we reorder the nodes
    // of the scene after this node has been properly raised.
    call_deferred("raise_request_node_reorder");
}

bool OrchestratorGraphNodeComment::is_group_selected()
{
    List<OrchestratorGraphNode*> intersections = get_nodes_within_global_rect();
    for (OrchestratorGraphNode* child : intersections)
        if (!child->is_selected())
            return false;
    return true;
}

void OrchestratorGraphNodeComment::select_group()
{
    // Select all child nodes
    List<OrchestratorGraphNode*> intersections = get_nodes_within_global_rect();
    for (OrchestratorGraphNode* child : intersections)
        child->set_selected(true);
}

void OrchestratorGraphNodeComment::deselect_group()
{
    // Deselects all child nodes
    List<OrchestratorGraphNode*> intersections = get_nodes_within_global_rect();
    for (OrchestratorGraphNode* child : intersections)
        child->set_selected(false);
}

void OrchestratorGraphNodeComment::raise_request_node_reorder()
{
    // This guarantees that any node that intersects with a comment node will be repositioned
    // in the scene after the comment, so that the rendering order appears correct.
    List<OrchestratorGraphNode*> intersections = get_nodes_within_global_rect();
    for (OrchestratorGraphNode* node : intersections)
        get_parent()->move_child(node, -1);
}

#if GODOT_VERSION >= 0x040300
void OrchestratorGraphFrameComment::_node_moved(Vector2 p_old_pos, Vector2 p_new_pos)
{
    _node->set_position(p_new_pos);
}

void OrchestratorGraphFrameComment::_node_resized()
{
    _node->set_size(get_size());
}

void OrchestratorGraphFrameComment::_script_node_changed()
{
    Label* title = Object::cast_to<Label>(get_titlebar_hbox()->get_child(1));
    if (title)
    {
        Ref<Font> label_bold_font = SceneUtils::get_editor_font("main_bold_msdf");

        title->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_LEFT);
        title->add_theme_font_size_override("font_size", 16);
        title->add_theme_font_override("font", label_bold_font);

        if (_node->is_title_center_aligned())
            title->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
    }

    set_title(_node->get_node_title());

    if (_text)
    {
        _text->set_text(_node->get("comments"));

        const int font_size = _node->get_font_size();
        if (_text->get_theme_font_size("font_size") != font_size)
            _text->add_theme_font_size_override("font_size", font_size != 0 ? font_size : 14);

        if (_text->get_theme_color("font_color") != _node->get_text_color())
            _text->add_theme_color_override("font_color", _node->get_text_color());
    }

    if (_icon)
        _icon->set_texture(SceneUtils::get_editor_icon(_node->get_icon()));
}

void OrchestratorGraphFrameComment::_update_theme()
{
    Ref<OrchestratorThemeCache> cache = OrchestratorPlugin::get_singleton()->get_theme_cache();
    if (cache.is_valid())
    {
        // Cache these for draw calls
        _theme_cache.titlebar = cache->get_theme_stylebox("titlebar", "GraphNode_comment");
        _theme_cache.titlebar_selected = cache->get_theme_stylebox("titlebar_selected", "GraphNode_comment");

        begin_bulk_theme_override();

        Ref<StyleBoxFlat> panel = cache->get_theme_stylebox("panel", "GraphFrame");
        if (panel.is_valid())
        {
            Ref<StyleBoxFlat> new_panel = panel->duplicate(true);
            new_panel->set_bg_color(_node->get_background_color());
            add_theme_stylebox_override("panel", new_panel);
        }

        Ref<StyleBoxFlat> panel_selected = cache->get_theme_stylebox("panel_selected", "GraphFrame");
        if (panel_selected.is_valid())
        {
            Ref<StyleBoxFlat> new_panel_selected = panel_selected->duplicate(true);
            new_panel_selected->set_bg_color(_node->get_background_color());
            add_theme_stylebox_override("panel_selected", new_panel_selected);
        }
        end_bulk_theme_override();
        queue_redraw();
    }
}

void OrchestratorGraphFrameComment::_notification(int p_what)
{
    if (p_what == NOTIFICATION_READY)
    {
        // Used to replicate size/position state to underlying node resource
        OCONNECT(this, "dragged", callable_mp(this, &OrchestratorGraphFrameComment::_node_moved));
        OCONNECT(this, "resized", callable_mp(this, &OrchestratorGraphFrameComment::_node_resized));

        // Notified when node attributes change
        OCONNECT(_node, "changed", callable_mp(this, &OrchestratorGraphFrameComment::_script_node_changed));

        Ref<OrchestratorThemeCache> cache = OrchestratorPlugin::get_singleton()->get_theme_cache();
        if (cache.is_valid())
            OCONNECT(cache, "theme_changed", callable_mp(this, &OrchestratorGraphFrameComment::_update_theme));

        _update_theme();
    }
    else if (p_what == NOTIFICATION_DRAW)
    {
        if (is_selected() && _theme_cache.titlebar_selected.is_valid())
            draw_style_box(_theme_cache.titlebar_selected, Rect2(0, 0, get_size().x, TITLEBAR_HEIGHT));
        else if (_theme_cache.titlebar.is_valid())
            draw_style_box(_theme_cache.titlebar, Rect2(0, 0, get_size().x, TITLEBAR_HEIGHT));
    }
}

void OrchestratorGraphFrameComment::_bind_methods()
{
}

OrchestratorGraphFrameComment::OrchestratorGraphFrameComment(OrchestratorGraphEdit* p_graph, const Ref<OScriptNodeComment>& p_node)
    : _graph(p_graph)
    , _node(p_node)
{
    set_meta("__script_node", p_node);
    set_tint_color_enabled(false);

    MarginContainer* margin = memnew(MarginContainer);
    margin->add_theme_constant_override("margin_left", 5);
    get_titlebar_hbox()->add_child(margin);
    get_titlebar_hbox()->move_child(margin, 0);

    _icon = memnew(TextureRect);
    _icon->set_custom_minimum_size(Vector2i(16, 16));
    _icon->set_h_size_flags(SIZE_SHRINK_BEGIN);
    _icon->set_v_size_flags(SIZE_SHRINK_CENTER);
    _icon->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
    margin->add_child(_icon);

    _text = memnew(Label);
    _text->set_vertical_alignment(VERTICAL_ALIGNMENT_TOP);
    _text->set_v_size_flags(SIZE_SHRINK_BEGIN);
    add_child(_text);

    get_titlebar_hbox()->set_custom_minimum_size(Vector2(0, TITLEBAR_HEIGHT));
    get_titlebar_hbox()->set_v_size_flags(SIZE_SHRINK_CENTER);

    _script_node_changed();
}
#endif
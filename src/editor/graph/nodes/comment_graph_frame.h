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

#include "orchestration/nodes/comment.h"

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/graph_frame.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/popup_panel.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/text_edit.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

using namespace godot;

class OrchestratorEditorContextMenu;
class OrchestratorEditorGraphPanel;

/// The editor visual representation of a comment/frame node. This extends the Godot <code>GraphFrame</code>
/// native class to supply auto-shrink, tint, attach/detach behavior out of the box.
///
class OrchestratorEditorGraphFrame : public GraphFrame {
    friend class OrchestratorEditorGraphPanel;

    GDCLASS(OrchestratorEditorGraphFrame, GraphFrame);

    struct ThemeCache {
        Ref<StyleBoxFlat> panel;
        Ref<StyleBoxFlat> panel_selected;
        Ref<StyleBox> titlebar;
        Ref<Font> font;
        int font_size = 0;
        Color font_color;
        String icon_name;
    } theme_cache;

    Ref<OScriptNodeComment> _comment;
    HBoxContainer* _title_hbox = nullptr;
    Label* _title_label = nullptr;
    TextureRect* _icon = nullptr;
    Label* _placeholder = nullptr;
    bool _has_attached_nodes = false;
    Label* _comments_text = nullptr;
    VBoxContainer* _vbox = nullptr;
    bool _theme_updating = false;

    //~ Begin GraphElement Signals
    void _frame_selected();
    //~ End GraphElement Signals

    //~ Begin GraphFrame Signals
    void _frame_dragged(const Vector2& p_from, const Vector2& p_to);
    void _frame_resize_request(const Vector2& p_size);
    void _frame_resize_end(const Vector2& p_size);
    void _frame_resized();
    void _frame_autoshrink_changed(const Vector2& p_size);
    //~ End GraphFrame Signals

    void _update_theme();

    void _toggle_autoshrink();
    void _toggle_tint();

    void _frame_title_text_changed(const String& p_text);
    void _change_frame_title();
    void _open_change_comment_text();
    void _comment_text_changed(TextEdit* p_editor);
    void _show_tint_color_picker();

    float _compute_top_margin() const;
    void _apply_panel_expand_margin_deferred();
    void _apply_panel_expand_margin();

    float _compute_width(const Vector2& p_size) const;
    String _wrap_text(const String& p_text, float p_width) const;
    void _update_comments_text(const Vector2& p_frame_size, bool p_toggle_visibility = false);

    PopupPanel* _show_popup_at_mouse(Control* p_content);

protected:
    static void _bind_methods();

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

    void _update_from_model();
    void _update_placeholder_visibility();

public:
    //~ Begin Control Interface
    void _gui_input(const Ref<InputEvent>& p_event) override;
    //~ End Control Interface

    void build_context_menu(OrchestratorEditorContextMenu* p_menu);

    Ref<OScriptNodeComment> get_comment() const { return _comment; }
    void set_node(const Ref<OrchestrationGraphNode>& p_node);

    void update();
    void update_placeholder(bool p_has_attached_nodes);

    OrchestratorEditorGraphFrame();
};
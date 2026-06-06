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
#include "editor/graph/nodes/comment_graph_frame.h"

#include "common/callable_lambda.h"
#include "common/macros.h"
#include "common/scene_utils.h"
#include "common/settings.h"
#include "core/godot/core_string_names.h"
#include "core/godot/io/resource_uid.h"
#include "core/godot/scene_string_names.h"
#include "editor/editor.h"
#include "editor/graph/graph_panel.h"
#include "editor/gui/context_menu.h"
#include "orchestration/orchestration.h"

#include <godot_cpp/classes/color_picker.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/popup_panel.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>

void OrchestratorEditorGraphFrame::_frame_selected() {
    if (_comment.is_valid() && _comment->can_inspect_node_properties()) {
        EI->edit_resource(_comment->get_inspect_object());
    }
}

void OrchestratorEditorGraphFrame::_frame_dragged(const Vector2& p_from, const Vector2& p_to) {
    if (_comment.is_valid()) {
        _comment->set_position(p_to);
    }
}

void OrchestratorEditorGraphFrame::_frame_resize_request(const Vector2& p_size) {
    if (!_comments_text || !_comment.is_valid()) {
        return;
    }

    // Re-wrap text to the new frame width so the label's height adapts.
    // Hide-and-reshow forces the engine to re-evaluate the label's contribution
    // to the titlebar minimum without it transiently inflating from the old width.
    _update_comments_text(p_size, true);

    _apply_panel_expand_margin();
}

void OrchestratorEditorGraphFrame::_frame_resize_end(const Vector2& p_size) {
    if (_comment.is_valid()) {
        _comment->set_size(p_size);
    }
}

void OrchestratorEditorGraphFrame::_frame_resized() {
    if (_comment.is_valid() && is_autoshrink_enabled()) {
        _comment->set_size(get_size());
        _update_comments_text(get_size(), true);
        _apply_panel_expand_margin();
    }
}

void OrchestratorEditorGraphFrame::_frame_autoshrink_changed(const Vector2& p_size) {
    if (_comment.is_valid()) {
        _comment->set_autoshrink_enabled(is_autoshrink_enabled());
        _comment->set_size(p_size);
    }
}

void OrchestratorEditorGraphFrame::_update_theme() {
    if (_theme_updating) {
        return;
    }

    _theme_updating = true;

    const int corner_radius = ORCHESTRATOR_GET("theme/nodes/border_radius", 4);
    const int border_width = ORCHESTRATOR_GET("theme/nodes/border_width", 2);
    const Color border_color = ORCHESTRATOR_GET("theme/nodes/border_color", Color(0, 0, 0));
    const Color select_border_color = ORCHESTRATOR_GET("theme/nodes/selected_border_color", Color(0.68f, 0.44f, 0.09f));

    // Duplicate the default GraphFrame StyleBoxes so that we can mutate expand_margin without
    // affecting the shared editor theme resources.
    const Ref<StyleBox> panel_sb = SceneUtils::get_editor_stylebox(SceneStringName(panel), "GraphFrame");
    if (panel_sb.is_valid()) {
        theme_cache.panel = panel_sb->duplicate();
        theme_cache.panel->set_border_color(border_color);
        theme_cache.panel->set_border_width_all(border_width);
        theme_cache.panel->set_corner_radius_all(corner_radius);
    }

    const Ref<StyleBox> panel_selected_sb = SceneUtils::get_editor_stylebox("panel_selected", "GraphFrame");
    if (panel_selected_sb.is_valid()) {
        theme_cache.panel_selected = panel_selected_sb->duplicate();
        theme_cache.panel_selected->set_border_color(select_border_color);
        theme_cache.panel_selected->set_border_width_all(border_width);
        theme_cache.panel_selected->set_corner_radius_all(corner_radius);
    }

    theme_cache.titlebar = SceneUtils::get_editor_stylebox("titlebar", "GraphFrame");

    if (_comments_text) {
        theme_cache.font = _comments_text->get_theme_font(SceneStringName(font));
    }

    begin_bulk_theme_override();
    add_theme_stylebox_override(SceneStringName(panel), theme_cache.panel);
    add_theme_stylebox_override("panel_selected", theme_cache.panel_selected);
    end_bulk_theme_override();
}

void OrchestratorEditorGraphFrame::_toggle_autoshrink() {
    if (_comment.is_valid()) {
        _comment->set_autoshrink_enabled(!_comment->is_autoshrink_enabled());
    }
}

void OrchestratorEditorGraphFrame::_toggle_tint() {
    if (_comment.is_valid()) {
        _comment->set_tint_color_enabled(!_comment->is_tint_color_enabled());
    }
}

void OrchestratorEditorGraphFrame::_frame_title_text_changed(const String& p_text) {
    if (_comment.is_valid()) {
        _comment->set_title_text(p_text);
    }
}

void OrchestratorEditorGraphFrame::_change_frame_title() {
    VBoxContainer* vbox = memnew(VBoxContainer);

    Label* label = memnew(Label);
    label->set_text("Frame Title:");
    vbox->add_child(label);

    LineEdit* edit = memnew(LineEdit);
    edit->set_expand_to_text_length_enabled(true);
    edit->set_select_all_on_focus(true);
    edit->set_text(_comment->get_title_text());
    edit->connect(SceneStringName(text_changed), callable_mp_this(_frame_title_text_changed));
    edit->connect(SceneStringName(text_submitted), callable_mp_this(_frame_title_text_changed));
    vbox->add_child(edit);

    _show_popup_at_mouse(vbox);
    edit->grab_focus();
}

void OrchestratorEditorGraphFrame::_open_change_comment_text() {
    TextEdit* editor = memnew(TextEdit);
    editor->connect(SceneStringName(text_changed), callable_mp_this(_comment_text_changed).bind(editor));
    editor->set_line_wrapping_mode(TextEdit::LineWrappingMode::LINE_WRAPPING_BOUNDARY);

    AcceptDialog* dialog = memnew(AcceptDialog);
    dialog->add_child(editor);
    dialog->set_title("Edit Comment Text");
    dialog->connect(SceneStringName(confirmed), callable_mp_cast(dialog, Node, queue_free));
    dialog->connect(SceneStringName(canceled), callable_mp_cast(dialog, Node, queue_free));
    get_titlebar_hbox()->add_child(dialog);

    dialog->popup_centered_clamped(Size2(1000, 900) * EDSCALE, 0.8);
    editor->set_text(_comment->get_comments_text());
    editor->grab_focus();
}

void OrchestratorEditorGraphFrame::_comment_text_changed(TextEdit* p_editor) {
    _comment->set_comments_text(p_editor->get_text());
}

void OrchestratorEditorGraphFrame::_show_tint_color_picker() {
    ColorPicker* picker = memnew(ColorPicker);
    picker->set_pick_color(_comment->get_tint_color());
    picker->connect("color_changed", callable_mp_lambda(this, [this](const Color& c) {
        _comment->set_tint_color(c);
    }));

    _show_popup_at_mouse(picker);
}

float OrchestratorEditorGraphFrame::_compute_top_margin() const {
    // Total height of the titlebar HBox (title row + static text VBox) plus the titlebar's
    // StyleBox's own vertical margins. The body panel's expand_margin_top is set to this
    // value so the panel visually extends upward to cover the titlebar region, presenting
    // a unified frame.
    float total = _title_hbox->get_combined_minimum_size().y;

    if (theme_cache.titlebar.is_valid()) {
        total += theme_cache.titlebar->get_minimum_size().y;
    }

    return total;
}

void OrchestratorEditorGraphFrame::_apply_panel_expand_margin_deferred() {
    callable_mp_this(_apply_panel_expand_margin).call_deferred();
}

void OrchestratorEditorGraphFrame::_apply_panel_expand_margin() {
    const float panel_top_margin = _compute_top_margin();

    if (theme_cache.panel.is_valid()) {
        theme_cache.panel->set_expand_margin(SIDE_TOP, panel_top_margin);
    }

    if (theme_cache.panel_selected.is_valid()) {
        theme_cache.panel_selected->set_expand_margin(SIDE_TOP, panel_top_margin);
    }

    // Re-evaluate minimum size, then notify GraphEdit so the frame rect updates
    // for font size or text-content changes.
    update_minimum_size();
    emit_signal("autoshrink_changed", get_size());

    queue_redraw();
}

float OrchestratorEditorGraphFrame::_compute_width(const Vector2& p_size) const {
    const Ref<StyleBox> panel_sb = theme_cache.panel;

    const float left = panel_sb.is_valid() ? panel_sb->get_margin(SIDE_LEFT) : 0;
    const float right = panel_sb.is_valid() ? panel_sb->get_margin(SIDE_RIGHT) : 0;

    return MAX(0, p_size.x - left - right);
}

String OrchestratorEditorGraphFrame::_wrap_text(const String& p_text, float p_width) const {
    if (p_text.is_empty() || p_width <= 0) {
        return p_text;
    }

    if (theme_cache.font.is_null() || theme_cache.font_size == 0) {
        return p_text;
    }

    PackedStringArray output_lines;
    PackedStringArray source_lines = p_text.split("\n", true);
    for (const String& source_line : source_lines) {
        if (source_line.is_empty()) {
            output_lines.push_back("");
            continue;
        }

        String current;
        const PackedStringArray words = source_line.split(" ", false);
        for (const String& word : words) {
            String candidate = current.is_empty() ? word : current + " " + word;

            real_t w = theme_cache.font->get_string_size(candidate, HORIZONTAL_ALIGNMENT_LEFT, -1, theme_cache.font_size).x;
            if (w <= p_width || current.is_empty()) {
                current = candidate;
            } else {
                output_lines.push_back(current);
                current = word;
            }
        }

        if (!current.is_empty()) {
            output_lines.push_back(current);
        }
    }

    return String("\n").join(output_lines);
}

void OrchestratorEditorGraphFrame::_update_comments_text(const Vector2& p_frame_size, bool p_toggle_visibility) {
    if (!_comments_text || !_comment.is_valid()) {
        return;
    }

    if (p_toggle_visibility) {
        _comments_text->set_visible(false);
    }

    _comments_text->set_text(_wrap_text(_comment->get_comments_text(), _compute_width(p_frame_size)));

    if (p_toggle_visibility) {
        _comments_text->set_visible(true);
    }
}

PopupPanel* OrchestratorEditorGraphFrame::_show_popup_at_mouse(Control* p_content) {
    PopupPanel* panel = memnew(PopupPanel);
    panel->set_position(get_screen_position() + get_local_mouse_position());
    panel->add_child(p_content);

    p_content->reset_size();
    panel->reset_size();

    panel->connect(SceneStringName(focus_exited), callable_mp_cast(panel, Node, queue_free));
    panel->connect("popup_hide", callable_mp_cast(panel, Node, queue_free));

    get_titlebar_hbox()->add_child(panel);
    panel->popup();

    return panel;
}

void OrchestratorEditorGraphFrame::_update_from_model() {
    if (_comment.is_null()) {
        return;
    }

    // GraphFrame internally compares state and only triggers when values differ
    set_title(_comment->get_title_text());
    set_autoshrink_enabled(_comment->is_autoshrink_enabled());

    if (is_tint_color_enabled() != _comment->is_tint_color_enabled()) {
        set_tint_color_enabled(_comment->is_tint_color_enabled());
    }

    if (is_tint_color_enabled() && !get_tint_color().is_equal_approx(_comment->get_tint_color())) {
        set_tint_color(_comment->get_tint_color());
    }

    if (theme_cache.icon_name != _comment->get_icon()) {
        theme_cache.icon_name = _comment->get_icon();

        Ref<Texture2D> icon;
        if (theme_cache.icon_name.begins_with("res://")) {
            icon = ResourceLoader::get_singleton()->load(theme_cache.icon_name);
        } else if (theme_cache.icon_name.begins_with("uid://")) {
            icon = ResourceLoader::get_singleton()->load(GDE::ResourceUID::uid_to_path(theme_cache.icon_name));
        } else {
            icon = SceneUtils::get_editor_icon(theme_cache.icon_name);
        }

        _icon->set_texture(icon);
        _icon->set_visible(icon.is_valid());
    }

    if (_title_label) {
        const HorizontalAlignment alignment = _comment->is_title_text_center_aligned()
            ? HORIZONTAL_ALIGNMENT_CENTER
            : HORIZONTAL_ALIGNMENT_LEFT;

        _title_label->set_horizontal_alignment(alignment);
    }

    if (_comments_text) {
        if (theme_cache.font_size != _comment->get_comments_text_font_size()
                || !theme_cache.font_color.is_equal_approx(_comment->get_comments_text_color())) {

            theme_cache.font_size = _comment->get_comments_text_font_size();
            if (theme_cache.font_size == 0) {
                theme_cache.font_size = 14;
            }

            theme_cache.font_color = _comment->get_comments_text_color();

            _comments_text->begin_bulk_theme_override();
            _comments_text->add_theme_font_size_override(SceneStringName(font_size), theme_cache.font_size);
            _comments_text->add_theme_color_override(SceneStringName(font_color), theme_cache.font_color);
            _comments_text->end_bulk_theme_override();

            // Forces a recomputation of the static label comments after font resize
            _comments_text->update_minimum_size();
        }

        _comments_text->set_visible(!_comment->get_comments_text().is_empty());
        _update_comments_text(get_size());
    }

    _update_placeholder_visibility();
    _apply_panel_expand_margin_deferred();

    queue_redraw();
}

void OrchestratorEditorGraphFrame::_update_placeholder_visibility() {
    const bool has_body = _comment.is_valid() && !_comment->get_comments_text().is_empty();
    _placeholder->set_visible(!has_body && !_has_attached_nodes);
}

void OrchestratorEditorGraphFrame::_gui_input(const Ref<InputEvent>& p_event) {
    const Ref<InputEventMouseButton> mb = p_event;
    if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == MOUSE_BUTTON_RIGHT) {
        emit_signal("context_menu_requested", this, mb->get_position());
        accept_event();
        return;
    }
    GraphFrame::_gui_input(p_event);
}

void OrchestratorEditorGraphFrame::build_context_menu(OrchestratorEditorContextMenu* p_menu) {
    p_menu->add_separator();
    p_menu->add_item("Set Frame Title", callable_mp_this(_change_frame_title), false);
    p_menu->add_item("Set Comment Text", callable_mp_this(_open_change_comment_text), false);
    p_menu->add_check_item("Enable Auto Shrink", callable_mp_this(_toggle_autoshrink), is_autoshrink_enabled());
    p_menu->add_check_item("Enable Tint Color", callable_mp_this(_toggle_tint), is_tint_color_enabled());
    if (is_tint_color_enabled()) {
        p_menu->add_item("Set Tint Color", callable_mp_this(_show_tint_color_picker));
    }
}

void OrchestratorEditorGraphFrame::set_node(const Ref<OrchestrationGraphNode>& p_node) {
    _comment = p_node;

    // Start resizable; the model's auto-shrink state will modify this later.
    set_resizable(true);

    // Apply the persisted position and size immediately.
    // Safe before the layout pass because they're just property assignments.
    set_position_offset(p_node->get_position());
    set_size(p_node->get_size());

    _comment->connect(CoreStringName(changed), callable_mp_this(update));

    // Defer the model-driven update so the initial layout pass runs first.
    // This ensures the static label has a stable width before its text is set,
    // avoiding inflation from layout invalidation during construction.
    queue_sort();
    callable_mp_this(_update_from_model).call_deferred();
}

void OrchestratorEditorGraphFrame::update() {
    callable_mp_this(_update_from_model).call_deferred();
    emit_signal("changed");
}

void OrchestratorEditorGraphFrame::update_placeholder(bool p_has_attached_nodes) {
    _has_attached_nodes = p_has_attached_nodes;
    _update_placeholder_visibility();
}

void OrchestratorEditorGraphFrame::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_THEME_CHANGED: {
            _update_theme();
            break;
        }
    }
}

void OrchestratorEditorGraphFrame::_bind_methods() {
    ADD_SIGNAL(MethodInfo("context_menu_requested", PropertyInfo(Variant::OBJECT, "frame"), PropertyInfo(Variant::VECTOR2, "position")));
    ADD_SIGNAL(MethodInfo("changed"));
}

OrchestratorEditorGraphFrame::OrchestratorEditorGraphFrame() {
    // Initially disable autoshrink; the model state will override this later.
    set_autoshrink_enabled(false);

    _title_hbox = get_titlebar_hbox();

    // Remove the engine-created title Label from the titlebar HBox so we can re-add it
    // inside our own VBox/HBox layout (with the icon prepended).
    for (int i = 0; i < _title_hbox->get_child_count(); i++) {
        if (Label* label = cast_to<Label>(_title_hbox->get_child(i))) {
            _title_label = label;
            _title_hbox->remove_child(_title_label);
            break;
        }
    }

    // Titlebar contains a VBOx with two rows:
    // 1. HBox holding the Icon and the reused title Label
    // 2. The static (multi-line) comment text label
    _vbox = memnew(VBoxContainer);
    _vbox->set_h_size_flags(SIZE_EXPAND_FILL);
    _vbox->set_v_size_flags(SIZE_SHRINK_BEGIN);
    _title_hbox->add_child(_vbox);

    MarginContainer* margin_container = memnew(MarginContainer);
    margin_container->set_h_size_flags(SIZE_EXPAND_FILL);
    margin_container->add_theme_constant_override("margin_left", 4);
    margin_container->add_theme_constant_override("margin_right", 4);
    _vbox->add_child(margin_container);

    HBoxContainer* hbox = memnew(HBoxContainer);
    margin_container->add_child(hbox);

    // AUTOWRAP_WORD can cause spurious frame growth due to a min-size propagation
    // race when text changes. We wrap manually via _wrap_text() instead.
    _comments_text = memnew(Label);
    _comments_text->set_h_size_flags(SIZE_EXPAND_FILL);
    _comments_text->set_autowrap_mode(TextServer::AUTOWRAP_OFF);
    _vbox->add_child(_comments_text);

    _icon = memnew(TextureRect);
    _icon->set_name("FrameIcon");
    _icon->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
    _icon->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
    _icon->set_custom_minimum_size(Vector2(20, 20));
    hbox->add_child(_icon);
    hbox->add_child(_title_label);

    _placeholder = memnew(Label);
    _placeholder->set_text("Drag and drop nodes here to attach them.");
    _placeholder->add_theme_color_override(SceneStringName(font_color), Color(1, 1, 1, 0.4f));
    _placeholder->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
    add_child(_placeholder);

    connect("node_selected", callable_mp_this(_frame_selected));
    connect("dragged", callable_mp_this(_frame_dragged));
    connect("resize_request", callable_mp_this(_frame_resize_request));
    connect("resize_end", callable_mp_this(_frame_resize_end));
    connect("autoshrink_changed", callable_mp_this(_frame_autoshrink_changed));
    connect("resized", callable_mp_this(_frame_resized));
}
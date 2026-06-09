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
#include "editor/gui/search/help.h"

#include "common/macros.h"
#include "common/scene_utils.h"
#include "core/godot/core_string_names.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/style_box.hpp>

void OrchestratorEditorSearchHelpBit::_meta_clicked() {
    // no-op
}

void OrchestratorEditorSearchHelpBit::_add_text(const String& p_bbcode_text) {
    _help->append_text(p_bbcode_text);
}

void OrchestratorEditorSearchHelpBit::set_disabled(bool p_disabled) {
    _disabled = p_disabled;
    _help->set_modulate(Color(1, 1, 1, p_disabled ? 0.5f : 1.0f));
}

void OrchestratorEditorSearchHelpBit::set_title(const String& p_title) {
    _title->clear();
    _title->show();

    if (!p_title.is_empty()) {
        _title->push_font(SceneUtils::get_editor_font("doc_bold"));
        _title->push_color(SceneUtils::get_editor_color("title_color", "EditorHelp"));
        _title->append_text("Type");
        _title->pop();
        _title->append_text(vformat(" %s ", p_title));
        _title->pop();
    } else {
        _title->push_font(SceneUtils::get_editor_font("doc_bold"));
        _title->push_color(SceneUtils::get_editor_color("title_color", "EditorHelp"));
        _title->add_text(" ");
        _title->pop();
        _title->pop();
    }

    if (is_inside_tree()) {
        update_content_height();
    }
}

void OrchestratorEditorSearchHelpBit::set_text(const String& p_text) {
    _help->clear();
    _add_text(p_text);

    if (is_inside_tree()) {
        update_content_height();
    }
}

void OrchestratorEditorSearchHelpBit::set_content_help_limits(float p_min, float p_max) {
    ERR_FAIL_COND(p_min > p_max);
    _content_min_height = p_min;
    _content_max_height = p_max;

    if (is_inside_tree()) {
        update_content_height();
    }
}

void OrchestratorEditorSearchHelpBit::update_content_height() {
    int32_t content_height = _help->get_content_height();
    const Ref<StyleBox> style_box = _help->get_theme_stylebox(CoreStringName(normal));
    if (style_box.is_valid()) {
        content_height += style_box->get_content_margin(SIDE_TOP) + style_box->get_content_margin(SIDE_BOTTOM);
    }

    const Vector2 help_min_size = _help->get_custom_minimum_size();
    _help->set_custom_minimum_size(Size2(help_min_size.x, CLAMP(content_height, _content_min_height, _content_max_height)));
}

void OrchestratorEditorSearchHelpBit::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_ENTER_TREE: {
            update_content_height();
            break;
        }
        case NOTIFICATION_THEME_CHANGED: {
            _help->clear();
            _help->add_theme_color_override("selection_color", get_theme_color("selection_color", "EditorHelp"));
            break;
        }
    }
}

void OrchestratorEditorSearchHelpBit::_bind_methods() {

}

OrchestratorEditorSearchHelpBit::OrchestratorEditorSearchHelpBit() {
    add_theme_constant_override("separation", 0);

    _title = memnew(RichTextLabel);
    _title->set_theme_type_variation("EditorHelpBitTitle");
    _title->set_custom_minimum_size(Size2(640 * EDSCALE, 0));
    _title->set_fit_content(true);
    _title->connect("meta_clicked", callable_mp_this(_meta_clicked));
    _title->append_text(" ");
    add_child(_title);

    _content_min_height = 48 * EDSCALE;
    _content_max_height = 360 * EDSCALE;

    _help = memnew(RichTextLabel);
    _help->set_theme_type_variation("EditorHelpBitContent");
    _help->set_custom_minimum_size(Size2(640 * EDSCALE, _content_min_height));
    _help->set_v_size_flags(SIZE_EXPAND_FILL);
    _help->set_fit_content(true);
    _help->set_use_bbcode(true);
    _help->connect("meta_clicked", callable_mp_this(_meta_clicked));
    _help->append_text(" ");
    add_child(_help);
}
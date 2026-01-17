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
#include "editor/actions/help.h"

#include "common/macros.h"
#include "common/scene_utils.h"
#include "common/string_utils.h"
#include "core/godot/core_string_names.h"
#include "editor/actions/definition.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/style_box.hpp>

void OrchestratorEditorActionHelp::_meta_clicked(const Variant& p_value) {
}

void OrchestratorEditorActionHelp::_add_text(const String& p_text) {
    _help->append_text(p_text);
}

void OrchestratorEditorActionHelp::set_disabled(bool p_disabled) {
    _help->set_modulate(Color(1, 1, 1, p_disabled ? 0.5f : 1.0f));
}

void OrchestratorEditorActionHelp::set_text(const String& p_text) {
    _help->clear();
    _add_text(p_text);

    if (is_inside_tree())
        update_content_height();
}

void OrchestratorEditorActionHelp::set_content_help_limits(float p_min, float p_max) {
    _content_size = Size2(p_min, p_max);

    if (is_inside_tree())
        update_content_height();
}

void OrchestratorEditorActionHelp::update_content_height() {
    float content_height = _help->get_content_height();
    const Ref<StyleBox> style = _help->get_theme_stylebox(CoreStringName(normal));
    if (style.is_valid()) {
        content_height += style->get_content_margin(SIDE_TOP) + style->get_content_margin(SIDE_BOTTOM);
    }

    _help->set_custom_minimum_size(
        Size2(
            _help->get_custom_minimum_size().x,
            CLAMP(content_height, _content_size.x, _content_size.y)));
}

void OrchestratorEditorActionHelp::parse_action(const Ref<OrchestratorEditorActionDefinition>& p_action) {
    if (p_action.is_valid()) {
        const Ref<Font> doc_bold_font = SceneUtils::get_editor_font("doc_bold");

        _title->clear();
        _title->push_font(doc_bold_font);

        const String categories = StringUtils::join(" > ", p_action->category.split("/"));
        if (!categories.is_empty()) {
            _title->push_color(SceneUtils::get_editor_color("title_color", "EditorHelp"));
            _title->add_text(vformat("%s: ", categories));
            _title->pop();
        }

        _title->add_text(p_action->name);
        _title->pop(); // font

        _title->show();

        set_text(p_action->tooltip);
        set_disabled(false);
    } else {
        _title->clear();
        _title->add_text(" ");
        set_text("");
        set_disabled(true);
    }
}

void OrchestratorEditorActionHelp::_notification(int p_what) {
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

OrchestratorEditorActionHelp::OrchestratorEditorActionHelp() {
    add_theme_constant_override("separation", 0);

    _title = memnew(RichTextLabel);
    _title->set_theme_type_variation("EditorHelpBitTitle");
    _title->set_custom_minimum_size(Size2(640 * EDSCALE, 0));
    _title->set_fit_content(true);
    _title->connect("meta_clicked", callable_mp_this(_meta_clicked));
    add_child(_title);

    _content_size = Size2(48 * EDSCALE, 360 * EDSCALE);

    _help = memnew(RichTextLabel);
    _help->set_theme_type_variation("EditorHelpBitContent");
    _help->set_custom_minimum_size(Size2(640* EDSCALE, _content_size.x));
    _help->set_fit_content(true);
    _help->set_use_bbcode(true);
    _help->connect("meta_clicked", callable_mp_this(_meta_clicked));
    add_child(_help);
}
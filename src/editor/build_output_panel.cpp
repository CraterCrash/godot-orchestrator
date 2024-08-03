// This file is part of the Godot Orchestrator project.
//
// Copyright (c) 2023-present Vahera Studios LLC and its contributors.
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
#include "editor/build_output_panel.h"

#include "common/callable_lambda.h"
#include "common/scene_utils.h"
#include "common/version.h"
#include "plugins/orchestrator_editor_plugin.h"

#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

void OrchestratorBuildOutputPanel::_append_text(const String& p_text)
{
    _rtl->append_text(p_text);
}

void OrchestratorBuildOutputPanel::reset()
{
    if (_button)
        _button->set_button_icon(Ref<Texture2D>());

    _rtl->clear();
    _rtl->append_text(vformat("%s - (c) 2023-present Vahera Studios, LCC and its contributors.\n\n", VERSION_FULL_NAME));
}

void OrchestratorBuildOutputPanel::add_error(const String& p_text)
{
    _append_text(vformat("* [b][color=#a95853]ERROR[/color][/b] : %s\n\n", p_text));
    _button->set_button_icon(SceneUtils::get_editor_icon("Error"));
}

void OrchestratorBuildOutputPanel::add_warning(const String& p_text)
{
    _append_text(vformat("* [b][color=yellow]WARNING[/color][/b] : %s\n\n", p_text));
    _button->set_button_icon(SceneUtils::get_editor_icon("Error"));
}

void OrchestratorBuildOutputPanel::add_message(const String& p_text)
{
    _append_text(p_text);
}

void OrchestratorBuildOutputPanel::set_tool_button(Button* p_button)
{
    _button = p_button;
}

void OrchestratorBuildOutputPanel::_notification(int p_what)
{
    if (p_what == NOTIFICATION_THEME_CHANGED)
    {
        const Ref<Font> normal_font = SceneUtils::get_editor_font("output_source");
        if (normal_font.is_valid())
            _rtl->add_theme_font_override("normal_font", normal_font);

        const Ref<Font> bold_normal_font = SceneUtils::get_editor_font("output_source_bold");
        if (bold_normal_font.is_valid())
            _rtl->add_theme_font_override("bold_font", bold_normal_font);

        const Ref<Font> mono_font = SceneUtils::get_editor_font("output_source_mono");
        if (mono_font.is_valid())
            _rtl->add_theme_font_override("mono_font", mono_font);

        const int font_size = SceneUtils::get_editor_font_size("output_source_size");
        _rtl->begin_bulk_theme_override();
        _rtl->add_theme_font_size_override("normal_font_size", font_size);
        _rtl->add_theme_font_size_override("bold_font_size", font_size);
        _rtl->add_theme_font_size_override("italics_font_size", font_size);
        _rtl->add_theme_font_size_override("mono_font_size", font_size);
        _rtl->end_bulk_theme_override();
    }
    else if (p_what == NOTIFICATION_READY)
    {
        _rtl->connect("meta_clicked", callable_mp_lambda(this, [this](const Variant& p_meta) {
            emit_signal("meta_clicked", p_meta);
        }));

        _clear_button->set_button_icon(SceneUtils::get_editor_icon("Clear"));
        _clear_button->connect("pressed", callable_mp_lambda(this, [this] { reset(); }));
    }
}

void OrchestratorBuildOutputPanel::_bind_methods()
{
    ADD_SIGNAL(MethodInfo("meta_clicked", PropertyInfo(Variant::NIL, "meta", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NIL_IS_VARIANT)));
}

OrchestratorBuildOutputPanel::OrchestratorBuildOutputPanel()
{
    _rtl = memnew(RichTextLabel);
    _rtl->set_h_size_flags(SIZE_EXPAND_FILL);
    _rtl->set_v_size_flags(SIZE_EXPAND_FILL);
    _rtl->set_use_bbcode(true);
    add_child(_rtl);

    VBoxContainer* button_container = memnew(VBoxContainer);
    _clear_button = memnew(Button);
    _clear_button->set_focus_mode(FOCUS_NONE);
    _clear_button->set_tooltip_text("Clear Orchestrator's Build Output");
    button_container->add_child(_clear_button);
    add_child(button_container);

    reset();
}

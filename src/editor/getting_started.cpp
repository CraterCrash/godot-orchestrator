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
#include "editor/getting_started.h"

#include "common/macros.h"
#include "common/scene_utils.h"
#include "common/version.h"
#include "core/godot/scene_string_names.h"
#include "editor/plugins/orchestrator_editor_plugin.h"

#include <godot_cpp/classes/texture_rect.hpp>

void OrchestratorGettingStarted::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_THEME_CHANGED: {
            const Color font_color = SceneUtils::get_editor_color("font_color");
            const bool dark_theme = font_color.get_luminance() > 0.5;
            const Color shadow = dark_theme ? Color(0, 0, 0, 0.3) : Color(1, 1, 1, 0.3);

            _plugin_version->add_theme_font_size_override(SceneStringName(font_size), 24);
            _plugin_version->add_theme_color_override("font_shadow_color", shadow);
            _plugin_version->add_theme_constant_override("shadow_outline_size", 3);

            _create_script->set_button_icon(SceneUtils::get_editor_icon("ScriptCreateDialog"));
            _open_script->set_button_icon(SceneUtils::get_editor_icon("Script"));
            _open_docs->set_button_icon(SceneUtils::get_editor_icon("ExternalLink"));
        }
        default: {
            break;
        }
    }
}

void OrchestratorGettingStarted::_bind_methods() {
    ADD_SIGNAL(MethodInfo("create_requested"));
    ADD_SIGNAL(MethodInfo("open_requested"));
    ADD_SIGNAL(MethodInfo("documentation_requested"));
}

OrchestratorGettingStarted::OrchestratorGettingStarted() {
    set_alignment(ALIGNMENT_CENTER);
    set_anchors_preset(PRESET_FULL_RECT);
    set_v_size_flags(SIZE_EXPAND_FILL);

    TextureRect* logo = memnew(TextureRect);
    logo->set_custom_minimum_size(Vector2(128, 128));
    logo->set_texture(OrchestratorPlugin::get_singleton()->get_plugin_icon_hires());
    logo->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
    logo->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
    logo->set_h_size_flags(SIZE_SHRINK_CENTER);
    add_child(logo);

    _plugin_version = memnew(Label);
    _plugin_version->set_text(vformat("Godot %s - %s", VERSION_NAME, VERSION_FULL_BUILD));
    _plugin_version->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
    add_child(_plugin_version);

    VBoxContainer* button_container = memnew(VBoxContainer);
    button_container->set_h_size_flags(SIZE_SHRINK_CENTER);
    button_container->set_custom_minimum_size(Vector2(256, 0));
    add_child(button_container);

    _create_script = memnew(Button);
    _create_script->set_text("Create New Orchestration");
    _create_script->set_focus_mode(FOCUS_NONE);
    _create_script->connect(SceneStringName(pressed), callable_mp_this(_create_new));
    button_container->add_child(_create_script);

    _open_script = memnew(Button);
    _open_script->set_text("Open Orchestration");
    _open_script->set_focus_mode(FOCUS_NONE);
    _open_script->connect(SceneStringName(pressed), callable_mp_this(_open));
    button_container->add_child(_open_script);

    _open_docs = memnew(Button);
    _open_docs->set_text("Get Help");
    _open_docs->set_focus_mode(FOCUS_NONE);
    _open_docs->connect(SceneStringName(pressed), callable_mp_this(_show_docs));
    button_container->add_child(_open_docs);
}
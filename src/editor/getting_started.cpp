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

#include "common/scene_utils.h"
#include "common/version.h"
#include "editor/plugins/orchestrator_editor_plugin.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/texture_rect.hpp>

void OrchestratorGettingStarted::_notification(int p_what)
{
    if (p_what == NOTIFICATION_READY)
    {
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

        Label* plugin_version = memnew(Label);
        plugin_version->set_text(vformat("Godot %s - %s", VERSION_NAME, VERSION_FULL_BUILD));
        plugin_version->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
        plugin_version->add_theme_font_size_override("font_size", 24);
        plugin_version->add_theme_color_override("font_shadow_color", Color(0, 0, 0, 1));
        plugin_version->add_theme_constant_override("shadow_outline_size", 3);
        add_child(plugin_version);

        VBoxContainer* button_container = memnew(VBoxContainer);
        button_container->set_h_size_flags(SIZE_SHRINK_CENTER);
        button_container->set_custom_minimum_size(Vector2(256, 0));
        add_child(button_container);

        Button* create_new = memnew(Button);
        create_new->set_button_icon(SceneUtils::get_editor_icon("ScriptCreateDialog"));
        create_new->set_text("Create New Orchestration");
        create_new->set_focus_mode(FOCUS_NONE);
        create_new->connect("pressed", callable_mp(this, &OrchestratorGettingStarted::_create_new));
        button_container->add_child(create_new);

        Button* open = memnew(Button);
        open->set_button_icon(SceneUtils::get_editor_icon("Script"));
        open->set_text("Open Orchestration");
        open->set_focus_mode(FOCUS_NONE);
        open->connect("pressed", callable_mp(this, &OrchestratorGettingStarted::_open));
        button_container->add_child(open);

        Button* open_docs = memnew(Button);
        open_docs->set_button_icon(SceneUtils::get_editor_icon("ExternalLink"));
        open_docs->set_text("Get Help");
        open_docs->set_focus_mode(FOCUS_NONE);
        open_docs->connect("pressed", callable_mp(this, &OrchestratorGettingStarted::_show_docs));
        button_container->add_child(open_docs);
    }
}

void OrchestratorGettingStarted::_bind_methods()
{
    ADD_SIGNAL(MethodInfo("create_requested"));
    ADD_SIGNAL(MethodInfo("open_requested"));
    ADD_SIGNAL(MethodInfo("documentation_requested"));
}

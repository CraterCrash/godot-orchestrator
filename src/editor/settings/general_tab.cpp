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
#include "editor/settings/general_tab.h"

#include "common/macros.h"
#include "common/scene_utils.h"
#include "common/settings.h"
#include "core/godot/scene_string_names.h"
#include "editor/gui/sectioned_inspector.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/project_settings.hpp>

void OrchestratorEditorSettingsGeneralTab::_advanced_toggled(bool p_toggled) {
    EI->get_editor_settings()->set("_editor_settings_advanced_mode", p_toggled);
}

void OrchestratorEditorSettingsGeneralTab::_editor_restart_request() {
    _restart_container->show();
}

void OrchestratorEditorSettingsGeneralTab::_editor_restart() {
    emit_signal("restart_requested");
}

void OrchestratorEditorSettingsGeneralTab::_editor_restart_close() {
    _restart_container->hide();
}

void OrchestratorEditorSettingsGeneralTab::_project_settings_changed() {
    // todo: We use this to specifically set Custom theme when theme items change
}

void OrchestratorEditorSettingsGeneralTab::_update_icons() {
    _search_box->set_right_icon(SceneUtils::get_editor_icon("Search"));
    _search_box->set_clear_button_enabled(true);

    _restart_close_button->set_button_icon(SceneUtils::get_editor_icon("Close"));
    _restart_container->add_theme_stylebox_override(SceneStringName(panel), SceneUtils::get_editor_stylebox(SceneStringName(panel), "Tree"));
    _restart_icon->set_texture(SceneUtils::get_editor_icon("StatusWarning"));
    _restart_label->add_theme_color_override(SceneStringName(font_color), SceneUtils::get_editor_color("warning_color"));
}

void OrchestratorEditorSettingsGeneralTab::edit(Object* p_object) {
    _inspector->edit(p_object);
}

void OrchestratorEditorSettingsGeneralTab::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_READY: {
            edit(OrchestratorSettings::get_singleton());
            break;
        }
        case NOTIFICATION_ENTER_TREE: {
            _update_icons();
            break;
        }
        case NOTIFICATION_VISIBILITY_CHANGED: {
            if (is_visible()) {
                _search_box->grab_focus();
                _search_box->select_all();
            }
            break;
        }
    }
}

void OrchestratorEditorSettingsGeneralTab::_bind_methods() {
    ADD_SIGNAL(MethodInfo("restart_requested"));
}

OrchestratorEditorSettingsGeneralTab::OrchestratorEditorSettingsGeneralTab() {
    set_name("General");

    HBoxContainer* hbox = memnew(HBoxContainer);
    hbox->set_h_size_flags(SIZE_EXPAND_FILL);
    add_child(hbox);

    _search_box = memnew(LineEdit);
    _search_box->set_placeholder("Filter Settings");
    #if GODOT_VERSION >= 0x040500
    _search_box->set_accessibility_name("Filter Settings");
    _search_box->set_virtual_keyboard_show_on_focus(false);
    #endif
    _search_box->set_clear_button_enabled(true);
    _search_box->set_h_size_flags(SIZE_EXPAND_FILL);
    hbox->add_child(_search_box);

    _advanced_switch = memnew(CheckButton);
    _advanced_switch->set_text("Advanced Settings");
    hbox->add_child(_advanced_switch);

    const bool use_advanced = EDITOR_GET("_editor_settings_advanced_mode");
    _advanced_switch->set_pressed(use_advanced);
    _advanced_switch->connect(SceneStringName(toggled), callable_mp_this(_advanced_toggled));

    _inspector = memnew(OrchestratorEditorSectionedInspector);
    _inspector->register_search_box(_search_box);
    _inspector->register_advanced_toggle(_advanced_switch);
    _inspector->set_v_size_flags(SIZE_EXPAND_FILL);
    add_child(_inspector);

    _inspector->get_inspector()->connect("restart_requested", callable_mp_this(_editor_restart_request));

    _restart_container = memnew(PanelContainer);
    add_child(_restart_container);

    HBoxContainer* restart_hbox = memnew(HBoxContainer);
    _restart_container->add_child(restart_hbox);
    _restart_icon = memnew(TextureRect);
    _restart_icon->set_v_size_flags(SIZE_SHRINK_CENTER);
    restart_hbox->add_child(_restart_icon);

    _restart_label = memnew(Label);
    #if GODOT_VERSION >= 0x040500
    _restart_label->set_focus_mode(FOCUS_ACCESSIBILITY);
    #endif
    _restart_label->set_text("The editor must be restarted for changes to take effect.");

    restart_hbox->add_child(_restart_label);
    restart_hbox->add_spacer(false);

    Button* restart_button = memnew(Button);
    restart_button->set_text("Save & Restart");
    restart_button->connect(SceneStringName(pressed), callable_mp_this(_editor_restart));
    restart_hbox->add_child(restart_button);

    _restart_close_button = memnew(Button);
    #if GODOT_VERSION >= 0x040500
    _restart_close_button->set_accessibility_name("Close");
    #endif
    _restart_close_button->set_flat(true);
    _restart_close_button->connect(SceneStringName(pressed), callable_mp_this(_editor_restart_close));
    restart_hbox->add_child(_restart_close_button);
    _restart_container->hide();

    ProjectSettings::get_singleton()->connect("settings_changed", callable_mp_this(_project_settings_changed));
}
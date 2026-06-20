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
#include "editor/settings/settings_dialog.h"

#include "common/macros.h"
#include "common/scene_utils.h"
#include "common/settings.h"
#include "core/godot/scene_string_names.h"
#include "editor/gui/search_dialog.h"
#include "editor/settings/editor_settings.h"
#include "editor/settings/general_tab.h"
#include "editor/settings/input_event_configuration_dialog.h"
#include "editor/settings/shortcuts_tab.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/scene_tree.hpp>

void OrchestratorEditorSettingsDialog::_confirmed() {
    if (!OrchestratorSettings::get_singleton()) {
        return;
    }
    OrchestratorEditorSettings::get_singleton()->notify_changes();
    OrchestratorEditorSettings::get_singleton()->save();
}

void OrchestratorEditorSettingsDialog::_canceled() {
    if (!OrchestratorSettings::get_singleton()) {
        return;
    }

    OrchestratorEditorSettings::get_singleton()->notify_changes();
}

void OrchestratorEditorSettingsDialog::_editor_restart() {
    EI->restart_editor(true);
}

void OrchestratorEditorSettingsDialog::_shortcut_input(const Ref<InputEvent>& p_event) {
    const Ref<InputEventKey> key = p_event;
    if (key.is_valid() && key->is_pressed()) {
        bool handled = false;

        if (EditorNode) {
            if (ED_IS_SHORTCUT("ui_undo", p_event)) {
                // todo: not sure if there is anything we do here
                // EditorNode::get_singleton()->undo();
                handled = true;
            }
            if (ED_IS_SHORTCUT("ui_redo", p_event)) {
                // todo: not sure if there is anything we do here
                // EditorNode::get_singleton()->redo();
                handled = true;
            }
        }

        if (ED_IS_NATIVE_SHORTCUT("editor/open_search", p_event)) {
            // This allows for the visibility changed signal to trigger refocus
            Control* control = _tabs->get_current_tab_control();
            control->set_visible(false);
            control->set_visible(true);
            handled = true;
        }

        if (handled) {
            set_input_as_handled();
        }
    }
}

void OrchestratorEditorSettingsDialog::popup_edit_settings() {
    if (!ProjectSettings::get_singleton()) {
        return;
    }

    set_process_shortcut_input(true);

    Rect2 saved_rect = EI->get_editor_settings()->get_project_metadata("dialog_bounds", "editor_settings", Rect2());
    if (saved_rect != Rect2()) {
        popup(saved_rect);
    } else {
        popup_centered_clamped(Size2(900, 700) * EDSCALE, 0.8);
    }
}

void OrchestratorEditorSettingsDialog::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_VISIBILITY_CHANGED: {
            if (!is_visible()) {
                EI->get_editor_settings()->set_project_metadata("dialog_bounds", "editor_settings", Rect2(get_position(), get_size()));
                set_process_shortcut_input(false);
            }
            break;
        }
        case NOTIFICATION_READY: {
            // Sets up some global redo stuff
            break;
        }
    }
}

void OrchestratorEditorSettingsDialog::_bind_methods() {
}

OrchestratorEditorSettingsDialog::OrchestratorEditorSettingsDialog() {
    set_title("Orchestrator Settings");
    set_ok_button_text("Close");
    #if GODOT_VERSION >= 0x040500
    set_flag(FLAG_MAXIMIZE_DISABLED, false);
    #endif

    _tabs = memnew(TabContainer);
    _tabs->set_theme_type_variation("TabContainerOdd");
    add_child(_tabs);

    //~ Begin General Tab
    OrchestratorEditorSettingsGeneralTab* general = memnew(OrchestratorEditorSettingsGeneralTab);
    general->connect("restart_requested", callable_mp_this(_editor_restart)); // Propagate upward
    _tabs->add_child(general);

    //~ Begin Shortcuts Tab
    OrchestratorEditorSettingsShortcutsTab* shortcuts = memnew(OrchestratorEditorSettingsShortcutsTab);
    _tabs->add_child(shortcuts);

    set_exclusive(true);
    set_hide_on_ok(true);

    connect(SceneStringName(confirmed), callable_mp_this(_confirmed));
    connect(SceneStringName(canceled), callable_mp_this(_canceled));
}
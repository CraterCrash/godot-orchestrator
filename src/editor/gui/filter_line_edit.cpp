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
#include "editor/gui/filter_line_edit.h"

#include "common/macros.h"
#include "common/scene_utils.h"

#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/viewport.hpp>

void OrchestratorEditorFilterLineEdit::_gui_input(const Ref<InputEvent>& p_event) {
    ERR_FAIL_NULL(_forward_control);

    Ref<InputEventKey> key = p_event;
    if (key.is_valid()) {
        // Redirect navigational key events to the control.
        if (key->is_action(SNAME("ui_up"), true) ||
                key->is_action(SNAME("ui_down"), true) ||
                key->is_action(SNAME("ui_page_up")) ||
                key->is_action(SNAME("ui_page_down"))) {
            push_event(key, this, _forward_control);
            accept_event();
        }
    }
}

void OrchestratorEditorFilterLineEdit::set_forward_control(Control* p_control) {
    ERR_FAIL_NULL(p_control);
    _forward_control = p_control;
}

void OrchestratorEditorFilterLineEdit::_notification(int p_what) {
    if (p_what == NOTIFICATION_THEME_CHANGED) {
        set_right_icon(SceneUtils::get_editor_icon("Search"));
    }
}

void OrchestratorEditorFilterLineEdit::_bind_methods() {

}

OrchestratorEditorFilterLineEdit::OrchestratorEditorFilterLineEdit() {
    set_clear_button_enabled(true);
    set_h_size_flags(SIZE_EXPAND_FILL);
}
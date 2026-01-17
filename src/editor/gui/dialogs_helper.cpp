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
#include "editor/gui/dialogs_helper.h"

#include "common/macros.h"

#include <godot_cpp/classes/accept_dialog.hpp>
#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/label.hpp>

void OrchestratorEditorDialogs::accept(const String& p_message, const String& p_button) {
    AcceptDialog* dialog = memnew(AcceptDialog);
    dialog->set_ok_button_text(p_button);
    dialog->set_text(p_message);
    dialog->set_autowrap(true);
    dialog->set_min_size(Vector2i(600, 0));
    dialog->reset_size();

    dialog->connect("canceled", callable_mp_cast(dialog, Node, queue_free));
    dialog->connect("confirmed", callable_mp_cast(dialog, Node, queue_free));

    EI->popup_dialog_centered_clamped(dialog, Size2i(), 0.0);
}

void OrchestratorEditorDialogs::confirm(const String& p_message, const Callable& p_callback, const String& p_yes_label, const String& p_no_label) {
    ConfirmationDialog* dialog = memnew(ConfirmationDialog);
    dialog->set_cancel_button_text(p_no_label);
    dialog->set_ok_button_text(p_yes_label);
    dialog->set_text(p_message);
    dialog->set_title("Please confirm...");
    dialog->get_label()->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);

    if (p_callback.is_valid()) {
        dialog->connect("confirmed", p_callback);
    }

    dialog->connect("canceled", callable_mp_cast(dialog, Node, queue_free));
    dialog->connect("confirmed", callable_mp_cast(dialog, Node, queue_free));

    EI->popup_dialog_centered(dialog);
}

void OrchestratorEditorDialogs::error(const String& p_message, const String& p_title, bool p_exclusive) {
    AcceptDialog* dialog = memnew(AcceptDialog);
    dialog->set_text(p_message);
    dialog->set_title(p_title);
    dialog->set_exclusive(p_exclusive);

    dialog->connect("canceled", callable_mp_cast(dialog, Node, queue_free));
    dialog->connect("confirmed", callable_mp_cast(dialog, Node, queue_free));

    EI->popup_dialog_centered(dialog);
}

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
#include "editor/goto_node_dialog.h"

#include "common/macros.h"
#include "editor/script_editor_view.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

void OrchestratorGotoNodeDialog::_goto_node() {
    if (_line_edit && _line_edit->get_text().is_valid_int()) {
        _editor_view->goto_node(_line_edit->get_text().to_int());
    }
    queue_free();
}

void OrchestratorGotoNodeDialog::_visibility_changed() {
    if (is_visible() && _line_edit) {
        _line_edit->grab_focus();
    }
}

void OrchestratorGotoNodeDialog::popup_find_node(OrchestratorScriptGraphEditorView* p_view) {
    _editor_view = p_view;
    EI->popup_dialog_centered(this);
}

void OrchestratorGotoNodeDialog::_bind_methods() {
}

OrchestratorGotoNodeDialog::OrchestratorGotoNodeDialog() {
    set_title("Go to Node");

    VBoxContainer* container = memnew(VBoxContainer);
    add_child(container);

    Label* label = memnew(Label);
    label->set_text("Node Number:");
    container->add_child(label);

    _line_edit = memnew(LineEdit);
    _line_edit->set_select_all_on_focus(true);
    container->add_child(_line_edit);
    register_text_enter(_line_edit);

    connect("confirmed", callable_mp_this(_goto_node));
    connect("canceled", callable_mp_cast(this, Node, queue_free));
    connect("visibility_changed", callable_mp_this(_visibility_changed));
}

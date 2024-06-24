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
#include "editor/goto_node_dialog.h"

#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

void OrchestratorGotoNodeDialog::_confirmed()
{
    if (_line_edit && _line_edit->get_text().is_valid_int())
        emit_signal("goto_node", _line_edit->get_text().to_int());

    _line_edit->clear();
}

void OrchestratorGotoNodeDialog::_cancelled()
{
    _line_edit->clear();
}

void OrchestratorGotoNodeDialog::_notification(int p_what)
{
    switch (p_what)
    {
        case NOTIFICATION_VISIBILITY_CHANGED:
        {
            if (is_visible())
                _line_edit->grab_focus();
            break;
        }
        case NOTIFICATION_ENTER_TREE:
        {
            connect("confirmed", callable_mp(this, &OrchestratorGotoNodeDialog::_confirmed));
            connect("canceled", callable_mp(this, &OrchestratorGotoNodeDialog::_cancelled));
            break;
        }
        case NOTIFICATION_EXIT_TREE:
        {
            disconnect("confirmed", callable_mp(this, &OrchestratorGotoNodeDialog::_confirmed));
            disconnect("canceled", callable_mp(this, &OrchestratorGotoNodeDialog::_cancelled));
            break;
        }
    }
}

void OrchestratorGotoNodeDialog::_bind_methods()
{
    ADD_SIGNAL(MethodInfo("goto_node", PropertyInfo(Variant::INT, "node_id")));
}

OrchestratorGotoNodeDialog::OrchestratorGotoNodeDialog()
{
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
}

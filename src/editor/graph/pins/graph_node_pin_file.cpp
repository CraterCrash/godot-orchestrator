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
#include "graph_node_pin_file.h"

#include "common/string_utils.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/file_dialog.hpp>

OrchestratorGraphNodePinFile::OrchestratorGraphNodePinFile(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin)
    : OrchestratorGraphNodePin(p_node, p_pin)
{
}

void OrchestratorGraphNodePinFile::_bind_methods()
{
}

void OrchestratorGraphNodePinFile::_on_show_file_dialog(Button* p_button)
{
    p_button->set_text("Default Scene");
    _pin->set_default_value(Variant());

    FileDialog* dialog = memnew(FileDialog);
    dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
    dialog->set_min_size(Vector2(700, 400));
    dialog->set_initial_position(Window::WINDOW_INITIAL_POSITION_CENTER_SCREEN_WITH_MOUSE_FOCUS);
    dialog->set_hide_on_ok(true);
    dialog->set_title("Select a file");
    add_child(dialog);

    dialog->connect("file_selected", callable_mp(this, &OrchestratorGraphNodePinFile::_on_file_selected).bind(dialog, p_button));
    dialog->connect("canceled", callable_mp(this, &OrchestratorGraphNodePinFile::_on_file_canceled).bind(dialog, p_button));
    dialog->show();
}

void OrchestratorGraphNodePinFile::_on_file_selected(const String& p_file_name, FileDialog* p_dialog, Button* p_button)
{
    p_button->set_text(p_file_name);
    _pin->set_default_value(p_file_name);

    p_dialog->queue_free();
}

void OrchestratorGraphNodePinFile::_on_file_canceled(FileDialog* p_dialog, Button* p_button)
{
    p_dialog->queue_free();
}

Control* OrchestratorGraphNodePinFile::_get_default_value_widget()
{
    Button* button = memnew(Button);
    button->set_custom_minimum_size(Vector2(28, 0));
    button->set_focus_mode(Control::FOCUS_NONE);
    button->set_text(StringUtils::default_if_empty(_pin->get_effective_default_value(), "Default Scene"));
    button->connect("pressed", callable_mp(this, &OrchestratorGraphNodePinFile::_on_show_file_dialog).bind(button));
    return button;
}
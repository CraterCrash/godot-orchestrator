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
#include "graph_node_pin_file.h"

#include "common/scene_utils.h"
#include "common/string_utils.h"
#include "editor/file_dialog.h"
#include "script/nodes/script_nodes.h"

#include <godot_cpp/classes/button.hpp>

OrchestratorGraphNodePinFile::OrchestratorGraphNodePinFile(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin)
    : OrchestratorGraphNodePin(p_node, p_pin)
{
}

void OrchestratorGraphNodePinFile::_bind_methods()
{
}

String OrchestratorGraphNodePinFile::_get_default_text() const
{
    if (_pin->get_owning_node()->get_class() == OScriptNodeDialogueMessage::get_class_static())
        return "Default Scene";

    return "Assign...";
}

void OrchestratorGraphNodePinFile::_on_clear_file(Button* p_button)
{
    _pin->set_default_value("");
    p_button->set_text(_get_default_text());
    _clear_button->set_visible(false);
}

void OrchestratorGraphNodePinFile::_on_show_file_dialog(Button* p_button)
{
    OrchestratorFileDialog* dialog = memnew(OrchestratorFileDialog);
    dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
    dialog->set_hide_on_ok(true);
    dialog->set_title("Select a file");

    const String file_types = _pin->get_file_types();
    if (!file_types.is_empty())
        dialog->set_filters(Array::make(file_types));

    add_child(dialog);

    dialog->connect("file_selected", callable_mp(this, &OrchestratorGraphNodePinFile::_on_file_selected).bind(dialog, p_button));
    dialog->connect("canceled", callable_mp(this, &OrchestratorGraphNodePinFile::_on_file_canceled).bind(dialog, p_button));
    dialog->popup_file_dialog();
}

void OrchestratorGraphNodePinFile::_on_file_selected(const String& p_file_name, FileDialog* p_dialog, Button* p_button)
{
    p_button->set_text(p_file_name);
    _pin->set_default_value(p_file_name);
    _clear_button->set_visible(p_button->get_text() != _get_default_text());

    p_dialog->queue_free();
}

void OrchestratorGraphNodePinFile::_on_file_canceled(FileDialog* p_dialog, Button* p_button)
{
    p_dialog->queue_free();
}

Control* OrchestratorGraphNodePinFile::_get_default_value_widget()
{
    HBoxContainer* container = memnew(HBoxContainer);
    container->add_theme_constant_override("separation", 1);

    Button* file_button = memnew(Button);
    file_button->set_custom_minimum_size(Vector2(28, 0));
    file_button->set_focus_mode(Control::FOCUS_NONE);
    file_button->set_text(StringUtils::default_if_empty(_pin->get_effective_default_value(), _get_default_text()));
    file_button->connect("pressed", callable_mp(this, &OrchestratorGraphNodePinFile::_on_show_file_dialog).bind(file_button));
    container->add_child(file_button);

    _clear_button = memnew(Button);
    _clear_button->set_focus_mode(Control::FOCUS_NONE);
    _clear_button->set_button_icon(SceneUtils::get_editor_icon("Reload"));
    _clear_button->connect("pressed", callable_mp(this, &OrchestratorGraphNodePinFile::_on_clear_file).bind(file_button));
    _clear_button->set_visible(file_button->get_text() != _get_default_text());
    container->add_child(_clear_button);

    return container;
}
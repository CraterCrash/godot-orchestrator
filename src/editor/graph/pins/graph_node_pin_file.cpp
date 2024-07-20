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

#include "common/scene_utils.h"
#include "common/string_utils.h"
#include "editor/file_dialog.h"
#include "editor/plugins/orchestrator_editor_plugin.h"
#include "script/nodes/script_nodes.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>

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

void OrchestratorGraphNodePinFile::_on_clear_file()
{
    _pin->set_default_value("");
    _file_button->set_text(_get_default_text());
    _clear_button->set_visible(false);
}

void OrchestratorGraphNodePinFile::_on_show_file_dialog()
{
    OrchestratorFileDialog* dialog = memnew(OrchestratorFileDialog);
    dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
    dialog->set_hide_on_ok(true);
    dialog->set_title("Select a file");

    const String file_types = _pin->get_file_types();
    if (!file_types.is_empty())
        dialog->set_filters(Array::make(file_types));

    add_child(dialog);

    dialog->connect("file_selected", callable_mp(this, &OrchestratorGraphNodePinFile::_on_file_selected).bind(dialog));
    dialog->connect("canceled", callable_mp(this, &OrchestratorGraphNodePinFile::_on_file_canceled).bind(dialog));
    dialog->popup_file_dialog();
}

void OrchestratorGraphNodePinFile::_on_file_selected(const String& p_file_name, FileDialog* p_dialog)
{
    EditorUndoRedoManager* undo = OrchestratorPlugin::get_singleton()->get_undo_redo();
    undo->create_action("Orchestration: Change file pin");
    undo->add_do_method(_pin.ptr(), "set_default_value", p_file_name);
    undo->add_do_method(_file_button, "set_text", p_file_name);
    undo->add_do_method(_clear_button, "set_visible", p_file_name != _get_default_text());

    const String value = _pin->get_effective_default_value();
    undo->add_undo_method(_pin.ptr(), "set_default_value", value);
    undo->add_undo_method(_file_button, "set_text", value.is_empty() ? _get_default_text() : value);
    undo->add_undo_method(_clear_button, "set_visible", !value.is_empty());
    undo->commit_action();

    p_dialog->queue_free();
}

void OrchestratorGraphNodePinFile::_on_file_canceled(FileDialog* p_dialog)
{
    p_dialog->queue_free();
}

Control* OrchestratorGraphNodePinFile::_get_default_value_widget()
{
    HBoxContainer* container = memnew(HBoxContainer);
    container->add_theme_constant_override("separation", 1);

    _file_button = memnew(Button);
    _file_button->set_custom_minimum_size(Vector2(28, 0));
    _file_button->set_focus_mode(Control::FOCUS_NONE);
    _file_button->set_text(StringUtils::default_if_empty(_pin->get_effective_default_value(), _get_default_text()));
    _file_button->connect("pressed", callable_mp(this, &OrchestratorGraphNodePinFile::_on_show_file_dialog));
    container->add_child(_file_button);

    _clear_button = memnew(Button);
    _clear_button->set_focus_mode(Control::FOCUS_NONE);
    _clear_button->set_button_icon(SceneUtils::get_editor_icon("Reload"));
    _clear_button->connect("pressed", callable_mp(this, &OrchestratorGraphNodePinFile::_on_clear_file));
    _clear_button->set_visible(_file_button->get_text() != _get_default_text());
    container->add_child(_clear_button);

    return container;
}
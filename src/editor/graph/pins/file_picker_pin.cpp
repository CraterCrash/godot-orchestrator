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
#include "editor/graph/pins/file_picker_pin.h"

#include "common/callable_lambda.h"
#include "common/macros.h"
#include "editor/file_dialog.h"

void OrchestratorEditorGraphPinFilePicker::_handle_selector_button_pressed()
{
    _dialog = memnew(OrchestratorFileDialog);
    _dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
    _dialog->set_hide_on_ok(true);
    _dialog->set_title("Select a file");

    if (!_file_type_filters.is_empty())
        _dialog->set_filters(_file_type_filters);

    _dialog->connect("file_selected", callable_mp_lambda(this, [&](const String& v) { _handle_selector_button_response(v); }));
    _dialog->connect("canceled", callable_mp_cast(this, Node, queue_free));

    add_child(_dialog);

    _dialog->popup_file_dialog();
}

OrchestratorEditorGraphPinFilePicker::OrchestratorEditorGraphPinFilePicker()
{
    set_default_text("Assign...");
}
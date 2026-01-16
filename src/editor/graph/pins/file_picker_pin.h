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
#ifndef ORCHESTRATOR_EDITOR_GRAPH_PIN_FILE_PICKER_H
#define ORCHESTRATOR_EDITOR_GRAPH_PIN_FILE_PICKER_H

#include "editor/graph/pins/button_base_pin.h"

class OrchestratorFileDialog;

/// An implementation of <code>OrchestratorEditorGraphPinButtonBase</code> wrapping a <code>FileDialog</code>
/// that provides selecting a file path value.
///
class OrchestratorEditorGraphPinFilePicker : public OrchestratorEditorGraphPinButtonBase {
    GDCLASS(OrchestratorEditorGraphPinFilePicker, OrchestratorEditorGraphPinButtonBase);

    OrchestratorFileDialog* _dialog = nullptr;
    PackedStringArray _file_type_filters;

protected:
    static void _bind_methods() { }

    //~ Begin OrchestratorEditorGraphPinButtonBase Interface
    void _handle_selector_button_pressed() override;
    //~ End OrchestratorEditorGraphPinButtonBase Interface

public:
    void set_filters(const PackedStringArray& p_file_type_filters) { _file_type_filters = p_file_type_filters; }

    OrchestratorEditorGraphPinFilePicker();
};

#endif // ORCHESTRATOR_EDITOR_GRAPH_PIN_FILE_PICKER_H

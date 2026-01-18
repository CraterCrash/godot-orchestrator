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
#include "editor/graph/pins/input_action_picker_pin.h"

#include "common/macros.h"
#include "editor/editor.h"

void OrchestratorEditorGraphPinInputActionPicker::_update_action_items() {
    clear();

    const OrchestratorEditor* editor = OrchestratorEditor::get_singleton();
    for (const OrchestratorEditor::InputAction& action : editor->get_input_actions_cache()) {
        if (!action.name.begins_with("spatial_editor/")) {
            add_item(action.name);
        }
    }
}

Control* OrchestratorEditorGraphPinInputActionPicker::_create_default_value_widget() {
    Control* control = OrchestratorEditorGraphPinOptionPicker::_create_default_value_widget();

    set_tooltip_text("Actions defined in Project Settings: Input Map");

    // By linking to the OrchestratorEditor, it compares the existing action cache values with the most
    // recent ProjectSettings changed event, and if there are no input map changes, this signal isn't
    // fired, which helps keep the redraw/update noise in the edited graph minimized.
    OrchestratorEditor::get_singleton()->connect("input_action_cache_updated", callable_mp_this(_update_action_items));

    // Prepopulate the option list
    _update_action_items();

    return control;
}

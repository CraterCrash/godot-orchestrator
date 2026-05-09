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
#include "editor/graph/pins/input_action_picker_value_editor.h"

#include "common/macros.h"
#include "editor/editor.h"

void OrchestratorEditorGraphPinValueEditorInputActionPicker::_update_action_items() {
    clear();

    const OrchestratorEditor* editor = OrchestratorEditor::get_singleton();
    for (const OrchestratorEditor::InputAction& action : editor->get_input_actions_cache()) {
        if (!action.name.begins_with("spatial_editor/")) {
            add_item(action.name);
        }
    }
}

void OrchestratorEditorGraphPinValueEditorInputActionPicker::configure(const PropertyInfo& p_property) {
    OrchestratorEditorGraphPinValueEditorOptionPicker::configure(p_property);

    set_control_tooltip("Actions defined in Project Settings: Input Map");

    // The OrchestratorEditor compares the existing action cache values with the most recent ProjectSettings
    // changed event. If there are no changes, this signal isn't fired, which keeps redraw/update noise minimal.
    OrchestratorEditor::get_singleton()->connect("input_action_cache_updated", callable_mp_this(_update_action_items));

    _update_action_items();
}

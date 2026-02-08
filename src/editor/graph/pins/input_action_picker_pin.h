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
#ifndef ORCHESTRATOR_EDITOR_GRAPH_PIN_INPUT_ACTION_PICKER_H
#define ORCHESTRATOR_EDITOR_GRAPH_PIN_INPUT_ACTION_PICKER_H

#include "editor/graph/pins/option_picker_pin.h"

class OrchestratorEditorGraphPinInputActionPicker : public OrchestratorEditorGraphPinOptionPicker {
    GDCLASS(OrchestratorEditorGraphPinInputActionPicker, OrchestratorEditorGraphPinOptionPicker);

    void _update_action_items();

protected:
    static void _bind_methods() { }

    //~ Begin OrchestratorEditorGraphPin Interface
    Control* _create_default_value_widget() override;
    //~ End OrchestratorEditorGraphPin interface
};

#endif // ORCHESTRATOR_EDITOR_GRAPH_PIN_INPUT_ACTION_PICKER_H
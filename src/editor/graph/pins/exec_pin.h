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
#ifndef ORCHESTRATOR_EDITOR_GRAPH_PIN_EXEC_H
#define ORCHESTRATOR_EDITOR_GRAPH_PIN_EXEC_H

#include "editor/graph/graph_pin.h"

/// An implementation of <code>OrchestratorEditorGraphPin</code> execution pins.
///
/// An execution pin is one that manages control flow between the graph, and these typically
/// do not have labels, but most importantly any type or default value widgets.
///
class OrchestratorEditorGraphPinExec : public OrchestratorEditorGraphPin {
    GDCLASS(OrchestratorEditorGraphPinExec, OrchestratorEditorGraphPin);

protected:
    static void _bind_methods() { }

    //~ Begin OrchestratorEditorGraphPin Interface
    String _get_pin_color_name() const override;
    //~ End OrchestratorEditorGraphPin Interface
};

#endif // ORCHESTRATOR_EDITOR_GRAPH_PIN_EXEC_H
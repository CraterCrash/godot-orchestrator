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
#ifndef ORCHESTRATOR_GRAPH_NODE_PIN_BOOL_H
#define ORCHESTRATOR_GRAPH_NODE_PIN_BOOL_H

#include "editor/graph/graph_node_pin.h"

/// An implementation of OrchestratorGraphNodePin for boolean pin types that provides a check-box
/// to represent the default value associated with the pin.
class OrchestratorGraphNodePinBool : public OrchestratorGraphNodePin
{
    GDCLASS(OrchestratorGraphNodePinBool, OrchestratorGraphNodePin);

    static void _bind_methods();

protected:
    OrchestratorGraphNodePinBool() = default;

    /// Called when the default value is changed in the UI.
    /// @param p_new_value the new default value
    void _on_default_value_changed(bool p_new_value);

    //~ Begin OrchestratorGraphNodePin Interface
    Control* _get_default_value_widget() override;
    //~ End OrchestratorGraphNodePin Interface

public:
    OrchestratorGraphNodePinBool(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin);

};

#endif  // ORCHESTRATOR_GRAPH_NODE_PIN_BOOL_H

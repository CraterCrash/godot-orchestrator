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
#ifndef ORCHESTRATOR_GRAPH_NODE_PIN_EXEC_H
#define ORCHESTRATOR_GRAPH_NODE_PIN_EXEC_H

#include "editor/graph/graph_node_pin.h"

/// An implementation of OrchestratorGraphNodePin for execution control-flow pins.
class OrchestratorGraphNodePinExec : public OrchestratorGraphNodePin
{
    GDCLASS(OrchestratorGraphNodePinExec, OrchestratorGraphNodePin);

    static void _bind_methods();

protected:
    OrchestratorGraphNodePinExec() = default;

    //~ Begin OrchestratorGraphNodePin Interface
    String _get_color_name() const override;
    bool _can_promote_to_variable() const override { return false; }
    //~ End OrchestratorGraphNodePin Interface

public:
    OrchestratorGraphNodePinExec(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin);

    //~ Begin OrchestratorGraphNodePin Interface
    int get_slot_type() const override { return 0; }
    String get_slot_icon_name() const override { return "VisualShaderPort"; }
    //~ End OrchestratorGraphNodePin Interface
};

#endif  // ORCHESTRATOR_GRAPH_NODE_PIN_EXEC_H

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
#ifndef ORCHESTRATOR_GRAPH_NODE_PIN_OBJECT_H
#define ORCHESTRATOR_GRAPH_NODE_PIN_OBJECT_H

#include "editor/graph/graph_node_pin.h"

/// An implementation of OrchestratorGraphNodePin for Godot object pin types.
class OrchestratorGraphNodePinObject : public OrchestratorGraphNodePin
{
    GDCLASS(OrchestratorGraphNodePinObject, OrchestratorGraphNodePin);

    static void _bind_methods();

protected:
    OrchestratorGraphNodePinObject() = default;

    //~ Begin OrchestratorGraphNodePin Interface
    void _update_label() override;
    bool _is_label_updated_on_default_value_visibility_change() override { return true; }
    //~ End OrchestratorGraphNodePin Interface

public:
    OrchestratorGraphNodePinObject(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin);
};

#endif  // ORCHESTRATOR_GRAPH_NODE_PIN_OBJECT_H

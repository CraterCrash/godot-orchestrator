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
#ifndef ORCHESTRATOR_GRAPH_NODE_PIN_FACTORY_H
#define ORCHESTRATOR_GRAPH_NODE_PIN_FACTORY_H

#include "editor/graph/graph_node_pin.h"

/// A simple OrchestratorGraphNodePin factory helper
class OrchestratorGraphNodePinFactory
{
    // Intentionally private
    OrchestratorGraphNodePinFactory() = default;

    /// Resolves the string-based pin type
    /// @param p_node the graph node that will own the pin
    /// @param p_pin the pin to create a rendering widget for
    static OrchestratorGraphNodePin* _resolve_string_based_pin(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin);

public:

    /// Creates the appropriate OrchestratorGraphNodePin implementation for the given node and pin.
    /// @param p_node the graph node that will own the pin
    /// @param p_pin the orchestration script pin reference
    /// @return the orchestration graph node pin instance, never null
    static OrchestratorGraphNodePin* create_pin(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin);
};

#endif  // ORCHESTRATOR_GRAPH_NODE_PIN_FACTORY_H

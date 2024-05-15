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
#ifndef ORCHESTRATOR_GRAPH_NODE_FACTORY_H
#define ORCHESTRATOR_GRAPH_NODE_FACTORY_H

#include "editor/graph/graph_node.h"

/// A simple OrchestratorGraphNode factory helper
class OrchestratorGraphNodeFactory
{
    // Intentionally private
    OrchestratorGraphNodeFactory() = default;

public:

    /// Creates the appropriate OrchestratorGraphNode implementation for the given graph and node.
    /// @param p_graph the graph that will own the node
    /// @param p_node the orchestration node reference
    /// @return the editor graph node instance, never null
    static OrchestratorGraphNode* create_node(OrchestratorGraphEdit* p_graph, const Ref<OScriptNode>& p_node);

};

#endif  // ORCHESTRATOR_GRAPH_NODE_FACTORY_H

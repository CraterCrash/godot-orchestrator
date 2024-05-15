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
#include "graph_node_factory.h"

#include "editor/graph/nodes/graph_node_comment.h"
#include "editor/graph/nodes/graph_node_default.h"

OrchestratorGraphNode* OrchestratorGraphNodeFactory::create_node(OrchestratorGraphEdit* p_graph, const Ref<OScriptNode>& p_node)
{
    Ref<OScriptNodeComment> comment = p_node;
    if (comment.is_valid())
        return memnew(OrchestratorGraphNodeComment(p_graph, comment));

    return memnew(OrchestratorGraphNodeDefault(p_graph, p_node));
}
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
#ifndef ORCHESTRATOR_GRAPH_NODE_COMMENT_H
#define ORCHESTRATOR_GRAPH_NODE_COMMENT_H

#include "editor/graph/graph_node.h"

#include "script/nodes/utilities/comment.h"

#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>

/// A simple OrchestratorGraphNode implementation to render comment nodes.
class OrchestratorGraphNodeComment : public OrchestratorGraphNode
{
    GDCLASS(OrchestratorGraphNodeComment, OrchestratorGraphNode);

    static void _bind_methods();

protected:
    Label* _label{ nullptr };
    Ref<OScriptNodeComment> _comment_node;
    bool _selected{ false };

    OrchestratorGraphNodeComment() = default;

    /// Reorders graph nodes that intersect the comment node, making sure that any
    /// other nodes that intersect are positioned after this comment node.
    void raise_request_node_reorder();

    /// Called when the comment node is raised, brought to the front.
    void _on_raise_request();

    //~ Begin OrchestratorGraphNode Interface
    void _update_pins() override;
    bool _resize_on_update() const override { return false; }
    //~ End OrchestratorGraphNode Interface

public:
    OrchestratorGraphNodeComment(OrchestratorGraphEdit* p_graph, const Ref<OScriptNodeComment>& p_node);

    //~ Begin OrchestratorGraphNode Interface
    bool is_groupable() const override { return true; }
    bool is_group_selected() override;
    void select_group() override;
    void deselect_group() override;
    //~ End OrchestratorGraphNode Interface

    //~ Begin Object Interface
    void _notification(int p_what);
    //~ End Object Interface

    //~ Begin Control Interface
    void _gui_input(const Ref<InputEvent>& p_event) override;
    //~ End Control Interface
};

#endif // ORCHESTRATOR_GRAPH_NODE_COMMENT_H
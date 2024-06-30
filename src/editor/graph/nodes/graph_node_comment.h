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
#ifndef ORCHESTRATOR_GRAPH_NODE_COMMENT_H
#define ORCHESTRATOR_GRAPH_NODE_COMMENT_H

#include "editor/graph/graph_node.h"

#include "script/nodes/utilities/comment.h"

#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>

#if GODOT_VERSION >= 0x040300
#include <godot_cpp/classes/graph_frame.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#endif

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

#if GODOT_VERSION >= 0x040300
class OrchestratorGraphFrameComment : public GraphFrame
{
    GDCLASS(OrchestratorGraphFrameComment, GraphFrame);
    static void _bind_methods();

    struct ThemeCache
    {
        Ref<StyleBox> titlebar;
        Ref<StyleBox> titlebar_selected;
    };

    const int TITLEBAR_HEIGHT{ 30 };

protected:
    OrchestratorGraphEdit* _graph{ nullptr };
    Ref<OScriptNodeComment> _node;
    Label* _text{ nullptr };
    TextureRect* _icon{ nullptr };
    ThemeCache _theme_cache;

    //~ Begin Object Interface
    void _notification(int p_what);
    //~ End Object Interface

    void _node_moved(Vector2 p_old_pos, Vector2 p_new_pos);
    void _node_resized();
    void _script_node_changed();
    void _update_theme();

    /// Default constructor, intentionally protected
    OrchestratorGraphFrameComment() = default;

public:
    /// Get a reference to the underlying comment node
    /// @return the comment node or an invalid reference if its invalid.
    Ref<OScriptNodeComment> get_comment_node() { return _node; }

    /// Constructor
    /// @param p_graph the graph edit instance
    /// @param p_node the comment node
    OrchestratorGraphFrameComment(OrchestratorGraphEdit* p_graph, const Ref<OScriptNodeComment>& p_node);
};
#endif

#endif // ORCHESTRATOR_GRAPH_NODE_COMMENT_H
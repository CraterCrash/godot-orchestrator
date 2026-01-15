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
#ifndef ORCHESTRATOR_EDITOR_GRAPH_NODE_COMMENT_H
#define ORCHESTRATOR_EDITOR_GRAPH_NODE_COMMENT_H

#include "editor/graph/graph_node.h"

#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>

class OrchestratorEditorGraphNodeComment : public OrchestratorEditorGraphNode
{
    GDCLASS(OrchestratorEditorGraphNodeComment, OrchestratorEditorGraphNode);

    struct {
        Ref<StyleBoxFlat> panel;
        Ref<StyleBoxFlat> panel_selected;
    } _theme_cache;

    HBoxContainer* _title_hbox = nullptr;
    Label* _text = nullptr;

    void _raise_request();

protected:
    static void _bind_methods();

    //~ Begin OrchestratorEditorGraphNode Interface
    void _update_styles() override;
    //~ End OrchestratorEditorGraphNode Interface

public:
    //~ Begin Control Interface
    void _gui_input(const Ref<InputEvent>& p_event) override;
    bool _has_point(const Vector2& p_point) const override;
    //~ Begin OrchestratorEditorGraphNode Interface

    void update() override;
    //~ End OrchestratorEditorGraphNode Interface

    OrchestratorEditorGraphNodeComment();
};

#endif // ORCHESTRATOR_EDITOR_GRAPH_NODE_COMMENT_H
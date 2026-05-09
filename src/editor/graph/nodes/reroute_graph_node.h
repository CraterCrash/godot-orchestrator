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
#pragma once

#include "editor/graph/graph_node.h"

/// A compact visual graph node for reroute nodes.
///
/// Renders as a small dot with no title bar and minimal body, similar to how UE Blueprints
/// and the Godot Visual Shader display reroute nodes.
///
class OrchestratorEditorGraphNodeReroute : public OrchestratorEditorGraphNode {
    GDCLASS(OrchestratorEditorGraphNodeReroute, OrchestratorEditorGraphNode);

    const float FADE_ANIMATION_LENGTH_SECONDS = 0.3;
    float _icon_opacity = 0.f;

protected:
    static void _bind_methods() { }

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

    //~ Begin OrchestratorEditorGraphNode Interface
    void _update_styles() override;
    void _create_indicators() override { }     // No indicators on reroutes
    void _create_add_button_widgets() override { }  // No add buttons on reroutes
    void _create_pin_widgets() override;
    //~ End OrchestratorEditorGraphNode Interface

    void _mouse_entered();
    void _mouse_exited();

public:

    //~ Begin Control Interface
    bool _has_point(const Vector2& p_point) const override;
    //~ End Control Interface

    //~ Begin GraphNode Interface
    void _draw_port(int32_t p_slot_index, const Vector2i& p_position, bool p_left, const Color& p_color) override;
    //~ End GraphNode Interface

    void set_icon_opacity(float p_opacity);
};

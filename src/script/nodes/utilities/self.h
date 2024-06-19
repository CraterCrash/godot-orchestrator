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
#ifndef ORCHESTRATOR_SCRIPT_NODE_SELF_H
#define ORCHESTRATOR_SCRIPT_NODE_SELF_H

#include "script/script.h"

/// A node that outputs a reference to self, which is the orchestration and the
/// owning node of the OrchestratorScript instance.
class OScriptNodeSelf : public OScriptNode
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeSelf, OScriptNode);
    static void _bind_methods() { }

    void _on_script_changed();

public:
    //~ Begin OScriptNode Interface
    void post_initialize() override;
    void post_placed_new_node() override;
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "variable"; }
    bool should_draw_as_bead() const override { return true; }
    String get_icon() const override;
    OScriptNodeInstance* instantiate() override;
    //~ End OScriptNode Interface
};

#endif  // ORCHESTRATOR_SCRIPT_NODE_SELF_H

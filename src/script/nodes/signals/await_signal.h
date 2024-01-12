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
#ifndef ORCHESTRATOR_SCRIPT_NODE_AWAIT_SIGNAL_H
#define ORCHESTRATOR_SCRIPT_NODE_AWAIT_SIGNAL_H

#include "script/script.h"

/// Awaits a signal.
///
/// Much like GDScript, you can use the "await" keyword to create a coroutine that will yield
/// and waits until the specified signal is raised before the program flow continues. This
/// node is designed to provide the same functionality in Orchestrator.
///
/// This node requires two inputs, the object that will emit the signal and the signal name
/// that should cause the yield until it is raised.
///
class OScriptNodeAwaitSignal : public OScriptNode
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeAwaitSignal, OScriptNode);

public:
    //~ Begin OScriptNode Interface
    void post_initialize() override;
    void post_placed_new_node() override;
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "signals"; }
    bool validate_node_during_build() const override;
    OScriptNodeInstance* instantiate(OScriptInstance* p_instance) override;
    //~ End OScriptNode Interface
};

#endif // ORCHESTRATOR_SCRIPT_NODE_AWAIT_SIGNAL_H

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
#ifndef ORCHESTRATOR_NODE_PRINT_STRING_H
#define ORCHESTRATOR_NODE_PRINT_STRING_H

#include "script/script.h"

using namespace godot;

/// A custom function that allows for printing text to the render viewport.
///
/// During gameplay, there is often a need to output details about what may be happening in
/// an Orchestration; however, you typically only want this to occur in the editor or when
/// your game is started from the editor. This node allows for this functionality and will
/// not perform any actions when your games are exported.
///
class OScriptNodePrintString : public OScriptNode
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodePrintString, OScriptNode);
    static void _bind_methods() { }

public:

    //~ Begin OScriptNode Interface
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override { return "Print String"; }
    String get_node_title_color_name() const override { return "function_call"; }
    String get_icon() const override { return "MemberMethod"; }
    void reallocate_pins_during_reconstruction(const Vector<Ref<OScriptNodePin>>& p_old_pins) override;
    OScriptNodeInstance* instantiate() override;
    //~ End OScriptNode Interface

    OScriptNodePrintString();
};

#endif  // ORCHESTRATOR_NODE_PRINT_STRING_H
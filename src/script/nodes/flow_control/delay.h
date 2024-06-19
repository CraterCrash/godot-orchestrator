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
#ifndef ORCHESTRATOR_SCRIPT_NODE_DELAY_H
#define ORCHESTRATOR_SCRIPT_NODE_DELAY_H

#include "script/script.h"

/// Performs a flow delay for the specified duration.
///
/// @details This is provided purely for experimental purposes and may likely be removed
/// in a future build as introducing hard-coded delays are generally not something that
/// is ideal or should be implemented.
///
class OScriptNodeDelay : public OScriptNode
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeDelay, OScriptNode);
    static void _bind_methods() { }

protected:
    float _duration{ 1 };  //! Delay duration

public:
    //~ Begin OScriptNode Interface
    void post_initialize() override;
    void reallocate_pins_during_reconstruction(const Vector<Ref<OScriptNodePin>>& p_old_pins) override;
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "flow_control"; }
    String get_icon() const override;
    OScriptNodeInstance* instantiate() override;
    //~ End OScriptNode Interface
};

#endif  // ORCHESTRATOR_SCRIPT_NODE_DELAY_H

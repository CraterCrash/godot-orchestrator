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
#ifndef ORCHESTRATOR_SCRIPT_NODE_DIALOGUE_MESSAGE_H
#define ORCHESTRATOR_SCRIPT_NODE_DIALOGUE_MESSAGE_H

#include "script/nodes/editable_pin_node.h"

/// A node that represents a dialogue message that is part of a conversation.
class OScriptNodeDialogueMessage : public OScriptEditablePinNode
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeDialogueMessage, OScriptEditablePinNode);
    static void _bind_methods() { }

protected:
    int _choices{ 0 };

    //~ Begin OScriptNode Interface
    void _upgrade(uint32_t p_version, uint32_t p_current_version) override;
    //~ End OScriptNode Interface

public:
    //~ Begin OScriptNode Interface
    void post_initialize() override;
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "dialogue"; }
    void validate_node_during_build(BuildLog& p_log) const override;
    OScriptNodeInstance* instantiate() override;
    //~ End OScriptNode Interface

    //~ Begin OScriptEditablePinNode Interface
    void add_dynamic_pin() override;
    bool can_add_dynamic_pin() const override;
    bool can_remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin) const override;
    void remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin) override;
    String get_pin_prefix() const override { return "choice"; }
    //~ End OScriptEditablePinNode Interface
};

#endif // ORCHESTRATOR_SCRIPT_NODE_DIALOGUE_MESSAGE_H
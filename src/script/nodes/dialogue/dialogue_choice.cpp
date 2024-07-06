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
#include "dialogue_choice.h"

class OScriptNodeDialogueChoiceInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeDialogueChoice)
public:
    int step(OScriptExecutionContext& p_context) override
    {
        Dictionary dict;
        dict["text"] = p_context.get_input(0);
        dict["visible"] = p_context.get_input(1);
        p_context.set_output(0, dict);
        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeDialogueChoice::allocate_default_pins()
{
    create_pin(PD_Input, PT_Data, "text", Variant::STRING)->set_flag(OScriptNodePin::Flags::MULTILINE);
    create_pin(PD_Input, PT_Data, "visible", Variant::BOOL);
    create_pin(PD_Output, PT_Data, "choice", Variant::OBJECT);
}

String OScriptNodeDialogueChoice::get_tooltip_text() const
{
    return "Creates a dialogue message choice that can be selected by the player.";
}

String OScriptNodeDialogueChoice::get_node_title() const
{
    return "Dialogue Choice";
}

OScriptNodeInstance* OScriptNodeDialogueChoice::instantiate()
{
    OScriptNodeDialogueChoiceInstance* i = memnew(OScriptNodeDialogueChoiceInstance);
    i->_node = this;
    return i;
}


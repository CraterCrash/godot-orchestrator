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

#include "common/property_utils.h"

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
    create_pin(PD_Input, PT_Data, PropertyUtils::make_multiline("text"));
    create_pin(PD_Input, PT_Data, PropertyUtils::make_typed("visible", Variant::BOOL), true);

    // todo: this outputs a dictionary but we encode this as an object, any issue here?
    create_pin(PD_Output, PT_Data, PropertyUtils::make_object("choice", get_class_static()));
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

void OScriptNodeDialogueChoice::validate_node_during_build(BuildLog& p_log) const
{
    const Ref<OScriptNodePin> output = find_pin("choice", PD_Output);
    if (output.is_valid() && !output->has_any_connections())
        p_log.error(this, output, "Requires a connection.");

    super::validate_node_during_build(p_log);
}


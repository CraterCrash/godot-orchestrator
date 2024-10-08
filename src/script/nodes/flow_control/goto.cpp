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
#include "goto.h"

#include "common/property_utils.h"

class OScriptNodeGotoInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeGoto);

public:
    int step(OScriptExecutionContext& p_context) override
    {
        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeGoto::allocate_default_pins()
{
    create_pin(PD_Input, PT_Data, PropertyUtils::make_typed("Name", Variant::STRING), "StartHere");
    create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("ExecOut"));

    super::allocate_default_pins();
}

String OScriptNodeGoto::get_tooltip_text() const
{
    return "Begins orchestration execution from this node.";
}

String OScriptNodeGoto::get_node_title() const
{
    return "Goto" + name;
}

String OScriptNodeGoto::get_icon() const
{
    return "VcsBranches";
}

OScriptNodeInstance* OScriptNodeGoto::instantiate()
{
    OScriptNodeGotoInstance* i = memnew(OScriptNodeGotoInstance);
    i->_node = this;
    return i;
}

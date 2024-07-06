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
#include "branch.h"

class OScriptNodeBranchInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeBranch);

public:
    int step(OScriptExecutionContext& p_context) override
    {
        return p_context.get_input(0) ? 0 : 1;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeBranch::allocate_default_pins()
{
    Ref<OScriptNodePin> exec_in = create_pin(PD_Input, PT_Execution, "ExecIn");
    exec_in->set_label("if [condition]");

    create_pin(PD_Input, PT_Data, "condition", Variant::BOOL, false);

    create_pin(PD_Output, PT_Execution, "true")->show_label();
    create_pin(PD_Output, PT_Execution, "false")->show_label();

    super::allocate_default_pins();
}

String OScriptNodeBranch::get_tooltip_text() const
{
    return "If condition is true, execution goes to true; otherwise, it goes to false.";
}

String OScriptNodeBranch::get_node_title() const
{
    return "Branch";
}

String OScriptNodeBranch::get_icon() const
{
    return "VcsBranches";
}

OScriptNodeInstance* OScriptNodeBranch::instantiate()
{
    OScriptNodeBranchInstance* i = memnew(OScriptNodeBranchInstance);
    i->_node = this;
    return i;
}

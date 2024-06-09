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
#include "while.h"

class OScriptNodeWhileInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeWhile);

public:
    int step(OScriptExecutionContext& p_context) override
    {
        Variant& condition = p_context.get_input(0);
        if (condition.get_type() != Variant::BOOL)
        {
            p_context.set_invalid_argument(this, 0, condition.get_type(), Variant::BOOL);
            return -1;
        }

        const bool status = condition.operator bool();
        return status ? (0 | STEP_FLAG_PUSH_STACK_BIT) : 1;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeWhile::allocate_default_pins()
{
    Ref<OScriptNodePin> exec_in = create_pin(PD_Input, "ExecIn");
    exec_in->set_flags(OScriptNodePin::Flags::EXECUTION | OScriptNodePin::Flags::SHOW_LABEL);
    exec_in->set_label("while [condition]");

    create_pin(PD_Input, "condition", Variant::BOOL, _condition)->set_flags(OScriptNodePin::Flags::DATA);

    create_pin(PD_Output, "repeat")->set_flags(OScriptNodePin::Flags::EXECUTION | OScriptNodePin::Flags::SHOW_LABEL);
    create_pin(PD_Output, "done")->set_flags(OScriptNodePin::Flags::EXECUTION | OScriptNodePin::Flags::SHOW_LABEL);
}

String OScriptNodeWhile::get_tooltip_text() const
{
    return "Repeatedly executes the 'Loop Body' as long as the condition is true.";
}

String OScriptNodeWhile::get_node_title() const
{
    return "While Loop";
}

String OScriptNodeWhile::get_icon() const
{
    return "Loop";
}

OScriptNodeInstance* OScriptNodeWhile::instantiate()
{
    OScriptNodeWhileInstance* i = memnew(OScriptNodeWhileInstance);
    i->_node = this;
    return i;
}

void OScriptNodeWhile::initialize(const OScriptNodeInitContext& p_context)
{
    _condition = false;
    super::initialize(p_context);
}
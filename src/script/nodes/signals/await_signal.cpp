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
#include "await_signal.h"

#include "script/vm/script_state.h"

class OScriptNodeAwaitSignalInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeAwaitSignal);

public:
    int get_working_memory_size() const override { return 1; }

    int step(OScriptExecutionContext& p_context) override
    {
        // Check whether the signal was raised and we should resume.
        if (p_context.get_step_mode() == STEP_MODE_RESUME)
            return 0;

        // Get the target, falling back to self when not specified.
        Object* target = p_context.get_input(0);
        if (!target)
            target = p_context.get_owner();


        const String signal_name = p_context.get_input(1);
        if (!target->has_signal(signal_name))
        {
            p_context.set_error(GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT, "No signal defined on target.");
            p_context.set_invalid_argument(this, 1, Variant::STRING_NAME, Variant::STRING_NAME);
            return -1;
        }

        // Connect to signal and await
        Ref<OScriptState> state;
        state.instantiate();
        state->connect_to_signal(target, signal_name, Array());
        p_context.set_working_memory(0, state);
        return STEP_FLAG_YIELD;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeAwaitSignal::post_initialize()
{
    super::post_initialize();
}

void OScriptNodeAwaitSignal::post_placed_new_node()
{
    super::post_placed_new_node();
}

void OScriptNodeAwaitSignal::allocate_default_pins()
{
    create_pin(PD_Input, "ExecIn")->set_flags(OScriptNodePin::Flags::EXECUTION);
    create_pin(PD_Input, "target", Variant::OBJECT)->set_flags(OScriptNodePin::Flags::DATA);
    create_pin(PD_Input, "signal_name", Variant::STRING)->set_flags(OScriptNodePin::Flags::DATA);
    create_pin(PD_Output, "ExecOut")->set_flags(OScriptNodePin::Flags::EXECUTION);
    super::allocate_default_pins();
}

String OScriptNodeAwaitSignal::get_tooltip_text() const
{
    return "Yields/Awaits the script's execution until the given signal occurs.";
}

String OScriptNodeAwaitSignal::get_node_title() const
{
    return "Await Signal";
}

void OScriptNodeAwaitSignal::validate_node_during_build(BuildLog& p_log) const
{
    return super::validate_node_during_build(p_log);
}

OScriptNodeInstance* OScriptNodeAwaitSignal::instantiate()
{
    OScriptNodeAwaitSignalInstance* i = memnew(OScriptNodeAwaitSignalInstance);
    i->_node = this;
    return i;
}
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
#include "delay.h"

#include "script/vm/script_state.h"

#include <godot_cpp/classes/main_loop.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/scene_tree_timer.hpp>

class OScriptNodeDelayInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeDelay);
public:
    int get_working_memory_size() const override { return 1; }

    int step(OScriptExecutionContext& p_context) override
    {
        // Resume mode means that the delay has concluded, it's safe to proceed.
        if (p_context.get_step_mode() == STEP_MODE_RESUME)
            return 0;

        SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
        if (!tree)
        {
            p_context.set_error(GDEXTENSION_CALL_ERROR_INVALID_METHOD, "Main loop is not a scene tree");
            return -1;
        }

        float duration = p_context.get_input(0);

        // Construct node state
        Ref<OScriptState> state;
        state.instantiate();

        // Associate it with a scene tree timer for the delay
        state->connect_to_signal(tree->create_timer(duration).ptr(), "timeout", Array());

        // Stash the state and return yield request
        p_context.set_working_memory(0, state);
        return STEP_FLAG_YIELD;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeDelay::post_initialize()
{
    _duration = find_pin("duration", PD_Input)->get_effective_default_value();
    super::post_initialize();
}

void OScriptNodeDelay::reallocate_pins_during_reconstruction(const Vector<Ref<OScriptNodePin>>& p_old_pins)
{
    super::reallocate_pins_during_reconstruction(p_old_pins);

    for (const Ref<OScriptNodePin>& pin : p_old_pins)
    {
        if (pin->is_input() && !pin->is_execution())
        {
            const Ref<OScriptNodePin> new_input = find_pin(pin->get_pin_name(), PD_Input);
            if (new_input.is_valid())
                new_input->set_default_value(pin->get_default_value());
        }
    }
}

void OScriptNodeDelay::allocate_default_pins()
{
    create_pin(PD_Input, "ExecIn")->set_flags(OScriptNodePin::Flags::EXECUTION);
    create_pin(PD_Input, "duration", Variant::FLOAT, _duration)->set_flags(OScriptNodePin::Flags::DATA);
    create_pin(PD_Output, "ExecOut")->set_flags(OScriptNodePin::Flags::EXECUTION);

    super::allocate_default_pins();
}

String OScriptNodeDelay::get_tooltip_text() const
{
    return "Causes the orchestration flow to pause processing for the specified number of seconds.";
}

String OScriptNodeDelay::get_node_title() const
{
    return "Delay";
}

String OScriptNodeDelay::get_icon() const
{
    return "Timer";
}

OScriptNodeInstance* OScriptNodeDelay::instantiate()
{
    OScriptNodeDelayInstance *i = memnew(OScriptNodeDelayInstance);
    i->_node = this;
    return i;
}
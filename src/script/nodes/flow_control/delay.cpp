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
#include "script/nodes/flow_control/delay.h"

#include "common/property_utils.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>

void OScriptNodeDelay::post_initialize() {
    const Ref<OScriptNodePin> duration  = find_pin("duration", PD_Input);
    if (duration.is_valid()) {
        _duration = duration->get_effective_default_value();
    }
    super::post_initialize();
}

void OScriptNodeDelay::reallocate_pins_during_reconstruction(const Vector<Ref<OScriptNodePin>>& p_old_pins) {
    super::reallocate_pins_during_reconstruction(p_old_pins);

    for (const Ref<OScriptNodePin>& pin : p_old_pins) {
        if (pin->is_input() && !pin->is_execution()) {
            const Ref<OScriptNodePin> new_input = find_pin(pin->get_pin_name(), PD_Input);
            if (new_input.is_valid()) {
                new_input->set_default_value(pin->get_default_value());
            }
        }
    }
}

void OScriptNodeDelay::allocate_default_pins() {
    create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"));
    create_pin(PD_Input, PT_Data, PropertyUtils::make_typed("duration", Variant::FLOAT), _duration);
    create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("ExecOut"));

    super::allocate_default_pins();
}

String OScriptNodeDelay::get_tooltip_text() const {
    return "Causes the orchestration flow to pause processing for the specified number of seconds.";
}

String OScriptNodeDelay::get_node_title() const {
    return "Delay";
}

String OScriptNodeDelay::get_icon() const {
    return "Timer";
}

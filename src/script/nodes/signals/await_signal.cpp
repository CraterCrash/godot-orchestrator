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
#include "script/nodes/signals/await_signal.h"

#include "common/property_utils.h"

void OScriptNodeAwaitSignal::allocate_default_pins() {
    create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"));
    create_pin(PD_Input, PT_Data, PropertyUtils::make_object("target"));
    create_pin(PD_Input, PT_Data, PropertyUtils::make_typed("signal_name", Variant::STRING));
    create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("ExecOut"));
    super::allocate_default_pins();
}

String OScriptNodeAwaitSignal::get_tooltip_text() const {
    return "Yields/Awaits the script's execution until the given signal occurs.";
}

String OScriptNodeAwaitSignal::get_node_title() const {
    return "Await Signal";
}

void OScriptNodeAwaitSignal::validate_node_during_build(BuildLog& p_log) const {
    // todo: need to validate signal exists on target object instance
    return super::validate_node_during_build(p_log);
}

void OScriptNodeAwaitSignal::on_pin_disconnected(const Ref<OScriptNodePin>& p_pin) {
    // Makes sure that signal list pin changes to string renderer
    if (p_pin.is_valid() && p_pin->get_pin_name().match("target")) {
        _notify_pins_changed();
    }
    super::on_pin_disconnected(p_pin);
}

PackedStringArray OScriptNodeAwaitSignal::get_suggestions(const Ref<OScriptNodePin>& p_pin) {
    if (p_pin.is_valid() && p_pin->is_input() && p_pin->get_pin_name().match("signal_name"))
    {
        const Ref<OScriptNodePin> target_pin = find_pin("target", PD_Input);
        if (target_pin.is_valid()) {
            return target_pin->resolve_signal_names();
        }
    }
    return super::get_suggestions(p_pin);
}

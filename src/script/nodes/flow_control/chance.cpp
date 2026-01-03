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
#include "script/nodes/flow_control/chance.h"

#include "common/property_utils.h"

void OScriptNodeChance::_get_property_list(List<PropertyInfo>* r_list) const {
    r_list->push_back(PropertyInfo(Variant::INT, "chance", PROPERTY_HINT_RANGE, "0,100"));
}

bool OScriptNodeChance::_get(const StringName& p_name, Variant& r_value) const {
    if (p_name.match("chance")) {
        r_value = _chance;
        return true;
    }
    return false;
}

bool OScriptNodeChance::_set(const StringName& p_name, const Variant& p_value) {
    if (p_name.match("chance")) {
        _chance = p_value;
        _notify_pins_changed();
        return true;
    }
    return false;
}

void OScriptNodeChance::post_initialize() {
    reconstruct_node();
    super::post_initialize();
}

void OScriptNodeChance::allocate_default_pins() {
    create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"));

    create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("Within"))->set_label(vformat("0 to %d %%", _chance));
    create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("Outside"))->set_label(vformat("%d to 100 %%", _chance + 1));

    super::allocate_default_pins();
}

String OScriptNodeChance::get_tooltip_text() const {
    return "Calculates a percentage chance (0 to 100), taking the path based on the chance.";
}

String OScriptNodeChance::get_node_title() const {
    return "Chance";
}

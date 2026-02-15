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
#include "script/nodes/properties/property_set.h"

#include "common/property_utils.h"
#include "script/script.h"

#include <godot_cpp/classes/node.hpp>

void OScriptNodePropertySet::allocate_default_pins() {
    create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"));

    if (_call_mode == CALL_INSTANCE) {
        Ref<OScriptNodePin> target = create_pin(PD_Input, PT_Data, PropertyUtils::make_object("target", _base_type));
        target->set_label(_base_type);
        target->no_pretty_format();
    }

    create_pin(PD_Input, PT_Data, _property);
    create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("ExecOut"));
}

String OScriptNodePropertySet::get_tooltip_text() const {
    if (!_property.name.is_empty()) {
        String tooltip = vformat("Sets the value of the property '%s'", _property.name);
        if (!_node_path.is_empty()) {
            tooltip += "\nNode Path: " + _node_path;
        }
        return tooltip;
    }

    return "Sets the value for a given property";
}

String OScriptNodePropertySet::get_node_title() const {
    return vformat("Set %s%s", _property.name.capitalize(), _call_mode == CALL_SELF ? " (Self)" : "");
}

void OScriptNodePropertySet::set_default_value(const Variant& p_default_value) {
    find_pin(1, PD_Input)->set_default_value(p_default_value);
}
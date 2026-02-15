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
#include "script/nodes/properties/property_get.h"

#include "common/property_utils.h"
#include "script/script.h"

#include <godot_cpp/classes/node.hpp>

void OScriptNodePropertyGet::allocate_default_pins() {
    if (_call_mode == CALL_INSTANCE) {
        Ref<OScriptNodePin> target = create_pin(PD_Input, PT_Data, PropertyUtils::make_object("target", _base_type));
        target->set_label(_base_type);
        target->no_pretty_format();
    }

    create_pin(PD_Output, PT_Data, _property);
}

String OScriptNodePropertyGet::get_tooltip_text() const {
    if (!_property.name.is_empty()) {
        String tooltip = vformat("Returns the value of the property '%s'.", _property.name);
        if (!_node_path.is_empty()) {
            tooltip += "\nNode Path: " + _node_path;
        }
        return tooltip;
    }
    return "Returns the value of a given property";
}

String OScriptNodePropertyGet::get_node_title() const {
    return vformat("Get %s%s", _property.name.capitalize(), _call_mode == CALL_SELF ? " (Self)" : "");
}

StringName OScriptNodePropertyGet::resolve_type_class(const Ref<OScriptNodePin>& p_pin) const {
    if (p_pin.is_valid() && p_pin->is_output()) {
        if (!_property.hint_string.is_empty()) {
            return _property.hint_string;
        }
        if (!_base_type.is_empty()) {
            return _base_type;
        }
    }
    return super::resolve_type_class(p_pin);
}

void OScriptNodePropertyGet::initialize(const OScriptNodeInitContext& p_context) {
    super::initialize(p_context);
}
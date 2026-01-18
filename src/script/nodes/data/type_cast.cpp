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
#include "script/nodes/data/type_cast.h"

#include "common/property_utils.h"
#include "common/string_utils.h"

void OScriptNodeTypeCast::_get_property_list(List<PropertyInfo>* r_list) const {
    r_list->push_back(PropertyInfo(Variant::STRING, "type", PROPERTY_HINT_TYPE_STRING, "Object", PROPERTY_USAGE_DEFAULT));
}

bool OScriptNodeTypeCast::_get(const StringName& p_name, Variant& r_value) const {
    if (p_name == StringName("type")) {
        r_value = _target_type;
        return true;
    }
    return false;
}

bool OScriptNodeTypeCast::_set(const StringName& p_name, const Variant& p_value) {
    if (p_name == StringName("type")) {
        _target_type = p_value;
        _notify_pins_changed();
        return true;
    }
    return false;
}

void OScriptNodeTypeCast::_upgrade(uint32_t p_version, uint32_t p_current_version) {
    if (p_version == 1 && p_current_version >= 2) {
        // Fixup - encode class type in the output pin
        Ref<OScriptNodePin> output = find_pin("output", PD_Output);
        if (!output.is_valid() || output->get_property_info().class_name.is_empty()) {
            reconstruct_node();
        }
    }

    super::_upgrade(p_version, p_current_version);
}

String OScriptNodeTypeCast::_get_target_type() const {
    return StringUtils::default_if_empty(_target_type, "Object");
}

void OScriptNodeTypeCast::post_node_autowired(const Ref<OScriptNode>& p_node, EPinDirection p_direction) {
    if (p_direction == PD_Output) {
        // User is auto-wiring from an output pin to this node's input
        Ref<OScriptNodePin> exec_in = find_pin("ExecIn", PD_Input);
        if (exec_in.is_valid() && !exec_in->has_any_connections()) {
            // Attempt to auto-wire the execution flow
            for (const Ref<OScriptNodePin>& out : p_node->find_pins(PD_Output)) {
                if (out.is_valid() && out->is_execution()) {
                    out->link(exec_in);
                    break;
                }
            }
        }

        Ref<OScriptNodePin> instance = find_pin("instance", PD_Input);
        if (instance.is_valid()) {
            if (!instance->has_any_connections()) {
                // Attempt to auto-wire the instance pin
                for (const Ref<OScriptNodePin>& out : p_node->find_pins(PD_Output)) {
                    if (out.is_valid() && !out->is_execution() && out->get_type() == Variant::OBJECT) {
                        out->link(instance);
                        break;
                    }
                }
            }

            for (const Ref<OScriptNodePin>& pin : instance->get_connections()) {
                _target_type = pin->get_owning_node()->resolve_type_class(pin);
                reconstruct_node();
                break;
            }
        }
    }

    super::post_node_autowired(p_node, p_direction);
}

void OScriptNodeTypeCast::allocate_default_pins() {
    create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"));
    create_pin(PD_Input, PT_Data, PropertyUtils::make_object("instance"));

    create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("yes"))->show_label();
    create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("no"))->show_label();

    Ref<OScriptNodePin> output = create_pin(PD_Output, PT_Data, PropertyUtils::make_object("output", _target_type));
    output->set_label("as " + StringUtils::default_if_empty(_target_type, "Object"), false);
}

String OScriptNodeTypeCast::get_tooltip_text() const {
    if (!_get_target_type().is_empty()) {
        return vformat("Tries to access the object as a '%s', it may be an instance of.", _get_target_type());
    } else {
        return "Tries to access the object as the given type.";
    }
}

String OScriptNodeTypeCast::get_node_title() const {
    return vformat("Cast To %s", _get_target_type());
}

String OScriptNodeTypeCast::get_icon() const {
    return StringUtils::default_if_empty(_target_type, super::get_icon());
}

void OScriptNodeTypeCast::initialize(const OScriptNodeInitContext& p_context) {
    if (p_context.class_name) {
        const StringName& class_name = p_context.class_name.value();
        if (!class_name.is_empty()) {
            _target_type = class_name;
        }
    }
    super::initialize(p_context);
}

StringName OScriptNodeTypeCast::resolve_type_class(const Ref<OScriptNodePin>& p_pin) const {
    if (p_pin->is_output()) {
        switch (p_pin->get_pin_index()) {
            case 1:
                return "Object";
            case 0:
            case 2:
                return StringUtils::default_if_empty(_target_type, "Object");
        }
    }
    return super::resolve_type_class(p_pin);
}

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
#include "orchestration/nodes/variables.h"

#include "common/dictionary_utils.h"
#include "common/macros.h"
#include "common/property_utils.h"
#include "common/variant_utils.h"
#include "orchestration/orchestration.h"

void OScriptNodeVariable::_get_property_list(List<PropertyInfo>* r_list) const {
    r_list->push_back(PropertyInfo(Variant::STRING, "variable_name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
}

bool OScriptNodeVariable::_get(const StringName& p_name, Variant& r_value) const {
    if (p_name.match("variable_name")) {
        r_value = _variable_name;
        return true;
    }
    return false;
}

bool OScriptNodeVariable::_set(const StringName& p_name, const Variant& p_value) {
    if (p_name.match("variable_name")) {
        _variable_name = p_value;
        return true;
    }
    return false;
}

void OScriptNodeVariable::_on_variable_changed() {
    if (_variable.is_valid()) {
        _variable_name = _variable->get_variable_name();
        reconstruct_node();

        // This must be triggered after reconstruction
        _variable_changed();
    }
}

void OScriptNodeVariable::post_initialize() {
    if (!_variable_name.is_empty()) {
        _variable = get_orchestration()->get_variable(_variable_name);
        if (_variable.is_valid() && _is_in_editor()) {
            OCONNECT(_variable, "changed", callable_mp_this(_on_variable_changed));
        }
    }
    super::post_initialize();
}

String OScriptNodeVariable::get_icon() const {
    return "MemberProperty";
}

void OScriptNodeVariable::initialize(const OScriptNodeInitContext& p_context) {
    ERR_FAIL_COND_MSG(!p_context.variable_name, "Failed to initialize Variable without a variable name");

    _variable_name = p_context.variable_name.value();
    _variable = get_orchestration()->get_variable(_variable_name);

    if (_variable.is_valid() && _is_in_editor()) {
        OCONNECT(_variable, "changed", callable_mp_this(_on_variable_changed));
    }

    super::initialize(p_context);
}

OScriptNodeVariable::OScriptNodeVariable() {
    // Catalog versions are added explicitly
    _flags = NONE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OScriptNodeVariableGet
///

void OScriptNodeVariableGet::_get_property_list(List<PropertyInfo>* r_list) const {
    r_list->push_back(PropertyInfo(Variant::BOOL, "validated", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
}

bool OScriptNodeVariableGet::_get(const StringName& p_name, Variant& r_value) const {
    if (p_name.match("validated")) {
        r_value = _validated;
        return true;
    }

    // todo: GodotCPP expects this to be done by the developer, Wrapped::get_bind doesn't do this
    // see https://github.com/godotengine/godot-cpp/pull/1539
    return OScriptNodeVariable::_get(p_name, r_value);
}

bool OScriptNodeVariableGet::_set(const StringName& p_name, const Variant& p_value) {
    if (p_name.match("validated")) {
        _validated = p_value;
        return true;
    }

    // todo: GodotCPP expects this to be done by the developer, Wrapped::set_bind doesn't do this
    // see https://github.com/godotengine/godot-cpp/pull/1539
    return OScriptNodeVariable::_set(p_name, p_value);
}

void OScriptNodeVariableGet::_upgrade(uint32_t p_version, uint32_t p_current_version) {
    if (p_version == 1 && p_current_version >= 2) {
        // Fixup - makes sure that stored property matches variable, if not reconstructs
        if (_variable.is_valid()) {
            const Ref<OScriptNodePin> output = find_pin("value", PD_Output);
            if (output.is_valid() && !PropertyUtils::are_equal(_variable->get_info(), output->get_property_info())) {
                reconstruct_node();
            }
        }
    }

    super::_upgrade(p_version, p_current_version);
}

void OScriptNodeVariableGet::_variable_changed() {
    if (_is_in_editor()) {
        if (!can_be_validated() && _validated) {
            set_validated(false);
        }

        // Defer this so that all nodes have updated
        // This is necessary so that all target types that may have changed (i.e. get connected to set)
        // have updated to make sure that the "can_accept" logic works as expected.
        callable_mp_this(_validate_output_connection).call_deferred();
    }

    super::_variable_changed();
}

void OScriptNodeVariableGet::_validate_output_connection() {
    Ref<OScriptNodePin> output = find_pin("value", PD_Output);
    if (output.is_valid() && output->has_any_connections()) {
        for (const Ref<OScriptNodePin>& target : output->get_connections()) {
            if (target.is_valid() && !target->can_accept(output)) {
                output->unlink(target);
            }
        }
    }
}

void OScriptNodeVariableGet::allocate_default_pins() {
    if (_validated) {
        create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"));
        create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("is_valid"))->set_label("Is Valid");
        create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("is_invalid"))->set_label("Is Invalid");
    }

    create_pin(PD_Output, PT_Data, PropertyUtils::as("value", _variable->get_info()))->set_label(_variable_name, false);

    super::allocate_default_pins();
}

String OScriptNodeVariableGet::get_tooltip_text() const {
    if (_variable.is_valid()) {
        String tooltip_text = vformat("Read the value of variable %s", _variable->get_variable_name());
        if (!_variable->get_description().is_empty()) {
            tooltip_text += "\n\nDescription:\n" + _variable->get_description();
        }
        return tooltip_text;
    }

    return "Read the value of a variable";
}

String OScriptNodeVariableGet::get_node_title() const {
    return vformat("Get %s", _variable->get_variable_name());
}

void OScriptNodeVariableGet::initialize(const OScriptNodeInitContext& p_context) {
    if (p_context.user_data) {
        const Dictionary& data = p_context.user_data.value();
        if (data.has("validation")) {
            _validated = data["validation"];
        }
    }

    super::initialize(p_context);
}

bool OScriptNodeVariableGet::can_be_validated() {
    PropertyInfo property = _variable->get_info();
    if (property.type == Variant::OBJECT) {
        return true;
    }
    return false;
}

void OScriptNodeVariableGet::set_validated(bool p_validated) {
    if (_validated != p_validated) {
        _validated = p_validated;

        if (!_validated) {
            // Disconnect any control flow pins, if they exist
            Ref<OScriptNodePin> exec_in = find_pin("ExecIn", PD_Input);
            if (exec_in.is_valid()) {
                exec_in->unlink_all();
            }

            Ref<OScriptNodePin> is_valid = find_pin("is_valid", PD_Output);
            if (is_valid.is_valid()) {
                is_valid->unlink_all();
            }

            Ref<OScriptNodePin> is_not_valid = find_pin("is_not_valid", PD_Output);
            if (is_not_valid.is_valid()) {
                is_not_valid->unlink_all();
            }
        }

        // Record the connection before the change
        Vector<Ref<OScriptNodePin>> connections;
        Ref<OScriptNodePin> value = find_pin("value", PD_Output);
        if (value.is_valid() && value->has_any_connections()) {
            for (const Ref<OScriptNodePin>& target : value->get_connections()) {
                connections.push_back(target);
            }
            value->unlink_all();
        }

        _notify_pins_changed();

        if (!connections.is_empty()) {
            // Relink connection on change
            value = find_pin("value", PD_Output);
            if (value.is_valid()) {
                for (const Ref<OScriptNodePin>& target : connections)
                    value->link(target);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OScriptNodeVariableSet
///

void OScriptNodeVariableSet::_upgrade(uint32_t p_version, uint32_t p_current_version) {
    if (p_version == 1 && p_current_version >= 2) {
        // Fixup - makes sure that stored property matches variable, if not reconstructs
        if (_variable.is_valid()) {
            const Ref<OScriptNodePin> input = find_pin(_variable->get_variable_name(), PD_Input);
            if (input.is_valid() && !PropertyUtils::are_equal(_variable->get_info(), input->get_property_info())) {
                reconstruct_node();
            }
        }
    }

    super::_upgrade(p_version, p_current_version);
}

void OScriptNodeVariableSet::_variable_changed() {
    if (_is_in_editor()) {
        Ref<OScriptNodePin> input = find_pin(1, PD_Input);
        if (input.is_valid() && input->has_any_connections()) {
            Ref<OScriptNodePin> source = input->get_connections()[0];
            if (!input->can_accept(source)) {
                input->unlink_all();
            }
        }

        Ref<OScriptNodePin> output = find_pin("value", PD_Output);
        if (output.is_valid() && output->has_any_connections()) {
            Ref<OScriptNodePin> target = output->get_connections()[0];
            if (!target->can_accept(output)) {
                output->unlink_all();
            }
        }
    }

    super::_variable_changed();
}

void OScriptNodeVariableSet::allocate_default_pins() {
    create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"));
    create_pin(PD_Input, PT_Data, _variable->get_info())->no_pretty_format();

    create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("ExecOut"));
    create_pin(PD_Output, PT_Data, PropertyUtils::as("value", _variable->get_info()))->hide_label();

    super::allocate_default_pins();
}

String OScriptNodeVariableSet::get_tooltip_text() const {
    if (_variable.is_valid()) {
        String tooltip_text = vformat("Set the value of variable %s", _variable->get_variable_name());
        if (!_variable->get_description().is_empty()) {
            tooltip_text += "\n\nDescription:\n" + _variable->get_description();
        }
        return tooltip_text;
    }

    return vformat("Set the value of a variable");
}

String OScriptNodeVariableSet::get_node_title() const {
    return vformat("Set %s", _variable_name);
}

void OScriptNodeVariableSet::reallocate_pins_during_reconstruction(const Vector<Ref<OScriptNodePin>>& p_old_pins) {
    super::reallocate_pins_during_reconstruction(p_old_pins);

    // Keep old default value if one was set that differs from the variable's default value
    for (const Ref<OScriptNodePin>& old_pin : p_old_pins) {
        if (old_pin->is_input() && !old_pin->is_execution()) {
            if (old_pin->get_effective_default_value() != _variable->get_default_value()) {
                Ref<OScriptNodePin> value_pin = find_pin(_variable->get_variable_name(), PD_Input);
                if (value_pin.is_valid() && !value_pin->has_any_connections()) {
                    value_pin->set_default_value(VariantUtils::convert(old_pin->get_effective_default_value(), value_pin->get_type()));
                }
                break;
            }
        }
    }
}

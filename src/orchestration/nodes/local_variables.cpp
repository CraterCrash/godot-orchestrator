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
#include "orchestration/nodes/local_variables.h"

#include "common/macros.h"
#include "common/property_utils.h"
#include "common/variant_utils.h"
#include "orchestration/orchestration.h"

void OScriptNodeLocalVariable::_variable_changed() {
    if (!_variable.is_valid()) {
        return;
    }

    // The name and the type are independent facts: a rename only relabels pins, whereas a type
    // change reallocates them and may sever connections that no longer fit.
    const bool renamed = _variable_name != _variable->get_variable_name();

    const Ref<OScriptNodePin> pin = _get_variable_pin();
    const bool retyped = !pin.is_valid() || !PropertyUtils::are_equal(_variable->get_info(), pin->get_property_info());

    if (renamed) {
        _variable_name = _variable->get_variable_name();
    }

    if (renamed || retyped) {
        reconstruct_node();
    }

    if (retyped) {
        _variable_type_changed();
    }
}

void OScriptNodeLocalVariable::_resolve(const StringName& p_function_name) {
    if (_variable.is_valid() || _variable_name.is_empty()) {
        return;
    }

    Orchestration* orchestration = get_orchestration();
    if (!orchestration) {
        return;
    }

    Ref<OScriptFunction> function;
    if (!p_function_name.is_empty()) {
        function = orchestration->find_function(p_function_name);
    } else {
        // The declaring function is the one whose graph holds this node
        for (const Ref<OScriptGraph>& graph : orchestration->get_graphs()) {
            if (graph->has_node(get_id())) {
                function = orchestration->find_function(graph->get_graph_name());
                break;
            }
        }
    }

    if (!function.is_valid()) {
        return;
    }

    _variable = function->find_local_variable(_variable_name);
    if (_variable.is_valid() && _is_in_editor()) {
        OCONNECT(_variable, "changed", callable_mp_this(_variable_changed));
    }
}

void OScriptNodeLocalVariable::_get_property_list(List<PropertyInfo>* r_list) const {
    r_list->push_back(PropertyInfo(Variant::STRING, "variable_name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
}

bool OScriptNodeLocalVariable::_get(const StringName& p_name, Variant& r_value) const {
    if (p_name == StringName("variable_name")) {
        r_value = _variable_name;
        return true;
    }
    return false;
}

bool OScriptNodeLocalVariable::_set(const StringName& p_name, const Variant& p_value) {
    if (p_name == StringName("variable_name")) {
        _variable_name = p_value;
        return true;
    }
    return false;
}

PropertyInfo OScriptNodeLocalVariable::_get_variable_info() const {
    // An unresolved reference still renders so the parser can report it rather than the node vanishing
    return _variable.is_valid() ? _variable->get_info() : PropertyUtils::make_variant(_variable_name);
}

void OScriptNodeLocalVariable::post_initialize() {
    _resolve();
    super::post_initialize();
}

void OScriptNodeLocalVariable::post_placed_new_node() {
    _resolve();
    super::post_placed_new_node();
}

String OScriptNodeLocalVariable::get_icon() const {
    return "LocalVariable";
}

bool OScriptNodeLocalVariable::is_compatible_with_graph(const Ref<OScriptGraph>& p_graph) const {
    return p_graph->get_flags().has_flag(OScriptGraph::GraphFlags::GF_FUNCTION);
}

void OScriptNodeLocalVariable::initialize(const OScriptNodeInitContext& p_context) {
    ERR_FAIL_COND_MSG(!p_context.variable_name, "Failed to initialize local variable node without a variable name");

    _variable_name = p_context.variable_name.value();

    // A freshly spawned node is not yet in a graph, so the declaring function comes from the context
    _resolve(p_context.function_name.value_or(StringName()));

    super::initialize(p_context);
}

OScriptNodeLocalVariable::OScriptNodeLocalVariable() {
    // Catalog versions are added explicitly
    _flags = NONE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OScriptNodeLocalVariableGet
///

void OScriptNodeLocalVariableGet::_validate_output_connection() {
    Ref<OScriptNodePin> output = find_pin("value", PD_Output);
    if (output.is_valid() && output->has_any_connections()) {
        for (const Ref<OScriptNodePin>& target : output->get_connections()) {
            if (target.is_valid() && !target->can_accept(output)) {
                output->unlink(target);
            }
        }
    }
}

Ref<OScriptNodePin> OScriptNodeLocalVariableGet::_get_variable_pin() const {
    return find_pin("value", PD_Output);
}

void OScriptNodeLocalVariableGet::_variable_type_changed() {
    if (_is_in_editor()) {
        // Defer this so that all nodes have updated
        // This is necessary so that all target types that may have changed (i.e. get connected to set)
        // have updated to make sure that the "can_accept" logic works as expected.
        callable_mp_this(_validate_output_connection).call_deferred();
    }
}

void OScriptNodeLocalVariableGet::allocate_default_pins() {
    create_pin(PD_Output, PT_Data, PropertyUtils::as("value", _get_variable_info()))->set_label(_variable_name, false);
    super::allocate_default_pins();
}

String OScriptNodeLocalVariableGet::get_tooltip_text() const {
    if (_variable_name.is_empty()) {
        return "Read the value of a local variable";
    }

    String tooltip = "Read the value of local variable " + _variable_name;
    if (!_variable.is_valid() || _variable->get_description().strip_edges().is_empty()) {
        return tooltip;
    }

    tooltip += "\n\nDescription:\n" + _variable->get_description().strip_edges();
    return tooltip;
}

String OScriptNodeLocalVariableGet::get_node_title() const {
    return vformat("Get %s", _variable_name);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OScriptNodeLocalVariableSet
///

Ref<OScriptNodePin> OScriptNodeLocalVariableSet::_get_variable_pin() const {
    return find_pin(1, PD_Input);
}

void OScriptNodeLocalVariableSet::_variable_type_changed() {
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
}

void OScriptNodeLocalVariableSet::allocate_default_pins() {
    const PropertyInfo info = _get_variable_info();

    create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"));
    create_pin(PD_Input, PT_Data, PropertyUtils::as(_variable_name, info))->no_pretty_format();

    create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("ExecOut"));
    create_pin(PD_Output, PT_Data, PropertyUtils::as("value", info))->hide_label();

    super::allocate_default_pins();
}

String OScriptNodeLocalVariableSet::get_tooltip_text() const {
    if (_variable_name.is_empty()) {
        return "Set the value of a local variable";
    }

    String tooltip = "Set the value of local variable " + _variable_name;
    if (!_variable.is_valid() || _variable->get_description().strip_edges().is_empty()) {
        return tooltip;
    }

    tooltip += "\n\nDescription:\n" + _variable->get_description().strip_edges();
    return tooltip;
}

String OScriptNodeLocalVariableSet::get_node_title() const {
    return vformat("Set %s", _variable_name);
}

void OScriptNodeLocalVariableSet::reallocate_pins_during_reconstruction(const Vector<Ref<OScriptNodePin>>& p_old_pins) {
    super::reallocate_pins_during_reconstruction(p_old_pins);

    // The value pin is named after the variable, so the base class cannot match the old pin to the
    // new pin by name after its renamed; carry any user-supplied value over positionally instead.
    for (const Ref<OScriptNodePin>& old_pin : p_old_pins) {
        if (!old_pin->is_input() || old_pin->is_execution()) {
            continue;
        }

        const Variant user_value = old_pin->get_default_value();
        if (user_value.get_type() != Variant::NIL) {
            const Ref<OScriptNodePin> value_pin = find_pin(1, PD_Input);
            if (value_pin.is_valid() && !value_pin->has_any_connections()) {
                value_pin->set_default_value(VariantUtils::convert(user_value, value_pin->get_type()));
            }
        }
        break;
    }
}
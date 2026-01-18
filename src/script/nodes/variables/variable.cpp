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
#include "script/nodes/variables/variable.h"

#include "common/macros.h"

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

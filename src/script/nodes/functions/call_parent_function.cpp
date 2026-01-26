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
#include "script/nodes/functions/call_parent_function.h"

Ref<OScriptNodePin> OScriptNodeCallParentMemberFunction::_create_target_pin() {
    // Calling parent is not chainable and target is implied as self
    _chainable = false;
    return nullptr;
}

String OScriptNodeCallParentMemberFunction::get_tooltip_text() const {
    if (!_reference.method.name.is_empty()) {
        return vformat("Calls the parent function '%s'", _reference.method.name);
    }
    return "Calls the specified parent function";
}

String OScriptNodeCallParentMemberFunction::get_node_title() const {
    return vformat("Parent: %s", super::get_node_title());
}

OScriptNodeCallParentMemberFunction::OScriptNodeCallParentMemberFunction() {
    _function_flags.set_flag(FF_SUPER);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// OScriptNodeCallParentScriptFunction

String OScriptNodeCallParentScriptFunction::get_tooltip_text() const {
    if (!_reference.method.name.is_empty()) {
        return vformat("Calls the parent script function '%s'", _reference.method.name);
    }
    return "Calls the specified parent script function";
}

String OScriptNodeCallParentScriptFunction::get_node_title() const {
    return vformat("Parent: %s", super::get_node_title());
}

void OScriptNodeCallParentScriptFunction::initialize(const OScriptNodeInitContext& p_context) {
    super::initialize(p_context);

    // When callling super, we never want to imply self in this context
    if (get_function().is_valid() && _function_flags.has_flag(FF_IS_SELF)) {
        _function_flags.clear_flag(FF_IS_SELF);
    }
}

OScriptNodeCallParentScriptFunction::OScriptNodeCallParentScriptFunction() {
    _function_flags.set_flag(FF_SUPER);
}
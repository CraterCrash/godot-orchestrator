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
#ifndef ORCHESTRATOR_SCRIPT_NODE_CALL_PARENT_FUNCTION_H
#define ORCHESTRATOR_SCRIPT_NODE_CALL_PARENT_FUNCTION_H

#include "script/nodes/functions/call_member_function.h"
#include "script/nodes/functions/call_script_function.h"

/// A node that delegates control flow to a parent member function
class OScriptNodeCallParentMemberFunction : public OScriptNodeCallMemberFunction {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeCallParentMemberFunction, OScriptNodeCallMemberFunction);

protected:
    static void _bind_methods() { }

    //~ Begin OScripNodeCallFunction Interface
    Ref<OScriptNodePin> _create_target_pin() override;
    //~ End OScriptNodeCallFunction Interface

public:
    //~ Begin OScriptNode Interface
    String get_tooltip_text() const override;
    String get_node_title() const override;
    bool is_override() const override { return false; }
    //~ End OScriptNode Interface

    OScriptNodeCallParentMemberFunction();
};

class OScriptNodeCallParentScriptFunction : public OScriptNodeCallScriptFunction {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeCallParentScriptFunction, OScriptNodeCallScriptFunction);

protected:
    static void _bind_methods() { }

public:
    //~ Begin OScriptNode Interface
    String get_tooltip_text() const override;
    String get_node_title() const override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    bool is_override() const override { return false; }
    //~ End OScriptNode Interface

    OScriptNodeCallParentScriptFunction();
};

#endif // ORCHESTRATOR_SCRIPT_NODE_CALL_PARENT_FUNCTION_H
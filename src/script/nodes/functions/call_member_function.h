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
#ifndef ORCHESTRATOR_SCRIPT_NODE_CALL_MEMBER_FUNCTION_H
#define ORCHESTRATOR_SCRIPT_NODE_CALL_MEMBER_FUNCTION_H

#include "call_function.h"

/// An implementation of OrchestratorScript CallFunction node that calls a method
/// on a Godot object.
class OScriptNodeCallMemberFunction : public OScriptNodeCallFunction
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeCallMemberFunction, OScriptNodeCallFunction);

    static void _bind_methods() {}

    //~ Begin OScriptNode Interface
    void _upgrade(uint32_t p_version, uint32_t p_current_version) override;
    //~ End OScriptNode Interface

    //~ Begin OScripNodeCallFunction Interface
    Ref<OScriptNodePin> _create_target_pin() override;
    int get_argument_offset() const override { return 1; }
    //~ End OScriptNodeCallFunction Interface

    /// Gets the class in the hierarchy that owns the method
    /// @param p_class_name eldest class in hierarchy to search
    /// @param p_method_name the method name to look for
    /// @return the class name that owns the method or an empty string if not found
    StringName _get_method_class_hierarchy_owner(const String& p_class_name, const String& p_method_name);

public:
    OScriptNodeCallMemberFunction();

    //~ Begin OScriptNode Interface
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override;
    String get_help_topic() const override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    void validate_node_during_build(BuildLog& p_log) const override;
    //~ End OScriptNode Interface

    /// Get the target function class
    /// @return the target function class
    String get_target_class() const { return _reference.target_class_name; }

    /// Get the Godot function reference
    /// @return the function reference
    const MethodInfo& get_function() const { return _reference.method; }

};

#endif  // ORCHESTRATOR_SCRIPT_NODE_CALL_MEMBER_FUNCTION_H

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
#ifndef ORCHESTRATOR_SCRIPT_NODE_VARIABLE_H
#define ORCHESTRATOR_SCRIPT_NODE_VARIABLE_H

#include "script/script.h"

/// An abstract script node for all variable types (script and local)
class OScriptNodeVariableBase : public OScriptNode
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeVariableBase, OScriptNode);
    static void _bind_methods() { }

protected:
    StringName _variable_name; //! Variable name

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    //~ End Wrapped Interface

    /// Lookup and set the variable
    /// @param p_variable_name the variable name
    virtual void _lookup_and_set_variable(const StringName& p_variable_name) { }

    /// Allow subclasses to update when variable changes
    virtual void _variable_changed() { }

public:
    //~ Begin OScriptNode Interface
    String get_icon() const override { return "MemberProperty"; }
    void initialize(const OScriptNodeInitContext& p_context) override;
    //~ End OScriptNode Interface

    /// Get the variable name this node represents
    String get_variable_name() const { return _variable_name; }

    OScriptNodeVariableBase() = default; // todo: is this needed?
};

/// An abstract script node for script variables.
class OScriptNodeScriptVariableBase : public OScriptNodeVariableBase
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeScriptVariableBase, OScriptNodeVariableBase);
    static void _bind_methods() { }

protected:
    Ref<OScriptVariable> _variable; //! Script variable

    //~ Begin OScriptNodeVariableBase Interface
    void _lookup_and_set_variable(const StringName& p_variable_name) override;
    //~ End OScriptNodeVariableBase Interface

    /// Called when the script variable is modified
    void _on_variable_changed();

public:
    //~ Begin OScriptNode Interface
    void post_initialize() override;
    String get_node_title_color_name() const override { return "variable"; }
    void validate_node_during_build(BuildLog& p_log) const override;
    //~ End OScriptNode Interface

    /// Get the variable this node represents
    Ref<OScriptVariable> get_variable() const { return _variable; }
};

/// An abstract script node for local variables.
class OScriptNodeLocalVariableBase : public OScriptNodeVariableBase
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeLocalVariableBase, OScriptNodeVariableBase);
    static void _bind_methods() { }

protected:
    Guid _function_guid; //! Function guid
    Ref<OScriptLocalVariable> _variable; //! Local variable

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    //~ End Wrapped Interface

    //~ Begin OScriptNodeVariableBase Interface
    void _lookup_and_set_variable(const StringName& p_variable_name) override;
    //~ End OScriptNodeVariableBase Interface

    /// Called when the local variable is modified
    void _on_variable_changed();

public:
    //~ Begin OScriptNode Interface
    void post_initialize() override;
    String get_node_title_color_name() const override { return "local_variable"; }
    void initialize(const OScriptNodeInitContext& p_context) override;
    void validate_node_during_build(BuildLog& p_log) const override;
    //~ End OScriptNode Interface

    Ref<OScriptFunction> get_function() const;

    /// Get the local variable this node represents
    Ref<OScriptLocalVariable> get_variable() const { return _variable; }
};

#endif  // ORCHESTRATOR_SCRIPT_NODE_VARIABLE_H

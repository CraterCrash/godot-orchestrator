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

/// An abstract script node for all variable operations.
class OScriptNodeVariable : public OScriptNode
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeVariable, OScriptNode);
    static void _bind_methods() { }

protected:
    StringName _variable_name;       //! Variable name reference
    Ref<OScriptVariable> _variable;  //! Variable reference

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    //~ End Wrapped Interface

    /// Called when the script variable is modified
    void _on_variable_changed();

    /// Allows subclasses to handle variable changed 
    virtual void _variable_changed() { }

public:
    OScriptNodeVariable();

    //~ Begin OScriptNode Interface
    void post_initialize() override;
    String get_icon() const override;
    String get_node_title_color_name() const override { return "variable"; }
    Ref<Resource> get_inspect_object() override { return _variable; }
    void initialize(const OScriptNodeInitContext& p_context) override;
    void validate_node_during_build(BuildLog& p_log) const override;
    //~ End OScriptNode Interface

    Ref<OScriptVariable> get_variable() { return _variable; }
};

#endif  // ORCHESTRATOR_SCRIPT_NODE_VARIABLE_H

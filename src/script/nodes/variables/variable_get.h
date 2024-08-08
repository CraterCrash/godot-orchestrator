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
#ifndef ORCHESTRATOR_SCRIPT_NODE_VARIABLE_GET_H
#define ORCHESTRATOR_SCRIPT_NODE_VARIABLE_GET_H

#include "variable.h"

/// A variable implementation that gets the value of a variable.
class OScriptNodeVariableGet : public OScriptNodeVariable
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeVariableGet, OScriptNodeVariable);
    static void _bind_methods() { }

protected:
    bool _validated{ false };  //! Whether to represent get as validated get

    // //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    // //~ End Wrapped Interface

    //~ Begin OScriptNode Interface
    void _upgrade(uint32_t p_version, uint32_t p_current_version) override;
    //~ End OScriptNode Interface

    //~ Begin OScriptNodeVariable Interface
    void _variable_changed() override;
    //~ End OScriptNodeVariable Interface

public:
    //~ Begin OScriptNode Interface
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    bool should_draw_as_bead() const override { return true; }
    OScriptNodeInstance* instantiate() override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    //~ End OScriptNode Interface

    /// Return whether the node can be validated
    /// @return true if the node can be validated, false otherwise
    bool can_be_validated();

    /// Return whether the variable is validated
    /// @return true if the node is rendered as a validated node, false otherwise.
    bool is_validated() const { return _validated; }

    /// Change whether the node is rendered as a validated get
    /// @param p_validated when true, rendered as validated get
    void set_validated(bool p_validated);
};

#endif  // ORCHESTRATOR_SCRIPT_NODE_VARIABLE_GET_H

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
#pragma once

#include "orchestration/local_variable.h"
#include "orchestration/node.h"

/// An abstract script node for all function-scoped local variable operations.
///
/// The node persists only the local variable's name. The declaring function is the function whose
/// graph contains the node, so the reference is resolved from the owning graph rather than stored.
///
class OScriptNodeLocalVariable : public OScriptNode {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeLocalVariable, OScriptNode);

    void _variable_changed();
    void _resolve(const StringName& p_function_name = StringName());

protected:
    static void _bind_methods() { }

    StringName _variable_name;            //! Local variable name reference
    Ref<OScriptLocalVariable> _variable;  //! Resolved local variable reference

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    //~ End Wrapped Interface

    // The node factory instantiates every registered class, so these cannot be pure virtual
    virtual Ref<OScriptNodePin> _get_variable_pin() const { return {}; }
    virtual void _variable_type_changed() { }

    PropertyInfo _get_variable_info() const;

public:
    //~ Begin OScriptNode Interface
    void post_initialize() override;
    void post_placed_new_node() override;
    String get_icon() const override;
    String get_node_title_color_name() const override { return "variable"; }
    Ref<Resource> get_inspect_object() override { return _variable; }
    bool is_compatible_with_graph(const Ref<OScriptGraph>& p_graph) const override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    //~ End OScriptNode Interface

    Ref<OScriptLocalVariable> get_variable() const { return _variable; }
    StringName get_variable_name() const { return _variable_name; }

    OScriptNodeLocalVariable();
};

/// A local variable implementation that gets the value of a local variable.
class OScriptNodeLocalVariableGet : public OScriptNodeLocalVariable {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeLocalVariableGet, OScriptNodeLocalVariable);

    void _validate_output_connection();

protected:
    static void _bind_methods() { }

    //~ Begin OScriptNodeLocalVariable Interface
    Ref<OScriptNodePin> _get_variable_pin() const override;
    void _variable_type_changed() override;
    //~ End OScriptNodeLocalVariable Interface

public:
    //~ Begin OScriptNode Interface
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    bool should_draw_as_bead() const override { return true; }
    bool is_pure() const override { return true; }
    //~ End OScriptNode Interface
};

/// A local variable implementation that sets the value of a local variable.
class OScriptNodeLocalVariableSet : public OScriptNodeLocalVariable {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeLocalVariableSet, OScriptNodeLocalVariable);

protected:
    static void _bind_methods() { }

    //~ Begin OScriptNodeLocalVariable Interface
    Ref<OScriptNodePin> _get_variable_pin() const override;
    void _variable_type_changed() override;
    //~ End OScriptNodeLocalVariable Interface

public:
    //~ Begin OScriptNode Interface
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    void reallocate_pins_during_reconstruction(const Vector<Ref<OScriptNodePin>>& p_old_pins) override;
    //~ End OScriptNode Interface
};
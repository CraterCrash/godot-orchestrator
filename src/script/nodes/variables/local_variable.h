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
#ifndef ORCHESTRATOR_SCRIPT_NODE_LOCAL_VARIABLE_H
#define ORCHESTRATOR_SCRIPT_NODE_LOCAL_VARIABLE_H

#include "script/script.h"

class OScriptNodeLocalVariable : public OScriptNode
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeLocalVariable, OScriptNode);
    static void _bind_methods() { }

protected:
    Guid _guid;           //! Uniquely identifies this variable from any other.
    Variant::Type _type;  //! Transient type used to initialize the pin.
    String _description;  //! User-defined description

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    //~ End Wrapped Interface

public:
    //~ Begin OScriptNode Interface
    void post_initialize() override;
    void allocate_default_pins() override;
    String get_node_title() const override;
    String get_icon() const override;
    String get_node_title_color_name() const override { return "variable"; }
    String get_tooltip_text() const override;
    bool is_compatible_with_graph(const Ref<OScriptGraph>& p_graph) const override;
    bool can_duplicate() const override { return false; }
    OScriptNodeInstance* instantiate() override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    //~ End OScriptNode Interface

    /// Get the associated variable GUID
    String get_variable_guid() const { return _guid.to_string(); }
};

class OScriptNodeAssignLocalVariable : public OScriptNode
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeAssignLocalVariable, OScriptNode);
    static void _bind_methods() { }

protected:
    Variant::Type _type{ Variant::NIL }; //! Transient type used to identify pin type

    //~ Begin OScriptNode Interface
    void _upgrade(uint32_t p_version, uint32_t p_current_version) override;
    //~ End OScriptNode Interface

public:
    //~ Begin OScriptNode Interface
    void post_initialize() override;
    void post_placed_new_node() override;
    void allocate_default_pins() override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "variable"; }
    String get_tooltip_text() const override;
    bool is_compatible_with_graph(const Ref<OScriptGraph>& p_graph) const override;
    OScriptNodeInstance* instantiate() override;
    void validate_node_during_build(BuildLog& p_log) const override;
    void on_pin_connected(const Ref<OScriptNodePin>& p_pin) override;
    void on_pin_disconnected(const Ref<OScriptNodePin>& p_pin) override;
    //~ End OScriptNode Interface

    /// Get the associated variable GUID
    String get_variable_guid() const;
};

#endif  // ORCHESTRATOR_SCRIPT_NODE_LOCAL_VARIABLE_H
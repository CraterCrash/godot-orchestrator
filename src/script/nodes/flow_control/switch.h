// This file is part of the Godot Orchestrator project.
//
// Copyright (c) 2023-present Vahera Studios LLC and its contributors.
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
#ifndef ORCHESTRATOR_SCRIPT_NODE_SWITCH_H
#define ORCHESTRATOR_SCRIPT_NODE_SWITCH_H

#include "script/nodes/editable_pin_node.h"

/// A simple switch statement that takes an input, compares against a set of
/// cases and determines which of the output paths to take before exiting the node.
class OScriptNodeSwitch : public OScriptEditablePinNode
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeSwitch, OScriptEditablePinNode);
    static void _bind_methods() { }

protected:
    int _cases{ 0 };     //! Transient case count

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* p_list) const;
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    //~ End Wrapped Interface

    void _unlink_pins(int p_new_cases);

public:
    //~ Begin OScriptNode Interface
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "flow_control"; }
    String get_icon() const override;
    OScriptNodeInstance* instantiate() override;
    //~ End OScriptNode Interface

    //~ Begin OScriptEditablePinNode Interface
    bool can_add_dynamic_pin() const override;
    bool can_remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin) const override;
    void add_dynamic_pin() override;
    void remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin) override;
    String get_pin_prefix() const override { return "case"; }
    //~ End OScriptEditablePinNode Interface
};

class OScriptNodeSwitchEditablePin : public OScriptEditablePinNode
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeSwitchEditablePin, OScriptEditablePinNode);
    static void _bind_methods() { }

protected:
    PackedStringArray _pin_names;
    bool _case_sensitive{ false };
    bool _has_default_value{ true };

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* p_list) const;
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    //~ End Wrapped Interface

    virtual bool _supports_case_sensitive_pins() const { return false; }
    virtual bool _can_pin_names_be_edited() const { return true; }
    virtual void _recompute_pin_names(int p_index);
    virtual Variant::Type _get_input_pin_type() const { return Variant::NIL; }
    virtual String _get_new_pin_name() { return {}; }

public:
    //~ Begin OScriptNode Interface
    void allocate_default_pins() override;
    String get_node_title_color_name() const override { return "flow_control"; }
    String get_icon() const override { return "ClassList"; }
    String get_tooltip_text() const override;
    //~ End OScriptNode Interface

    //~ Begin OScriptEditablePinNode Interface
    bool can_add_dynamic_pin() const override;
    bool can_remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin) const override;
    void add_dynamic_pin() override;
    void remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin) override;
    //~ End OScriptEditablePinNode Interface
};

/// A switch statement that takes an input string value and compares it against
/// one of the output pins, exiting on the pin that matches or the default pin.
class OScriptNodeSwitchString : public OScriptNodeSwitchEditablePin
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeSwitchString, OScriptNodeSwitchEditablePin);
    static void _bind_methods() { }

protected:
    //~ Begin OScriptNodeSwitchEditablePin Interface
    bool _supports_case_sensitive_pins() const override { return true; }
    Variant::Type _get_input_pin_type() const override { return Variant::STRING; }
    String _get_new_pin_name() override;
    //~ End OScriptNodeSwitchEditablePin Interface

public:
    //~ Begin OScriptNode Interface
    String get_node_title() const override { return "Switch on String"; }
    OScriptNodeInstance* instantiate() override;
    //~ End OScriptNode Interface
};

/// A switch statement that takes an input numeric value and compares it against
/// one of the output pins, exiting on the pin that matches or the default pin.
class OScriptNodeSwitchInteger : public OScriptNodeSwitchEditablePin
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeSwitchInteger, OScriptNodeSwitchEditablePin);
    static void _bind_methods() { }

protected:
    int _start_index{ 0 };

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* p_list) const;
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    //~ End Wrapped Interface

    //~ Begin OScriptNodeSwitchEditablePin Interface
    bool _supports_case_sensitive_pins() const override { return false; }
    bool _can_pin_names_be_edited() const override { return false; }
    void _recompute_pin_names(int p_index) override;
    Variant::Type _get_input_pin_type() const override { return Variant::INT; }
    String _get_new_pin_name() override;
    //~ End OScriptNodeSwitchEditablePin Interface

public:
    //~ Begin OScriptNode Interface
    String get_node_title() const override { return "Switch on Integer"; }
    OScriptNodeInstance* instantiate() override;
    //~ End OScriptNode Interface
};

/// A switch statement that takes an input enum, and compares it against all the
/// possible values, exiting on the pin that matches the enum value.
class OScriptNodeSwitchEnum : public OScriptNode
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeSwitchEnum, OScriptNode);
    static void _bind_methods() { }

protected:
    String _enum_name; //! Transient enum name

public:
    //~ Begin OScriptNode Interface
    void post_initialize() override;
    void post_placed_new_node() override;
    void allocate_default_pins() override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "flow_control"; }
    String get_icon() const override { return "ClassList"; }
    String get_tooltip_text() const override;
    OScriptNodeInstance* instantiate() override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    //~ End OScriptNode Interface
};

#endif  // ORCHESTRATOR_SCRIPT_NODE_SWITCH_H

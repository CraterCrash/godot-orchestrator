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

#include "api/extension_db.h"
#include "orchestration/node.h"
#include "orchestration/nodes/editable_pin_node.h"

/// A node that accepts a set of inputs and performs an operation.
class OScriptNodeOperator : public OScriptNode {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeOperator, OScriptNode);

    OperatorInfo _info;

protected:
    static void _bind_methods() { }

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    //~ End Wrapped Interface

    String _get_expression() const;
    bool _is_unary() const;

    bool _should_expand_instead_compile() const;

public:
    //~ Begin OScriptNode Interface
    void post_initialize() override;
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "math_operations"; }
    String get_icon() const override { return "Translation"; }
    void initialize(const OScriptNodeInitContext& p_context) override;
    bool is_pure() const override;
    //~ End OScriptNode Interface

    const OperatorInfo& get_info() { return _info; }

    /// Returns whether the type is supported.
    /// @param p_type the type
    /// @return true whether a type is supported by this node, false if it is not.
    static bool is_supported(Variant::Type p_type);

    /// Returns whether the operator is supported.
    /// @param p_operator the operator
    /// @return true whether an operator is supported by this node, false if it is not.
    static bool is_operator_supported(const OperatorInfo& p_operator);
};

/// A new operator node that allows for its type pins to be promoted or changed.
///
/// This node is guarded by an option 'Enable Type Promotion' in the settings.
/// When the option is disabled, the editor shows the legacy OScriptNodeOperator options.
/// When the option is enabled, the editor shows the consolidated utility OScriptNodePromotableOperator options.
///
/// These nodes are only available in format version 4+.
///
class OScriptNodePromotableOperator : public OScriptEditablePinNode {
    ORCHESTRATOR_NODE_CLASS(OScriptNodePromotableOperator, OScriptEditablePinNode);

    VariantOperators::Code _op = VariantOperators::OP_EQUAL;
    Vector<Variant::Type> _operands;
    Variant::Type _result = Variant::NIL;

protected:
    static void _bind_methods();

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    //~ End Wrapped Interface

    Variant::Type _get_result_type(Variant::Type p_left, Variant::Type p_right) const;
    Variant::Type _get_result_type() const;

public:
    //~ Begin OScriptNode Interface
    void allocate_default_pins() override;
    bool rewire_old_pins_by_position() const override { return true; }
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "math_operations"; }
    String get_icon() const override { return "Translation"; }
    String get_help_topic() const override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    bool is_pure() const override;
    bool can_change_pin_type(const Ref<OScriptNodePin>& p_pin) const override;
    Vector<Variant::Type> get_possible_pin_types(const Ref<OScriptNodePin>& p_pin) const override;
    void change_pin_types(const Ref<OScriptNodePin>& p_pin, Variant::Type p_type) override;
    PackedStringArray get_keywords() const override;
    void get_actions(List<Ref<OScriptAction>>& p_action_list) override;
    void post_reconstruct_node() override;
    //~ End OScriptNode Interface

    //~ Begin OScriptEditablePinNode Interface
    bool can_add_dynamic_pin() const override;
    void add_dynamic_pin() override;
    bool can_remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin) const override;
    void remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin) override;
    //~ End OScriptEditablePinNode Interface

    bool is_unary() const;
    bool is_string_format_using_modulo() const;
    bool is_in_object() const;
    VariantOperators::Code get_operator() const { return _op; }

    static bool is_supported(Variant::Type p_type);
    static bool is_operator_supported(const OperatorInfo& p_operator);

    static String get_operator_name(VariantOperators::Code p_operator);
    static String get_operator_lexical_code(VariantOperators::Code p_operator);
    static String get_operator_tooltip_text(VariantOperators::Code p_operator);

};
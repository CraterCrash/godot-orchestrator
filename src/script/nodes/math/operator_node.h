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
#ifndef ORCHESTRATOR_SCRIPT_OPERATOR_NODE_H
#define ORCHESTRATOR_SCRIPT_OPERATOR_NODE_H

#include "script/script.h"
#include "api/extension_db.h"

/// A node that accepts a set of inputs and performs an operation.
class OScriptNodeOperator : public OScriptNode
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeOperator, OScriptNode);
    static void _bind_methods();

protected:
    OperatorInfo _info; //! Operator information

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    //~ End Wrapped Interface

    String _get_expression() const;
    bool _is_unary() const;

public:
    //~ Begin OScriptNode Interface
    void post_initialize() override;
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "math_operations"; }
    String get_icon() const override { return "Translation"; }
    OScriptNodeInstance* instantiate() override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    void validate_node_during_build(BuildLog& p_log) const override;
    //~ End OScriptNode Interface

    /// Returns whether the type is supported.
    /// @param p_type the type
    /// @return true whether a type is supported by this node, false if it is not.
    static bool is_supported(Variant::Type p_type);

    /// Returns whether the operator is supported.
    /// @param p_operator the operator
    /// @return true whether an operator is supported by this node, false if it is not.
    static bool is_operator_supported(const OperatorInfo& p_operator);
};

#endif  // ORCHESTRATOR_SCRIPT_OPERATOR_NODE_H
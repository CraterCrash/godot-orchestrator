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
#ifndef ORCHESTRATOR_SCRIPT_NODE_DECOMPOSE_H
#define ORCHESTRATOR_SCRIPT_NODE_DECOMPOSE_H

#include "script/script.h"

/// Decompose a variant value into its sub-parts.
///
/// Certain Godot Variant types such as Vector, Color, and Rect have sub-parts that make up
/// the actual variant. When decomposing such variants, the input variant is split into the
/// appropriate number of output components.
///
/// For example, Vector2 is split into two outgoing pins for its X and Y coordinates.
///
/// For other types, such as Rect2, it is split into its responsible size and position and
/// a follow-up decompose node can be used to split those variant types as needed.
///
class OScriptNodeDecompose : public OScriptNode
{
    // todo: this node needs to have its rendering fixed

    ORCHESTRATOR_NODE_CLASS(OScriptNodeDecompose, OScriptNode);
    static void _bind_methods();

    using TypeMap = HashMap<Variant::Type, Array>;

protected:
    Variant::Type _type;              //! Transient type to pass from creation metadata
    static TypeMap _type_components;  //! Various types and respective components

public:
    //~ Begin OScriptNode Interface
    void post_initialize() override;
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "pure_function_call"; }
    String get_icon() const override;
    String get_help_topic() const override;
    OScriptNodeInstance* instantiate() override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    //~ End OScriptNode Interface
};

#endif  // ORCHESTRATOR_SCRIPT_NODE_DECOMPOSE_H

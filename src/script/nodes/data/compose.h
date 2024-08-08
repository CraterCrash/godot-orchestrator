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
#ifndef ORCHESTRATOR_SCRIPT_NODE_COMPOSE_H
#define ORCHESTRATOR_SCRIPT_NODE_COMPOSE_H

#include "script/script.h"

/// Compose a variant value from its sub-parts.
///
/// Certain Godot Variant types such as Vector, Color, and Rect have sub-parts that make up
/// the actual variant. When composing such variants, the input variant is split into the
/// appropriate components to make the output variant type.
///
/// For example, Vector2 is split into two incoming pins for its X and Y coordinates.
///
/// For other types, such as Rect2, it is split into its respectable size and position and
/// a preceding compose node can be used to create those struct types.
///
class OScriptNodeCompose : public OScriptNode
{
    // todo: it seems this class is no longer used?
    ORCHESTRATOR_NODE_CLASS(OScriptNodeCompose, OScriptNode);
    static void _bind_methods();

    using TypeMap = HashMap<Variant::Type, Array>;

protected:
    Variant::Type _type;              //! Transient type to pass from creation metadata
    static TypeMap _type_components;  //! Variant types and the respective components

public:
    //~ Begin OScriptNode Interface
    void post_initialize() override;
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "pure_function_call"; }
    String get_icon() const override;
    OScriptNodeInstance* instantiate() override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    //~ End OScriptNode Interface

    /// Returns whether the type is supported.
    /// @param p_type the type
    /// @return true whether a type is supported by this node, false if it is not.
    static bool is_supported(Variant::Type p_type);
};

/// Composes a variant using its constructor signatures
class OScriptNodeComposeFrom : public OScriptNode
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeComposeFrom, OScriptNode);
    static void _bind_methods();

protected:
    Variant::Type _type;                         //! Transient type to pass from creation metadata
    Vector<PropertyInfo> _constructor_args;      //! Transient constructor arguments

public:
    //~ Begin OScriptNode Interface
    void post_initialize() override;
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "pure_function_call"; }
    String get_icon() const override;
    String get_help_topic() const override;
    PackedStringArray get_keywords() const override;
    OScriptNodeInstance* instantiate() override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    //~ End OScriptNode Interface

    /// Returns whether the type is supported.
    /// @param p_type the type
    /// @param p_arguments the constructor argument list
    /// @return true whether a type is supported by this node, false if it is not.
    static bool is_supported(Variant::Type p_type, const Vector<PropertyInfo>& p_arguments);
};

#endif  // ORCHESTRATOR_SCRIPT_NODE_COMPOSE_H
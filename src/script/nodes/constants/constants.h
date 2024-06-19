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
#ifndef ORCHESTRATOR_SCRIPT_NODE_CONSTANTS_H
#define ORCHESTRATOR_SCRIPT_NODE_CONSTANTS_H

#include "script/script.h"

/// Base class for all constant-based script nodes
class OScriptNodeConstant : public OScriptNode
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeConstant, OScriptNode);
    static void _bind_methods() { }

public:
    OScriptNodeConstant();

    //~ Begin OScriptNode Interface
    String get_node_title_color_name() const override { return "constants_and_literals"; }
    //~ End OScriptNode Interface
};

/// Allows specifying a global constant value
class OScriptNodeGlobalConstant : public OScriptNodeConstant
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeGlobalConstant, OScriptNodeConstant);
    static void _bind_methods() { }

protected:
    StringName _constant_name;

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    //~ End Wrapped Interface

    /// Calculates the first, default constant name on placement.
    /// If a constant is already set, it is returned.
    StringName _get_default_constant_name() const;

public:
    //~ Begin OScriptNode Interface
    void post_initialize() override;
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_icon() const override;
    PackedStringArray get_keywords() const override;
    OScriptNodeInstance* instantiate() override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    void validate_node_during_build(BuildLog& p_log) const override;
    //~ End OScriptNodeInterface
};

/// Allows specifying a math-specific constant value
class OScriptNodeMathConstant : public OScriptNodeConstant
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeMathConstant, OScriptNodeConstant);
    static void _bind_methods() { }

protected:
    String _constant_name{ "One" };  //! The math constant

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    //~ End Wrapped Interface

public:
    //~ Begin OScriptNode Interface
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_icon() const override;
    PackedStringArray get_keywords() const override;
    OScriptNodeInstance* instantiate() override;
    void validate_node_during_build(BuildLog& p_log) const override;
    //~ End OScriptNodeInterface
};

/// Allows specifying a type-based constant
class OScriptNodeTypeConstant : public OScriptNodeConstant
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeTypeConstant, OScriptNodeConstant);
    static void _bind_methods();

    static Vector<Variant::Type> _types;
    static HashMap<Variant::Type, HashMap<StringName, Variant>> _type_constants;

protected:
    Variant::Type _type{ Variant::NIL };  //! Constant basic type
    String _constant_name;                //! Constant name

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    //~ End Wrapped Interface

public:
    //~ Begin OScriptNode Interface
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_icon() const override;
    OScriptNodeInstance* instantiate() override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    void validate_node_during_build(BuildLog& p_log) const override;
    //~ End OScriptNodeInterface
};

/// Base class for class-based and singleton-based constants
class OScriptNodeClassConstantBase : public OScriptNodeConstant
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeClassConstantBase, OScriptNodeConstant);
    static void _bind_methods() { }

protected:
    String _class_name;     //! Constant class name
    String _constant_name;  //! Constant name

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    //~ End Wrapped Interface

    /// Return a list of class names that are only selectable within the node's inspector.
    /// If this returns an empty list, the editor's class selector dialog will be used instead.
    /// @return array of class names, may be empty.
    virtual PackedStringArray _get_class_names() const { return {}; }

    /// Return a list of constant choices for the specified class name.
    /// @param p_class_name the class name, should not be empty
    /// @return string array of constant choices, may be empty but ideally should not.
    virtual PackedStringArray _get_class_constant_choices(const String& p_class_name) const { return {}; }

public:
    //~ Begin OScriptNode Interface
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_icon() const override;
    void validate_node_during_build(BuildLog& p_log) const override;
    //~ End OScriptNodeInterface
};

/// Allows specifying a class-based constant
class OScriptNodeClassConstant : public OScriptNodeClassConstantBase
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeClassConstant, OScriptNodeClassConstantBase);
    static void _bind_methods() { }

protected:
    //~ Begin OScriptNodeClassConstantBase Interface
    PackedStringArray _get_class_constant_choices(const String& p_class_name) const override;
    //~ End OScriptNodeClassConstantBase Interface

public:
    OScriptNodeClassConstant();

    //~ Begin OScriptNode Interface
    OScriptNodeInstance* instantiate() override;
    //~ End OScriptNodeInterface
};

/// Allows specifying singleton-based constants.
class OScriptNodeSingletonConstant : public OScriptNodeClassConstantBase
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeSingletonConstant, OScriptNodeClassConstantBase);
    static void _bind_methods() { }

protected:
    PackedStringArray _singletons;    //! Cached list of applicable singletons

    //~ Begin Wrapped Interface
    PackedStringArray _get_class_names() const override;
    PackedStringArray _get_class_constant_choices(const String& p_class_name) const override;
    //~ End Wrapped Interface

    PackedStringArray _get_singletons_with_enum_constants() const;

public:
    OScriptNodeSingletonConstant();

    //~ Begin OScriptNode Interface
    String get_node_title() const override { return "Singleton Class Constant"; }
    OScriptNodeInstance* instantiate() override;
    //~ End OScriptNodeInterface
};
#endif  // ORCHESTRATOR_SCRIPT_NODE_CONSTANTS_H
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
#ifndef ORCHESTRATOR_SCRIPT_NODE_CONSTANTS_H
#define ORCHESTRATOR_SCRIPT_NODE_CONSTANTS_H

#include "script/script.h"

/// Base class for all constant-based script nodes
class OScriptNodeConstant : public OScriptNode {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeConstant, OScriptNode);

protected:
    static void _bind_methods() { }

public:
    //~ Begin OScriptNode Interface
    String get_node_title_color_name() const override { return "constants_and_literals"; }
    bool is_pure() const override { return true; }
    //~ End OScriptNode Interface

    OScriptNodeConstant();
};

/// Allows specifying a global constant value
class OScriptNodeGlobalConstant : public OScriptNodeConstant
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeGlobalConstant, OScriptNodeConstant);

    StringName _constant_name;

protected:
    static void _bind_methods() { }

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    bool _property_can_revert(const StringName& p_name) const;
    bool _property_get_revert(const StringName& p_name, Variant& r_value) const;
    //~ End Wrapped Interface

    //~ Begin OScriptNode Interface
    void _upgrade(uint32_t p_version, uint32_t p_current_version) override;
    //~ End OScriptNode Interface

    /// Calculates the first, default constant name on placement.
    /// If a constant is already set, it is returned.
    StringName _get_default_constant_name() const;

public:
    //~ Begin OScriptNode Interface
    void post_initialize() override;
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_help_topic() const override;
    String get_icon() const override;
    PackedStringArray get_keywords() const override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    //~ End OScriptNodeInterface

    String get_constant_name() const { return _constant_name; }
};

/// Allows specifying a math-specific constant value
class OScriptNodeMathConstant : public OScriptNodeConstant {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeMathConstant, OScriptNodeConstant);

    String _constant_name = "One";  //! The math constant

protected:
    static void _bind_methods() { }

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
    String get_help_topic() const override;
    String get_icon() const override;
    PackedStringArray get_keywords() const override;
    //~ End OScriptNodeInterface

    String get_constant_name() const { return _constant_name; }
};

/// Allows specifying a type-based constant
class OScriptNodeTypeConstant : public OScriptNodeConstant {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeTypeConstant, OScriptNodeConstant);

    static Vector<Variant::Type> _types;
    static HashMap<Variant::Type, HashMap<StringName, Variant>> _type_constants;

    Variant::Type _type = Variant::NIL;   //! Constant basic type
    String _constant_name;                //! Constant name

protected:
    static void _bind_methods();

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    bool _property_can_revert(const StringName& p_name) const;
    bool _property_get_revert(const StringName& p_name, Variant& r_value) const;
    //~ End Wrapped Interface

    //~ Begin OScriptNode Interface
    void _upgrade(uint32_t p_version, uint32_t p_current_version) override;
    //~ End OScriptNode Interface

    /// Create a pin's property info structure
    PropertyInfo _create_pin_property_info();

public:
    //~ Begin OScriptNode Interface
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_help_topic() const override;
    String get_icon() const override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    //~ End OScriptNodeInterface

    Variant::Type get_type() const { return _type; }
    String get_constant_name() const { return _constant_name; }
};

/// Base class for class-based and singleton-based constants
class OScriptNodeClassConstantBase : public OScriptNodeConstant {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeClassConstantBase, OScriptNodeConstant);

protected:
    static void _bind_methods() { }

    String _class_name;     //! Constant class name
    String _constant_name;  //! Constant name

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    //~ End Wrapped Interface

    //~ Begin OScriptNode Interface
    void _upgrade(uint32_t p_version, uint32_t p_current_version) override;
    //~ End OScriptNode Interface

    /// Return a list of class names that are only selectable within the node's inspector.
    /// If this returns an empty list, the editor's class selector dialog will be used instead.
    /// @return array of class names, may be empty.
    virtual PackedStringArray _get_class_names() const { return {}; }

    /// Return a list of constant choices for the specified class name.
    /// @param p_class_name the class name, should not be empty
    /// @return string array of constant choices, may be empty but ideally should not.
    virtual PackedStringArray _get_class_constant_choices(const String& p_class_name) const { return {}; }

    /// Creates the constant pin
    /// @return the constant pin reference
    Ref<OScriptNodePin> _create_constant_pin();

public:
    //~ Begin OScriptNode Interface
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_help_topic() const override;
    String get_icon() const override;
    //~ End OScriptNodeInterface

    String get_constant_class_name() const { return _class_name; }
    String get_constant_name() const { return _constant_name; }
};

/// Allows specifying a class-based constant
class OScriptNodeClassConstant : public OScriptNodeClassConstantBase {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeClassConstant, OScriptNodeClassConstantBase);

protected:
    static void _bind_methods() { }

    //~ Begin OScriptNodeClassConstantBase Interface
    PackedStringArray _get_class_constant_choices(const String& p_class_name) const override;
    //~ End OScriptNodeClassConstantBase Interface

public:
    OScriptNodeClassConstant();
};

/// Allows specifying singleton-based constants.
class OScriptNodeSingletonConstant : public OScriptNodeClassConstantBase {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeSingletonConstant, OScriptNodeClassConstantBase);

    PackedStringArray _singletons;    //! Cached list of applicable singletons

protected:
    static void _bind_methods() { }

    //~ Begin Wrapped Interface
    PackedStringArray _get_class_names() const override;
    PackedStringArray _get_class_constant_choices(const String& p_class_name) const override;
    //~ End Wrapped Interface

    PackedStringArray _get_singletons_with_enum_constants() const;

public:
    //~ Begin OScriptNode Interface
    String get_node_title() const override { return "Singleton Class Constant"; }
    //~ End OScriptNodeInterface

    OScriptNodeSingletonConstant();
};

#endif  // ORCHESTRATOR_SCRIPT_NODE_CONSTANTS_H
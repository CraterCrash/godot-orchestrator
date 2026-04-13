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

#include "orchestration/nodes/editable_pin_node.h"
#include "orchestration/function.h"

struct OScriptFunctionReference {
    Guid guid;                                  //! The function's GUID, only applicable for script functions
    MethodInfo method;                          //! The godot method reference
    Variant::Type target_type = Variant::NIL;   //! The target type
    String target_class_name;                   //! The target class name
};

/// Represents a call to a function.
///
class OScriptNodeCallFunction : public OScriptEditablePinNode {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeCallFunction, OScriptEditablePinNode);

public:
    // clang-format off
    enum FunctionFlags
    {
        FF_NONE         = 0,        //! No flags
        FF_PURE         = 1 << 0,   //! Pure function, i.e. Godot built-in function
        FF_CONST        = 1 << 1,   //! Function is marked as const
        FF_IS_BEAD      = 1 << 2,   //! Function should be rendered as a bead
        FF_IS_SELF      = 1 << 3,   //! Function is called on self, script owner
        FF_IS_VIRTUAL   = 1 << 4,   //! Function is marked as virtual
        FF_VARARG       = 1 << 5,   //! Function accepts variable arguments
        FF_STATIC       = 1 << 6,   //! Function is marked as static
        FF_OBJECT_CORE  = 1 << 7,   //! Function is a core Object virtual method, i.e. _notification
        FF_EDITOR       = 1 << 8,   //! Function is an editor method
        FF_TARGET       = 1 << 9,   //! Function has a target object
        FF_SUPER        = 1 << 10,  //! Function call is the super implementation
    };
    // clang-format on

protected:
    static void _bind_methods();

    BitField<FunctionFlags> _function_flags = FF_NONE;   //! Function flags
    OScriptFunctionReference _reference;                 //! Function reference
    int _vararg_count = 0;                               //! Variable argument count
    bool _chain = false;                                 //! If the node should chain function calls
    bool _chainable = false;                             //! Whether the node is chainable

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    //~ End Wrapped Interface

    //~ Begin OScriptNode Interface
    void _upgrade(uint32_t p_version, uint32_t p_current_version) override;
    //~ End OScriptNode Interface

    /// Creates the target pin
    /// @return the target pin, or an invalid reference if no target pin is required
    virtual Ref<OScriptNodePin> _create_target_pin() { return nullptr; }

    /// Creates pins for the specified method.
    /// @param p_method the Godot method object
    virtual void _create_pins_for_method(const MethodInfo& p_method);

    /// Set flags for the function based on the Godot method flags
    /// @param p_method the Godot method object
    virtual void _set_function_flags(const MethodInfo& p_method);

    /// Check whether the Godot method demands execution pins.
    /// @param p_method the Godot method object
    /// @return true if there should be execution pins on the node, false means only data pins.
    virtual bool _has_execution_pins(const MethodInfo& p_method) const;

    /// Check whether the referenced MethodInfo object should be serialized for this node's data.
    /// @return true if the MethodInfo is saved as part of the node's data, false if it isn't
    virtual bool _is_method_info_serialized() const { return true; }

    /// Specifies whether arguments that are class types should be labeled by class names
    /// @return true to use class names, return false to use pin label or name
    virtual bool _use_argument_class_name() const { return false; }

    /// Return whether the return value pin should be labeled
    /// @param p_pin the return value pin
    /// @return true if the pin is labeled, false otherwise
    virtual bool _is_return_value_labeled(const Ref<OScriptNodePin>& p_pin) const;

    /// Get the input data pin offset for where function call arguments start.
    /// @return the function argument offset, defaults to 0
    virtual int get_argument_offset() const { return 0; }

    /// Get the number of input arguments for the function.
    /// @return the number of input arguments, should be 0 or greater
    virtual int get_argument_count() const;

public:
    //~ Begin OScriptNode Interface
    void reallocate_pins_during_reconstruction(const Vector<Ref<OScriptNodePin>>& p_old_pins) override;
    void post_initialize() override;
    void allocate_default_pins() override;
    String get_icon() const override { return "MemberMethod"; }
    void initialize(const OScriptNodeInitContext& p_context) override;
    bool is_pure() const override;
    //~ End OScriptNode Interface

    //~ Begin OScriptEditablePinNode Interface
    String get_pin_prefix() const override { return "arg"; }
    bool can_add_dynamic_pin() const override { return _reference.method.flags & METHOD_FLAG_VARARG; }
    bool can_remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin) const override;
    void add_dynamic_pin() override;
    void remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin) override;
    //~ End OScriptEditablePinNode Interface

    bool is_chained() const { return _chain; }

    /// Get the Godot method object.
    /// @return the available Godot MethodInfo
    virtual MethodInfo get_method_info() { return _reference.method; }

    /// Check whether this call function overrides a parent function
    /// @return true if this is an overridden function, false otherwise
    virtual bool is_override() const { return false; }

    OScriptNodeCallFunction() { _flags = NONE; }
};

VARIANT_ENUM_CAST(OScriptNodeCallFunction::FunctionFlags)

/// An implementation of OrchestratorScript CallFunction node that calls a method
/// on a Godot object.
class OScriptNodeCallMemberFunction : public OScriptNodeCallFunction {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeCallMemberFunction, OScriptNodeCallFunction);

    /// Gets the class in the hierarchy that owns the method
    /// @param p_class_name eldest class in hierarchy to search
    /// @param p_method_name the method name to look for
    /// @return the class name that owns the method or an empty string if not found
    StringName _get_method_class_hierarchy_owner(const String& p_class_name, const String& p_method_name);

protected:
    static void _bind_methods() {}

    //~ Begin OScriptNode Interface
    void _upgrade(uint32_t p_version, uint32_t p_current_version) override;
    //~ End OScriptNode Interface

    //~ Begin OScripNodeCallFunction Interface
    Ref<OScriptNodePin> _create_target_pin() override;
    int get_argument_offset() const override { return 1; }
    //~ End OScriptNodeCallFunction Interface

public:
    //~ Begin OScriptNode Interface
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override;
    String get_help_topic() const override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    PackedStringArray get_suggestions(const Ref<OScriptNodePin>& p_pin) override;
    //~ End OScriptNode Interface

    //~ Begin OScriptNodeCallFunction Interface
    bool is_override() const override;
    //~ End OScriptNodeCallFunction Interface

    /// Get the target function class
    /// @return the target function class
    String get_target_class() const { return _reference.target_class_name; }

    /// Get the Godot function reference
    /// @return the function reference
    const MethodInfo& get_function() const { return _reference.method; }

    OScriptNodeCallMemberFunction();
};

/// A node that delegates control flow to a parent member function
class OScriptNodeCallParentMemberFunction : public OScriptNodeCallMemberFunction {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeCallParentMemberFunction, OScriptNodeCallMemberFunction);

protected:
    static void _bind_methods() { }

    //~ Begin OScripNodeCallFunction Interface
    Ref<OScriptNodePin> _create_target_pin() override;
    //~ End OScriptNodeCallFunction Interface

public:
    //~ Begin OScriptNode Interface
    String get_tooltip_text() const override;
    String get_node_title() const override;
    bool is_override() const override { return false; }
    //~ End OScriptNode Interface

    OScriptNodeCallParentMemberFunction();
};

/// An implementation of OrchestratorScript CallFunction node that calls functions
/// that are defined as a part of an Orchestration script.
class OScriptNodeCallScriptFunction : public OScriptNodeCallFunction {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeCallScriptFunction, OScriptNodeCallFunction);

    Ref<OScriptFunction> _function;

protected:
    static void _bind_methods() {}

    /// Called when the script function is modified
    void _on_function_changed();

    //~ Begin OScriptNodeCallFunction Interface
    bool _is_method_info_serialized() const override { return false; }
    bool _use_argument_class_name() const override { return false; }
    int get_argument_count() const override;
    //~ End OScriptNodeCallFunction Interface

public:

    //~ Begin OScriptNode Interface
    void post_initialize() override;
    void post_placed_new_node() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "orchestration_function_call"; }
    Object* get_jump_target_for_double_click() const override;
    bool can_jump_to_definition() const override;
    bool can_inspect_node_properties() const override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    //~ End OScriptNode Interface

    //~ Begin OScriptNodeCallFunction Interface
    MethodInfo get_method_info() override;
    bool is_override() const override;
    //~ End OScriptNodeCallFunction Interface

    /// Get the function reference
    /// @return the script function reference the node calls
    Ref<OScriptFunction> get_function() { return _function; }
};

class OScriptNodeCallParentScriptFunction : public OScriptNodeCallScriptFunction {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeCallParentScriptFunction, OScriptNodeCallScriptFunction);

protected:
    static void _bind_methods() { }

public:
    //~ Begin OScriptNode Interface
    String get_tooltip_text() const override;
    String get_node_title() const override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    bool is_override() const override { return false; }
    //~ End OScriptNode Interface

    OScriptNodeCallParentScriptFunction();
};

/// Supports calling a built-in Godot function
class OScriptNodeCallBuiltinFunction : public OScriptNodeCallFunction {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeCallBuiltinFunction, OScriptNodeCallFunction);

protected:
    static void _bind_methods() {}

    //~ Begin OScriptNodeCallFunction Interface
    bool _has_execution_pins(const MethodInfo& p_method) const override;
    //~ End OScriptNodeCallFunction Interface

public:
    //~ Begin OScriptNode Interface
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "pure_function_call"; }
    String get_help_topic() const override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    bool is_pure() const override { return true; }
    //~ End OScriptNode Interface

    OScriptNodeCallBuiltinFunction();
};

class OScriptNodeCallStaticFunction : public OScriptNode {
    ORCHESTRATOR_NODE_CLASS(OScriptNodeCallStaticFunction, OScriptNode);

    StringName _class_name;
    StringName _method_name;
    MethodInfo _method;

protected:
    static void _bind_methods() { }

    //~ Begin Wrapped Interface
    void _upgrade(uint32_t p_version, uint32_t p_current_version) override;
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    //~ End Wrapped Interface

public:
    //~ Begin OScriptNode Interface
    void post_initialize() override;
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "function_call"; }
    String get_help_topic() const override;
    String get_icon() const override { return "MemberMethod"; }
    void initialize(const OScriptNodeInitContext& p_context) override;
    //~ End OScriptNode Interface

    StringName get_target_class_name() const { return _class_name; }
    StringName get_target_method_name() const { return _method_name; }
    MethodInfo get_target_method() const { return _method; }
};
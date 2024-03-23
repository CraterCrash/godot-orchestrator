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
#ifndef ORCHESTRATOR_SCRIPT_NODE_CALL_FUNCTION_H
#define ORCHESTRATOR_SCRIPT_NODE_CALL_FUNCTION_H

#include "script/script.h"

struct OScriptFunctionReference
{
    //! The object the function is called on.
    Object* object{ nullptr };

    //! The object class type;
    Variant::Type target_type{ Variant::NIL };

    //! The object class name
    String target_class_name;

    //! The function name to be called.
    String name;

    //! The function guid, only applicable for OScriptFunction
    Guid guid;

    //! The function return type
    Variant::Type return_type{ Variant::NIL };

    //! Godot method reference
    MethodInfo method;
};

/// Represents a call to a function.
///
class OScriptNodeCallFunction : public OScriptNode
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeCallFunction, OScriptNode);

    static void _bind_methods();

public:
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
        FF_EDITOR       = 1 << 8    //! Function is an editor method
    };

protected:
    BitField<FunctionFlags> _function_flags{ FF_NONE };  //! Function flags
    OScriptFunctionReference _reference;                 //! Function reference
    int _vararg_count{ 0 };                              //! Variable argument count

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    //~ End Wrapped Interface

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

    /// Check whether the Godot method has a return value.
    /// @param p_method the Godot method object
    /// @return true if the method has a return value, false otherwise
    virtual bool _has_return_value(const MethodInfo& p_method) const;

    /// Check whether the referenced MethodInfo object should be serialized for this node's data.
    /// @return true if the MethodInfo is saved as part of the node's data, false if it isn't
    virtual bool _is_method_info_serialized() const { return true; }

    /// Get the Godot method object.
    /// @return the available Godot MethodInfo
    virtual MethodInfo get_method_info() { return _reference.method; }

    /// Get the input data pin offset for where function call arguments start.
    /// @return the function argument offset, defaults to 1
    virtual int get_argument_offset() const { return 1; }

    /// Get the number of input arguments for the function.
    /// @return the number of input arguments, should be 0 or greater
    virtual int get_argument_count() const;

public:
    //~ Begin OScriptNode Interface
    void reallocate_pins_during_reconstruction(const Vector<Ref<OScriptNodePin>>& p_old_pins) override;
    void post_initialize() override;
    void allocate_default_pins() override;
    OScriptNodeInstance* instantiate(OScriptInstance* p_instance) override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    //~ End OScriptNode Interface

    OScriptNodeCallFunction() { _flags = ScriptNodeFlags::NONE; }

    /// Returns whether the function call supports variadic arguments
    bool is_vararg() const { return _reference.method.flags & METHOD_FLAG_VARARG; }

    /// Adds a new dynamic pin to the node
    void add_dynamic_pin();

    /// Check whether the specified pin can be removed
    /// @param p_pin the pin to be removed
    bool can_remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin) const;

    /// Removes the variadic argument pin
    /// @param p_pin the pin to be removed
    void remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin);
};

VARIANT_ENUM_CAST(OScriptNodeCallFunction::FunctionFlags)

#endif  // ORCHESTRATOR_SCRIPT_NODE_CALL_FUNCTION_H

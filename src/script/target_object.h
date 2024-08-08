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
#ifndef ORCHESTRATOR_SCRIPT_TARGET_OBJECT_H
#define ORCHESTRATOR_SCRIPT_TARGET_OBJECT_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/resource.hpp>

using namespace godot;

/// Reference counted object that deallocates the target object when destroyed.
class OScriptTargetObject : public Resource
{
    GDCLASS(OScriptTargetObject, Resource);
    static void _bind_methods() { }

    Object* _wrapped{ nullptr };  //! The wrapped target object
    bool _owned{ false };         //! Whether the wrapped object is already owned

    OScriptTargetObject() { }

public:
    /// Creates the wrapped target object
    /// @param p_object the object being wrapped
    /// @param p_owner whether the object is owned by another object
    explicit OScriptTargetObject(Object* p_object, bool p_owner);

    /// Destructor
    ~OScriptTargetObject() override;

    /// Returns whether there is a target object
    /// @return <code>true</code> if there is a target object, <code>false</code> otherwise
    bool has_target() const { return _wrapped != nullptr; }

    /// Get the wrapped object target
    /// @return the target object
    Object* get_target() const { return _wrapped; }

    /// Get the target object class name
    /// @return the class name
    StringName get_target_class() const { return _wrapped->get_class(); }

    /// Get the target object property list
    /// @return the property list
    TypedArray<Dictionary> get_target_property_list() const { return _wrapped->get_property_list(); }

    /// Get the target object method list
    /// @return the method list
    TypedArray<Dictionary> get_target_method_list() const { return _wrapped->get_method_list(); }

    /// Get the target object signal list
    /// @return the signal list
    TypedArray<Dictionary> get_target_signal_list() const { return _wrapped->get_signal_list(); }
};

#endif  // ORCHESTRATOR_SCRIPT_TARGET_OBJECT_H
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
#ifndef ORCHESTRATOR_METHOD_UTILS_H
#define ORCHESTRATOR_METHOD_UTILS_H

#include <godot_cpp/core/method_bind.hpp>

using namespace godot;

namespace MethodUtils {

    /// Checks whether the specified property info for a return method attribute returns a value.
    /// @param p_return_val the return attribute from a <code>MethodInfo</cde>
    /// @return true if a value is returned, false otherwise
    bool has_return_value(const PropertyInfo& p_return_val);

    /// Checks whether the specified Godot <code>MethodInfo</code> has a return value.
    /// @param p_method the Godot method info structure
    /// @return <code>true</code> if the method returns a value; <code>false</code> otherwise
    bool has_return_value(const MethodInfo& p_method);

    /// Sets the method to not return to value.
    /// @param p_method the method to modify
    void set_no_return_value(MethodInfo& p_method);

    /// Sets the method to return a value
    /// @param p_method the method to modify
    void set_return_value(MethodInfo& p_method);

    /// Sets the method to return the specified type
    /// @param p_method the method to modify
    void set_return_value_type(MethodInfo& p_method, Variant::Type p_type);

    /// Starts looking for the method in the specified class, and then in its parent classes.
    /// @param p_class_name the name of the class to start the search
    /// @param p_method_name the name of the method to search for
    /// @return the class that contains the method, or an empty string if the method is not found
    String get_method_class(const String& p_class_name, const String& p_method_name);

    /// Generates a method signature based on the specified method.
    /// @param p_method the method
    /// @return the signature
    String get_signature(const MethodInfo& p_method);

    /// Calculates the number of arguments that have no default values
    /// @param p_method the method
    /// @return the number of arguments that have no default values
    size_t get_argument_count_without_defaults(const MethodInfo& p_method);

    //// Checks whether two <code>MethodInfo</code> structures have the same structures
    /// @param p_method_a the left method structure
    /// @param p_method_b the right method structure
    /// @return true if the method structures are the same, false otherwise
    bool has_same_signature(const MethodInfo& p_method_a, const MethodInfo& p_method_b);
}

#endif // ORCHESTRATOR_METHOD_UTILS_H
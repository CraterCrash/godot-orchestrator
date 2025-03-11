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
#ifndef ORCHESTRATOR_SCRIPT_SERVER_H
#define ORCHESTRATOR_SCRIPT_SERVER_H

#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

/// A helper class to accessing methods similarly found in Godot's ScriptServer.
class ScriptServer
{
public:
    /// Represents a Global Class entry in the script server.
    struct GlobalClass
    {
        StringName name;        //! Global class name
        StringName base_type;   //! The type that the global class extends
        String icon_path;       //! The path to the `@icon`
        String path;            //! The path to the global class script
        String language;        //! Language that contributes the class

        /// Returns the list of properties on the global class
        /// @return an array of dictionary entries for properties
        TypedArray<Dictionary> get_property_list() const;

        /// Returns the list of methods on the global class
        /// @return an array of dictionary entries for methods
        TypedArray<Dictionary> get_method_list() const;

        /// Returns the list of signals on the global class
        /// @return an array of dictionary entries for signals
        TypedArray<Dictionary> get_signal_list() const;

        /// Returns the constants of a global class
        /// @return the constants map
        Dictionary get_constants_list() const;

        /// Checks whether the method name exists for the global class
        /// @param p_method_name the method name to check
        /// @return true if the method exists, false otherwise
        bool has_method(const StringName& p_method_name) const;

        /// Checks whether the property exists for the global class
        /// @param p_property_name the property name to check
        /// @return true if the property exists, false otherwise
        bool has_property(const StringName& p_property_name) const;

        /// Checks whether the signal exists for the global class
        /// @param p_signal_name the signal name to check
        /// @return true if the signal exists, false otherwise
        bool has_signal(const StringName& p_signal_name) const;

        /// Returns the list of static methods on this class
        /// @return an array of dictionary entries for static methods
        TypedArray<Dictionary> get_static_method_list() const;
    };

protected:
    /// Get the global class dictionary entry
    /// @param p_class_name the global class name to find
    /// @return the dictionary for the global class, or an empty dictionary if not found
    static Dictionary _get_global_class(const StringName& p_class_name);

public:
    /// Checks whether the specified class name is a global script class.
    /// @param p_class_name the global class name to check
    /// @return true if the class is a global class, or false if not
    static bool is_global_class(const StringName& p_class_name);

    /// Checks whether the specified source class name is descendent of the target class.
    /// @param p_source_class_name the source class name
    /// @param p_target_class_name the target class_name
    /// @return true if the source is a descendant of the target class, false otherwise
    static bool is_parent_class(const StringName& p_source_class_name, const StringName& p_target_class_name);

    /// Get a list of all global classes
    /// @return an array of global class names
    static PackedStringArray get_global_class_list();

    /// Returns the global class entry for a specified class name.
    /// @param p_class_name the global class name to check
    /// @return the global class info structure, or an empty structure if not found
    static GlobalClass get_global_class(const StringName& p_class_name);

    /// Returns the native class for which this global class derives
    /// @param p_class_name the global class name
    /// @return the native class name
    static StringName get_native_class_name(const StringName& p_class_name);

    /// Returns a list of class hierarchies, starting with the global class first
    /// @param p_class_name the global class name to check
    /// @param p_include_native_classes if true, all native classes are included in bottom-up order
    /// @return all classes in the hierarchy, in bottom-up order
    static PackedStringArray get_class_hierarchy(const StringName& p_class_name, bool p_include_native_classes = false);

    /// Get the global name of the specified script
    /// @param p_script the script instance
    /// @return the global name of the script, if one is present
    static String get_global_name(const Ref<Script>& p_script);
};

#endif // ORCHESTRATOR_SCRIPT_SERVER_H
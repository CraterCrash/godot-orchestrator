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
#ifndef ORCHESTRATOR_SCRIPT_VARIABLE_H
#define ORCHESTRATOR_SCRIPT_VARIABLE_H

#include <godot_cpp/classes/resource.hpp>

using namespace godot;

/// Forward declarations
class Orchestration;

/// Defines a script variable
///
/// Variables are defined as resources which provides multiple benefits. First, it allows
/// us to use Godot's serialization technique for them as embedded elements in the script
/// while also exposing the directly in the Editor's InspectorDock.
///
class OScriptVariable : public Resource
{
    friend class Orchestration;

    GDCLASS(OScriptVariable, Resource);

    static void _bind_methods();

    Orchestration* _orchestration{ nullptr };  //! The owning orchestration
    PropertyInfo _info;                        //! Basic property details
    Variant _default_value;                    //! Optional defined default value
    String _description;                       //! An optional description for the variable
    String _category;                          //! Category for variables
    bool _exported{ false };                   //! Whether the variable is exposed on the node
    bool _exportable{ false };                 //! Tracks whether the variable can be exported
    String _classification;                    //! Variable classification
    int _type_category{ 0 };                   //! Defaults to basic
    Variant _type_subcategory;                 //! Subcategory type
    String _value_list;                        //! Enum/Bitfield custom value list
    bool _constant{ false };                   //! Whether variable is a constant

protected:
    //~ Begin Wrapped Interface
    void _validate_property(PropertyInfo& p_property) const;
    bool _property_can_revert(const StringName& p_name) const;
    bool _property_get_revert(const StringName& p_name, Variant& r_property);
    //~ End Wrapped Interface

    /// Get whether the specified property is exportable.
    /// @param p_property the property
    /// @return true if the property can be exported, false otherwise
    bool _is_exportable_type(const PropertyInfo& p_property) const;

    /// Attempt to convert the default value to the new type
    /// @return true if the conversion was successful, false otherwise
    bool _convert_default_value(Variant::Type p_new_type);

    /// Constructor
    /// Intentionally protected, variables created via an Orchestration
    OScriptVariable();

public:
    /// Performs post resource initialization.
    /// This is used to align and fix-up state across versions.
    void post_initialize();

    /// Get a reference to the orchestration that owns this variable.
    /// @return the owning orchestration reference, should always be valid
    Orchestration* get_orchestration() const;

    /// Get the variable's PropertyInfo structure
    /// @return the property info
    const PropertyInfo& get_info() const { return _info; }

    /// Get the variable's name
    /// @return the variable's name
    String get_variable_name() const { return _info.name; }

    /// Set the variable's name
    /// @param p_name the variable name
    void set_variable_name(const String& p_name);

    /// Returns whether the category name should be used in grouping.
    /// This centralizes the logic as using "Default", "None", or an empty string will validate
    /// at not being grouped where-as any other value will.
    /// @return true if the variable should be categorized, false to show it uncategorized.
    bool is_grouped_by_category() const;

    /// Get the variable's category
    /// @return the category for the variable
    String get_category() const { return _category; }

    /// Sets the category
    /// @param p_category the variable's category
    void set_category(const String& p_category);

    /// Get the variable's type
    /// @return the variable type
    String get_classification() const { return _classification; }

    /// Set the variable's type
    /// @param p_classification the variable type
    void set_classification(const String& p_classification);

    /// Get the custom value list for enum/bitfields
    /// @return the custom value list
    String get_custom_value_list() const { return _value_list; }

    /// Sets the custom value list for enum/bitfields
    /// @param p_value_list the custom value list
    void set_custom_value_list(const String& p_value_list);

    /// Get the variable type
    /// @return the variable type
    Variant::Type get_variable_type() const { return _info.type; }

    /// Set the variable type
    /// @param p_type the variable type
    void set_variable_type(Variant::Type p_type);

    /// Get the variable type name
    /// @return the variable type name
    String get_variable_type_name() const;

    /// Get the variable's description
    /// @return description for the variable
    String get_description() const { return _description; }

    /// Set the variable's description
    /// @param p_description the description
    void set_description(const String& p_description);

    /// Is the variable to be exported
    /// @return true if the variable is exported, false otherwise
    bool is_exported() const { return _exported; }

    /// Set the variable as exported.
    /// @param p_exported true exports the variable, false otherwise
    void set_exported(bool p_exported);

    /// Is the variable exportable.
    /// @return true if the variable can be exported, false otherwise
    bool is_exportable() const { return _is_exportable_type(_info); }

    /// Get the default value for the variable, if one is defined
    /// @return variable's default value
    Variant get_default_value() const { return _default_value; }

    /// Set the variable's default value
    /// @param p_default_value the default value
    void set_default_value(const Variant& p_default_value);

    /// Return whether the variable is a constant value
    /// @return true if the variable is a constant and cannot be set, false otherwise
    bool is_constant() const { return _constant; }

    /// Sets whether the variable is a constant
    /// @param p_constant whether the variable is constant
    void set_constant(bool p_constant);

    /// Copy the persistent state from one variable to this variable
    /// @param p_other the other variable to source state from
    void copy_persistent_state(const Ref<OScriptVariable>& p_other);
};

#endif  // ORCHESTRATOR_SCRIPT_VARIABLE_H

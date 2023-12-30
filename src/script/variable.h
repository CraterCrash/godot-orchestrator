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
#ifndef ORCHESTRATOR_SCRIPT_VARIABLE_H
#define ORCHESTRATOR_SCRIPT_VARIABLE_H

#include <godot_cpp/classes/resource.hpp>

using namespace godot;

/// Forward declarations
class OScript;

/// Defines a script variable
///
/// Variables are defined as resources which provides multiple benefits. First, it allows
/// us to use Godot's serialization technique for them as embedded elements in the script
/// while also exposing the directly in the Editor's InspectorDock.
///
class OScriptVariable : public Resource
{
    GDCLASS(OScriptVariable, Resource);

    static void _bind_methods();

    PropertyInfo _info;           //! Basic property details
    Variant _default_value;       //! Optional defined default value
    String _description;          //! An optional description for the variable
    String _category;             //! Category for variables
    bool _exported{ false };      //! Whether the variable is exposed on the node
    OScript* _script{ nullptr };  //! Owning script

protected:

    //~ Begin Wrapped Interface
    void _validate_property(PropertyInfo& p_property) const;
    //~ End Wrapped Interface

public:
    /// Constructor
    OScriptVariable();

    /// Get a reference to the script that owns this variable.
    /// @return the owning script reference, should always be valid
    Ref<OScript> get_owning_script() const;

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

    /// Get the variable type
    /// @return the variable type
    Variant::Type get_variable_type() const { return _info.type; }

    /// Set the variable type
    /// @param p_type the variable type
    void set_variable_type(Variant::Type p_type);

    /// Get the variable type name
    /// @return the variable type name
    String get_variable_type_name() const { return Variant::get_type_name(_info.type); }

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

    /// Get the default value for the variable, if one is defined
    /// @return variable's default value
    Variant get_default_value() const { return _default_value; }

    /// Set the variable's default value
    /// @param p_default_value the default value
    void set_default_value(const Variant& p_default_value);

    /// Helper method to construct a OScriptVariable from a Godot PropertyInfo struct.
    /// @param p_script the script that will own the signal
    /// @param p_property the property info struct
    static Ref<OScriptVariable> create(OScript* p_script, const PropertyInfo& p_property);

};

#endif  // ORCHESTRATOR_SCRIPT_VARIABLE_H

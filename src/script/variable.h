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
class OScriptFunction;

/// The base implementation for all variable types (orchestration and local function variables)
class OScriptVariableBase : public Resource
{
    GDCLASS(OScriptVariableBase, Resource);
    static void _bind_methods();

protected:
    PropertyInfo _info;         //! Variable property details
    String _description;        //! Description
    String _category;           //! Category
    Variant _default_value;     //! Default value
    String _classification;     //! Variable type classification
    bool _constant{ false };    //! Is variable a constant
    bool _exportable{ false };  //! Whether the variable can be exported
    bool _exported{ false };    //! Is variable exported
    String _custom_value_list;  //! Custom value list for enum/bitfields

    //~ Begin Wrapped Interface
    void _validate_property(PropertyInfo& p_property) const;
    bool _property_can_revert(const StringName& p_name) const;
    bool _property_get_revert(const StringName& p_name, Variant& r_property);
    //~ End Wrapped Interface

    /// Whether the variable type supports constants
    virtual bool _supports_constants() const { return false; }

    /// Whether the variable supports exporting
    virtual bool _supports_exported() const { return false; }

    /// Whether the variable supports legacy "type" attributes
    virtual bool _supports_legacy_type() const { return false; }

    /// Gets the variable type, if supported
    /// @return the variable type
    Variant::Type _get_variable_type() const { return _info.type; }

    /// Sets the variable type, if supported
    /// @param p_type the variable type
    void _set_variable_type(Variant::Type p_type);

    /// Returns whether the property definition is an exportable type
    /// @param p_property the property to check if it can be exported
    /// @return true if the property can be marked as exported, false otherwise
    virtual bool _is_exportable_type(const PropertyInfo& p_property) const;

    // Abstract class, cannot be constructed
    OScriptVariableBase() = default;

public:
    /// Perform post initialization steps after orchestration is loaded
    virtual void post_initialize();

    /// Returns if the category name should be used in grouping.
    /// @return true if the category should apply grouping, false otherwise
    bool is_grouped_by_category() const;

    /// Get the variable's property details
    /// @return the property info
    const PropertyInfo& get_info() const { return _info; }

    /// Get the variable type
    /// @return the variable type
    [[deprecated("Use get_info().type instead")]]
    Variant::Type get_variable_type() const
    {
        return _info.type;
    }

    /// Get the variable type name
    /// @return variable type name
    String get_variable_type_name() const;

    /// Get the variable name
    /// @return the variable's name
    String get_variable_name() const { return _info.name; }

    /// Set the variable name
    /// @param p_name the new variable name
    void set_variable_name(const String& p_name);

    /// Get the variable description
    /// @return the description
    String get_description() const { return _description; }

    /// Set the variable's description
    /// @param p_description the variable's description
    void set_description(const String& p_description);

    /// Get the variable's category
    /// @return the category
    String get_category() const { return _category; }

    /// Set the variable's category
    /// @param p_category the category
    void set_category(const String& p_category);

    /// Get the variable's default value
    /// @return the default value
    Variant get_default_value() const { return _default_value; }

    /// Sets the default value for the variable
    /// @param p_value the default value to be set
    void set_default_value(const Variant& p_value);

    /// Get the variable type classification
    /// @return the classification
    String get_classification() const { return _classification; }

    /// Sets the variable type's classification
    /// @param p_classification the type classification
    void set_classification(const String& p_classification);

    /// Gets the custom value list
    /// @return the custom value list for enum/bitfields
    String get_custom_value_list() const { return _custom_value_list; }

    /// Sets the custom value list, used for custom enums/bitfields
    /// @param p_custom_value_list
    void set_custom_value_list(const String& p_custom_value_list);

    /// Get whether the variable is a constant
    /// @return whether the variable is read-only, constant
    bool is_constant() const { return _constant; }

    /// Sets whether the variable is a constant
    /// @param p_constant whether the variable is a constant
    void set_constant(bool p_constant);

    /// Get whether the variable can be exported
    /// @return true if the variable can be exported, false otherwise
    bool is_exportable() const { return _exportable; }

    /// Get whether the variable is exported
    /// @return whether the variable is exported
    bool is_exported() const { return _exported; }

    /// Set whether the variable is exported
    /// @param p_exported is the variable exported
    void set_exported(bool p_exported);

    /// Check whether the variable supports validated getters
    /// @return true if validated getters are allowed, false otherwise
    bool supports_validated_getter() const;
};

/// Variable implementation for function local variables
class OScriptLocalVariable : public OScriptVariableBase
{
    friend class OScriptFunction;

    GDCLASS(OScriptLocalVariable, OScriptVariableBase);
    static void _bind_methods() { }

protected:
    /// Intentionally protected, created via OScriptFunction
    OScriptLocalVariable() = default;
};

/// Defines a top-level script variable that can be exported and available to the outside world.
class OScriptVariable : public OScriptVariableBase
{
    friend class Orchestration;

    GDCLASS(OScriptVariable, OScriptVariableBase);
    static void _bind_methods() { }

protected:
    Orchestration* _orchestration{ nullptr };  //! Owning orchestration

    //~ Begin OScriptVariableBase Interface
    bool _supports_constants() const override { return true; }
    bool _supports_exported() const override { return true; }
    bool _supports_legacy_type() const override { return true; }
    //~ End OScriptVariableBase Interface

    /// Intentionally protected, created via OScriptFunction
    OScriptVariable() = default;
};

#endif  // ORCHESTRATOR_SCRIPT_VARIABLE_H

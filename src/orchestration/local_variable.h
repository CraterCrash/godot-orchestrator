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

#include <godot_cpp/classes/resource.hpp>

using namespace godot;

/// Forward declarations
class OScriptFunction;

/// Defines a function-scoped local variable
///
/// A local variable is declared by, and lives for the duration of, a single function. It is a trimmed-down
/// variable: it has a name, a type, a default value and a description, but it can never be exported, constant,
/// categorized or annotated.
///
/// Local variables are resources so the editor's InspectorDock can edit them directly, but they are persisted
/// as plain property maps inside the owning function rather than as sub-resources, so the function's storage
/// properties carry them wherever the function goes.
///
class OScriptLocalVariable : public Resource {
    friend class Orchestration;
    friend class OScriptFunction;

    GDCLASS(OScriptLocalVariable, Resource);

    OScriptFunction* _function = nullptr;  //! The owning function
    PropertyInfo _info;                    //! Basic property details
    Variant _default_value;                //! Optional defined default value
    String _description;                   //! An optional description for the variable

    //~ Begin Serializers
    void _set_property_info(const Dictionary& p_property);
    Dictionary _get_property_info() const;
    //~ End Serializers

    /// Marks the owning orchestration as edited
    void _mark_edited();

protected:
    static void _bind_methods();

    //~ Begin Wrapped Interface
    void _validate_property(PropertyInfo& p_property) const;
    bool _property_can_revert(const StringName& p_name) const;
    bool _property_get_revert(const StringName& p_name, Variant& r_property);
    //~ End Wrapped Interface

public:
    /// Get the function that declares this local variable
    /// @return the owning function
    OScriptFunction* get_function() const;

    const PropertyInfo& get_info() const;
    void set_info(const PropertyInfo& p_property);

    String get_variable_name() const { return _info.name; }
    void set_variable_name(const String& p_name);

    String get_variable_type_name() const;

    Variant get_default_value() const { return _default_value; }
    void set_default_value(const Variant& p_default_value);

    String get_description() const { return _description; }
    void set_description(const String& p_description);

    void copy_persistent_state(const Ref<OScriptLocalVariable>& p_other);

    OScriptLocalVariable(); // Should be created inside OScriptFunction
};
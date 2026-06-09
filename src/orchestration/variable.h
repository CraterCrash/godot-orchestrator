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
class Orchestration;

/// Defines a script variable
///
/// Variables are defined as resources which provides multiple benefits. First, it allows
/// us to use Godot's serialization technique for them as embedded elements in the script
/// while also exposing it directly in the Editor's InspectorDock.
///
class OScriptVariable : public Resource {
    friend class Orchestration;

    GDCLASS(OScriptVariable, Resource);

    Orchestration* _orchestration = nullptr;   //! The owning orchestration
    PropertyInfo _info;                        //! Basic property details
    Variant _default_value;                    //! Optional defined default value
    String _description;                       //! An optional description for the variable
    String _category;                          //! Category for variables
    bool _exported = false;                    //! Whether the variable is exposed on the node
    bool _constant = false;                    //! Whether variable is a constant

protected:
    static void _bind_methods();

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_properties) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    void _validate_property(PropertyInfo& p_property) const;
    bool _property_can_revert(const StringName& p_name) const;
    bool _property_get_revert(const StringName& p_name, Variant& r_property);
    //~ End Wrapped Interface

    //~ Begin Serializers
    void _set_property_info(const Dictionary& p_property);
    Dictionary _get_property_info() const;
    //~ End Serializers

    /// Attempt to convert the default value to the new type
    /// @return true if the conversion was successful, false otherwise
    bool _convert_default_value(Variant::Type p_new_type);

public:
    Orchestration* get_orchestration() const;

    const PropertyInfo& get_info() const;
    void set_info(const PropertyInfo& p_property);

    PropertyInfo get_export_info() const;

    String get_variable_name() const { return _info.name; }
    void set_variable_name(const String& p_name);

    bool is_grouped_by_category() const;
    String get_category() const { return _category; }
    void set_category(const String& p_category);

    String get_variable_type_name() const;

    String get_description() const { return _description; }
    void set_description(const String& p_description);

    bool is_exported() const { return _exported; }
    void set_exported(bool p_exported);
    bool is_exportable() const;

    Variant get_default_value() const { return _default_value; }
    void set_default_value(const Variant& p_default_value);

    bool is_constant() const { return _constant; }
    void set_constant(bool p_constant);

    void copy_persistent_state(const Ref<OScriptVariable>& p_other);

    static String decode_property(const String& p_value);

    OScriptVariable(); // Should be created inside Orchestration
};

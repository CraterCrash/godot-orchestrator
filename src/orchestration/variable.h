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

#include "orchestration/annotation.h"

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

public:
    /// How the default value initializes the variable
    enum InitializerKind {
        INITIALIZER_LITERAL,    //! The default value is assigned as-is
        INITIALIZER_NODE_PATH,  //! The default value is a NodePath resolved with get_node() on the owner
    };

private:
    Orchestration* _orchestration = nullptr;   //! The owning orchestration
    PropertyInfo _info;                        //! Basic property details
    Variant _default_value;                    //! Optional defined default value
    String _description;                       //! An optional description for the variable
    String _category;                          //! Category for variables
    OScriptAnnotationList _annotations;        //! Annotations applied to the variable
    InitializerKind _initializer = INITIALIZER_LITERAL;  //! How the default value is applied
    bool _constant = false;                    //! Whether variable is a constant

    /// Falls back to a literal initializer when the type no longer accepts a node path
    void _reset_initializer_if_needed();

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
    void _set_annotations_array(const Array& p_annotations);
    Array _get_annotations_array() const;
    void _set_initializer(int p_initializer);
    int _get_initializer() const;
    //~ End Serializers

    /// Attempt to convert the default value to the new type
    /// @return true if the conversion was successful, false otherwise
    bool _convert_default_value(Variant::Type p_new_type);

    /// Drops annotations that no longer apply to the variable's type
    /// @return true if any annotation was removed
    bool _prune_annotations();

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

    /// Whether an annotation from the export family is applied
    bool is_exported() const;
    /// Applies a plain "@export" when true and none of the export family is present,
    /// or removes whichever export-family annotation is applied when false.
    void set_exported(bool p_exported);
    bool is_exportable() const;

    const Vector<OScriptAnnotation>& get_annotations() const { return _annotations.get_items(); }
    bool has_annotation(const StringName& p_name) const { return _annotations.has(p_name); }
    bool has_annotation_family(const StringName& p_family) const { return _annotations.has_family(p_family); }
    int find_annotation(const StringName& p_name) const { return _annotations.find(p_name); }

    /// Adds an annotation when the registry permits it for this variable
    /// @param p_annotation the annotation to add
    /// @param r_reason optional explanation when the annotation is rejected
    /// @return OK if added, otherwise the registry's error
    Error add_annotation(const OScriptAnnotation& p_annotation, String* r_reason = nullptr);
    void remove_annotation(int p_index);
    void set_annotation_arguments(int p_index, const Array& p_arguments);

    /// Replaces the whole annotation list, used by serialization and undo snapshots
    void set_annotations(const Vector<OScriptAnnotation>& p_annotations);

    Variant get_default_value() const { return _default_value; }
    void set_default_value(const Variant& p_default_value);

    InitializerKind get_initializer_kind() const { return _initializer; }
    /// Switching to a node path clears the default to an empty NodePath; switching back restores
    /// the type's default. The "@onready" annotation is left to the user, as in GDScript.
    void set_initializer_kind(InitializerKind p_initializer);
    bool is_node_path_initializer() const { return _initializer == INITIALIZER_NODE_PATH; }
    /// Whether the variable's type can be initialized from a node path: Variant or a Node class
    bool is_node_path_initializer_allowed() const;

    bool is_constant() const { return _constant; }
    void set_constant(bool p_constant);

    void copy_persistent_state(const Ref<OScriptVariable>& p_other);

    static String decode_property(const String& p_value);

    OScriptVariable(); // Should be created inside Orchestration
};

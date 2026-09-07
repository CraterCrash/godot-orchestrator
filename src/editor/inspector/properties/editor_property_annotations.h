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

#include <godot_cpp/classes/editor_property.hpp>
#include <godot_cpp/classes/grid_container.hpp>
#include <godot_cpp/classes/h_flow_container.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/menu_button.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/object.hpp>

using namespace godot;

/// Forward declarations
struct OScriptAnnotationDescriptor;

/// An EditorProperty for the "annotations" list on a script member.
///
/// Renders one chip per applied annotation, an add menu filtered through the annotation registry,
/// and an argument form generated from the selected annotation's signature. Every edit emits the
/// whole list, so the inspector's undo history captures it as a single property change.
///
class OrchestratorEditorPropertyAnnotations : public EditorProperty {
    GDCLASS(OrchestratorEditorPropertyAnnotations, EditorProperty);

    MenuButton* _add_button = nullptr;               //! Add menu, populated from the registry when opened
    MarginContainer* _margin = nullptr;              //! Bottom editor margin
    HFlowContainer* _chips = nullptr;                //! One chip per applied annotation
    VBoxContainer* _form_box = nullptr;              //! Holds the argument form for the expanded chip
    GridContainer* _form = nullptr;                  //! Argument form, label and editor per row
    Vector<OScriptAnnotation> _annotations;          //! Working copy of the edited list
    int _expanded = -1;                              //! Index of the chip whose form is shown
    bool _self_change = false;                       //! Whether the next update was caused by this editor
    bool _form_stale = true;                         //! Whether the form must be rebuilt on the next update
    bool _read_only = false;                         //! Whether editing is disabled

    /// Reads the owner's type information and kind for registry checks
    /// @param r_owner receives the owner's property information
    /// @param r_target receives the registry target kind
    /// @param r_constant receives whether the owner is a constant
    void _get_owner(PropertyInfo& r_owner, uint32_t& r_target, bool& r_constant);

    /// Formats an annotation as it would appear in source, for chip text
    String _get_summary(const OScriptAnnotation& p_annotation) const;

    /// Default value for a signature argument, from the signature or the argument type
    Variant _get_argument_default(const OScriptAnnotationDescriptor& p_descriptor, int p_index) const;

    /// Signature argument index for a stored argument, clamped for vararg signatures
    int _get_signature_index(const OScriptAnnotationDescriptor& p_descriptor, int p_argument) const;

    void _emit();
    void _rebuild_chips();
    void _rebuild_form();
    void _refresh_chip_text();

    /// Builds the editor control for one argument
    Control* _make_argument_editor(const OScriptAnnotationDescriptor& p_descriptor, int p_signature_index, const Variant& p_value, int p_argument);

    /// Writes an argument value, filling any missing preceding optional arguments with defaults
    void _set_argument(int p_argument, const Variant& p_value);

    //~ Begin Signal Handlers
    void _add_menu_about_to_popup();
    void _add_menu_id_pressed(int p_id);
    void _chip_toggled(bool p_pressed, int p_index);
    void _chip_remove_pressed(int p_index);
    void _argument_value_changed(double p_value, int p_argument, bool p_integer);
    void _argument_text_submitted(const String& p_text, int p_argument);
    void _argument_focus_exited(int p_argument);
    void _argument_toggled(bool p_pressed, int p_argument);
    void _argument_item_selected(int p_item, int p_argument);
    void _argument_remove_pressed(int p_argument);
    void _argument_add_pressed();
    //~ End Signal Handlers

protected:
    static void _bind_methods() { }

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

public:
    //~ Begin EditorProperty Interface
    void _update_property() override;
    void _set_read_only(bool p_read_only) override;
    //~ End EditorProperty Interface
};
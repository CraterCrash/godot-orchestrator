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
#ifndef ORCHESTRATOR_EDITOR_INSPECTOR_PROPERTY_INFO_CONTAINER_PROPERTY_H
#define ORCHESTRATOR_EDITOR_INSPECTOR_PROPERTY_INFO_CONTAINER_PROPERTY_H

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_property.hpp>
#include <godot_cpp/classes/grid_container.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/margin_container.hpp>

using namespace godot;

/// Forward declarations
class OrchestratorVariableTypeSearchDialog;

/// An EditorProperty implementation that works with a TypedArray<Dictionary> collection of
/// zero or more PropertyInfo objects, allowing the user to set the name and type of each property.
class OrchestratorPropertyInfoContainerEditorProperty : public EditorProperty
{
    GDCLASS(OrchestratorPropertyInfoContainerEditorProperty, EditorProperty);
    static void _bind_methods();

    /// A slot represents a collection of UI widgets for a given property.
    /// Each slot is mapped to a specific PropertyInfo object by index.
    struct Slot
    {
        LineEdit* name{ nullptr };               //! Property name
        Button* type{ nullptr };                 //! Property type
        HBoxContainer* button_group{ nullptr };  //! Button group
    };

protected:
    OrchestratorVariableTypeSearchDialog* _dialog{ nullptr };  //! Dialog for selecting property types
    MarginContainer* _margin{ nullptr };                       //! Margin container for the widgets
    GridContainer* _container{ nullptr };                      //! Grid container for all controls
    Button* _add_button{ nullptr };                            //! Button for adding a new property
    Vector<Slot> _slots;                                       //! Slots
    Vector<PropertyInfo> _properties;                          //! Properties
    int _max_entries{ INT_MAX };                               //! Maximum allowed properties
    bool _args{ false };                                       //! Represents argument or return value list
    bool _allow_rearrange{ false };                            //! Whether move up/down is enabled

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

    //~ Begin Signal Handlers
    void _add_property();
    void _rename_property(const String& p_name, int p_index);
    void _remove_property(int p_index);
    void _argument_type_selected(int p_index);
    void _show_type_selection(int p_index, const String& p_value);
    void _cleanup_selection();
    void _move_up(int p_index);
    void _move_down(int p_index);
    //~ End Signal Handlers

    /// Reads the properties from the object and populates the internal list.
    void _get_properties();

    /// Writes the internal list of properties to the edited object.
    void _set_properties();

    /// Updates the pass by button details
    /// @param p_index the slot index
    /// @param p_property the property details
    void _update_pass_by_details(int p_index, const PropertyInfo& p_property);

    /// Updates move button state
    /// @param p_force_disable force buttons disabled
    void _update_move_buttons(bool p_force_disable = false);

public:
    //~ Begin EditorProperty Interface
    void _update_property() override;
    //~ End EditorProperty Interface

    /// Sets whether rearrangement of properties is allowed.
    /// @param p_allow_rearrange true if move up/down is allowed
    void set_allow_rearrange(bool p_allow_rearrange) { _allow_rearrange = p_allow_rearrange; }

    /// Set up the editor property
    /// @param p_inputs specifies whether this represents input arguments, defaults to false
    /// @param p_max_entries the maximum number of allowed properties, defaults to INT_MAX
    void setup(bool p_inputs = false, int p_max_entries = INT_MAX);
};

#endif  // ORCHESTRATOR_EDITOR_INSPECTOR_PROPERTY_INFO_CONTAINER_PROPERTY_H
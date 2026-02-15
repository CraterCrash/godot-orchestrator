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
#ifndef ORCHESTRATOR_PROPERTY_SELECTOR_H
#define ORCHESTRATOR_PROPERTY_SELECTOR_H

#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/templates/vector.hpp>

using namespace godot;

/// Displays a list of properties for a given criteria.
class OrchestratorPropertySelector : public ConfirmationDialog {
    GDCLASS(OrchestratorPropertySelector, ConfirmationDialog);

    LineEdit* _search_box = nullptr;     //! The filter/search box
    Tree* _search_options = nullptr;     //! The list of search options
    String _selected;                    //! The selected property name
    Variant::Type _type;                 //! The property type to limit
    String _base_type;                   //! The base type
    ObjectID _script;                    //! The script's object identifier
    Object* _instance = nullptr;         //! The object to base the list on
    Vector<Variant::Type> _type_filter;  //! The type filter

    //~ Begin Signal Handlers
    void _text_changed(const String& p_new_text);
    void _sbox_input(const Ref<InputEvent>& p_event);
    void _confirmed();
    void _item_selected();
    //~ End Signal Handlers

    /// Checks whether the text contains the what
    /// @param p_text the text to search
    /// @param p_what what to search
    bool _contains_ignore_case(const String& p_text, const String& p_what) const;

    /// Updates the search options based on the filter
    void _update_search();

protected:
    static void _bind_methods();

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

public:
    /// Set property from an object instance
    /// @param p_instance the object instance
    /// @param p_current the current property choice
    void select_property_from_instance(Object* p_instance, const String& p_current = "");

    /// Set the type filter
    /// @param p_type_filter the types to filter
    void set_type_filter(const Vector<Variant::Type>& p_type_filter);

    /// Constructs the property selector
    OrchestratorPropertySelector();
};

#endif  // ORCHESTRATOR_PROPERTY_SELECTOR_H

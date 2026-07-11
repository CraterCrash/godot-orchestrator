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

#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/templates/vector.hpp>

using namespace godot;

class OrchestratorEditorFilterLineEdit;
class OrchestratorEditorHelpBit;

/// Displays a list of properties for a given criteria.
class OrchestratorPropertySelector : public ConfirmationDialog {
    GDCLASS(OrchestratorPropertySelector, ConfirmationDialog);

    OrchestratorEditorFilterLineEdit* _search_box = nullptr;
    OrchestratorEditorHelpBit* _help_bit = nullptr;

    Tree* _search_options = nullptr;

    bool _properties = false;
    String _selected;
    Variant::Type _type;
    String _base_type;
    ObjectID _script;
    Object* _instance = nullptr;
    bool _virtuals_only = false;

    Vector<Variant::Type> _type_filter;

    //~ Begin Signal Handlers
    void _text_changed(const String& p_new_text);
    void _confirmed();
    void _item_selected();
    void _hide_requested();
    //~ End Signal Handlers

    bool _contains_ignore_case(const String& p_text, const String& p_what) const;
    void _update_search();

    void _create_subproperties(TreeItem* p_parent_item, Variant::Type p_type);
    void _create_subproperty(TreeItem* p_parent_item, const String& p_name, Variant::Type p_type);

protected:
    static void _bind_methods();

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

public:

    void select_method_from_base_type(const String& p_base, const String& p_current = "", bool p_virtuals_only = false);
    void select_method_from_script(const Ref<Script>& p_script, const String& p_current = "");
    void select_method_from_basic_type(Variant::Type p_type, const String& p_current = "");
    void select_method_from_instance(Object* p_instance, const String& p_current = "");

    void select_property_from_base_type(const String& p_base_type, const String& p_current = "");
    void select_property_from_script(const Ref<Script>& p_script, const String& p_current = "");
    void select_property_from_basic_type(Variant::Type p_type, const String& p_current = "");
    void select_property_from_instance(Object* p_instance, const String& p_current = "");

    void set_type_filter(const Vector<Variant::Type>& p_type_filter);

    /// Constructs the property selector
    OrchestratorPropertySelector();
};

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
#ifndef ORCHESTRATOR_EDITOR_INSPECTOR_PROPERTY_TYPE_BUTTON_PROPERTY_H
#define ORCHESTRATOR_EDITOR_INSPECTOR_PROPERTY_TYPE_BUTTON_PROPERTY_H

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_property.hpp>

using namespace godot;

/// Forward declarations
class OrchestratorSelectTypeSearchDialog;

/// EditorProperty implementation for OScriptVariable classification properties
class OrchestratorEditorPropertyVariableClassification : public EditorProperty {
    GDCLASS(OrchestratorEditorPropertyVariableClassification, EditorProperty);

    OrchestratorSelectTypeSearchDialog* _dialog = nullptr;
    Button* _property = nullptr;
    String _selected_name;
    String _base_type = "Object";

protected:
    static void _bind_methods() { }

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

    //~ Begin Signal Handlers
    void _property_selected();
    void _search_selected();
    //~ End Signal Handlers

public:
    //~ Begin EditorProperty Interface
    void _update_property() override;
    //~ End EditorProperty Interface

    /// Shows the variable type dialog
    void edit();

    /// Set up the classification property editor
    /// @param p_base_type the base type
    /// @param p_selected_type the selected type
    void setup(const String& p_base_type, const String& p_selected_type);
};

#endif // ORCHESTRATOR_EDITOR_INSPECTOR_PROPERTY_TYPE_BUTTON_PROPERTY_H
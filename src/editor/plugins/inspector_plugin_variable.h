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
#ifndef ORCHESTRATOR_EDITOR_INSPECTOR_PLUGIN_VARIABLE_H
#define ORCHESTRATOR_EDITOR_INSPECTOR_PLUGIN_VARIABLE_H

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_inspector_plugin.hpp>
#include <godot_cpp/classes/editor_property.hpp>

using namespace godot;

/// Forward declarations
class OrchestratorVariableTypeSearchDialog;
class OrchestratorEditorPropertyVariableClassification;

/// EditorInspectorPlugin implementation for OScriptVariable objects
class OrchestratorEditorInspectorPluginVariable : public EditorInspectorPlugin
{
    GDCLASS(OrchestratorEditorInspectorPluginVariable, EditorInspectorPlugin);
    static void _bind_methods() { }

    mutable OrchestratorEditorPropertyVariableClassification* _classification;

public:
    //~ Begin EditorInspectorPlugin Interface
    bool _can_handle(Object* p_object) const override;
    bool _parse_property(Object* p_object, Variant::Type p_type, const String& p_name, PropertyHint p_hint,
                         const String& p_hint_string, BitField<PropertyUsageFlags> p_usage, bool p_wide) override;
    //~ End EditorInspectorPlugin Interface

    /// Allows external callers to edit the currently active variable's classification
    /// @param p_object the object to edit
    void edit_classification(Object* p_object);
};

/// EditorProperty implementation for OScriptVariable classification properties
class OrchestratorEditorPropertyVariableClassification : public EditorProperty
{
    GDCLASS(OrchestratorEditorPropertyVariableClassification, EditorProperty);
    static void _bind_methods() { }

    OrchestratorVariableTypeSearchDialog* _dialog{ nullptr };
    Button* _property{ nullptr };
    String _selected_name;
    String _base_type{ "Object" };

public:

    //~ Begin EditorProperty Interface
    void _update_property() override;
    //~ End EditorProperty Interface

    /// Godot callback that handles notifications
    /// @param p_what the notification to be handled
    void _notification(int p_what);

    /// Setup the classification property editor
    /// @param p_base_type the base type
    /// @param p_selected_type the selected type
    void setup(const String& p_base_type, const String& p_selected_type);

    /// Shows the variable type dialog
    void edit();

private:

    /// Dispatched when the inspector property button is clicked.
    void _on_property_selected();

    /// Dispatched when a selection happens in the search dialog
    void _on_search_selected();
};

#endif // ORCHESTRATOR_EDITOR_INSPECTOR_PLUGIN_VARIABLE_H
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
#ifndef ORCHESTRATOR_EDITOR_INSPECTOR_PLUGIN_VARIABLE_H
#define ORCHESTRATOR_EDITOR_INSPECTOR_PLUGIN_VARIABLE_H

#include <godot_cpp/classes/editor_inspector_plugin.hpp>

using namespace godot;

/// Forward declarations
class OrchestratorEditorPropertyVariableClassification;

/// An EditorInspectorPlugin that handles selecting the variable type for OScriptVariable objects
class OrchestratorEditorInspectorPluginVariable : public EditorInspectorPlugin {
    GDCLASS(OrchestratorEditorInspectorPluginVariable, EditorInspectorPlugin);

    mutable OrchestratorEditorPropertyVariableClassification* _classification = nullptr;

protected:
    static void _bind_methods() { }

public:
    //~ Begin EditorInspectorPlugin Interface
    bool _can_handle(Object* p_object) const override;
    bool _parse_property(Object* p_object, Variant::Type p_type, const String& p_name, PropertyHint p_hint, const String& p_hint_string, BitField<PropertyUsageFlags> p_usage, bool p_wide) override;
    //~ End EditorInspectorPlugin Interface

    /// Allows external callers to edit the currently active variable's classification
    /// @param p_object the object to edit
    void edit_classification(Object* p_object);
};


#endif // ORCHESTRATOR_EDITOR_INSPECTOR_PLUGIN_VARIABLE_H
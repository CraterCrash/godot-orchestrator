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
#ifndef ORCHESTRATOR_EDITOR_INSPECTOR_PLUGIN_FUNCTION_H
#define ORCHESTRATOR_EDITOR_INSPECTOR_PLUGIN_FUNCTION_H

#include "script/function.h"

#include <godot_cpp/classes/editor_inspector_plugin.hpp>

using namespace godot;

/// A simple EditorInspectorPlugin that adds custom UI widgets for function input/output properties
class OrchestratorEditorInspectorPluginFunction : public EditorInspectorPlugin
{
    GDCLASS(OrchestratorEditorInspectorPluginFunction, EditorInspectorPlugin);

protected:
    static void _bind_methods() { }

    //~ Begin Function Handlers
    void _move_up(int p_index, const Ref<OScriptFunction>& p_function);
    void _move_down(int p_index, const Ref<OScriptFunction>& p_function);
    //~ End Function Handlers

    /// Swaps two function arguments with one another
    /// @param p_index the function argument index
    /// @param p_pin_offset the emit function pin offset to swap
    /// @param p_argument_offset the argument offset to swap
    /// @param p_function the function to mutate
    static void _swap(int p_index, int p_pin_offset, int p_argument_offset, const Ref<OScriptFunction>& p_function);

public:
    //~ Begin EditorInspectorPlugin Interface
    bool _can_handle(Object* p_object) const override;
    bool _parse_property(Object* p_object, Variant::Type p_type, const String& p_name, PropertyHint p_hint_type, const String& p_hint_string, BitField<PropertyUsageFlags> p_usage_flags, bool p_wide) override;
    //~ End EditorInspectorPlugin Interface
};

#endif // ORCHESTRATOR_EDITOR_INSPECTOR_PLUGIN_FUNCTION_H
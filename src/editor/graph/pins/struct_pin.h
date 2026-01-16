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
#ifndef ORCHESTRATOR_EDITOR_GRAPH_PIN_STRUCT_H
#define ORCHESTRATOR_EDITOR_GRAPH_PIN_STRUCT_H

#include "editor/graph/graph_pin.h"

#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/templates/vector.hpp>

/// An implementation of <code>OrchestratorEditorGraphPin</code> wrapping struct-like Godot variant
/// data types, like Vector2, Quaternion, and Projection, which are types that are composed of two
/// or more smaller struct-like or primitive data types.
///
class OrchestratorEditorGraphPinStruct : public OrchestratorEditorGraphPin {
    GDCLASS(OrchestratorEditorGraphPinStruct, OrchestratorEditorGraphPin);

    Vector<LineEdit*> _controls;

    static int _get_grid_columns_for_type(Variant::Type p_type);
    static bool _is_property_excluded(Variant::Type p_type, const PropertyInfo& p_property);
    static PackedStringArray _get_property_paths(Variant::Type p_type);

    void _update_control_value_part(const String& p_path, int p_index, const Variant& p_value);
    void _read_control_value_part(const String& p_path, int p_index, Variant& r_value);

protected:
    static void _bind_methods() { }

    //~ Begin OrchestratorEditorGraphPin Interface
    bool _is_default_value_below_label() const override { return true; }
    void _update_control_value(const Variant& p_value) override;
    Variant _read_control_value() override;
    Control* _create_default_value_widget() override;
    //~ End OrchestratorEditorGraphPin Interface
};

#endif // ORCHESTRATOR_EDITOR_GRAPH_PIN_STRUCT_H
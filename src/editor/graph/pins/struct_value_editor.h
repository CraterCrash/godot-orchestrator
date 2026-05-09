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

#include "editor/graph/pins/pin_value_editor.h"

#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/templates/vector.hpp>

/// An implementation of <code>OrchestratorEditorGraphPinValueEditor</code> wrapping a Godot struct-like
/// variant data type, such as Vector2, Quaternion, and Projection. These types are composed
/// of two or more smaller struct or primitive data types.
///
class OrchestratorEditorGraphPinValueEditorStruct : public OrchestratorEditorGraphPinValueEditor {
    GDCLASS(OrchestratorEditorGraphPinValueEditorStruct, OrchestratorEditorGraphPinValueEditor);

    Vector<LineEdit*> _controls;
    PropertyInfo _property;

    static int _get_grid_columns_for_type(Variant::Type p_type);
    static bool _is_property_excluded(Variant::Type p_type, const PropertyInfo& p_property);
    static PackedStringArray _get_property_paths(Variant::Type p_type);

    void _update_control_part(const String& p_path, int p_index, const Variant& p_value);
    void _read_control_part(const String& p_path, int p_index, Variant& r_value);

    void _commit();

protected:
    static void _bind_methods() { }

public:
    //~ Begin OrchestratorEditorGraphPinValueEditor Interface
    bool is_below_label() const override { return true; }
    void configure(const PropertyInfo& p_property) override;
    void set_value(const Variant& p_value) override;
    //~ End OrchestratorEditorGraphPinValueEditor Interface
};

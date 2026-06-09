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

#include <godot_cpp/classes/color_picker_button.hpp>

/// An implementation of <code>OrchestratorEditorGraphPinValueEditor</code> that wraps a Godot
/// <code>ColorPickerButton</code>, allowing the user to select a <code>Color</code> value.
///
class OrchestratorEditorGraphPinValueEditorColorPicker : public OrchestratorEditorGraphPinValueEditor {
    GDCLASS(OrchestratorEditorGraphPinValueEditorColorPicker, OrchestratorEditorGraphPinValueEditor);

    ColorPickerButton* _control = nullptr;

protected:
    static void _bind_methods() { }

public:
    //~ Begin OrchestratorEditorGraphPinValueEditor Interface
    void configure(const PropertyInfo& p_property) override;
    void set_value(const Variant& p_value) override;
    //~ End OrchestratorEditorGraphPinValueEditor Interface
};

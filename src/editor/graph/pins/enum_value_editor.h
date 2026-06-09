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

#include <godot_cpp/classes/option_button.hpp>

/// An implementation of <code>OrchestratorEditorGraphPinValueEditor</code> allowing for selection enum values.
///
/// An enumeration is a data type that is represented by human-readable values that map to exactly
/// one value in a set of predefined name-to-integer mappings.
///
class OrchestratorEditorGraphPinValueEditorEnum : public OrchestratorEditorGraphPinValueEditor {
    GDCLASS(OrchestratorEditorGraphPinValueEditorEnum, OrchestratorEditorGraphPinValueEditor);

    OptionButton* _button = nullptr;
    int _selected_index = -1;
    bool _generated = false;
    PropertyInfo _property;

    void _item_selected(int p_index);

protected:
    static void _bind_methods() { }

public:
    //~ Begin OrchestratorEditorGraphPinValueEditor Interface
    void configure(const PropertyInfo& p_property) override;
    void set_value(const Variant& p_value) override;
    //~ End OrchestratorEditorGraphPinValueEditor Interface
};

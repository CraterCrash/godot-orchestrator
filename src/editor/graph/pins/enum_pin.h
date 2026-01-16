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
#ifndef ORCHESTRATOR_EDITOR_GRAPH_PIN_ENUM_H
#define ORCHESTRATOR_EDITOR_GRAPH_PIN_ENUM_H

#include "editor/graph/graph_pin.h"

#include <godot_cpp/classes/option_button.hpp>

/// An implementation of <code>OrchestratorEditorGraphPin</code> for enum data types.
///
/// An enumeration is a data type that is represented by human-readable values that map to
/// exactly one value in a set of predefined mappings.
///
class OrchestratorEditorGraphPinEnum : public OrchestratorEditorGraphPin {
    GDCLASS(OrchestratorEditorGraphPinEnum, OrchestratorEditorGraphPin);

    OptionButton* _button;
    int _selected_index;
    bool _generated = false;

    void _item_selected(int p_index);

protected:
    static void _bind_methods() { }

    //~ Begin OrchestratorEditorGraphPin Interface
    bool _is_default_value_below_label() const override { return true; }
    void _update_control_value(const Variant& p_value) override;
    Variant _read_control_value() override;
    Control* _create_default_value_widget() override;
    //~ End OrchestratorEditorGraphPin Interface
};

#endif // ORCHESTRATOR_EDITOR_GRAPH_PIN_ENUM_H
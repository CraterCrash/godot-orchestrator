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
#ifndef ORCHESTRATOR_EDITOR_GRAPH_PIN_BITFIELD_H
#define ORCHESTRATOR_EDITOR_GRAPH_PIN_BITFIELD_H

#include "editor/graph/pins/button_base_pin.h"

#include <godot_cpp/classes/check_box.hpp>

/// An implementation of <code>OrchestratorEditorGraphPin</code> for bitfield data type pins.
///
/// A bitfield data pin is an <code>int64_t</code> value that can represent zero, one, or more options.
/// This is made possible because the bitfield human-readable values, much like enumerations, are mapped
/// but with values that represent specific bits within the numeric value. This allows for the selection
/// of multiple values without overriding the other selections.
///
class OrchestratorEditorGraphPinBitfield : public OrchestratorEditorGraphPinButtonBase
{
    GDCLASS(OrchestratorEditorGraphPinBitfield, OrchestratorEditorGraphPinButtonBase);

    void _update_checkboxes(bool p_state, const CheckBox* p_box_control);

protected:
    static void _bind_methods() { }

    //~ Begin OrchestratorEditorGraphPinButtonBase Interface
    bool _is_default_value_below_label() const override { return true; }
    void _handle_selector_button_pressed() override;
    //~ End OrchestratorEditorGraphPinButtonBase Interface
};

#endif // ORCHESTRATOR_EDITOR_GRAPH_PIN_BITFIELD_H
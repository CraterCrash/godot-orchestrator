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
#ifndef ORCHESTRATOR_GRAPH_NODE_PIN_BITFIELD_H
#define ORCHESTRATOR_GRAPH_NODE_PIN_BITFIELD_H

#include "editor/graph/graph_node_pin.h"

/// Forward declaration
namespace godot
{
    class Button;
    class PopupPanel;
}

/// An implementation of OrchestratorGraphNodePin for bitfield pin types, which renders a
/// drop down multi-selection box for choices.
class OrchestratorGraphNodePinBitField : public OrchestratorGraphNodePin
{
    GDCLASS(OrchestratorGraphNodePinBitField, OrchestratorGraphNodePin);

    static void _bind_methods();

    Button* _button{ nullptr }; //! The button that shows the pop-up

protected:
    OrchestratorGraphNodePinBitField() = default;

    //~ Begin OrchestratorGraphNodePin Interface
    Control* _get_default_value_widget() override;
    //~ End OrchestratorGraphNodePin Interface

    /// Dispatched when a bitfield checkbox is toggled
    /// @param p_state true if the checkbox is toggled, false otherwise
    /// @param p_value the value to be adjusted for the bitfield
    void _on_bit_toggle(bool p_state, int64_t p_value);

    /// Dispatched when the popup panel is hidden.
    /// @param p_panel the panel being hidden
    void _on_hide_flags(PopupPanel* p_panel);

    /// Displays the flag choices for user selection
    void _on_show_flags();

public:
    OrchestratorGraphNodePinBitField(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin);
};

#endif // ORCHESTRATOR_GRAPH_NODE_PIN_BITFIELD_H
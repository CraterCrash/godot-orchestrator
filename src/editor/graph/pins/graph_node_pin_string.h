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
#ifndef ORCHESTRATOR_GRAPH_NODE_PIN_STRING_H
#define ORCHESTRATOR_GRAPH_NODE_PIN_STRING_H

#include "editor/graph/graph_node_pin.h"

namespace godot
{
    class LineEdit;
    class PopupMenu;
    class TextEdit;
}

/// An implementation of OrchestratorGraphNodePin for types that want to represent their default values
/// using a multi-line text field for data entry.
class OrchestratorGraphNodePinText : public OrchestratorGraphNodePin
{
    GDCLASS(OrchestratorGraphNodePinText, OrchestratorGraphNodePin);

    TextEdit* _editor{ nullptr };

protected:
    //~ Begin Wrapped Interface
    static void _bind_methods() {}
    //~ End Wrapped Interface

    //~ Begin OrchestratorGraphNodePin Interface
    Control* _get_default_value_widget() override;
    bool _render_default_value_below_label() const override { return true; }
    //~ End OrchestratorGraphNodePin Interface

    void _text_changed();

    // Default constructor
    OrchestratorGraphNodePinText() = default;

public:
    /// Constructs a multi-line text-based pin
    /// @param p_node the graph node that owns the pin
    /// @param p_pin the script pin
    OrchestratorGraphNodePinText(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin);
};

/// An implementation of OrchestratorGraphNodePin for types that want to represent their default values
/// using a string-based text field for data entry.
class OrchestratorGraphNodePinString : public OrchestratorGraphNodePin
{
    GDCLASS(OrchestratorGraphNodePinString, OrchestratorGraphNodePin);

    LineEdit* _editor{ nullptr };       //! Single line input widget
    PopupMenu* _popup{ nullptr };       //! Suggestions popup menu
    PackedStringArray _suggestions;     //! Context suggestions

protected:
    //~ Begin Wrapped Interface
    static void _bind_methods() {}
    //~ End Wrapped Interface

    //~ Begin OrchestratorGraphNodePin Interface
    Control* _get_default_value_widget() override;
    //~ End OrchestratorGraphNodePin Interface

    /// Called when the line edit's text submitted handler.
    /// @param p_value the text value submitted
    void _text_submitted(const String& p_value);

    /// Called when focus is gained for the line edit widget.
    void _focus_entered();

    /// Called when focus is lost on the line edit widget.
    void _focus_exited();

    /// Called when the popup suggestion menu is hidden
    void _popup_hide();

    /// Handles popup window input
    /// @param p_event the input event
    void _window_input(const Ref<InputEvent>& p_event);

    /// Handles a suggestion pick
    /// @param p_index the suggestion index
    void _suggestion_picked(int p_index);

    // Default constructor
    OrchestratorGraphNodePinString() = default;

public:
    /// Constructs a string-based pin
    /// @param p_node the graph node that owns the pin
    /// @param p_pin the script pin
    OrchestratorGraphNodePinString(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin);
};

#endif  // ORCHESTRATOR_GRAPH_NODE_PIN_STRING_H

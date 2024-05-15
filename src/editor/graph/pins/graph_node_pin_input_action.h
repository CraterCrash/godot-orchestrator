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
#ifndef ORCHESTRATOR_GRAPH_NODE_PIN_INPUT_ACTION_H
#define ORCHESTRATOR_GRAPH_NODE_PIN_INPUT_ACTION_H

#include "editor/graph/graph_node_pin.h"

#include <godot_cpp/classes/option_button.hpp>

/// Pin input widget for selecting input actions from a drop-down list
class OrchestratorGraphNodePinInputAction : public OrchestratorGraphNodePin
{
    GDCLASS(OrchestratorGraphNodePinInputAction, OrchestratorGraphNodePin);
    static void _bind_methods() {}

protected:
    OptionButton* _button{ nullptr };

    //~ Begin OrchestratorGraphNodePin Interface
    Control* _get_default_value_widget() override;
    //~ End OrchestratorGraphNodePin Interface

    /// Populates the button's action list
    void _populate_action_list();

    // Default constructor
    OrchestratorGraphNodePinInputAction() = default;

public:
    /// Constructs the input action pin
    /// @param p_node the graph node
    /// @param p_pin the script node pin
    OrchestratorGraphNodePinInputAction(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin);
};

#endif // ORCHESTRATOR_GRAPH_NODE_PIN_INPUT_ACTION_H
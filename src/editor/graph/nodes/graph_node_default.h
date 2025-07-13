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
#ifndef ORCHESTRATOR_GRAPH_NODE_DEFAULT_H
#define ORCHESTRATOR_GRAPH_NODE_DEFAULT_H

#include "editor/graph/graph_node.h"

/// Default implementation of OrchestratorGraphNode for rendering OrchestratorScript nodes
///
/// When creating an Orchestration in the editor, the user interacts with a GraphEdit implementation,
/// and this implementation uses OrchestratorGraphNode objects to represent the visual script nodes in the
/// node graph.
///
/// The default implementation creates a node using a series of rows that may optionally contain
/// either an input, an output, or both an input and output pin reference. The structure of the node
/// layout is as follows:
///
///   +------- Row -------+
///   * L |   inner   | R *
///   +-------------------+
///
/// Both the left (L) and right (R) consist of an optional pin structure that contains a pin type
/// image reference, an optional label, and default value widgets for the left (aka input) pin.
///
class OrchestratorGraphNodeDefault : public OrchestratorGraphNode
{
    GDCLASS(OrchestratorGraphNodeDefault, OrchestratorGraphNode);

    static void _bind_methods();

    struct Row
    {
        int index { 0 };
        Control* widget { nullptr };                  //! Reference to the base row widget for the row
        OrchestratorGraphNodePin* left { nullptr };   //! Reference to the left/input pin widget
        OrchestratorGraphNodePin* right { nullptr };  //! Reference to the right/output pin widget
    };

    HashMap<int, Row> _pin_rows;

protected:
    OrchestratorGraphNodeDefault() = default;

    //~ Begin OrchestratorGraphNode Interface
    void _update_pins() override;
    //~ End OrchestratorGraphNode Interface

    /// Creates the row user interface widget
    /// @param p_row the row object
    void _create_row_widget(Row& p_row);

    /// Called when a new pin row is ready in the UI.
    /// @param p_row_index the row index
    void _on_row_ready(int p_row_index);

    /// Get the previous row count before pins are updated
    int _get_previous_row_count();

public:
    OrchestratorGraphNodeDefault(OrchestratorGraphEdit* p_graph, const Ref<OScriptNode>& p_node);

    //~ Begin OrchestratorGraphNode Interface
    OrchestratorGraphNodePin* get_input_pin(int p_port) override;
    OrchestratorGraphNodePin* get_output_pin(int p_port) override;
    void update_pins(bool p_visible) override;
    Vector<OrchestratorGraphNodePin*> get_pins() const override;
    Vector<OrchestratorGraphNodePin*> get_eligible_autowire_pins(OrchestratorGraphNodePin* p_pin) const override;
    //~ End OrchestratorGraphNode Interface
};

#endif  // ORCHESTRATOR_GRAPH_NODE_DEFAULT_H

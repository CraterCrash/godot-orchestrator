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
#ifndef ORCHESTRATOR_GRAPH_NODE_PIN_ENUM_H
#define ORCHESTRATOR_GRAPH_NODE_PIN_ENUM_H

#include "editor/graph/graph_node_pin.h"

/// Forward declarations
namespace godot
{
    class OptionButton;
}

/// An implementation of OrchestratorGraphNodePin for enum pin types, which renders a
/// drop down selection box for choices.
class OrchestratorGraphNodePinEnum : public OrchestratorGraphNodePin
{
    GDCLASS(OrchestratorGraphNodePinEnum, OrchestratorGraphNodePin);

    static void _bind_methods();

    // Defines an entry that describes a single list item
    struct ListItem
    {
        String name;
        String friendly_name;
        uint64_t value;
    };

protected:
    List<ListItem> _items; //! All the items that are in the drop-down list

    /// Dispatched when the user makes a selection.
    /// @param p_index the choice index that was selected
    /// @param p_button the button widget
    void _on_item_selected(int p_index, OptionButton* p_button);

    /// Generate the list of items for the drop-down
    void _generate_items();

    /// In Godot, enums values are often prefixed, i.e. `MOUSE_BUTTON_xxxx`. This method is used to
    /// calculate the common "prefix" among the list of given enum names.
    /// @param p_names array of enum names
    /// @return the calculated prefix, or empty string if no prefixed is determined
    String _calculate_enum_prefix(const PackedStringArray& p_names);

    /// Generates a friendly enum name
    /// @param p_prefix the enum prefix
    /// @param p_enum_name the enum name to make friendly
    /// @return the friendly name
    String _generate_friendly_name(const String& p_prefix, const String& p_enum_name);

    //~ Begin OrchestratorGraphNodePin Interface
    Control* _get_default_value_widget() override;
    bool _render_default_value_below_label() const override { return true; }
    //~ End OrchestratorGraphNodePin Interface

    /// Constructor, intentionally protected
    OrchestratorGraphNodePinEnum() = default;

public:
    OrchestratorGraphNodePinEnum(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin);
};

#endif  // ORCHESTRATOR_GRAPH_NODE_PIN_ENUM_H

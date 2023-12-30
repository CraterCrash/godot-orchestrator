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
#ifndef ORCHESTRATOR_SCRIPT_EDITABLE_PIN_NODE_H
#define ORCHESTRATOR_SCRIPT_EDITABLE_PIN_NODE_H

#include "script/script.h"

/// An abstract implementation of OScriptNode that allows for editable pins
/// to be added or removed from the node.
class OScriptEditablePinNode : public OScriptNode
{
    ORCHESTRATOR_NODE_CLASS(OScriptEditablePinNode, OScriptNode);

protected:

    /// Adjust this node's connections after a pin has been removed.
    /// @param p_start_offset the slot offset to start adjustments
    /// @param p_adjustment the adjustment calculation to make, +/- values acceptable.
    /// @param p_direction the port direction to be adjusted, defaults to both.
    void _adjust_connections(int p_start_offset, int p_adjustment, EPinDirection p_direction = PD_MAX);

    /// Compute the pin name with the given index combined with the prefix.
    /// @param p_index the index
    /// @return the computed pin name in the format of {@code prefix_index}.
    String _get_pin_name_given_index(int p_index) const;

public:

    /// Get the pin name prefix, defaults to "out".
    /// @return the pin name prefix to be used
    virtual String get_pin_prefix() const { return "out"; }

    /// Called to add a pin to the node
    virtual void add_dynamic_pin() { }

    /// Checks whether the node permits adding a pin
    /// @return true if a pin can be added, false otherwise
    virtual bool can_add_dynamic_pin() const { return true; }

    /// Removes the specified input from this node
    /// @param p_pin the pin to be removed
    virtual void remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin) { }

    /// Checks whether the specified pin can be removed.
    /// @return true if the pin can be removed, false otherwise
    virtual bool can_remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin) const { return false; }
};

#endif  // ORCHESTRATOR_SCRIPT_EDITABLE_PIN_NODE_H

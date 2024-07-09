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
#ifndef ORCHESTRATOR_SCRIPT_AUTOWIRE_SELECTIONS_H
#define ORCHESTRATOR_SCRIPT_AUTOWIRE_SELECTIONS_H

#include "script/node_pin.h"

#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/tree.hpp>

using namespace godot;

/// The autowire selection dialog
class OrchestratorScriptAutowireSelections : public ConfirmationDialog
{
    GDCLASS(OrchestratorScriptAutowireSelections, ConfirmationDialog);
    static void _bind_methods();

    Ref<OScriptNodePin> _pin;     //! The source pin
    Ref<OScriptNode> _spawned;    //! The spawned node
    Ref<OScriptNodePin> _choice;  //! The selected choice
    Tree* _tree{ nullptr };       //! The tree

protected:
    //~ Begin Signal Handlers
    void _confirm_selection();
    void _select();
    //~ End Signal Handlers

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

    /// Based on eligible choices, return a list that explicitly matches pin's class type
    /// @param p_choices possible choices
    /// @return list of pins that match the pin's class type
    Vector<Ref<OScriptNodePin>> _get_choices_that_match_class(const Vector<Ref<OScriptNodePin>>& p_choices);

    /// Closes the autowire window
    void _close_window();

public:

    /// Get the node pin source
    /// @return the pin source
    Ref<OScriptNodePin> get_source() { return _pin; }

    /// Sets the node pin source
    /// @param p_source the pin source
    void set_source(const Ref<OScriptNodePin>& p_source) { _pin = p_source; }

    /// Get the spawned node
    /// @return the spawned node
    Ref<OScriptNode> get_spawned() { return _spawned; }

    /// Set the spawned node
    /// @param p_spawned the spawned node
    void set_spawned(const Ref<OScriptNode>& p_spawned) { _spawned = p_spawned; }

    /// Get the selected autowire choice
    /// @return the autowire choice
    Ref<OScriptNodePin> get_autowire_choice() { return _choice; }

    /// Show the autowire selection dialog, shows the choices for the given <code>pin</code>.
    void popup_autowire();
};

#endif // ORCHESTRATOR_SCRIPT_AUTOWIRE_SELECTIONS_H
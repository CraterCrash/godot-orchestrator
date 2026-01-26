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
#ifndef ORCHESTRATOR_AUTOWIRE_CONNECTION_DIALOG_H
#define ORCHESTRATOR_AUTOWIRE_CONNECTION_DIALOG_H

#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/tree.hpp>

using namespace godot;

/// Forward declarations
class OrchestratorEditorGraphPin;

/// Displays a dialog of details about a signal connection
class OrchestratorAutowireConnectionDialog : public ConfirmationDialog {
    GDCLASS(OrchestratorAutowireConnectionDialog, ConfirmationDialog);

    OrchestratorEditorGraphPin* _choice = nullptr;
    Tree* _tree = nullptr;

protected:
    static void _bind_methods();

    void _item_activated();
    void _item_selected();
    void _close();

public:

    OrchestratorEditorGraphPin* get_autowire_choice() const;

    void popup_autowire(const Vector<OrchestratorEditorGraphPin*>& p_choices);

    OrchestratorAutowireConnectionDialog();
};

#endif // ORCHESTRATOR_AUTOWIRE_CONNECTION_DIALOG_H
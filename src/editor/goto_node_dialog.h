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
#ifndef ORCHESTRATOR_GOTO_NODE_DIALOG_H
#define ORCHESTRATOR_GOTO_NODE_DIALOG_H

#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/confirmation_dialog.hpp>

using namespace godot;

/// A simple confirmation dialog that allows the user to specify which node to jump to.
class OrchestratorGotoNodeDialog : public ConfirmationDialog
{
    GDCLASS(OrchestratorGotoNodeDialog, ConfirmationDialog);
    static void _bind_methods();

protected:
    LineEdit* _line_edit{ nullptr };

    //~ Begin Godot Interface
    void _notification(int p_what);
    //~ End Godot Interface

    //~ Begin Signal Handlers
    void _confirmed();
    void _cancelled();
    //~ End Signal Handlers

public:

    /// Constructs the goto node dialog
    OrchestratorGotoNodeDialog();
};

#endif // ORCHESTRATOR_GOTO_NODE_DIALOG_H
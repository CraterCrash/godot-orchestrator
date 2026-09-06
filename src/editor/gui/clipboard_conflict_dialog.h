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
#pragma once

#include "editor/graph/graph_clipboard.h"

#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/tree.hpp>

using namespace godot;

/// Asks the user how to paste declarations that already exist in the target script with a different
/// definition. Each conflict can be pasted under a unique name or skipped; cancel aborts the paste.
class OrchestratorEditorClipboardConflictDialog : public ConfirmationDialog {
    GDCLASS(OrchestratorEditorClipboardConflictDialog, ConfirmationDialog);

    using Conflict = OrchestratorEditorGraphClipboard::Conflict;
    using Resolution = OrchestratorEditorGraphClipboard::Resolution;

    enum Column {
        COLUMN_KIND,
        COLUMN_NAME,
        COLUMN_SOURCE,
        COLUMN_TARGET,
        COLUMN_RENAME,   //! Checked pastes under a unique name, unchecked skips
        COLUMN_MAX
    };

    Tree* _tree = nullptr;

    static String _get_kind_name(Conflict::Kind p_kind);

protected:
    static void _bind_methods();

public:
    Vector<Resolution> get_resolutions() const;
    void popup_conflicts(const Vector<Conflict>& p_conflicts, const Callable& p_confirmed);

    OrchestratorEditorClipboardConflictDialog();
};
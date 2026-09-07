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
#include "editor/gui/clipboard_conflict_dialog.h"

#include "common/macros.h"
#include "common/scene_utils.h"
#include "core/godot/scene_string_names.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/tree_item.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

String OrchestratorEditorClipboardConflictDialog::_get_kind_name(Conflict::Kind p_kind) {
    switch (p_kind) {
        case Conflict::FUNCTION:
            return "Function";
        case Conflict::VARIABLE:
            return "Variable";
        case Conflict::SIGNAL:
            return "Signal";
        case Conflict::LOCAL_VARIABLE:
            return "Local Variable";
    }
    return String();
}

void OrchestratorEditorClipboardConflictDialog::_bind_methods() {
}

Vector<OrchestratorEditorClipboardConflictDialog::Resolution> OrchestratorEditorClipboardConflictDialog::get_resolutions() const {
    Vector<Resolution> resolutions;

    TreeItem* root = _tree->get_root();
    if (!root) {
        return resolutions;
    }

    for (TreeItem* item = root->get_first_child(); item; item = item->get_next()) {
        Resolution resolution;
        resolution.kind = static_cast<Conflict::Kind>(static_cast<int>(item->get_meta("__kind")));
        resolution.name = item->get_meta("__name");
        resolution.rename = item->is_checked(COLUMN_RENAME);
        resolutions.push_back(resolution);
    }

    return resolutions;
}

void OrchestratorEditorClipboardConflictDialog::popup_conflicts(const Vector<Conflict>& p_conflicts, const Callable& p_confirmed) {
    _tree->clear();

    TreeItem* root = _tree->create_item();
    for (const Conflict& conflict : p_conflicts) {
        TreeItem* item = _tree->create_item(root);
        item->set_text(COLUMN_KIND, _get_kind_name(conflict.kind));
        item->set_text(COLUMN_NAME, conflict.name);
        item->set_text(COLUMN_SOURCE, conflict.source);
        item->set_text(COLUMN_TARGET, conflict.target);

        // A check reads at a glance: on pastes under a unique name, off skips the item
        item->set_cell_mode(COLUMN_RENAME, TreeItem::CELL_MODE_CHECK);
        item->set_checked(COLUMN_RENAME, true);
        item->set_editable(COLUMN_RENAME, true);

        item->set_meta("__kind", conflict.kind);
        item->set_meta("__name", conflict.name);
    }

    if (p_confirmed.is_valid()) {
        connect(SceneStringName(confirmed), p_confirmed);
    }

    // Connected after the caller so the caller reads the choices before the dialog goes away
    connect(SceneStringName(confirmed), callable_mp_cast(this, Node, queue_free));
    connect(SceneStringName(canceled), callable_mp_cast(this, Node, queue_free));

    EI->popup_dialog_centered(this, Size2(820, 360) * EDSCALE);
}

OrchestratorEditorClipboardConflictDialog::OrchestratorEditorClipboardConflictDialog() {
    set_title("Paste Conflicts");
    set_ok_button_text("Paste");

    VBoxContainer* container = memnew(VBoxContainer);
    add_child(container);

    Label* label = memnew(Label);
    label->set_text("These items already exist in the script with a different definition.\n"
                    "Checked items are pasted under a new name, unchecked items are skipped.");
    container->add_child(label);

    _tree = memnew(Tree);
    SceneUtils::set_theme_type_variation(_tree, "Tree");
    _tree->set_columns(COLUMN_MAX);
    _tree->set_column_titles_visible(true);
    _tree->set_column_title(COLUMN_KIND, "Type");
    _tree->set_column_title(COLUMN_NAME, "Name");
    _tree->set_column_title(COLUMN_SOURCE, "In Clipboard");
    _tree->set_column_title(COLUMN_TARGET, "In Script");
    _tree->set_column_title(COLUMN_RENAME, "Paste as New Name");

    // Every column expands, weighted by how long its content runs; minimums keep short columns readable
    _tree->set_column_expand_ratio(COLUMN_KIND, 1);
    _tree->set_column_expand_ratio(COLUMN_NAME, 2);
    _tree->set_column_expand_ratio(COLUMN_SOURCE, 3);
    _tree->set_column_expand_ratio(COLUMN_TARGET, 3);
    _tree->set_column_expand_ratio(COLUMN_RENAME, 1);
    _tree->set_column_custom_minimum_width(COLUMN_KIND, 90 * EDSCALE);
    _tree->set_column_custom_minimum_width(COLUMN_NAME, 120 * EDSCALE);
    _tree->set_column_custom_minimum_width(COLUMN_RENAME, 150 * EDSCALE);

    _tree->set_hide_root(true);
    _tree->set_select_mode(Tree::SELECT_ROW);
    _tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    container->add_child(_tree);
}
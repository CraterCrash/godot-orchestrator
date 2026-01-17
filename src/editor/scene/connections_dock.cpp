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
#include "editor/scene/connections_dock.h"

#include "common/macros.h"
#include "common/scene_utils.h"
#include "core/godot/core_string_names.h"
#include "core/godot/scene_string_names.h"
#include "editor/editor.h"
#include "script/script.h"

#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/classes/window.hpp>

OrchestratorEditorConnectionsDock* OrchestratorEditorConnectionsDock::_singleton = nullptr;

void OrchestratorEditorConnectionsDock::_slot_menu_option(int p_option) {
    switch (p_option) {
        case SLOT_MENU_GO_TO_METHOD: {
            TreeItem* selected = _connections_tree->get_selected();
            if (selected) {
                _go_to_method(selected);
            }
            break;
        }
        case SLOT_MENU_DISCONNECT: {
            _notify_connections_dock_changed();
            break;
        }
        default: {
            break;
        }
    }
}

void OrchestratorEditorConnectionsDock::_go_to_method(TreeItem* p_item) {
    ERR_FAIL_NULL(p_item);

    const Dictionary connection = p_item->get_metadata(0);
    const Signal& signal = connection["signal"];
    const Callable& callable = connection["callable"];

    Node* object = cast_to<Node>(ObjectDB::get_instance(signal.get_object_id()));
    if (!object) {
        return;
    }

    const Ref<OScript> script = object->get_script();
    if (!script.is_valid() || !object->has_method(callable.get_method())) {
        return;
    }

    const Ref<OScriptFunction> function = script->get_orchestration()->find_function(callable.get_method());
    if (!function.is_valid()) {
        return;
    }

    OrchestratorEditor::get_singleton()->edit(script, function->get_owning_node_id());

    // When Orchestrator edits a script and a node is focused, the inspector is changed.
    // In this case, we want to explicitly reset the focused inspect object on the node so that the UI
    // does not clear the signals connection dock.
    EI->inspect_object(object);
}

void OrchestratorEditorConnectionsDock::_notify_connections_dock_changed() {
    emit_signal(CoreStringName(changed));
}

bool OrchestratorEditorConnectionsDock::disconnect_slot(const Ref<Script>& p_script, const StringName& p_method) {
    bool result = false;

    for (Node* node : SceneUtils::find_all_nodes_for_script_in_edited_scene(p_script)) {
        for (const Variant& value : node->get_incoming_connections()) {
            const Dictionary& connection = value;

            const Callable& callable = connection["callable"];
            if (callable.get_method() != p_method) {
                continue;
            }

            const Signal& signal = connection["signal"];
            if (Node* source = cast_to<Node>(ObjectDB::get_instance(signal.get_object_id()))) {
                source->disconnect(signal.get_name(), callable);

                _connections_dock->call("update_tree");
                _scene_tree_editor->call("update_tree");

                result = true;
                break;
            }
        }
    }

    return result;
}

void OrchestratorEditorConnectionsDock::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_READY: {
            if (Node* editor_node = EditorNode) {
                _scene_tree_editor = editor_node->find_child("*SceneTreeEditor*", true, false);

                _connections_dock = editor_node->find_child("Signals", true, false);
                if (_connections_dock) {
                    const TypedArray<Node> tree = _connections_dock->find_children(
                        "*", Tree::get_class_static(), false, false);
                    if (tree.size() == 1) {
                        _connections_tree = cast_to<Tree>(tree[0]);
                    }

                    const TypedArray<Node> dialogs = _connections_dock->find_children(
                        "*", ConfirmationDialog::get_class_static(), false, false);
                    if (dialogs.size() >= 2) {
                        // The second one is the ConfirmationDialog that is for handling whether the user
                        // really wants to process the "Disconnect All Connections" menu choice.
                        if (ConfirmationDialog* dialog = cast_to<ConfirmationDialog>(dialogs[1])) {
                            dialog->connect(SceneStringName(confirmed), callable_mp_this(_notify_connections_dock_changed));
                        }
                    }

                    const TypedArray<Node> menus = _connections_dock->find_children(
                        "*", PopupMenu::get_class_static(), false, false);
                    if (menus.size() >= 3) {
                        // The third PopupMenu is what we are interested in.
                        if (PopupMenu* menu = cast_to<PopupMenu>(menus[2])) {
                            menu->connect(SceneStringName(id_pressed), callable_mp_this(_slot_menu_option));
                        }
                    }
                }
            }
            break;
        }
        default: {
            break;
        }
    }
}

void OrchestratorEditorConnectionsDock::_bind_methods() {
    ADD_SIGNAL(MethodInfo(CoreStringName(changed)));
}

OrchestratorEditorConnectionsDock::OrchestratorEditorConnectionsDock() {
    _singleton = this;
}

OrchestratorEditorConnectionsDock::~OrchestratorEditorConnectionsDock() {
    _singleton = nullptr;
}
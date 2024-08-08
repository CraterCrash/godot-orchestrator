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
#include "common/scene_utils.h"
#include "editor/script_connections.h"

#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/templates/vector.hpp>

void OrchestratorScriptConnectionsDialog::_notification(int p_what)
{
    if (p_what == NOTIFICATION_READY)
    {
        set_title("Connections to method:");

        VBoxContainer* vbox = memnew(VBoxContainer);
        vbox->set_anchor_and_offset(SIDE_LEFT, Control::ANCHOR_BEGIN, 8);
        vbox->set_anchor_and_offset(SIDE_TOP, Control::ANCHOR_BEGIN, 8);
        vbox->set_anchor_and_offset(SIDE_RIGHT, Control::ANCHOR_END, -8);
        vbox->set_anchor_and_offset(SIDE_BOTTOM, Control::ANCHOR_END, -8);
        add_child(vbox);

        _method = memnew(Label);
        _method->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
        vbox->add_child(_method);

        _tree = memnew(Tree);
        _tree->set_columns(3);
        _tree->set_hide_root(true);
        _tree->set_column_titles_visible(true);
        _tree->set_column_title(0, "Source");
        _tree->set_column_title(1, "Signal");
        _tree->set_column_title(2, "Target");
        _tree->set_column_title_alignment(0, HORIZONTAL_ALIGNMENT_LEFT);
        _tree->set_column_title_alignment(1, HORIZONTAL_ALIGNMENT_LEFT);
        _tree->set_column_title_alignment(2, HORIZONTAL_ALIGNMENT_LEFT);
        vbox->add_child(_tree);
        _tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);
        _tree->set_allow_rmb_select(true);

        connect("confirmed", callable_mp(this, &OrchestratorScriptConnectionsDialog::_on_confirmed));
    }
}

void OrchestratorScriptConnectionsDialog::_on_confirmed()
{
    queue_free();
}

void OrchestratorScriptConnectionsDialog::popup_connections(const String& p_method, const Vector<Node*>& p_nodes)
{
    _method->set_text(p_method);
    _tree->clear();

    TreeItem* root = _tree->create_item();
    for (int i = 0; i < p_nodes.size(); i++)
    {
        Node* node = p_nodes[i];
        TypedArray<Dictionary> incoming_connections = node->get_incoming_connections();
        for (int j = 0; j < incoming_connections.size(); ++j)
        {
            const Dictionary& connection = incoming_connections[j];
            const Signal signal = connection["signal"];

            const Callable& callable = connection["callable"];
            if (callable.get_method() != p_method)
                 continue;

            Node* source = Object::cast_to<Node>(ObjectDB::get_instance(signal.get_object_id()));
            TreeItem* node_item = _tree->create_item(root);
            node_item->set_text(0, source->get_name());
            node_item->set_icon(0, SceneUtils::get_editor_icon(source->get_class()));
            node_item->set_selectable(0, false);
            node_item->set_editable(0, false);

            node_item->set_text(1, signal.get_name());
            node_item->set_icon(1, SceneUtils::get_editor_icon("Slot"));
            node_item->set_selectable(1, false);
            node_item->set_editable(1, false);

            Node* callable_node = Object::cast_to<Node>(ObjectDB::get_instance(callable.get_object_id()));
            node_item->set_text(2, callable_node->get_name());
            node_item->set_icon(2, SceneUtils::get_editor_icon(callable_node->get_class()));
            node_item->set_selectable(2, false);
            node_item->set_editable(2, false);
        }
    }

    popup_centered(Vector2(700, 250));
}

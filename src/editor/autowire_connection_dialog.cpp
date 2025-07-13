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
#include "editor/autowire_connection_dialog.h"

#include "common/macros.h"
#include "common/property_utils.h"
#include "common/scene_utils.h"
#include "editor/graph/graph_node_pin.h"
#include "script/nodes/math/operator_node.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

void OrchestratorAutowireConnectionDialog::_item_activated()
{
    TreeItem* item = _tree->get_selected();
    if (item)
        _close();
}

void OrchestratorAutowireConnectionDialog::_item_selected()
{
    TreeItem* item = _tree->get_selected();
    if (item)
    {
        OrchestratorGraphNodePin* pin = cast_to<OrchestratorGraphNodePin>(item->get_meta("__pin"));
        _choice = pin;

        get_ok_button()->set_disabled(_choice == nullptr);
    }
}

void OrchestratorAutowireConnectionDialog::_close()
{
    get_ok_button()->call_deferred("emit_signal", "pressed");
}

OrchestratorGraphNodePin* OrchestratorAutowireConnectionDialog::get_autowire_choice() const
{
    return _choice;
}

void OrchestratorAutowireConnectionDialog::popup_autowire(const Vector<OrchestratorGraphNodePin*>& p_choices)
{
    // At this point, no automatic choice was made, so populate the tree/dialog
    get_ok_button()->set_disabled(true);
    _tree->clear();

    TreeItem* root = _tree->create_item();
    for (OrchestratorGraphNodePin* choice : p_choices)
    {
        const String pin_type_name = PropertyUtils::get_property_type_name(choice->get_property_info());

        TreeItem* item = _tree->create_item(root);
        item->set_text(0, choice->get_pin_name());
        item->set_icon(0, SceneUtils::get_editor_icon(pin_type_name));
        item->set_selectable(0, true);
        item->set_editable(0, false);
        item->set_meta("__pin", choice);
    }

    EI->popup_dialog_centered_ratio(this, 0.4);
}

void OrchestratorAutowireConnectionDialog::_bind_methods()
{
}

OrchestratorAutowireConnectionDialog::OrchestratorAutowireConnectionDialog()
{
    set_title("Possible autowire pins:");
    set_ok_button_text("Autowire");
    set_cancel_button_text("Skip");

    VBoxContainer* vbox = memnew(VBoxContainer);
    vbox->set_anchor_and_offset(SIDE_LEFT, Control::ANCHOR_BEGIN, 8);
    vbox->set_anchor_and_offset(SIDE_TOP, Control::ANCHOR_BEGIN, 8);
    vbox->set_anchor_and_offset(SIDE_RIGHT, Control::ANCHOR_END, -8);
    vbox->set_anchor_and_offset(SIDE_BOTTOM, Control::ANCHOR_END, -8);
    add_child(vbox);

    _tree = memnew(Tree);
    _tree->set_columns(1);
    _tree->set_hide_root(true);
    _tree->set_column_titles_visible(true);
    _tree->set_column_title(0, "Pin Name");
    _tree->set_column_title_alignment(0, HORIZONTAL_ALIGNMENT_CENTER);
    _tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    _tree->set_allow_rmb_select(true);
    vbox->add_child(_tree);

    _tree->connect("item_activated", callable_mp(this, &OrchestratorAutowireConnectionDialog::_item_activated));
    _tree->connect("item_selected", callable_mp(this, &OrchestratorAutowireConnectionDialog::_item_selected));

    connect("confirmed", callable_mp(static_cast<Node*>(this), &Node::queue_free));
    connect("canceled", callable_mp(static_cast<Node*>(this), &Node::queue_free));
}
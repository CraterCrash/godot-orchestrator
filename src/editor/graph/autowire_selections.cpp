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
#include "editor/graph/autowire_selections.h"

#include "common/scene_utils.h"
#include "common/settings.h"
#include "script/node.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

void OrchestratorScriptAutowireSelections::_confirm_selection()
{
    TreeItem* selected = _tree->get_selected();
    if (selected)
        get_ok_button()->emit_signal("pressed");
}

void OrchestratorScriptAutowireSelections::_select()
{
    TreeItem* selected = _tree->get_selected();
    if (selected)
    {
        get_ok_button()->set_disabled(false);
        const Ref<OScriptNodePin> pin  = selected->get_meta("__pin");
        if (pin.is_valid())
            _choice = pin;
    }
}

void OrchestratorScriptAutowireSelections::_notification(int p_what)
{
    if (p_what == NOTIFICATION_READY)
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

        _tree->connect("item_activated", callable_mp(this, &OrchestratorScriptAutowireSelections::_confirm_selection));
        _tree->connect("item_selected", callable_mp(this, &OrchestratorScriptAutowireSelections::_select));
    }
}

void OrchestratorScriptAutowireSelections::popup_autowire()
{
    get_ok_button()->set_disabled(true);

    _tree->clear();

    const Vector<Ref<OScriptNodePin>> choices = _spawned->get_eligible_autowire_pins(_pin);
    if (choices.size() <= 1)
    {
        if (choices.size() == 1)
            _choice = choices[0];

        get_ok_button()->call_deferred("emit_signal", "pressed");
        return;
    }

    // If the autowire selection dialog is disabled, then just return
    OrchestratorSettings* settings = OrchestratorSettings::get_singleton();
    if (!settings->get_setting("ui/graph/show_autowire_selection_dialog", true))
    {
        get_ok_button()->call_deferred("emit_signal", "pressed");
        return;
    }

    TreeItem* root = _tree->create_item();
    for (int i = 0; i < choices.size(); i++)
    {
        const Ref<OScriptNodePin>& choice = choices[i];

        TreeItem* item = _tree->create_item(root);
        item->set_text(0, choice->get_pin_name());
        item->set_icon(0, SceneUtils::get_editor_icon(choice->get_pin_type_name()));
        item->set_selectable(0, true);
        item->set_editable(0, false);
        item->set_meta("__pin", choice);
    }

    popup_centered_ratio(0.3);
}

void OrchestratorScriptAutowireSelections::_bind_methods()
{
}

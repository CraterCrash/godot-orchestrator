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
#include "editor/component_panels/signals_panel.h"

#include "common/dictionary_utils.h"
#include "common/scene_utils.h"
#include "common/settings.h"
#include "editor/plugins/orchestrator_editor_plugin.h"

#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/tree.hpp>

PackedStringArray OrchestratorScriptSignalsComponentPanel::_get_existing_names() const
{
    return _orchestration->get_custom_signal_names();
}

String OrchestratorScriptSignalsComponentPanel::_get_tooltip_text() const
{
    return "A signal is used to send a notification synchronously to any number of observers that have "
           "connected to the defined signal on the orchestration. Signals allow for a variable number "
           "of arguments to be passed to the observer.\n\n"
           "Selecting a signal in the component view displays the signal details in the inspector.";
}

String OrchestratorScriptSignalsComponentPanel::_get_remove_confirm_text(TreeItem* p_item) const
{
    return "Removing a signal will remove all nodes that emit the signal.";
}

bool OrchestratorScriptSignalsComponentPanel::_populate_context_menu(TreeItem* p_item)
{
    _context_menu->add_icon_item(SceneUtils::get_editor_icon("Rename"), "Rename", CM_RENAME_SIGNAL);
    _context_menu->add_icon_item(SceneUtils::get_editor_icon("Remove"), "Remove", CM_REMOVE_SIGNAL);
    return true;
}

void OrchestratorScriptSignalsComponentPanel::_handle_context_menu(int p_id)
{
    switch (p_id)
    {
        case CM_RENAME_SIGNAL:
            _edit_selected_tree_item();
            break;
        case CM_REMOVE_SIGNAL:
            _confirm_removal(_tree->get_selected());
            break;
    }
}

bool OrchestratorScriptSignalsComponentPanel::_handle_add_new_item(const String& p_name)
{
    // Add the new signal and update the components display
    return _orchestration->create_custom_signal(p_name).is_valid();
}

void OrchestratorScriptSignalsComponentPanel::_handle_item_selected()
{
    TreeItem* item = _tree->get_selected();

    Ref<OScriptSignal> signal = _orchestration->get_custom_signal(_get_tree_item_name(item));
    OrchestratorPlugin::get_singleton()->get_editor_interface()->edit_resource(signal);
}

void OrchestratorScriptSignalsComponentPanel::_handle_item_activated(TreeItem* p_item)
{
    Ref<OScriptSignal> signal = _orchestration->get_custom_signal(_get_tree_item_name(p_item));
    OrchestratorPlugin::get_singleton()->get_editor_interface()->edit_resource(signal);
}

bool OrchestratorScriptSignalsComponentPanel::_can_be_renamed(const String& p_old_name, const String& p_new_name)
{
    if (_get_existing_names().has(p_new_name))
    {
        _show_notification("A signal with the name '" + p_new_name + "' already exists.");
        return false;
    }
    return true;
}

void OrchestratorScriptSignalsComponentPanel::_handle_item_renamed(const String& p_old_name, const String& p_new_name)
{
    _orchestration->rename_custom_user_signal(p_old_name, p_new_name);
}

void OrchestratorScriptSignalsComponentPanel::_handle_remove(TreeItem* p_item)
{
    _orchestration->remove_custom_signal(_get_tree_item_name(p_item));
}

Dictionary OrchestratorScriptSignalsComponentPanel::_handle_drag_data(const Vector2& p_position)
{
    Dictionary data;

    TreeItem* selected = _tree->get_selected();
    if (selected)
    {
        Ref<OScriptSignal> signal = _orchestration->find_custom_signal(StringName(_get_tree_item_name(selected)));
        if (signal.is_valid())
        {
            data["type"] = "signal";
            data["signals"] = DictionaryUtils::from_method(signal->get_method_info());
        }
    }
    return data;
}

void OrchestratorScriptSignalsComponentPanel::update()
{
    _clear_tree();

    PackedStringArray signal_names = _orchestration->get_custom_signal_names();
    if (!signal_names.is_empty())
    {
        signal_names.sort();
        for (const String& signal_name : signal_names)
        {
            Ref<OScriptSignal> signal = _orchestration->get_custom_signal(signal_name);

            _create_item(_tree->get_root(), signal_name, signal_name, "MemberSignal");
        }
    }

    if (_tree->get_root()->get_child_count() == 0)
    {
        TreeItem* item = _tree->get_root()->create_child();
        item->set_text(0, "No signals defined");
        item->set_selectable(0, false);
        return;
    }

    OrchestratorScriptComponentPanel::update();
}

void OrchestratorScriptSignalsComponentPanel::_bind_methods()
{
}

OrchestratorScriptSignalsComponentPanel::OrchestratorScriptSignalsComponentPanel(Orchestration* p_orchestration)
    : OrchestratorScriptComponentPanel("Signals", p_orchestration)
{
}

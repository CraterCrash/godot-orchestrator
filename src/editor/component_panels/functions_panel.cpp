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
#include "editor/component_panels/functions_panel.h"

#include "common/callable_lambda.h"
#include "common/dictionary_utils.h"
#include "common/scene_utils.h"
#include "common/settings.h"
#include "editor/script_connections.h"
#include "script/script.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/shortcut.hpp>
#include <godot_cpp/classes/tree.hpp>

void OrchestratorScriptFunctionsComponentPanel::_show_function_graph(TreeItem* p_item)
{
    // Function name and graph names are synonymous
    const String function_name = _get_tree_item_name(p_item);
    emit_signal("show_graph_requested", function_name);
    emit_signal("focus_node_requested", function_name, _orchestration->get_function_node_id(function_name));
    _tree->deselect_all();
}

void OrchestratorScriptFunctionsComponentPanel::_update_slots()
{
    if (_orchestration->get_type() != OrchestrationType::OT_Script)
        return;

    const Ref<OScript> script = _orchestration->get_self();
    const Vector<Node*> script_nodes = SceneUtils::find_all_nodes_for_script_in_edited_scene(script);
    const String base_type = script->get_instance_base_type();

    _iterate_tree_items(callable_mp_lambda(this, [&](TreeItem* item) {
        if (item->has_meta("__name"))
        {
            Ref<OScriptGraph> graph = _orchestration->get_graph(item->get_meta("__name"));
            if (graph.is_valid() && graph->get_flags().has_flag(OScriptGraph::GraphFlags::GF_FUNCTION))
            {
                const String function_name = item->get_meta("__name");
                if (SceneUtils::has_any_signals_connected_to_function(function_name, base_type, script_nodes))
                {
                    if (item->get_button_count(0) == 0)
                    {
                        item->add_button(0, SceneUtils::get_editor_icon("Slot"));
                        item->set_meta("__slot", true);
                    }
                }
                else if (item->get_button_count(0) > 0)
                {
                    item->erase_button(0, 0);
                    item->remove_meta("__slot");
                }
            }
        }
    }));
}

PackedStringArray OrchestratorScriptFunctionsComponentPanel::_get_existing_names() const
{
    return _orchestration->get_function_names();
}

String OrchestratorScriptFunctionsComponentPanel::_get_tooltip_text() const
{
    return "A function graph allows the encapsulation of functionality for re-use. Function graphs have "
           "a single input with an optional output node. Function graphs have a single execution pin "
           "with multiple input data pins and the result node may return a maximum of one data value to "
           "the caller.\n\n"
           "Functions can be called by selecting the action in the action menu or by dragging the "
           "function from this component view onto the graph area.";
}

String OrchestratorScriptFunctionsComponentPanel::_get_remove_confirm_text(TreeItem* p_item) const
{
    return "Removing a function removes all nodes that participate in the function and any nodes\n"
           "that call that function from the event graphs.";
}

bool OrchestratorScriptFunctionsComponentPanel::_populate_context_menu(TreeItem* p_item)
{
    _context_menu->add_item("Open in Graph", CM_OPEN_FUNCTION_GRAPH, KEY_ENTER);
    _context_menu->add_icon_item(SceneUtils::get_editor_icon("Rename"), "Rename", CM_RENAME_FUNCTION, KEY_F2);
    _context_menu->add_icon_item(SceneUtils::get_editor_icon("Remove"), "Remove", CM_REMOVE_FUNCTION, KEY_DELETE);

    if (p_item->has_meta("__slot") && p_item->get_meta("__slot"))
    {
        _context_menu->add_icon_item(SceneUtils::get_editor_icon("Unlinked"), "Disconnect", CM_DISCONNECT_SLOT);
        _context_menu->set_item_tooltip(_context_menu->get_item_index(CM_DISCONNECT_SLOT), "Disconnect the slot function from the signal.");
    }

    return true;
}

void OrchestratorScriptFunctionsComponentPanel::_handle_context_menu(int p_id)
{
    switch (p_id)
    {
        case CM_OPEN_FUNCTION_GRAPH:
            _show_function_graph(_tree->get_selected());
            break;
        case CM_RENAME_FUNCTION:
            _edit_selected_tree_item();
            break;
        case CM_REMOVE_FUNCTION:
            _confirm_removal(_tree->get_selected());
            break;
        case CM_DISCONNECT_SLOT:
            _disconnect_slot(_tree->get_selected());
            break;
    }
}

bool OrchestratorScriptFunctionsComponentPanel::_handle_add_new_item(const String& p_name)
{
    if (_new_function_callback.is_valid())
    {
        const Ref<OScriptFunction> result = _new_function_callback.call(p_name, false);
        return result.is_valid();
    }
    return false;
}

void OrchestratorScriptFunctionsComponentPanel::_handle_item_selected()
{
    TreeItem* item = _tree->get_selected();
    if (item)
    {
        const Ref<OScriptFunction> function = _orchestration->find_function(StringName(_get_tree_item_name(item)));
        if (function.is_valid())
        {
            const Ref<OScriptNode> node = function->get_owning_node();
            if (node.is_valid())
                EditorInterface::get_singleton()->edit_resource(node);
        }
    }
}

void OrchestratorScriptFunctionsComponentPanel::_handle_item_activated(TreeItem* p_item)
{
    _show_function_graph(p_item);
}

bool OrchestratorScriptFunctionsComponentPanel::_handle_item_renamed(const String& p_old_name, const String& p_new_name)
{
    if (_get_existing_names().has(p_new_name))
    {
        _show_notification("A function with the name '" + p_new_name + "' already exists.");
        return false;
    }

    if (!p_new_name.is_valid_identifier())
    {
        _show_invalid_name("function");
        return false;
    }

    if (!_orchestration->rename_function(p_old_name, p_new_name))
        return false;

    emit_signal("graph_renamed", p_old_name, p_new_name);
    return true;
}

void OrchestratorScriptFunctionsComponentPanel::_handle_remove(TreeItem* p_item)
{
    // Function name and graph names are synonymous
    const String function_name = _get_tree_item_name(p_item);
    emit_signal("close_graph_requested", function_name);

    _orchestration->remove_function(function_name);
}

void OrchestratorScriptFunctionsComponentPanel::_handle_button_clicked(TreeItem* p_item, int p_column, int p_id,
                                                                    int p_mouse_button)
{
    if (_orchestration->get_type() != OrchestrationType::OT_Script)
        return;

    const Ref<OScript> script = _orchestration->get_self();
    const Vector<Node*> nodes = SceneUtils::find_all_nodes_for_script_in_edited_scene(script);

    OrchestratorScriptConnectionsDialog* dialog = memnew(OrchestratorScriptConnectionsDialog);
    add_child(dialog);
    dialog->popup_connections(_get_tree_item_name(p_item), nodes);
}

Dictionary OrchestratorScriptFunctionsComponentPanel::_handle_drag_data(const Vector2& p_position)
{
    Dictionary data;

    TreeItem* selected = _tree->get_selected();
    if (selected)
    {
        Ref<OScriptFunction> function = _orchestration->find_function(StringName(_get_tree_item_name(selected)));
        if (function.is_valid())
        {
            data["type"] = "function";
            data["functions"] = DictionaryUtils::from_method(function->get_method_info());
        }
    }
    return data;
}

void OrchestratorScriptFunctionsComponentPanel::_handle_tree_gui_input(const Ref<InputEvent>& p_event, TreeItem* p_item)
{
    const Ref<InputEventKey> key = p_event;
    if (key.is_valid() && key->is_pressed() && !key->is_echo())
    {
        if (key->get_keycode() == KEY_ENTER)
        {
            _handle_context_menu(CM_OPEN_FUNCTION_GRAPH);
            accept_event();
        }
        else if (key->get_keycode() == KEY_F2)
        {
            _handle_context_menu(CM_RENAME_FUNCTION);
            accept_event();
        }
        else if (key->get_keycode() == KEY_DELETE)
        {
            _handle_context_menu(CM_REMOVE_FUNCTION);
            accept_event();
        }
    }
}

void OrchestratorScriptFunctionsComponentPanel::update()
{
    if (_update_blocked)
        return;

    _clear_tree();

    OrchestratorSettings* settings = OrchestratorSettings::get_singleton();
    bool use_friendly_names = settings->get_setting("ui/components_panel/show_function_friendly_names", true);

    for (const Ref<OScriptGraph>& graph : _orchestration->get_graphs())
    {
        if (!(graph->get_flags().has_flag(OScriptGraph::GraphFlags::GF_FUNCTION)))
            continue;

        String friendly_name = graph->get_graph_name();
        if (use_friendly_names)
            friendly_name = graph->get_graph_name().capitalize();

        _create_item(_tree->get_root(), friendly_name, graph->get_graph_name(), "MemberMethod");
    }

    if (_tree->get_root()->get_child_count() == 0)
    {
        TreeItem* item = _tree->get_root()->create_child();
        item->set_text(0, "No functions defined");
        item->set_selectable(0, false);
        return;
    }

    _update_slots();

    OrchestratorScriptComponentPanel::update();
}

void OrchestratorScriptFunctionsComponentPanel::_notification(int p_what)
{
    #if GODOT_VERSION < 0x040300
    // Godot does not dispatch to parent (shrugs)
    OrchestratorScriptComponentPanel::_notification(p_what);
    #endif

    if (p_what == NOTIFICATION_READY)
    {
        _slot_update_timer = memnew(Timer);
        _slot_update_timer->set_wait_time(1);
        _slot_update_timer->set_autostart(true);
        _slot_update_timer->connect("timeout", callable_mp(this, &OrchestratorScriptFunctionsComponentPanel::_update_slots));
        add_child(_slot_update_timer);

        HBoxContainer* container = _get_panel_hbox();

        _override_button = memnew(Button);
        _override_button->set_focus_mode(FOCUS_NONE);
        _override_button->set_button_icon(SceneUtils::get_editor_icon("Override"));
        _override_button->set_tooltip_text("Override a Godot virtual function");
        container->add_child(_override_button);

        _override_button->connect("pressed", callable_mp_lambda(this, [=, this] { emit_signal("override_function_requested"); }));
    }
    else if (p_what == NOTIFICATION_THEME_CHANGED)
    {
        if (_override_button)
            _override_button->set_button_icon(SceneUtils::get_editor_icon("Override"));
    }
}

void OrchestratorScriptFunctionsComponentPanel::_bind_methods()
{
    ADD_SIGNAL(MethodInfo("show_graph_requested", PropertyInfo(Variant::STRING, "graph_name")));
    ADD_SIGNAL(MethodInfo("close_graph_requested", PropertyInfo(Variant::STRING, "graph_name")));
    ADD_SIGNAL(MethodInfo("graph_renamed", PropertyInfo(Variant::STRING, "old_name"), PropertyInfo(Variant::STRING, "new_name")));
    ADD_SIGNAL(MethodInfo("focus_node_requested", PropertyInfo(Variant::STRING, "graph_name"), PropertyInfo(Variant::INT, "node_id")));
    ADD_SIGNAL(MethodInfo("override_function_requested"));
}

OrchestratorScriptFunctionsComponentPanel::OrchestratorScriptFunctionsComponentPanel(Orchestration* p_orchestration, Callable p_new_function_callback)
    : OrchestratorScriptComponentPanel("Functions", p_orchestration), _new_function_callback(p_new_function_callback)
{
}

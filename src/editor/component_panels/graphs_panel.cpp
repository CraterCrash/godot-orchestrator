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
#include "editor/component_panels/graphs_panel.h"

#include "common/callable_lambda.h"
#include "common/scene_utils.h"
#include "common/settings.h"
#include "editor/script_connections.h"

#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/tree.hpp>

void OrchestratorScriptGraphsComponentPanel::_show_graph_item(TreeItem* p_item)
{
    // Graph
    emit_signal("show_graph_requested", _get_tree_item_name(p_item));
    _tree->deselect_all();
}

void OrchestratorScriptGraphsComponentPanel::_focus_graph_function(TreeItem* p_item)
{
    const int node_id = _orchestration->get_function_node_id(_get_tree_item_name(p_item));

    // Specific event node
    emit_signal("focus_node_requested", _get_tree_item_name(p_item->get_parent()), node_id);
    _tree->deselect_all();
}

void OrchestratorScriptGraphsComponentPanel::_remove_graph(TreeItem* p_item)
{
    const String graph_name = _get_tree_item_name(p_item);
    emit_signal("close_graph_requested", graph_name);

    _orchestration->remove_graph(graph_name);
}

void OrchestratorScriptGraphsComponentPanel::_remove_graph_function(TreeItem* p_item)
{
    _orchestration->remove_function(_get_tree_item_name(p_item));
    update();
}

PackedStringArray OrchestratorScriptGraphsComponentPanel::_get_existing_names() const
{
    PackedStringArray result;
    for (const Ref<OScriptGraph>& graph : _orchestration->get_graphs())
        result.push_back(graph->get_graph_name());
    return result;
}

String OrchestratorScriptGraphsComponentPanel::_get_tooltip_text() const
{
    return "A graph allows you to place many types of nodes to create various behaviors. "
           "Event graphs are flexible and can control multiple event nodes that start execution, "
           "nodes that may take time, react to signals, or call functions and macro nodes.\n\n"
           "While there is always one event graph called \"EventGraph\", you can create new "
           "event graphs to better help organize event logic.";
}

String OrchestratorScriptGraphsComponentPanel::_get_remove_confirm_text(TreeItem* p_item) const
{
    // Only confirm graphs, not event functions
    if (p_item->get_parent() == _tree->get_root())
        return "Removing a graph removes all nodes within the graph.";

    return OrchestratorScriptComponentPanel::_get_remove_confirm_text(p_item);
}

bool OrchestratorScriptGraphsComponentPanel::_populate_context_menu(TreeItem* p_item)
{
    if (p_item->get_parent() == _tree->get_root())
    {
        // Graph
        Ref<OScriptGraph> graph = _orchestration->get_graph(_get_tree_item_name(p_item));
        bool rename_disabled = !graph->get_flags().has_flag(OScriptGraph::GraphFlags::GF_RENAMABLE);
        bool delete_disabled = !graph->get_flags().has_flag(OScriptGraph::GraphFlags::GF_DELETABLE);
        _context_menu->add_item("Open Graph", CM_OPEN_GRAPH);
        _context_menu->add_icon_item(SceneUtils::get_editor_icon("Rename"), "Rename", CM_RENAME_GRAPH);
        _context_menu->set_item_disabled(_context_menu->get_item_count() - 1, rename_disabled);
        _context_menu->add_icon_item(SceneUtils::get_editor_icon("Remove"), "Remove", CM_REMOVE_GRAPH);
        _context_menu->set_item_disabled(_context_menu->get_item_count() - 1, delete_disabled);
    }
    else
    {
        // Graph Functions
        _context_menu->add_item("Focus", CM_FOCUS_FUNCTION);
        _context_menu->add_icon_item(SceneUtils::get_editor_icon("Remove"), "Remove", CM_REMOVE_FUNCTION);

        if (p_item->has_meta("__slot") && p_item->get_meta("__slot"))
        {
            _context_menu->add_icon_item(SceneUtils::get_editor_icon("Unlinked"), "Disconnect", CM_DISCONNECT_SLOT);
            _context_menu->set_item_tooltip(_context_menu->get_item_index(CM_DISCONNECT_SLOT), "Disconnect the slot function from the signal.");
        }
    }
    return true;
}

void OrchestratorScriptGraphsComponentPanel::_handle_context_menu(int p_id)
{
    switch (p_id)
    {
        case CM_OPEN_GRAPH:
            _show_graph_item(_tree->get_selected());
            break;
        case CM_RENAME_GRAPH:
            _edit_selected_tree_item();
            break;
        case CM_REMOVE_GRAPH:
            _confirm_removal(_tree->get_selected());
            break;
        case CM_FOCUS_FUNCTION:
            _focus_graph_function(_tree->get_selected());
            break;
        case CM_REMOVE_FUNCTION:
            _remove_graph_function(_tree->get_selected());
            break;
        case CM_DISCONNECT_SLOT:
            _disconnect_slot(_tree->get_selected());
            break;
    }
}

bool OrchestratorScriptGraphsComponentPanel::_handle_add_new_item(const String& p_name)
{
    // Add the new graph and update the components display
    return _orchestration->create_graph(p_name, OScriptGraph::GF_EVENT | OScriptGraph::GF_DEFAULT).is_valid();
}

void OrchestratorScriptGraphsComponentPanel::_handle_item_activated(TreeItem* p_item)
{
    if (p_item->get_parent() == _tree->get_root())
        _show_graph_item(p_item);
    else
        _focus_graph_function(p_item);
}

bool OrchestratorScriptGraphsComponentPanel::_handle_item_renamed(const String& p_old_name, const String& p_new_name)
{
    if (_get_existing_names().has(p_new_name))
    {
        _show_notification("A graph with the name '" + p_new_name + "' already exists.");
        return false;
    }

    if (!p_new_name.is_valid_identifier())
    {
        _show_invalid_name("graph");
        return false;
    }

    if (!_orchestration->rename_graph(p_old_name, p_new_name))
        return false;

    emit_signal("graph_renamed", p_old_name, p_new_name);
    return true;
}

void OrchestratorScriptGraphsComponentPanel::_handle_remove(TreeItem* p_item)
{
    if (p_item->get_parent() == _tree->get_root())
        _remove_graph(p_item);
}

void OrchestratorScriptGraphsComponentPanel::_handle_button_clicked(TreeItem* p_item, int p_column, int p_id, int p_mouse_button)
{
    if (_orchestration->get_type() != OrchestrationType::OT_Script)
        return;

    const Ref<OScript> script = _orchestration->get_self();
    const Vector<Node*> nodes = SceneUtils::find_all_nodes_for_script_in_edited_scene(script);

    OrchestratorScriptConnectionsDialog* dialog = memnew(OrchestratorScriptConnectionsDialog);
    add_child(dialog);
    dialog->popup_connections(_get_tree_item_name(p_item), nodes);
}

void OrchestratorScriptGraphsComponentPanel::_update_slots()
{
    if (_orchestration->get_type() != OrchestrationType::OT_Script)
        return;

    const Ref<OScript> script = _orchestration->get_self();
    const Vector<Node*> script_nodes = SceneUtils::find_all_nodes_for_script_in_edited_scene(script);
    const String base_type = script->get_instance_base_type();

    _iterate_tree_items(callable_mp_lambda(this, [&](TreeItem* item) {
        if (item->has_meta("__name"))
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
    }));
}

void OrchestratorScriptGraphsComponentPanel::update()
{
    _clear_tree();

    Vector<Ref<OScriptGraph>> graphs = _orchestration->get_graphs();
    if (graphs.is_empty())
    {
        TreeItem* item = _tree->get_root()->create_child();
        item->set_text(0, "No graphs defined");
        item->set_selectable(0, false);
        return;
    }

    OrchestratorSettings* settings = OrchestratorSettings::get_singleton();
    bool use_friendly_names = settings->get_setting("ui/components_panel/show_graph_friendly_names", true);

    const PackedStringArray functions = _orchestration->get_function_names();
    for (const Ref<OScriptGraph>& graph : graphs)
    {
        if (!(graph->get_flags().has_flag(OScriptGraph::GraphFlags::GF_EVENT)))
            continue;

        String friendly_name = graph->get_graph_name();
        if (use_friendly_names)
            friendly_name = friendly_name.capitalize();

        TreeItem* item = _create_item(_tree->get_root(), friendly_name, graph->get_graph_name(), "ClassList");

        for (const String& function_name : functions)
        {
            int function_id = _orchestration->get_function_node_id(function_name);
            if (graph->has_node(function_id))
            {
                friendly_name = function_name;
                if (use_friendly_names)
                    friendly_name = vformat("%s Event", function_name.capitalize());

                _create_item(item, friendly_name, function_name, "PlayStart");
            }
        }
    }

    _update_slots();

    OrchestratorScriptComponentPanel::update();
}

void OrchestratorScriptGraphsComponentPanel::_notification(int p_what)
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
        _slot_update_timer->connect("timeout", callable_mp(this, &OrchestratorScriptGraphsComponentPanel::_update_slots));
        add_child(_slot_update_timer);
    }
}

void OrchestratorScriptGraphsComponentPanel::_bind_methods()
{
    ADD_SIGNAL(MethodInfo("show_graph_requested", PropertyInfo(Variant::STRING, "graph_name")));
    ADD_SIGNAL(MethodInfo("close_graph_requested", PropertyInfo(Variant::STRING, "graph_name")));
    ADD_SIGNAL(MethodInfo("graph_renamed", PropertyInfo(Variant::STRING, "old_name"), PropertyInfo(Variant::STRING, "new_name")));
    ADD_SIGNAL(MethodInfo("focus_node_requested", PropertyInfo(Variant::STRING, "graph_name"), PropertyInfo(Variant::INT, "node_id")));
}

OrchestratorScriptGraphsComponentPanel::OrchestratorScriptGraphsComponentPanel(Orchestration* p_orchestration)
    : OrchestratorScriptComponentPanel("Graphs", p_orchestration)
{
}

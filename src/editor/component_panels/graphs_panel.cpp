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

#include "common/scene_utils.h"
#include "editor/script_connections.h"

#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/tree.hpp>

void OrchestratorScriptGraphsComponentPanel::_show_graph_item(TreeItem* p_item)
{
    const String graph_name = p_item->get_text(0);

    // Graph
    emit_signal("show_graph_requested", graph_name);
    _tree->deselect_all();
}

void OrchestratorScriptGraphsComponentPanel::_focus_graph_function(TreeItem* p_item)
{
    const String graph_name = p_item->get_parent()->get_text(0);
    const int node_id = _script->get_function_node_id(p_item->get_text(0));

    // Specific event node
    emit_signal("focus_node_requested", graph_name, node_id);
    _tree->deselect_all();
}

void OrchestratorScriptGraphsComponentPanel::_remove_graph(TreeItem* p_item)
{
    const String graph_name = p_item->get_text(0);
    emit_signal("close_graph_requested", graph_name);

    _script->remove_graph(graph_name);
}

void OrchestratorScriptGraphsComponentPanel::_remove_graph_function(TreeItem* p_item)
{
    const String function_name = p_item->get_text(0);

    _script->remove_function(function_name);
    update();
}

PackedStringArray OrchestratorScriptGraphsComponentPanel::_get_existing_names() const
{
    PackedStringArray result;
    for (const Ref<OScriptGraph>& graph : _script->get_graphs())
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
        Ref<OScriptGraph> graph = _script->get_graph(p_item->get_text(0));
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
            _tree->edit_selected(true);
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
    }
}

bool OrchestratorScriptGraphsComponentPanel::_handle_add_new_item(const String& p_name)
{
    // Add the new graph and update the components display
    return _script->create_graph(p_name, OScriptGraph::GF_EVENT | OScriptGraph::GF_DEFAULT).is_valid();
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

    _script->rename_graph(p_old_name, p_new_name);
    emit_signal("graph_renamed", p_old_name, p_new_name);
    return true;
}

void OrchestratorScriptGraphsComponentPanel::_handle_remove(TreeItem* p_item)
{
    if (p_item->get_parent() == _tree->get_root())
        _remove_graph(p_item);
}

void OrchestratorScriptGraphsComponentPanel::_handle_button_clicked(TreeItem* p_item, int p_column, int p_id,
                                                                 int p_mouse_button)
{
    const Vector<Node*> nodes = SceneUtils::find_all_nodes_for_script_in_edited_scene(_script);

    OrchestratorScriptConnectionsDialog* dialog = memnew(OrchestratorScriptConnectionsDialog);
    add_child(dialog);
    dialog->popup_connections(p_item->get_text(0), nodes);
}

void OrchestratorScriptGraphsComponentPanel::update()
{
    _clear_tree();

    Vector<Ref<OScriptGraph>> graphs = _script->get_graphs();
    if (graphs.is_empty())
    {
        TreeItem* item = _tree->get_root()->create_child();
        item->set_text(0, "No graphs defined");
        item->set_selectable(0, false);
        return;
    }

    const Vector<Node*> script_nodes = SceneUtils::find_all_nodes_for_script_in_edited_scene(_script);
    const String base_type = _script->get_instance_base_type();

    const PackedStringArray functions = _script->get_function_names();
    for (const Ref<OScriptGraph>& graph : graphs)
    {
        if (!(graph->get_flags().has_flag(OScriptGraph::GraphFlags::GF_EVENT)))
            continue;

        TreeItem* item = _tree->get_root()->create_child();
        item->set_text(0, graph->get_graph_name());
        item->set_meta("__name", graph->get_graph_name()); // Used for renames
        item->set_icon(0, SceneUtils::get_editor_icon("ClassList"));

        TypedArray<int> nodes = graph->get_nodes();
        for (const String& function_name : functions)
        {
            int function_id = _script->get_function_node_id(function_name);
            if (nodes.has(function_id))
            {
                TreeItem* func = item->create_child();
                func->set_text(0, function_name);
                func->set_icon(0, SceneUtils::get_editor_icon("PlayStart"));

                if (SceneUtils::has_any_signals_connected_to_function(function_name, base_type, script_nodes))
                    func->add_button(0, SceneUtils::get_editor_icon("Slot"));
            }
        }
    }

    OrchestratorScriptComponentPanel::update();
}

void OrchestratorScriptGraphsComponentPanel::_bind_methods()
{
    ADD_SIGNAL(MethodInfo("show_graph_requested", PropertyInfo(Variant::STRING, "graph_name")));
    ADD_SIGNAL(MethodInfo("close_graph_requested", PropertyInfo(Variant::STRING, "graph_name")));
    ADD_SIGNAL(MethodInfo("graph_renamed", PropertyInfo(Variant::STRING, "old_name"), PropertyInfo(Variant::STRING, "new_name")));
    ADD_SIGNAL(MethodInfo("focus_node_requested", PropertyInfo(Variant::STRING, "graph_name"), PropertyInfo(Variant::INT, "node_id")));
}

OrchestratorScriptGraphsComponentPanel::OrchestratorScriptGraphsComponentPanel(const Ref<OScript>& p_script)
    : OrchestratorScriptComponentPanel("Graphs", p_script)
{
}

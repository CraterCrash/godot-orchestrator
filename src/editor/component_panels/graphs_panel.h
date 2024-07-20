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
#ifndef ORCHESTRATOR_SCRIPT_GRAPHS_COMPONENT_PANEL_H
#define ORCHESTRATOR_SCRIPT_GRAPHS_COMPONENT_PANEL_H

#include "editor/component_panels/component_panel.h"

class OrchestratorScriptGraphsComponentPanel : public OrchestratorScriptComponentPanel
{
    GDCLASS(OrchestratorScriptGraphsComponentPanel, OrchestratorScriptComponentPanel);
    static void _bind_methods();

    enum ContextMenuIds
    {
        CM_OPEN_GRAPH,
        CM_RENAME_GRAPH,
        CM_REMOVE_GRAPH,
        CM_FOCUS_FUNCTION,
        CM_REMOVE_FUNCTION,
        CM_DISCONNECT_SLOT
    };

protected:
    //~ Begin OrchestratorScriptComponentPanel Interface
    String _get_unique_name_prefix() const override { return "NewEventGraph"; }
    PackedStringArray _get_existing_names() const override;
    String _get_tooltip_text() const override;
    String _get_remove_confirm_text(TreeItem* p_item) const override;
    String _get_item_name() const override { return "EventGraph"; }
    bool _populate_context_menu(TreeItem* p_item) override;
    void _handle_context_menu(int p_id) override;
    bool _handle_add_new_item(const String& p_name) override;
    void _handle_item_activated(TreeItem* p_item) override;
    bool _can_be_renamed(const String& p_old_name, const String& p_new_name) override;
    void _handle_item_renamed(const String& p_old_name, const String& p_new_name) override;
    void _handle_remove(TreeItem* p_item) override;
    void _handle_button_clicked(TreeItem* p_item, int p_column, int p_id, int p_mouse_button) override;
    //~ End OrchestratorScriptComponentPanel Interface

    /// Notifies the script view to show the specified graph associated with the tree item
    /// @param p_item the graph tree item, should not be null
    void _show_graph_item(TreeItem* p_item);

    /// Notifies the script view to focus a specific event node in the graph.
    /// @param p_item the graph event item, should not be null
    void _focus_graph_function(TreeItem* p_item);

    /// Notifies the script view that a graph has been removed.
    /// This is useful to close any views that pertain to the graph.
    /// @param p_item the graph item, should not be null
    void _remove_graph(TreeItem* p_item);

    /// Notifies the script view that a graph function was removed.
    /// @param p_item the graph function item, should not be null
    void _remove_graph_function(TreeItem* p_item);

    /// Handles disconnecting a signal slot function from the signal
    void _disconnect_slot(TreeItem* p_item);

    /// Default constructor
    OrchestratorScriptGraphsComponentPanel() = default;

public:
    //~ Begin OrchestratorScriptViewSection Interface
    void update() override;
    //~ End OrchestratorScriptViewSection Interface

    /// Constructs the graphs component panel
    /// @param p_orchestration the orchestration
    explicit OrchestratorScriptGraphsComponentPanel(Orchestration* p_orchestration);
};

#endif // ORCHESTRATOR_SCRIPT_GRAPHS_COMPONENT_PANEL_H
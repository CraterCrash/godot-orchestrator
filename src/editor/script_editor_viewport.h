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
#ifndef ORCHESTRATOR_SCRIPT_EDITOR_VIEWPORT_H
#define ORCHESTRATOR_SCRIPT_EDITOR_VIEWPORT_H

#include "editor/editor_viewport.h"

/// Forward declarations
class OrchestratorScriptComponentPanel;
class OScript;
class OScriptFunction;

/// Viewport implementation for Orchestrator scripts
class OrchestratorScriptEditorViewport : public OrchestratorEditorViewport
{
    GDCLASS(OrchestratorScriptEditorViewport, OrchestratorEditorViewport);
    static void _bind_methods();

protected:
    OrchestratorGraphEdit* _event_graph{ nullptr };           //! The event graph
    OrchestratorScriptComponentPanel* _graphs{ nullptr };     //! Graphs section
    OrchestratorScriptComponentPanel* _functions{ nullptr };  //! Functions section
    OrchestratorScriptComponentPanel* _macros{ nullptr };     //! Macros section
    OrchestratorScriptComponentPanel* _variables{ nullptr };  //! Variables section
    OrchestratorScriptComponentPanel* _signals{ nullptr };    //! Signals section

    //~ Begin Godot Interface
    void _notification(int p_what);
    //~ End Godot Interface

    //~ Begin OrchestratorEditorViewport Interface
    void _update_components() override;
    bool _can_graph_be_closed(OrchestratorGraphEdit* p_graph) override;
    void _focus_object(Object* p_object) override;
    void _graph_opened(OrchestratorGraphEdit* p_graph) override;
    //~ End OrchestratorEditorViewport Interface

    /// Saves the editor state to the cache
    void _save_state();

    /// Restores the editor state from the cache
    void _restore_state();

    /// Creates a new function in the script
    /// @param p_name the function name
    /// @param p_has_return whether function has a return node
    /// @return the newly constructed function
    Ref<OScriptFunction> _create_new_function(const String& p_name, bool p_has_return);

    /// Shows the graph with the given name, adding the tab if it doesn't exist.
    /// @param p_name the graph name
    void _show_graph(const String& p_name);

    /// Closes the graph with the given name
    /// @param p_name the graph name
    void _close_graph(const String& p_name);

    /// Called when a graph is renamed, updating the graph tab
    /// @param p_old_name the old graph name
    /// @param p_new_name the new graph name
    void _graph_renamed(const String& p_old_name, const String& p_new_name);

    /// Focuses the specified node in the given graph
    /// @param p_graph_name the graph name
    /// @param p_node_id the node ID to focus
    void _focus_node(const String& p_graph_name, int p_node_id);

    /// Scrolls to the specified tree item
    /// @param p_item the item to scroll to
    void _scroll_to_item(TreeItem* p_item);

    /// Shows the available Godot function overrides
    void _override_godot_function();

    /// Collapse the selected nodes to a function
    /// @param p_graph the graph where selected nodes should be collapsed
    void _collapse_selected_to_function(OrchestratorGraphEdit* p_graph);

    /// Expand the node
    /// @param p_node_id the node to be expanded
    /// @param p_graph the graph that owns the node that is being expanded
    void _expand_node(int p_node_id, OrchestratorGraphEdit* p_graph);

    /// Default constructor, intentionally protected
    OrchestratorScriptEditorViewport() = default;

public:
    //~ Begin OrchestratorEditorViewport Interface
    void apply_changes() override;
    void add_script_function(Object* p_object, const String& p_function_name, const PackedStringArray& p_args) override;
    //~ End OrchestratorEditorViewport Interface

    /// Constructor
    /// @param p_script the orchestration script
    explicit OrchestratorScriptEditorViewport(const Ref<OScript>& p_script);

    /// Default destructor
    ~OrchestratorScriptEditorViewport() override = default;
};

#endif  // ORCHESTRATOR_SCRIPT_EDITOR_VIEWPORT_H
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
#ifndef ORCHESTRATOR_EDITOR_SCRIPT_COMPONENTS_CONTAINER_H
#define ORCHESTRATOR_EDITOR_SCRIPT_COMPONENTS_CONTAINER_H

#include "orchestration/orchestration.h"
#include "script/script.h"

#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/tree_item.hpp>

using namespace godot;

class OrchestratorEditorComponentView;
class OrchestratorEditorGraphPanel;

/// A container that includes all the various components that can exist in an <code>Orchestration</code> script.
///
class OrchestratorScriptComponentsContainer : public ScrollContainer
{
    GDCLASS(OrchestratorScriptComponentsContainer, ScrollContainer);

    // Simple RAII handler to provide a deferred callback that fire when the current
    // execution scope ends in a function block
    struct ScopedDeferredCallable {
        Callable _callable;

        ScopedDeferredCallable(const ScopedDeferredCallable&) = delete;
        ScopedDeferredCallable& operator=(const ScopedDeferredCallable&) = delete;

        ScopedDeferredCallable(ScopedDeferredCallable&&) = default;
        ScopedDeferredCallable& operator=(ScopedDeferredCallable&&) = default;

        explicit ScopedDeferredCallable(Callable&& p_callable) : _callable(std::move(p_callable)) {}

        ~ScopedDeferredCallable()
        {
            if (_callable.is_valid())
                _callable.call_deferred();
        }
    };

    enum ComponentItemType {
        NONE,
        EVENT_GRAPH,
        EVENT_GRAPH_FUNCTION,
        SCRIPT_FUNCTION,
        SCRIPT_VARIABLE,
        SCRIPT_MACRO,
        SCRIPT_SIGNAL,
        COMPONENT_MAX
    };

    Ref<Orchestration> _orchestration;

    OrchestratorEditorComponentView* _graphs = nullptr;
    OrchestratorEditorComponentView* _functions = nullptr;
    OrchestratorEditorComponentView* _macros = nullptr;
    OrchestratorEditorComponentView* _variables = nullptr;
    OrchestratorEditorComponentView* _signals = nullptr;

    bool _use_graph_friendly_names = false;
    bool _use_function_friendly_names = false;

    Ref<Orchestration> _get_orchestration();

    void _open_graph(const String& p_graph_name);
    void _open_graph_with_focus(const String& p_graph_name, int p_node_id);
    void _close_graph(const String& p_graph_name);

    void _show_invalid_identifier(const String& p_name, bool p_friendly_names);

    void _component_show_context_menu(Node* p_node, TreeItem* p_item, const Vector2& p_position);
    void _component_item_gui_input(TreeItem* p_item, const Ref<InputEvent>& p_event);
    Variant _component_item_dragged(TreeItem* p_item, const Vector2& p_position);
    void _component_item_button_clicked(Node* p_node, TreeItem* p_item, int p_column, int p_id, int p_button);
    void _component_item_selected(Node* p_node, TreeItem* p_item);
    void _component_item_activated(Node* p_node, TreeItem* p_item);
    void _component_add_item(int p_component_type);
    void _component_add_item_commit(TreeItem* p_item);
    void _component_add_item_canceled(TreeItem* p_item);
    void _component_duplicate_item(TreeItem* p_item, const Dictionary& p_data);
    void _component_rename_item(TreeItem* p_item);
    void _component_remove_item(TreeItem* p_item, bool p_confirm = true);
    void _component_focus_item(TreeItem* p_item);
    void _update_components(int p_component_type = COMPONENT_MAX);
    void _find_and_edit_function(const String& p_function_name);
    void _find_and_edit_variable(const String& p_variable_name);
    void _update_graphs_and_functions();
    void _update_macros();
    void _update_variables();
    void _update_signals();
    void _update_slots();
    void _update_slot_item(TreeItem* p_item);

    void _scene_changed(Node* p_node);
    void _project_settings_changed();

    void _set_edited(bool p_edited);

protected:
    static void _bind_methods();

public:

    void set_edited_resource(const Ref<Resource>& p_resource);

    Dictionary get_edit_state();
    void set_edit_state(const Variant& p_state);

    void update();

    void notify_graph_opened(OrchestratorEditorGraphPanel* p_graph);

    OrchestratorScriptComponentsContainer();
};

#endif
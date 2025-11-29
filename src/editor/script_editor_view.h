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
#ifndef ORCHESTRATOR_EDITOR_VIEW_SCRIPT_H
#define ORCHESTRATOR_EDITOR_VIEW_SCRIPT_H

#include "editor/editor_component_view.h"
#include "editor/editor_view.h"
#include "script/language.h"

#include <godot_cpp/classes/h_split_container.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/menu_button.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/tab_container.hpp>

using namespace godot;

class OrchestratorEditorGraphPanel;
class OrchestratorScriptComponentsContainer;
class OScript;

/// Main editor view for Orchestration scripts
class OrchestratorScriptGraphEditorView : public OrchestratorEditorView
{
    GDCLASS(OrchestratorScriptGraphEditorView, OrchestratorEditorView);

    enum {
        EDIT_UNDO,
        EDIT_REDO,
        EDIT_CUT,
        EDIT_COPY,
        EDIT_PASTE,
        EDIT_SELECT_ALL,
        EDIT_SELECT_DUPLICATE,
        SEARCH_FIND,
        SEARCH_FIND_NEXT,
        SEARCH_FIND_PREVIOUS,
        SEARCH_REPLACE,
        SEARCH_LOCATE_NODE,

        TOGGLE_BOOKMARK,
        REMOVE_BOOKMARKS,
        GOTO_NEXT_BOOKMARK,
        GOTO_PREV_BOOKMARK,

        TOGGLE_BREAKPOINT,
        REMOVE_BREAKPOINTS,
        GOTO_NEXT_BREAKPOINT,
        GOTO_PREV_BREAKPOINT,
    };

    Ref<OScript> _script;

    List<OScriptLanguage::Warning> _warnings;
    List<OScriptLanguage::ScriptError> _errors;

    Dictionary _editor_state;
    Vector<String> _restore_tab_list;

    bool _editor_enabled = false;

    HBoxContainer* _edit_hb = nullptr;
    MenuButton* _edit_menu = nullptr;
    MenuButton* _search_menu = nullptr;
    MenuButton* _goto_menu = nullptr;
    PopupMenu* _bookmarks_menu = nullptr;
    PopupMenu* _breakpoints_menu = nullptr;

    HSplitContainer* _graph_split = nullptr;
    TabContainer* _tab_container = nullptr;
    RichTextLabel* _warnings_panel = nullptr;
    RichTextLabel* _errors_panel = nullptr;

    OrchestratorEditorGraphPanel* _event_graph = nullptr;
    OrchestratorScriptComponentsContainer* _components = nullptr;

protected:
    static void _bind_methods();

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped interface

    // Tab API
    OrchestratorEditorGraphPanel* _create_graph_tab(const String& p_name);
    OrchestratorEditorGraphPanel* _get_graph_tab(const String& p_name);
    OrchestratorEditorGraphPanel* _get_graph_tab(int p_index);
    OrchestratorEditorGraphPanel* _get_active_graph_tab();
    OrchestratorEditorGraphPanel* _open_graph_tab(const String& p_name);
    void _close_graph_editor(const String& p_name);
    void _focus_graph_tab(OrchestratorEditorGraphPanel* p_tab_panel);
    Dictionary _get_graph_tab_state(OrchestratorEditorGraphPanel* p_tab_panel, bool p_open = true);
    void _store_graph_tab_state(const String& p_name, const Dictionary& p_state);
    void _go_to_graph_tab(int p_index);
    void _close_graph_tab(int p_index);
    void _restore_next_tab();

    void _update_editor_script_buttons();
    void _change_script_type();

    void _component_panel_resized();

    void _show_override_function_menu();

    void _scroll_to_graph_center();
    void _scroll_to_graph_node(int p_node_id);

    void _focus_object(Object* p_object);

    void _toggle_bookmark_for_selected_nodes();
    void _remove_all_bookmarks();
    void _toggle_breakpoint_for_selected_nodes();
    void _remove_all_breakpoints();
    void _breakpoints_menu_option(int p_index);
    void _bookmarks_menu_option(int p_index);
    void _update_bookmarks_list();
    void _update_breakpoints_list();

    void _menu_option(int p_index);

    void _prepare_edit_menu();
    void _enable_editor();
    void _validate_script();

    void _show_warnings_panel(bool p_show);
    void _warning_clicked(const Variant& p_node);

    void _show_errors_panel(bool p_show);
    void _error_clicked(const Variant p_node);

    void _update_warnings();
    void _update_errors();

public:
    static void register_editor();

    //~ Begin OrchestratorGraphEditorView Interface
    Ref<Resource> get_edited_resource() const override;
    void set_edited_resource(const Ref<Resource>& p_resource) override;
    Control* get_editor() const override;
    Variant get_edit_state() override;
    void set_edit_state(const Variant& p_state) override;
    void store_previous_state() override;
    void apply_code() override;
    void enable_editor(Control* p_shortcut_context) override;
    void reload_text() override;
    String get_name() override;
    Ref<Texture2D> get_theme_icon() override;
    Ref<Texture2D> get_indicator_icon() override;
    bool is_unsaved() override;
    void add_callback(const String& p_function, const PackedStringArray& p_args) override;
    PackedInt32Array get_breakpoints() override;
    void set_breakpoint(int p_node, bool p_enabled) override;
    void clear_breakpoints() override;
    void set_debugger_active(bool p_active) override;
    Control* get_edit_menu() override;
    void clear_edit_menu() override;
    void tag_saved_version() override;
    void validate() override;
    void update_settings() override;
    void ensure_focus() override;
    void goto_node(int p_node) override;
    bool can_lose_focus_on_node_selection() const override;
    //~ End OrchestratorGraphEditorView Interface

    OrchestratorScriptGraphEditorView();
    ~OrchestratorScriptGraphEditorView() override;
};

#endif // ORCHESTRATOR_EDITOR_VIEW_SCRIPT_H
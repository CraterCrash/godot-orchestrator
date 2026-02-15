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
#include "editor/script_editor_view.h"

#include "actions/registry.h"
#include "api/extension_db.h"
#include "common/callable_lambda.h"
#include "common/macros.h"
#include "common/name_utils.h"
#include "common/resource_utils.h"
#include "common/scene_utils.h"
#include "core/godot/core_string_names.h"
#include "core/godot/scene_string_names.h"
#include "editor/editor.h"
#include "editor/goto_node_dialog.h"
#include "editor/graph/graph_panel.h"
#include "editor/gui/dialogs_helper.h"
#include "editor/plugins/orchestrator_editor_plugin.h"
#include "editor/scene/script_connections.h"
#include "editor/script_components_container.h"
#include "script/nodes/functions/event.h"
#include "script/script.h"

#include <godot_cpp/classes/editor_inspector.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/engine_debugger.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/v_separator.hpp>

void OrchestratorScriptGraphEditorView::_idle_timeout() {
    for (int i = 0; i < _tab_container->get_child_count(); i++) {
        OrchestratorEditorGraphPanel* tab_panel = _get_graph_tab(i);
        if (tab_panel) {
            tab_panel->idle_timeout();
        }
    }

    _validate_script();
}

OrchestratorEditorGraphPanel* OrchestratorScriptGraphEditorView::_create_graph_tab(const String& p_name) {
    ERR_FAIL_COND_V_MSG(!_script.is_valid(), nullptr, "OScript is not valid");

    const Ref<OScriptGraph> script_graph = _script->get_orchestration()->find_graph(p_name);
    if (!script_graph.is_valid()) {
        return nullptr;
    }

    const Ref<Texture2D> tab_icon = script_graph->get_flags().has_flag(OrchestrationGraph::GF_FUNCTION)
        ? SceneUtils::get_editor_icon("MemberMethod")
        : SceneUtils::get_editor_icon("ClassList");

    OrchestratorEditorGraphPanel* tab_panel = memnew(OrchestratorEditorGraphPanel);

    // Must add the panel first before setting the graph model
    _tab_container->add_child(tab_panel);
    _tab_container->set_tab_icon(_tab_container->get_tab_count() - 1, tab_icon);

    tab_panel->set_graph(script_graph);

    tab_panel->connect("validate_script", callable_mp_this(_queue_validate_script));
    tab_panel->connect("focus_requested", callable_mp_this(_focus_object));

    // Wire up component callbacks
    _components->notify_graph_opened(tab_panel);

    _update_editor_script_buttons();

    return tab_panel;
}

OrchestratorEditorGraphPanel* OrchestratorScriptGraphEditorView::_get_graph_tab(const String& p_name) {
    for (int i = 0; i < _tab_container->get_child_count(); i++) {
        OrchestratorEditorGraphPanel* tab_panel = _get_graph_tab(i);
        if (tab_panel && tab_panel->get_name() == p_name) {
            return tab_panel;
        }
    }
    return nullptr;
}

OrchestratorEditorGraphPanel* OrchestratorScriptGraphEditorView::_get_graph_tab(int p_index) {
    ERR_FAIL_COND_V_MSG(p_index < 0 && p_index >= _tab_container->get_tab_count(), nullptr, "Invalid tab index " + itos(p_index));
    return cast_to<OrchestratorEditorGraphPanel>(_tab_container->get_tab_control(p_index));
}

OrchestratorEditorGraphPanel* OrchestratorScriptGraphEditorView::_get_active_graph_tab() {
    return _get_graph_tab(_tab_container->get_current_tab());
}

OrchestratorEditorGraphPanel* OrchestratorScriptGraphEditorView::_open_graph_tab(const String& p_name) {
    OrchestratorEditorGraphPanel* tab_panel = _get_graph_tab(p_name);
    if (!tab_panel) {
        tab_panel = _create_graph_tab(p_name);

        // Restore state
        const Dictionary graph_states = _editor_state.get("graphs", Dictionary());
        if (graph_states.has(p_name)) {
            const Dictionary graph_state = graph_states[p_name];
            if (!graph_state.is_empty()) {
                tab_panel->set_edit_state(graph_state, Callable());
            }
        }
    }

    if (tab_panel) {
        _focus_graph_tab(tab_panel);
    }

    return tab_panel;
}

void OrchestratorScriptGraphEditorView::_close_graph_editor(const String& p_name) {
    OrchestratorEditorGraphPanel* tab_panel = _get_graph_tab(p_name);
    if (tab_panel && tab_panel != _event_graph) {
        _store_graph_tab_state(tab_panel->get_name(), _get_graph_tab_state(tab_panel, false));
        tab_panel->queue_free();
    }
}

void OrchestratorScriptGraphEditorView::_focus_graph_tab(OrchestratorEditorGraphPanel* p_tab_panel) {
    const int32_t tab_index = _tab_container->get_tab_idx_from_control(p_tab_panel);
    ERR_FAIL_COND(tab_index < 0);

    _go_to_graph_tab(tab_index);
}

Dictionary OrchestratorScriptGraphEditorView::_get_graph_tab_state(OrchestratorEditorGraphPanel* p_tab_panel, bool p_open) {
    Dictionary panel_state = p_tab_panel->get_edit_state();
    panel_state["open"] = p_open;
    panel_state["active"] = _get_active_graph_tab() == p_tab_panel && p_open;

    return panel_state;
}

void OrchestratorScriptGraphEditorView::_store_graph_tab_state(const String& p_name, const Dictionary& p_state) {
    if (!_editor_state.has("graphs")) {
        _editor_state["graphs"] = Dictionary();
    }
    _editor_state["graphs"].operator Dictionary()[p_name] = p_state;
}

void OrchestratorScriptGraphEditorView::_go_to_graph_tab(int p_index) {
    ERR_FAIL_INDEX(p_index, _tab_container->get_tab_count());

    if (_tab_container->get_current_tab() != p_index) {
        _tab_container->set_current_tab(p_index);
    }

    OrchestratorEditorGraphPanel* current_tab_panel =
        cast_to<OrchestratorEditorGraphPanel>(_tab_container->get_current_tab_control());

    if (current_tab_panel) {
        validate();
    }
}

void OrchestratorScriptGraphEditorView::_close_graph_tab(int p_index) {
    OrchestratorEditorGraphPanel* tab_panel = _get_graph_tab(p_index);
    if (tab_panel) {
        // The event graph cannot be closed
        if (tab_panel == _event_graph) {
            return;
        }
        _close_graph_editor(tab_panel->get_name());
    }
}

void OrchestratorScriptGraphEditorView::_restore_next_tab() {
    // This handles multi-frame restoration of open tabs.
    if (_restore_tab_list.is_empty()) {
        emit_signal("view_layout_restored");
        return;
    }

    String graph_name = _restore_tab_list[0];
    _restore_tab_list.erase(graph_name);

    const Dictionary graph_states = _editor_state.get("graphs", Dictionary());
    const Dictionary graph_state = graph_states[graph_name];
    if (graph_state.get("open", false)) {
        if (OrchestratorEditorGraphPanel* tab_panel = _open_graph_tab(graph_name)) {
            tab_panel->set_edit_state(graph_state, callable_mp_this(_restore_next_tab));
            return;
        }
    }

    // Always make sure that we call the next restore if there was an issue
    _restore_next_tab();
}

void OrchestratorScriptGraphEditorView::_update_editor_post_reload() {
    for (uint32_t i = 0; i < _tab_container->get_tab_count(); i++) {
        const String tab_name = _tab_container->get_tab_control(i)->get_name();
        const Ref<OScriptGraph> graph = _script->get_orchestration()->get_graph(tab_name);
        ERR_CONTINUE(!graph.is_valid());

        OrchestratorEditorGraphPanel* tab_panel = _get_graph_tab(i);
        if (tab_panel) {
            tab_panel->set_graph(graph);
        }
    }
}

void OrchestratorScriptGraphEditorView::_update_editor_script_buttons() {
    static String DETAILS_BUTTON_NAME = "ScriptDetailsButton";
    static String WARN_ERROR_SEP = "ScriptWarnErrorSep";
    static String WARNING_BUTTON_NAME = "ScriptWarningButton";
    static String ERROR_BUTTON_NAME   = "ScriptErrorButton";

    for (int i = 0; i < _tab_container->get_child_count(); i++) {
        OrchestratorEditorGraphPanel* tab_panel = _get_graph_tab(i);

        Button* button = cast_to<Button>(tab_panel->get_menu_control()->find_child(DETAILS_BUTTON_NAME, false, false));
        if (!button) {
            button = memnew(Button);
            button->set_name(DETAILS_BUTTON_NAME);
            button->set_focus_mode(FOCUS_NONE);
            button->connect(SceneStringName(pressed), callable_mp_this(_change_script_type));
            tab_panel->get_menu_control()->add_child(memnew(VSeparator));
            tab_panel->get_menu_control()->add_child(button);
        }

        const String script_type = _script->get_orchestration()->get_base_type();
        const String global_name = _script->get_orchestration()->get_global_name();
        if (!global_name.is_empty()) {
            button->set_text(vformat("%s Extends %s", global_name, script_type));
            button->set_button_icon(SceneUtils::get_class_icon(global_name));
        } else {
            button->set_text(vformat("Extends %s", script_type));
            button->set_button_icon(SceneUtils::get_class_icon(script_type));
        }

        button->add_theme_constant_override("icon_max_width", SceneUtils::get_editor_class_icon_size());

        VSeparator* sep = cast_to<VSeparator>(tab_panel->get_menu_control()->find_child(WARN_ERROR_SEP, false, false));
        if (!sep) {
            sep = memnew(VSeparator);
            sep->set_name(WARN_ERROR_SEP);
            tab_panel->get_menu_control()->add_child(sep);
        }

        button = cast_to<Button>(tab_panel->get_menu_control()->find_child(WARNING_BUTTON_NAME, false, false));
        if (!button) {
            button = memnew(Button);
            button->set_name(WARNING_BUTTON_NAME);
            button->set_focus_mode(FOCUS_NONE);
            button->set_toggle_mode(true);
            button->set_button_icon(SceneUtils::get_editor_icon("NodeWarning"));
            button->connect(SceneStringName(pressed), callable_mp_lambda(this, [&] { _show_warnings_panel(!_warnings_panel->is_visible()); }));
            button->set_tooltip_text("There are script warnings.");
            tab_panel->get_menu_control()->add_child(button);
        }

        button->set_visible(_warnings.size() > 0);
        button->set_pressed_no_signal(_warnings_panel->is_visible());
        button->set_text(vformat("%d", _warnings.size()));

        button = cast_to<Button>(tab_panel->get_menu_control()->find_child(ERROR_BUTTON_NAME, false, false));
        if (!button) {
            button = memnew(Button);
            button->set_name(ERROR_BUTTON_NAME);
            button->set_focus_mode(FOCUS_NONE);
            button->set_toggle_mode(true);
            button->set_button_icon(SceneUtils::get_editor_icon("StatusError"));
            button->connect(SceneStringName(pressed), callable_mp_lambda(this, [&] { _show_errors_panel(!_errors_panel->is_visible()); }));
            button->set_tooltip_text("There are script errors.");
            tab_panel->get_menu_control()->add_child(button);
        }

        button->set_visible(_errors.size() > 0);
        button->set_pressed_no_signal(_errors_panel->is_visible());
        button->set_text(vformat("%d", _errors.size()));

        sep->set_visible(_errors.size() > 0 || _warnings.size() > 0);
    }

    _components->update();
}

void OrchestratorScriptGraphEditorView::_change_script_type() {
    OrchestratorEditor::get_singleton()->make_inspector_visible();
    EI->inspect_object(_script->get_orchestration().ptr());
}

void OrchestratorScriptGraphEditorView::_component_panel_resized() {
    const int32_t offset = _graph_split->get_split_offset();

    Array views = get_tree()->get_nodes_in_group("_orchestrator_script_graph_views");
    for (int i = 0; i < views.size(); i++) {
        OrchestratorScriptGraphEditorView* view = cast_to<OrchestratorScriptGraphEditorView>(views[i]);
        if (view != this) {
            view->_graph_split->set_split_offset(offset);
        }
    }

    PROJECT_SET("Orchestrator", "component_panel_width", offset);
}

void OrchestratorScriptGraphEditorView::_show_override_function_menu() {
    if (OrchestratorEditorGraphPanel* active_panel = _get_active_graph_tab()) {
        String graph_name = active_panel->get_name();

        const Ref<OScriptGraph> graph = _script->get_orchestration()->find_graph(graph_name);
        ERR_FAIL_COND(graph.is_null());

        if (!graph->get_flags().has_flag(OScriptGraph::GF_EVENT)) {
            active_panel = _get_graph_tab("EventGraph");
            active_panel->show_override_function_action_menu(
                callable_mp_lambda(this, [this, active_panel] (const Variant& value) {
                    _focus_graph_tab(active_panel);
                }));
        } else {
            active_panel->show_override_function_action_menu();
        }
    }
}

void OrchestratorScriptGraphEditorView::_scroll_to_graph_center() {
    OrchestratorEditorGraphPanel* active_panel = _get_active_graph_tab();
    if (active_panel) {
        const Rect2 bounds = active_panel->get_bounds_for_nodes(false);
        active_panel->scroll_to_position(bounds.get_center());
    }
}

void OrchestratorScriptGraphEditorView::_scroll_to_graph_node(int p_node_id) {
    OrchestratorEditorGraphPanel* active_panel = _get_active_graph_tab();
    if (active_panel) {
        active_panel->center_node_id(p_node_id);
    }
}

void OrchestratorScriptGraphEditorView::_focus_object(Object* p_object) {
    const Ref<OScriptFunction> function = cast_to<OScriptFunction>(p_object);
    if (function.is_valid()) {
        if (_open_graph_tab(function->get_function_name())) {
            callable_mp_this(_scroll_to_graph_node).bind(function->get_owning_node_id()).call_deferred();
        }
    }
}

void OrchestratorScriptGraphEditorView::_toggle_bookmark_for_selected_nodes() {
    if (OrchestratorEditorGraphPanel* active_panel = _get_active_graph_tab()) {
        for (OrchestratorEditorGraphNode* node : active_panel->get_selected<OrchestratorEditorGraphNode>()) {
            active_panel->set_bookmarked(node, !node->is_bookmarked());
        }
    }
}

void OrchestratorScriptGraphEditorView::_remove_all_bookmarks() {
    if (OrchestratorEditorGraphPanel* active_panel = _get_active_graph_tab()) {
        for (OrchestratorEditorGraphNode* node : active_panel->get_all<OrchestratorEditorGraphNode>(false)) {
            if (node->is_bookmarked()) {
                active_panel->set_bookmarked(node, false);
            }
        }
    }
}

void OrchestratorScriptGraphEditorView::_toggle_breakpoint_for_selected_nodes() {
    if (OrchestratorEditorGraphPanel* active_panel = _get_active_graph_tab()) {
        for (OrchestratorEditorGraphNode* node : active_panel->get_selected<OrchestratorEditorGraphNode>()) {
            active_panel->set_breakpoint(node, !node->is_breakpoint());
        }
    }
}

void OrchestratorScriptGraphEditorView::_remove_all_breakpoints() {
    if (OrchestratorEditorGraphPanel* active_panel = _get_active_graph_tab()) {
        for (OrchestratorEditorGraphNode* node : active_panel->get_all<OrchestratorEditorGraphNode>(false)) {
            if (node->is_breakpoint()) {
                active_panel->set_breakpoint(node, false);
            }
        }
    }
}

void OrchestratorScriptGraphEditorView::_breakpoints_menu_option(int p_index) {
    if (p_index < 4) {
        // Any item before the separator chosen.
        _menu_option(_breakpoints_menu->get_item_id(p_index));
        return;
    }

    const int node_id = _breakpoints_menu->get_item_metadata(p_index);
    OrchestratorEditorGraphNode* node = _get_active_graph_tab()->find_node(node_id);
    _get_active_graph_tab()->center_node(node);
}

void OrchestratorScriptGraphEditorView::_bookmarks_menu_option(int p_index) {
    if (p_index < 4) {
        // Any item before the separator chosen.
        _menu_option(_bookmarks_menu->get_item_id(p_index));
        return;
    }

    const int node_id = _bookmarks_menu->get_item_metadata(p_index);
    OrchestratorEditorGraphNode* node = _get_active_graph_tab()->find_node(node_id);
    _get_active_graph_tab()->center_node(node);
}

void OrchestratorScriptGraphEditorView::_update_bookmarks_list() {
    _bookmarks_menu->clear();
    _bookmarks_menu->set_min_size(Vector2());
    _bookmarks_menu->reset_size();

    _bookmarks_menu->add_item("Toggle Bookmark", TOGGLE_BOOKMARK);
    _bookmarks_menu->add_item("Remove All Bookmarks", REMOVE_BOOKMARKS);
    _bookmarks_menu->add_item("Goto Next Bookmark", GOTO_NEXT_BOOKMARK);
    _bookmarks_menu->add_item("Goto Previous Bookmark", GOTO_PREV_BOOKMARK);

    if (OrchestratorEditorGraphPanel* active_panel = _get_active_graph_tab()) {
        const Vector<OrchestratorEditorGraphNode*> nodes = active_panel->predicate_find<OrchestratorEditorGraphNode>(
            [](OrchestratorEditorGraphNode* node) { return node->is_bookmarked(); });

        if (!nodes.is_empty()) {
            _bookmarks_menu->add_separator();
            for (OrchestratorEditorGraphNode* node : nodes) {
                _bookmarks_menu->add_item(vformat("%d - %s", node->get_id(), node->get_title()));
                _bookmarks_menu->set_item_metadata(-1, node->get_id());
            }
        }
    }
}

void OrchestratorScriptGraphEditorView::_update_breakpoints_list() {
    _breakpoints_menu->clear();
    _breakpoints_menu->set_min_size(Vector2());
    _breakpoints_menu->reset_size();

    _breakpoints_menu->add_item("Toggle Breakpoint", TOGGLE_BREAKPOINT);
    _breakpoints_menu->add_item("Remove All Breakpoints", REMOVE_BREAKPOINTS);
    _breakpoints_menu->add_item("Goto Next Breakpoint", GOTO_NEXT_BREAKPOINT);
    _breakpoints_menu->add_item("Goto Previous Breakpoint", GOTO_PREV_BREAKPOINT);

    if (OrchestratorEditorGraphPanel* active_panel = _get_active_graph_tab()) {
        const Vector<OrchestratorEditorGraphNode*> nodes = active_panel->predicate_find<OrchestratorEditorGraphNode>(
            [](OrchestratorEditorGraphNode* node) { return node->is_breakpoint(); });

        if (!nodes.is_empty()) {
            _breakpoints_menu->add_separator();
            for (OrchestratorEditorGraphNode* node : nodes) {
                _breakpoints_menu->add_item(vformat("%d - %s", node->get_id(), node->get_title()));
                _breakpoints_menu->set_item_metadata(-1, node->get_id());
            }
        }
    }
}

void OrchestratorScriptGraphEditorView::_update_debug_menu() {
    if (_debug_menu) {
        const bool debugger_active = OrchestratorEditorDebuggerPlugin::get_singleton()->is_active();

        PopupMenu* popup = _debug_menu->get_popup();
        popup->set_item_disabled(popup->get_item_index(DEBUG_STEP_INTO), !debugger_active);
        popup->set_item_disabled(popup->get_item_index(DEBUG_STEP_OVER), !debugger_active);
        popup->set_item_disabled(popup->get_item_index(DEBUG_BREAK), false);
        popup->set_item_disabled(popup->get_item_index(DEBUG_CONTINUE), !debugger_active);
    }
}

void OrchestratorScriptGraphEditorView::_menu_option(int p_index) {
    switch (p_index) {
        case SEARCH_LOCATE_NODE: {
            memnew(OrchestratorGotoNodeDialog)->popup_find_node(this);
            break;
        }
        case TOGGLE_BOOKMARK: {
            _toggle_bookmark_for_selected_nodes();
            break;
        }
        case REMOVE_BOOKMARKS: {
            _remove_all_bookmarks();
            break;
        }
        case GOTO_NEXT_BOOKMARK: {
            _get_active_graph_tab()->goto_next_bookmark();
            break;
        }
        case GOTO_PREV_BOOKMARK: {
            _get_active_graph_tab()->goto_previous_bookmark();
            break;
        }
        case TOGGLE_BREAKPOINT: {
            _toggle_breakpoint_for_selected_nodes();
            break;
        }
        case REMOVE_BREAKPOINTS: {
            _remove_all_breakpoints();
            break;
        }
        case GOTO_NEXT_BREAKPOINT: {
            _get_active_graph_tab()->goto_next_breakpoint();
            break;
        }
        case GOTO_PREV_BREAKPOINT: {
            _get_active_graph_tab()->goto_previous_breakpoint();
            break;
        }
        case DEBUG_BREAK: {
            OrchestratorEditorDebuggerPlugin::get_singleton()->debug_break();
            break;
        }
        case DEBUG_STEP_INTO: {
            OrchestratorEditorDebuggerPlugin::get_singleton()->debug_step_into();
            break;
        }
        case DEBUG_STEP_OVER: {
            OrchestratorEditorDebuggerPlugin::get_singleton()->debug_step_over();
            break;
        }
        case DEBUG_CONTINUE: {
            OrchestratorEditorDebuggerPlugin::get_singleton()->debug_continue();
            break;
        }
        default:
            break;
    }
}

void OrchestratorScriptGraphEditorView::_prepare_edit_menu() {
    PopupMenu* popup = _edit_menu->get_popup();
    popup->set_item_disabled(popup->get_item_index(EDIT_UNDO), true);
    popup->set_item_disabled(popup->get_item_index(EDIT_REDO), true);
}

void OrchestratorScriptGraphEditorView::_enable_editor() {
    _edit_hb->add_child(_edit_menu);
    _edit_menu->connect("about_to_popup", callable_mp_this(_prepare_edit_menu));
    _edit_menu->get_popup()->connect(SceneStringName(id_pressed), callable_mp_this(_menu_option));
    _edit_menu->get_popup()->add_item("Undo", EDIT_UNDO, OACCEL_KEY(KEY_MASK_CMD_OR_CTRL, KEY_Z));
    _edit_menu->get_popup()->add_item("Redo", EDIT_REDO, OACCEL_KEY(KEY_MASK_CMD_OR_CTRL | KEY_MASK_SHIFT, KEY_Z));

    _edit_menu->get_popup()->add_separator();
    _edit_menu->get_popup()->add_item("Cut", EDIT_CUT, OACCEL_KEY(KEY_MASK_CMD_OR_CTRL, KEY_X));
    _edit_menu->get_popup()->add_item("Copy", EDIT_COPY, OACCEL_KEY(KEY_MASK_CMD_OR_CTRL, KEY_C));
    _edit_menu->get_popup()->add_item("Paste", EDIT_PASTE, OACCEL_KEY(KEY_MASK_CMD_OR_CTRL, KEY_V));

    _edit_menu->get_popup()->add_separator();
    _edit_menu->get_popup()->add_item("Select All", EDIT_SELECT_ALL, OACCEL_KEY(KEY_MASK_CMD_OR_CTRL, KEY_A));
    _edit_menu->get_popup()->add_item("Duplicate Selection", EDIT_SELECT_DUPLICATE, OACCEL_KEY(KEY_MASK_CMD_OR_CTRL, KEY_D));
    _edit_menu->hide();

    _edit_hb->add_child(_search_menu);
    _search_menu->get_popup()->connect(SceneStringName(id_pressed), callable_mp_this(_menu_option));
    _search_menu->get_popup()->add_item("Find", SEARCH_FIND, OACCEL_KEY(KEY_MASK_CMD_OR_CTRL, KEY_F));
    _search_menu->get_popup()->add_item("Find Next", SEARCH_FIND_NEXT, KEY_F3);
    _search_menu->get_popup()->add_item("Find Previous", SEARCH_FIND_PREVIOUS, OACCEL_KEY(KEY_MASK_SHIFT, KEY_F3));
    _search_menu->get_popup()->add_item("Replace", SEARCH_REPLACE, OACCEL_KEY(KEY_MASK_CMD_OR_CTRL, KEY_R));
    _search_menu->hide();

    _edit_hb->add_child(_goto_menu);
    _goto_menu->get_popup()->connect(SceneStringName(id_pressed), callable_mp_this(_menu_option));
    _goto_menu->get_popup()->add_item("Goto Node", SEARCH_LOCATE_NODE, OACCEL_KEY(KEY_MASK_CMD_OR_CTRL, KEY_L));

    _goto_menu->get_popup()->add_separator();
    #if GODOT_VERSION >= 0x040300
    _goto_menu->get_popup()->add_submenu_node_item("Bookmarks", _bookmarks_menu);
    #else
    _goto_menu->add_child(_bookmarks_menu);
    _goto_menu->get_popup()->add_submenu_item("Bookmarks", _bookmarks_menu->get_name());
    #endif
    _update_bookmarks_list();
    _bookmarks_menu->connect("about_to_popup", callable_mp_this(_update_bookmarks_list));
    _bookmarks_menu->connect("index_pressed", callable_mp_this(_bookmarks_menu_option));

    #if GODOT_VERSION >= 0x040300
    _goto_menu->get_popup()->add_submenu_node_item("Breakpoints", _breakpoints_menu);
    _update_breakpoints_list();
    _breakpoints_menu->connect("about_to_popup", callable_mp_this(_update_breakpoints_list));
    _breakpoints_menu->connect("index_pressed", callable_mp_this(_breakpoints_menu_option));
    #endif

    _edit_hb->add_child(_debug_menu);
    _debug_menu->get_popup()->connect(SceneStringName(id_pressed), callable_mp_this(_menu_option));
    _debug_menu->get_popup()->add_item("Step Into", DEBUG_STEP_INTO, KEY_F11);
    _debug_menu->get_popup()->add_item("Step Over", DEBUG_STEP_OVER, KEY_F10);
    _debug_menu->get_popup()->add_separator();
    _debug_menu->get_popup()->add_item("Break", DEBUG_BREAK);
    _debug_menu->get_popup()->add_item("Continue", DEBUG_CONTINUE, KEY_F12);
    _debug_menu->connect("about_to_popup", callable_mp_this(_update_debug_menu));
}

void OrchestratorScriptGraphEditorView::_queue_validate_script() {
    _idle_timer->start();
}

void OrchestratorScriptGraphEditorView::_validate_script() {
    _validation_pending = false;

    if (!_script.is_valid()) {
        return;
    }

    OScriptLanguage* language = cast_to<OScriptLanguage>(_script->get_language());
    if (!language) {
        return;
    }

    List<String> functions;
    _warnings.clear();
    _errors.clear();

    if (language->validate(_script, _script->get_path(), &functions, &_warnings, &_errors)) {
        if (!_script->is_tool()) {
            _script->_update_exports();
        }
    }

    _update_warnings();
    _update_errors();

    emit_signal("name_changed");
    emit_signal("edited_script_changed");
}

void OrchestratorScriptGraphEditorView::_show_warnings_panel(bool p_show) {
    _warnings_panel->set_visible(p_show);
}

void OrchestratorScriptGraphEditorView::_warning_clicked(const Variant& p_node) {
    if (p_node.get_type() == Variant::INT) {
        goto_node(p_node);
    }
}

void OrchestratorScriptGraphEditorView::_show_errors_panel(bool p_show) {
    _errors_panel->set_visible(p_show);
}

void OrchestratorScriptGraphEditorView::_error_clicked(const Variant p_node) {
    if (p_node.get_type() == Variant::INT) {
        goto_node(p_node);
    }
}

void OrchestratorScriptGraphEditorView::_update_warnings() {
    _warnings_panel->clear();
    _warnings_panel->push_table(2);

    const Color warning_color = SceneUtils::get_editor_color("warning_color");
    for (const OScriptLanguage::Warning& warning : _warnings) {
        _warnings_panel->push_cell();
        _warnings_panel->push_meta(warning.node);
        _warnings_panel->push_color(warning_color);
        _warnings_panel->add_text(vformat("Node %d - %s: ", warning.node, warning.name));
        _warnings_panel->pop();
        _warnings_panel->pop();
        _warnings_panel->pop();

        _warnings_panel->push_cell();
        _warnings_panel->add_text(warning.message);
        _warnings_panel->pop();
    }
    _warnings_panel->pop();

    if (_warnings_panel->is_visible() && _warnings.size() == 0) {
        _warnings_panel->hide();
    }

    _update_editor_script_buttons();
}

void OrchestratorScriptGraphEditorView::_update_errors() {
    _errors_panel->clear();
    _errors_panel->push_table(2);

    const Color error_color = _errors_panel->get_theme_color("error_color", "Editor");
    for (const OScriptLanguage::ScriptError& script_error : _errors) {
        Dictionary ignore_meta;
        ignore_meta["node"] = script_error.node;

        _errors_panel->push_cell();
        _errors_panel->push_meta(script_error.node);;
        _errors_panel->push_color(error_color);
        _errors_panel->add_text(vformat("Node %d - %s: ", script_error.node, script_error.name));
        _errors_panel->pop();
        _errors_panel->pop();
        _errors_panel->pop();

        _errors_panel->push_cell();
        _errors_panel->add_text(script_error.message);
        _errors_panel->pop();
    }
    _errors_panel->pop();

    if (_errors_panel->is_visible() && _errors.size() == 0) {
        _errors_panel->hide();
    }

    _update_editor_script_buttons();
}

Ref<Resource> OrchestratorScriptGraphEditorView::get_edited_resource() const {
    return _script;
}

void OrchestratorScriptGraphEditorView::set_edited_resource(const Ref<Resource>& p_resource) {
    ERR_FAIL_COND(_script.is_valid());
    ERR_FAIL_COND(p_resource.is_null());

    _script = p_resource;
    if (!_script.is_valid()) {
        _script.unref();
        ORCHESTRATOR_ACCEPT("Script or orchestration is invalid and cannot be edited");
    }

    const Ref<OScriptGraph>& graph = _script->get_orchestration()->get_graph("EventGraph");
    if (!graph.is_valid()) {
        _script.unref();
        ORCHESTRATOR_ACCEPT("Orchestration has no event graph and cannot be opened.");
    }

    // Makes sure that when Orchestration changes, any editor tab panels are updated
    _script->get_orchestration()->connect(CoreStringName(changed), callable_mp_this(_update_editor_script_buttons));
    _script->get_orchestration()->connect("reloaded", callable_mp_this(_update_editor_post_reload));
    _script->connect(CoreStringName(changed), callable_mp_this(_update_editor_script_buttons));

    _event_graph = _create_graph_tab("EventGraph");

    _components->set_edited_resource(p_resource);
    _components->update();

    emit_signal("name_changed");
}

Control* OrchestratorScriptGraphEditorView::get_editor() const {
    // HACKY but it works
    return const_cast<OrchestratorScriptGraphEditorView*>(this)->_get_active_graph_tab();
}

Variant OrchestratorScriptGraphEditorView::get_edit_state() {
    // Update any graph tabs that remain open
    // Any tabs that are not open will be left as they were in the cached state
    for (int i = 0; i < _tab_container->get_tab_count(); i++) {
        OrchestratorEditorGraphPanel* tab_panel = _get_graph_tab(i);
        _store_graph_tab_state(tab_panel->get_name(), _get_graph_tab_state(tab_panel, true));
    }

    _editor_state["panels"] = _components->get_edit_state();

    return _editor_state;
}

void OrchestratorScriptGraphEditorView::set_edit_state(const Variant& p_state) {
    _editor_state = p_state;

    // During restoration of tab details, we need to do this in a way where we allow the scroll/zoom
    // to be applied when the GraphEdit is visible in the tab. To do this, thet tween used needs to
    // finish for each tab before we move onto the next. So this queues up a list of graph tabs we
    // want to restore, and then kicks off at the end the _restore_next_tab that performs this over
    // a series of frames.
    String active_tab_name;
    const Dictionary graph_states = _editor_state.get("graphs", Dictionary());
    const Array graph_keys = graph_states.keys();
    for (int i = 0; i < graph_keys.size(); i++) {
        const String graph_name = graph_keys[i];

        const Ref<OScriptGraph> graph = _script->get_orchestration()->find_graph(graph_name);
        if (!graph.is_valid()) {
            // Graph must have been removed or failed to save properly, remove it
            Dictionary(_editor_state["graphs"]).erase(graph_name);
            continue;
        }

        const Dictionary graph_state = graph_states[graph_name];
        if (graph_state.get("open", false)) {
            if (graph_state.get("active", false)) {
                active_tab_name = graph_name;
                continue;
            }

            _restore_tab_list.push_back(graph_name);
        }
    }

    if (!active_tab_name.is_empty()) {
        _restore_tab_list.push_back(active_tab_name);
    }

    // Panels
    _components->set_edit_state(p_state);

    _restore_next_tab();
}

void OrchestratorScriptGraphEditorView::store_previous_state() {
}

void OrchestratorScriptGraphEditorView::apply_code() {
    if (_script.is_null()) {
        return;
    }
    _script->_update_exports();
}

void OrchestratorScriptGraphEditorView::enable_editor(Control* p_shortcut_context) {
    if (_editor_enabled) {
        return;
    }

    _editor_enabled = true;

    _enable_editor();
    _validate_script();

    if (p_shortcut_context) {
        for (int i = 0; i < _edit_hb->get_child_count(); i++) {
            if (Control* child = cast_to<Control>(_edit_hb->get_child(i))) {
                child->set_shortcut_context(p_shortcut_context);
            }
        }
    }
}

void OrchestratorScriptGraphEditorView::reload_text() {
    ERR_FAIL_COND(_script.is_null());

    for (int i = 0; i < _tab_container->get_tab_count(); i++) {
        if (OrchestratorEditorGraphPanel* tab_panel = _get_graph_tab(i)) {
            tab_panel->reloaded_from_file();
        }
    }

    if (_editor_enabled) {
        _validate_script();
    }
}

String OrchestratorScriptGraphEditorView::get_name() {
    if (!_script.is_valid()) {
        return {};
    }

    String name = _script->get_path().get_file();
    if (name.is_empty()) {
        name = "[unsaved]";
    } else if (ResourceUtils::is_builtin(_script)) {
        const String& script_name = _script->get_name();
        if (!script_name.is_empty()) {
            name = vformat("%s (%s)", script_name, name.get_slice("::", 0));
        }
    }

    if (is_unsaved()) {
        name += "(*)";
    }

    return name;
}

Ref<Texture2D> OrchestratorScriptGraphEditorView::get_theme_icon() {
    if (get_parent_control() && _script.is_valid()) {
        String icon_name = _script->get_class();
        if (ResourceUtils::is_builtin(_script)) {
            icon_name += "Internal";
        }
        if (get_parent_control()->has_theme_icon(icon_name, "EditorIcons")) {
            return get_parent_control()->get_theme_icon(icon_name, "EditorIcons");
        }
        if (get_parent_control()->has_theme_icon(_script->get_class(), "EditorIcons")) {
            return get_parent_control()->get_theme_icon(_script->get_class(), "EditorIcons");
        }
    }

    return SceneUtils::get_editor_icon("GDScript");
}

Ref<Texture2D> OrchestratorScriptGraphEditorView::get_indicator_icon() {
    if (get_parent_control()) {
        if (!_errors.is_empty()) {
            return SceneUtils::get_editor_icon("StatusError");
        }
        if (!_warnings.is_empty()) {
            return SceneUtils::get_editor_icon("NodeWarning");
        }
    }

    return {};
}

bool OrchestratorScriptGraphEditorView::is_unsaved() {
    ERR_FAIL_COND_V(!_script.is_valid(), false);
    ERR_FAIL_COND_V(!_script->get_orchestration().is_valid(), false);
    return _script->get_orchestration()->is_edited();
}

void OrchestratorScriptGraphEditorView::add_callback(const String& p_function, const PackedStringArray& p_args) {
    if (_script->get_orchestration()->has_function(p_function)) {
        // This could be the user relinking an existing function to a signal.
        // In this case, we simply toggle a component view update only.
        _components->update();
        return;
    }

    callable_mp(OrchestratorPlugin::get_singleton(), &OrchestratorPlugin::make_active).call_deferred();

    MethodInfo method;
    method.name = p_function;
    method.return_val.type = Variant::NIL;

    for (const String& argument : p_args) {
        PackedStringArray bits = argument.split(":");
        for (String& bit : bits) {
            bit = bit.strip_edges();
        }

        if (ClassDB::get_class_list().has(bits[1])) {
            // Type represents a registered class.
            PropertyInfo property;
            property.name = bits[0];
            property.class_name = bits[1];
            property.type = Variant::OBJECT;
            method.arguments.push_back(property);
        } else if (ExtensionDB::is_builtin_type(bits[1])) {
            // Built-in Type
            PropertyInfo property;
            property.name = bits[0];
            property.type = ExtensionDB::get_builtin_type(bits[1]).type;
            method.arguments.push_back(property);
        } else {
            ORCHESTRATOR_ERROR("Failed to create argument from \"" + argument + "\"");
        }
    }

    OrchestratorEditorGraphPanel* editor = _get_active_graph_tab();
    if (editor) {
        NodeSpawnOptions options;
        options.context.method = method;
        options.position = editor->get_scroll_offset() + (editor->get_size() / 2.0);

        OrchestratorEditorGraphNode* node = editor->spawn_node<OScriptNodeEvent>(options);
        callable_mp(editor, &OrchestratorEditorGraphPanel::center_node).bind(node).call_deferred();
    }
}

PackedInt32Array OrchestratorScriptGraphEditorView::get_breakpoints() {
    PackedInt32Array breakpoints;
    for (int i = 0; i < _tab_container->get_tab_count(); i++) {
        OrchestratorEditorGraphPanel* tab_panel = _get_graph_tab(i);
        if (!tab_panel) {
            continue;
        }

        for (int32_t breakpoint : tab_panel->get_breakpoints()) {
            if (!breakpoints.has(breakpoint)) {
                breakpoints.push_back(breakpoint);
            }
        }
    }
    return breakpoints;
}

void OrchestratorScriptGraphEditorView::set_breakpoint(int p_node, bool p_enabled) {
    const Ref<OrchestrationGraphNode> node = _script->get_orchestration()->get_node(p_node);
    if (node.is_valid()) {
        const String graph_name = node->get_owning_graph()->get_graph_name();
        if (OrchestratorEditorGraphPanel* tab_panel = _open_graph_tab(graph_name)) {
            tab_panel->set_breakpoint(tab_panel->find_node(p_node), p_enabled);
        }
    }
}

void OrchestratorScriptGraphEditorView::clear_breakpoints() {
    for (int i = 0; i < _tab_container->get_tab_count(); i++) {
        OrchestratorEditorGraphPanel* tab_panel = _get_graph_tab(i);
        if (tab_panel) {
            tab_panel->clear_breakpoints();
        }
    }
}

void OrchestratorScriptGraphEditorView::set_debugger_active(bool p_active) {
}

Control* OrchestratorScriptGraphEditorView::get_edit_menu() {
    return _edit_hb;
}

void OrchestratorScriptGraphEditorView::clear_edit_menu() {
    if (_editor_enabled) {
        memdelete(_edit_hb);
    }
}

void OrchestratorScriptGraphEditorView::tag_saved_version() {
    // code_editor->get_text_editor()->tag_saved_version()
    edited_file_data.last_modified_time = FileAccess::get_modified_time(edited_file_data.path);
}

void OrchestratorScriptGraphEditorView::validate() {
    _queue_validate_script();
}

void OrchestratorScriptGraphEditorView::update_settings() {
    _idle_time = EDITOR_GET("text_editor/completion/idle_parse_delay");
    _idle_time_with_errors = EDITOR_GET("text_editor/completion/idle_parse_delay_with_errors_found");
}

void OrchestratorScriptGraphEditorView::ensure_focus() {
    if (OrchestratorEditorGraphPanel* tab_panel = _get_active_graph_tab()) {
        tab_panel->grab_focus();
    }
}

void OrchestratorScriptGraphEditorView::goto_node(int p_node) {
    for (const Ref<OScriptGraph>& graph : _script->get_orchestration()->get_graphs()) {
        if (graph->has_node(p_node)) {
            if (_open_graph_tab(graph->get_graph_name())) {
                _get_active_graph_tab()->center_node_id(p_node);
            }
            return;
        }
    }

    if (p_node >= 0) {
        ORCHESTRATOR_ERROR(vformat("Node %d not found in script", p_node));
    }
}

bool OrchestratorScriptGraphEditorView::can_lose_focus_on_node_selection() const {
    return true;
}

static OrchestratorEditorView* create_editor(const Ref<Resource>& p_resource) {
    if (Object::cast_to<OScript>(*p_resource)) {
        return memnew(OrchestratorScriptGraphEditorView);
    }
    return nullptr;
}

void OrchestratorScriptGraphEditorView::register_editor() {
    OrchestratorEditor::register_create_view_function(create_editor);
}

void OrchestratorScriptGraphEditorView::_notification(int p_what) {
    GDE_NOTIFICATION(OrchestratorEditorView, p_what);

    // We maintain a private group of objects under the "_orchestrator_script_graph_views" group, which
    // is used by the plugin to identify all script graph views. This group is used to coordinate when
    // the component panel visibility and width changes across the views.
    switch (p_what) {
        case NOTIFICATION_ENTER_TREE: {
            add_to_group("_orchestrator_script_graph_views");
            break;
        }
        case NOTIFICATION_EXIT_TREE: {
            remove_from_group("_orchestrator_script_graph_views");
            break;
        }
        case NOTIFICATION_THEME_CHANGED: {
            if (!_editor_enabled) {
                return;
            }
            if (is_visible_in_tree()) {
                _update_warnings();
                _update_errors();
            }
            break;
        }
    }
}

void OrchestratorScriptGraphEditorView::_bind_methods() {
}

OrchestratorScriptGraphEditorView::OrchestratorScriptGraphEditorView() {
    _idle_timer = memnew(Timer);
    _idle_timer->set_one_shot(true);
    _idle_timer->connect("timeout", callable_mp_this(_idle_timeout));
    add_child(_idle_timer);

    VBoxContainer* container = memnew(VBoxContainer);
    container->set_v_size_flags(SIZE_EXPAND_FILL);
    add_child(container);

    _graph_split = memnew(HSplitContainer);
    _graph_split->set_h_size_flags(SIZE_EXPAND_FILL);
    _graph_split->set_v_size_flags(SIZE_EXPAND_FILL);
    _graph_split->set_split_offset(PROJECT_GET("Orchestrator", "component_panel_width", 0));
    _graph_split->connect("drag_ended", callable_mp_this(_component_panel_resized));
    container->add_child(_graph_split);

    _tab_container = memnew(TabContainer);
    _tab_container->set_h_size_flags(SIZE_EXPAND_FILL);
    _tab_container->get_tab_bar()->set_tab_close_display_policy(TabBar::CLOSE_BUTTON_SHOW_ALWAYS);
    _tab_container->get_tab_bar()->connect("tab_close_pressed", callable_mp_this(_close_graph_tab));
    _tab_container->connect("tab_changed", callable_mp_this(_go_to_graph_tab));
    _graph_split->add_child(_tab_container);

    _components = memnew(OrchestratorScriptComponentsContainer);
    _components->connect("open_graph_requested", callable_mp_this(_open_graph_tab));
    _components->connect("close_graph_requested", callable_mp_this(_close_graph_editor));
    _components->connect("scroll_to_center", callable_mp_this(_scroll_to_graph_center));
    _components->connect("focus_node", callable_mp_this(_scroll_to_graph_node));
    _components->connect("add_function_override_requested", callable_mp_this(_show_override_function_menu));
    _components->connect("validate_script", callable_mp_this(_validate_script));
    _graph_split->add_child(_components);

    _warnings_panel = memnew(RichTextLabel);
    _warnings_panel->set_custom_minimum_size(Size2(0, 100 * EDSCALE));
    _warnings_panel->set_h_size_flags(SIZE_EXPAND_FILL);
    _warnings_panel->set_meta_underline(true);
    _warnings_panel->set_selection_enabled(true);
    _warnings_panel->set_context_menu_enabled(true);
    _warnings_panel->set_focus_mode(FOCUS_CLICK);
    _warnings_panel->hide();
    _warnings_panel->add_theme_font_override("normal_font", SceneUtils::get_editor_font("main"));
    _warnings_panel->add_theme_font_size_override("normal_font_size", SceneUtils::get_editor_font_size("main_size"));
    _warnings_panel->connect("meta_clicked", callable_mp_this(_warning_clicked));
    container->add_child(_warnings_panel);

    _errors_panel = memnew(RichTextLabel);
    _errors_panel->set_custom_minimum_size(Size2(0, 100 * EDSCALE));
    _errors_panel->set_h_size_flags(SIZE_EXPAND_FILL);
    _errors_panel->set_meta_underline(true);
    _errors_panel->set_selection_enabled(true);
    _errors_panel->set_context_menu_enabled(true);
    _errors_panel->set_focus_mode(FOCUS_CLICK);
    _errors_panel->hide();
    _errors_panel->add_theme_font_override("normal_font", SceneUtils::get_editor_font("main"));
    _errors_panel->add_theme_font_size_override("normal_font_size", SceneUtils::get_editor_font_size("main_size"));
    _errors_panel->connect("meta_clicked", callable_mp_this(_error_clicked));
    container->add_child(_errors_panel);

    Label* status = memnew(Label);
    status->set_visible(false);
    status->set_text("StatusPanel");
    container->add_child(status);

    _edit_hb = memnew(HBoxContainer);

    _edit_menu = memnew(MenuButton);
    _edit_menu->set_text("Edit");
    _edit_menu->set_switch_on_hover(true);
    _edit_menu->set_shortcut_context(this);

    _search_menu = memnew(MenuButton);
    _search_menu->set_text("Search");
    _search_menu->set_switch_on_hover(true);
    _search_menu->set_shortcut_context(this);

    _goto_menu = memnew(MenuButton);
    _goto_menu->set_text("Goto");
    _goto_menu->set_switch_on_hover(true);
    _goto_menu->set_shortcut_context(this);

    _debug_menu = memnew(MenuButton);
    _debug_menu->set_text("Debug");
    _debug_menu->set_switch_on_hover(true);
    _debug_menu->set_shortcut_context(this);

    _bookmarks_menu = memnew(PopupMenu);
    _breakpoints_menu = memnew(PopupMenu);
}

OrchestratorScriptGraphEditorView::~OrchestratorScriptGraphEditorView() {
    if (!_editor_enabled) {
        memdelete(_edit_hb);
        memdelete(_edit_menu);
        memdelete(_search_menu);
        memdelete(_goto_menu);
        memdelete(_debug_menu);
        memdelete(_bookmarks_menu);
        memdelete(_breakpoints_menu);
    }
}
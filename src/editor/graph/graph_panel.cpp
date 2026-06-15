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
#include "editor/graph/graph_panel.h"

#include "common/callable_lambda.h"
#include "common/dictionary_utils.h"
#include "common/macros.h"
#include "common/name_utils.h"
#include "common/property_utils.h"
#include "common/scene_utils.h"
#include "common/settings.h"
#include "common/string_utils.h"
#include "common/variant_utils.h"
#include "core/godot/config/project_settings_cache.h"
#include "core/godot/core_string_names.h"
#include "core/godot/scene_string_names.h"
#include "editor/actions/filter_engine.h"
#include "editor/actions/menu.h"
#include "editor/actions/registry.h"
#include "editor/actions/rules/override_function_rule.h"
#include "editor/autowire_connection_dialog.h"
#include "editor/graph/graph_markers.h"
#include "editor/graph/graph_node.h"
#include "editor/graph/graph_node_factory.h"
#include "editor/graph/graph_panel_styler.h"
#include "editor/graph/graph_pin.h"
#include "editor/graph/nodes/comment_graph_frame.h"
#include "editor/graph/nodes/reroute_graph_node.h"
#include "editor/gui/context_menu.h"
#include "editor/gui/dialogs_helper.h"
#include "editor/settings/editor_settings.h"
#include "orchestration/graph.h"
#include "orchestration/nodes.h"
#include "orchestration/orchestration.h"
#include "script/script.h"
#include "script/script_server.h"

#include <godot_cpp/classes/center_container.hpp>
#include <godot_cpp/classes/curve2d.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/graph_frame.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/method_tweener.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/script_editor.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/tween.hpp>
#include <godot_cpp/classes/v_separator.hpp>

using Connection = OScriptConnection;

void OrchestratorEditorGraphPanel::_child_entered_tree(Node* p_node) {
    if (OrchestratorEditorGraphFrame* frame = cast_to<OrchestratorEditorGraphFrame>(p_node)) {
        _connect_graph_frame_signals(frame);
    } else if (OrchestratorEditorGraphNode* node = cast_to<OrchestratorEditorGraphNode>(p_node)) {
        _connect_graph_node_signals(node);
    }
}

void OrchestratorEditorGraphPanel::_child_exiting_tree(Node* p_node) {
    if (OrchestratorEditorGraphFrame* frame = cast_to<OrchestratorEditorGraphFrame>(p_node)) {
        _disconnect_graph_frame_signals(frame);
    } else if (OrchestratorEditorGraphNode* node = cast_to<OrchestratorEditorGraphNode>(p_node)) {
        _disconnect_graph_node_signals(node);
    }
}

void OrchestratorEditorGraphPanel::_connection_from_empty(const StringName& p_name, int p_port, const Vector2& p_position) {
    ERR_FAIL_COND_MSG(!p_name.is_valid_int(), "Connection name is expected to be an integer value");

    PinHandle handle;
    handle.node_id = p_name.to_int();
    handle.pin_port = p_port;

    _connect_with_menu(handle, p_position, true);
}

void OrchestratorEditorGraphPanel::_connection_to_empty(const StringName& p_name, int p_port, const Vector2& p_position) {
    ERR_FAIL_COND_MSG(!p_name.is_valid_int(), "Connection name is expected to be an integer value");

    PinHandle handle;
    handle.node_id = p_name.to_int();
    handle.pin_port = p_port;

    _connect_with_menu(handle, p_position, false);
}

void OrchestratorEditorGraphPanel::_connection_request(const StringName& p_from, int p_from_port, const StringName& p_to, int p_to_port) {
    ERR_FAIL_COND_MSG(!p_from.is_valid_int(), "Connection from name is expected to be an integer value");
    ERR_FAIL_COND_MSG(!p_to.is_valid_int(), "Connection to name is expected to be an integer value");

    PinHandle from_handle;
    from_handle.node_id = p_from.to_int();
    from_handle.pin_port = p_from_port;

    PinHandle to_handle;
    to_handle.node_id = p_to.to_int();
    to_handle.pin_port = p_to_port;

    OrchestratorEditorGraphPin* source = _resolve_pin_from_handle(from_handle, false);
    OrchestratorEditorGraphPin* target = _resolve_pin_from_handle(to_handle, true);
    ERR_FAIL_COND_MSG(!source || !target, "Could not resolve one of the connection pins");

    link(source, target);
}

void OrchestratorEditorGraphPanel::_disconnection_request(const StringName& p_from, int p_from_port, const StringName& p_to, int p_to_port) {
    ERR_FAIL_COND_MSG(!p_from.is_valid_int(), "Connection from name is expected to be an integer value");
    ERR_FAIL_COND_MSG(!p_to.is_valid_int(), "Connection to name is expected to be an integer value");

    PinHandle from_handle;
    from_handle.node_id = p_from.to_int();
    from_handle.pin_port = p_from_port;

    PinHandle to_handle;
    to_handle.node_id = p_to.to_int();
    to_handle.pin_port = p_to_port;

    OrchestratorEditorGraphPin* source = _resolve_pin_from_handle(from_handle, false);
    OrchestratorEditorGraphPin* target = _resolve_pin_from_handle(to_handle, true);
    ERR_FAIL_COND_MSG(!source || !target, "Could not resolve one of the connection pins");

    unlink(source, target);
}

void OrchestratorEditorGraphPanel::_popup_request(const Vector2& p_position) {
    _popup_menu(p_position);
}

void OrchestratorEditorGraphPanel::_node_selected(Node* p_node) {
}

void OrchestratorEditorGraphPanel::_node_deselected(Node* p_node) {
    // Only clear the inspector when nothing remains selected. Clearing unconditionally
    // races with the new node's selection: Godot selects the incoming node first, then
    // deselects the old one, so an unconditional clear wipes out the inspector that
    // the new node's _on_selected just populated.
    if (get_selected<GraphElement>().is_empty()) {
        EI->inspect_object(nullptr);
    }
}

template <typename T>
static TypedArray<T> set_to_typed_array(const HashSet<T*>& p_set) {
    TypedArray<T> results;
    for (T* entry : p_set) {
        results.push_back(entry);
    }
    return results;
}

void OrchestratorEditorGraphPanel::_delete_nodes_request(const PackedStringArray& p_names) {
    // In Godot 4.2, there is a case where this method can be called with no values
    if (p_names.is_empty()) {
        return;
    }

    HashSet<OrchestratorEditorGraphNode*> node_set;
    for (const String& name : p_names) {
        GraphElement* element = cast_to<GraphElement>(find_child(name, false, false));
        if (!element) {
            continue;
        }

        if (OrchestratorEditorGraphFrame* frame = cast_to<OrchestratorEditorGraphFrame>(element)) {
            remove_frame(frame, false);
        } else if (OrchestratorEditorGraphNode* node = cast_to<OrchestratorEditorGraphNode>(element)) {
            node_set.insert(node);
        }
    }

    const uint32_t node_count = node_set.size();
    const TypedArray<OrchestratorEditorGraphNode> node_array = set_to_typed_array(node_set);

    if (node_count > 0) {
        remove_nodes(node_array);
    }
}

void OrchestratorEditorGraphPanel::_connection_drag_started(const StringName& p_from, int p_port, bool p_output) {
    ERR_FAIL_COND_MSG(!p_from.is_valid_int(), "Drag from node name is expected to be an integer value");

    PinHandle handle;
    handle.node_id = p_from.to_int();
    handle.pin_port = p_port;

    OrchestratorEditorGraphPin* pin = _resolve_pin_from_handle(handle, !p_output);
    ERR_FAIL_NULL_MSG(pin, "Failed to resolve drag from pin");

    _drag_from_pin = pin;

    if (p_output && _disconnect_control_flow_when_dragged && _drag_from_pin->is_execution()) {
        if (_drag_from_pin->is_linked()) {
            unlink_all(_drag_from_pin);
        }
    }

    emit_signal("connection_pin_drag_started", _drag_from_pin.get());
}

void OrchestratorEditorGraphPanel::_connection_drag_ended() {
    emit_signal("connection_pin_drag_ended");
}

void OrchestratorEditorGraphPanel::_copy_nodes_request() {
    const Vector<OrchestratorEditorGraphNode*> selected_nodes = get_selected<OrchestratorEditorGraphNode>();
    if (!selected_nodes.is_empty() && !_can_duplicate_nodes(selected_nodes)) {
        return;
    }

    _clipboard.copy(selected_nodes, _graph);
}

void OrchestratorEditorGraphPanel::_cut_nodes_request() {
    const Vector<OrchestratorEditorGraphNode*> selected = get_selected<OrchestratorEditorGraphNode>();
    if (selected.is_empty() || !_can_duplicate_nodes(selected)) {
        return;
    }

    const OrchestratorEditorGraphClipboard::ClipboardResult result = _clipboard.copy(selected, _graph);
    if (result.added_nodes.is_empty()) {
        return;
    }

    for (OrchestratorEditorGraphNode* node : selected) {
        remove_node(node, false);
    }
}

void OrchestratorEditorGraphPanel::_duplicate_nodes_request() {
    const Vector<OrchestratorEditorGraphNode*> selected = get_selected<OrchestratorEditorGraphNode>();
    if (selected.is_empty() || !_can_duplicate_nodes(selected)) {
        return;
    }

    OrchestratorEditorGraphClipboard::ClipboardResult result = _clipboard.duplicate(selected, _graph, Vector2(25, 25));
    if (result.added_nodes.is_empty()) {
        return;
    }

    _set_edited(true);
    _refresh_panel_connections_with_model();

    emit_signal("nodes_changed");

    clear_selections();

    for (uint64_t node_id : result.added_nodes) {
        find_node(node_id)->set_selected(true);
    }
}

void OrchestratorEditorGraphPanel::_paste_nodes_request() {
    const Vector2 offset = (get_scroll_offset() + get_local_mouse_position()) / get_zoom();

    OrchestratorEditorGraphClipboard::ClipboardResult result = _clipboard.paste(
        _graph, offset, is_snapping_enabled(), get_snapping_distance());

    if (result.added_nodes.is_empty()) {
        ORCHESTRATOR_ERROR("No nodes were pasted");
    }

    _refresh_panel_connections_with_model();

    emit_signal("nodes_changed");

    clear_selections();

    for (uint64_t node_id : result.added_nodes) {
        find_node(node_id)->set_selected(true);
    }

    _set_edited(true);

    if (result.had_skipped_nodes()) {
        String message = "Several nodes were not pasted due to the following reasons:\n\n";
        for (const KeyValue<StringName, String>& E : result.skipped_functions) {
            message += "* Function " + E.key + ": " + E.value + "\n";
        }
        for (const KeyValue<StringName, String>& E : result.skipped_events) {
            message += "* Event " + E.key + ": " + E.value + "\n";
        }
        for (const KeyValue<StringName, String>& E : result.skipped_variables) {
            message += "* Variable " + E.key + ": " + E.value + "\n";
        }
        for (const KeyValue<StringName, String>& E : result.skipped_signals) {
            message += "* Signal " + E.key + ": " + E.value + "\n";
        }
        ORCHESTRATOR_ERROR(message);
    }
}

void OrchestratorEditorGraphPanel::_begin_node_move() {
    _moving_selection = true;
}

void OrchestratorEditorGraphPanel::_end_node_move() {
    _moving_selection = false;
}

void OrchestratorEditorGraphPanel::_scroll_offset_changed(const Vector2& p_scroll_offset) {
}

void OrchestratorEditorGraphPanel::_update_panel_hint() {
    if (!is_inside_tree()) {
        return;
    }

    if (!_graph.is_valid() || !_graph->get_orchestration()) {
        return;
    }

    if (_graph->get_orchestration()->get_tool()) {
        Ref<StyleBox> sb = SceneUtils::get_editor_stylebox("panel", "GraphEdit");
        if (sb.is_valid()) {
            Ref<StyleBoxFlat> sbf = sb;
            if (sbf.is_valid()) {
                sbf = sbf->duplicate(true);
                sbf->set_bg_color(ORCHESTRATOR_GET("interface/theme/tool_scripts/background_color", Color(1,1,0,.1)));
                add_theme_stylebox_override("panel", sbf);
            }
        }
        if (!find_child("ToolScriptWarning", false, false)) {
            Label* label = memnew(Label);
            label->set_name("ToolScriptWarning");
            label->set_anchor_and_offset(SIDE_TOP, ANCHOR_END, 0);
            label->set_anchor_and_offset(SIDE_BOTTOM, ANCHOR_END, -25);
            label->set_anchor_and_offset(SIDE_RIGHT, ANCHOR_END, 0);
            label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
            label->set_vertical_alignment(VERTICAL_ALIGNMENT_BOTTOM);
            label->set_text("Godot tool scripts permanently modify your scenes, so be careful how and what you save.");
            add_child(label);
        }
    } else {
        remove_theme_stylebox_override("panel");
        if (Node* node = find_child("ToolScriptWarning", false, false)) {
            node->queue_free();
        }
    }
}

void OrchestratorEditorGraphPanel::_connect_graph_node_pin_signals(OrchestratorEditorGraphNode* p_node) {
    GUARD_NULL(p_node);

    const Callable context_menu_requested_cb = callable_mp_this(_show_pin_context_menu);
    const Callable default_value_changed_cb = callable_mp_this(_pin_default_value_changed);

    for (OrchestratorEditorGraphPin* pin : p_node->get_pins()) {
        if (!pin->is_connected("context_menu_requested", context_menu_requested_cb)) {
            pin->connect("context_menu_requested", context_menu_requested_cb);
        }
        if (!pin->is_connected("default_value_changed", default_value_changed_cb)) {
            pin->connect("default_value_changed", default_value_changed_cb);
        }
    }
}

void OrchestratorEditorGraphPanel::_disconnect_graph_node_pin_signals(OrchestratorEditorGraphNode* p_node) {
    GUARD_NULL(p_node);

    const Callable context_menu_requested_cb = callable_mp_this(_show_pin_context_menu);
    const Callable default_value_changed_cb = callable_mp_this(_pin_default_value_changed);

    for (OrchestratorEditorGraphPin* pin : p_node->get_pins()) {
        if (pin->is_connected("context_menu_requested", context_menu_requested_cb)) {
            pin->disconnect("context_menu_requested", context_menu_requested_cb);
        }
        if (pin->is_connected("default_value_changed", default_value_changed_cb)) {
            pin->disconnect("default_value_changed", default_value_changed_cb);
        }
    }
}

void OrchestratorEditorGraphPanel::_double_click_node_jump_request(OrchestratorEditorGraphNode* p_node) {
    GUARD_NULL(p_node);

    if (p_node->can_jump_to_definition()) {
        Object* definition_object = p_node->get_definition_object();
        if (definition_object) {
            emit_signal("focus_requested", definition_object);
            accept_event();
        }
    }
}

void OrchestratorEditorGraphPanel::_show_node_context_menu(OrchestratorEditorGraphNode* p_node, const Vector2& p_position) {
    ERR_FAIL_NULL_MSG(p_node, "Cannot create context menu for an invalid pin.");
    accept_event();

    p_node->set_selected(true);

    const bool are_multiple_selections = get_selection_count() > 1;
    const bool is_reroute = cast_to<OrchestratorEditorGraphNodeReroute>(p_node);

    OrchestratorEditorContextMenu* menu = memnew(OrchestratorEditorContextMenu);
    menu->set_auto_destroy(true);
    add_child(menu);

    const Ref<OrchestrationGraphNode> script_node = p_node->_node;

    menu->add_separator(is_reroute ? "Reroute Actions" : "Node Actions");

    List<Ref<OScriptAction>> script_node_actions;
    script_node->get_actions(script_node_actions);
    for (const Ref<OScriptAction>& node_action : script_node_actions) {
        if (node_action->get_icon().is_empty()) {
            menu->add_item(node_action->get_text(), node_action->get_handler());
        } else {
            menu->add_icon_item(node_action->get_icon(), node_action->get_text(), node_action->get_handler());
        }
    }

    const bool can_delete = p_node->can_user_delete_node();
    menu->add_icon_shortcut("Remove", ED_ACTION_SHORTCUT("ui_graph_delete", "Delete"), callable_mp_this(remove_selected_nodes).bind(true), !can_delete);

    menu->add_icon_shortcut("ActionCut", ED_ACTION_SHORTCUT("ui_cut", "Cut"), callable_mp_this(_cut_nodes_request), false);
    menu->add_icon_shortcut("ActionCopy", ED_ACTION_SHORTCUT("ui_copy", "Copy"), callable_mp_this(_copy_nodes_request), false);
    menu->add_icon_shortcut("Duplicate", ED_ACTION_SHORTCUT("ui_graph_duplicate", "Duplicate"), callable_mp_this(_duplicate_nodes_request), false);

    if (is_reroute) {
        menu->set_position(p_node->get_screen_position() + p_position * get_zoom());
        menu->popup();
        return;
    }

    menu->add_icon_shortcut("DistractionFree", ED_GET_SHORTCUT("graph_editor/toggle_resizer"), callable_mp_this(_toggle_resizer_for_selected_nodes));
    menu->add_icon_shortcut("KeepAspect", ED_GET_SHORTCUT("graph_editor/resize_to_content"), callable_mp_this(_resize_selected_nodes_to_content));

    bool has_connections = !get_connected_nodes(p_node).is_empty();
    menu->add_icon_shortcut("Loop", ED_GET_SHORTCUT("graph_editor/refresh_nodes"), callable_mp_this(_refresh_selected_nodes));
    menu->add_icon_shortcut("Unlinked", ED_GET_SHORTCUT("graph_editor/break_node_links"), callable_mp_this(unlink_node_all).bind(p_node), !has_connections);


    if (_can_call_parent_function(p_node)) {
        menu->add_icon_shortcut("Override", ED_GET_SHORTCUT("graph_editor/add_call_parent_function"), callable_mp_this(_create_call_to_parent_function).bind(p_node));
    }

    if (!are_multiple_selections) {
        menu->add_icon_shortcut("Anchor", ED_GET_SHORTCUT("graph_editor/bookmark/toggle"), callable_mp(_markers, &OrchestratorEditorGraphMarkers::toggle_bookmark).bind(p_node));
    }

    if (p_node->is_add_pin_button_visible() && !are_multiple_selections) {
        menu->add_shortcut(ED_GET_SHORTCUT("graph_editor/add_option_pin"), callable_mp_this(_add_node_pin).bind(p_node));
    }

    const Ref<OScriptNodeCallFunction> call_function = script_node;
    if (call_function.is_valid()) {
        menu->add_separator("Settings");
        menu->add_check_shortcut(ED_GET_SHORTCUT("graph_editor/await_function"), callable_mp_this(_toggle_await_function).bind(p_node), call_function->is_awaited(), false);
    }

    menu->add_separator("Organization");

    GraphFrame* parent_frame = get_element_frame(p_node->get_name());
    if (parent_frame != nullptr) {
        menu->add_shortcut(ED_GET_SHORTCUT("graph_editor/detach_from_frame"), callable_mp_this(_detach_node_from_frame).bind(p_node->get_name()));
    }

    const bool can_expand = cast_to<OScriptNodeCallScriptFunction>(script_node.ptr()) != nullptr;
    menu->add_shortcut(ED_GET_SHORTCUT("graph_editor/expand_node"), callable_mp_this(_expand_node).bind(p_node), !can_expand);
    menu->add_shortcut(ED_GET_SHORTCUT("graph_editor/collapse_to_function"), callable_mp_this(_collapse_selected_nodes_to_function));

    OrchestratorEditorContextMenu* align = menu->add_submenu("Alignment");
    align->add_icon_shortcut("ControlAlignTopWide", ED_GET_SHORTCUT("graph_editor/alignment/align_top"), callable_mp_this(_align_nodes).bind(p_node, ALIGN_TOP));
    align->add_icon_shortcut("ControlAlignHCenterWide", ED_GET_SHORTCUT("graph_editor/alignment/align_middle"), callable_mp_this(_align_nodes).bind(p_node, ALIGN_MIDDLE));
    align->add_icon_shortcut("ControlAlignBottomWide", ED_GET_SHORTCUT("graph_editor/alignment/align_bottom"), callable_mp_this(_align_nodes).bind(p_node, ALIGN_BOTTOM));
    align->add_icon_shortcut("ControlAlignLeftWide", ED_GET_SHORTCUT("graph_editor/alignment/align_left"), callable_mp_this(_align_nodes).bind(p_node, ALIGN_LEFT));
    align->add_icon_shortcut("ControlAlignVCenterWide", ED_GET_SHORTCUT("graph_editor/alignment/align_center"), callable_mp_this(_align_nodes).bind(p_node, ALIGN_CENTER));
    align->add_icon_shortcut("ControlAlignRightWide", ED_GET_SHORTCUT("graph_editor/alignment/align_right"), callable_mp_this(_align_nodes).bind(p_node, ALIGN_RIGHT));

    if (!are_multiple_selections) {
        menu->add_separator("Breakpoints");
        menu->add_shortcut(ED_GET_SHORTCUT("graph_editor/breakpoint/toggle"), callable_mp(_markers, &OrchestratorEditorGraphMarkers::toggle_breakpoint).bind(p_node), false);

        const bool has_breakpoints = _markers->has_breakpoint(script_node->get_id());
        const bool has_active_breakpoint = _markers->is_breakpoint_enabled(script_node->get_id());

        menu->add_item(
            vformat("%s breakpoint", has_breakpoints ? "Remove" : "Add"),
            callable_mp(_markers, &OrchestratorEditorGraphMarkers::set_breakpoint).bind(p_node, !has_breakpoints));

        if (has_breakpoints) {
            menu->add_shortcut(
                has_active_breakpoint
                    ? ED_GET_SHORTCUT("graph_eidtor/breakpoint/disable")
                    : ED_GET_SHORTCUT("graph_editor/breakpoint/enable"),
                callable_mp(_markers, &OrchestratorEditorGraphMarkers::set_breakpoint_enabled).bind(p_node, !has_active_breakpoint));
        }
    }

    menu->add_separator("Documentation");

    const String view_doc_topic = script_node->get_help_topic();
    menu->add_icon_shortcut("Help", ED_GET_SHORTCUT("graph_editor/view_documentation"), callable_mp_this(_view_documentation).bind(view_doc_topic));

    const Ref<OScriptNodeVariableGet> variable_get = script_node;
    if (variable_get.is_valid() && variable_get->can_be_validated()) {
        menu->add_separator("Variable Get");

        const String label = variable_get->is_validated() ? "Make Pure" : "Make Validated";
        menu->add_item(label, callable_mp_this(_set_variable_node_validation)
            .bind(p_node, !variable_get->is_validated()));
    }

    menu->set_position(p_node->get_screen_position() + p_position * get_zoom());
    menu->popup();
}

void OrchestratorEditorGraphPanel::_node_position_changed(const Vector2& p_old_position, const Vector2& p_new_position, OrchestratorEditorGraphNode* p_node) {
    ERR_FAIL_NULL_MSG(p_node, "Cannot update node position with an invalid node reference");
    if (p_node->_node->get_position() != p_new_position) {
        p_node->_node->set_position(p_new_position);
        p_node->set_position_offset(p_new_position);
        _set_edited(true);
    }
}

void OrchestratorEditorGraphPanel::_node_resized(OrchestratorEditorGraphNode* p_node) {
    ERR_FAIL_NULL_MSG(p_node, "Cannot update node position with an invalid node reference");
    _node_resize_end(p_node->get_position(), p_node);
}

void OrchestratorEditorGraphPanel::_node_resize_end(const Vector2& p_size, OrchestratorEditorGraphNode* p_node) {
    ERR_FAIL_NULL_MSG(p_node, "Cannot update node position with an invalid node reference");
    if (p_node->_node->get_size() != p_size) {
        p_node->_node->set_size(p_size);
        _set_edited(true);
    }
}

void OrchestratorEditorGraphPanel::_show_pin_context_menu(OrchestratorEditorGraphPin* p_pin, const Vector2& p_position) {
    ERR_FAIL_NULL_MSG(p_pin, "Cannot create context menu for an invalid pin.");
    accept_event();

    // Pin context-menu only operates on the current pin's node, so deselect any existing selections
    for (GraphElement* element : get_selected<GraphElement>()) {
        element->set_selected(false);
    }

    OrchestratorEditorGraphNode* owning_node = p_pin->get_graph_node();
    owning_node->set_selected(true);

    OrchestratorEditorContextMenu* menu = memnew(OrchestratorEditorContextMenu);
    menu->set_auto_destroy(true);
    add_child(menu);

    menu->add_separator("Pin Actions");

    const HashSet<OrchestratorEditorGraphPin*> pin_connections = get_connected_pins(p_pin);

    if (p_pin->is_linked() && p_pin->is_execution()) {
        const String label = vformat("Select All %s Nodes", p_pin->get_direction() == PD_Input ? "Input" : "Output");
        menu->add_item(label, callable_mp_this(_select_connected_execution_pins).bind(p_pin));
    }

    const Ref<OrchestrationGraphNode> script_node = owning_node->_node;
    const Ref<OrchestrationGraphPin> script_pin = p_pin->_pin;

    const Ref<OScriptEditablePinNode>& editable_node = script_node;
    if (editable_node.is_valid() && editable_node->can_remove_dynamic_pin(script_pin)) {
        const Ref<OScriptNodeMakeDictionary> make_dict = script_node;
        const String label = make_dict.is_valid() ? "Remove key/value pair" : "Remove pin";
        menu->add_item(label, callable_mp_this(_remove_node_pin).bind(p_pin));
    }

    if (script_node->can_change_pin_type()) {
        const Vector<Variant::Type> options = script_node->get_possible_pin_types();
        if (!options.is_empty()) {
            OrchestratorEditorContextMenu* submenu = menu->add_submenu("Change Pin Type");
            for (Variant::Type option : options) {
                const String label = VariantUtils::get_friendly_type_name(option, true).capitalize();
                submenu->add_item(label, callable_mp_this(_change_node_pin_type).bind(p_pin, option));
            }
        }
    }

    if (pin_connections.size() > 1) {
        menu->add_icon_item("Unlinked", "Break All Pin Links", callable_mp_this(unlink_all).bind(p_pin, true));

        OrchestratorEditorContextMenu* submenu = menu->add_submenu("Break Link To...");
        for (OrchestratorEditorGraphPin* connection : pin_connections) {
            const String node_name = connection->get_graph_node()->get_title();
            const String pin_name = connection->get_pin_name().capitalize();

            const String label = vformat("Break Pin Link to %s - %s", node_name, pin_name);
            submenu->add_item(label, callable_mp_this(unlink).bind(p_pin, connection));
        }
    } else {
        Callable callback;
        if (pin_connections.size() == 1) {
            OrchestratorEditorGraphPin* link = *pin_connections.begin();
            callback = callable_mp_this(unlink).bind(p_pin, link);
        }

        menu->add_icon_item("Unlinked", "Break This Link", callback, pin_connections.is_empty());
    }

    if (!pin_connections.is_empty()) {
        {
            OrchestratorEditorContextMenu* submenu = menu->add_submenu("Jump to connected node...");
            for (OrchestratorEditorGraphPin* connection : pin_connections) {
                const int node_id = connection->get_graph_node()->get_id();
                const String node_name = connection->get_graph_node()->get_title();

                const String label = vformat("Jump to %d - %s", node_id, node_name);
                submenu->add_item(label, callable_mp_this(center_node).bind(connection->get_graph_node()));
            }
        }
        {
            OrchestratorEditorContextMenu* submenu = menu->add_submenu("Straighten Connection...");
            if (pin_connections.size() > 1) {
                submenu->add_item("Straighten All Pin Connections", callable_mp_this(straighten_all_connections).bind(p_pin));
            }

            for (OrchestratorEditorGraphPin* connection : pin_connections) {
                const String node_name = connection->get_graph_node()->get_title();
                const String pin_name = connection->get_pin_name();

                const String label = vformat("Straighten Connection to %s (%s)", node_name, pin_name);
                submenu->add_item(label, callable_mp_this(straighten_connection).bind(p_pin, connection));
            }
        }
    }

    if (_can_promote_pin_to_variable(p_pin)) {
        menu->add_item("Promote to Variable", callable_mp_this(_promote_pin_to_variable).bind(p_pin));
    }

    if (!p_pin->is_execution() && pin_connections.is_empty() && p_pin->is_connectable() && p_pin->get_direction() == PD_Input) {
        menu->add_item("Reset to Default Value", callable_mp_this(_reset_pin_to_generated_default_value).bind(p_pin));
    }

    menu->add_separator("Documentation");

    const String view_doc_topic = script_node->get_help_topic();
    menu->add_icon_shortcut("Help", ED_GET_SHORTCUT("graph_editor/view_documentation"), callable_mp_this(_view_documentation).bind(view_doc_topic));

    menu->set_position(p_pin->get_screen_position() + p_position * get_zoom());
    menu->popup();
}

void OrchestratorEditorGraphPanel::_pin_default_value_changed(OrchestratorEditorGraphPin* p_pin, const Variant& p_value) {
    ERR_FAIL_NULL_MSG(p_pin, "Cannot update pin default value with an invalid pin reference");

    const Variant old_value = p_pin->_pin->get_effective_default_value();
    if (old_value != p_value) {
        p_pin->_pin->set_default_value(p_value);
        _set_edited(true);
    }
}

void OrchestratorEditorGraphPanel::_node_added(int p_node_id) {
    if (_graph->has_node(p_node_id)) {
        const Ref<OrchestrationGraphNode> node = _graph->get_node(p_node_id);
        if (node.is_valid()) {
            _add_node_to_panel(node);
            _update_center_status();

            emit_signal("validate_script");
        }
    }
}

void OrchestratorEditorGraphPanel::_node_removed(int p_node_id) {
    OrchestratorEditorGraphNode* node = find_node(p_node_id);
    if (node) {
        remove_node(node, false);
    }

    _update_center_status();
    emit_signal("validate_script");
}

void OrchestratorEditorGraphPanel::_graph_changed() {
    // Graph was renamed
    if (_graph.is_valid() && _graph->get_graph_name() != get_name()) {
        set_name(_graph->get_graph_name());
    }
}


void OrchestratorEditorGraphPanel::_clear_copy_buffer() {
    _clipboard.clear();
}

void OrchestratorEditorGraphPanel::_toggle_resizer_for_selected_nodes() {
    for (OrchestratorEditorGraphNode* node : get_selected<OrchestratorEditorGraphNode>()) {
        node->set_resizable(!node->is_resizable());
    }
}

void OrchestratorEditorGraphPanel::_resize_selected_nodes_to_content() {
    for (OrchestratorEditorGraphNode* node : get_selected<OrchestratorEditorGraphNode>()) {
        node->_resize_to_content();
        _set_edited(true);
    }
}

void OrchestratorEditorGraphPanel::_refresh_selected_nodes() {
    for (OrchestratorEditorGraphNode* node : get_selected<OrchestratorEditorGraphNode>()) {
        node->_node->reconstruct_node();
    }
}

void OrchestratorEditorGraphPanel::_add_node_pin(OrchestratorEditorGraphNode* p_node) {
    ERR_FAIL_NULL_MSG(p_node, "Cannot add node pin to an invalid node reference");

    const Ref<OScriptEditablePinNode> editable_node = p_node->_node;
    if (editable_node.is_valid() && editable_node->can_add_dynamic_pin()) {
        editable_node->add_dynamic_pin();
        _set_edited(true);
    }
}

void OrchestratorEditorGraphPanel::_expand_node(OrchestratorEditorGraphNode* p_node) {
    const Ref<OScriptNodeCallScriptFunction> call_script_function = p_node->_node;
    if (!call_script_function.is_valid()) {
        ORCHESTRATOR_ERROR(vformat("Node '%s' is not a call script function node and can't be expanded", p_node->get_title()));
    }

    const Ref<OScriptFunction> function = call_script_function->get_function();
    if (!function.is_valid()) {
        ORCHESTRATOR_ERROR(vformat("Function the node references cannot be found."));
    }

    Rect2 nodes_area;
    HashSet<int> nodes_to_duplicate;
    const Ref<OrchestrationGraph> function_graph = function->get_function_graph();
    for (const Ref<OrchestrationGraphNode>& node : function_graph->get_nodes()) {
        const Ref<OScriptNodeFunctionEntry> entry = node;
        const Ref<OScriptNodeFunctionResult> result = node;

        if (!entry.is_valid() && !result.is_valid() && node->can_duplicate()) {
            const Rect2 node_rect = Rect2(node->get_position(), node->get_size());
            if (nodes_to_duplicate.is_empty()) {
                nodes_area = node_rect;
            } else {
                nodes_area = nodes_area.merge(node_rect);
            }
            nodes_to_duplicate.insert(node->get_id());
        }
    }

    if (!nodes_to_duplicate.is_empty()) {
        const Vector2 position_delta = p_node->get_graph_rect().get_center() - nodes_area.get_center();

        HashMap<int, int> connection_remap;
        for (const int node_id : nodes_to_duplicate) {
            const Ref<OrchestrationGraphNode> new_node = _graph->duplicate_node(node_id, position_delta, true);
            ERR_CONTINUE(!new_node.is_valid());

            connection_remap[node_id] = new_node->get_id();
        }

        for (const Connection& C : _graph->get_orchestration()->get_connections()) {
            if (connection_remap.has(C.from_node) && connection_remap.has(C.to_node)) {
                _graph->link(connection_remap[C.from_node], C.from_port, connection_remap[C.to_node], C.to_port);
            }
        }
    }

    remove_node(p_node, false);
    _set_edited(true);
}

void OrchestratorEditorGraphPanel::_collapse_selected_nodes_to_function() {
    const Vector<OrchestratorEditorGraphNode*> selected_nodes = get_selected<OrchestratorEditorGraphNode>();
    if (selected_nodes.is_empty()) {
        return;
    }

    if (!_can_duplicate_nodes(selected_nodes)) {
        return;
    }

    int input_executions = 0;
    int output_executions = 0;
    int input_data = 0;
    int output_data = 0;
    HashSet<int> node_set;
    for (OrchestratorEditorGraphNode* node : selected_nodes) {
        node_set.insert(node->get_id());

        const Vector<OrchestratorEditorGraphPin*> node_pins = node->get_pins();
        for (OrchestratorEditorGraphPin* pin : node_pins) {
            const HashSet<OrchestratorEditorGraphPin*> connected_pins = get_connected_pins(pin);
            for (OrchestratorEditorGraphPin* connected_pin : connected_pins) {
                if (!selected_nodes.has(connected_pin->get_graph_node())) {
                    if (pin->get_direction() == PD_Input && pin->is_execution()) {
                        input_executions++;
                    } else if (pin->get_direction() == PD_Input) {
                        input_data++;
                    } else if (pin->get_direction() == PD_Output && pin->is_execution()) {
                        output_executions++;
                    } else if (pin->get_direction() == PD_Output) {
                        output_data++;
                    }
                }
            }
        }
    }

    HashSet<uint64_t> connections;
    HashSet<uint64_t> input_connections;
    HashSet<uint64_t> output_connections;
    for (const Connection& C : _graph->get_orchestration()->get_connections()) {
        if (node_set.has(C.from_node) && node_set.has(C.to_node)) {
            connections.insert(C.id);
        }
        if (!node_set.has(C.from_node) && node_set.has(C.to_node)) {
            input_connections.insert(C.id);
        }
        if (node_set.has(C.from_node) && !node_set.has(C.to_node)) {
            output_connections.insert(C.id);
        }
    }

    ERR_FAIL_COND_EDMSG(input_executions > 1, "Cannot collapse with more than one external input execution wire.");
    ERR_FAIL_COND_EDMSG(output_executions > 1, "Cannot collapse with more than one external output execution wire.");
    ERR_FAIL_COND_EDMSG(output_data > 1, "Cannot collapse to function with more than one output data wire.");
    ERR_FAIL_COND_EDMSG(output_connections.size() > 2, "Cannot output more than one execution and one data pin.");

    const StringName function_name = NameUtils::create_unique_name("NewFunction", _graph->get_orchestration()->get_function_names());
    if (!_create_new_function(function_name, !output_connections.is_empty())) {
        ORCHESTRATOR_ERROR("Failed to create new function for collapse");
    }

    const Ref<OScriptFunction> function = _graph->get_orchestration()->find_function(function_name);

    const Ref<OrchestrationGraph> source_graph = _graph;
    const Ref<OrchestrationGraph> target_graph = function->get_function_graph();

    const Rect2 selected_node_area = get_bounds_for_nodes(selected_nodes);

    // Before moving the nodes, their connections to non-collapsed nodes must be severed
    for (uint64_t connection_id : input_connections) {
        const Connection C(connection_id);
        source_graph->unlink(C.from_node, C.from_port, C.to_node, C.to_port);
    }

    for (uint64_t connection_id : output_connections) {
        const Connection C(connection_id);
        source_graph->unlink(C.from_node, C.from_port, C.to_node, C.to_port);
    }

    // Transfer the nodes between the graphs
    for (OrchestratorEditorGraphNode* node : selected_nodes) {
        source_graph->move_node_to(node->_node, target_graph);
    }

    // Reapply connections in new graph
    for (uint64_t connection_id : connections) {
        const Connection C(connection_id);
        target_graph->link(C.from_node, C.from_port, C.to_node, C.to_port);
    }

    // Spawn the call functino node in the source graph
    NodeSpawnOptions options;
    options.node_class = OScriptNodeCallScriptFunction::get_class_static();
    options.context.method = function->get_method_info();
    options.position = selected_node_area.get_center();

    OrchestratorEditorGraphNode* call_function = spawn_node(options).node;

    int call_input_index = 1;
    int input_index = 1;
    bool input_execution_wired = false;
    bool call_execution_wired = false;
    bool entry_positioned = false;
    for (uint64_t connection_id : input_connections) {
        const Connection C(connection_id);

        const Ref<OrchestrationGraphNode> source = _graph->get_orchestration()->get_node(C.from_node);
        const Ref<OrchestrationGraphPin> source_pin = source->find_pins(PD_Output)[C.from_port];
        if (source_pin->is_execution() && !call_execution_wired) {
            source_graph->link(C.from_node, C.from_port, call_function->get_id(), 0);
            call_execution_wired = true;
        } else {
            source_graph->link(C.from_node, C.from_port, call_function->get_id(), call_input_index++);
        }

        const Ref<OrchestrationGraphNode> target = _graph->get_orchestration()->get_node(C.to_node);
        const Ref<OrchestrationGraphPin> target_pin = target->find_pins(PD_Input)[C.to_port];

        if (!entry_positioned) {
            const Ref<OrchestrationGraphNode> entry = _graph->get_orchestration()->get_node(function->get_owning_node_id());
            entry->set_position(target->get_position() - Vector2(250, 0));
            entry->emit_changed();
            entry_positioned = true;
        }

        if (!target_pin->is_execution()) {
            const size_t size = function->get_argument_count() + 1;
            function->resize_argument_list(size);

            PropertyInfo property = target_pin->get_property_info();
            if (!target_pin->get_label().is_empty() && property.name != target_pin->get_label()) {
                property.name = target_pin->get_label();
            }

            PackedStringArray names;
            for (const PropertyInfo& argument : function->get_method_info().arguments) {
                if (!names.has(argument.name)) {
                    names.push_back(argument.name);
                }
            }

            if (names.has(property.name)) {
                property.name = NameUtils::create_unique_name(property.name, names);
            }

            function->set_argument(size - 1, property);

            // Wire entry data output to this connection
            target_graph->link(function->get_owning_node_id(), input_index++, C.to_node, C.to_port);
        } else if (!input_execution_wired) {
            // Wire entry execution output to this connection
            target_graph->link(function->get_owning_node_id(), 0, C.to_node, C.to_port);
            input_execution_wired = true;
        }
    }

    const Ref<OrchestrationGraphNode> result = function->get_return_node();
    if (result.is_valid()) {
        bool output_execution_wired = false;
        bool output_data_wired = false;
        bool positioned = false;

        for (uint64_t connection_id : output_connections) {
            const Connection C(connection_id);

            const Ref<OrchestrationGraphNode> source = _graph->get_orchestration()->get_node(C.from_node);
            const Ref<OrchestrationGraphPin> source_pin = source->find_pins(PD_Output)[C.from_port];

            if (!positioned) {
                result->set_position(source->get_position() + Vector2(250, 0));
                result->emit_changed();
                positioned = true;
            }

            if (source_pin->is_execution() && !output_execution_wired) {
                // Connect execution
                target_graph->link(C.from_node, C.from_port, result->get_id(), 0);
                output_execution_wired = true;
            } else if (!source_pin->is_execution() && !output_data_wired) {
                // Connect data
                function->set_has_return_value(true);
                function->set_return_type(source_pin->get_type());

                target_graph->link(C.from_node, C.from_port, result->get_id(), 1);
                output_data_wired = true;
            }
        }

        const Ref<OrchestrationGraphPin> result_exec = result->find_pin(0, PD_Output);
        if (result_exec.is_valid() && !result_exec->has_any_connections()) {
            const Ref<OrchestrationGraphNode> entry = function->get_owning_node();
            const Ref<OrchestrationGraphPin> entry_exec = entry->find_pin(0, PD_Output);
            if (entry_exec.is_valid() && !entry_exec->has_any_connections()) {
                entry_exec->link(result_exec);
                if (entry->find_pins(PD_Output).size() == 1) {
                    entry->set_position(result->get_position() - Vector2(250, 0));
                    entry->emit_changed();
                }
            }
        }
    }

    // Finally wire up the call node in the main graph
    int call_output_index = 1;
    call_execution_wired = false;
    for (uint64_t connection_id : output_connections) {
        const Connection C(connection_id);

        // Get the exterior node connected to the selected node
        const Ref<OrchestrationGraphNode> target = _graph->get_orchestration()->get_node(C.to_node);
        const Ref<OrchestrationGraphPin> target_pin = target->find_pins(PD_Input)[C.to_port];
        if (target_pin->is_execution() && !call_execution_wired) {
            source_graph->link(call_function->get_id(), 0, C.to_node, C.to_port);
            call_execution_wired = true;
        } else if (!target_pin->is_execution()) {
            source_graph->link(call_function->get_id(), call_output_index++, C.to_node, C.to_port);
        }
    }

    call_function->_node->emit_changed();
    _set_edited(true);

    _refresh_panel_connections_with_model();

    emit_signal("nodes_changed");
    call_deferred("emit_signal", "edit_function_requested", function->get_function_name());
}

bool OrchestratorEditorGraphPanel::_create_new_function(const String& p_name, bool p_has_return) {
    const Ref<OScriptFunction> function = _graph->get_orchestration()->create_function(p_name, p_has_return);
    if (function.is_null()) {
        return false;
    }

    _set_edited(true);

    return true;
}

bool OrchestratorEditorGraphPanel::_create_new_function_override(const MethodInfo& p_method) {
    bool user_defined = !(p_method.flags & METHOD_FLAG_VIRTUAL);

    const Ref<OScriptFunction> function = _graph->get_orchestration()->create_function(p_method, user_defined);
    if (!function.is_valid()) {
        return false;
    }

    _set_edited(true);

    call_deferred("emit_signal", "focus_requested", function);

    return true;
}

bool OrchestratorEditorGraphPanel::_can_call_parent_function(OrchestratorEditorGraphNode* p_node) {
    ERR_FAIL_NULL_V(p_node, false);

    const Ref<OrchestrationGraphNode> node = p_node->get_graph_node();
    if (node.is_valid()) {
        if (const Ref<OScriptNodeEvent>& event = node; event.is_valid()) {
            return true;
        }

        if (const Ref<OScriptNodeCallFunction>& call_func = node; call_func.is_valid() && call_func->is_override()) {
            return true;
        }

        if (const Ref<OScriptNodeFunctionEntry>& entry = node; entry.is_valid() && entry->is_override()) {
            return true;
        }
    }

    return false;
}

void OrchestratorEditorGraphPanel::_create_call_to_parent_function(OrchestratorEditorGraphNode* p_node) {
    if (!_can_call_parent_function(p_node)) {
        return;
    }

    const Ref<OrchestrationGraphNode> graph_node = p_node->get_graph_node();
    ERR_FAIL_COND(graph_node.is_null());

    if (const Ref<OScriptNodeCallScriptFunction>& node = graph_node; node.is_valid()) {
        NodeSpawnOptions options;
        options.node_class = OScriptNodeCallParentScriptFunction::get_class_static();
        options.context.method = node->get_method_info();
        options.position = p_node->get_position_offset() + Vector2(0, p_node->get_graph_rect().get_size().y + 10);

        if (spawn_node(options)) {
            _set_edited(true);
        }
    } else if (const Ref<OScriptNodeFunctionEntry>& node = graph_node; node.is_valid()) {
        NodeSpawnOptions options;
        options.node_class = OScriptNodeCallParentScriptFunction::get_class_static();
        options.context.method = node->get_function()->get_method_info();
        options.position = p_node->get_position_offset() + Vector2(0, p_node->get_graph_rect().get_size().y + 10);

        if (spawn_node(options)) {
            _set_edited(true);
        }
    } else if (const Ref<OScriptNodeCallMemberFunction>& node = graph_node; node.is_valid()) {
        StringName parent_class_name;
        if (ScriptServer::is_global_class(node->get_target_class())) {
            parent_class_name = ScriptServer::get_global_class(node->get_target_class()).base_type;
        } else {
            parent_class_name = ClassDB::get_parent_class(_graph->get_orchestration()->get_base_type());
        }

        NodeSpawnOptions options;
        options.node_class = OScriptNodeCallParentMemberFunction::get_class_static();
        options.context.method = node->get_method_info();
        options.context.class_name = parent_class_name;
        options.position = p_node->get_position_offset() + Vector2(0, p_node->get_graph_rect().get_size().y + 10);

        if (spawn_node(options)) {
            _set_edited(true);
        }
    } else if (const Ref<OScriptNodeEvent>& node = graph_node; node.is_valid()) {
        StringName parent_class_name;
        StringName global_name = _graph->get_orchestration()->get_global_name();
        if (ScriptServer::is_global_class(global_name)) {
            parent_class_name = ScriptServer::get_global_class(global_name).base_type;
        } else {
            parent_class_name = ClassDB::get_parent_class(_graph->get_orchestration()->get_base_type());
        }

        NodeSpawnOptions options;
        options.node_class = OScriptNodeCallParentScriptFunction::get_class_static();
        options.context.method = node->get_function()->get_method_info();
        options.context.class_name = parent_class_name;
        options.position = p_node->get_position_offset() + Vector2(0, p_node->get_graph_rect().get_size().y + 10);

        if (spawn_node(options)) {
            _set_edited(true);
        }
    }
}

void OrchestratorEditorGraphPanel::_align_nodes(OrchestratorEditorGraphNode* p_anchor, int p_alignment) {
    ERR_FAIL_NULL_MSG(p_anchor, "Cannot perform node alignment with an invalid anchor node reference");
    ERR_FAIL_INDEX(p_alignment, GraphNodeAlignment::ALIGN_MAX);

    #define SET_NODE_POS(node_obj, position)                            \
        node_obj->set_position_offset(position);                        \
        node_obj->_node->set_position(node_obj->get_position_offset());

    const Vector2 align_offset = p_anchor->get_position_offset();
    const Vector2 align_size = p_anchor->get_size();

    switch (p_alignment) {
        case ALIGN_TOP: {
            // Align all selected nodes to match top of this specific node.
            const float top = align_offset.y;
            for_each<OrchestratorEditorGraphNode>([&] (OrchestratorEditorGraphNode* node) {
                const float adjust = top - node->get_position_offset().y;
                SET_NODE_POS(node, node->get_position_offset() + Vector2(0, adjust));
            }, true);
            _set_edited(true);
            break;
        }
        case ALIGN_MIDDLE: {
            // Align all selected nodes to center to this specific node.
            const float mid_y = align_offset.y + (align_size.y / 2);
            for_each<OrchestratorEditorGraphNode>([&] (OrchestratorEditorGraphNode* node) {
                const float node_mid_y = node->get_position_offset().y + (node->get_size().y / 2);
                SET_NODE_POS(node, node->get_position_offset() + Vector2(0, mid_y - node_mid_y));
            }, true);
            _set_edited(true);
            break;
        }
        case ALIGN_BOTTOM: {
            // Align all selected nodes to match bottom of this specific node.
            const float bottom = align_offset.y + align_size.y;
            for_each<OrchestratorEditorGraphNode>([&] (OrchestratorEditorGraphNode* node) {
                const float adjust = bottom - (node->get_position_offset().y + node->get_size().y);
                SET_NODE_POS(node, node->get_position_offset() + Vector2(0, adjust));
            }, true);
            _set_edited(true);
            break;
        }
        case ALIGN_LEFT: {
            // Align all selected nodes to this specific node.
            const Vector2 pos = align_offset;
            for_each<OrchestratorEditorGraphNode>([&] (OrchestratorEditorGraphNode* node) {
                const float left = node->get_position_offset().x;
                SET_NODE_POS(node, node->get_position_offset() + Vector2(pos.x - left, 0));
            }, true);
            _set_edited(true);
            break;
        }
        case ALIGN_CENTER: {
            // Align all selected nodes to center to this specific node.
            const float mid_x = align_offset.x + (align_size.x / 2);
            for_each<OrchestratorEditorGraphNode>([&] (OrchestratorEditorGraphNode* node) {
                const float node_mid_x = node->get_position_offset().x + (node->get_size().x / 2);
                SET_NODE_POS(node, node->get_position_offset() + Vector2(mid_x - node_mid_x, 0));
            }, true);
            _set_edited(true);
            break;
        }
        case ALIGN_RIGHT: {
            // Align all selected nodes to this specific node.
            const float right = align_offset.x + align_size.x;
            for_each<OrchestratorEditorGraphNode>([&] (OrchestratorEditorGraphNode* node) {
                const float adjust = right - (node->get_position_offset().x + node->get_size().x);
                SET_NODE_POS(node, node->get_position_offset() + Vector2(adjust, 0));
            }, true);
            _set_edited(true);
            break;
        }
    }

    #undef SET_NODE_POS
}

void OrchestratorEditorGraphPanel::_set_variable_node_validation(OrchestratorEditorGraphNode* p_node, bool p_validated) {
    ERR_FAIL_NULL_MSG(p_node, "Cannot set variable node validation on an invalid node reference");

    // This shrinks the node when validation is toggled
    p_node->set_anchor_and_offset(SIDE_BOTTOM, 0, 0);

    const Ref<OScriptNodeVariableGet> variable_node = p_node->_node;
    if (variable_node.is_valid()) {
        variable_node->set_validated(p_validated);
        _set_edited(true);
    }
}

void OrchestratorEditorGraphPanel::_toggle_await_function(OrchestratorEditorGraphNode* p_node) {
    ERR_FAIL_NULL_MSG(p_node, "Cannot toggle function await on an invalid node reference");

    const Ref<OScriptNodeCallFunction> call_function_node = p_node->get_graph_node();
    if (call_function_node.is_valid()) {
        call_function_node->set_awaited(!call_function_node->is_awaited());
        _set_edited(true);
    }
}

void OrchestratorEditorGraphPanel::_select_connected_execution_pins(OrchestratorEditorGraphPin* p_pin) {
    ERR_FAIL_NULL_MSG(p_pin, "Cannot selected connected execution pins on an invalid pin reference");

    clear_selections();

    Vector<OrchestratorEditorGraphPin*> stack;
    stack.push_back(p_pin);

    HashSet<OrchestratorEditorGraphPin*> visited_pins;
    while (!stack.is_empty()) {
        OrchestratorEditorGraphPin* current_pin = stack[stack.size() - 1];
        stack.remove_at(stack.size() - 1);

        if (visited_pins.has(current_pin)) {
            continue;
        }

        visited_pins.insert(current_pin);

        OrchestratorEditorGraphNode* node = current_pin->get_graph_node();
        node->set_selected(true);

        // Push opposite direction connected pins onto the stack
        for (OrchestratorEditorGraphPin* pin : get_connected_pins(current_pin)) {
            if (!visited_pins.has(pin) && pin->get_direction() != p_pin->get_direction()) {
                stack.push_back(pin);
            }
        }

        // Walk sibling pins
        for (OrchestratorEditorGraphPin* node_pin : node->get_pins()) {
            if (node_pin->is_execution() && node_pin->get_direction() == p_pin->get_direction()) {
                stack.push_back(node_pin);
            }
        }
    }
}

void OrchestratorEditorGraphPanel::_remove_node_pin(OrchestratorEditorGraphPin* p_pin) {
    ERR_FAIL_NULL_MSG(p_pin, "Cannot remove dynamic pin for an invalid pin reference");

    // This shrinks the node when pins are removed
    p_pin->get_graph_node()->set_anchor_and_offset(SIDE_BOTTOM, 0, 0);

    const Ref<OScriptEditablePinNode> editable = p_pin->get_graph_node()->_node;
    if (editable.is_valid()) {
        if (editable->can_remove_dynamic_pin(p_pin->_pin)) {
            editable->remove_dynamic_pin(p_pin->_pin);
            _set_edited(true);
        }
    }
}

void OrchestratorEditorGraphPanel::_change_node_pin_type(OrchestratorEditorGraphPin* p_pin, int p_type) {
    ERR_FAIL_NULL_MSG(p_pin, "Cannot change pin type for an invalid pin reference");

    const Ref<OrchestrationGraphNode> script_node = p_pin->get_graph_node()->_node;
    if (script_node.is_valid() && script_node->can_change_pin_type()) {
        script_node->change_pin_types(VariantUtils::to_type(p_type));
        _set_edited(true);
    }

    // This shrinks the node when widget layouts change
    p_pin->get_graph_node()->set_anchor_and_offset(SIDE_BOTTOM, 0, 0);
}

bool OrchestratorEditorGraphPanel::_can_promote_pin_to_variable(OrchestratorEditorGraphPin* p_pin) {
    ERR_FAIL_NULL_V(p_pin, false);
    return !p_pin->is_execution();
}

void OrchestratorEditorGraphPanel::_promote_pin_to_variable(OrchestratorEditorGraphPin* p_pin) {
    ERR_FAIL_NULL_MSG(p_pin, "Cannot promote pin to a variable with an invalid pin reference");
    ERR_FAIL_COND_MSG(!_can_promote_pin_to_variable(p_pin), "Pin is not eligible for promotion to variable");

    int index = 0;
    String name = vformat("%s_%d", p_pin->get_pin_name(), index++);
    while (_graph->get_orchestration()->has_variable(name)) {
        name = vformat("%s_%d", p_pin->get_pin_name(), index++);
    }

    const Ref<OScriptVariable> variable = _graph->get_orchestration()->create_variable(name);
    if (variable.is_valid()) {
        const bool is_input = p_pin->get_direction() == PD_Input;
        const Vector2 port_offset = p_pin->get_graph_node()->get_port_position_for_pin(p_pin);
        const Vector2 pin_position = p_pin->get_graph_node()->get_position_offset() + port_offset;

        NodeSpawnOptions options;
        options.context.variable_name = variable->get_variable_name();
        options.position = pin_position + Vector2(250, 0) * (is_input ? -1 : 1);

        variable->set_info(p_pin->get_property_info());

        variable->set_default_value(p_pin->_pin->get_effective_default_value());

        variable->emit_changed();
        variable->notify_property_list_changed();

        _graph->get_orchestration()->mark_dirty();

        if (is_input) {
            OrchestratorEditorGraphNode* node = spawn_node<OScriptNodeVariableGet>(options).node;
            if (node) {
                link(node->get_output_pin(0), p_pin);
            }
        } else {
            OrchestratorEditorGraphNode* node = spawn_node<OScriptNodeVariableSet>(options).node;
            if (node) {
                link(node->get_input_pin(1), p_pin);
            }
        }

        _set_edited(true);
    }
}

void OrchestratorEditorGraphPanel::_reset_pin_to_generated_default_value(OrchestratorEditorGraphPin* p_pin) {
    ERR_FAIL_NULL_MSG(p_pin, "Cannot reset pin to generated default value with an invalid pin reference");

    p_pin->_pin->set_default_value(p_pin->_pin->get_generated_default_value());
    _set_edited(true);
}

void OrchestratorEditorGraphPanel::_view_documentation(const String& p_topic) {
    EI->set_main_screen_editor("Script");
    EI->get_script_editor()->goto_help(p_topic);
}

void OrchestratorEditorGraphPanel::_connect_graph_node_signals(OrchestratorEditorGraphNode* p_node) {
    GUARD_NULL(p_node);

    p_node->connect("node_pins_changed", callable_mp_this(_connect_graph_node_pin_signals));
    p_node->connect("context_menu_requested", callable_mp_this(_show_node_context_menu));
    p_node->connect("double_click_jump_request", callable_mp_this(_double_click_node_jump_request));
    p_node->connect("add_node_pin_requested", callable_mp_this(_add_node_pin));
    p_node->connect("dragged", callable_mp_this(_node_position_changed).bind(p_node));

    // Godot 4.3 introduced a new resize_end callback that we will use now to handle triggering the
    // final size of a node. This helps to avoid issues with editor scale changes being problematic
    // by leaving nodes too large after scale up.
    p_node->connect("resize_end", callable_mp_this(_node_resize_end).bind(p_node));

    _connect_graph_node_pin_signals(p_node);
}

void OrchestratorEditorGraphPanel::_disconnect_graph_node_signals(OrchestratorEditorGraphNode* p_node) {
    GUARD_NULL(p_node);

    p_node->disconnect("node_pins_changed", callable_mp_this(_connect_graph_node_pin_signals));
    p_node->disconnect("context_menu_requested", callable_mp_this(_show_node_context_menu));
    p_node->disconnect("double_click_jump_request", callable_mp_this(_double_click_node_jump_request));
    p_node->disconnect("add_node_pin_requested", callable_mp_this(_add_node_pin));
    p_node->disconnect("dragged", callable_mp_this(_node_position_changed).bind(p_node));

    // Godot 4.3 introduced a new resize_end callback that we will use now to handle triggering the
    // final size of a node. This helps to avoid issues with editor scale changes being problematic
    // by leaving nodes too large after scale up.
    p_node->disconnect("resize_end", callable_mp_this(_node_resize_end).bind(p_node));

    _disconnect_graph_node_pin_signals(p_node);
}

void OrchestratorEditorGraphPanel::_connect_graph_frame_signals(OrchestratorEditorGraphFrame* p_frame) {
    GUARD_NULL(p_frame);
    p_frame->connect("context_menu_requested", callable_mp_this(_show_frame_context_menu));
    p_frame->connect("changed", callable_mp_this(_set_edited).bind(true));
}

void OrchestratorEditorGraphPanel::_disconnect_graph_frame_signals(OrchestratorEditorGraphFrame* p_frame) {
    GUARD_NULL(p_frame);
    p_frame->disconnect("context_menu_requested", callable_mp_this(_show_frame_context_menu));
    p_frame->disconnect("changed", callable_mp_this(_set_edited).bind(true));
}

void OrchestratorEditorGraphPanel::_show_frame_context_menu(OrchestratorEditorGraphFrame* p_frame, const Vector2& p_position) {
    ERR_FAIL_NULL_MSG(p_frame, "Cannot create context menu for an invalid frame.");
    accept_event();

    p_frame->set_selected(true);

    OrchestratorEditorContextMenu* menu = memnew(OrchestratorEditorContextMenu);
    menu->set_auto_destroy(true);
    add_child(menu);

    menu->add_separator("Frame Actions");
    menu->add_icon_shortcut("Remove", ED_ACTION_SHORTCUT("ui_graph_delete", "Delete"), callable_mp_this(remove_frame).bind(p_frame, true), false);
    menu->add_icon_shortcut("ActionCopy", ED_ACTION_SHORTCUT("ui_copy", "Copy"), callable_mp_this(_copy_nodes_request), false);
    menu->add_icon_shortcut("Duplicate", ED_ACTION_SHORTCUT("ui_graph_duplicate", "Duplicate"), callable_mp_this(_duplicate_nodes_request), false);
    p_frame->build_context_menu(menu);

    GraphFrame* parent_frame = get_element_frame(p_frame->get_name());
    if (parent_frame != nullptr) {
        menu->add_separator();
        menu->add_shortcut(ED_GET_SHORTCUT("graph_editor/detach_from_frame"), callable_mp_this(_detach_node_from_frame).bind(p_frame->get_name()));
    }

    menu->set_position(p_frame->get_screen_position() + p_position * get_zoom());
    menu->popup();
}

void OrchestratorEditorGraphPanel::_graph_elements_linked_to_frame_request(const Array& p_elements, const StringName& p_frame_name) {
    if (OrchestratorEditorGraphFrame* frame = find_frame(p_frame_name)) {
        for (int i = 0; i < p_elements.size(); i++) {
            const StringName element_name = p_elements[i];
            attach_graph_element_to_frame(element_name, p_frame_name);
        }

        _save_frame_attachments(frame);
        _set_edited(true);
    }
}

void OrchestratorEditorGraphPanel::_detach_node_from_frame(const StringName& p_node_name) {
    if (GraphFrame* parent_frame = get_element_frame(p_node_name)) {
        const StringName frame_name = parent_frame->get_name();
        detach_graph_element_from_frame(p_node_name);

        if (OrchestratorEditorGraphFrame* frame = cast_to<OrchestratorEditorGraphFrame>(parent_frame)) {
            _save_frame_attachments(frame);
        }

        _set_edited(true);
    }
}

void OrchestratorEditorGraphPanel::_save_frame_attachments(OrchestratorEditorGraphFrame* p_frame) {
    GUARD_NULL(p_frame);

    const Ref<OScriptNodeComment> comment = p_frame->get_comment();
    if (comment.is_null()) {
        return;
    }

    const TypedArray<StringName> attached = get_attached_nodes_of_frame(p_frame->get_name());
    PackedInt64Array ids;
    for (int i = 0; i < attached.size(); i++) {
        const StringName name = attached[i];
        if (name.is_valid_int()) {
            ids.push_back(name.to_int());
        }
    }

    comment->set_attached_nodes(ids);
    p_frame->update_placeholder(ids.size() > 0);
}

void OrchestratorEditorGraphPanel::_restore_frame_attachments() {
    for_each<OrchestratorEditorGraphFrame>([&](OrchestratorEditorGraphFrame* frame) {
        const Ref<OScriptNodeComment> comment = frame->get_comment();
        if (comment.is_null()) {
            return;
        }

        const PackedInt64Array attached_ids = comment->get_attached_nodes();
        for (int i = 0; i < attached_ids.size(); i++) {
            const StringName node_name = itos(attached_ids[i]);
            attach_graph_element_to_frame(node_name, frame->get_name());
        }

        frame->update_placeholder(attached_ids.size() > 0);
    });
}

void OrchestratorEditorGraphPanel::_spawn_frame() {

    OScriptNodeInitContext context;
    const Vector2 offset = (get_scroll_offset() + get_local_mouse_position()) / get_zoom();
    const Ref<OScriptNodeComment> comment = _graph->create_node<OScriptNodeComment>(context, offset);

    if (comment.is_valid()) {
        comment->set_title_text("New Comment");

        _set_edited(true);
        emit_signal("nodes_changed");

        if (OrchestratorEditorGraphFrame* frame = find_frame(itos(comment->get_id()))) {
            const Vector<OrchestratorEditorGraphNode*> selected = get_selected<OrchestratorEditorGraphNode>();
            if (!selected.is_empty()) {
                comment->set_autoshrink_enabled(true);

                for (OrchestratorEditorGraphNode* node : selected) {
                    if (!get_element_frame(node->get_name())) {
                        attach_graph_element_to_frame(node->get_name(), frame->get_name());
                        node->set_selected(false);
                    }
                }

                frame->update_placeholder(true);
            }

            frame->set_selected(true);
        }
    }
}

OrchestratorEditorGraphPin* OrchestratorEditorGraphPanel::_resolve_pin_from_handle(const PinHandle& p_handle, bool p_input) {
    if (OrchestratorEditorGraphNode* node = find_node(p_handle.node_id)) {
        const int32_t pin_slot = node->get_port_slot(p_handle.pin_port, p_input ? PD_Input : PD_Output);
        return node->get_pin(pin_slot, p_input ? PD_Input : PD_Output);
    }
    return nullptr;
}

void OrchestratorEditorGraphPanel::_connect_with_menu(const PinHandle& p_handle, const Vector2& p_position, bool p_input) {
    OrchestratorEditorGraphPin* pin = _resolve_pin_from_handle(p_handle, p_input);
    ERR_FAIL_NULL_MSG(pin, "Failed to resolve pin from context");

    _menu_position = (p_position + get_scroll_offset()) / get_zoom();

    _drag_from_pin = pin;

    // Resolve the drag pin target if one is available
    Object* target = nullptr;
    const Ref<OScriptTargetObject> target_reference = _drag_from_pin->_pin->resolve_target();
    if (target_reference.is_valid() && target_reference->has_target()) {
        target = target_reference->get_target();
    }

    Ref<OrchestratorEditorActionPortRule> port_rule;
    if (!PropertyUtils::is_variant(_drag_from_pin->get_property_info())) {
        port_rule.instantiate();
        port_rule->configure(_drag_from_pin, target);
    }

    Ref<OrchestratorEditorActionGraphTypeRule> graph_type_rule;
    graph_type_rule.instantiate();
    graph_type_rule->set_graph_type(_graph->get_flags().has_flag(OrchestrationGraph::GF_FUNCTION)
        ? OrchestratorEditorActionDefinition::GRAPH_FUNCTION
        : OrchestratorEditorActionDefinition::GRAPH_EVENT);

    OrchestratorEditorActionMenu* menu = memnew(OrchestratorEditorActionMenu);
    menu->set_title("Select a graph action");
    menu->set_suffix("graph_editor");
    menu->set_close_on_focus_lost(ORCHESTRATOR_GET("interface/editor/actions_menu/close_on_focus_lost", false));
    menu->set_show_filter_option(false);
    menu->set_start_collapsed(true);
    menu->connect("action_selected", callable_mp_this(_action_menu_selection));
    menu->connect(SceneStringName(canceled), callable_mp_this(_action_menu_canceled));

    Ref<OrchestratorEditorActionFilterEngine> filter_engine;
    filter_engine.instantiate();
    filter_engine->add_rule(memnew(OrchestratorEditorActionSearchTextRule));
    filter_engine->add_rule(graph_type_rule);
    if (port_rule.is_valid()) {
        filter_engine->add_rule(port_rule);
    }

    const Ref<Script> source_script = _graph->get_orchestration()->as_script();

    if (_drag_from_pin->is_execution()) {
        Ref<OrchestratorEditorActionClassHierarchyScopeRule> class_hierarchy_rule;
        class_hierarchy_rule.instantiate();
        class_hierarchy_rule->set_script_classes(source_script);
        filter_engine->add_rule(class_hierarchy_rule);
    }

    OrchestratorEditorActionRegistry* action_registry = OrchestratorEditorActionRegistry::get_singleton();

    OrchestratorEditorActionSet actions;
    if (target) {
        actions = action_registry->get_actions(target);
    } else if (target_reference.is_valid()) {
        if (target_reference->has_target() && !target_reference->get_target_class().is_empty()) {
            actions = action_registry->get_actions(target_reference->get_target_class());
        }
    }

    if (actions.is_empty()) {
        actions = action_registry->get_actions(source_script);
    }

    menu->popup(p_position + get_screen_position(), actions, filter_engine);
}

void OrchestratorEditorGraphPanel::_popup_menu(const Vector2& p_position) {
    _menu_position = (p_position + get_scroll_offset()) / get_zoom();

    Ref<OrchestratorEditorActionGraphTypeRule> graph_type_rule;
    graph_type_rule.instantiate();
    graph_type_rule->set_graph_type(_graph->get_flags().has_flag(OrchestrationGraph::GF_FUNCTION)
        ? OrchestratorEditorActionDefinition::GRAPH_FUNCTION
        : OrchestratorEditorActionDefinition::GRAPH_EVENT);

    Ref<OrchestratorEditorActionClassHierarchyScopeRule> class_hierarchy_rule;
    class_hierarchy_rule.instantiate();
    class_hierarchy_rule->set_script_classes(_graph->get_orchestration()->as_script());

    const PackedStringArray method_names = _graph->get_orchestration()->get_function_names();
    Ref<OrchestratorEditorActionOverrideFunctionRule> override_rule;
    override_rule.instantiate();
    override_rule->set_overridden_methods(method_names);

    Ref<OrchestratorEditorActionFilterEngine> filter_engine;
    filter_engine.instantiate();
    filter_engine->add_rule(memnew(OrchestratorEditorActionSearchTextRule));
    filter_engine->add_rule(class_hierarchy_rule);
    filter_engine->add_rule(graph_type_rule);
    filter_engine->add_rule(override_rule);

    OrchestratorEditorActionMenu* menu = memnew(OrchestratorEditorActionMenu);
    menu->set_title("Select a graph action");
    menu->set_suffix("graph_editor");
    menu->set_close_on_focus_lost(ORCHESTRATOR_GET("interface/editor/actions_menu/close_on_focus_lost", false));
    menu->set_show_filter_option(false);
    menu->set_start_collapsed(true);
    menu->connect("action_selected", callable_mp_this(_action_menu_selection));
    menu->connect(SceneStringName(canceled), callable_mp_this(_action_menu_canceled));

    menu->popup(
        p_position + get_screen_position(),
        OrchestratorEditorActionRegistry::get_singleton()->get_actions(_graph->get_orchestration()->as_script()),
        filter_engine);
}

void OrchestratorEditorGraphPanel::_action_menu_selection(const Ref<OrchestratorEditorActionDefinition>& p_action) {
    ERR_FAIL_COND_MSG(!p_action.is_valid(), "Cannot execute the action, it is invalid.");

    const Vector2 spawn_position = _menu_position;

    switch (p_action->type) {
        case OrchestratorEditorActionDefinition::ACTION_SPAWN_NODE: {
            ERR_FAIL_COND_MSG(!p_action->node_class.has_value(), "Spawn action node has no node class type");

            NodeSpawnOptions options;
            options.node_class = p_action->node_class.value();
            options.context.user_data = p_action->data;
            options.context.method = p_action->method;
            options.position = spawn_position;
            options.drag_pin = _drag_from_pin;

            spawn_node(options);
            break;
        }
        case OrchestratorEditorActionDefinition::ACTION_GET_PROPERTY: {
            ERR_FAIL_COND_MSG(!p_action->property.has_value(), "Get property has no property");

            NodeSpawnOptions options;
            options.node_class = OScriptNodePropertyGet::get_class_static();
            options.context.property = p_action->property;
            options.context.node_path = p_action->node_path;
            options.context.class_name = p_action->class_name;
            options.position = spawn_position;
            options.drag_pin = _drag_from_pin;

            spawn_node(options);
            break;
        }
        case OrchestratorEditorActionDefinition::ACTION_SET_PROPERTY: {
            ERR_FAIL_COND_MSG(!p_action->property.has_value(), "Set property has no property");

            NodeSpawnOptions options;
            options.node_class = OScriptNodePropertySet::get_class_static();
            options.context.property = p_action->property;
            options.context.node_path = p_action->node_path;
            options.context.class_name = p_action->class_name;
            options.position = spawn_position;
            options.drag_pin = _drag_from_pin;

            spawn_node(options);
            break;
        }
        case OrchestratorEditorActionDefinition::ACTION_CALL_MEMBER_FUNCTION: {
            ERR_FAIL_COND_MSG(!p_action->method.has_value(), "Call member function has no method");
            if (_treat_call_member_as_override) {
                _create_new_function_override(p_action->method.value());
                emit_signal("nodes_changed");
            } else {
                NodeSpawnOptions options;
                options.node_class = OScriptNodeCallMemberFunction::get_class_static();
                options.context.user_data = p_action->data;
                options.context.method = p_action->method;
                options.context.class_name = p_action->class_name;
                options.position = spawn_position;
                options.drag_pin = _drag_from_pin;

                spawn_node(options);
            }
            break;
        }
        case OrchestratorEditorActionDefinition::ACTION_CALL_SCRIPT_FUNCTION: {
            ERR_FAIL_COND_MSG(!p_action->method.has_value(), "Call script function has no method");

            NodeSpawnOptions options;
            options.node_class = OScriptNodeCallScriptFunction::get_class_static();
            options.context.method = p_action->method;
            options.position = spawn_position;
            options.drag_pin = _drag_from_pin;

            spawn_node(options);
            break;
        }
        case OrchestratorEditorActionDefinition::ACTION_EVENT: {
            ERR_FAIL_COND_MSG(!p_action->method.has_value(), "Handle event has no method");

            NodeSpawnOptions options;
            options.node_class = OScriptNodeEvent::get_class_static();
            options.context.method = p_action->method;
            options.position = spawn_position;
            options.drag_pin = _drag_from_pin;

            // For override functions, this is helpful to center where the event node spawns
            options.center_on_spawn = true;

            spawn_node(options);
            break;
        }
        case OrchestratorEditorActionDefinition::ACTION_EMIT_MEMBER_SIGNAL: {
            ERR_FAIL_COND_MSG(!p_action->method.has_value(), "Emit member signal function has no method");

            NodeSpawnOptions options;
            options.node_class = OScriptNodeEmitMemberSignal::get_class_static();
            options.context.method = p_action->method;
            options.context.user_data = p_action->data;
            options.position = spawn_position;
            options.drag_pin = _drag_from_pin;

            spawn_node(options);
            break;
        }
        case OrchestratorEditorActionDefinition::ACTION_EMIT_SIGNAL: {
            ERR_FAIL_COND_MSG(!p_action->method.has_value(), "Emit signal function has no method");

            NodeSpawnOptions options;
            options.node_class = OScriptNodeEmitSignal::get_class_static();
            options.context.method = p_action->method;
            options.position = spawn_position;
            options.drag_pin = _drag_from_pin;

            spawn_node(options);
            break;
        }
        case OrchestratorEditorActionDefinition::ACTION_VARIABLE_GET: {
            ERR_FAIL_COND_MSG(!p_action->property.has_value(), "Get variable has no property");

            NodeSpawnOptions options;
            options.node_class = OScriptNodeVariableGet::get_class_static();
            options.context.variable_name = p_action->property.value().name;
            options.context.user_data = DictionaryUtils::of({ { "validation", false } });
            options.position = spawn_position;
            options.drag_pin = _drag_from_pin;

            spawn_node(options);
            break;
        }
        case OrchestratorEditorActionDefinition::ACTION_VARIABLE_SET: {
            ERR_FAIL_COND_MSG(!p_action->property.has_value(), "Set variable has no property");

            NodeSpawnOptions options;
            options.node_class = OScriptNodeVariableSet::get_class_static();
            options.context.variable_name = p_action->property.value().name;
            options.position = spawn_position;
            options.drag_pin = _drag_from_pin;

            spawn_node(options);
            break;
        }
        default: {
            const String message = vformat("Unknown action type %d - %s", p_action->type, p_action->name);
            OrchestratorEditorDialogs::error(message, "Failed to spawn node", false);
            break;
        }
    }

    _treat_call_member_as_override = false;
}

void OrchestratorEditorGraphPanel::_action_menu_canceled() {
    _drag_from_pin.reset();
    _treat_call_member_as_override = false;
}

void OrchestratorEditorGraphPanel::_grid_pattern_changed(int p_index) {
    set_grid_pattern(CAST_INT_TO_ENUM(GridPattern, _grid_pattern->get_item_metadata(p_index)));
}

void OrchestratorEditorGraphPanel::_settings_changed() {
    if (_theme_update_timer->is_inside_tree()) {
        if (!_theme_update_timer->is_stopped()) {
            return;
        }
        _theme_update_timer->start();
    }

    _update_panel_hint();

    set_minimap_enabled(ORCHESTRATOR_GET("interface/editor/graph/show_minimap", false));
    set_show_arrange_button(ORCHESTRATOR_GET("interface/editor/graph/show_arrange_button", false));

    _show_overlay_action_tooltips = ORCHESTRATOR_GET("interface/editor/graph/show_overlay_action_tooltips", true);
    _disconnect_control_flow_when_dragged = ORCHESTRATOR_GET("interface/editor/graph/disconnect_control_flow_when_dragged", true);
    _show_advanced_tooltips= ORCHESTRATOR_GET("interface/editor/graph/show_advanced_tooltips", false);

    bool node_update_required = false;
    node_update_required |= ORCHESTRATOR_GET_TRACK(_show_type_icons, "interface/editor/graph_nodes/show_type_icons", true);
    node_update_required |= ORCHESTRATOR_GET_TRACK(_resizable_by_default, "interface/editor/graph_nodes/resizable_by_default", true);

    if (_graph.is_valid()) {
        // While we iterate each node, each call checks the current state against the settings values
        // and only queues redraws if and only if there are variances in the values to minimize the
        // impact of these types of changes.
        for_each<GraphElement>([&] (GraphElement* element) {
            if (OrchestratorEditorGraphNode* node = cast_to<OrchestratorEditorGraphNode>(element)) {
                if (node->is_resizable() != _resizable_by_default) {
                    node->set_resizable(_resizable_by_default);
                }

                node->set_show_type_icons(_show_type_icons);
                node->set_show_advanced_tooltips(_show_advanced_tooltips);

                // Needed for connection color changes.
                node->redraw_connections();
            }
            element->queue_redraw();
        });

    }
}

void OrchestratorEditorGraphPanel::_show_drag_hint(const String& p_hint_text) const {
    if (!_show_overlay_action_tooltips || !_drag_hint || !_drag_hint_timer) {
        return;
    }

    _drag_hint->set_text(vformat("Hint:\n%s", p_hint_text));
    _drag_hint->show();
    _drag_hint_timer->start();
}

bool OrchestratorEditorGraphPanel::_is_delete_confirmation_enabled() {
    return ORCHESTRATOR_GET("interface/editor/graph/confirm_on_delete", true);
}

bool OrchestratorEditorGraphPanel::_can_duplicate_nodes(const Vector<OrchestratorEditorGraphNode*>& p_nodes, bool p_error_dialog) {
    for (OrchestratorEditorGraphNode* node : p_nodes) {
        if (!node->_node->can_duplicate()) {
            if (p_error_dialog) {
                const String message = vformat("Cannot duplicate node '%s' with ID %d", node->get_title(), node->get_id());
                OrchestratorEditorDialogs::error(message);
            }
            return false;
        }
    }
    return true;
}

void OrchestratorEditorGraphPanel::_set_scroll_offset_and_zoom(const Vector2& p_scroll_offset, float p_zoom, const Callable& p_callback) {
    if (is_inside_tree() && get_tree()) {
        const Ref<Tween> tween = get_tree()->create_tween();
        if (!tween.is_valid()) {
            return;
        }

        tween->tween_method(Callable(this, "set_zoom"), get_zoom(), p_zoom, 0.0);
        tween->chain()->tween_method(Callable(this, "set_scroll_offset"), get_scroll_offset(), p_scroll_offset, 0.0);
        tween->set_ease(Tween::EASE_IN_OUT);

        if (p_callback.is_valid()) {
            tween->connect(SceneStringName(finished), p_callback);
        }

        tween->play();
    }
}

void OrchestratorEditorGraphPanel::_schedule_restore() {
    if (_restore_scheduled || _initialized || _edit_state.is_empty()) {
        return;
    }
    if (!is_inside_tree() || !get_tree()) {
        return;
    }

    // Apply on the next frame, after the owning TabContainer has settled on its current
    // tab and that tab has been laid out. A tab that is only momentarily current during
    // restoration is no longer visible when this fires and is skipped (see below).
    _restore_scheduled = true;
    get_tree()->connect("process_frame", callable_mp_this(_restore_edit_state), CONNECT_ONE_SHOT);
}

void OrchestratorEditorGraphPanel::_restore_edit_state() {
    _restore_scheduled = false;

    if (_initialized || _edit_state.is_empty() || !is_visible_in_tree()) {
        return;
    }

    const Dictionary state = _edit_state;

    const float zoom = state.get("zoom", 1.0);
    const Vector2 offset = state.get("viewport_offset", Vector2());

    _set_scroll_offset_and_zoom(offset, zoom);

    set_minimap_enabled(state.get("minimap", false));
    set_snapping_enabled(state.get("snapping", true));

    _markers->load_state(state);

    set_show_grid(state.get("grid", true));

    const int grid_pattern = state.get("grid_pattern", 0);
    set_grid_pattern(CAST_INT_TO_ENUM(GridPattern, grid_pattern));
    _grid_pattern->select(grid_pattern);

    _initialized = true;
}

void OrchestratorEditorGraphPanel::_schedule_refresh() {
    if (_refresh_scheduled || !_panel_refresh_pending || !is_inside_tree() || !get_tree()) {
        return;
    }

    // Building the graph nodes and connections remain the dominant cost when a layout is restored,
    // particularly when there are multiple graphs open.
    //
    // This defers to the next frame, after the owning TabContainer has settled which tab is current,
    // then build only the graph that is actually visible (see _refresh_panel_with_model). Any graph
    // that is hidden, other files or other tabs, remain dirty and build when they're first shown.
    //
    // This allows layout load to only pay for the single visible graph instead of all open graphs
    // across all open script files.
    _refresh_scheduled = true;
    get_tree()->connect("process_frame", callable_mp_this(_refresh_panel_with_model), CONNECT_ONE_SHOT);
}

void OrchestratorEditorGraphPanel::_queue_autowire(OrchestratorEditorGraphNode* p_spawned_node, OrchestratorEditorGraphPin* p_origin_pin) {
    ERR_FAIL_NULL_MSG(p_spawned_node, "Cannot initiate an autowire operation with an invalid node reference");
    ERR_FAIL_NULL_MSG(p_origin_pin, "Cannot initiate an autowire operation with an invalid pin reference");

    const Vector<OrchestratorEditorGraphPin*> choices = p_spawned_node->get_eligible_autowire_pins(p_origin_pin);

    // Do nothing if there are no eligible choices
    if (choices.size() == 0) {
        return;
    }

    OrchestratorEditorGraphPin* pin = _get_autowire_pin(p_spawned_node, p_origin_pin, choices);
    if (pin) {
        // Pin was selected by logic, handle.
        link(p_origin_pin, pin);
        pin->get_graph_node()->_resize_to_content();
        return;
    }

    // At this point no auto-resolution could be made, show the dialog if enabled
    const bool autowire_dialog_enabled = ORCHESTRATOR_GET("interface/editor/graph/show_autowire_selection_dialog", true);
    if (!autowire_dialog_enabled) {
        return;
    }

    OrchestratorAutowireConnectionDialog* autowire = memnew(OrchestratorAutowireConnectionDialog);

    autowire->connect(SceneStringName(confirmed), callable_mp_lambda(this, [autowire, p_origin_pin, this] {
        OrchestratorEditorGraphPin* selected = autowire->get_autowire_choice();
        if (selected) {
            link(p_origin_pin, selected);
            selected->get_graph_node()->_resize_to_content();
        }
    }));

    autowire->popup_autowire(choices);
}

OrchestratorEditorGraphPin* OrchestratorEditorGraphPanel::_get_autowire_pin(OrchestratorEditorGraphNode* p_spawned_node,
    OrchestratorEditorGraphPin* p_origin_pin, const Vector<OrchestratorEditorGraphPin*>& p_choices) {

    if (p_choices.size() == 1) {
        return p_choices[0];
    }

    // Compute exact matches for class types
    Vector<OrchestratorEditorGraphPin*> exact_matches;
    for (OrchestratorEditorGraphPin* choice : p_choices) {
        if (choice->get_property_info().class_name.match(p_origin_pin->get_property_info().class_name)) {
            exact_matches.push_back(choice);
        }
    }

    // Handle cases where class matches rank higher and have precedence
    if (exact_matches.size() == 1) {
        return exact_matches[0];
    }

    // For operator nodes, always auto-wire the first eligible pin.
    if (cast_to<OScriptNodeOperator>(p_spawned_node->_node.ptr())) {
        return p_choices[0];
    }

    return nullptr;
}

Vector2 OrchestratorEditorGraphPanel::_get_center() const {
    return get_scroll_offset() + (get_size() / 2.0);
}

void OrchestratorEditorGraphPanel::_update_theme_item_cache() {
    if (_in_theme_update) {
        return;
    }

    // As this method sets the theme below, this guard will trigger setting the argument
    // as true and will only clear it back to false when the method exits. So when the
    // set_theme causes a new NOTIFICATION_THEME_CHANGED notification, this method acts
    // as a no-op and exits early.
    ScopedThemeGuard guard(_in_theme_update);

    Control* parent_control = get_menu_control()->get_parent_control();
    Ref<StyleBoxFlat> panel = parent_control->get_theme_stylebox(SceneStringName(panel))->duplicate();
    panel->set_shadow_size(1);
    panel->set_shadow_offset(Vector2(2.f, 2.f));
    panel->set_bg_color(panel->get_bg_color() + Color(0, 0, 0, .3));
    panel->set_border_width(SIDE_LEFT, 1);
    panel->set_border_width(SIDE_TOP, 1);
    panel->set_border_color(panel->get_shadow_color());
    _theme_cache.panel = panel;

    _theme_cache.label_font = SceneUtils::get_editor_font("main_msdf");
    _theme_cache.label_bold_font = SceneUtils::get_editor_font("main_bold_msdf");

    Ref<Theme> theme;
    theme.instantiate();
    theme->set_font(SceneStringName(font), "Label", _theme_cache.label_font);
    theme->set_font(SceneStringName(font), "GraphNodeTitleLabel", _theme_cache.label_bold_font);
    theme->set_font(SceneStringName(font), "LineEdit", _theme_cache.label_font);
    theme->set_font(SceneStringName(font), "Button", _theme_cache.label_font);
    set_theme(theme);
}

void OrchestratorEditorGraphPanel::_update_menu_theme() {
    Control* control = get_menu_control()->get_parent_control();
    control->add_theme_stylebox_override(SceneStringName(panel), _theme_cache.panel);
}

void OrchestratorEditorGraphPanel::_update_center_status() {
    if (_graph->get_nodes().is_empty()) {
        if (!_center_status->is_visible()) {
            _center_status->show();
        }
    } else {
        if (_center_status->is_visible()) {
            _center_status->hide();
        }
    }
}

void OrchestratorEditorGraphPanel::_add_node_to_panel(const Ref<OrchestrationGraphNode>& p_node) {
    if (OrchestratorEditorGraphFrame* frame = OrchestratorEditorGraphNodeFactory::try_create_frame(p_node)) {
        frame->set_name(itos(p_node->get_id()));
        add_child(frame);
        frame->set_node(p_node);
        return;
    }

    OrchestratorEditorGraphNode* graph_node = OrchestratorEditorGraphNodeFactory::create_node(p_node);
    ERR_FAIL_COND_MSG(!graph_node, "Failed to create graph node for node id " + itos(p_node->get_id()));

    // Must come first so when pin widget sizes are computed in set_node, they have non-zero values
    graph_node->set_name(itos(p_node->get_id()));
    add_child(graph_node);

    graph_node->set_node(p_node);
    graph_node->set_resizable(_resizable_by_default);
    graph_node->set_show_type_icons(_show_type_icons);
    graph_node->set_show_advanced_tooltips(_show_advanced_tooltips);
    graph_node->set_position_offset(p_node->get_position());
    graph_node->set_size(p_node->get_size());
}

void OrchestratorEditorGraphPanel::_remove_node_from_panel(const Ref<OrchestrationGraphNode>& p_node) {

}

void OrchestratorEditorGraphPanel::_refresh_panel_with_model() {
    _refresh_scheduled = false;

    // Only the visible graph builds. A graph scheduled while hidden (another file's tab, a non-active tab)
    // remains dirty and rebuilds from NOTIFICATION_VISIBILITY_CHANGED notifications when shown.
    if (!is_visible_in_tree()) {
        return;
    }

    clear_connections();

    for (int i = get_child_count() - 1; i >= 0; i--) {
        GraphElement* element = cast_to<GraphElement>(get_child(i));
        if (element) {
            remove_child(element);
            element->queue_free();
        }
    }

    for (const Ref<OrchestrationGraphNode>& node : _graph->get_nodes()) {
        _add_node_to_panel(node);
    }

    for (const Connection& E : _graph->get_connections()) {
        Error err = connect_node(itos(E.from_node), E.from_port, itos(E.to_node), E.to_port);
        ERR_CONTINUE_MSG(err != OK, "Failed to create graph connection for connection id " + itos(E.id));
    }

    _restore_frame_attachments();

    // Queue up a revalidation sequence
    emit_signal("validate_script");

    _update_center_status();

    _panel_refresh_pending = false;
}

void OrchestratorEditorGraphPanel::_refresh_panel_connections_with_model() {
    clear_connections();

    for (const Connection& E : _graph->get_connections()) {
        Error err = connect_node(itos(E.from_node), E.from_port, itos(E.to_node), E.to_port);
        ERR_CONTINUE_MSG(err != OK, "Failed to create graph connection for connection id " + itos(E.id));
    }

    emit_signal("connections_changed");
    emit_signal("validate_script");

    _panel_connections_refresh_pending = false;
}

void OrchestratorEditorGraphPanel::_queue_panel_refresh() {
    _panel_refresh_pending = true;
    _schedule_refresh();
}

void OrchestratorEditorGraphPanel::_queue_panel_connections_refresh() {
    // The panel refreshes connections, so no need to refresh connections separately.
    if (!_panel_refresh_pending) {
        if (!_panel_connections_refresh_pending) {
            _panel_connections_refresh_pending = true;
            callable_mp_this(_refresh_panel_connections_with_model).call_deferred();
        }
    }
}

void OrchestratorEditorGraphPanel::_drop_data_files(const String& p_node_type, const Array& p_files, const Vector2& p_at_position) {
    Vector2 position = p_at_position;

    for (int i = 0; i < p_files.size(); i++) {
        NodeSpawnOptions options;
        options.node_class = p_node_type;
        options.context.resource_path = p_files[i];
        options.position = position;

        GraphElement* element = spawn_node(options).element;
        if (element) {
            position.y += element->get_size().height + 10;
        }
    }
}

void OrchestratorEditorGraphPanel::_drop_data_property(const Dictionary& p_property, const Vector2& p_at_position, const String& p_path, bool p_setter) {
    const String node_class_type = p_setter
        ? OScriptNodePropertySet::get_class_static()
        : OScriptNodePropertyGet::get_class_static();

    NodeSpawnOptions options;
    options.node_class = node_class_type;
    options.context.property = DictionaryUtils::to_property(p_property);
    options.position = p_at_position;

    if (!p_path.is_empty()) {
        options.context.node_path = p_path;
    }

    spawn_node(options);
}

void OrchestratorEditorGraphPanel::_drop_data_variable(const String& p_name, const Vector2& p_at_position, bool p_validated, bool p_setter) {
    const String node_class_type = p_setter
        ? OScriptNodeVariableSet::get_class_static()
        : OScriptNodeVariableGet::get_class_static();

    NodeSpawnOptions options;
    options.node_class = node_class_type;
    options.context.variable_name = p_name;
    options.position = p_at_position;

    if (!p_setter) {
        options.context.user_data = DictionaryUtils::of({{ "validation", p_validated }});
    }

    spawn_node(options);
}

bool OrchestratorEditorGraphPanel::_is_in_port_hotzone(const Vector2& p_pos, const Vector2& p_mouse_pos, const Vector2i& p_port_size, bool p_left) {
    const int32_t port_hotzone_outer_extent = get_theme_constant("port_hotzone_outer_extent");
    const int32_t port_hotzone_inner_extent = get_theme_constant("port_hotzone_inner_extent");

    const String hotzone_percent = ORCHESTRATOR_GET("interface/editor/graph_nodes/connection_hotzone_scale", "100%");
    const Vector2i port_size = p_port_size * (hotzone_percent.replace("%", "").to_float() / 100.0);

    const Rect2 hotzone = Rect2(
        p_pos.x - (p_left ? port_hotzone_outer_extent : port_hotzone_inner_extent),
        p_pos.y - port_size.height / 2.0,
        port_hotzone_inner_extent + port_hotzone_outer_extent,
        port_size.height);

    return hotzone.has_point(p_mouse_pos);
}

void OrchestratorEditorGraphPanel::_set_edited(bool p_edited) {
    _graph->get_orchestration()->as_script()->set_edited(p_edited);

    // Request revalidation post change
    emit_signal("validate_script");
}

void OrchestratorEditorGraphPanel::_get_graph_node_and_port(const Vector2& p_position, int& r_id, int& r_port_index) const {
    r_id = -1;
    r_port_index = -1;

    for (int i = 0; i < get_child_count() && r_port_index == -1; i++) {
        if (OrchestratorEditorGraphNode* node = cast_to<OrchestratorEditorGraphNode>(get_child(i))) {
            const int port = node->get_port_at_position(p_position / get_zoom());
            if (port != -1) {
                r_id = node->get_id();
                r_port_index = port;
            }
        }
    }
}

bool OrchestratorEditorGraphPanel::_is_point_inside_node(const Vector2& p_point) const {
    for (int i = 0; i < get_child_count(); i++) {
        GraphElement* element = cast_to<GraphElement>(get_child(i));
        if (element != nullptr && cast_to<GraphFrame>(element) == nullptr && element->get_rect().has_point(p_point)) {
            return true;
        }
    }
    return false;
}

void OrchestratorEditorGraphPanel::_disconnect_connection(const Dictionary& p_connection) {
    const OScriptConnection connection = OScriptConnection::from_dict(p_connection);

    _disconnection_request(
        vformat("%d", connection.from_node),
        connection.from_port,
        vformat("%d", connection.to_node),
        connection.to_port);
}

void OrchestratorEditorGraphPanel::_create_connection_reroute(const Dictionary& p_connection, const Vector2& p_position) {
    if (p_connection.is_empty()) {
        return;
    }

    const Connection connection = Connection::from_dict(p_connection);
    const Vector2 graph_position = (p_position + get_scroll_offset()) / get_zoom();

    OrchestratorEditorGraphNode* source_graph_node = find_node(connection.from_node);
    OrchestratorEditorGraphNode* target_graph_node = find_node(connection.to_node);
    ERR_FAIL_NULL_MSG(source_graph_node, "Cannot insert reroute: source graph node not found.");
    ERR_FAIL_NULL_MSG(target_graph_node, "Cannot insert reroute: target graph node not found.");

    OrchestratorEditorGraphPin* source_pin = source_graph_node->get_output_pin(connection.from_port);
    OrchestratorEditorGraphPin* target_pin = target_graph_node->get_input_pin(connection.to_port);
    ERR_FAIL_NULL_MSG(source_pin, "Cannot insert reroute: source pin not found.");
    ERR_FAIL_NULL_MSG(target_pin, "Cannot insert reroute: target pin not found.");

    // Disconnect original connection before inserting reroute
    unlink(source_pin, target_pin);

    // Spawn the reroute node at the click position
    NodeSpawnOptions options;
    options.node_class = OScriptNodeReroute::get_class_static();
    options.position = graph_position;
    OrchestratorEditorGraphNode* reroute_graph_node = spawn_node(options).node;
    ERR_FAIL_NULL_MSG(reroute_graph_node, "Failed to spawn reroute node.");

    // Connect: original source TO reroute input TO reroute output TO original target
    OrchestratorEditorGraphPin* reroute_in  = reroute_graph_node->get_input_pin(0);
    OrchestratorEditorGraphPin* reroute_out = reroute_graph_node->get_output_pin(0);
    ERR_FAIL_NULL_MSG(reroute_in,  "Reroute node missing input pin.");
    ERR_FAIL_NULL_MSG(reroute_out, "Reroute node missing output pin.");

    link(source_pin, reroute_in);
    link(reroute_out, target_pin);
}

void OrchestratorEditorGraphPanel::_dissolve_selected_reroutes() {
    const Vector<OrchestratorEditorGraphNodeReroute*> reroutes = get_selected<OrchestratorEditorGraphNodeReroute>();
    if (reroutes.is_empty()) {
        return;
    }

    const RBSet<OScriptConnection>& all_connections = _graph->get_orchestration()->get_connections();

    for (OrchestratorEditorGraphNodeReroute* reroute : reroutes) {
        const int reroute_id = reroute->get_id();

        OScriptConnection incoming, outgoing;
        int incoming_count = 0, outgoing_count = 0;

        for (const OScriptConnection& C : all_connections) {
            if (C.to_node == static_cast<uint64_t>(reroute_id)) {
                incoming = C;
                incoming_count++;
            } else if (C.from_node == static_cast<uint64_t>(reroute_id)) {
                outgoing = C;
                outgoing_count++;
            }
        }

        if (incoming_count == 1 && outgoing_count == 1) {
            OrchestratorEditorGraphNode* source_node = find_node(static_cast<int>(incoming.from_node));
            OrchestratorEditorGraphNode* target_node = find_node(static_cast<int>(outgoing.to_node));
            if (source_node && target_node) {
                OrchestratorEditorGraphPin* source_pin = source_node->get_output_pin(static_cast<int>(incoming.from_port));
                OrchestratorEditorGraphPin* target_pin = target_node->get_input_pin(static_cast<int>(outgoing.to_port));
                if (source_pin && target_pin) {
                    link(source_pin, target_pin);
                }
            }
        }

        remove_node(reroute, false);
    }

    _drag_hint->hide();
}

void OrchestratorEditorGraphPanel::_drop_data_function(const Dictionary& p_function, const Vector2& p_at_position, bool p_as_callable) {
    const MethodInfo method = DictionaryUtils::to_method(p_function);

    if (!p_as_callable) {
        NodeSpawnOptions options;
        options.node_class = OScriptNodeCallScriptFunction::get_class_static();
        options.context.method = method;
        options.position = p_at_position;

        spawn_node(options);
    } else {
        int ctor_index = 0;
        bool found = false;
        const BuiltInType callable_type = ExtensionDB::get_builtin_type(Variant::CALLABLE);
        for (; ctor_index < callable_type.constructors.size(); ctor_index++) {
            const ConstructorInfo& ci = callable_type.constructors[ctor_index];
            if (ci.arguments.size() == 2 && ci.arguments[0].type == Variant::OBJECT && ci.arguments[1].type == Variant::STRING_NAME) {
                found = true;
                break;
            }
        }

        if (found) {
            const Array arguments = DictionaryUtils::from_properties(callable_type.constructors[ctor_index].arguments);

            NodeSpawnOptions options;
            options.node_class = OScriptNodeComposeFrom::get_class_static();
            options.context.user_data = DictionaryUtils::of({{ "type", Variant::CALLABLE }, { "constructor_args", arguments }});
            options.position = p_at_position;

            OrchestratorEditorGraphNode* compose_node = spawn_node(options).node;
            if (compose_node) {
                compose_node->get_input_pin(1)->_pin->set_default_value(method.name);

                options.node_class = OScriptNodeSelf::get_class_static();
                options.context.user_data.reset();
                options.position = options.position - Vector2(200, 0);

                OrchestratorEditorGraphNode* self = spawn_node(options).node;
                if (self) {
                    link(self->get_output_pin(0), compose_node->get_input_pin(0));
                }
            }
        }
    }
}

void OrchestratorEditorGraphPanel::_shortcut_input(const Ref<InputEvent>& p_event) {
    ERR_FAIL_COND(p_event.is_null());

    Ref<InputEventKey> key = p_event;
    if (is_visible_in_tree() && key.is_valid() && key->is_pressed() && !key->is_echo()) {

        // First handle keybinding actions, e.g. spawn nodes, etc.
        bool handled = true;
        if (ED_IS_SHORTCUT("graph_editor/create_nodes/branch", key)) {
            spawn_node<OScriptNodeBranch>();
        } else if (ED_IS_SHORTCUT("graph_editor/create_nodes/comment", key)) {
            _spawn_frame();
        } else if (ED_IS_SHORTCUT("graph_editor/create_nodes/delay", key)) {
            spawn_node<OScriptNodeDelay>();
        } else if (ED_IS_SHORTCUT("graph_editor/create_nodes/sequence", key)) {
            spawn_node<OScriptNodeSequence>();
        } else if (ED_IS_SHORTCUT("graph_editor/zoom_in", key)) {
            set_zoom(get_zoom() * get_zoom_step());
        } else if (ED_IS_SHORTCUT("graph_editor/zoom_out", key)) {
            set_zoom(get_zoom() / get_zoom_step());
        } else {
            handled = false;
        }

        // Check nodes that require a hovered/anchor node
        if (!handled) {
            handled = true;
            if (OrchestratorEditorGraphNode* hovered_node = get_hovered_element<OrchestratorEditorGraphNode>()) {
                if (ED_IS_SHORTCUT("graph_editor/break_node_links", p_event)) {
                    unlink_node_all(hovered_node);
                } else if (ED_IS_SHORTCUT("graph_editor/add_call_parent_function", p_event)) {
                    _create_call_to_parent_function(hovered_node);
                } else if (ED_IS_SHORTCUT("graph_editor/add_option_pin", p_event)) {
                    _add_node_pin(hovered_node);
                } else if (ED_IS_SHORTCUT("graph_editor/await_function", p_event)) {
                    _toggle_await_function(hovered_node);
                } else if (ED_IS_SHORTCUT("graph_editor/expand_node", p_event)) {
                    _expand_node(hovered_node);
                } else if (ED_IS_SHORTCUT("graph_editor/alignment/align_top", p_event)) {
                    _align_nodes(hovered_node, ALIGN_TOP);
                } else if (ED_IS_SHORTCUT("graph_editor/alignment/align_middle", p_event)) {
                    _align_nodes(hovered_node, ALIGN_MIDDLE);
                } else if (ED_IS_SHORTCUT("graph_editor/alignment/align_bottom", p_event)) {
                    _align_nodes(hovered_node, ALIGN_BOTTOM);
                } else if (ED_IS_SHORTCUT("graph_editor/alignment/align_left", p_event)) {
                    _align_nodes(hovered_node, ALIGN_LEFT);
                } else if (ED_IS_SHORTCUT("graph_editor/alignment/align_center", p_event)) {
                    _align_nodes(hovered_node, ALIGN_CENTER);
                } else if (ED_IS_SHORTCUT("graph_editor/alignment/align_right", p_event)) {
                    _align_nodes(hovered_node, ALIGN_RIGHT);
                } else if (ED_IS_SHORTCUT("graph_editor/view_documentation", p_event)) {
                    _view_documentation(hovered_node->get_graph_node()->get_help_topic());
                } else {
                    handled = false;
                }
            } else {
                handled = false;
            }
        }

        if (!handled) {
            handled = true;
            // First check nodes that require a hovered/anchor node
            if (OrchestratorEditorGraphFrame* frame = get_hovered_element<OrchestratorEditorGraphFrame>()) {
                if (ED_IS_SHORTCUT("graph_editor/frame/set_frame_title", p_event)) {
                    frame->_change_frame_title();
                } else if (ED_IS_SHORTCUT("graph_editor/frame/set_comment_text", p_event)) {
                    frame->_open_change_comment_text();
                } else if (ED_IS_SHORTCUT("graph_editor/frame/enable_auto_shrink", p_event)) {
                    frame->_toggle_autoshrink();
                } else if (ED_IS_SHORTCUT("graph_editor/frame/enable_tint_color", p_event)) {
                    frame->_toggle_tint();
                } else if (ED_IS_SHORTCUT("graph_editor/frame/set_tint_color", p_event)) {
                    frame->_show_tint_color_picker();
                } else {
                    handled = false;
                }
            } else {
                handled = false;
            }
        }

        if (!handled) {
            // Handle shortcuts that operate on all selected nodes
            handled = true;
            if (ED_IS_SHORTCUT("graph_editor/toggle_resizer", p_event)) {
                _toggle_resizer_for_selected_nodes();
            } else if (ED_IS_SHORTCUT("graph_editor/resize_to_content", p_event)) {
                _resize_selected_nodes_to_content();
            } else if (ED_IS_SHORTCUT("graph_editor/refresh_nodes", p_event)) {
                _refresh_selected_nodes();
            } else if (ED_IS_SHORTCUT("graph_editor/detach_from_frame", p_event)) {
                for (GraphElement* element : get_selected<GraphElement>()) {
                    if (get_element_frame(element->get_name())) {
                        _detach_node_from_frame(element->get_name());
                    }
                }
            } else if (ED_IS_SHORTCUT("graph_editor/collapse_to_function", p_event)) {
                _collapse_selected_nodes_to_function();
            } else {
                handled = false;
            }
        }

        if (handled) {
            accept_event();
        }
    }

}

void OrchestratorEditorGraphPanel::_gui_input(const Ref<InputEvent>& p_event) {
    static const std::unordered_map<StringName, Vector2> direction_map = {
        {StringName("ui_left"),  Vector2(-1,  0)},
        {StringName("ui_right"), Vector2( 1,  0)},
        {StringName("ui_up"),    Vector2( 0, -1)},
        {StringName("ui_down"),  Vector2( 0,  1)},
    };

    const Ref<InputEventMouseButton> mb = p_event;
    if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == MOUSE_BUTTON_RIGHT) {
        Dictionary hovered_connection = get_closest_connection_at_point(mb->get_position());
        if (!hovered_connection.is_empty()) {
            const Vector2 pos = mb->get_position() + get_screen_position();

            OrchestratorEditorContextMenu* menu = memnew(OrchestratorEditorContextMenu);
            menu->set_auto_destroy(true);
            add_child(menu);

            menu->add_separator("Connection Menu");
            menu->add_item("Disconnect", callable_mp_this(_disconnect_connection).bind(hovered_connection));
            menu->add_item("Insert Reroute Node", callable_mp_this(_create_connection_reroute).bind(hovered_connection, mb->get_position()));
            menu->set_position(pos);
            menu->popup();

            get_viewport()->set_input_as_handled();
            return;
        }
    }

    const Ref<InputEventKey> early_key = p_event;
    if (early_key.is_valid() && early_key->is_pressed() && !early_key->is_echo()) {
        // Intercept Shift+Delete before the parent processes ui_graph_delete, which would
        // remove nodes without giving us the chance to dissolve reroutes first.
        if (ED_IS_SHORTCUT("graph_editor/reroutes/delete_reroute", early_key)) {
            _dissolve_selected_reroutes();
            accept_event();
            return;
        }
    }

    // There is a bug where if the mouse hovers a connection and a node concurrently,
    // the connection color is changed, even when the mouse is inside the node.
    GraphEdit::_gui_input(p_event);

    const Ref<InputEventMouse> mouse = p_event;
    if (mouse.is_valid()) {
        const bool inside_node = _is_point_inside_node(mouse->get_position());
        const Ref<InputEventMouseMotion> mm = p_event;
        if (mm.is_valid()) {
            if (!inside_node) {
                _hovered_connection = get_closest_connection_at_point(mm->get_position());
                if (!_hovered_connection.is_empty()) {
                    const Ref<Shortcut> sc = ED_GET_SHORTCUT("graph_editor/reroutes/create_reroute");
                    if (sc.is_valid() && !sc->get_events().is_empty()) {
                        _show_drag_hint(vformat("Use %s to insert a reroute node on the connection.", sc->get_as_text()));
                    }
                }
            } else {
                for (int i = 0; i < get_child_count(); i++) {
                    OrchestratorEditorGraphNodeReroute* reroute = cast_to<OrchestratorEditorGraphNodeReroute>(get_child(i));
                    if (reroute && reroute->is_selected() && reroute->get_rect().has_point(mm->get_position())) {
                        Ref<Shortcut> sc = ED_GET_ACTION_SHORTCUT("ui_graph_delete");
                        if (sc.is_valid() && !sc->get_events().is_empty()) {
                            String hint = vformat("Use %s to remove the reroute and disconnect both sides.", sc->get_as_text());

                            const int reroute_id = reroute->get_id();
                            int in_count = 0, out_count = 0;
                            for (const OScriptConnection& C : _graph->get_orchestration()->get_connections()) {
                                if (C.to_node == static_cast<uint64_t>(reroute_id)) {
                                    in_count++;
                                } else if (C.from_node == static_cast<uint64_t>(reroute_id)) {
                                    out_count++;
                                }
                            }

                            if (in_count == 1 && out_count == 1) {
                                sc = ED_GET_SHORTCUT("graph_editor/reroutes/delete_reroute");
                                if (sc.is_valid() && !sc->get_events().is_empty()) {
                                    hint += vformat("\nUse %s to dissolve the reroute and keep the connection.", sc->get_as_text());
                                }
                            }

                            _show_drag_hint(hint);
                        }
                        break;
                    }
                }
            }
        }

        if (!inside_node && ED_IS_SHORTCUT("graph_editor/reroutes/create_reroute", p_event) && !_hovered_connection.is_empty()) {
            _create_connection_reroute(_hovered_connection, mouse->get_position());
        }
    }

    // todo:
    //  Submitted https://github.com/godotengine/godot/pull/95614
    //  Can eventually rely on the "cut_nodes_request" signal rather than this approach
    if (ED_IS_ACTION_SHORTCUT("ui_cut", p_event)) {
        _cut_nodes_request();
        accept_event();
        return;
    }

    const Ref<InputEventKey> key = p_event;
    if (key.is_valid() && key->is_pressed()) {
        for (const std::pair<const StringName, Vector2>& E : direction_map) {
            if (key->is_action(E.first, true)) {
                const float distance = is_snapping_enabled() ? get_snapping_distance() : 1;
                const Vector2 amount = E.second * distance;

                for_each<OrchestratorEditorGraphNode>([&] (OrchestratorEditorGraphNode* node) {
                    node->set_position_offset(node->get_position_offset() + amount);
                    node->_node->set_position(node->get_position_offset());
                }, true);

                accept_event();
                break;
            }
        }
    }
}

bool OrchestratorEditorGraphPanel::_can_drop_data(const Vector2& p_at_position, const Variant& p_data) const {
    // Widget types that can be dropped
    static PackedStringArray allowed_types = Array::make(
        "files", "obj_property", "nodes", "function", "variable", "signal");

    if (p_data.get_type() != Variant::DICTIONARY) {
        return false;
    }

    const Dictionary& data = p_data;
    if (!data.has("type")) {
        return false;
    }

    const String drop_type = data["type"];
    if (!allowed_types.has(drop_type)) {
        return false;
    }

    if (drop_type == "variable") {
        const Array& variable_data = data["variables"];
        if (!variable_data.is_empty()) {
            const String name = variable_data[0];
            const Ref<OScriptVariable> variable = _graph->get_orchestration()->get_variable(name);
            if (variable.is_valid() && !variable->is_constant()) {
                _show_drag_hint("Use Ctrl to drop a Setter, Shift to drop a Getter variable node");
            } else if (variable.is_valid()) {
                _show_drag_hint("Use Shift to drop a Getter variable node");
            }
        }
    }

    return true;
}

void OrchestratorEditorGraphPanel::_drop_data(const Vector2& p_at_position, const Variant& p_data) {
    // No need to let the hint continue to be visible when dropped
    _drag_hint->hide();

    // Since _can_drop_data validates this, this should be safe
    const Dictionary& data = p_data;
    const String drop_type = data["type"];

    // This is where the objects should spawn into the graph
    Vector2 spawn_position = (p_at_position + get_scroll_offset()) / get_zoom();;

    // Where the menu popup should spawn
    Vector2 popup_position = p_at_position + get_screen_position();

    if (drop_type == "nodes") {
        Node* edited_scene_root = get_tree()->get_edited_scene_root();
        if (!edited_scene_root) {
            return;
        }

        const Array nodes = data["nodes"];
        for (int i = 0; i < nodes.size(); i++) {
            Node* dropped_node = edited_scene_root->get_node_or_null(nodes[i]);
            if (!dropped_node) {
                continue;
            }

            NodePath path;
            if (dropped_node->is_unique_name_in_owner()) {
                path = NodePath("%" + dropped_node->get_name());
            } else {
                Vector<Node*> attached_nodes;
                SceneUtils::find_all_nodes_for_script(edited_scene_root, edited_scene_root, _graph->get_orchestration()->as_script(), attached_nodes);
                if (attached_nodes.is_empty()) {
                    ORCHESTRATOR_ERROR("Cannot drop a node in a script that is not attached to a node in this scene.");
                }

                path = attached_nodes[0]->get_path_to(dropped_node);
            }

            String global_name;
            const Ref<Script> dropped_node_script = dropped_node->get_script();
            if (dropped_node_script.is_valid()) {
                global_name = ScriptServer::get_global_name(dropped_node_script);
            }

            NodeSpawnOptions options;
            options.node_class = OScriptNodeSceneNode::get_class_static();
            options.context.node_path  = path;
            options.context.class_name = StringUtils::default_if_empty(global_name, dropped_node->get_class());
            options.position = spawn_position;

            GraphElement* element = spawn_node(options).element;
            if (element) {
                spawn_position.y += element->get_size().height + 10;
            }
        }
    } else if (drop_type == "files") {
        const Array& files = data["files"];

        OrchestratorEditorContextMenu* menu = memnew(OrchestratorEditorContextMenu);
        menu->set_auto_destroy(true);
        add_child(menu);

        menu->add_separator(files.size() == 1 ? vformat("File %s", files[0]) : vformat("%d Files", files.size()));
        menu->add_item("Get Path", callable_mp_this(_drop_data_files).bind(OScriptNodeResourcePath::get_class_static(), files, spawn_position));
        menu->add_item("Preload", callable_mp_this(_drop_data_files).bind(OScriptNodePreload::get_class_static(), files, spawn_position));

        menu->set_position(popup_position);
        menu->popup();
    } else if (drop_type == "obj_property") {
        Object* object = data["object"];
        if (!object) {
            return;
        }

        NodePath path;
        if (Node* root = get_tree()->get_edited_scene_root()) {
            if (Node* object_node = cast_to<Node>(object)) {
                path = root->get_path_to(object_node);
            }
        }

        StringName property_name = data["property"];
        for (const PropertyInfo& property : DictionaryUtils::to_properties(object->get_property_list())) {
            if (property.name == property_name) {
                OrchestratorEditorContextMenu* menu = memnew(OrchestratorEditorContextMenu);
                menu->set_auto_destroy(true);
                add_child(menu);

                Dictionary prop = DictionaryUtils::from_property(property);

                menu->add_separator("Property " + property_name);
                menu->add_item("Get " + property_name, callable_mp_this(_drop_data_property).bind(prop, spawn_position, path, false));
                menu->add_item("Set " + property_name, callable_mp_this(_drop_data_property).bind(prop, spawn_position, path, true));

                menu->set_position(popup_position);
                menu->popup();

                break;
            }
        }
    } else if (drop_type == "function") {
        const MethodInfo method = DictionaryUtils::to_method(data["functions"]);

        OrchestratorEditorContextMenu* menu = memnew(OrchestratorEditorContextMenu);
        menu->set_auto_destroy(true);
        add_child(menu);

        menu->add_separator("Function " + method.name);
        menu->add_item("Add Call to Function", callable_mp_this(_drop_data_function).bind(data["functions"], spawn_position, false));
        menu->add_item("Add as a Callable", callable_mp_this(_drop_data_function).bind(data["functions"], spawn_position, true));

        menu->set_position(popup_position);
        menu->popup();
    } else if (drop_type == "variable") {
        const Array& variables = data["variables"];
        if (variables.is_empty()) {
            return;
        }

        const String variable_name = variables[0];
        const Ref<OScriptVariable> variable = _graph->get_orchestration()->get_variable(variable_name);
        if (!variable.is_valid()) {
            return;
        }

        if (Input::get_singleton()->is_key_pressed(KEY_CTRL) && !variable->is_constant()) {
            _drop_data_variable(variable_name, spawn_position, false, true);
        } else if (Input::get_singleton()->is_key_pressed(KEY_SHIFT)) {
            _drop_data_variable(variable_name, spawn_position, false, false);
        } else {
            OrchestratorEditorContextMenu* menu = memnew(OrchestratorEditorContextMenu);
            menu->set_auto_destroy(true);
            add_child(menu);

            menu->add_separator("Variable " + variable_name);
            menu->add_item("Get " + variable_name, callable_mp_this(_drop_data_variable)
                .bind(variable_name, spawn_position, false, false));

            if (variable->get_info().type == Variant::OBJECT) {
                menu->add_item("Get " + variable_name + " with validation",
                    callable_mp_this(_drop_data_variable).bind(variable_name, spawn_position, true, false));
            }

            if (!variable->is_constant()) {
                menu->add_item("Set " + variable_name,
                    callable_mp_this(_drop_data_variable).bind(variable_name, spawn_position, false, true));
            }

            menu->set_position(popup_position);
            menu->popup();
        }
    } else if (drop_type == "signal") {
        NodeSpawnOptions options;
        options.node_class = OScriptNodeEmitSignal::get_class_static();
        options.context.method = DictionaryUtils::to_method(data["signals"]);
        options.position = spawn_position;

        spawn_node(options);
    }
}

PackedVector2Array OrchestratorEditorGraphPanel::_get_connection_line(const Vector2& p_from_position, const Vector2& p_to_position) const {
    const Vector2 from_adjusted = p_from_position;
    const Vector2 to_adjusted = p_to_position;

    int source_node_id = -1, source_node_port = -1;
    int target_node_id = -1, target_node_port = -1;
    _get_graph_node_and_port(from_adjusted, source_node_id, source_node_port);
    _get_graph_node_and_port(to_adjusted, target_node_id, target_node_port);

    const bool source_is_reroute = source_node_id != -1
        && cast_to<OrchestratorEditorGraphNodeReroute>(find_child(itos(source_node_id), false, false));
    const bool target_is_reroute = target_node_id != -1
        && cast_to<OrchestratorEditorGraphNodeReroute>(find_child(itos(target_node_id), false, false));

    // Suppress curvature between two reroute nodes so the wire runs straight, matching
    // the original knot behavior where only the first and last segments were curved.
    const float curvature = (source_is_reroute && target_is_reroute) ? 0.0f : get_connection_lines_curvature();

    float xdiff = p_from_position.x - p_to_position.x;
    float cp_offset = xdiff * curvature;
    if (xdiff < 0) {
        cp_offset *= -1;
    }

    Ref<Curve2D> curve;
    curve.instantiate();
    curve->add_point(p_from_position);
    curve->set_point_out(0, Vector2(cp_offset, 0));
    curve->add_point(p_to_position);
    curve->set_point_in(1, Vector2(-cp_offset, 0));

    return curvature > 0 ? curve->tessellate(5, 2.0) : curve->tessellate(1);
}

bool OrchestratorEditorGraphPanel::_is_node_hover_valid(const StringName& p_from_node, int32_t p_from_port,
                                                        const StringName& p_to_node, int32_t p_to_port) {
    OrchestratorEditorGraphNode* source = find_node(p_from_node);
    ERR_FAIL_NULL_V_MSG(source, false, "Failed to locate source node with name " + p_from_node);

    OrchestratorEditorGraphPin* source_pin = source->get_output_pin(p_from_port);
    ERR_FAIL_NULL_V_MSG(source_pin, false, "Failed to locate source node pin at port " + itos(p_from_port));

    OrchestratorEditorGraphNode* target = find_node(p_to_node);
    ERR_FAIL_NULL_V_MSG(target, false, "Failed to locate target node with name " + p_to_node);

    OrchestratorEditorGraphPin* target_pin = target->get_input_pin(p_to_port);
    ERR_FAIL_NULL_V_MSG(target_pin, false, "Failed to locate target node pin at port " + itos(p_to_port));

    return target_pin->_pin->can_accept(source_pin->_pin);
}

bool OrchestratorEditorGraphPanel::_is_in_input_hotzone(Object* p_in_node, int32_t p_in_port, const Vector2& p_mouse_position) {
    GraphNode* node = cast_to<GraphNode>(p_in_node);
    if (!node) {
        return false;
    }

    const int slot_index = node->get_input_port_slot(p_in_port);
    const Ref<Texture2D> icon = node->get_slot_custom_icon_left(slot_index);
    if (!icon.is_valid()) {
        return false;
    }

    Vector2i port_size = Vector2i(icon->get_width(), icon->get_height());
    Control* child = cast_to<Control>(node->get_child(slot_index, false));
    port_size.height = MAX(port_size.height, child ? child->get_size().y : 0);

    const float zoom = get_zoom();
    const Vector2 pos = node->get_input_port_position(p_in_port) * zoom + node->get_position();
    return _is_in_port_hotzone(pos / zoom, p_mouse_position, port_size, true);
}

bool OrchestratorEditorGraphPanel::_is_in_output_hotzone(Object* p_in_node, int32_t p_in_port, const Vector2& p_mouse_position) {
    GraphNode* node = cast_to<GraphNode>(p_in_node);
    if (!node) {
        return false;
    }

    const int slot_index = node->get_output_port_slot(p_in_port);
    const Ref<Texture2D> icon = node->get_slot_custom_icon_right(slot_index);
    if (!icon.is_valid()) {
        return false;
    }

    Vector2i port_size = Vector2i(icon->get_width(), icon->get_height());
    Control* child = cast_to<Control>(node->get_child(slot_index, false));
    port_size.height = MAX(port_size.height, child ? child->get_size().y : 0);

    const float zoom = get_zoom();
    const Vector2 pos = node->get_output_port_position(p_in_port) * zoom + node->get_position();
    return _is_in_port_hotzone(pos / zoom, p_mouse_position, port_size, false);
}

void OrchestratorEditorGraphPanel::set_graph(const Ref<OrchestrationGraph>& p_graph) {
    ERR_FAIL_COND_MSG(!p_graph.is_valid(), "The provided graph panel model is invalid");

    const bool reload = _graph.is_valid();

    _graph = p_graph;

    set_name(_graph->get_graph_name());

    // When nodes are spawned or removed, this triggers a panel rebuild based on the model
    _graph->connect("node_added", callable_mp_this(_node_added));
    _graph->connect("node_removed", callable_mp_this(_node_removed));
    _graph->connect(CoreStringName(changed), callable_mp_this(_graph_changed));

    if (!reload) {
        // When model triggers link/unlink, makes sure the UI updates
        // Great use case is when changing a variable type where a connection is no longer valid
        _graph->get_orchestration()->connect("connections_changed", callable_mp_this(_refresh_panel_connections_with_model));
        _graph->get_orchestration()->connect(CoreStringName(changed), callable_mp_this(_update_panel_hint));
    }

    callable_mp_this(_update_panel_hint).call_deferred();
    _queue_panel_refresh();
}

void OrchestratorEditorGraphPanel::reloaded_from_file() {
    _queue_panel_refresh();
}

void OrchestratorEditorGraphPanel::idle_timeout() {
}

Control* OrchestratorEditorGraphPanel::get_menu_control() const {
    return _toolbar_hflow;
}

Node* OrchestratorEditorGraphPanel::get_connection_layer_node() const {
    for (int i = 0; i < get_child_count(); i++) {
        Node* child = get_child(i);
        if (child->get_name().match("_connection_layer")) {
            return child;
        }
    }
    return nullptr;
}

bool OrchestratorEditorGraphPanel::is_bookmarked(const OrchestratorEditorGraphNode* p_node) const {
    return _markers->is_bookmarked(p_node);
}

void OrchestratorEditorGraphPanel::set_bookmarked(OrchestratorEditorGraphNode* p_node, bool p_bookmarked) {
    _markers->set_bookmarked(p_node, p_bookmarked);
}

void OrchestratorEditorGraphPanel::goto_next_bookmark() {
    _markers->goto_next_bookmark();
}

void OrchestratorEditorGraphPanel::goto_previous_bookmark() {
    _markers->goto_previous_bookmark();
}

bool OrchestratorEditorGraphPanel::is_breakpoint(const OrchestratorEditorGraphNode* p_node) const {
    return _markers->is_breakpoint(p_node);
}

void OrchestratorEditorGraphPanel::set_breakpoint(OrchestratorEditorGraphNode* p_node, bool p_breakpoint) {
    _markers->set_breakpoint(p_node, p_breakpoint);
}

bool OrchestratorEditorGraphPanel::get_breakpoint(OrchestratorEditorGraphNode* p_node) {
    return _markers->get_breakpoint(p_node);
}

void OrchestratorEditorGraphPanel::goto_next_breakpoint() {
    _markers->goto_next_breakpoint();
}

void OrchestratorEditorGraphPanel::goto_previous_breakpoint() {
    _markers->goto_previous_breakpoint();
}

PackedInt32Array OrchestratorEditorGraphPanel::get_breakpoints() const {
    return _markers->get_breakpoints();
}

void OrchestratorEditorGraphPanel::clear_breakpoints() {
    _markers->clear_breakpoints();
}

void OrchestratorEditorGraphPanel::show_override_function_action_menu(const Callable& p_callback) {
    _menu_position = _get_center();
    _treat_call_member_as_override = true;

    Ref<OrchestratorEditorActionGraphTypeRule> graph_type_rule;
    graph_type_rule.instantiate();
    graph_type_rule->set_graph_type(OrchestratorEditorActionDefinition::GRAPH_EVENT);

    PackedStringArray user_defined_methods;
    const TypedArray<Dictionary> methods = _graph->get_orchestration()->as_script()->get_script_method_list();
    for (int i = 0; i < methods.size(); i++) {
        const MethodInfo mi = DictionaryUtils::to_method(methods[i]);
        if (!user_defined_methods.has(mi.name)) {
            user_defined_methods.push_back(mi.name);
        }
    }

    Ref<OrchestratorEditorActionVirtualFunctionRule> virtual_function_rule;
    virtual_function_rule.instantiate();
    virtual_function_rule->set_method_exclusions(_graph->get_orchestration()->get_function_names());
    virtual_function_rule->set_method_overrides(user_defined_methods);

    Ref<OrchestratorEditorActionClassHierarchyScopeRule> class_hierarchy_rule;
    class_hierarchy_rule.instantiate();
    class_hierarchy_rule->set_script_classes(_graph->get_orchestration()->as_script());

    Ref<OrchestratorEditorActionFilterEngine> filter_engine;
    filter_engine.instantiate();
    filter_engine->add_rule(memnew(OrchestratorEditorActionSearchTextRule));
    filter_engine->add_rule(class_hierarchy_rule);
    filter_engine->add_rule(virtual_function_rule);
    filter_engine->add_rule(graph_type_rule);

    OrchestratorEditorActionMenu* menu = memnew(OrchestratorEditorActionMenu);
    menu->set_title("Select a graph action");
    menu->set_suffix("graph_editor_overrides");
    menu->set_close_on_focus_lost(ORCHESTRATOR_GET("interface/editor/actions_menu/close_on_focus_lost", false));
    menu->set_show_filter_option(false);
    menu->set_start_collapsed(false);
    if (p_callback.is_valid()) {
        // Callback should be attached first as a pre-handler
        menu->connect("action_selected", p_callback);
    }
    menu->connect("action_selected", callable_mp_this(_action_menu_selection));
    menu->connect(SceneStringName(canceled), callable_mp_this(_action_menu_canceled));

    menu->popup_centered(
        OrchestratorEditorActionRegistry::get_singleton()->get_actions(_graph->get_orchestration()->as_script()),
        filter_engine);
}

bool OrchestratorEditorGraphPanel::are_pins_compatible(OrchestratorEditorGraphPin* p_source, OrchestratorEditorGraphPin* p_target) const {
    // todo:
    //  pull OrchestrationGraphPin logic up or rework
    //  variable node implementations use this to deal with variable type changes
    //  base node uses this during build validation
    return p_source->_pin->can_accept(p_target->_pin);
}

void OrchestratorEditorGraphPanel::link(OrchestratorEditorGraphPin* p_source, OrchestratorEditorGraphPin* p_target) {
    p_source->_pin->link(p_target->_pin);
    _set_edited(true);

    _refresh_panel_connections_with_model();
}

void OrchestratorEditorGraphPanel::unlink(OrchestratorEditorGraphPin* p_source, OrchestratorEditorGraphPin* p_target) {
    p_source->_pin->unlink(p_target->_pin);
    _set_edited(true);

    _refresh_panel_connections_with_model();
}

void OrchestratorEditorGraphPanel::unlink_all(OrchestratorEditorGraphPin* p_target, bool p_notify) {
    p_target->_pin->unlink_all(p_notify);
    _set_edited(true);

    _refresh_panel_connections_with_model();
}

void OrchestratorEditorGraphPanel::unlink_node_all(OrchestratorEditorGraphNode* p_node) {
    ERR_FAIL_NULL_MSG(p_node, "Cannot remove all node links with an invalid node reference");

    for (OrchestratorEditorGraphPin* pin : p_node->get_pins())
        pin->_pin->unlink_all(true);

    _set_edited(true);
    _refresh_panel_connections_with_model();
}

HashSet<OrchestratorEditorGraphNode*> OrchestratorEditorGraphPanel::get_connected_nodes(OrchestratorEditorGraphNode* p_node) {
    const int node_id = p_node->get_id();

    HashSet<OrchestratorEditorGraphNode*> connections;
    for (const Connection& E : _graph->get_connections()) {
        if (E.from_node == node_id) {
            connections.insert(find_node(E.to_node));
        } else if (E.to_node == node_id) {
            connections.insert(find_node(E.from_node));
        }
    }

    return connections;
}

HashSet<OrchestratorEditorGraphPin*> OrchestratorEditorGraphPanel::get_connected_pins(OrchestratorEditorGraphPin* p_pin) {
    ERR_FAIL_NULL_V_MSG(p_pin, {}, "Cannot get connected pins for an invalid pin");

    const int32_t pin_port = p_pin->get_graph_node()->get_pin_port(p_pin);
    ERR_FAIL_COND_V_MSG(pin_port == -1, {}, "Failed to resolve pin port");

    const int node_id = p_pin->get_graph_node()->get_id();

    HashSet<OrchestratorEditorGraphPin*> connections;
    for (const Connection& E : _graph->get_connections()) {
        if (E.from_node == node_id && E.from_port == pin_port && p_pin->get_direction() == PD_Output) {
            // Found connection from this pin.
            OrchestratorEditorGraphNode* target_node = find_node(E.to_node);
            if (target_node) {
                const int32_t to_slot = target_node->get_input_port_slot(E.to_port);
                connections.insert(target_node->get_input_pin(to_slot));
            }
        } else if (E.to_node == node_id && E.to_port == pin_port && p_pin->get_direction() == PD_Input) {
            // Found connection to this pin.
            OrchestratorEditorGraphNode* source_node = find_node(E.from_node);
            if (source_node) {
                const int32_t to_slot = source_node->get_output_port_slot(E.from_port);
                connections.insert(source_node->get_output_pin(to_slot));
            }
        }
    }

    return connections;
}

void OrchestratorEditorGraphPanel::remove_node(OrchestratorEditorGraphNode* p_node, bool p_confirm) {
    if (p_confirm && _is_delete_confirmation_enabled()) {
        ORCHESTRATOR_CONFIRM("Do you wish to delete this node?", callable_mp_this(remove_node).bind(p_node, false));
    }

    _markers->set_bookmarked(p_node, false);
    _markers->set_breakpoint(p_node, false);

    if (p_node->is_selected()) {
        p_node->set_selected(false);
    }

    p_node->queue_free();

    const int node_id = p_node->get_id();
    _graph->get_orchestration()->remove_node(node_id);

    // This makes sure that we only ever emit 1 event during bulk node removal
    if (!_pending_nodes_changed_event) {
        _set_edited(true);
        _pending_nodes_changed_event = true;
        callable_mp_lambda(this, [&] {
            _pending_nodes_changed_event = false;
            emit_signal("nodes_changed");
        }).call_deferred();
    }
}

void OrchestratorEditorGraphPanel::remove_nodes(const TypedArray<OrchestratorEditorGraphNode>& p_nodes, bool p_confirm) {
    if (p_confirm && _is_delete_confirmation_enabled()) {
        ORCHESTRATOR_CONFIRM(vformat("Do you wish to delete %d node(s)?", p_nodes.size()),
            callable_mp_this(remove_nodes).bind(p_nodes, false));
    }

    for (int i = 0; i < p_nodes.size(); i++) {
        OrchestratorEditorGraphNode* node = cast_to<OrchestratorEditorGraphNode>(p_nodes[i]);
        if (node && node->can_user_delete_node()) {
            remove_node(node, false);
        }
    }
}

void OrchestratorEditorGraphPanel::remove_selected_nodes(bool p_confirm) {
    Vector<OrchestratorEditorGraphNode*> selected_nodes = get_selected<OrchestratorEditorGraphNode>();
    if (p_confirm && _is_delete_confirmation_enabled()) {
        ORCHESTRATOR_CONFIRM(vformat("Do you wish to delete %d node(s)?", selected_nodes.size()),
            callable_mp_this(remove_nodes).bind(false));
    }

    for (int i = 0; i < selected_nodes.size(); i++) {
        OrchestratorEditorGraphNode* node = selected_nodes[i];
        if (node && node->can_user_delete_node()) {
            remove_node(node, false);
        }
    }
}

void OrchestratorEditorGraphPanel::remove_frame(OrchestratorEditorGraphFrame* p_frame, bool p_confirm) {
    if (p_confirm && _is_delete_confirmation_enabled()) {
        ORCHESTRATOR_CONFIRM("Do you wish to delete this frame?", callable_mp_this(remove_frame).bind(p_frame, false));
    }

    if (p_frame->is_selected()) {
        p_frame->set_selected(false);
    }

    _detach_node_from_frame(p_frame->get_name());

    p_frame->queue_free();

    const Ref<OScriptNodeComment> comment = p_frame->get_comment();
    if (comment.is_valid()) {
        _graph->get_orchestration()->remove_node(comment->get_id());
    }

    if (!_pending_nodes_changed_event) {
        _set_edited(true);
        _pending_nodes_changed_event = true;
        callable_mp_lambda(this, [&] {
            _pending_nodes_changed_event = false;
            emit_signal("nodes_changed");
        }).call_deferred();
    }
}

OrchestratorEditorGraphNode* OrchestratorEditorGraphPanel::find_node(int p_id) {
    return find<OrchestratorEditorGraphNode>(itos(p_id));
}

OrchestratorEditorGraphNode* OrchestratorEditorGraphPanel::find_node(const StringName& p_name) {
    return find<OrchestratorEditorGraphNode>(p_name);
}

OrchestratorEditorGraphFrame* OrchestratorEditorGraphPanel::find_frame(const StringName& p_name) {
    return find<OrchestratorEditorGraphFrame>(p_name);
}

void OrchestratorEditorGraphPanel::clear_selections() {
    for_each<GraphElement>([&] (GraphElement* element) {
        element->set_selected(false);
    });
}

void OrchestratorEditorGraphPanel::select_nodes(const PackedInt64Array& p_ids) {
    clear_selections();

    for (const int64_t id : p_ids) {
        if (OrchestratorEditorGraphNode* node = find_node(id)) {
            node->set_selected(true);
        }
    }
}

int64_t OrchestratorEditorGraphPanel::get_selection_count() {
    return get_selected<GraphElement>().size();
}

Rect2 OrchestratorEditorGraphPanel::get_bounds_for_nodes(bool p_only_selected, bool p_padding) {
    const Vector<OrchestratorEditorGraphNode*> nodes = get_all<OrchestratorEditorGraphNode>(p_only_selected);
    if (nodes.is_empty()) {
        return {};
    }
    return get_bounds_for_nodes(nodes, p_padding);
}

Rect2 OrchestratorEditorGraphPanel::get_bounds_for_nodes(const Vector<OrchestratorEditorGraphNode*>& p_nodes, bool p_padding) {
    Rect2 bounds = p_nodes[0]->get_graph_rect().grow(p_padding);
    for (int i = 1; i < p_nodes.size(); i++) {
        bounds = bounds.merge(p_nodes[i]->get_graph_rect().grow(p_padding));
    }
    return bounds;
}

void OrchestratorEditorGraphPanel::scroll_to_position(const Vector2& p_position, float p_time) {
    // The provided position needs to be offset by half the viewport size to center on the position.
    const Vector2& position = p_position - (get_size() / 2.0);

    const Ref<Tween> tween = get_tree()->create_tween();
    if (!UtilityFunctions::is_equal_approx(1.f, get_zoom())) {
        tween->tween_method(Callable(this, "set_zoom"), get_zoom(), 1.f, p_time);
    }

    tween->chain()->tween_method(Callable(this, "set_scroll_offset"), get_scroll_offset(), position, p_time);
    tween->set_ease(Tween::EASE_IN_OUT);

    tween->play();
}

void OrchestratorEditorGraphPanel::center_node_id(int p_id) {
    // Attempts to locate the node and if found, proceeds to center it.
    if (OrchestratorEditorGraphNode* node = find_node(p_id)) {
        center_node(node);
        return;
    }

    // This may often be called from a sequence where the graph is first opened
    // and the graph node instance isn't yet available. In this case, centering
    // the node must be deferred until the graph is loaded.
    callable_mp_lambda(this, [p_id, this] {
        center_node(find_node(p_id));
    }).call_deferred();
}

void OrchestratorEditorGraphPanel::center_node(OrchestratorEditorGraphNode* p_node) {
    GUARD_NULL(p_node);

    clear_selections();
    p_node->set_selected(true);

    scroll_to_position(p_node->get_graph_rect().get_center());
}

void OrchestratorEditorGraphPanel::straighten_all_connections(OrchestratorEditorGraphPin* p_pin) {
    for (OrchestratorEditorGraphPin* connection : get_connected_pins(p_pin)) {
        if (p_pin->get_direction() == PD_Output) {
            straighten_connection(p_pin, connection);
        } else {
            straighten_connection(connection, p_pin);
        }
    }
}

void OrchestratorEditorGraphPanel::straighten_connection(OrchestratorEditorGraphPin* p_source, OrchestratorEditorGraphPin* p_target) {
    GUARD_NULL(p_source);
    GUARD_NULL(p_target);

    OrchestratorEditorGraphNode* source_node = p_source->get_graph_node();
    const Vector2 source_node_position = source_node->get_position_offset();
    const Vector2 source_pin_position = source_node_position + source_node->get_port_position_for_pin(p_source);

    OrchestratorEditorGraphNode* target_node = p_target->get_graph_node();
    Vector2 target_node_position = target_node->get_position_offset();
    const Vector2 target_pin_position = target_node_position + target_node->get_port_position_for_pin(p_target);

    Connection connection;
    connection.from_node = source_node->get_id();
    connection.from_port = source_node->get_pin_port(p_source);
    connection.to_node = target_node->get_id();
    connection.to_port = target_node->get_pin_port(p_target);

    target_node_position.y += source_pin_position.y - target_pin_position.y;
    target_node->_node->set_position(target_node_position);
}

OrchestratorEditorGraphPanel::NodeSpawnResult OrchestratorEditorGraphPanel::spawn_node(const NodeSpawnOptions& p_options) {
    ERR_FAIL_COND_V_MSG(p_options.node_class.is_empty(), {}, "No node class specified, cannot spawn node");
    ERR_FAIL_COND_V_MSG(!_graph.is_valid(), {}, "Cannot spawn into an invalid graph");

    const OScriptNodeInitContext& context = p_options.context;
    const Vector2& position = p_options.position;

    const Ref<OScriptNode> spawned_node = _graph->create_node(p_options.node_class, context, position);
    ERR_FAIL_COND_V_MSG(!spawned_node.is_valid(), {}, "Failed to spawn node");

    _set_edited(true);
    emit_signal("nodes_changed");

    GraphElement* element = cast_to<GraphElement>(find_child(itos(spawned_node->get_id()), false, false));
    ERR_FAIL_NULL_V_MSG(element, {}, "Failed to find spawned graph element");

    NodeSpawnResult result;
    result.element = element;
    result.node = cast_to<OrchestratorEditorGraphNode>(element);
    result.frame = cast_to<OrchestratorEditorGraphFrame>(element);

    if (p_options.select_on_spawn) {
        element->set_selected(true);
    }

    if (result.node) {
        if (p_options.center_on_spawn) {
            callable_mp_this(center_node).bind(result.node).call_deferred();
        }
        if (p_options.drag_pin) {
            // When dragging from a pin, this indicates that autowiring should happen, but this needs to be done
            // as part of the next frame. This allows the caller to get a reference to the spawned node so it
            // can continue to perform any additional operations without having to deal with async operations
            // with the autowire dialog window.
            callable_mp_this(_queue_autowire).bind(result.node, p_options.drag_pin).call_deferred();
        }
    }

    return result;
}

Variant OrchestratorEditorGraphPanel::get_edit_state() const {
    // When a cached layout is restored but never applied, this means the tab was never made visible.
    // In this case, the GraphEdit state cannot be used, so return the cached state verbatim.
    if (!_initialized && !_edit_state.is_empty()) {
        return _edit_state;
    }

    PackedStringArray selections;
    for (int i = 0; i < get_child_count(); i++) {
        OrchestratorEditorGraphNode* node = cast_to<OrchestratorEditorGraphNode>(get_child(i));
        if (node && node->is_selected()) {
            selections.push_back(node->get_name());
        }
    }

    Dictionary panel_state;
    panel_state["name"] = get_name();
    panel_state["viewport_offset"] = get_scroll_offset();
    panel_state["zoom"] = get_zoom();
    panel_state["selections"] = selections;
    panel_state["minimap"] = is_minimap_enabled();
    panel_state["snapping"] = is_snapping_enabled();
    panel_state["grid"] = is_showing_grid();
    panel_state["grid_pattern"] = get_grid_pattern();

    _markers->save_state(panel_state);

    return panel_state;
}

void OrchestratorEditorGraphPanel::set_edit_state(const Variant& p_state, const Callable& p_completion_callback) {
    // Cache the state and it will be applied when the panel first becomes visible.
    // Applying the scroll offset requires the panel to be laid out, which only happens when visible.
    _edit_state = p_state;
    _initialized = false;

    if (is_visible_in_tree()) {
        _schedule_restore();
    }

    if (p_completion_callback.is_valid()) {
        p_completion_callback.call_deferred();
    }
}

void OrchestratorEditorGraphPanel::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_THEME_CHANGED: {
            _update_theme_item_cache();
            _update_menu_theme();
            break;
        }
        case NOTIFICATION_VISIBILITY_CHANGED: {
            if (is_visible_in_tree()) {
                // Builds the graph nodes and connections if a refresh is pending from while the graph was
                // not visible. Afterward, the cached layout state is applied.
                if (_panel_refresh_pending) {
                    _schedule_refresh();
                }
                if (!_initialized) {
                    _schedule_restore();
                }
            }
            break;
        }
        default: {
            break;
        }
    }
}

void OrchestratorEditorGraphPanel::_bind_methods() {
    ADD_SIGNAL(MethodInfo("validate_script"));
    ADD_SIGNAL(MethodInfo("connection_pin_drag_started", PropertyInfo(Variant::OBJECT, "pin")));
    ADD_SIGNAL(MethodInfo("connection_pin_drag_ended"));

    ADD_SIGNAL(MethodInfo("focus_requested", PropertyInfo(Variant::OBJECT, "object")));

    ADD_SIGNAL(MethodInfo("nodes_changed"));

    // Used to notify parent type to focus & edit the function
    ADD_SIGNAL(MethodInfo("edit_function_requested", PropertyInfo(Variant::STRING, "function_name")));

    ADD_SIGNAL(MethodInfo("breakpoint_changed", PropertyInfo(Variant::INT, "node_id"), PropertyInfo(Variant::BOOL, "enabled")));
    ADD_SIGNAL(MethodInfo("breakpoint_added", PropertyInfo(Variant::INT, "node_id")));
    ADD_SIGNAL(MethodInfo("breakpoint_removed", PropertyInfo(Variant::INT, "node_id")));

    // Used by the styler for when highlighting with unlink operations
    ADD_SIGNAL(MethodInfo("connections_changed"));
}

OrchestratorEditorGraphPanel::OrchestratorEditorGraphPanel() {
    _markers = memnew(OrchestratorEditorGraphMarkers);
    _markers->initialize(this);

    _styler.instantiate();
    _styler->set_graph_panel(this);

    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);

    get_menu_hbox()->set_h_size_flags(SIZE_EXPAND_FILL);
    get_menu_hbox()->add_child(memnew(VSeparator));
    get_menu_hbox()->move_child(get_menu_hbox()->get_child(-1), 4);

    // Empty graph message
    Label* label = memnew(Label);
    label->set_text("Use Right Mouse Button To Add New Nodes");
    label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
    label->add_theme_font_size_override(SceneStringName(font_size), 24);

    _center_status = memnew(CenterContainer);
    _center_status->set_anchors_preset(PRESET_FULL_RECT);
    _center_status->add_child(label);
    _center_status->set_visible(false);
    add_child(_center_status);

    // A label that provides hint details when dragging into the editor
    _drag_hint = memnew(Label);
    _drag_hint->set_anchor_and_offset(SIDE_TOP, ANCHOR_END, 0);
    _drag_hint->set_anchor_and_offset(SIDE_BOTTOM, ANCHOR_END, -50);
    _drag_hint->set_anchor_and_offset(SIDE_RIGHT, ANCHOR_END, 0);
    _drag_hint->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
    _drag_hint->set_vertical_alignment(VERTICAL_ALIGNMENT_BOTTOM);
    add_child(_drag_hint);

    _drag_hint_timer = memnew(Timer);
    _drag_hint_timer->set_wait_time(5);
    _drag_hint_timer->connect("timeout", callable_mp_cast(_drag_hint, CanvasItem, hide));
    add_child(_drag_hint_timer);

    // Limits the frequency of theme updates from ProjectSettings
    _theme_update_timer = memnew(Timer);
    _theme_update_timer->set_wait_time(0.5f);
    _theme_update_timer->set_one_shot(true);
    add_child(_theme_update_timer);

    // New dots-based grid style was introduced in Godot 4.3.
    // Introduces a new drop-down option for selecting the specific grid pattern
    const String grid_pattern = ORCHESTRATOR_GET("interface/editor/graph/grid_pattern", "Lines");
    const int selected = grid_pattern == "Lines" ? 0 : 1;
    _grid_pattern = memnew(OptionButton);
    _grid_pattern->add_item("Lines");
    _grid_pattern->set_item_metadata(0, GRID_PATTERN_LINES);
    _grid_pattern->add_item("Dots");
    _grid_pattern->set_item_metadata(1, GRID_PATTERN_DOTS);
    _grid_pattern->connect(SceneStringName(item_selected), callable_mp_this(_grid_pattern_changed));
    _grid_pattern->select(selected);
    set_grid_pattern(CAST_INT_TO_ENUM(GridPattern, _grid_pattern->get_item_metadata(selected)));

    get_menu_hbox()->add_child(_grid_pattern);
    get_menu_hbox()->move_child(_grid_pattern, 5);

    VSeparator* sep = memnew(VSeparator());
    get_menu_hbox()->add_child(sep);
    get_menu_hbox()->move_child(sep, 6);

    set_minimap_enabled(ORCHESTRATOR_GET("interface/editor/graph/show_minimap", false));
    set_show_arrange_button(ORCHESTRATOR_GET("interface/editor/graph/show_arrange_button", false));
    set_show_grid(ORCHESTRATOR_GET("interface/editor/graph/grid_enabled", true));
    set_snapping_enabled(ORCHESTRATOR_GET("interface/editor/graph/grid_snapping_enabled", true));
    set_right_disconnects(true);
    set_show_zoom_label(true);

    // Slot type 2 = ANY reroute: allow connecting to both execution (0) and data (1) ports
    add_valid_connection_type(2, 0);
    add_valid_connection_type(2, 1);
    add_valid_connection_type(0, 2);
    add_valid_connection_type(1, 2);

    OrchestratorProjectSettingsCache::get_singleton()->connect("settings_changed", callable_mp_this(_settings_changed));
    EI->get_editor_settings()->connect("settings_changed", callable_mp_this(_settings_changed));

    _settings_changed();

    PanelContainer* toolbar_panel = static_cast<PanelContainer*>(get_menu_hbox()->get_parent());
    toolbar_panel->set_anchors_and_offsets_preset(PRESET_TOP_WIDE, PRESET_MODE_MINSIZE, 10);
    toolbar_panel->set_mouse_filter(MOUSE_FILTER_IGNORE);

    _toolbar_hflow = memnew(HFlowContainer);
    {
        Vector<Node*> nodes;
        for (int i = 0; i < get_menu_hbox()->get_child_count(); i++) {
            nodes.push_back(get_menu_hbox()->get_child(i));
        }

        for (Node* node : nodes) {
            get_menu_hbox()->remove_child(node);
            _toolbar_hflow->add_child(node);
        }

        get_menu_hbox()->hide();
        toolbar_panel->add_child(_toolbar_hflow);
    }

    connect("child_entered_tree", callable_mp_this(_child_entered_tree));
    connect("child_exiting_tree", callable_mp_this(_child_exiting_tree));
    connect("connection_from_empty", callable_mp_this(_connection_from_empty));
    connect("connection_to_empty", callable_mp_this(_connection_to_empty));
    connect("connection_request", callable_mp_this(_connection_request));
    connect("disconnection_request", callable_mp_this(_disconnection_request));
    connect("popup_request", callable_mp_this(_popup_request));
    connect("node_selected", callable_mp_this(_node_selected));
    connect("node_deselected", callable_mp_this(_node_deselected));
    connect("delete_nodes_request", callable_mp_this(_delete_nodes_request));
    connect("connection_drag_started", callable_mp_this(_connection_drag_started));
    connect("connection_drag_ended", callable_mp_this(_connection_drag_ended));
    connect("copy_nodes_request", callable_mp_this(_copy_nodes_request));
    connect("duplicate_nodes_request", callable_mp_this(_duplicate_nodes_request));
    connect("paste_nodes_request", callable_mp_this(_paste_nodes_request));
    connect("begin_node_move", callable_mp_this(_begin_node_move));
    connect("end_node_move", callable_mp_this(_end_node_move));
    connect("scroll_offset_changed", callable_mp_this(_scroll_offset_changed));
    connect("graph_elements_linked_to_frame_request", callable_mp_this(_graph_elements_linked_to_frame_request));
}

OrchestratorEditorGraphPanel::~OrchestratorEditorGraphPanel() {
    memdelete(_markers);
    _markers = nullptr;
}
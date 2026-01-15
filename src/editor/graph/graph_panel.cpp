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
#include "common/method_utils.h"
#include "common/name_utils.h"
#include "common/property_utils.h"
#include "common/scene_utils.h"
#include "common/settings.h"
#include "common/string_utils.h"
#include "common/variant_utils.h"
#include "editor/actions/filter_engine.h"
#include "editor/actions/menu.h"
#include "editor/actions/registry.h"
#include "editor/autowire_connection_dialog.h"
#include "editor/context_menu.h"
#include "editor/debugger/script_debugger_plugin.h"
#include "editor/dialogs_helper.h"
#include "editor/graph/graph_node.h"
#include "editor/graph/graph_node_factory.h"
#include "editor/graph/graph_panel_styler.h"
#include "editor/graph/graph_pin.h"
#include "editor/graph/knot_editor.h"
#include "editor/graph/nodes/comment_graph_node.h"
#include "editor/graph/nodes/knot_node.h"
#include "orchestration/orchestration.h"
#include "script/graph.h"
#include "script/nodes/data/compose.h"
#include "script/nodes/data/dictionary.h"
#include "script/nodes/editable_pin_node.h"
#include "script/nodes/functions/call_member_function.h"
#include "script/nodes/functions/call_script_function.h"
#include "script/nodes/functions/event.h"
#include "script/nodes/functions/function_result.h"
#include "script/nodes/math/operator_node.h"
#include "script/nodes/properties/property_get.h"
#include "script/nodes/properties/property_set.h"
#include "script/nodes/resources/preload.h"
#include "script/nodes/resources/resource_path.h"
#include "script/nodes/scene/scene_node.h"
#include "script/nodes/signals/emit_member_signal.h"
#include "script/nodes/signals/emit_signal.h"
#include "script/nodes/utilities/self.h"
#include "script/nodes/variables/variable_get.h"
#include "script/nodes/variables/variable_set.h"
#include "script/script.h"
#include "script/script_server.h"

#include <godot_cpp/classes/center_container.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
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

#define IS_COMMENT(n) cast_to<OrchestratorEditorGraphNodeComment>(n) != nullptr

OrchestratorEditorGraphPanel::CopyBuffer OrchestratorEditorGraphPanel::_copy_buffer;

using Connection = OScriptConnection;

void OrchestratorEditorGraphPanel::_child_entered_tree(Node* p_node)
{
    if (OrchestratorEditorGraphNode* node = cast_to<OrchestratorEditorGraphNode>(p_node))
        _connect_graph_node_signals(node);
}

void OrchestratorEditorGraphPanel::_child_exiting_tree(Node* p_node)
{
    if (OrchestratorEditorGraphNode* node = cast_to<OrchestratorEditorGraphNode>(p_node))
        _disconnect_graph_node_signals(node);
}

void OrchestratorEditorGraphPanel::_connection_from_empty(const StringName& p_name, int p_port, const Vector2& p_position)
{
    ERR_FAIL_COND_MSG(!p_name.is_valid_int(), "Connection name is expected to be an integer value");

    PinHandle handle;
    handle.node_id = p_name.to_int();
    handle.pin_port = p_port;

    _connect_with_menu(handle, p_position, true);
}

void OrchestratorEditorGraphPanel::_connection_to_empty(const StringName& p_name, int p_port, const Vector2& p_position)
{
    ERR_FAIL_COND_MSG(!p_name.is_valid_int(), "Connection name is expected to be an integer value");

    PinHandle handle;
    handle.node_id = p_name.to_int();
    handle.pin_port = p_port;

    _connect_with_menu(handle, p_position, false);
}

void OrchestratorEditorGraphPanel::_connection_request(const StringName& p_from, int p_from_port, const StringName& p_to, int p_to_port)
{
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

void OrchestratorEditorGraphPanel::_disconnection_request(const StringName& p_from, int p_from_port, const StringName& p_to, int p_to_port)
{
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

void OrchestratorEditorGraphPanel::_popup_request(const Vector2& p_position)
{
    _popup_menu(p_position);
}

void OrchestratorEditorGraphPanel::_node_selected(Node* p_node)
{
}

void OrchestratorEditorGraphPanel::_node_deselected(Node* p_node)
{
    // Clear inspector
    EI->inspect_object(nullptr);
}

void OrchestratorEditorGraphPanel::_delete_nodes_request(const PackedStringArray& p_names)
{
    // In Godot 4.2, there is a case where this method can be called with no values
    if (p_names.is_empty())
        return;

    HashSet<OrchestratorEditorGraphNode*> node_set;
    HashSet<GraphElement*> knot_set;
    for (const String& name : p_names)
    {
        GraphElement* element = cast_to<GraphElement>(find_child(name, false, false));
        if (!element)
            continue;

        if (_knot_editor->is_knot(element))
            knot_set.insert(element);

        if (OrchestratorEditorGraphNode* node = cast_to<OrchestratorEditorGraphNode>(element))
            node_set.insert(node);
    }

    const uint32_t knot_count = knot_set.size();
    const uint32_t node_count = node_set.size();

    const TypedArray<OrchestratorEditorGraphNode> node_array = GodotUtils::set_to_typed_array(node_set);
    const TypedArray<GraphElement> knot_array = GodotUtils::set_to_typed_array(knot_set);

    if (knot_count > 0 && node_count > 0)
    {
        const String message = vformat("Do you want to delete %d node(s) and %d knot(s)?", node_count, knot_count);
        OrchestratorEditorDialogs::confirm(message, callable_mp_lambda(this, [knot_array, node_array, this] {
            _knot_editor->remove_knots(knot_array);
            remove_nodes(node_array, false);
        }));
    }
    else if (knot_count > 0)
    {
        const String message = vformat("Do you want to delete %d knot(s)?", knot_count);
        OrchestratorEditorDialogs::confirm(message, callable_mp_lambda(this, [knot_array, this] {
            _knot_editor->remove_knots(knot_array);
        }));
    }
    else if (node_count > 0)
    {
        // No need to display any confirmation here, the call will handle that just for nodes.
        remove_nodes(node_array);
    }
}

void OrchestratorEditorGraphPanel::_connection_drag_started(const StringName& p_from, int p_port, bool p_output)
{
    ERR_FAIL_COND_MSG(!p_from.is_valid_int(), "Drag from node name is expected to be an integer value");

    PinHandle handle;
    handle.node_id = p_from.to_int();
    handle.pin_port = p_port;

    OrchestratorEditorGraphPin* pin = _resolve_pin_from_handle(handle, !p_output);
    ERR_FAIL_NULL_MSG(pin, "Failed to resolve drag from pin");

    _drag_from_pin = pin;

    if (p_output && _disconnect_control_flow_when_dragged && _drag_from_pin->is_execution())
    {
        if (_drag_from_pin->is_linked())
            unlink_all(_drag_from_pin);
    }

    emit_signal("connection_pin_drag_started", _drag_from_pin.get());
}

void OrchestratorEditorGraphPanel::_connection_drag_ended()
{
    emit_signal("connection_pin_drag_ended");
}

void OrchestratorEditorGraphPanel::_copy_nodes_request()
{
    _clear_copy_buffer();

    Vector2 selection_center;
    HashSet<int> node_ids;

    const Vector<OrchestratorEditorGraphNode*> selected_nodes = get_selected<OrchestratorEditorGraphNode>();
    if (!selected_nodes.is_empty() && !_can_duplicate_nodes(selected_nodes))
        return;

    for (OrchestratorEditorGraphNode* node : selected_nodes)
    {
        const int node_id = node->get_id();
        const Ref<OrchestrationGraphNode> script_node = _graph->get_orchestration()->get_node(node_id);

        const Vector2 position = node->get_position_offset();
        selection_center += position;

        CopyItem item;
        item.id = node_id;
        item.node = _graph->copy_node(node_id, true);
        item.position = position;
        item.size = node->get_size();

        node_ids.insert(node_id);
        _copy_buffer.nodes.push_back(item);

        const Ref<OScriptNodeCallScriptFunction> call_script_func_node = script_node;
        if (call_script_func_node.is_valid())
            _copy_buffer.function_names.insert(call_script_func_node->get_function()->get_function_name());

        const Ref<OScriptNodeVariable> variable_node = script_node;
        if (variable_node.is_valid())
            _copy_buffer.variable_names.insert(variable_node->get_variable()->get_variable_name());

        const Ref<OScriptNodeEmitSignal> signal_node = script_node;
        if (signal_node.is_valid())
            _copy_buffer.signal_names.insert(signal_node->get_signal()->get_signal_name());
    }

    for (const Connection& C : _graph->get_orchestration()->get_connections())
    {
        if (node_ids.has(C.from_node) && node_ids.has(C.to_node))
            _copy_buffer.connections.push_back(C.id);
    }

    _copy_buffer.orchestration = _graph->get_orchestration();
}

void OrchestratorEditorGraphPanel::_cut_nodes_request()
{
    _clear_copy_buffer();
    _copy_nodes_request();

    if (_copy_buffer.is_empty())
        return;

    for (const CopyItem& item : _copy_buffer.nodes)
    {
        OrchestratorEditorGraphNode* node = find_node(item.id);
        remove_node(node, false);
    }
}

void OrchestratorEditorGraphPanel::_duplicate_nodes_request()
{
    const Vector<OrchestratorEditorGraphNode*> selected = get_selected<OrchestratorEditorGraphNode>();
    if (selected.is_empty())
        return;

    if (!_can_duplicate_nodes(selected))
        return;

    HashMap<int, int> connection_remap;
    HashSet<int> added_set;

    const Vector2 offset = Vector2(25, 25);
    for (OrchestratorEditorGraphNode* node : selected)
    {
        const Ref<OrchestrationGraphNode> new_node = _graph->duplicate_node(node->get_id(), offset, true);
        ERR_CONTINUE(!new_node.is_valid());

        connection_remap[node->get_id()] = new_node->get_id();
        added_set.insert(new_node->get_id());
    }

    for (const Connection& C : _graph->get_orchestration()->get_connections())
    {
        if (connection_remap.has(C.from_node) && connection_remap.has(C.to_node))
            _graph->link(connection_remap[C.from_node], C.from_port, connection_remap[C.to_node], C.to_port);
    }

    _set_edited(true);
    _refresh_panel_connections_with_model();

    clear_selections();

    for (int node_id : added_set)
        find_node(node_id)->set_selected(true);
}

void OrchestratorEditorGraphPanel::_paste_nodes_request()
{
    // Pass 1 - Verify functions
    for (const StringName& function_name : _copy_buffer.function_names)
    {
        const Ref<OScriptFunction> source_function = _copy_buffer.orchestration->find_function(function_name);
        if (!source_function.is_valid())
        {
            const String message = vformat("Cannot paste because source function '%s' no longer exists", function_name);
            OrchestratorEditorDialogs::error(message, "Clipboard error");
            return;
        }

        const Ref<OScriptFunction> function = _graph->get_orchestration()->find_function(function_name);
        if (!function.is_valid())
        {
            const String message = vformat("Cannot paste because function '%s' does not exist", function_name);
            OrchestratorEditorDialogs::error(message, "Clipboard error");
            return;
        }

        if (!MethodUtils::has_same_signature(source_function->get_method_info(), function->get_method_info()))
        {
            const String message = vformat("Function '%s' exists but with a different definition", function_name);
            OrchestratorEditorDialogs::error(message, "Clipboard error");
            return;
        }
    }

    // Pass 2 - Verify Variables
    for (const StringName& variable_name : _copy_buffer.variable_names)
    {
        const Ref<OScriptVariable> source_variable = _copy_buffer.orchestration->get_variable(variable_name);
        if (!source_variable.is_valid())
        {
            const String message = vformat("Variable '%s' no longer exists in the source orchestration", variable_name);
            OrchestratorEditorDialogs::error(message, "Clipboard error");
            return;
        }

        const Ref<OScriptVariable> variable = _graph->get_orchestration()->get_variable(variable_name);
        if (variable.is_valid() && !PropertyUtils::are_equal(source_variable->get_info(), variable->get_info()))
        {
            const String message = vformat("Variable '%s' exists but with a different definition", variable_name);
            OrchestratorEditorDialogs::error(message, "Clipboard error");
            return;
        }
    }

    // Pass 3 - Verify Signals
    for (const StringName& signal_name : _copy_buffer.signal_names)
    {
        const Ref<OScriptSignal> source_signal = _copy_buffer.orchestration->find_custom_signal(signal_name);
        if (!source_signal.is_valid())
        {
            const String message = vformat("Cannot paste because source signal '%s' no longer exists", signal_name);
            OrchestratorEditorDialogs::error(message, "Clipboard error");
            return;
        }

        const Ref<OScriptSignal> signal = _graph->get_orchestration()->find_custom_signal(signal_name);
        if (!signal.is_valid())
        {
            const String message = vformat("Cannot paste because signal '%s' does not exist", signal_name);
            OrchestratorEditorDialogs::error(message, "Clipboard error");
            return;
        }

        if (!MethodUtils::has_same_signature(source_signal->get_method_info(), signal->get_method_info()))
        {
            const String message = vformat("Signal '%s' exists but with a different definition", signal_name);
            OrchestratorEditorDialogs::error(message, "Clipboard error");
            return;
        }
    }

    // Pass 4 - Create variable references that don't already exist
    for (const StringName& variable_name : _copy_buffer.variable_names)
    {
        const Ref<OScriptVariable> source_variable = _copy_buffer.orchestration->get_variable(variable_name);
        if (source_variable.is_valid())
            continue;

        const Ref<OScriptVariable> variable = _graph->get_orchestration()->create_variable(variable_name);
        ERR_CONTINUE(!variable.is_valid());

        variable->copy_persistent_state(source_variable);
    }

    // Pass 5 - Create signal references that don't already exist
    for (const StringName& signal_name : _copy_buffer.signal_names)
    {
        const Ref<OScriptSignal> source_signal = _copy_buffer.orchestration->find_custom_signal(signal_name);
        if (source_signal.is_valid())
            continue;

        const Ref<OScriptSignal> signal = _graph->get_orchestration()->create_custom_signal(signal_name);
        ERR_CONTINUE(!signal.is_valid());

        signal->copy_persistent_state(source_signal);
    }

    // Pass 6 - Compute paste offset
    Vector2 offset = (get_scroll_offset() + get_local_mouse_position()) / get_zoom();
    #if GODOT_VERSION >= 0x040500
    if (!_copy_buffer.nodes.is_empty())
        offset -= _copy_buffer.nodes.get(0).position;
    #else
    if (!_copy_buffer.nodes.is_empty())
        offset -= _copy_buffer.nodes[0].position;
    #endif

    if (is_snapping_enabled())
        offset = offset.snapped(Vector2(get_snapping_distance(), get_snapping_distance()));

    // Pass 7 - Create the nodes
    HashMap<int, int> connection_remap;
    HashSet<int> added_set;

    for (const CopyItem& item : _copy_buffer.nodes)
    {
        const Ref<OrchestrationGraphNode> node = item.node;

        // Since the source and target function definitions may, the copy needs to refer to the GUID
        // in the target because while the function signatures match, they have different GUIDs.
        const Ref<OScriptNodeCallScriptFunction> call_script_func = node;
        if (call_script_func.is_valid())
        {
            const StringName func_name = call_script_func->get_function()->get_function_name();
            const Ref<OScriptFunction> target_func = _graph->get_orchestration()->find_function(func_name);
            if (target_func.is_valid())
                call_script_func->set("guid", target_func->get_guid().to_string());
        }

        const Ref<OrchestrationGraphNode> new_node = _graph->paste_node(node, item.position + offset);

        connection_remap[item.id] = new_node->get_id();
        added_set.insert(new_node->get_id());
    }

    // Pass 8 - Apply connections between pasted nodes
    for (const uint64_t connection_id : _copy_buffer.connections)
    {
        const Connection C(connection_id);
        _graph->link(connection_remap[C.from_node], C.from_port, connection_remap[C.to_node], C.to_port);
    }

    // Pass 9 - Update the UI
    _refresh_panel_connections_with_model();

    // Pass 10 - Apply selections on the newly pasted nodes
    clear_selections();
    for (int node_id : added_set)
        find_node(node_id)->set_selected(true);

    _set_edited(true);
}

void OrchestratorEditorGraphPanel::_begin_node_move()
{
    _moving_selection = true;
}

void OrchestratorEditorGraphPanel::_end_node_move()
{
    _moving_selection = false;
}

void OrchestratorEditorGraphPanel::_scroll_offset_changed(const Vector2& p_scroll_offset)
{
}

void OrchestratorEditorGraphPanel::_connect_graph_node_pin_signals(OrchestratorEditorGraphNode* p_node)
{
    GUARD_NULL(p_node);

    const Callable context_menu_requested_cb = callable_mp_this(_show_pin_context_menu);
    const Callable default_value_changed_cb = callable_mp_this(_pin_default_value_changed);

    for (OrchestratorEditorGraphPin* pin : p_node->get_pins())
    {
        if (!pin->is_connected("context_menu_requested", context_menu_requested_cb))
            pin->connect("context_menu_requested", context_menu_requested_cb);

        if (!pin->is_connected("default_value_changed", default_value_changed_cb))
            pin->connect("default_value_changed", default_value_changed_cb);
    }
}

void OrchestratorEditorGraphPanel::_disconnect_graph_node_pin_signals(OrchestratorEditorGraphNode* p_node)
{
    GUARD_NULL(p_node);

    const Callable context_menu_requested_cb = callable_mp_this(_show_pin_context_menu);
    const Callable default_value_changed_cb = callable_mp_this(_pin_default_value_changed);

    for (OrchestratorEditorGraphPin* pin : p_node->get_pins())
    {
        if (pin->is_connected("context_menu_requested", context_menu_requested_cb))
            pin->disconnect("context_menu_requested", context_menu_requested_cb);

        if (pin->is_connected("default_value_changed", default_value_changed_cb))
            pin->disconnect("default_value_changed", default_value_changed_cb);
    }
}

void OrchestratorEditorGraphPanel::_double_click_node_jump_request(OrchestratorEditorGraphNode* p_node)
{
    GUARD_NULL(p_node);

    if (p_node->can_jump_to_definition())
    {
        Object* definition_object = p_node->get_definition_object();
        if (definition_object)
        {
            emit_signal("focus_requested", definition_object);
            accept_event();
        }
    }
}

void OrchestratorEditorGraphPanel::_show_node_context_menu(OrchestratorEditorGraphNode* p_node, const Vector2& p_position)
{
    ERR_FAIL_NULL_MSG(p_node, "Cannot create context menu for an invalid pin.");
    accept_event();

    p_node->set_selected(true);

    const bool are_multiple_selections = get_selection_count() > 1;

    OrchestratorEditorContextMenu* menu = memnew(OrchestratorEditorContextMenu);
    menu->set_auto_destroy(true);
    add_child(menu);


    const Ref<OrchestrationGraphNode> script_node = p_node->_node;

    menu->add_separator("Node Actions");

    List<Ref<OScriptAction>> script_node_actions;
    script_node->get_actions(script_node_actions);
    for (const Ref<OScriptAction>& node_action : script_node_actions)
    {
        if (node_action->get_icon().is_empty())
            menu->add_item(node_action->get_text(), node_action->get_handler());
        else
            menu->add_icon_item(node_action->get_icon(), node_action->get_text(), node_action->get_handler());
    }

    const bool can_delete = p_node->can_user_delete_node();
    menu->add_icon_item("Remove", "Delete", callable_mp_this(remove_node).bind(p_node, true ), !can_delete, KEY_DELETE);

    menu->add_icon_item("ActionCut", "Cut", callable_mp_this(_cut_nodes_request), false, OACCEL_KEY(KEY_MASK_CTRL, KEY_X));
    menu->add_icon_item("ActionCopy", "Copy", callable_mp_this(_copy_nodes_request), false, OACCEL_KEY(KEY_MASK_CTRL, KEY_C));
    menu->add_icon_item("Duplicate", "Duplicate", callable_mp_this(_duplicate_nodes_request), false, OACCEL_KEY(KEY_MASK_CTRL, KEY_D));
    menu->add_icon_item("DistractionFree", "Toggle Resizer", callable_mp_this(_toggle_resizer_for_selected_nodes));
    menu->add_icon_item("KeepAspect", "Resize to Content", callable_mp_this(_resize_node_to_content));

    bool has_connections = !get_connected_nodes(p_node).is_empty();
    menu->add_icon_item("Loop", "Refresh Nodes", callable_mp_this(_refresh_selected_nodes));
    menu->add_icon_item("Unlinked", "Break Node Link(s)", callable_mp_this(unlink_node_all).bind(p_node), !has_connections);

    if (!are_multiple_selections)
        menu->add_icon_item("Anchor", "Toggle Bookmark", callable_mp_this(_toggle_node_bookmark).bind(p_node));

    if (p_node->is_add_pin_button_visible() && !are_multiple_selections)
        menu->add_item("Add Option Pin", callable_mp_this(_add_node_pin).bind(p_node));

    menu->add_separator("Organization");

    const bool can_expand = cast_to<OScriptNodeCallScriptFunction>(script_node.ptr()) != nullptr;
    menu->add_item("Expand Node", callable_mp_this(_expand_node).bind(p_node), !can_expand);
    menu->add_item("Collapse to Function", callable_mp_this(_collapse_selected_nodes_to_function));

    OrchestratorEditorContextMenu* align = menu->add_submenu("Alignment");
    align->add_icon_item("ControlAlignTopWide", "Align Top", callable_mp_this(_align_nodes).bind(p_node, ALIGN_TOP));
    align->add_icon_item("ControlAlignHCenterWide", "Align Middle", callable_mp_this(_align_nodes).bind(p_node, ALIGN_MIDDLE));
    align->add_icon_item("ControlAlignBottomWide", "Align Bottom", callable_mp_this(_align_nodes).bind(p_node, ALIGN_BOTTOM));
    align->add_icon_item("ControlAlignLeftWide", "Align Left", callable_mp_this(_align_nodes).bind(p_node, ALIGN_LEFT));
    align->add_icon_item("ControlAlignVCenterWide", "Align Center", callable_mp_this(_align_nodes).bind(p_node, ALIGN_CENTER));
    align->add_icon_item("ControlAlignRightWide", "Align Right", callable_mp_this(_align_nodes).bind(p_node, ALIGN_RIGHT));

    if (!are_multiple_selections && _has_breakpoint_support())
    {
        menu->add_separator("Breakpoints");
        menu->add_item("Toggle Breakpoint", callable_mp_this(_toggle_node_breakpoint).bind(p_node), false, KEY_F9);

        const bool has_breakpoints = _breakpoints.has(script_node->get_id());
        const bool has_active_breakpoint = has_breakpoints && _breakpoint_state[script_node->get_id()];

        menu->add_item(
            vformat("%s breakpoint", has_breakpoints ? "Remove" : "Add"),
            callable_mp_this(_set_node_breakpoint).bind(p_node, !has_breakpoints));

        if (has_breakpoints)
        {
            const String label = has_active_breakpoint ? "Disable breakpoint" : "Enable breakpoint";
            menu->add_item(label,
                callable_mp_this(_set_node_breakpoint_enabled).bind(p_node, !has_active_breakpoint));
        }
    }

    menu->add_separator("Documentation");

    #if GODOT_VERSION >= 0x040300
    const String view_doc_topic = script_node->get_help_topic();
    #else
    const String view_doc_topic = script_node->get_class();
    #endif
    menu->add_icon_item("Help", "View Documentation", callable_mp_this(_view_documentation).bind(view_doc_topic));

    const Ref<OScriptNodeVariableGet> variable_get = script_node;
    if (variable_get.is_valid() && variable_get->can_be_validated())
    {
        menu->add_separator("Variable Get");

        const String label = variable_get->is_validated() ? "Make Pure" : "Make Validated";
        menu->add_item(label, callable_mp_this(_set_variable_node_validation)
            .bind(p_node, !variable_get->is_validated()));
    }

    menu->set_position(p_node->get_screen_position() + p_position * get_zoom());
    menu->popup();
}

void OrchestratorEditorGraphPanel::_node_position_changed(const Vector2& p_old_position, const Vector2& p_new_position, OrchestratorEditorGraphNode* p_node)
{
    ERR_FAIL_NULL_MSG(p_node, "Cannot update node position with an invalid node reference");
    if (p_node->_node->get_position() != p_new_position)
    {
        p_node->_node->set_position(p_new_position);
        p_node->set_position_offset(p_new_position);
        _set_edited(true);
    }
}

void OrchestratorEditorGraphPanel::_node_resized(OrchestratorEditorGraphNode* p_node)
{
    ERR_FAIL_NULL_MSG(p_node, "Cannot update node position with an invalid node reference");
    _node_resize_end(p_node->get_position(), p_node);
}

void OrchestratorEditorGraphPanel::_node_resize_end(const Vector2& p_size, OrchestratorEditorGraphNode* p_node)
{
    ERR_FAIL_NULL_MSG(p_node, "Cannot update node position with an invalid node reference");
    if (p_node->_node->get_size() != p_size)
    { 
        p_node->_node->set_size(p_size);
        _set_edited(true);
    }
}

void OrchestratorEditorGraphPanel::_show_pin_context_menu(OrchestratorEditorGraphPin* p_pin, const Vector2& p_position)
{
    ERR_FAIL_NULL_MSG(p_pin, "Cannot create context menu for an invalid pin.");
    accept_event();

    // Pin context-menu only operates on the current pin's node, so deselect any existing selections
    for (GraphElement* element : get_selected<GraphElement>())
        element->set_selected(false);

    OrchestratorEditorGraphNode* owning_node = p_pin->get_graph_node();
    owning_node->set_selected(true);

    OrchestratorEditorContextMenu* menu = memnew(OrchestratorEditorContextMenu);
    menu->set_auto_destroy(true);
    add_child(menu);

    menu->add_separator("Pin Actions");

    const HashSet<OrchestratorEditorGraphPin*> pin_connections = get_connected_pins(p_pin);

    if (p_pin->is_linked() && p_pin->is_execution())
    {
        const String label = vformat("Select All %s Nodes", p_pin->get_direction() == PD_Input ? "Input" : "Output");
        menu->add_item(label, callable_mp_this(_select_connected_execution_pins).bind(p_pin));
    }

    const Ref<OrchestrationGraphNode> script_node = owning_node->_node;
    const Ref<OrchestrationGraphPin> script_pin = p_pin->_pin;

    const Ref<OScriptEditablePinNode>& editable_node = script_node;
    if (editable_node.is_valid() && editable_node->can_remove_dynamic_pin(script_pin))
    {
        const Ref<OScriptNodeMakeDictionary> make_dict = script_node;
        const String label = make_dict.is_valid() ? "Remove key/value pair" : "Remove pin";
        menu->add_item(label, callable_mp_this(_remove_node_pin).bind(p_pin));
    }

    if (script_node->can_change_pin_type())
    {
        const Vector<Variant::Type> options = script_node->get_possible_pin_types();
        if (!options.is_empty())
        {
            OrchestratorEditorContextMenu* submenu = menu->add_submenu("Change Pin Type");
            for (Variant::Type option : options)
            {
                const String label = VariantUtils::get_friendly_type_name(option, true).capitalize();
                submenu->add_item(label, callable_mp_this(_change_node_pin_type).bind(p_pin, option));
            }
        }
    }

    if (pin_connections.size() > 1)
    {
        menu->add_icon_item("Unlinked", "Break All Pin Links", callable_mp_this(unlink_all).bind(p_pin, true));

        OrchestratorEditorContextMenu* submenu = menu->add_submenu("Break Link To...");
        for (OrchestratorEditorGraphPin* connection : pin_connections)
        {
            const String node_name = connection->get_graph_node()->get_title();
            const String pin_name = connection->get_pin_name().capitalize();

            const String label = vformat("Break Pin Link to %s - %s", node_name, pin_name);
            submenu->add_item(label, callable_mp_this(unlink).bind(p_pin, connection));
        }
    }
    else
    {
        Callable callback;
        if (pin_connections.size() == 1)
        {
            OrchestratorEditorGraphPin* link = *pin_connections.begin();
            callback = callable_mp_this(unlink).bind(p_pin, link);
        }

        menu->add_icon_item("Unlinked", "Break This Link", callback, pin_connections.is_empty());
    }

    if (!pin_connections.is_empty())
    {
        OrchestratorEditorContextMenu* submenu = menu->add_submenu("Jump to connected node...");
        for (OrchestratorEditorGraphPin* connection : pin_connections)
        {
            const int node_id = connection->get_graph_node()->get_id();
            const String node_name = connection->get_graph_node()->get_title();

            const String label = vformat("Jump to %d - %s", node_id, node_name);
            submenu->add_item(label, callable_mp_this(center_node).bind(connection->get_graph_node()));
        }
    }

    if (_can_promote_pin_to_variable(p_pin))
        menu->add_item("Promote to Variable", callable_mp_this(_promote_pin_to_variable).bind(p_pin));

    if (!p_pin->is_execution() && pin_connections.is_empty() && p_pin->is_connectable() && p_pin->get_direction() == PD_Input)
        menu->add_item("Reset to Default Value", callable_mp_this(_reset_pin_to_generated_default_value).bind(p_pin));

    menu->add_separator("Documentation");

    #if GODOT_VERSION >= 0x040300
    const String view_doc_topic = script_node->get_help_topic();
    #else
    const String view_doc_topic = script_node->get_class();
    #endif
    menu->add_icon_item("Help", "View Documentation", callable_mp_this(_view_documentation).bind(view_doc_topic));

    menu->set_position(p_pin->get_screen_position() + p_position * get_zoom());
    menu->popup();
}

void OrchestratorEditorGraphPanel::_pin_default_value_changed(OrchestratorEditorGraphPin* p_pin, const Variant& p_value)
{
    ERR_FAIL_NULL_MSG(p_pin, "Cannot update pin default value with an invalid pin reference");

    const Variant old_value = p_pin->_pin->get_effective_default_value();
    if (old_value != p_value)
    {
        p_pin->_pin->set_default_value(p_value);
        _set_edited(true);
    }
}

void OrchestratorEditorGraphPanel::_node_added(int p_node_id)
{
    _refresh_panel_with_model();

    if (_center_status->is_visible())
        _center_status->hide();
}

void OrchestratorEditorGraphPanel::_node_removed(int p_node_id)
{
    _refresh_panel_with_model();

    if (_graph->get_nodes().is_empty() && !_center_status->is_visible())
        _center_status->show();
}

void OrchestratorEditorGraphPanel::_graph_changed()
{
    // Graph was renamed
    if (_graph.is_valid() && _graph->get_graph_name() != get_name())
        set_name(_graph->get_graph_name());
}

void OrchestratorEditorGraphPanel::_knots_changed()
{
    _knot_editor->flush_knot_cache(_graph);
    _set_edited(true);
}

void OrchestratorEditorGraphPanel::_clear_copy_buffer()
{
    _copy_buffer.nodes.clear();
    _copy_buffer.connections.clear();
    _copy_buffer.orchestration = nullptr;
    _copy_buffer.variable_names.clear();
    _copy_buffer.function_names.clear();
    _copy_buffer.signal_names.clear();
}

void OrchestratorEditorGraphPanel::_toggle_resizer_for_selected_nodes()
{
    for (OrchestratorEditorGraphNode* node : get_selected<OrchestratorEditorGraphNode>())
        node->set_resizable(!node->is_resizable());
}

void OrchestratorEditorGraphPanel::_resize_node_to_content()
{
    for (OrchestratorEditorGraphNode* node : get_selected<OrchestratorEditorGraphNode>())
    {
        node->_resize_to_content();
        _set_edited(true);
    }
}

void OrchestratorEditorGraphPanel::_refresh_selected_nodes()
{
    for (OrchestratorEditorGraphNode* node : get_selected<OrchestratorEditorGraphNode>())
        node->_node->reconstruct_node();
}

void OrchestratorEditorGraphPanel::_add_node_pin(OrchestratorEditorGraphNode* p_node)
{
    ERR_FAIL_NULL_MSG(p_node, "Cannot add node pin to an invalid node reference");

    const Ref<OScriptEditablePinNode> editable_node = p_node->_node;
    if (editable_node.is_valid() && editable_node->can_add_dynamic_pin())
    {
        editable_node->add_dynamic_pin();
        _set_edited(true);
    }
}

void OrchestratorEditorGraphPanel::_expand_node(OrchestratorEditorGraphNode* p_node)
{
    const Ref<OScriptNodeCallScriptFunction> call_script_function = p_node->_node;
    if (!call_script_function.is_valid())
    {
        const String message = vformat("Node '%s' is not a call script function node and can't be expanded", p_node->get_title());
        OrchestratorEditorDialogs::error(message);
        return;
    }

    const Ref<OScriptFunction> function = call_script_function->get_function();
    if (!function.is_valid())
    {
        const String message = vformat("Function the node references cannot be found.");
        OrchestratorEditorDialogs::error(message);
        return;
    }

    Rect2 nodes_area;
    HashSet<int> nodes_to_duplicate;
    const Ref<OrchestrationGraph> function_graph = function->get_function_graph();
    for (const Ref<OrchestrationGraphNode>& node : function_graph->get_nodes())
    {
        const Ref<OScriptNodeFunctionEntry> entry = node;
        const Ref<OScriptNodeFunctionResult> result = node;

        if (!entry.is_valid() && !result.is_valid() && node->can_duplicate())
        {
            const Rect2 node_rect = Rect2(node->get_position(), node->get_size());
            if (nodes_to_duplicate.is_empty())
                nodes_area = node_rect;
            else
                nodes_area = nodes_area.merge(node_rect);

            nodes_to_duplicate.insert(node->get_id());
        }
    }

    if (!nodes_to_duplicate.is_empty())
    {
        const Vector2 position_delta = p_node->get_graph_rect().get_center() - nodes_area.get_center();

        HashMap<int, int> connection_remap;
        for (const int node_id : nodes_to_duplicate)
        {
            const Ref<OrchestrationGraphNode> new_node = _graph->duplicate_node(node_id, position_delta, true);
            ERR_CONTINUE(!new_node.is_valid());

            connection_remap[node_id] = new_node->get_id();
        }

        for (const Connection& C : _graph->get_orchestration()->get_connections())
        {
            if (connection_remap.has(C.from_node) && connection_remap.has(C.to_node))
                _graph->link(connection_remap[C.from_node], C.from_port, connection_remap[C.to_node], C.to_port);
        }
    }

    remove_node(p_node, false);
    _set_edited(true);
}

void OrchestratorEditorGraphPanel::_collapse_selected_nodes_to_function()
{
    const Vector<OrchestratorEditorGraphNode*> selected_nodes = get_selected<OrchestratorEditorGraphNode>();
    if (selected_nodes.is_empty())
        return;

    if (!_can_duplicate_nodes(selected_nodes))
        return;

    int input_executions = 0;
    int output_executions = 0;
    int input_data = 0;
    int output_data = 0;
    HashSet<int> node_set;
    for (OrchestratorEditorGraphNode* node : selected_nodes)
    {
        node_set.insert(node->get_id());

        const Vector<OrchestratorEditorGraphPin*> node_pins = node->get_pins();
        for (OrchestratorEditorGraphPin* pin : node_pins)
        {
            const HashSet<OrchestratorEditorGraphPin*> connected_pins = get_connected_pins(pin);
            for (OrchestratorEditorGraphPin* connected_pin : connected_pins)
            {
                if (!selected_nodes.has(connected_pin->get_graph_node()))
                {
                    if (pin->get_direction() == PD_Input && pin->is_execution())
                        input_executions++;
                    else if (pin->get_direction() == PD_Input)
                        input_data++;
                    else if (pin->get_direction() == PD_Output && pin->is_execution())
                        output_executions++;
                    else  if (pin->get_direction() == PD_Output)
                        output_data++;
                }
            }
        }
    }

    HashSet<uint64_t> connections;
    HashSet<uint64_t> input_connections;
    HashSet<uint64_t> output_connections;
    for (const Connection& C : _graph->get_orchestration()->get_connections())
    {
        if (node_set.has(C.from_node) && node_set.has(C.to_node))
            connections.insert(C.id);

        if (!node_set.has(C.from_node) && node_set.has(C.to_node))
            input_connections.insert(C.id);

        if (node_set.has(C.from_node) && !node_set.has(C.to_node))
            output_connections.insert(C.id);
    }

    ERR_FAIL_COND_EDMSG(input_executions > 1, "Cannot collapse with more than one external input execution wire.");
    ERR_FAIL_COND_EDMSG(output_executions > 1, "Cannot collapse with more than one external output execution wire.");
    ERR_FAIL_COND_EDMSG(output_data > 1, "Cannot collapse to function with more than one output data wire.");
    ERR_FAIL_COND_EDMSG(output_connections.size() > 2, "Cannot output more than one execution and one data pin.");

    const StringName function_name = NameUtils::create_unique_name("NewFunction", _graph->get_orchestration()->get_function_names());
    if (!_create_new_function(function_name, !output_connections.is_empty()))
    {
        const String message = "Failed to create new function for collapse";
        OrchestratorEditorDialogs::error(message);
        return;
    }

    const Ref<OScriptFunction> function = _graph->get_orchestration()->find_function(function_name);

    const Ref<OrchestrationGraph> source_graph = _graph;
    const Ref<OrchestrationGraph> target_graph = function->get_function_graph();

    const Rect2 selected_node_area = get_bounds_for_nodes(selected_nodes);

    // Before moving the nodes, their connections to non-collapsed nodes must be severed
    for (uint64_t connection_id : input_connections)
    {
        const Connection C(connection_id);
        source_graph->unlink(C.from_node, C.from_port, C.to_node, C.to_port);
    }
    for (uint64_t connection_id : output_connections)
    {
        const Connection C(connection_id);
        source_graph->unlink(C.from_node, C.from_port, C.to_node, C.to_port);
    }

    // Transfer the nodes between the graphs
    for (OrchestratorEditorGraphNode* node : selected_nodes)
        source_graph->move_node_to(node->_node, target_graph);

    // Spawn the call functino node in the source graph
    NodeSpawnOptions options;
    options.node_class = OScriptNodeCallScriptFunction::get_class_static();
    options.context.method = function->get_method_info();
    options.position = selected_node_area.get_center();

    OrchestratorEditorGraphNode* call_function = spawn_node(options);

    int call_input_index = 1;
    int input_index = 1;
    bool input_execution_wired = false;
    bool call_execution_wired = false;
    bool entry_positioned = false;
    for (uint64_t connection_id : input_connections)
    {
        const Connection C(connection_id);

        const Ref<OrchestrationGraphNode> source = _graph->get_orchestration()->get_node(C.from_node);
        const Ref<OrchestrationGraphPin> source_pin = source->find_pins(PD_Output)[C.from_port];
        if (source_pin->is_execution() && !call_execution_wired)
        {
            source_graph->link(C.from_node, C.from_port, call_function->get_id(), 0);
            call_execution_wired = true;
        }
        else
            source_graph->link(C.from_node, C.from_port, call_function->get_id(), call_input_index++);

        const Ref<OrchestrationGraphNode> target = _graph->get_orchestration()->get_node(C.to_node);
        const Ref<OrchestrationGraphPin> target_pin = target->find_pins(PD_Input)[C.to_port];

        if (!entry_positioned)
        {
            const Ref<OrchestrationGraphNode> entry = _graph->get_orchestration()->get_node(function->get_owning_node_id());
            entry->set_position(target->get_position() - Vector2(250, 0));
            entry->emit_changed();
            entry_positioned = true;
        }

        if (!target_pin->is_execution())
        {
            const size_t size = function->get_argument_count() + 1;
            function->resize_argument_list(size);

            PropertyInfo property = target_pin->get_property_info();
            if (!target_pin->get_label().is_empty() && property.name != target_pin->get_label())
                property.name = target_pin->get_label();

            PackedStringArray names;
            for (const PropertyInfo& argument : function->get_method_info().arguments)
            {
                if (!names.has(argument.name))
                    names.push_back(argument.name);
            }

            if (names.has(property.name))
                property.name = NameUtils::create_unique_name(property.name, names);

            function->set_argument(size - 1, property);

            // Wire entry data output to this connection
            target_graph->link(function->get_owning_node_id(), input_index++, C.to_node, C.to_port);
        }
        else if (!input_execution_wired)
        {
            // Wire entry execution output to this connection
            target_graph->link(function->get_owning_node_id(), 0, C.to_node, C.to_port);
            input_execution_wired = true;
        }
    }

    const Ref<OrchestrationGraphNode> result = function->get_return_node();
    if (result.is_valid())
    {
        bool output_execution_wired = false;
        bool output_data_wired = false;
        bool positioned = false;

        for (uint64_t connection_id : output_connections)
        {
            const Connection C(connection_id);

            const Ref<OrchestrationGraphNode> source = _graph->get_orchestration()->get_node(C.from_node);
            const Ref<OrchestrationGraphPin> source_pin = source->find_pins(PD_Output)[C.from_port];

            if (!positioned)
            {
                result->set_position(source->get_position() + Vector2(250, 0));
                result->emit_changed();
                positioned = true;
            }

            if (source_pin->is_execution() && !output_execution_wired)
            {
                // Connect execution
                target_graph->link(C.from_node, C.from_port, result->get_id(), 0);
                output_execution_wired = true;
            }
            else if (!source_pin->is_execution() && !output_data_wired)
            {
                // Connect data
                function->set_has_return_value(true);
                function->set_return_type(source_pin->get_type());

                target_graph->link(C.from_node, C.from_port, result->get_id(), 1);
                output_data_wired = true;
            }
        }

        const Ref<OrchestrationGraphPin> result_exec = result->find_pin(0, PD_Output);
        if (result_exec.is_valid() && !result_exec->has_any_connections())
        {
            const Ref<OrchestrationGraphNode> entry = function->get_owning_node();
            const Ref<OrchestrationGraphPin> entry_exec = entry->find_pin(0, PD_Output);
            if (entry_exec.is_valid() && !entry_exec->has_any_connections())
            {
                entry_exec->link(result_exec);
                if (entry->find_pins(PD_Output).size() == 1)
                {
                    entry->set_position(result->get_position() - Vector2(250, 0));
                    entry->emit_changed();
                }
            }
        }
    }

    // Finally wire up the call node in the main graph
    int call_output_index = 1;
    call_execution_wired = false;
    for (uint64_t connection_id : output_connections)
    {
        const Connection C(connection_id);

        // Get the exterior node connected to the selected node
        const Ref<OrchestrationGraphNode> target = _graph->get_orchestration()->get_node(C.to_node);
        const Ref<OrchestrationGraphPin> target_pin = target->find_pins(PD_Input)[C.to_port];
        if (target_pin->is_execution() && !call_execution_wired)
        {
            source_graph->link(call_function->get_id(), 0, C.to_node, C.to_port);
            call_execution_wired = true;
        }
        else if (!target_pin->is_execution())
        {
            source_graph->link(call_function->get_id(), call_output_index++, C.to_node, C.to_port);
        }
    }

    call_function->_node->emit_changed();
    _set_edited(true);

    _refresh_panel_connections_with_model();

    emit_signal("nodes_changed");
    call_deferred("emit_signal", "edit_function_requested", function->get_function_name());
}

bool OrchestratorEditorGraphPanel::_create_new_function(const String& p_name, bool p_has_return)
{
    ERR_FAIL_COND_V_MSG(_graph->get_orchestration()->has_function(p_name), false, "A function already exists with that name");

    const int flags = OrchestrationGraph::GF_FUNCTION | OrchestrationGraph::GF_DEFAULT;
    const Ref<OrchestrationGraph> function_graph = _graph->get_orchestration()->create_graph(p_name, flags);
    ERR_FAIL_COND_V_MSG(!function_graph.is_valid(), false, "Failed to create function graph");

    MethodInfo mi;
    mi.name = p_name;
    mi.flags = METHOD_FLAG_NORMAL;
    mi.return_val.type = Variant::NIL;
    mi.return_val.hint = PROPERTY_HINT_NONE;
    mi.return_val.usage = PROPERTY_USAGE_DEFAULT;

    NodeSpawnOptions options;
    options.node_class = OScriptNodeFunctionEntry::get_class_static();
    options.context.method = mi;

    const Ref<OrchestrationGraphNode> entry = function_graph->create_node<OScriptNodeFunctionEntry>(options.context);
    if (!entry.is_valid())
    {
        _graph->get_orchestration()->remove_graph(function_graph->get_graph_name());
        ERR_FAIL_V_MSG(false, "Failed to create function entry node in the function graph");
    }

    _set_edited(true);

    if (!p_has_return)
        return true;

    const Vector2 position = entry->get_position() + Vector2(300, 0);
    if (!function_graph->create_node<OScriptNodeFunctionResult>(options.context, position).is_valid())
    {
        ERR_FAIL_V_MSG(false, "Failed to create function result node in the function graph, please create it manually.");
    }

    return true;
}

void OrchestratorEditorGraphPanel::_align_nodes(OrchestratorEditorGraphNode* p_anchor, int p_alignment)
{
    ERR_FAIL_NULL_MSG(p_anchor, "Cannot perform node alignment with an invalid anchor node reference");
    ERR_FAIL_INDEX(p_alignment, GraphNodeAlignment::ALIGN_MAX);

    #define SET_NODE_POS(node_obj, position)                            \
        node_obj->set_position_offset(position);                        \
        node_obj->_node->set_position(node_obj->get_position_offset());

    const Vector2 align_offset = p_anchor->get_position_offset();
    const Vector2 align_size = p_anchor->get_size();

    switch (p_alignment)
    {
        case ALIGN_TOP:
        {
            // Align all selected nodes to match top of this specific node.
            const float top = align_offset.y;
            for_each<OrchestratorEditorGraphNode>([&] (OrchestratorEditorGraphNode* node) {
                const float adjust = top - node->get_position_offset().y;
                SET_NODE_POS(node, node->get_position_offset() + Vector2(0, adjust));
            }, true);
            _set_edited(true);
            break;
        }
        case ALIGN_MIDDLE:
        {
            // Align all selected nodes to center to this specific node.
            const float mid_y = align_offset.y + (align_size.y / 2);
            for_each<OrchestratorEditorGraphNode>([&] (OrchestratorEditorGraphNode* node) {
                const float node_mid_y = node->get_position_offset().y + (node->get_size().y / 2);
                SET_NODE_POS(node, node->get_position_offset() + Vector2(0, mid_y - node_mid_y));
            }, true);
            _set_edited(true);
            break;
        }
        case ALIGN_BOTTOM:
        {
            // Align all selected nodes to match bottom of this specific node.
            const float bottom = align_offset.y + align_size.y;
            for_each<OrchestratorEditorGraphNode>([&] (OrchestratorEditorGraphNode* node) {
                const float adjust = bottom - (node->get_position_offset().y + node->get_size().y);
                SET_NODE_POS(node, node->get_position_offset() + Vector2(0, adjust));
            }, true);
            _set_edited(true);
            break;
        }
        case ALIGN_LEFT:
        {
            // Align all selected nodes to this specific node.
            const Vector2 pos = align_offset;
            for_each<OrchestratorEditorGraphNode>([&] (OrchestratorEditorGraphNode* node) {
                const float left = node->get_position_offset().x;
                SET_NODE_POS(node, node->get_position_offset() + Vector2(pos.x - left, 0));
            }, true);
            _set_edited(true);
            break;
        }
        case ALIGN_CENTER:
        {
            // Align all selected nodes to center to this specific node.
            const float mid_x = align_offset.x + (align_size.x / 2);
            for_each<OrchestratorEditorGraphNode>([&] (OrchestratorEditorGraphNode* node) {
                const float node_mid_x = node->get_position_offset().x + (node->get_size().x / 2);
                SET_NODE_POS(node, node->get_position_offset() + Vector2(mid_x - node_mid_x, 0));
            }, true);
            _set_edited(true);
            break;
        }
        case ALIGN_RIGHT:
        {
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

void OrchestratorEditorGraphPanel::_toggle_node_bookmark(OrchestratorEditorGraphNode* p_node)
{
    GUARD_NULL(p_node);

    const int id = p_node->get_id();

    const int index = _bookmarks.find(id);
    if (index != -1)
        _bookmarks.remove_at(index);
    else
        _bookmarks.push_back(id);

    p_node->notify_bookmarks_changed();
}

bool OrchestratorEditorGraphPanel::_has_breakpoint_support() const
{
    #if GODOT_VERSION >= 0x040300
    return true;
    #else
    return false;
    #endif
}

void OrchestratorEditorGraphPanel::_toggle_node_breakpoint(OrchestratorEditorGraphNode* p_node)
{
    ERR_FAIL_NULL_MSG(p_node, "Cannot toggle node breakpoint on an invalid node reference");

    #if GODOT_VERSION >= 0x040300
    const int id = p_node->get_id();
    if (!_breakpoint_state.has(id))
    {
        _breakpoint_state[id] = true;
        _breakpoints.push_back(id);
        emit_signal("breakpoint_added", id);
    }
    else
    {
        _breakpoint_state.erase(id);

        if (_breakpoints.has(id))
            _breakpoints.remove_at(_breakpoints.find(id));

        emit_signal("breakpoint_removed", id);
    }

    if (OrchestratorEditorDebuggerPlugin* debugger = OrchestratorEditorDebuggerPlugin::get_singleton())
        debugger->set_breakpoint(_graph->get_orchestration()->as_script()->get_path(), id, _breakpoints.has(id));

    p_node->notify_breakpoints_changed();

    #endif
}

void OrchestratorEditorGraphPanel::_set_node_breakpoint(OrchestratorEditorGraphNode* p_node, bool p_breaks)
{
    ERR_FAIL_NULL_MSG(p_node, "Cannot set node breakpoint on an invalid node reference");

    #if GODOT_VERSION >= 0x040300
    const int id = p_node->get_id();
    if (p_breaks)
    {
        _breakpoint_state[id] = true;

        if (!_breakpoints.has(id))
            _breakpoints.push_back(id);

        emit_signal("breakpoint_added", id);
    }
    else
    {
        _breakpoint_state.erase(id);

        const int index = _breakpoints.find(id);
        if (index != -1)
            _breakpoints.remove_at(index);

        emit_signal("breakpoint_removed", id);
    }

    if (OrchestratorEditorDebuggerPlugin* debugger = OrchestratorEditorDebuggerPlugin::get_singleton())
        debugger->set_breakpoint(_graph->get_orchestration()->as_script()->get_path(), id, p_breaks);

    p_node->notify_breakpoints_changed();
    #endif
}

void OrchestratorEditorGraphPanel::_set_node_breakpoint_enabled(OrchestratorEditorGraphNode* p_node, bool p_enabled)
{
    ERR_FAIL_NULL_MSG(p_node, "Cannot set node breakpoint status on an invalid node reference");

    #if GODOT_VERSION >= 0x040300
    const int id = p_node->get_id();
    _breakpoint_state[id] = p_enabled;
    emit_signal("breakpoint_changed", id, p_enabled);

    if (!_breakpoints.has(id))
        _breakpoints.push_back(id);

    if (OrchestratorEditorDebuggerPlugin* debugger = OrchestratorEditorDebuggerPlugin::get_singleton())
        debugger->set_breakpoint(_graph->get_orchestration()->as_script()->get_path(), id, p_enabled);

    p_node->notify_breakpoints_changed();
    #endif
}

void OrchestratorEditorGraphPanel::_set_variable_node_validation(OrchestratorEditorGraphNode* p_node, bool p_validated)
{
    ERR_FAIL_NULL_MSG(p_node, "Cannot set variable node validation on an invalid node reference");

    // This shrinks the node when validation is toggled
    p_node->set_anchor_and_offset(SIDE_BOTTOM, 0, 0);

    const Ref<OScriptNodeVariableGet> variable_node = p_node->_node;
    if (variable_node.is_valid())
    {
        variable_node->set_validated(p_validated);
        _set_edited(true);
    }
}

void OrchestratorEditorGraphPanel::_select_connected_execution_pins(OrchestratorEditorGraphPin* p_pin)
{
    ERR_FAIL_NULL_MSG(p_pin, "Cannot selected connected execution pins on an invalid pin reference");

    clear_selections();

    Vector<OrchestratorEditorGraphPin*> stack;
    stack.push_back(p_pin);

    HashSet<OrchestratorEditorGraphPin*> visited_pins;
    while (!stack.is_empty())
    {
        OrchestratorEditorGraphPin* current_pin = stack[stack.size() - 1];
        stack.remove_at(stack.size() - 1);

        if (visited_pins.has(current_pin))
            continue;

        visited_pins.insert(current_pin);

        OrchestratorEditorGraphNode* node = current_pin->get_graph_node();
        node->set_selected(true);

        // Push opposite direction connected pins onto the stack
        for (OrchestratorEditorGraphPin* pin : get_connected_pins(current_pin))
        {
            if (!visited_pins.has(pin) && pin->get_direction() != p_pin->get_direction())
                stack.push_back(pin);
        }

        // Walk sibling pins
        for (OrchestratorEditorGraphPin* node_pin : node->get_pins())
        {
            if (node_pin->is_execution() && node_pin->get_direction() == p_pin->get_direction())
                stack.push_back(node_pin);
        }
    }
}

void OrchestratorEditorGraphPanel::_remove_node_pin(OrchestratorEditorGraphPin* p_pin)
{
    ERR_FAIL_NULL_MSG(p_pin, "Cannot remove dynamic pin for an invalid pin reference");

    // This shrinks the node when pins are removed
    p_pin->get_graph_node()->set_anchor_and_offset(SIDE_BOTTOM, 0, 0);

    const Ref<OScriptEditablePinNode> editable = p_pin->get_graph_node()->_node;
    if (editable.is_valid())
    {
        if (editable->can_remove_dynamic_pin(p_pin->_pin))
        {
            editable->remove_dynamic_pin(p_pin->_pin);
            _set_edited(true);
        }
    }
}

void OrchestratorEditorGraphPanel::_change_node_pin_type(OrchestratorEditorGraphPin* p_pin, int p_type)
{
    ERR_FAIL_NULL_MSG(p_pin, "Cannot change pin type for an invalid pin reference");

    const Ref<OrchestrationGraphNode> script_node = p_pin->get_graph_node()->_node;
    if (script_node.is_valid() && script_node->can_change_pin_type())
    {
        script_node->change_pin_types(VariantUtils::to_type(p_type));
        _set_edited(true);
    }

    // This shrinks the node when widget layouts change
    p_pin->get_graph_node()->set_anchor_and_offset(SIDE_BOTTOM, 0, 0);
}

bool OrchestratorEditorGraphPanel::_can_promote_pin_to_variable(OrchestratorEditorGraphPin* p_pin)
{
    ERR_FAIL_NULL_V(p_pin, false);
    return !p_pin->is_execution();
}

void OrchestratorEditorGraphPanel::_promote_pin_to_variable(OrchestratorEditorGraphPin* p_pin)
{
    // todo:
    //  For enum pins, like Switch On Direction, promotion sets the variable type properly but
    //  the default values are not correctly sourced. This is because it gets set with a
    //  classification of "class_enum:ClockDirection" when it should be "enum:ClockDirection".
    //  .
    //  In addition, size_flags_horizontal on promotion sets the classification to "bitfield:"
    //  which means the variable declaration is broken, too. It should have been set to
    //  "class_bitfield:Control.SizeFlags" for the inspector to render properly.

    ERR_FAIL_NULL_MSG(p_pin, "Cannot promote pin to a variable with an invalid pin reference");
    ERR_FAIL_COND_MSG(!_can_promote_pin_to_variable(p_pin), "Pin is not eligible for promotion to variable");

    int index = 0;
    String name = vformat("%s_%d", p_pin->get_pin_name(), index++);
    while (_graph->get_orchestration()->has_variable(name))
        name = vformat("%s_%d", p_pin->get_pin_name(), index++);

    const Ref<OScriptVariable> variable = _graph->get_orchestration()->create_variable(name);
    if (variable.is_valid())
    {
        const bool is_input = p_pin->get_direction() == PD_Input;
        const Vector2 port_offset = p_pin->get_graph_node()->get_port_position_for_pin(p_pin);
        const Vector2 pin_position = p_pin->get_graph_node()->get_position_offset() + port_offset;

        NodeSpawnOptions options;
        options.context.variable_name = variable->get_variable_name();
        options.position = pin_position + Vector2(250, 0) * (is_input ? -1 : 1);

        ClassificationParser parser;
        if (parser.parse(p_pin->get_property_info()))
            variable->set_classification(parser.get_classification());

        variable->set_info(p_pin->get_property_info());
        variable->set_default_value(p_pin->_pin->get_effective_default_value());

        variable->emit_changed();
        variable->notify_property_list_changed();

        _graph->get_orchestration()->mark_dirty();

        if (is_input)
        {
            OrchestratorEditorGraphNode* node = spawn_node<OScriptNodeVariableGet>(options);
            if (node)
                link(node->get_output_pin(0), p_pin);
        }
        else
        {
            OrchestratorEditorGraphNode* node = spawn_node<OScriptNodeVariableSet>(options);
            if (node)
                link(node->get_input_pin(1), p_pin);
        }

        _set_edited(true);
    }
}

void OrchestratorEditorGraphPanel::_reset_pin_to_generated_default_value(OrchestratorEditorGraphPin* p_pin)
{
    ERR_FAIL_NULL_MSG(p_pin, "Cannot reset pin to generated default value with an invalid pin reference");

    p_pin->_pin->set_default_value(p_pin->_pin->get_generated_default_value());
    _set_edited(true);
}

void OrchestratorEditorGraphPanel::_view_documentation(const String& p_topic)
{
    EI->set_main_screen_editor("Script");

    #if GODOT_VERSION >= 0x040300
    EI->get_script_editor()->goto_help(p_topic);
    #else
    EI->get_script_editor()->call("_help_class_open", p_topic);
    #endif
}

void OrchestratorEditorGraphPanel::_connect_graph_node_signals(OrchestratorEditorGraphNode* p_node)
{
    GUARD_NULL(p_node);

    p_node->connect("node_pins_changed", callable_mp_this(_connect_graph_node_pin_signals));
    p_node->connect("context_menu_requested", callable_mp_this(_show_node_context_menu));
    p_node->connect("double_click_jump_request", callable_mp_this(_double_click_node_jump_request));
    p_node->connect("add_node_pin_requested", callable_mp_this(_add_node_pin));
    p_node->connect("dragged", callable_mp_this(_node_position_changed).bind(p_node));

    // Godot 4.3 introduced a new resize_end callback that we will use now to handle triggering the
    // final size of a node. This helps to avoid issues with editor scale changes being problematic
    // by leaving nodes too large after scale up.
    #if GODOT_VERSION < 0x040300
    p_node->connect("resized", callable_mp_this(_node_resized).bind(p_node));
    #else
    p_node->connect("resize_end", callable_mp_this(_node_resize_end).bind(p_node));
    #endif

    _connect_graph_node_pin_signals(p_node);
}

void OrchestratorEditorGraphPanel::_disconnect_graph_node_signals(OrchestratorEditorGraphNode* p_node)
{
    GUARD_NULL(p_node);

    p_node->disconnect("node_pins_changed", callable_mp_this(_connect_graph_node_pin_signals));
    p_node->disconnect("context_menu_requested", callable_mp_this(_show_node_context_menu));
    p_node->disconnect("double_click_jump_request", callable_mp_this(_double_click_node_jump_request));
    p_node->disconnect("add_node_pin_requested", callable_mp_this(_add_node_pin));
    p_node->disconnect("dragged", callable_mp_this(_node_position_changed).bind(p_node));

    // Godot 4.3 introduced a new resize_end callback that we will use now to handle triggering the
    // final size of a node. This helps to avoid issues with editor scale changes being problematic
    // by leaving nodes too large after scale up.
    #if GODOT_VERSION < 0x040300
    p_node->disconnect("resized", callable_mp_this(_node_resized).bind(p_node));
    #else
    p_node->disconnect("resize_end", callable_mp_this(_node_resize_end).bind(p_node));
    #endif

    _disconnect_graph_node_pin_signals(p_node);
}

OrchestratorEditorGraphPin* OrchestratorEditorGraphPanel::_resolve_pin_from_handle(const PinHandle& p_handle, bool p_input)
{
    if (OrchestratorEditorGraphNode* node = find_node(p_handle.node_id))
    {
        const int32_t pin_slot = node->get_port_slot(p_handle.pin_port, p_input ? PD_Input : PD_Output);
        return node->get_pin(pin_slot, p_input ? PD_Input : PD_Output);
    }
    return nullptr;
}

void OrchestratorEditorGraphPanel::_connect_with_menu(const PinHandle& p_handle, const Vector2& p_position, bool p_input)
{
    OrchestratorEditorGraphPin* pin = _resolve_pin_from_handle(p_handle, p_input);
    ERR_FAIL_NULL_MSG(pin, "Failed to resolve pin from context");

    _menu_position = (p_position + get_scroll_offset()) / get_zoom();

    _drag_from_pin = pin;

    // Resolve the drag pin target if one is available
    Object* target = nullptr;
    const Ref<OScriptTargetObject> target_reference = _drag_from_pin->_pin->resolve_target();
    if (target_reference.is_valid() && target_reference->has_target())
        target = target_reference->get_target();

    Ref<OrchestratorEditorActionPortRule> port_rule;
    if (!PropertyUtils::is_variant(_drag_from_pin->get_property_info()))
    {
        port_rule.instantiate();
        port_rule->configure(_drag_from_pin, target);
    }

    Ref<OrchestratorEditorActionGraphTypeRule> graph_type_rule;
    graph_type_rule.instantiate();
    graph_type_rule->set_graph_type(_graph->get_flags().has_flag(OrchestrationGraph::GF_FUNCTION)
        ? OrchestratorEditorActionDefinition::GRAPH_FUNCTION
        : OrchestratorEditorActionDefinition::GRAPH_EVENT);

    GraphEditorFilterContext context;
    context.script = _graph->get_orchestration()->as_script();
    context.port_type = pin->get_property_info();
    context.output = pin->get_direction() == PD_Output;
    context.class_hierarchy = Array::make(_graph->get_orchestration()->get_base_type());

    OrchestratorEditorActionMenu* menu = memnew(OrchestratorEditorActionMenu);
    menu->set_title("Select a graph action");
    menu->set_suffix("graph_editor");
    menu->set_close_on_focus_lost(ORCHESTRATOR_GET("ui/actions_menu/close_on_focus_lost", false));
    menu->set_show_filter_option(false);
    menu->set_start_collapsed(true);
    menu->connect("action_selected", callable_mp_this(_action_menu_selection));
    menu->connect("canceled", callable_mp_this(_action_menu_canceled));

    Ref<OrchestratorEditorActionFilterEngine> filter_engine;
    filter_engine.instantiate();
    filter_engine->add_rule(memnew(OrchestratorEditorActionSearchTextRule));
    filter_engine->add_rule(graph_type_rule);
    if (port_rule.is_valid())
        filter_engine->add_rule(port_rule);

    if (_drag_from_pin->is_execution())
        filter_engine->add_rule(memnew(OrchestratorEditorActionClassHierarchyScopeRule));

    const Ref<Script> source_script = _graph->get_orchestration()->as_script();
    OrchestratorEditorActionRegistry* action_registry = OrchestratorEditorActionRegistry::get_singleton();

    Vector<Ref<OrchestratorEditorActionDefinition>> actions;
    if (target)
        actions = action_registry->get_actions(target);
    else if (target_reference.is_valid() && !target_reference->get_target_class().is_empty())
        actions = action_registry->get_actions(target_reference->get_target_class());

    if (actions.is_empty())
        actions = action_registry->get_actions(source_script);

    menu->popup(p_position + get_screen_position(), actions, filter_engine, context);
}

void OrchestratorEditorGraphPanel::_popup_menu(const Vector2& p_position)
{
    _menu_position = (p_position + get_scroll_offset()) / get_zoom();

    Ref<OrchestratorEditorActionGraphTypeRule> graph_type_rule;
    graph_type_rule.instantiate();
    graph_type_rule->set_graph_type(_graph->get_flags().has_flag(OrchestrationGraph::GF_FUNCTION)
        ? OrchestratorEditorActionDefinition::GRAPH_FUNCTION
        : OrchestratorEditorActionDefinition::GRAPH_EVENT);

    Ref<OrchestratorEditorActionFilterEngine> filter_engine;
    filter_engine.instantiate();
    filter_engine->add_rule(memnew(OrchestratorEditorActionSearchTextRule));
    filter_engine->add_rule(memnew(OrchestratorEditorActionClassHierarchyScopeRule));
    filter_engine->add_rule(graph_type_rule);

    GraphEditorFilterContext context;
    context.script = _graph->get_orchestration()->as_script();
    context.class_hierarchy = Array::make(_graph->get_orchestration()->get_base_type());

    OrchestratorEditorActionMenu* menu = memnew(OrchestratorEditorActionMenu);
    menu->set_title("Select a graph action");
    menu->set_suffix("graph_editor");
    menu->set_close_on_focus_lost(ORCHESTRATOR_GET("ui/actions_menu/close_on_focus_lost", false));
    menu->set_show_filter_option(false);
    menu->set_start_collapsed(true);
    menu->connect("action_selected", callable_mp_this(_action_menu_selection));
    menu->connect("canceled", callable_mp_this(_action_menu_canceled));

    menu->popup(
        p_position + get_screen_position(),
        OrchestratorEditorActionRegistry::get_singleton()->get_actions(_graph->get_orchestration()->as_script()),
        filter_engine,
        context);
}

void OrchestratorEditorGraphPanel::_action_menu_selection(const Ref<OrchestratorEditorActionDefinition>& p_action)
{
    ERR_FAIL_COND_MSG(!p_action.is_valid(), "Cannot execute the action, it is invaild.");

    const Vector2 spawn_position = _menu_position;

    switch (p_action->type)
    {
        case OrchestratorEditorActionDefinition::ACTION_SPAWN_NODE:
        {
            ERR_FAIL_COND_MSG(!p_action->node_class.has_value(), "Spawn action node has no node class type");

            NodeSpawnOptions options;
            options.node_class = p_action->node_class.value();
            options.context.user_data = p_action->data;
            options.position = spawn_position;
            options.drag_pin = _drag_from_pin;

            spawn_node(options);
            break;
        }
        case OrchestratorEditorActionDefinition::ACTION_GET_PROPERTY:
        {
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
        case OrchestratorEditorActionDefinition::ACTION_SET_PROPERTY:
        {
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
        case OrchestratorEditorActionDefinition::ACTION_CALL_MEMBER_FUNCTION:
        {
            ERR_FAIL_COND_MSG(!p_action->method.has_value(), "Call member function has no method");

            NodeSpawnOptions options;
            options.node_class = OScriptNodeCallMemberFunction::get_class_static();
            options.context.user_data = p_action->data;
            options.context.method = p_action->method;
            options.context.class_name = p_action->class_name;
            options.position = spawn_position;
            options.drag_pin = _drag_from_pin;

            spawn_node(options);
            break;
        }
        case OrchestratorEditorActionDefinition::ACTION_CALL_SCRIPT_FUNCTION:
        {
            ERR_FAIL_COND_MSG(!p_action->method.has_value(), "Call script function has no method");

            NodeSpawnOptions options;
            options.node_class = OScriptNodeCallScriptFunction::get_class_static();
            options.context.method = p_action->method;
            options.position = spawn_position;
            options.drag_pin = _drag_from_pin;

            spawn_node(options);
            break;
        }
        case OrchestratorEditorActionDefinition::ACTION_EVENT:
        {
            ERR_FAIL_COND_MSG(!p_action->method.has_value(), "Handle event has no method");

            NodeSpawnOptions options;
            options.node_class = OScriptNodeEvent::get_class_static();
            options.context.method = p_action->method;
            options.position = spawn_position;
            options.drag_pin = _drag_from_pin;

            spawn_node(options);
            break;
        }
        case OrchestratorEditorActionDefinition::ACTION_EMIT_MEMBER_SIGNAL:
        {
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
        case OrchestratorEditorActionDefinition::ACTION_EMIT_SIGNAL:
        {
            ERR_FAIL_COND_MSG(!p_action->method.has_value(), "Emit signal function has no method");

            NodeSpawnOptions options;
            options.node_class = OScriptNodeEmitSignal::get_class_static();
            options.context.method = p_action->method;
            options.position = spawn_position;
            options.drag_pin = _drag_from_pin;

            spawn_node(options);
            break;
        }
        case OrchestratorEditorActionDefinition::ACTION_VARIABLE_GET:
        {
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
        case OrchestratorEditorActionDefinition::ACTION_VARIABLE_SET:
        {
            ERR_FAIL_COND_MSG(!p_action->property.has_value(), "Set variable has no property");

            NodeSpawnOptions options;
            options.node_class = OScriptNodeVariableSet::get_class_static();
            options.context.variable_name = p_action->property.value().name;
            options.position = spawn_position;
            options.drag_pin = _drag_from_pin;

            spawn_node(options);
            break;
        }
        default:
        {
            const String message = vformat("Unknown action type %d - %s", p_action->type, p_action->name);
            OrchestratorEditorDialogs::error(message, "Failed to spawn node", false);
            break;
        }
    }
}

void OrchestratorEditorGraphPanel::_action_menu_canceled()
{
    _drag_from_pin.reset();
}

void OrchestratorEditorGraphPanel::_idle_timeout()
{
    if (_knot_editor)
        _knot_editor->flush_knot_cache(_graph);

    // Notify view container to execute validation
    emit_signal("validate_script");
}

void OrchestratorEditorGraphPanel::_grid_pattern_changed(int p_index)
{
    #if GODOT_VERSION >= 0x040300
    set_grid_pattern(CAST_INT_TO_ENUM(GridPattern, _grid_pattern->get_item_metadata(p_index)));
    #endif
}

void OrchestratorEditorGraphPanel::_settings_changed()
{
    if (_theme_update_timer->is_inside_tree())
    {
        if (!_theme_update_timer->is_stopped())
            return;
        _theme_update_timer->start();
    }

    set_minimap_enabled(ORCHESTRATOR_GET("ui/graph/show_minimap", false));
    set_show_arrange_button(ORCHESTRATOR_GET("ui/graph/show_arrange_button", false));

    const Color knot_selected_color = ORCHESTRATOR_GET("ui/graph/knot_selected_color", Color(0.68f, 0.44f, 0.09f));
    _knot_editor->set_selected_color(knot_selected_color);

    _idle_time = EDITOR_GET("text_editor/completion/idle_parse_delay");
    _idle_time_with_errors = EDITOR_GET("text_editor/completion/idle_parse_delay_with_errors_found");

    _show_overlay_action_tooltips = ORCHESTRATOR_GET("ui/graph/show_overlay_action_tooltips", true);
    _disconnect_control_flow_when_dragged = ORCHESTRATOR_GET("ui/graph/disconnect_control_flow_when_dragged", true);
    _show_advanced_tooltips= ORCHESTRATOR_GET("ui/graph/show_advanced_tooltips", false);

    bool node_update_required = false;
    node_update_required |= ORCHESTRATOR_GET_TRACK(_show_type_icons, "ui/nodes/show_type_icons", true);
    node_update_required |= ORCHESTRATOR_GET_TRACK(_resizable_by_default, "ui/nodes/resizable_by_default", true);

    if (_graph.is_valid())
    {
        // While we iterate each node, each call checks the current state against the settings values
        // and only queues redraws if and only if there are variances in the values to minimize the
        // impact of these types of changes.
        for_each<GraphElement>([&] (GraphElement* element) {
            if (OrchestratorEditorGraphNode* node = cast_to<OrchestratorEditorGraphNode>(element))
            {
                if (node->is_resizable() != _resizable_by_default)
                    node->set_resizable(_resizable_by_default);

                node->set_show_type_icons(_show_type_icons);
                node->set_show_advanced_tooltips(_show_advanced_tooltips);

                // Needed for connection color changes.
                node->redraw_connections();
            }
            element->queue_redraw();
        });

    }
}

void OrchestratorEditorGraphPanel::_show_drag_hint(const String& p_hint_text) const
{
    if (!_show_overlay_action_tooltips || !_drag_hint || !_drag_hint_timer)
        return;

    _drag_hint->set_text(vformat("Hint:\n%s", p_hint_text));
    _drag_hint->show();
    _drag_hint_timer->start();
}

bool OrchestratorEditorGraphPanel::_is_delete_confirmation_enabled()
{
    return ORCHESTRATOR_GET("ui/graph/confirm_on_delete", true);
}

bool OrchestratorEditorGraphPanel::_can_duplicate_nodes(const Vector<OrchestratorEditorGraphNode*>& p_nodes, bool p_error_dialog)
{
    for (OrchestratorEditorGraphNode* node : p_nodes)
    {
        if (!node->_node->can_duplicate())
        {
            if (p_error_dialog)
            {
                const String message = vformat("Cannot duplicate node '%s' with ID %d", node->get_title(), node->get_id());
                OrchestratorEditorDialogs::error(message);
            }
            return false;
        }
    }
    return true;
}

void OrchestratorEditorGraphPanel::_set_scroll_offset_and_zoom(const Vector2& p_scroll_offset, float p_zoom, const Callable& p_callback)
{
    if (is_inside_tree() && get_tree())
    {
        const Ref<Tween> tween = get_tree()->create_tween();
        if (!tween.is_valid())
            return;

        tween->tween_method(Callable(this, "set_zoom"), get_zoom(), p_zoom, 0.0);
        tween->chain()->tween_method(Callable(this, "set_scroll_offset"), get_scroll_offset(), p_scroll_offset, 0.0);
        tween->set_ease(Tween::EASE_IN_OUT);

        if (p_callback.is_valid())
            tween->connect("finished", p_callback);

        tween->play();
    }
}

void OrchestratorEditorGraphPanel::_queue_autowire(OrchestratorEditorGraphNode* p_spawned_node, OrchestratorEditorGraphPin* p_origin_pin)
{
    ERR_FAIL_NULL_MSG(p_spawned_node, "Cannot initiate an autowire operation with an invalid node reference");
    ERR_FAIL_NULL_MSG(p_origin_pin, "Cannot initiate an autowire operation with an invalid pin reference");

    const Vector<OrchestratorEditorGraphPin*> choices = p_spawned_node->get_eligible_autowire_pins(p_origin_pin);

    // Do nothing if there are no eligible choices
    if (choices.size() == 0)
        return;

    if (choices.size() == 1)
    {
        // When there is only one choice, there is no need for the autowire dialog.
        link(p_origin_pin, choices[0]);
        return;
    }

    // Compute exact matches for class types
    Vector<OrchestratorEditorGraphPin*> exact_matches;
    for (OrchestratorEditorGraphPin* choice : choices)
    {
        if (choice->get_property_info().class_name.match(p_origin_pin->get_property_info().class_name))
            exact_matches.push_back(choice);
    }

    // Handle cases where class matches rank higher and have precedence
    if (exact_matches.size() == 1)
    {
        link(p_origin_pin, exact_matches[0]);
        return;
    }

    // For operator nodes, always auto-wire the first eligible pin.
    if (cast_to<OScriptNodeOperator>(p_spawned_node->_node.ptr()))
    {
        link(p_origin_pin, choices[0]);
        return;
    }

    // At this point no auto-resolution could be made, show the dialog if enabled
    const bool autowire_dialog_enabled = ORCHESTRATOR_GET("ui/graph/show_autowire_selection_dialog", true);
    if (!autowire_dialog_enabled)
        return;

    OrchestratorAutowireConnectionDialog* autowire = memnew(OrchestratorAutowireConnectionDialog);

    autowire->connect("confirmed", callable_mp_lambda(this, [autowire, p_origin_pin, this] {
        OrchestratorEditorGraphPin* selected = autowire->get_autowire_choice();
        if (selected)
            link(p_origin_pin, selected);
    }));

    autowire->popup_autowire(choices);
}

Vector2 OrchestratorEditorGraphPanel::_get_center() const
{
    return get_scroll_offset() + (get_size() / 2.0);
}

void OrchestratorEditorGraphPanel::_update_theme_item_cache()
{
    if (_in_theme_update)
        return;

    // As this method sets the theme below, this guard will trigger setting the argument
    // as true and will only clear it back to false when the method exits. So when the
    // set_theme causes a new NOTIFICATION_THEME_CHANGED notification, this method acts
    // as a no-op and exits early.
    ScopedThemeGuard guard(_in_theme_update);

    Control* parent_control = get_menu_control()->get_parent_control();
    Ref<StyleBoxFlat> panel = parent_control->get_theme_stylebox("panel")->duplicate();
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
    theme->set_font("font", "Label", _theme_cache.label_font);
    theme->set_font("font", "GraphNodeTitleLabel", _theme_cache.label_bold_font);
    theme->set_font("font", "LineEdit", _theme_cache.label_font);
    theme->set_font("font", "Button", _theme_cache.label_font);
    set_theme(theme);
}

void OrchestratorEditorGraphPanel::_update_menu_theme()
{
    Control* control = get_menu_control()->get_parent_control();
    control->add_theme_stylebox_override("panel", _theme_cache.panel);
}

void OrchestratorEditorGraphPanel::_refresh_panel_with_model()
{
    clear_connections();

    for (int i = get_child_count() - 1; i >= 0; i--)
    {
        GraphElement* element = cast_to<GraphElement>(get_child(i));
        if (element)
        {
            remove_child(element);
            element->queue_free();
        }
    }

    for (const Ref<OrchestrationGraphNode>& node : _graph->get_nodes())
    {
        OrchestratorEditorGraphNode* graph_node = OrchestratorEditorGraphNodeFactory::create_node(node);
        ERR_CONTINUE_MSG(!graph_node, "Failed to create graph node for node id " + itos(node->get_id()));

        // Must come first so when pin widget sizes are computed in set_node, they have non-zero values
        graph_node->set_name(itos(node->get_id()));
        add_child(graph_node);

        graph_node->set_node(node);
        graph_node->set_resizable(_resizable_by_default);
        graph_node->set_show_type_icons(_show_type_icons);
        graph_node->set_show_advanced_tooltips(_show_advanced_tooltips);
        graph_node->set_position_offset(node->get_position());
        graph_node->set_size(node->get_size());
    }

    for (const Connection& E : _graph->get_connections())
    {
        Error err = connect_node(itos(E.from_node), E.from_port, itos(E.to_node), E.to_port);
        ERR_CONTINUE_MSG(err != OK, "Failed to create graph connection for connection id " + itos(E.id));
    }

    _knot_editor->update(_graph->get_knots());

    // Queue up a revalidation sequence
    if (_idle_timer->is_stopped())
        _idle_timer->start();
}

void OrchestratorEditorGraphPanel::_refresh_panel_connections_with_model()
{
    clear_connections();

    for (const Connection& E : _graph->get_connections())
    {
        Error err = connect_node(itos(E.from_node), E.from_port, itos(E.to_node), E.to_port);
        ERR_CONTINUE_MSG(err != OK, "Failed to create graph connection for connection id " + itos(E.id));
    }

    emit_signal("connections_changed");

    if (_idle_timer->is_stopped())
        _idle_timer->start();
}

void OrchestratorEditorGraphPanel::_update_box_selection_state(const Ref<InputEvent>& p_event)
{
    Ref<InputEventMouseButton> mb = p_event;
    if (mb.is_valid())
    {
        if (mb->get_button_index() == MOUSE_BUTTON_LEFT && mb->is_pressed())
        {
            // Check whether the left click triggers box reselection
            // While GraphEdit manages this internally, the information is not directly made available
            // to derived implementations, and this information is needed to ignore selecting specific
            // custom graph elements, like GraphEdit does for GraphFrame objects in 4.3+.
            GraphElement* element = nullptr;
            for (int i = 0; i < get_child_count(); i++)
            {
                if (GraphElement* child = cast_to<GraphElement>(get_child(i)))
                {
                    const Rect2 area(Point2(), child->get_size());
                    const Vector2 point = (mb->get_position() - child->get_position()) / get_zoom();
                    if (area.has_point(point) && IS_COMMENT(child) && child->_has_point(point))
                    {
                        element = child;
                        break;
                    }
                }
            }

            if (!element)
            {
                _box_selection = true;
                _box_selection_from = mb->get_position();
            }
        }

        if (mb->get_button_index() == MOUSE_BUTTON_LEFT && !mb->is_pressed() && _box_selection)
            _box_selection = false;
    }

    const Ref<InputEventMouseMotion> mm = p_event;
    if (mm.is_valid() && _box_selection)
    {
        const Vector2 select_to = mm->get_position();
        const Rect2 select_area = Rect2(_box_selection_from.min(select_to), (_box_selection_from - select_to).abs());

        for_each<GraphElement>([&] (GraphElement* element) {
            if (IS_COMMENT(element) && !select_area.encloses(element->get_rect()))
                element->call_deferred("set_selected", false);
        });
    }
}

void OrchestratorEditorGraphPanel::_drop_data_files(const String& p_node_type, const Array& p_files, const Vector2& p_at_position)
{
    Vector2 position = p_at_position;

    for (int i = 0; i < p_files.size(); i++)
    {
        NodeSpawnOptions options;
        options.node_class = p_node_type;
        options.context.resource_path = p_files[i];
        options.position = position;

        OrchestratorEditorGraphNode* spawned_node = spawn_node(options);
        if (spawned_node)
            position.y += spawned_node->get_size().height + 10;
    }
}

void OrchestratorEditorGraphPanel::_drop_data_property(const Dictionary& p_property, const Vector2& p_at_position, const String& p_path, bool p_setter)
{
    const String node_class_type = p_setter
        ? OScriptNodePropertySet::get_class_static()
        : OScriptNodePropertyGet::get_class_static();

    NodeSpawnOptions options;
    options.node_class = node_class_type;
    options.context.property = DictionaryUtils::to_property(p_property);
    options.position = p_at_position;

    if (!p_path.is_empty())
        options.context.node_path = p_path;

    spawn_node(options);
}

void OrchestratorEditorGraphPanel::_drop_data_variable(const String& p_name, const Vector2& p_at_position, bool p_validated, bool p_setter)
{
    const String node_class_type = p_setter
        ? OScriptNodeVariableSet::get_class_static()
        : OScriptNodeVariableGet::get_class_static();

    NodeSpawnOptions options;
    options.node_class = node_class_type;
    options.context.variable_name = p_name;
    options.position = p_at_position;

    if (!p_setter)
        options.context.user_data = DictionaryUtils::of({{ "validation", p_validated }});

    spawn_node(options);
}

bool OrchestratorEditorGraphPanel::_is_in_port_hotzone(const Vector2& p_pos, const Vector2& p_mouse_pos, const Vector2i& p_port_size, bool p_left)
{
    const int32_t port_hotzone_outer_extent = get_theme_constant("port_hotzone_outer_extent");
    const int32_t port_hotzone_inner_extent = get_theme_constant("port_hotzone_inner_extent");

    const String hotzone_percent = ORCHESTRATOR_GET("ui/nodes/connection_hotzone_scale", "100%");
    const Vector2i port_size = p_port_size * (hotzone_percent.replace("%", "").to_float() / 100.0);

    const Rect2 hotzone = Rect2(
        p_pos.x - (p_left ? port_hotzone_outer_extent : port_hotzone_inner_extent),
        p_pos.y - port_size.height / 2.0,
        port_hotzone_inner_extent + port_hotzone_outer_extent,
        port_size.height);

    return hotzone.has_point(p_mouse_pos);
}

void OrchestratorEditorGraphPanel::_set_edited(bool p_edited)
{
    _graph->get_orchestration()->as_script()->set_edited(p_edited);

    // Request revalidation post change
    _idle_timer->start();
}

void OrchestratorEditorGraphPanel::_get_graph_node_and_port(const Vector2& p_position, int& r_id, int& r_port_index) const
{
    r_id = -1;
    r_port_index = -1;

    for (int i = 0; i < get_child_count() && r_port_index == -1; i++)
    {
        if (OrchestratorEditorGraphNode* node = cast_to<OrchestratorEditorGraphNode>(get_child(i)))
        {
            const int port = node->get_port_at_position(p_position / get_zoom());
            if (port != -1)
            {
                r_id = node->get_id();
                r_port_index = port;
            }
        }
    }
}

bool OrchestratorEditorGraphPanel::_is_point_inside_node(const Vector2& p_point) const
{
    for (int i = 0; i < get_child_count(); i++)
    {
        GraphNode* node = cast_to<GraphNode>(get_child(i));
        OrchestratorEditorGraphNodeComment* comment = cast_to<OrchestratorEditorGraphNodeComment>(node);
        if (!comment && node && node->get_rect().has_point(p_point))
            return true;
    }
    return false;
}

void OrchestratorEditorGraphPanel::_disconnect_connection(const Dictionary& p_connection)
{
    const OScriptConnection connection = OScriptConnection::from_dict(p_connection);

    _disconnection_request(
        vformat("%d", connection.from_node),
        connection.from_port,
        vformat("%d", connection.to_node),
        connection.to_port);
}

void OrchestratorEditorGraphPanel::_create_connection_reroute(const Dictionary& p_connection, const Vector2& p_position)
{
    if (p_connection.is_empty())
        return;

    const Connection connection = Connection::from_dict(p_connection);
    const Vector2 position = (p_position + get_scroll_offset()) / get_zoom();

    GraphNode* source = find_node(connection.from_node);
    GraphNode* target = find_node(connection.to_node);

    _knot_editor->create_knot(connection, position, source, target, get_connection_lines_curvature());
}

void OrchestratorEditorGraphPanel::_drop_data_function(const Dictionary& p_function, const Vector2& p_at_position, bool p_as_callable)
{
    const MethodInfo method = DictionaryUtils::to_method(p_function);

    if (!p_as_callable)
    {
        NodeSpawnOptions options;
        options.node_class = OScriptNodeCallScriptFunction::get_class_static();
        options.context.method = method;
        options.position = p_at_position;

        spawn_node(options);
    }
    else
    {
        int ctor_index = 0;
        bool found = false;
        const BuiltInType callable_type = ExtensionDB::get_builtin_type(Variant::CALLABLE);
        for (; ctor_index < callable_type.constructors.size(); ctor_index++)
        {
            const ConstructorInfo& ci = callable_type.constructors[ctor_index];
            if (ci.arguments.size() == 2 && ci.arguments[0].type == Variant::OBJECT && ci.arguments[1].type == Variant::STRING_NAME)
            {
                found = true;
                break;
            }
        }

        if (found)
        {
            const Array arguments = DictionaryUtils::from_properties(callable_type.constructors[ctor_index].arguments);

            NodeSpawnOptions options;
            options.node_class = OScriptNodeComposeFrom::get_class_static();
            options.context.user_data = DictionaryUtils::of({{ "type", Variant::CALLABLE }, { "constructor_args", arguments }});
            options.position = p_at_position;

            OrchestratorEditorGraphNode* compose_node = spawn_node(options);
            if (compose_node)
            {
                compose_node->get_input_pin(1)->_pin->set_default_value(method.name);

                options.node_class = OScriptNodeSelf::get_class_static();
                options.context.user_data.reset();
                options.position = options.position - Vector2(200, 0);

                OrchestratorEditorGraphNode* self = spawn_node(options);
                if (self)
                    link(self->get_output_pin(0), compose_node->get_input_pin(0));
            }
        }
    }
}

void OrchestratorEditorGraphPanel::_gui_input(const Ref<InputEvent>& p_event)
{
    static const std::unordered_map<StringName, Vector2> direction_map = {
        {StringName("ui_left"),  Vector2(-1,  0)},
        {StringName("ui_right"), Vector2( 1,  0)},
        {StringName("ui_up"),    Vector2( 0, -1)},
        {StringName("ui_down"),  Vector2( 0,  1)},
    };

    // In Godot 4.2, UI delete events only applied to GraphNode and not GraphElement objects.
    // This creates an issue with Knots as they are based on GraphElement.
    // This will make sure a follow-up signal removes the selected knots.
    if (!_godot_version.at_least(4, 3))
    {
        if (p_event.is_valid() && p_event->is_action_pressed("ui_graph_delete", true) && p_event->is_pressed())
        {
            PackedStringArray knot_names;
            for_each<OrchestratorEditorGraphNodeKnot>([&] (auto* knot) { knot_names.push_back(knot->get_name()); });
            emit_signal("delete_nodes_request", knot_names);
        }
    }

    const Ref<InputEventMouseButton> mb = p_event;
    if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == MOUSE_BUTTON_RIGHT)
    {
        Dictionary hovered_connection = get_closest_connection_at_point(mb->get_position());
        if (!hovered_connection.is_empty())
        {
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

    // There is a bug where if the mouse hovers a connection and a node concurrently,
    // the connection color is changed, even when the mouse is inside the node.
    GraphEdit::_gui_input(p_event);

    const Ref<InputEventMouse> mouse = p_event;
    if (mouse.is_valid() && !_is_point_inside_node(mouse->get_position()))
    {
        const Ref<InputEventMouseMotion> mm = p_event;
        if (mm.is_valid())
        {
            _hovered_connection = get_closest_connection_at_point(mm->get_position());
            if (!_hovered_connection.is_empty())
                _show_drag_hint(_knot_editor->get_hint_message());

        }

        if (_knot_editor->is_create_knot_keybind(p_event) && !_hovered_connection.is_empty())
            _create_connection_reroute(_hovered_connection, mouse->get_position());
    }

    _update_box_selection_state(p_event);

    const Ref<InputEventKey> key = p_event;
    if (key.is_valid() && key->is_pressed())
    {
        // todo:
        //  Submitted https://github.com/godotengine/godot/pull/95614
        //  Can eventually rely on the "cut_nodes_request" signal rather than this approach
        if (key->is_action("ui_cut", true))
        {
            _cut_nodes_request();
            accept_event();
        }

        for (const std::pair<const StringName, Vector2>& E : direction_map)
        {
            if (key->is_action(E.first, true))
            {
                const float distance = is_snapping_enabled() ? get_snapping_distance() : 1;
                const Vector2 amount = E.second * distance;

                for_each<GraphElement>([&] (GraphElement* element) {
                    if (OrchestratorEditorGraphNode* node = cast_to<OrchestratorEditorGraphNode>(element))
                    {
                        node->set_position_offset(node->get_position_offset() + amount);
                        node->_node->set_position(node->get_position_offset());
                    }
                    else if (OrchestratorEditorGraphNodeKnot* knot = cast_to<OrchestratorEditorGraphNodeKnot>(element))
                    {
                        knot->set_position_offset(knot->get_position_offset() + amount);
                    }
                }, true);

                accept_event();
                break;
            }
        }

        if (key->get_keycode() == KEY_F9)
        {
            for_each<OrchestratorEditorGraphNode>([&] (OrchestratorEditorGraphNode* node) {
                _toggle_node_breakpoint(node);
            }, true);

            accept_event();
        }
    }
}

bool OrchestratorEditorGraphPanel::_can_drop_data(const Vector2& p_at_position, const Variant& p_data) const
{
    // Widget types that can be dropped
    static PackedStringArray allowed_types = Array::make(
        "files", "obj_property", "nodes", "function", "variable", "signal");

    if (p_data.get_type() != Variant::DICTIONARY)
        return false;

    const Dictionary& data = p_data;
    if (!data.has("type"))
        return false;

    const String drop_type = data["type"];
    if (!allowed_types.has(drop_type))
        return false;

    if (drop_type == "variable")
    {
        const Array& variable_data = data["variables"];
        if (!variable_data.is_empty())
        {
            const String name = variable_data[0];
            const Ref<OScriptVariable> variable = _graph->get_orchestration()->get_variable(name);
            if (variable.is_valid() && !variable->is_constant())
                _show_drag_hint("Use Ctrl to drop a Setter, Shift to drop a Getter variable node");
            else if (variable.is_valid())
                _show_drag_hint("Use Shift to drop a Getter variable node");
        }
    }

    return true;
}

void OrchestratorEditorGraphPanel::_drop_data(const Vector2& p_at_position, const Variant& p_data)
{
    // No need to let the hint continue to be visible when dropped
    _drag_hint->hide();

    // Since _can_drop_data validates this, this should be safe
    const Dictionary& data = p_data;
    const String drop_type = data["type"];

    // This is where the objects should spawn into the graph
    Vector2 spawn_position = (p_at_position + get_scroll_offset()) / get_zoom();;

    // Where the menu popup should spawn
    Vector2 popup_position = p_at_position + get_screen_position();

    if (drop_type == "nodes")
    {
        Node* edited_scene_root = get_tree()->get_edited_scene_root();
        if (!edited_scene_root)
            return;

        const Array nodes = data["nodes"];
        for (int i = 0; i < nodes.size(); i++)
        {
            Node* dropped_node = edited_scene_root->get_node_or_null(nodes[i]);
            if (!dropped_node)
                continue;

            const NodePath path = dropped_node->is_unique_name_in_owner()
                ? NodePath("%" + dropped_node->get_name())
                : edited_scene_root->get_path_to(dropped_node);

            String global_name;
            const Ref<Script> dropped_node_script = dropped_node->get_script();
            if (dropped_node_script.is_valid())
                global_name = ScriptServer::get_global_name(dropped_node_script);

            NodeSpawnOptions options;
            options.node_class = OScriptNodeSceneNode::get_class_static();
            options.context.node_path  = path;
            options.context.class_name = StringUtils::default_if_empty(global_name, dropped_node->get_class());
            options.position = spawn_position;

            OrchestratorEditorGraphNode* spawned = spawn_node(options);
            if (spawned)
                spawn_position.y += spawned->get_size().height + 10;
        }
    }
    else if (drop_type == "files")
    {
        const Array& files = data["files"];

        OrchestratorEditorContextMenu* menu = memnew(OrchestratorEditorContextMenu);
        menu->set_auto_destroy(true);
        add_child(menu);

        menu->add_separator(files.size() == 1 ? vformat("File %s", files[0]) : vformat("%d Files", files.size()));
        menu->add_item("Get Path", callable_mp_this(_drop_data_files).bind(OScriptNodeResourcePath::get_class_static(), files, spawn_position));
        menu->add_item("Preload", callable_mp_this(_drop_data_files).bind(OScriptNodePreload::get_class_static(), files, spawn_position));

        menu->set_position(popup_position);
        menu->popup();
    }
    else if (drop_type == "obj_property")
    {
        Object* object = data["object"];
        if (!object)
            return;

        NodePath path;
        if (Node* root = get_tree()->get_edited_scene_root())
        {
            if (Node* object_node = cast_to<Node>(object))
                path = root->get_path_to(object_node);
        }

        StringName property_name = data["property"];
        for (const PropertyInfo& property : DictionaryUtils::to_properties(object->get_property_list()))
        {
            if (property.name == property_name)
            {
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
    }
    else if (drop_type == "function")
    {
        const MethodInfo method = DictionaryUtils::to_method(data["functions"]);

        OrchestratorEditorContextMenu* menu = memnew(OrchestratorEditorContextMenu);
        menu->set_auto_destroy(true);
        add_child(menu);

        menu->add_separator("Function " + method.name);
        menu->add_item("Add Call to Function", callable_mp_this(_drop_data_function).bind(data["functions"], spawn_position, false));
        menu->add_item("Add as a Callable", callable_mp_this(_drop_data_function).bind(data["functions"], spawn_position, true));

        menu->set_position(popup_position);
        menu->popup();
    }
    else if (drop_type == "variable")
    {
        const Array& variables = data["variables"];
        if (variables.is_empty())
            return;

        const String variable_name = variables[0];
        const Ref<OScriptVariable> variable = _graph->get_orchestration()->get_variable(variable_name);
        if (!variable.is_valid())
            return;

        if (Input::get_singleton()->is_key_pressed(KEY_CTRL) && !variable->is_constant())
            _drop_data_variable(variable_name, spawn_position, false, true);
        else if (Input::get_singleton()->is_key_pressed(KEY_SHIFT))
            _drop_data_variable(variable_name, spawn_position, false, false);
        else
        {
            OrchestratorEditorContextMenu* menu = memnew(OrchestratorEditorContextMenu);
            menu->set_auto_destroy(true);
            add_child(menu);

            menu->add_separator("Variable " + variable_name);
            menu->add_item("Get " + variable_name, callable_mp_this(_drop_data_variable)
                .bind(variable_name, spawn_position, false, false));

            if (variable->get_variable_type() == Variant::OBJECT)
            {
                menu->add_item("Get " + variable_name + " with validation",
                    callable_mp_this(_drop_data_variable).bind(variable_name, spawn_position, true, false));
            }

            if (!variable->is_constant())
            {
                menu->add_item("Set " + variable_name,
                    callable_mp_this(_drop_data_variable).bind(variable_name, spawn_position, false, true));
            }

            menu->set_position(popup_position);
            menu->popup();
        }
    }
    else if (drop_type == "signal")
    {
        NodeSpawnOptions options;
        options.node_class = OScriptNodeEmitSignal::get_class_static();
        options.context.method = DictionaryUtils::to_method(data["signals"]);
        options.position = spawn_position;

        spawn_node(options);
    }
}

PackedVector2Array OrchestratorEditorGraphPanel::_get_connection_line(const Vector2& p_from_position, const Vector2& p_to_position) const
{
    // Create array of points from the from position to the to position, including all existing knots
    PackedVector2Array points;
    points.push_back(p_from_position);

    // Godot 4.2 does not provide the from/to positions affected by the zoom when called
    // Godot 4.3 provides the values pre-multiplied by the zoom
    Vector2 p_from_adjusted = p_from_position * (_godot_version.at_least(4, 3) ? 1.0 : get_zoom());
    Vector2 p_to_adjusted = p_to_position * (_godot_version.at_least(4, 3) ? 1.0 : get_zoom());

    int source_node_id = -1;
    int source_node_port = -1;
    int target_node_id = -1;
    int target_node_port = -1;

    _get_graph_node_and_port(p_from_adjusted, source_node_id, source_node_port);
    _get_graph_node_and_port(p_to_adjusted, target_node_id, target_node_port);

    if (source_node_port != -1 && target_node_port != -1)
    {
        Connection connection;
        connection.from_node = source_node_id;
        connection.from_port = source_node_port;
        connection.to_node = target_node_id;
        connection.to_port = target_node_port;

        PackedVector2Array knot_points = _knot_editor->get_knots_for_connection(connection.id);
        if (_godot_version.at_least(4, 3))
        {
            for (int i = 0; i < knot_points.size(); i++)
                knot_points[i] = knot_points[i] * get_zoom();
        }

        points.append_array(knot_points);
    }

    points.push_back(p_to_position);

    PackedVector2Array curve_points;
    const float curvature = get_connection_lines_curvature();
    for (const Ref<Curve2D>& curve : _knot_editor->get_curves_for_points(points, curvature))
    {
        if (curvature > 0)
            curve_points.append_array(curve->tessellate(5, 2.0));
        else
            curve_points.append_array(curve->tessellate(1));
    }

    return curve_points;
}

bool OrchestratorEditorGraphPanel::_is_node_hover_valid(const StringName& p_from_node, int32_t p_from_port,
                                                        const StringName& p_to_node, int32_t p_to_port)
{
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

bool OrchestratorEditorGraphPanel::_is_in_input_hotzone(Object* p_in_node, int32_t p_in_port, const Vector2& p_mouse_position)
{
    GraphNode* node = cast_to<GraphNode>(p_in_node);
    if (!node)
        return false;

    const Ref<Texture2D> icon = node->get_slot_custom_icon_left(p_in_port);
    if (!icon.is_valid())
        return false;

    Vector2i port_size = Vector2i(icon->get_width(), icon->get_height());
    int slot_index = node->get_input_port_slot(p_in_port);
    Control* child = cast_to<Control>(node->get_child(slot_index, false));
    port_size.height = MAX(port_size.height, child ? child->get_size().y : 0);

    const float zoom = get_zoom();
    const Vector2 pos = node->get_input_port_position(p_in_port) * zoom + node->get_position();
    return _is_in_port_hotzone(pos / zoom, p_mouse_position, port_size, true);
}

bool OrchestratorEditorGraphPanel::_is_in_output_hotzone(Object* p_in_node, int32_t p_in_port, const Vector2& p_mouse_position)
{
    GraphNode* node = cast_to<GraphNode>(p_in_node);
    if (!node)
        return false;

    const Ref<Texture2D> icon = node->get_slot_custom_icon_right(p_in_port);
    if (!icon.is_valid())
        return false;

    Vector2i port_size = Vector2i(icon->get_width(), icon->get_height());
    int slot_index = node->get_output_port_slot(p_in_port);
    Control* child = cast_to<Control>(node->get_child(slot_index, false));
    port_size.height = MAX(port_size.height, child ? child->get_size().y : 0);

    const float zoom = get_zoom();
    const Vector2 pos = node->get_output_port_position(p_in_port) * zoom + node->get_position();
    return _is_in_port_hotzone(pos / zoom, p_mouse_position, port_size, false);
}

#if GODOT_VERSION < 0x040300
static Vector2 get_closest_point_to_segment(const Vector2& p_point, const Vector2* p_segment)
{
    Vector2 p = p_point - p_segment[0];
    Vector2 n = p_segment[1] - p_segment[0];
    real_t l2 = n.length_squared();

    if (l2 < 1e-20f)
        return p_segment[0]; // Both points are the same, just give any.

    real_t d = n.dot(p) / l2;

    if (d <= 0.0f)
        return p_segment[0]; // Before first point.

    if (d >= 1.0f)
        return p_segment[1]; // After first point.

    return p_segment[0] + n * d; // Inside.
}

static float get_distance_to_segment(const Vector2& p_point, const Vector2* p_segment)
{
    return p_point.distance_to(get_closest_point_to_segment(p_point, p_segment));
}

Dictionary OrchestratorEditorGraphPanel::get_closest_connection_at_point(const Vector2& p_position, float p_max_distance)
{
    Vector2 transformed_point = p_position + get_scroll_offset();

    Dictionary closest_connection;
    float closest_distance = p_max_distance;

    TypedArray<Dictionary> connections = get_connection_list();
    for (int i = 0; i < connections.size(); i++)
    {
        const Dictionary& connection = connections[i];

        const String source_name = connection["from_node"];
        const int32_t source_port = connection["from_port"];
        OrchestratorEditorGraphNode* source = find_node(source_name);
        if (!source)
            continue;

        const String target_name = connection["to_node"];
        const int32_t target_port = connection["to_port"];
        OrchestratorEditorGraphNode* target = find_node(target_name);
        if (!target)
            continue;

        // What is cached
        Vector2 from_pos = source->get_output_port_position(source_port) + source->get_position_offset();
        Vector2 to_pos = target->get_input_port_position(target_port) + target->get_position_offset();

        if (_godot_version.at_least(4, 3))
        {
            from_pos *= get_zoom();
            to_pos *= get_zoom();
        }

        // This function is called during both draw and this logic, and so the results need to be handled
        // differently based on the context of the call in Godot 4.2.
        PackedVector2Array points = get_connection_line(from_pos, to_pos);
        if (points.is_empty())
            continue;

        if (!_godot_version.at_least(4, 3))
        {
            for (int j = 0; j < points.size(); j++)
                points[j] *= get_zoom();
        }

        const real_t line_thickness = get_connection_lines_thickness();

        Rect2 aabb(points[0], Vector2());
        for (int j = 0; j < points.size(); j++)
            aabb = aabb.expand(points[j]);

        aabb.grow_by(line_thickness * static_cast<real_t>(0.5));

        if (aabb.distance_to(transformed_point) > p_max_distance)
            continue;

        for (int j = 0; j < points.size(); j++)
        {
            float distance = get_distance_to_segment(transformed_point, &points[j]);
            if (distance <= line_thickness * 0.5 + p_max_distance && distance < closest_distance)
            {
                closest_distance = distance;
                closest_connection = connection;
            }
        }
    }

    return closest_connection;
}
#endif

void OrchestratorEditorGraphPanel::set_graph(const Ref<OrchestrationGraph>& p_graph)
{
    ERR_FAIL_COND_MSG(!p_graph.is_valid(), "The provided graph panel model is invalid");

    _graph = p_graph;

    set_name(_graph->get_graph_name());

    // When nodes are spawned or removed, this triggers a panel rebuild based on the model
    _graph->connect("node_added", callable_mp_this(_node_added));
    _graph->connect("node_removed", callable_mp_this(_node_removed));
    _graph->connect("changed", callable_mp_this(_graph_changed));
    _graph->connect("connection_knots_removed", callable_mp(_knot_editor, &KnotHelper::remove_knots_for_connection));
    // Setup events with KnotEditor now that a graph has been set
    _knot_editor->connect("refresh_connections_requested", callable_mp_this(_refresh_panel_connections_with_model));
    _knot_editor->connect("changed", callable_mp_this(_knots_changed));

    // When model triggers link/unlink, makes sure the UI updates
    // Great use case is when changing a variable type where a connection is no longer valid
    _graph->get_orchestration()->connect("connections_changed", callable_mp_this(_refresh_panel_connections_with_model));

    callable_mp_this(_refresh_panel_with_model).call_deferred();
}

void OrchestratorEditorGraphPanel::reloaded_from_file()
{
    _refresh_panel_with_model();
}

Control* OrchestratorEditorGraphPanel::get_menu_control() const
{
    return _toolbar_hflow;
}

Node* OrchestratorEditorGraphPanel::get_connection_layer_node() const
{
    for (int i = 0; i < get_child_count(); i++)
    {
        Node* child = get_child(i);
        if (child->get_name().match("_connection_layer"))
            return child;
    }
    return nullptr;
}

bool OrchestratorEditorGraphPanel::is_bookmarked(const OrchestratorEditorGraphNode* p_node) const
{
    return !p_node ? false : _bookmarks.has(p_node->get_id());
}

void OrchestratorEditorGraphPanel::set_bookmarked(OrchestratorEditorGraphNode* p_node, bool p_bookmarked)
{
    ERR_FAIL_NULL(p_node);

    const int node_id = p_node->get_id();

    const int index = _bookmarks.find(node_id);
    if (index != -1 && !p_bookmarked)
    {
        _bookmarks.remove_at(index);
        p_node->notify_bookmarks_changed();
    }
    else if (index == -1 && p_bookmarked)
    {
        _bookmarks.push_back(node_id);
        p_node->notify_bookmarks_changed();
    }
}

void OrchestratorEditorGraphPanel::goto_next_bookmark()
{
    if (_bookmarks.is_empty())
    {
        _bookmarks_index = -1;
        return;
    }

    if (_bookmarks_index >= _bookmarks.size())
        _bookmarks_index = -1;

    _bookmarks_index = (_bookmarks_index == -1)
        ? 0
        : (_bookmarks_index + 1) % _bookmarks.size();

    center_node_id(_bookmarks[_bookmarks_index]);
}

void OrchestratorEditorGraphPanel::goto_previous_bookmark()
{
    if (_bookmarks.is_empty())
    {
        _bookmarks_index = -1;
        return;
    }

    if (_bookmarks_index >= _bookmarks.size())
        _bookmarks_index = -1;

    _bookmarks_index = (_bookmarks_index == -1)
        ? _bookmarks.size() - 1
        : (_bookmarks_index - 1 + _bookmarks.size()) % _bookmarks.size();

    center_node_id(_bookmarks[_bookmarks_index]);
}

bool OrchestratorEditorGraphPanel::is_breakpoint(const OrchestratorEditorGraphNode* p_node) const
{
    ERR_FAIL_NULL_V(p_node, false);
    return _breakpoints.has(p_node->get_id());
}

void OrchestratorEditorGraphPanel::set_breakpoint(OrchestratorEditorGraphNode* p_node, bool p_breakpoint)
{
    ERR_FAIL_NULL(p_node);

    const int node_id = p_node->get_id();

    const int index = _breakpoints.find(node_id);
    if (index != -1 && !p_breakpoint)
    {
        _breakpoints.remove_at(index);
        _breakpoint_state.erase(node_id);
        p_node->notify_breakpoints_changed();
    }
    else if (index == -1 && p_breakpoint)
    {
        _breakpoints.push_back(node_id);
        _breakpoint_state[node_id] = true;
        p_node->notify_breakpoints_changed();
    }
}

bool OrchestratorEditorGraphPanel::get_breakpoint(OrchestratorEditorGraphNode* p_node)
{
    ERR_FAIL_NULL_V(p_node, false);
    return _breakpoint_state.has(p_node->get_id()) ? _breakpoint_state[p_node->get_id()] : false;
}

void OrchestratorEditorGraphPanel::goto_next_breakpoint()
{
    if (_breakpoints.is_empty())
    {
        _breakpoints_index = -1;
        return;
    }

    if (_breakpoints_index >= _breakpoints.size())
        _breakpoints_index = -1;

    _breakpoints_index = (_breakpoints_index == -1)
        ? 0
        : (_breakpoints_index + 1) % _breakpoints.size();

    center_node_id(_breakpoints[_breakpoints_index]);
}

void OrchestratorEditorGraphPanel::goto_previous_breakpoint()
{
    if (_breakpoints.is_empty())
    {
        _breakpoints_index = -1;
        return;
    }

    if (_breakpoints_index >= _breakpoints.size())
        _breakpoints_index = -1;

    _breakpoints_index = (_breakpoints_index == -1)
        ? _breakpoints.size() - 1
        : (_breakpoints_index - 1 + _breakpoints.size()) % _breakpoints.size();

    center_node_id(_breakpoints[_breakpoints_index]);
}

PackedInt32Array OrchestratorEditorGraphPanel::get_breakpoints() const
{
    PackedInt32Array active_breakpoints;
    for (const KeyValue<int, bool>& E : _breakpoint_state)
    {
        if (E.value && !active_breakpoints.has(E.key))
            active_breakpoints.push_back(E.key);
    }
    return active_breakpoints;
}

void OrchestratorEditorGraphPanel::clear_breakpoints()
{
    while (!_breakpoints.is_empty())
    {
        int node_id = _breakpoints[_breakpoints.size() - 1];

        #if GODOT_VERSION >= 0x040300
        if (OrchestratorEditorDebuggerPlugin* debugger = OrchestratorEditorDebuggerPlugin::get_singleton())
            debugger->set_breakpoint(_graph->get_orchestration()->as_script()->get_path(), node_id, false);
        #endif

        _breakpoints.remove_at(_breakpoints.size() - 1);
        _breakpoint_state.erase(node_id);
    }

    _refresh_panel_with_model();
}

void OrchestratorEditorGraphPanel::show_override_function_action_menu()
{
    _menu_position = _get_center();

    Ref<OrchestratorEditorActionGraphTypeRule> graph_type_rule;
    graph_type_rule.instantiate();
    graph_type_rule->set_graph_type(OrchestratorEditorActionDefinition::GRAPH_EVENT);

    Ref<OrchestratorEditorActionFilterEngine> filter_engine;
    filter_engine.instantiate();
    filter_engine->add_rule(memnew(OrchestratorEditorActionSearchTextRule));
    filter_engine->add_rule(memnew(OrchestratorEditorActionClassHierarchyScopeRule));
    filter_engine->add_rule(memnew(OrchestratorEditorActionVirtualFunctionRule));
    filter_engine->add_rule(graph_type_rule);

    GraphEditorFilterContext context;
    context.script = _graph->get_orchestration()->as_script();
    context.class_hierarchy = Array::make(_graph->get_orchestration()->get_base_type());

    OrchestratorEditorActionMenu* menu = memnew(OrchestratorEditorActionMenu);
    menu->set_title("Select a graph action");
    menu->set_suffix("graph_editor_overrides");
    menu->set_close_on_focus_lost(ORCHESTRATOR_GET("ui/actions_menu/close_on_focus_lost", false));
    menu->set_show_filter_option(false);
    menu->set_start_collapsed(false);
    menu->connect("action_selected", callable_mp_this(_action_menu_selection));
    menu->connect("canceled", callable_mp_this(_action_menu_canceled));

    menu->popup_centered(
        OrchestratorEditorActionRegistry::get_singleton()->get_actions(_graph->get_orchestration()->as_script()),
        filter_engine,
        context);
}

bool OrchestratorEditorGraphPanel::are_pins_compatible(OrchestratorEditorGraphPin* p_source, OrchestratorEditorGraphPin* p_target) const
{
    // todo:
    //  pull OrchestrationGraphPin logic up or rework
    //  variable node implementations use this to deal with variable type changes
    //  base node uses this during build validation
    return p_source->_pin->can_accept(p_target->_pin);
}

void OrchestratorEditorGraphPanel::link(OrchestratorEditorGraphPin* p_source, OrchestratorEditorGraphPin* p_target)
{
    p_source->_pin->link(p_target->_pin);
    _set_edited(true);

    _refresh_panel_connections_with_model();
}

void OrchestratorEditorGraphPanel::unlink(OrchestratorEditorGraphPin* p_source, OrchestratorEditorGraphPin* p_target)
{
    p_source->_pin->unlink(p_target->_pin);
    _set_edited(true);

    _refresh_panel_connections_with_model();
}

void OrchestratorEditorGraphPanel::unlink_all(OrchestratorEditorGraphPin* p_target, bool p_notify)
{
    p_target->_pin->unlink_all(p_notify);
    _set_edited(true);

    _refresh_panel_connections_with_model();
}

void OrchestratorEditorGraphPanel::unlink_node_all(OrchestratorEditorGraphNode* p_node)
{
    ERR_FAIL_NULL_MSG(p_node, "Cannot remove all node links with an invalid node reference");

    for (OrchestratorEditorGraphPin* pin : p_node->get_pins())
        pin->_pin->unlink_all(true);

    _set_edited(true);
    _refresh_panel_connections_with_model();
}

HashSet<OrchestratorEditorGraphNode*> OrchestratorEditorGraphPanel::get_connected_nodes(OrchestratorEditorGraphNode* p_node)
{
    const int node_id = p_node->get_id();

    HashSet<OrchestratorEditorGraphNode*> connections;
    for (const Connection& E : _graph->get_connections())
    {
        if (E.from_node == node_id)
            connections.insert(find_node(E.to_node));
        else if (E.to_node == node_id)
            connections.insert(find_node(E.from_node));
    }

    return connections;
}

HashSet<OrchestratorEditorGraphPin*> OrchestratorEditorGraphPanel::get_connected_pins(OrchestratorEditorGraphPin* p_pin)
{
    ERR_FAIL_NULL_V_MSG(p_pin, {}, "Cannot get connected pins for an invalid pin");

    const int32_t pin_port = p_pin->get_graph_node()->get_pin_port(p_pin);
    ERR_FAIL_COND_V_MSG(pin_port == -1, {}, "Failed to resolve pin port");

    const int node_id = p_pin->get_graph_node()->get_id();

    HashSet<OrchestratorEditorGraphPin*> connections;
    for (const Connection& E : _graph->get_connections())
    {
        if (E.from_node == node_id && E.from_port == pin_port && p_pin->get_direction() == PD_Output)
        {
            // Found connection from this pin.
            OrchestratorEditorGraphNode* target_node = find_node(E.to_node);
            if (target_node)
            {
                const int32_t to_slot = target_node->get_input_port_slot(E.to_port);
                connections.insert(target_node->get_input_pin(to_slot));
            }
        }
        else if (E.to_node == node_id && E.to_port == pin_port && p_pin->get_direction() == PD_Input)
        {
            // Found connection to this pin.
            OrchestratorEditorGraphNode* source_node = find_node(E.from_node);
            if (source_node)
            {
                const int32_t to_slot = source_node->get_output_port_slot(E.from_port);
                connections.insert(source_node->get_output_pin(to_slot));
            }
        }
    }

    return connections;
}

void OrchestratorEditorGraphPanel::remove_node(OrchestratorEditorGraphNode* p_node, bool p_confirm)
{
    if (p_confirm && _is_delete_confirmation_enabled())
        ORCHESTRATOR_CONFIRM("Do you wish to delete this node?", callable_mp_this(remove_node).bind(p_node, false));

    const int node_id = p_node->get_id();

    if (_breakpoints.has(node_id))
    {
        _breakpoint_state.erase(node_id);
        _breakpoints.remove_at(_breakpoints.find(node_id));
    }

    if (_bookmarks.has(node_id))
        _bookmarks.remove_at(_bookmarks.find(node_id));

    if (p_node->is_selected())
        p_node->set_selected(false);
    
    p_node->queue_free();

    _graph->get_orchestration()->remove_node(node_id);

    // This makes sure that we only ever emit 1 event during bulk node removal
    if (!_pending_nodes_changed_event)
    {
        _set_edited(true);
        _pending_nodes_changed_event = true;
        callable_mp_lambda(this, [&] {
            _pending_nodes_changed_event = false;
            emit_signal("nodes_changed");
        }).call_deferred();
    }
}

void OrchestratorEditorGraphPanel::remove_nodes(const TypedArray<OrchestratorEditorGraphNode>& p_nodes, bool p_confirm)
{
    if (p_confirm && _is_delete_confirmation_enabled())
    {
        ORCHESTRATOR_CONFIRM(vformat("Do you wish to delete %d node(s)?", p_nodes.size()),
            callable_mp_this(remove_nodes).bind(p_nodes, false));
    }

    for (int i = 0; i < p_nodes.size(); i++)
    {
        OrchestratorEditorGraphNode* node = cast_to<OrchestratorEditorGraphNode>(p_nodes[i]);
        if (node && node->can_user_delete_node())
            remove_node(node, false);
    }
}

OrchestratorEditorGraphNode* OrchestratorEditorGraphPanel::find_node(int p_id)
{
    return cast_to<OrchestratorEditorGraphNode>(find_child(itos(p_id), false, false));
}

OrchestratorEditorGraphNode* OrchestratorEditorGraphPanel::find_node(const StringName& p_name)
{
    return cast_to<OrchestratorEditorGraphNode>(find_child(p_name, false, false));
}

void OrchestratorEditorGraphPanel::clear_selections()
{
    for_each<GraphElement>([&] (GraphElement* element) {
        element->set_selected(false);
    });
}

void OrchestratorEditorGraphPanel::select_nodes(const PackedInt64Array& p_ids)
{
    clear_selections();

    for (const int64_t id : p_ids)
    {
        if (OrchestratorEditorGraphNode* node = find_node(id))
            node->set_selected(true);
    }
}

int64_t OrchestratorEditorGraphPanel::get_selection_count()
{
    return get_selected<GraphElement>().size();
}

Rect2 OrchestratorEditorGraphPanel::get_bounds_for_nodes(bool p_only_selected, bool p_padding)
{
    const Vector<OrchestratorEditorGraphNode*> nodes = get_all<OrchestratorEditorGraphNode>(p_only_selected);
    if (nodes.is_empty())
        return {};

    return get_bounds_for_nodes(nodes, p_padding);
}

Rect2 OrchestratorEditorGraphPanel::get_bounds_for_nodes(const Vector<OrchestratorEditorGraphNode*>& p_nodes, bool p_padding)
{
    Rect2 bounds = p_nodes[0]->get_graph_rect().grow(p_padding);
    for (int i = 1; i < p_nodes.size(); i++)
        bounds = bounds.merge(p_nodes[i]->get_graph_rect().grow(p_padding));

    return bounds;
}

void OrchestratorEditorGraphPanel::scroll_to_position(const Vector2& p_position, float p_time)
{
    // The provided position needs to be offset by half the viewport size to center on the position.
    const Vector2& position = p_position - (get_size() / 2.0);

    const Ref<Tween> tween = get_tree()->create_tween();
    if (!UtilityFunctions::is_equal_approx(1.f, get_zoom()))
        tween->tween_method(Callable(this, "set_zoom"), get_zoom(), 1.f, p_time);

    tween->chain()->tween_method(Callable(this, "set_scroll_offset"), get_scroll_offset(), position, p_time);
    tween->set_ease(Tween::EASE_IN_OUT);

    tween->play();
}

void OrchestratorEditorGraphPanel::center_node_id(int p_id)
{
    // Attempts to locate the node and if found, proceeds to center it.
    if (OrchestratorEditorGraphNode* node = find_node(p_id))
    {
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

void OrchestratorEditorGraphPanel::center_node(OrchestratorEditorGraphNode* p_node)
{
    GUARD_NULL(p_node);

    clear_selections();
    p_node->set_selected(true);

    scroll_to_position(p_node->get_graph_rect().get_center());
}

OrchestratorEditorGraphNode* OrchestratorEditorGraphPanel::spawn_node(const NodeSpawnOptions& p_options)
{
    ERR_FAIL_COND_V_MSG(p_options.node_class.is_empty(), nullptr, "No node class specified, cannot spawn node");
    ERR_FAIL_COND_V_MSG(!_graph.is_valid(), nullptr, "Cannot spawn into an invalid graph");

    const OScriptNodeInitContext& context = p_options.context;
    const Vector2& position = p_options.position;

    const Ref<OScriptNode> spawned_node = _graph->create_node(p_options.node_class, context, position);
    ERR_FAIL_COND_V_MSG(!spawned_node.is_valid(), nullptr, "Failed to spawn node");

    _set_edited(true);
    emit_signal("nodes_changed");

    OrchestratorEditorGraphNode* spawned_graph_node = find_node(spawned_node->get_id());
    ERR_FAIL_NULL_V_MSG(spawned_graph_node, nullptr, "Failed to find the spawned graph node");

    if (p_options.select_on_spawn)
        spawned_graph_node->set_selected(true);

    if (p_options.center_on_spawn)
        callable_mp_this(center_node).bind(spawned_graph_node).call_deferred();

    if (p_options.drag_pin && spawned_graph_node)
    {
        // When dragging from a pin, this indicates that autowiring should happen, but this needs to be done
        // as part of the next frame. This allows the caller to get a reference to the spawned node so it
        // can continue to perform any additional operations without having to deal with async operations
        // with the autowire dialog window.
        callable_mp_this(_queue_autowire).bind(spawned_graph_node, p_options.drag_pin).call_deferred();
    }

    return spawned_graph_node;
}

void OrchestratorEditorGraphPanel::validate()
{
    _idle_timer->start();
}

Variant OrchestratorEditorGraphPanel::get_edit_state() const
{
    PackedStringArray selections;
    for (int i = 0; i < get_child_count(); i++)
    {
        OrchestratorEditorGraphNode* node = cast_to<OrchestratorEditorGraphNode>(get_child(i));
        if (node && node->is_selected())
            selections.push_back(node->get_name());
    }

    Array breakpoints;
    for (const KeyValue<int, bool>& E : _breakpoint_state)
    {
        Dictionary data;
        data[E.key] = E.value;
        breakpoints.push_back(data);
    }

    Dictionary panel_state;
    panel_state["name"] = get_name();
    panel_state["viewport_offset"] = get_scroll_offset();
    panel_state["zoom"] = get_zoom();
    panel_state["selections"] = selections;
    panel_state["bookmarks"] = _bookmarks;
    panel_state["breakpoints"] = breakpoints;
    panel_state["minimap"] = is_minimap_enabled();
    panel_state["snapping"] = is_snapping_enabled();

    #if GODOT_VERSION >= 0x040300
    panel_state["grid"] = is_showing_grid();
    panel_state["grid_pattern"] = get_grid_pattern();
    #endif

    return panel_state;
}

void OrchestratorEditorGraphPanel::set_edit_state(const Variant& p_state, const Callable& p_completion_callback)
{
    const Dictionary state = p_state;

    const float zoom = state.get("zoom", 1.0);
    const Vector2 offset = state.get("viewport_offset", Vector2());

    _set_scroll_offset_and_zoom(offset, zoom, p_completion_callback);

    set_minimap_enabled(state.get("minimap", false));
    set_snapping_enabled(state.get("snapping", true));

    _bookmarks = state.get("bookmarks", PackedInt64Array());
    for (int bookmark : _bookmarks)
    {
        if (OrchestratorEditorGraphNode* node = find_node(bookmark))
            node->notify_bookmarks_changed();
    }

    Array breakpoints = state.get("breakpoints", Array());
    for (int i = 0; i < breakpoints.size(); i++)
    {
        const Dictionary& data = breakpoints[i];

        const int node_id = data.keys()[0];
        const bool status = data[node_id];

        if (!_graph->has_node(node_id))
            continue;

        _breakpoint_state[node_id] = status;
        _breakpoints.push_back(node_id);

        // Notify in deferred as GraphEdit does not yet have GraphNode instances
        callable_mp_lambda(this, [this, node_id] {
            OrchestratorEditorGraphNode* node = find_node(node_id);
            if (node) {
                node->notify_breakpoints_changed();
            }
        }).call_deferred();
    }

    #if GODOT_VERSION >= 0x040300
    set_show_grid(state.get("grid", true));

    const int grid_pattern = state.get("grid_pattern", 0);
    set_grid_pattern(CAST_INT_TO_ENUM(GridPattern, grid_pattern));
    _grid_pattern->select(grid_pattern);
    #endif
}

void OrchestratorEditorGraphPanel::_notification(int p_what)
{
    switch (p_what)
    {
        case NOTIFICATION_THEME_CHANGED:
        {
            _update_theme_item_cache();
            _update_menu_theme();
            break;
        }
        default:
            break;
    }
}

void OrchestratorEditorGraphPanel::_bind_methods()
{
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

OrchestratorEditorGraphPanel::OrchestratorEditorGraphPanel()
{
    _knot_editor = memnew(KnotHelper(_godot_version));
    add_child(_knot_editor);

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
    label->add_theme_font_size_override("font_size", 24);

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

    _idle_timer = memnew(Timer);
    _idle_timer->set_one_shot(true);
    _idle_timer->connect("timeout", callable_mp_this(_idle_timeout));
    add_child(_idle_timer);

    // New dots-based grid style was introduced in Godot 4.3.
    // Introduces a new drop-down option for selecting the specific grid pattern
    #if GODOT_VERSION >= 0x040300
    const String grid_pattern = ORCHESTRATOR_GET("ui/graph/grid_pattern", "Lines");
    const int selected = grid_pattern == "Lines" ? 0 : 1;
    _grid_pattern = memnew(OptionButton);
    _grid_pattern->add_item("Lines");
    _grid_pattern->set_item_metadata(0, GRID_PATTERN_LINES);
    _grid_pattern->add_item("Dots");
    _grid_pattern->set_item_metadata(1, GRID_PATTERN_DOTS);
    _grid_pattern->connect("item_selected", callable_mp_this(_grid_pattern_changed));
    _grid_pattern->select(selected);
    set_grid_pattern(CAST_INT_TO_ENUM(GridPattern, _grid_pattern->get_item_metadata(selected)));

    get_menu_hbox()->add_child(_grid_pattern);
    get_menu_hbox()->move_child(_grid_pattern, 5);

    VSeparator* sep = memnew(VSeparator());
    get_menu_hbox()->add_child(sep);
    get_menu_hbox()->move_child(sep, 6);
    #endif

    set_minimap_enabled(ORCHESTRATOR_GET("ui/graph/show_minimap", false));
    set_show_arrange_button(ORCHESTRATOR_GET("ui/graph/show_arrange_button", false));
    set_show_grid(ORCHESTRATOR_GET("ui/graph/grid_enabled", true));
    set_snapping_enabled(ORCHESTRATOR_GET("ui/graph/grid_snapping_enabled", true));
    set_right_disconnects(true);
    set_show_zoom_label(true);

    ProjectSettings::get_singleton()->connect("settings_changed", callable_mp_this(_settings_changed));
    EI->get_editor_settings()->connect("settings_changed", callable_mp_this(_settings_changed));

    _settings_changed();

    PanelContainer* toolbar_panel = static_cast<PanelContainer*>(get_menu_hbox()->get_parent());
    toolbar_panel->set_anchors_and_offsets_preset(PRESET_TOP_WIDE, PRESET_MODE_MINSIZE, 10);
    toolbar_panel->set_mouse_filter(MOUSE_FILTER_IGNORE);

    _toolbar_hflow = memnew(HFlowContainer);
    {
        Vector<Node*> nodes;
        for (int i = 0; i < get_menu_hbox()->get_child_count(); i++)
            nodes.push_back(get_menu_hbox()->get_child(i));

        for (Node* node : nodes)
        {
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
}

OrchestratorEditorGraphPanel::~OrchestratorEditorGraphPanel()
{
    // memdelete(_knot_editor);
}
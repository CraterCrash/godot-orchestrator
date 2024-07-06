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
#include "graph_node.h"

#include "common/logger.h"
#include "common/macros.h"
#include "common/scene_utils.h"
#include "common/settings.h"
#include "editor/plugins/orchestrator_editor_plugin.h"
#include "graph_edit.h"
#include "graph_node_pin.h"
#include "script/nodes/editable_pin_node.h"
#include "script/nodes/functions/call_function.h"
#include "script/nodes/functions/call_script_function.h"
#include "script/script.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_inspector.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_action.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/script_editor_base.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>

OrchestratorGraphNode::OrchestratorGraphNode(OrchestratorGraphEdit* p_graph, const Ref<OScriptNode>& p_node)
{
    _graph = p_graph;
    _node = p_node;

    // Setup defaults
    set_name(itos(_node->get_id()));
    set_resizable(OrchestratorSettings::get_singleton()->get_setting("ui/nodes/resizable_by_default", false));
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);
    set_meta("__script_node", p_node);

    #if GODOT_VERSION >= 0x040300
    _initialize_node_beakpoint_state();
    #endif

    _update_tooltip();
}

void OrchestratorGraphNode::_bind_methods()
{
}

void OrchestratorGraphNode::_notification(int p_what)
{
    if (p_what == NOTIFICATION_READY)
    {
        // Update the title bar widget layouts
        HBoxContainer* titlebar = get_titlebar_hbox();
        _indicators = memnew(HBoxContainer);
        titlebar->add_child(_indicators);

        Control* spacer = memnew(Control);
        spacer->set_custom_minimum_size(Vector2(3, 0));
        titlebar->add_child(spacer);

        // Used to replicate size/position state to underlying node resource
        connect("dragged", callable_mp(this, &OrchestratorGraphNode::_on_node_moved));
        connect("resized", callable_mp(this, &OrchestratorGraphNode::_on_node_resized));

        // Used to replicate state changes from node resource to the UI
        _node->connect("pins_changed", callable_mp(this, &OrchestratorGraphNode::_on_pins_changed));
        _node->connect("pin_connected", callable_mp(this, &OrchestratorGraphNode::_on_pin_connected));
        _node->connect("pin_disconnected", callable_mp(this, &OrchestratorGraphNode::_on_pin_disconnected));
        _node->connect("changed", callable_mp(this, &OrchestratorGraphNode::_on_changed));

        // Update title bar aspects
        _update_titlebar();
        _update_styles();

        // Update the pin display upon entering
        _update_pins();

        // IMPORTANT
        // The context menu must be attached to the title bar or else this will cause
        // problems with the GraphNode and slot/index logic when calling set_slot
        // functions.
        _context_menu = memnew(PopupMenu);
        _context_menu->connect("id_pressed", callable_mp(this, &OrchestratorGraphNode::_on_context_menu_selection));
        get_titlebar_hbox()->add_child(_context_menu);
    }
}

void OrchestratorGraphNode::_gui_input(const Ref<InputEvent>& p_event)
{
    Ref<InputEventMouseButton> button = p_event;
    if (button.is_null() || !button->is_pressed())
        return;

    if (button->is_double_click() && button->get_button_index() == MOUSE_BUTTON_LEFT)
    {
        if (_node->can_jump_to_definition())
        {
            if (Object* target = _node->get_jump_target_for_double_click())
            {
                _graph->request_focus(target);
                accept_event();
            }
        }
        return;
    }
    else if (button->get_button_index() == MOUSE_BUTTON_RIGHT)
    {
        // Show menu
        _show_context_menu(button->get_position());
        accept_event();
    }
}

OrchestratorGraphEdit* OrchestratorGraphNode::get_graph()
{
    return _graph;
}

int OrchestratorGraphNode::get_script_node_id() const
{
    return _node->get_id();
}

void OrchestratorGraphNode::set_inputs_for_accept_opacity(float p_opacity, OrchestratorGraphNodePin* p_other)
{
    for (int i = 0; i < get_input_port_count(); i++)
    {
        if (is_slot_enabled_left(i))
        {
            OrchestratorGraphNodePin* pin = get_input_pin(i);
            if (!pin->can_accept(p_other))
            {
                Color color = get_input_port_color(i);
                color.a = p_opacity;
                set_slot_color_left(i, color);
            }
        }
    }
}

void OrchestratorGraphNode::set_outputs_for_accept_opacity(float p_opacity, OrchestratorGraphNodePin* p_other)
{
    for (int i = 0; i < get_output_port_count(); i++)
    {
        if (is_slot_enabled_right(i))
        {
            OrchestratorGraphNodePin* pin = get_output_pin(i);
            if (!p_other->can_accept(pin))
            {
                Color color = get_output_port_color(i);
                color.a = p_opacity;
                set_slot_color_right(i, color);
            }
        }
    }
}

void OrchestratorGraphNode::set_all_inputs_opacity(float p_opacity)
{
    for (int i = 0; i < get_input_port_count(); i++)
    {
        if (is_slot_enabled_left(i))
        {
            Color color = get_input_port_color(i);
            color.a = p_opacity;
            set_slot_color_left(i, color);
        }
    }
}

void OrchestratorGraphNode::set_all_outputs_opacity(float p_opacity)
{
    for (int i = 0; i < get_output_port_count(); i++)
    {
        if (is_slot_enabled_right(i))
        {
            Color color = get_output_port_color(i);
            color.a = p_opacity;
            set_slot_color_right(i, color);
        }
    }
}

int OrchestratorGraphNode::get_inputs_with_opacity(float p_opacity)
{
    int count = 0;
    for (int i = 0; i < get_input_port_count(); i++)
    {
        if (is_slot_enabled_left(i))
        {
            Color color = get_input_port_color(i);
            if (UtilityFunctions::is_equal_approx(color.a, p_opacity))
                count++;
        }
    }
    return count;
}

int OrchestratorGraphNode::get_outputs_with_opacity(float p_opacity)
{
    int count = 0;
    for (int i = 0; i < get_input_port_count(); i++)
    {
        if (is_slot_enabled_right(i))
        {
            Color color = get_output_port_color(i);
            if (UtilityFunctions::is_equal_approx(color.a, p_opacity))
                count++;
        }
    }
    return count;
}

void OrchestratorGraphNode::unlink_all()
{
    Vector<Ref<OScriptNodePin>> pins = _node->find_pins();
    for (const Ref<OScriptNodePin>& pin : pins)
        pin->unlink_all();
}

void OrchestratorGraphNode::_update_pins()
{
    if (_is_add_pin_button_visible())
    {
        MarginContainer* margin = memnew(MarginContainer);
        margin->add_theme_constant_override("margin_bottom", 4);
        add_child(margin);

        HBoxContainer* container = memnew(HBoxContainer);
        container->set_h_size_flags(SIZE_EXPAND_FILL);
        container->set_alignment(BoxContainer::ALIGNMENT_END);
        margin->add_child(container);

        Button* button = memnew(Button);
        button->set_button_icon(SceneUtils::get_editor_icon("ZoomMore"));
        button->set_tooltip_text("Add new pin");
        container->add_child(button);

        button->connect("pressed", callable_mp(this, &OrchestratorGraphNode::_on_add_pin_pressed));
    }
}

void OrchestratorGraphNode::_update_indicators()
{
    // Free all child indicators
    for (int i = 0; i < _indicators->get_child_count(); i++)
        _indicators->get_child(i)->queue_free();

    if (_node->get_flags().has_flag(OScriptNode::ScriptNodeFlags::DEVELOPMENT_ONLY))
    {
        TextureRect* notification = memnew(TextureRect);
        notification->set_texture(SceneUtils::get_editor_icon("Notification"));
        notification->set_custom_minimum_size(Vector2(0, 24));
        notification->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
        notification->set_tooltip_text("Node only executes during development builds, not included in exported builds.");
        _indicators->add_child(notification);
    }

    if (_node->get_flags().has_flag(OScriptNode::ScriptNodeFlags::EXPERIMENTAL))
    {
        TextureRect* notification = memnew(TextureRect);
        notification->set_texture(SceneUtils::get_editor_icon("NodeWarning"));
        notification->set_custom_minimum_size(Vector2(0, 24));
        notification->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
        notification->set_tooltip_text("Node is experimental and behavior may change without notice.");
        _indicators->add_child(notification);
    }

    #if GODOT_VERSION >= 0x040300
    if (_node->has_breakpoint())
    {
        TextureRect* breakpoint = memnew(TextureRect);
        breakpoint->set_custom_minimum_size(Vector2(0, 24));
        breakpoint->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
        if (!_node->has_disabled_breakpoint())
        {
            breakpoint->set_texture(SceneUtils::get_editor_icon("DebugSkipBreakpointsOff"));
            breakpoint->set_tooltip_text("Debugger will break when processing this node");
        }
        else
        {
            breakpoint->set_texture(SceneUtils::get_editor_icon("DebugSkipBreakpointsOn"));
            breakpoint->set_tooltip_text("Debugger will skip the breakpoint when processing this node");
        }
        _indicators->add_child(breakpoint);
    }
    #endif
}

void OrchestratorGraphNode::_update_titlebar()
{
    HBoxContainer* titlebar = get_titlebar_hbox();

    // This should always be true but sanity check
    if (titlebar->get_child_count() > 0)
    {
        Ref<Texture2D> icon_texture;
        if (!_node->get_icon().is_empty())
            icon_texture = SceneUtils::get_editor_icon(_node->get_icon());

        TextureRect* rect = Object::cast_to<TextureRect>(titlebar->get_child(0));
        if (!rect && icon_texture.is_valid())
        {
            // Add node's icon to the UI
            rect = memnew(TextureRect);
            rect->set_custom_minimum_size(Vector2(0, 24));
            rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
            rect->set_texture(icon_texture);

            // Add the icon and move it to the start of the HBox.
            titlebar->add_child(rect);
            titlebar->move_child(rect, 0);
        }
        else if (rect && !icon_texture.is_valid())
        {
            if (!_node->get_icon().is_empty())
            {
                // New icon cannot be changed (make it look broken)
                rect->set_texture(SceneUtils::get_editor_icon("Unknown"));
            }
            else
            {
                // Icon removed, remove this from the UI
                rect->queue_free();
                rect = nullptr;
            }
        }
        else if (rect && icon_texture.is_valid())
        {
            // Changing the texture
            rect->set_texture(icon_texture);
        }

        set_title((rect ? " " : "") + _node->get_node_title() + "   ");
    }

    _update_indicators();
}

void OrchestratorGraphNode::_update_styles()
{
    Ref<OrchestratorThemeCache> cache = OrchestratorPlugin::get_singleton()->get_theme_cache();
    if (cache.is_valid())
    {
        const String type_name = vformat("GraphNode_%s", _node->get_node_title_color_name());
        begin_bulk_theme_override();
        add_theme_stylebox_override("panel", cache->get_theme_stylebox("panel", "GraphNode"));
        add_theme_stylebox_override("panel_selected", cache->get_theme_stylebox("panel_selected", "GraphNode"));
        add_theme_stylebox_override("titlebar", cache->get_theme_stylebox("titlebar", type_name));
        add_theme_stylebox_override("titlebar_selected", cache->get_theme_stylebox("titlebar_selected", type_name));
        end_bulk_theme_override();
    }
}

Color OrchestratorGraphNode::_get_selection_color() const
{
    return Color(0.68f, 0.44f, 0.09f);
}

void OrchestratorGraphNode::_update_node_attributes()
{
    // Attempt to shrink the container
    if (_resize_on_update())
        call_deferred("set_size", Vector2());

    // Some pin changes may affect the titlebar
    // We explicitly update the title here on change to capture that possibility
    _update_titlebar();

    _update_pins();
}

void OrchestratorGraphNode::_update_tooltip()
{
    String tooltip_text = _node->get_node_title();

    if (!_node->get_tooltip_text().is_empty())
        tooltip_text += "\n\n" + _node->get_tooltip_text();

    if (_node->get_flags().has_flag(OScriptNode::ScriptNodeFlags::DEVELOPMENT_ONLY))
        tooltip_text += "\n\nNode only executes during development. Exported builds will not include this node.";
    else if (_node->get_flags().has_flag(OScriptNode::ScriptNodeFlags::EXPERIMENTAL))
        tooltip_text += "\n\nThis node is experimental and may change in the future without warning.";

    tooltip_text += "\n\nID: " + itos(_node->get_id());
    tooltip_text += "\nClass: " + _node->get_class();
    tooltip_text += "\nFlags: " + itos(_node->get_flags());

    set_tooltip_text(SceneUtils::create_wrapped_tooltip_text(tooltip_text));
}

void OrchestratorGraphNode::_show_context_menu(const Vector2& p_position)
{
    // When showing the context-menu, if the current node is not selected, we should clear the
    // selection and the operation will only be applicable for this node and its pin.
    if (!is_selected())
    {
        get_graph()->clear_selection();
        set_selected(true);
    }

    _context_menu->clear();

    // Node actions
    _context_menu->add_separator("Node Actions");

    // todo: could consider a delegation to gather actions even from OrchestratorGraphNode impls
    // Get all node-specific actions, which are not UI-specific actions but rather logical actions that
    // should be taken by the OScriptNode resource rather than the OrchestratorGraphNode UI component.
    int node_action_id = CM_NODE_ACTION;
    _node->get_actions(_context_actions);
    for (const Ref<OScriptAction>& action : _context_actions)
    {
        if (action->get_icon().is_empty())
            _context_menu->add_item(action->get_text(), node_action_id);
        else
            _context_menu->add_icon_item(SceneUtils::get_editor_icon(action->get_icon()), action->get_text(), node_action_id);

        node_action_id++;
    }

    // Comment nodes are group-able, meaning that any node that is contained with the Comment node's rect window
    // can be automatically selected and dragged with the comment node. This can be done in two ways, one by
    // double-clicking the comment node to trigger the selection/deselection process or two by selecting the
    // "Select Group" or "Deselect Group" added here.
    if (is_groupable())
    {
        const String icon = vformat("Theme%sAll", is_group_selected() ? "Deselect" : "Select");
        const String text = vformat("%s Group", is_group_selected() ? "Deselect" : "Select");
        const int32_t id = is_group_selected() ? CM_DESELECT_GROUP : CM_SELECT_GROUP;
        _context_menu->add_icon_item(SceneUtils::get_editor_icon(icon), text, id);
    }

    _context_menu->add_icon_item(SceneUtils::get_editor_icon("Remove"), "Delete", CM_DELETE, KEY_DELETE);
    _context_menu->set_item_disabled(_context_menu->get_item_index(CM_DELETE), !_node->can_user_delete_node());

    _context_menu->add_icon_item(SceneUtils::get_editor_icon("ActionCut"), "Cut", CM_CUT, OACCEL_KEY(KEY_MASK_CTRL, KEY_X));
    _context_menu->add_icon_item(SceneUtils::get_editor_icon("ActionCopy"), "Copy", CM_COPY, OACCEL_KEY(KEY_MASK_CTRL, KEY_C));
    _context_menu->add_icon_item(SceneUtils::get_editor_icon("Duplicate"), "Duplicate", CM_DUPLICATE, OACCEL_KEY(KEY_MASK_CTRL, KEY_D));
    _context_menu->add_icon_item(SceneUtils::get_editor_icon("DistractionFree"), "Toggle Resizer", CM_RESIZABLE);

    _context_menu->add_icon_item(SceneUtils::get_editor_icon("Loop"), "Refresh Nodes", CM_REFRESH);
    _context_menu->add_icon_item(SceneUtils::get_editor_icon("Unlinked"), "Break Node Link(s)", CM_BREAK_LINKS);
    _context_menu->set_item_disabled(_context_menu->get_item_index(CM_BREAK_LINKS), !_node->has_any_connections());

    if (_is_editable())
        _context_menu->add_item("Add Option Pin", CM_ADD_OPTION_PIN);

    _context_menu->add_separator("Organization");
    _context_menu->add_item("Expand Node", CM_EXPAND_NODE);
    _context_menu->add_item("Collapse to Function", CM_COLLAPSE_FUNCTION);

    Ref<OScriptNodeCallScriptFunction> call_script_function = _node;
    if (!call_script_function.is_valid())
        _context_menu->set_item_disabled(_context_menu->get_item_index(CM_EXPAND_NODE), true);

    #if GODOT_VERSION >= 0x040300
    _context_menu->add_separator("Breakpoints");
    _context_menu->add_item("Toggle Breakpoint", CM_TOGGLE_BREAKPOINT, KEY_F9);
    if (_node->has_breakpoint())
    {
        _context_menu->add_item("Remove breakpoint", CM_REMOVE_BREAKPOINT);
        if (_node->has_disabled_breakpoint())
            _context_menu->add_item("Enable breakpoint", CM_ADD_BREAKPOINT);
        else
            _context_menu->add_item("Disable breakpoint", CM_DISABLE_BREAKPOINT);
    }
    #endif

    _context_menu->add_separator("Documentation");
    _context_menu->add_icon_item(SceneUtils::get_editor_icon("Help"), "View Documentation", CM_VIEW_DOCUMENTATION);

    _context_menu->set_position(get_screen_position() + (p_position * (real_t) get_graph()->get_zoom()));
    _context_menu->reset_size();
    _context_menu->popup();
}

void OrchestratorGraphNode::_simulate_action_pressed(const String& p_action_name)
{
    get_graph()->execute_action(p_action_name);
}

#if GODOT_VERSION >= 0x040300
void OrchestratorGraphNode::_initialize_node_beakpoint_state()
{
    Ref<OrchestratorEditorCache> cache = OrchestratorPlugin::get_singleton()->get_editor_cache();
    if (cache.is_valid())
    {
        const String path = _node->get_orchestration()->get_self()->get_path();
        #if GODOT_VERSION >= 0x040300
        const bool disabled = cache->is_node_disabled_breakpoint(path, _node->get_id());
        const bool enabled  = cache->is_node_breakpoint(path, _node->get_id());
        if (disabled || enabled)
            _set_breakpoint_state(disabled ? OScriptNode::BREAKPOINT_DISABLED : OScriptNode::BREAKPOINT_ENABLED);
        else
            _set_breakpoint_state(OScriptNode::BREAKPOINT_NONE);
        #endif
    }
}

void OrchestratorGraphNode::_set_breakpoint_state(OScriptNode::BreakpointFlags p_flag)
{
    _node->set_breakpoint_flag(p_flag);

    OrchestratorEditorDebuggerPlugin* debugger = OrchestratorEditorDebuggerPlugin::get_singleton();
    if (!debugger)
        return;

    const int node_id = _node->get_id();
    const String path = _node->get_orchestration()->get_self()->get_path();

    debugger->set_breakpoint(path, node_id, p_flag != OScriptNode::BreakpointFlags::BREAKPOINT_NONE);

    Ref<OrchestratorEditorCache> cache = OrchestratorPlugin::get_singleton()->get_editor_cache();
    if (cache.is_valid())
    {
        cache->set_breakpoint(path, node_id, p_flag == OScriptNode::BreakpointFlags::BREAKPOINT_ENABLED);
        cache->set_disabled_breakpoint(path, node_id, p_flag != OScriptNode::BreakpointFlags::BREAKPOINT_DISABLED);
    }
}
#endif

void OrchestratorGraphNode::_on_changed()
{
    // Notifications can bubble up to the OrchestratorGraphNode from either the OrchestratorGraphNodePin
    // or the underlying ScriptNode depending on the property that was changed and how
    // it is managed by the node. In this case, it's important that we also listen for
    // this callback and adjust the node-level attributes accordingly.
    _update_node_attributes();
}

bool OrchestratorGraphNode::_is_editable() const
{
    Ref<OScriptEditablePinNode> editable_node = _node;
    if (editable_node.is_valid())
        return true;

    Ref<OScriptNodeCallFunction> function_call_node = _node;
    if (function_call_node.is_valid() && function_call_node->is_vararg())
        return true;

    return false;
}

bool OrchestratorGraphNode::_is_add_pin_button_visible() const
{
    Ref<OScriptEditablePinNode> editable_node = _node;
    if (editable_node.is_valid())
        return editable_node->can_add_dynamic_pin();

    Ref<OScriptNodeCallFunction> function_call_node = _node;
    if (function_call_node.is_valid())
        return function_call_node->is_vararg();

    return false;
}

void OrchestratorGraphNode::_add_option_pin()
{
    Ref<OScriptEditablePinNode> editable = _node;
    if (editable.is_valid())
        editable->add_dynamic_pin();

    Ref<OScriptNodeCallFunction> function_call_node = _node;
    if(function_call_node.is_valid() && function_call_node->is_vararg())
        function_call_node->add_dynamic_pin();
}

List<OrchestratorGraphNode*> OrchestratorGraphNode::get_nodes_within_global_rect()
{
    Rect2 rect = get_global_rect();

    List<OrchestratorGraphNode*> results;
    _graph->for_each_graph_node([&](OrchestratorGraphNode* other) {
        if (other && other != this)
        {
            Rect2 other_rect = other->get_global_rect();
            if (rect.intersects(other_rect))
                results.push_back(other);
        }
    });
    return results;
}

int32_t OrchestratorGraphNode::get_port_at_position(const Vector2& p_position, EPinDirection p_direction)
{
    if (p_direction == PD_Input)
    {
        const Vector2 position = p_position - get_position_offset();
        const Rect2 zone = Rect2(position - Vector2(1,1), Vector2(2,2)); // Fake hotsize
        for (int i = 0; i < get_input_port_count(); i++)
        {
            if (zone.has_point(get_input_port_position(i)))
                return i;
        }
    }
    else if (p_direction == PD_Output)
    {
        const Vector2 position = p_position - get_position_offset();
        const Rect2 zone = Rect2(position - Vector2(1,1), Vector2(2,2)); // Fake hotzone
        for (int i = 0; i < get_output_port_count(); i++)
        {
            if (zone.has_point(get_output_port_position(i)))
                return i;
        }
    }
    return -1;
}

void OrchestratorGraphNode::_on_node_moved([[maybe_unused]] Vector2 p_old_pos, Vector2 p_new_pos)
{
    _node->set_position(p_new_pos);
}

void OrchestratorGraphNode::_on_node_resized()
{
    _node->set_size(get_size());
}

void OrchestratorGraphNode::_on_pins_changed()
{
    // no-op
}

void OrchestratorGraphNode::_on_pin_connected(int p_type, int p_index)
{
    if (OrchestratorGraphNodePin* pin = p_type == PD_Input ? get_input_pin(p_index) : get_output_pin(p_index))
        pin->set_default_value_control_visibility(false);
}

void OrchestratorGraphNode::_on_pin_disconnected(int p_type, int p_index)
{
    if (OrchestratorGraphNodePin* pin = p_type == PD_Input ? get_input_pin(p_index) : get_output_pin(p_index))
        pin->set_default_value_control_visibility(true);
}

void OrchestratorGraphNode::_on_add_pin_pressed()
{
    _add_option_pin();
}

void OrchestratorGraphNode::_on_context_menu_selection(int p_id)
{
    if (p_id >= CM_NODE_ACTION)
    {
        int action_index = p_id - CM_NODE_ACTION;
        if (action_index < _context_actions.size())
        {
            const Ref<OScriptAction>& action = _context_actions[action_index];
            if (action->get_handler().is_valid())
                action->get_handler().call();
        }
    }
    else
    {
        switch (p_id)
        {
            case CM_CUT:
            {
                _simulate_action_pressed("ui_copy");
                _simulate_action_pressed("ui_graph_delete");
                break;
            }
            case CM_COPY:
            {
                _simulate_action_pressed("ui_copy");
                break;
            }
            case CM_DUPLICATE:
            {
                _simulate_action_pressed("ui_graph_duplicate");
                break;
            }
            case CM_DELETE:
            {
                _simulate_action_pressed("ui_graph_delete");
                break;
            }
            case CM_REFRESH:
            {
                for (Ref<OScriptNode>& node : get_graph()->get_selected_script_nodes())
                    node->reconstruct_node();
                break;
            }
            case CM_BREAK_LINKS:
            {
                unlink_all();
                break;
            }
            case CM_VIEW_DOCUMENTATION:
            {
                get_graph()->goto_class_help(_node->get_help_topic());
                break;
            }
            case CM_RESIZABLE:
            {
                set_resizable(!is_resizable());
                break;
            }
            case CM_SELECT_GROUP:
            {
                select_group();
                break;
            }
            case CM_DESELECT_GROUP:
            {
                deselect_group();
                break;
            }
            case CM_ADD_OPTION_PIN:
            {
                _add_option_pin();
                break;
            }
            case CM_COLLAPSE_FUNCTION:
            {
                get_graph()->emit_signal("collapse_selected_to_function");
                break;
            }
            case CM_EXPAND_NODE:
            {
                get_graph()->emit_signal("expand_node", _node->get_id());
                break;
            }
            #if GODOT_VERSION >= 0x040300
            case CM_TOGGLE_BREAKPOINT:
            {
                // An OScriptNode registers the breakpoint data from the current open session.
                // This data is transient and is lost across restarts, although GDScript persists
                // this and we should implement this too.
                //
                // What we need to integrate with is EditorDebugNode. It is what will be responsible
                // for coordinating the breakpoint communication with the EngineDebugger that runs
                // in the separate prcoess that runs the game in F5. It uses Local/Remove debuggers.
                if (_node->has_breakpoint())
                    _set_breakpoint_state(OScriptNode::BreakpointFlags::BREAKPOINT_NONE);
                else
                    _set_breakpoint_state(OScriptNode::BreakpointFlags::BREAKPOINT_ENABLED);
                break;
            }
            case CM_ADD_BREAKPOINT:
            case CM_ENABLE_BREAKPOINT:
            {
                _set_breakpoint_state(OScriptNode::BreakpointFlags::BREAKPOINT_ENABLED);
                break;
            }
            case CM_REMOVE_BREAKPOINT:
            {
                _set_breakpoint_state(OScriptNode::BreakpointFlags::BREAKPOINT_NONE);
                break;
            }
            case CM_DISABLE_BREAKPOINT:
            {
                _set_breakpoint_state(OScriptNode::BreakpointFlags::BREAKPOINT_DISABLED);
                break;
            }
            #endif
            default:
            {
                WARN_PRINT("Feature not yet implemented");
                break;
            }
        }
    }

    // Cleanup actions
    _context_actions.clear();
}

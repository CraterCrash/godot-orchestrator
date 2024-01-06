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
#include "common/scene_utils.h"
#include "graph_edit.h"
#include "graph_node_pin.h"
#include "plugin/plugin.h"
#include "plugin/settings.h"
#include "script/nodes/editable_pin_node.h"
#include "script/script.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_inspector.hpp>
#include <godot_cpp/classes/gradient.hpp>
#include <godot_cpp/classes/gradient_texture2d.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_action.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/script_editor.hpp>
#include <godot_cpp/classes/script_editor_base.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/style_box_texture.hpp>

OrchestratorGraphNode::OrchestratorGraphNode(OrchestratorGraphEdit* p_graph, const Ref<OScriptNode>& p_node)
{
    _graph = p_graph;
    _node = p_node;

    // Setup defaults
    set_name(itos(_node->get_id()));
    set_resizable(true);
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);
    set_meta("__script_node", p_node);

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
        button->set_button_icon(SceneUtils::get_icon(this, "ZoomMore"));
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
        notification->set_texture(SceneUtils::get_icon(this, "Notification"));
        notification->set_custom_minimum_size(Vector2(0, 24));
        notification->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
        notification->set_tooltip_text("Node only executes during development builds, not included in exported builds.");
        _indicators->add_child(notification);
    }

    if (_node->get_flags().has_flag(OScriptNode::ScriptNodeFlags::EXPERIMENTAL))
    {
        TextureRect* notification = memnew(TextureRect);
        notification->set_texture(SceneUtils::get_icon(this, "NodeWarning"));
        notification->set_custom_minimum_size(Vector2(0, 24));
        notification->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
        notification->set_tooltip_text("Node is experimental and behavior may change without notice.");
        _indicators->add_child(notification);
    }
}

void OrchestratorGraphNode::_update_titlebar()
{
    HBoxContainer* titlebar = get_titlebar_hbox();

    // This should always be true but sanity check
    if (titlebar->get_child_count() > 0)
    {
        Ref<Texture2D> icon_texture;
        if (!_node->get_icon().is_empty())
            icon_texture = SceneUtils::get_icon(this, _node->get_icon());

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
                rect->set_texture(SceneUtils::get_icon(this, "Unknown"));
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
    bool apply_style_defaults = true;
    const String color_name = _node->get_node_title_color_name();
    if (!color_name.is_empty())
    {
        OrchestratorSettings* os = OrchestratorSettings::get_singleton();
        const String key = vformat("ui/node_colors/%s", color_name);
        if (os->has_setting(key))
        {
            apply_style_defaults = false;
            Color color = os->get_setting(key);

            Ref<StyleBox> panel = _make_colored_style("panel_selected", color);
            if (panel.is_valid())
                add_theme_stylebox_override("panel", panel);

            Ref<StyleBox> panel_selected = _make_selected_style("panel");
            if (panel_selected.is_valid())
                add_theme_stylebox_override("panel_selected", panel_selected);

            if (_use_gradient_color_style())
            {
                Ref<StyleBox> sb = _make_colored_style("titlebar_selected", color, true);
                if (sb.is_valid())
                    add_theme_stylebox_override("titlebar", sb);

                Ref<StyleBox> sb_selected = _make_selected_style("titlebar", true);
                if (sb_selected.is_valid())
                    add_theme_stylebox_override("titlebar_selected", sb_selected);

                Ref<StyleBox> sb2 = _make_gradient_titlebar_style("titlebar_selected", color, false);
                if (sb2.is_valid())
                    add_theme_stylebox_override("titlebar", sb2);
            }
            else
            {
                Ref<StyleBox> sb = _make_colored_style("titlebar_selected", color, true);
                if (sb.is_valid())
                    add_theme_stylebox_override("titlebar", sb);

                Ref<StyleBox> sb_selected = _make_selected_style("titlebar", true);
                if (sb_selected.is_valid())
                    add_theme_stylebox_override("titlebar_selected", sb_selected);
            }
        }
    }

    if (apply_style_defaults)
    {
        Ref<StyleBox> panel_selected = _make_selected_style("panel_selected");
        if (panel_selected.is_valid())
            add_theme_stylebox_override("panel_selected", panel_selected);

        Ref<StyleBox> titlebar_selected = _make_selected_style("titlebar_selected", true);
        if (titlebar_selected.is_valid())
            add_theme_stylebox_override("titlebar_selected", titlebar_selected);
    }
}

Color OrchestratorGraphNode::_get_selection_color() const
{
    return { 0.68f, 0.44f, 0.09f };
}

bool OrchestratorGraphNode::_use_gradient_color_style() const
{
    OrchestratorSettings* os = OrchestratorSettings::get_singleton();
    if (os)
        return os->get_setting("ui/nodes/titlebar/use_gradient_colors", false);

    return false;
}

Ref<StyleBox> OrchestratorGraphNode::_make_colored_style(const String& p_existing_name, const Color& p_color, bool p_titlebar)
{
    Ref<StyleBoxFlat> sb = get_theme_stylebox(p_existing_name);
    if (sb.is_valid())
    {
        Ref<StyleBoxFlat> dup = sb->duplicate(true);
        if (p_titlebar)
            dup->set_bg_color(p_color);
        else
            dup->set_border_color(p_color);

        _apply_corner_radius(dup, p_titlebar);

        return dup;
    }
    return sb;
}

Ref<StyleBox> OrchestratorGraphNode::_make_selected_style(const String& p_existing_name, bool p_titlebar)
{
    Ref<StyleBoxFlat> sb = get_theme_stylebox(p_existing_name);
    if (sb.is_valid())
    {
        Ref<StyleBoxFlat> dup = sb->duplicate(true);
        dup->set_border_color(_get_selection_color());
        dup->set_border_width(p_titlebar ? SIDE_TOP : SIDE_BOTTOM, 2);
        dup->set_border_width(SIDE_LEFT, 2);
        dup->set_border_width(SIDE_RIGHT, 2);

        _apply_corner_radius(dup, p_titlebar);

        return dup;
    }
    return sb;
}

Ref<StyleBox> OrchestratorGraphNode::_make_gradient_titlebar_style(const String& p_existing_name, const Color& p_color,
                                                                   bool p_selected)
{
    Ref<Gradient> gradient(memnew(Gradient));

    PackedFloat32Array offsets;
    offsets.push_back(0);
    offsets.push_back(1);
    gradient->set_offsets(offsets);

    PackedColorArray colors = gradient->get_colors();
    colors.reverse();
    gradient->set_colors(colors);

    Ref<GradientTexture2D> texture(memnew(GradientTexture2D));
    texture->set_gradient(gradient);
    texture->set_width(64);
    texture->set_height(64);
    texture->set_fill_to(Vector2(1.1, 0));

    Ref<StyleBoxTexture> titlebar_tex(memnew(StyleBoxTexture));
    titlebar_tex->set_texture(texture);
    titlebar_tex->set_modulate(p_color);

    Ref<StyleBox> sb = get_theme_stylebox(p_existing_name);
    if (sb.is_valid())
    {
        titlebar_tex->set_content_margin(SIDE_TOP, sb->get_content_margin(SIDE_TOP));
        titlebar_tex->set_content_margin(SIDE_RIGHT, sb->get_content_margin(SIDE_RIGHT));
        titlebar_tex->set_content_margin(SIDE_BOTTOM, sb->get_content_margin(SIDE_BOTTOM));
        titlebar_tex->set_content_margin(SIDE_LEFT, sb->get_content_margin(SIDE_LEFT));
    }

    if (p_selected)
        titlebar_tex->set_modulate(_get_selection_color());

    return titlebar_tex;
}

void OrchestratorGraphNode::_apply_corner_radius(Ref<StyleBoxFlat>& p_stylebox, bool p_titlebar)
{
    if (p_stylebox.is_valid() && _use_gradient_color_style())
    {
        // In this case, we explicitly only support a border radius of 6 on the bottom part.
        p_stylebox->set_corner_radius(CORNER_BOTTOM_LEFT, 6);
        p_stylebox->set_corner_radius(CORNER_BOTTOM_RIGHT, 6);
        return;
    }

    OrchestratorSettings* os = OrchestratorSettings::get_singleton();
    if (p_stylebox.is_valid() && os)
    {
        int border_radius = os->get_setting("ui/nodes/border_radius", 6);
        if (p_titlebar)
        {
            p_stylebox->set_corner_radius(CORNER_TOP_LEFT, border_radius);
            p_stylebox->set_corner_radius(CORNER_TOP_RIGHT, border_radius);
        }
        else
        {
            p_stylebox->set_corner_radius(CORNER_BOTTOM_LEFT, border_radius);
            p_stylebox->set_corner_radius(CORNER_BOTTOM_RIGHT, border_radius);
        }
    }
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
            _context_menu->add_icon_item(SceneUtils::get_icon(this, action->get_icon()), action->get_text(), node_action_id);

        node_action_id++;
    }

    // Check the node type
    Ref<OScriptEditablePinNode> editable_node = _node;

    // Comment nodes are group-able, meaning that any node that is contained with the Comment node's rect window
    // can be automatically selected and dragged with the comment node. This can be done in two ways, one by
    // double-clicking the comment node to trigger the selection/deselection process or two by selecting the
    // "Select Group" or "Deselect Group" added here.
    if (is_groupable())
    {
        const String icon = vformat("Theme%sAll", is_group_selected() ? "Deselect" : "Select");
        const String text = vformat("%s Group", is_group_selected() ? "Deselect" : "Select");
        const int32_t id = is_group_selected() ? CM_DESELECT_GROUP : CM_SELECT_GROUP;
        _context_menu->add_icon_item(SceneUtils::get_icon(this, icon), text, id);
    }

    _context_menu->add_icon_item(SceneUtils::get_icon(this, "Remove"), "Delete", CM_DELETE, KEY_DELETE);
    _context_menu->set_item_disabled(_context_menu->get_item_index(CM_DELETE), !_node->can_user_delete_node());

    _context_menu->add_icon_item(SceneUtils::get_icon(this, "ActionCut"), "Cut", CM_CUT, Key(KEY_MASK_CTRL | KEY_X));
    _context_menu->add_icon_item(SceneUtils::get_icon(this, "ActionCopy"), "Copy", CM_COPY, Key(KEY_MASK_CTRL | KEY_C));
    _context_menu->add_icon_item(SceneUtils::get_icon(this, "Duplicate"), "Duplicate", CM_DUPLICATE, Key(KEY_MASK_CTRL | KEY_D));

    _context_menu->add_icon_item(SceneUtils::get_icon(this, "Loop"), "Refresh Nodes", CM_REFRESH);
    _context_menu->add_icon_item(SceneUtils::get_icon(this, "Unlinked"), "Break Node Link(s)", CM_BREAK_LINKS);
    _context_menu->set_item_disabled(_context_menu->get_item_index(CM_BREAK_LINKS), !_node->has_any_connections());

    if (editable_node.is_valid())
        _context_menu->add_item("Add Option Pin", CM_ADD_OPTION_PIN);

    // todo: support breakpoints (See Trello)
    // _context_menu->add_separator("Breakpoints");
    // _context_menu->add_item("Toggle Breakpoint", CM_TOGGLE_BREAKPOINT, KEY_F9);
    // _context_menu->add_item("Add Breakpoint", CM_ADD_BREAKPOINT);

    _context_menu->add_separator("Documentation");
    _context_menu->add_icon_item(SceneUtils::get_icon(this, "Help"), "View Documentation", CM_VIEW_DOCUMENTATION);

    #ifdef _DEBUG
    _context_menu->add_separator("Debugging");
    _context_menu->add_icon_item(SceneUtils::get_icon(this, "Godot"), "Show details", CM_SHOW_DETAILS);
    #endif

    _context_menu->set_position(get_screen_position() + (p_position * (real_t) get_graph()->get_zoom()));
    _context_menu->reset_size();
    _context_menu->popup();
}

void OrchestratorGraphNode::_simulate_action_pressed(const String& p_action_name)
{
    Ref<InputEventAction> action = memnew(InputEventAction());
    action->set_action(p_action_name);
    action->set_pressed(true);

    Input::get_singleton()->parse_input_event(action);
}

void OrchestratorGraphNode::_on_changed()
{
    // Notifications can bubble up to the OrchestratorGraphNode from either the OrchestratorGraphNodePin
    // or the underlying ScriptNode depending on the property that was changed and how
    // it is managed by the node. In this case, it's important that we also listen for
    // this callback and adjust the node-level attributes accordingly.
    _update_node_attributes();
}

bool OrchestratorGraphNode::_is_add_pin_button_visible() const
{
    Ref<OScriptEditablePinNode> editable_node = _node;
    return editable_node.is_valid() && editable_node->can_add_dynamic_pin();
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
    Ref<OScriptEditablePinNode> editable_node = _node;
    if (editable_node.is_valid() && editable_node->can_add_dynamic_pin())
        editable_node->add_dynamic_pin();
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
                if (_node->can_user_delete_node())
                    get_script_node()->get_owning_script()->remove_node(_node->get_id());
                break;
            }
            case CM_REFRESH:
            {
                _node->reconstruct_node();
                break;
            }
            case CM_BREAK_LINKS:
            {
                unlink_all();
                break;
            }
            case CM_VIEW_DOCUMENTATION:
            {
                get_graph()->goto_class_help(_node->get_class());
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
                Ref<OScriptEditablePinNode> editable = _node;
                if (editable.is_valid())
                    editable->add_dynamic_pin();
                break;
            }
            #ifdef _DEBUG
            case CM_SHOW_DETAILS:
            {
                UtilityFunctions::print("--- Dump Node ", _node->get_class(), " ---");
                UtilityFunctions::print("Position: ", _node->get_position());

                Vector<Ref<OScriptNodePin>> pins = _node->get_all_pins();
                UtilityFunctions::print("Pins: ", pins.size());
                for (const Ref<OScriptNodePin>& pin : pins)
                {
                    UtilityFunctions::print("Pin[", pin->get_pin_name(), "]: ",
                                            pin->is_input() ? "Input" : "Output",
                                            " Default: ", pin->get_effective_default_value(),
                                            " Type: ", pin->get_pin_type_name(), " (", pin->get_type(), ")",
                                            " Target: ", pin->get_target_class(),
                                            " Flags: ", pin->get_flags().operator Variant());
                }
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

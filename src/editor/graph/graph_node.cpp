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
#include "editor/graph/graph_node.h"

#include "common/macros.h"
#include "common/property_utils.h"
#include "common/scene_utils.h"
#include "common/settings.h"
#include "editor/editor.h"
#include "editor/graph/graph_panel.h"
#include "editor/graph/graph_pin.h"
#include "editor/graph/graph_pin_factory.h"
#include "editor/theme/theme_cache.h"
#include "script/nodes/editable_pin_node.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

void OrchestratorEditorGraphNode::_resize_to_content()
{
    set_anchor_and_offset(SIDE_RIGHT, 0, 0);
    set_anchor_and_offset(SIDE_BOTTOM, 0, 0);
    _node->set_size(get_size());
}

void OrchestratorEditorGraphNode::_node_selected()
{
    if (_node.is_valid() && _node->can_inspect_node_properties())
        EI->edit_resource(_node->get_inspect_object());
}

void OrchestratorEditorGraphNode::_pin_connection_status_changed(int p_type, int p_index, bool p_connected)
{
    OrchestratorEditorGraphPin* pin = p_type == PD_Input ? get_input_pin(p_index) : get_output_pin(p_index);
    if (pin)
        pin->set_default_value_control_visible(!p_connected);
}

void OrchestratorEditorGraphNode::_update_titlebar()
{
    HBoxContainer* hbox = get_titlebar_hbox();

    Ref<Texture2D> icon;

    const String icon_name = _node->get_icon();
    if (!icon_name.is_empty() && icon_name.begins_with("res://"))
        icon = ResourceLoader::get_singleton()->load(icon_name);
    else if (!icon_name.is_empty())
        icon = SceneUtils::get_editor_icon(icon_name);

    TextureRect* icon_rect = cast_to<TextureRect>(hbox->find_child("NodeIcon", false, false));
    if (!icon_rect && icon.is_valid())
    {
        icon_rect = memnew(TextureRect);
        icon_rect->set_name("NodeIcon");
        icon_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
        icon_rect->set_expand_mode(TextureRect::EXPAND_FIT_WIDTH_PROPORTIONAL);
        icon_rect->set_size(Vector2(20, 20));
        hbox->add_child(icon_rect);
        hbox->move_child(icon_rect, 0);
    }

    if (icon_rect)
    {
        if (icon.is_valid())
            icon_rect->set_texture(icon);
        else
            icon_rect->queue_free();
    }

    set_title(_node->get_node_title());
}

// SplitPin
//  1. Pin is marked hidden.
//  2. Creates split-pin prototype node with the hidden pin.
//  3. Iterates all non-hidden pins on prototype node
//  4. Calls OrchestrationGraphNode::CreatePin() with details from prototype node pin.
//  5. Mark SubPin parent as the hidden pin.
//  6. The call to CreatePin() adds the pin to the GraphNode::Pins array, so they pop it.
//  7. The SubPin is added to the hidden pin's sub-pin list.
//  8. Performs some default value manipulation
//  9. Inserts the Pin->SubPins collection into the GraphNode->Pins array just after the parent pin.
// 10. Notifies GraphNode of pin changes

void OrchestratorEditorGraphNode::_create_pin_widgets()
{
    // todo:
    //  when we support split pins, these collections need to be filtered to exclude hidden pins,
    //  assuming that we handle split pins in the same way as UE

    const Vector<Ref<OrchestrationGraphPin>> inputs = _node->find_pins(PD_Input);
    const Vector<Ref<OrchestrationGraphPin>> outputs = _node->find_pins(PD_Output);

    const int64_t max_slots = Math::max(inputs.size(), outputs.size());
    if (max_slots == 0)
        return;

    // Used in a second pass to align left/right columns in multiple HBoxContainers for each row
    // to appear to have the same width, as one would expect had GraphNode used a GridContainer.
    real_t max_left_width = 0;
    real_t max_right_width = 0;

    for (int64_t slot_index = 0; slot_index < max_slots; slot_index++)
    {
        Slot slot;
        slot.slot = slot_index;

        HBoxContainer* row = memnew(HBoxContainer);
        row->set_h_size_flags(SIZE_FILL);
        row->add_theme_constant_override("separation", 10);
        slot.row = row;

        if (slot_index < inputs.size())
        {
            slot.left = OrchestratorEditorGraphPinFactory::create_pin_widget(inputs[slot_index]);
            if (slot.left)
            {
                slot.left->set_graph_node(this);
                row->add_child(slot.left);
            }
        }

        if (slot_index < outputs.size())
        {
            slot.right = OrchestratorEditorGraphPinFactory::create_pin_widget(outputs[slot_index]);
            if (slot.right)
            {
                slot.right->set_graph_node(this);
                row->add_child(slot.right);
            }
        }

        add_child(row);

        _slots[slot_index] = slot;

        OrchestratorEditorGraphPinSlotInfo left;
        OrchestratorEditorGraphPinSlotInfo right;

        if (slot.left)
        {
            max_left_width = Math::max(slot.left->get_size().x, max_left_width);
            left = slot.left->get_slot_info();
        }

        if (slot.right)
        {
            max_right_width = Math::max(slot.right->get_size().x, max_right_width);
            right = slot.right->get_slot_info();
        }

        set_slot(
            slot.slot,
            left.enabled,
            left.enabled ? left.type : 0,
            left.enabled ? left.color : Color(),
            right.enabled,
            right.enabled ? right.type : 0,
            right.enabled ? right.color : Color(),
            left.enabled ? SceneUtils::get_editor_icon(left.icon) : nullptr,
            right.enabled ? SceneUtils::get_editor_icon(right.icon) : nullptr);
    }

    // This keeps all rows aligned with the same widths
    // This prevents one row from encroaching on another's opposite side
    for (const SlotMapKeyValue& E : _slots)
    {
        if (E.value.left)
            E.value.left->set_custom_minimum_size(Vector2(max_left_width, 0));

        if (E.value.right)
            E.value.right->set_custom_minimum_size(Vector2(max_right_width, 0));
    }

    emit_signal("node_pins_changed", this);
}

void OrchestratorEditorGraphNode::_update_pin_widgets()
{
    for (const SlotMapKeyValue& E : _slots)
    {
        if (E.value.left)
        {
            E.value.left->set_icon_visible(_show_type_icons);
            E.value.left->set_show_advanced_tooltips(_show_advanced_tooltips);
        }

        if (E.value.right)
        {
            E.value.right->set_icon_visible(_show_type_icons);
            E.value.right->set_show_advanced_tooltips(_show_advanced_tooltips);
        }
    }
}

void OrchestratorEditorGraphNode::_create_add_button_widgets()
{
    if (is_add_pin_button_visible())
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
        button->set_tooltip_text("Add a new pin");
        container->add_child(button);

        button->connect("pressed", callable_mp_this(_add_pin_requested));
    }
}

void OrchestratorEditorGraphNode::_add_pin_requested()
{
    ERR_FAIL_COND_MSG(!is_add_pin_button_visible(), "Cannot add a pin to a node that doesn't support dynamic pins");

    emit_signal("add_node_pin_requested", this);
}

void OrchestratorEditorGraphNode::_create_indicators()
{
    if (get_graph()->is_bookmarked(this))
        _add_indicator("Anchor", "This node is bookmarked");

    if (_node->get_flags().has_flag(OrchestrationGraphNode::DEVELOPMENT_ONLY))
        _add_indicator("Notification", "Node only executes during development builds and is excluded in export builds.");

    if (_node->get_flags().has_flag(OrchestrationGraphNode::EXPERIMENTAL))
        _add_indicator("NodeWarning", "Node is experimental and behavior may change without notice");

    #if GODOT_VERSION >= 0x040300
    if (get_graph()->is_breakpoint(this))
    {
        const bool breakpoint_enabled = get_graph()->get_breakpoint(this);
        const String suffix = !breakpoint_enabled ? "On" : "Off";
        const String tooltip_text = !breakpoint_enabled
            ? "Debugger will skip the breakpoint when processing this node."
            : "Debugger will break when processing this node.";

        _add_indicator("DebugSkipBreakpoints" + suffix, tooltip_text);
    }
    #endif
}

void OrchestratorEditorGraphNode::_add_indicator(const String& p_icon_name, const String& p_tooltip_text)
{
    ERR_FAIL_COND_MSG(p_icon_name.is_empty(), "Cannot create an indicator with no icon specified.");

    TextureRect* rect = memnew(TextureRect);
    rect->set_texture(SceneUtils::get_editor_icon(p_icon_name));
    rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
    rect->set_custom_minimum_size(Vector2(0, 20));
    rect->set_tooltip_text(SceneUtils::create_wrapped_tooltip_text(p_tooltip_text.strip_edges()));
    _indicators_hbox->add_child(rect);
}

void OrchestratorEditorGraphNode::_update_styles()
{
    const Ref<OrchestratorThemeCache> cache = OrchestratorEditor::get_singleton()->get_theme_cache();
    ERR_FAIL_COND_MSG(!cache.is_valid(), "Cannot apply graph themes, theme cache is invalid");;

    const String type_name = vformat("GraphNode_%s", _node->get_node_title_color_name());
    begin_bulk_theme_override();
    add_theme_stylebox_override("panel", cache->get_theme_stylebox("panel", "GraphNode"));
    add_theme_stylebox_override("panel_selected", cache->get_theme_stylebox("panel_selected", "GraphNode"));
    add_theme_stylebox_override("titlebar", cache->get_theme_stylebox("titlebar", type_name));
    add_theme_stylebox_override("titlebar_selected", cache->get_theme_stylebox("titlebar_selected", type_name));
    end_bulk_theme_override();
}

String OrchestratorEditorGraphNode::_get_tooltip_text()
{
    String tooltip_text = _node->get_node_title();

    if (!_node->get_tooltip_text().is_empty())
        tooltip_text = "\n\n" + _node->get_tooltip_text();

    if (_node->get_flags().has_flag(OrchestrationGraphNode::DEVELOPMENT_ONLY))
        tooltip_text += "\n\nNode only executes during development. Exported builds will not include this node.";

    if (_node->get_flags().has_flag(OrchestrationGraphNode::EXPERIMENTAL))
        tooltip_text += "\n\nThis node is experimental and may change in the future without warning.";

    String class_name = _node->get_class();
    if (class_name.begins_with("OScriptNode"))
        class_name = class_name.replace("OScriptNode", "").capitalize();

    tooltip_text += "\n\nID: " + itos(_node->get_id());
    tooltip_text += "\nClass: " + class_name;
    tooltip_text += "\nFlags: " + itos(_node->get_flags());

    const bool advanced_tooltips = ORCHESTRATOR_GET("ui/graph/show_advanced_tooltips", false);
    if (advanced_tooltips)
    {
        tooltip_text += "\n";
        tooltip_text += "\nUI Instance: " + itos(get_instance_id());
        tooltip_text += "\nModel Instance: " + itos(_node->get_instance_id());
    }

    return tooltip_text;
}

void OrchestratorEditorGraphNode::_draw_port2(int32_t p_slot_index, const Vector2i& p_position, bool p_left, const Color& p_color, const Color& p_rim_color)
{
    Ref<Texture2D> port_icon = p_left
        ? get_slot_custom_icon_left(p_slot_index)
        : get_slot_custom_icon_right(p_slot_index);

    if (port_icon.is_null())
        port_icon = get_theme_icon("port", "GraphNode");

    const int port_type = p_left
        ? get_slot_type_left(p_slot_index)
        : get_slot_type_right(p_slot_index);

    const Point2 icon_offset = -port_icon->get_size() * 0.5;

    // Draw "shadow" / outline in the connection rim color
    const float scale = port_type == 0 ? 1.f : EDSCALE;
    draw_texture_rect(port_icon, Rect2(p_position + (icon_offset - Size2(2, 2)) * scale, (port_icon->get_size() + Size2(4, 4)) * scale), false, p_rim_color);
    draw_texture_rect(port_icon, Rect2(p_position + icon_offset * scale, port_icon->get_size() * scale), false, p_color);
}

void OrchestratorEditorGraphNode::_gui_input(const Ref<InputEvent>& p_event)
{
    const Ref<InputEventMouseButton> mb = p_event;
    if (mb.is_valid() && mb->is_pressed())
    {
        if (mb->is_double_click() && mb->get_button_index() == MOUSE_BUTTON_LEFT)
            emit_signal("double_click_jump_request", this);

        else if (mb->get_button_index() == MOUSE_BUTTON_RIGHT)
            emit_signal("context_menu_requested", this, mb->get_position());
    }
}

void OrchestratorEditorGraphNode::_draw_port(int32_t p_slot_index, const Vector2i& p_position, bool p_left, const Color& p_color)
{
    const Color rim_color = get_theme_color("connection_rim_color", "GraphEdit");
    _draw_port2(p_slot_index, p_position, p_left, p_color, rim_color);
}

void OrchestratorEditorGraphNode::set_node(const Ref<OrchestrationGraphNode>& p_node)
{
    ERR_FAIL_COND_MSG(!p_node.is_valid(), "Trying to create an OrchestratorEditorGraphNode with an invalid model");

    _node = p_node;
    _node->connect("changed", callable_mp_this(update));

    // Serves to handle toggling pin default value visibility when pins are connected/disconnected
    _node->connect("pin_connected", callable_mp_this(_pin_connection_status_changed).bind(true));
    _node->connect("pin_disconnected", callable_mp_this(_pin_connection_status_changed).bind(false));

    // When the editor is opened in floating mode, the styles need to be applied deferred.
    // This is so the parenting of the editor to the window has been applied before the style lookup occurs
    callable_mp_this(_update_styles).call_deferred();
    update();
}

OrchestratorEditorGraphPanel* OrchestratorEditorGraphNode::get_graph() const
{
    return cast_to<OrchestratorEditorGraphPanel>(get_parent());
}

int OrchestratorEditorGraphNode::get_id() const
{
    return _node->get_id();
}

bool OrchestratorEditorGraphNode::can_user_delete_node() const
{
    return _node->can_user_delete_node();
}

bool OrchestratorEditorGraphNode::is_bookmarked() const
{
    return get_graph()->is_bookmarked(this);
}

bool OrchestratorEditorGraphNode::is_breakpoint() const
{
    return get_graph()->is_breakpoint(this);
}

bool OrchestratorEditorGraphNode::can_jump_to_definition() const
{
    return _node->can_jump_to_definition();
}

Object* OrchestratorEditorGraphNode::get_definition_object() const
{
    return _node->get_jump_target_for_double_click();
}

Vector<OrchestratorEditorGraphPin*> OrchestratorEditorGraphNode::get_pins() const
{
    Vector<OrchestratorEditorGraphPin*> result;
    for (const SlotMapKeyValue& E : _slots)
    {
        if (E.value.left)
            result.push_back(E.value.left);
        if (E.value.right)
            result.push_back(E.value.right);
    }
    return result;
}

Vector<OrchestratorEditorGraphPin*> OrchestratorEditorGraphNode::get_eligible_autowire_pins(OrchestratorEditorGraphPin* p_pin) const
{
    Vector<OrchestratorEditorGraphPin*> result;
    ERR_FAIL_NULL_V(p_pin, result);

    for (OrchestratorEditorGraphPin* pin : get_pins())
    {
        if (pin->is_hidden() || !pin->is_autowire_enabled() || pin->get_direction() == p_pin->get_direction())
            continue;

        if (pin->is_execution() != p_pin->is_execution())
            continue;

        if (!pin->is_execution() && !p_pin->is_execution())
        {
            const Variant::Type lhs_type = pin->get_property_info().type;
            const Variant::Type rhs_type = p_pin->get_property_info().type;
            const bool lhs_variant = PropertyUtils::is_variant(pin->get_property_info());
            const bool rhs_variant = PropertyUtils::is_variant(p_pin->get_property_info());
            if (lhs_type != rhs_type && !lhs_variant && !rhs_variant)
                continue;
        }

        result.push_back(pin);
    }
    return result;
}

OrchestratorEditorGraphPin* OrchestratorEditorGraphNode::get_input_pin(int32_t p_slot)
{
    for (const SlotMapKeyValue& E : _slots)
    {
        if (E.value.left && E.value.slot == p_slot)
            return E.value.left;
    }
    return nullptr;
}

OrchestratorEditorGraphPin* OrchestratorEditorGraphNode::get_output_pin(int32_t p_slot)
{
    for (const SlotMapKeyValue& E : _slots)
    {
        if (E.value.right && E.value.slot == p_slot)
            return E.value.right;
    }
    return nullptr;
}

OrchestratorEditorGraphPin* OrchestratorEditorGraphNode::get_pin(int32_t p_slot, EPinDirection p_direction)
{
    return p_direction == PD_Input ? get_input_pin(p_slot) : get_output_pin(p_slot);
}

int32_t OrchestratorEditorGraphNode::get_input_port_slot(const String& p_pin_name)
{
    for (const SlotMapKeyValue& E : _slots)
    {
        if (E.value.left && E.value.left->get_pin_name() == p_pin_name)
            return E.value.slot;
    }
    return -1;
}

int32_t OrchestratorEditorGraphNode::get_output_port_slot(const String& p_pin_name)
{
    for (const SlotMapKeyValue& E : _slots)
    {
        if (E.value.right && E.value.right->get_pin_name() == p_pin_name)
            return E.value.slot;
    }
    return -1;
}

int32_t OrchestratorEditorGraphNode::get_port_slot(const String& p_pin_name, EPinDirection p_direction)
{
    return p_direction == PD_Input ? get_input_port_slot(p_pin_name) : get_output_port_slot(p_pin_name);
}

int32_t OrchestratorEditorGraphNode::get_port_slot(int p_port, EPinDirection p_direction)
{
    return p_direction == PD_Input ? get_input_port_slot(p_port) : get_output_port_slot(p_port);
}

Vector2 OrchestratorEditorGraphNode::get_port_position_for_pin(OrchestratorEditorGraphPin* p_pin)
{
    ERR_FAIL_NULL_V_MSG(p_pin, Vector2(), "Pin object was null when getting port position for the pin");

    int32_t index = 0;
    for (const SlotMapKeyValue& E : _slots)
    {
        if (p_pin->get_direction() == PD_Input)
        {
            if (E.value.left == p_pin)
                return get_input_port_position(index);
            index++;
        }
        else
        {
            if (E.value.right == p_pin)
                return get_output_port_position(index);
            index++;
        }
    }
    return Vector2();
}

int32_t OrchestratorEditorGraphNode::get_port_at_position(const Vector2& p_position)
{
    const Vector2 position = p_position - get_position_offset();
    const Rect2 zone = Rect2(position - Vector2(1, 1), Vector2(2, 2)); // Fake hotsize

    const int32_t max_ports = MAX(get_input_port_count(), get_output_port_count());
    for (int32_t port = 0; port < max_ports; port++)
    {
        if (port < get_input_port_count() && zone.has_point(get_input_port_position(port)))
            return port;

        if (port < get_output_port_count() && zone.has_point(get_output_port_position(port)))
            return port;
    }

    return -1;
}

int32_t OrchestratorEditorGraphNode::get_slot_at_position(const Vector2& p_position)
{
    const Vector2 position = p_position - get_position_offset();
    const Rect2 zone = Rect2(position - Vector2(1, 1), Vector2(2, 2)); // Fake hotsize

    const int32_t max_ports = MAX(get_input_port_count(), get_output_port_count());
    for (int32_t port = 0; port < max_ports; port++)
    {
        if (port < get_input_port_count() && zone.has_point(get_input_port_position(port)))
            return GraphNode::get_input_port_slot(port);

        if (port < get_output_port_count() && zone.has_point(get_output_port_position(port)))
            return GraphNode::get_output_port_slot(port);
    }

    return -1;
}

int32_t OrchestratorEditorGraphNode::get_pin_port(OrchestratorEditorGraphPin* p_pin)
{
    if (p_pin->get_direction() == PD_Input)
    {
        for (int32_t port = 0; port < get_input_port_count(); port++)
        {
            int32_t slot_index = get_input_port_slot(port);
            if (_slots.has(slot_index) && _slots[slot_index].left == p_pin)
                return port;
        }
    }

    if (p_pin->get_direction() == PD_Output)
    {
        for (int32_t port = 0; port < get_output_port_count(); port++)
        {
            int32_t slot_index = get_output_port_slot(port);
            if (_slots.has(slot_index) && _slots[slot_index].right == p_pin)
                return port;
        }
    }

    return -1;
}

Vector<GraphElement*> OrchestratorEditorGraphNode::get_overlapping_elements() const
{
    Vector<GraphElement*> result;

    const Rect2 area = get_global_rect();
    get_graph()->for_each<GraphElement>([&] (GraphElement* element) {
        if (element && element != this && area.intersects(element->get_global_rect()))
            result.push_back(element);
    });

    return result;
}

Rect2 OrchestratorEditorGraphNode::get_graph_rect() const
{
    return Rect2(get_position_offset(), get_size());
}

bool OrchestratorEditorGraphNode::is_selected_exclusively() const
{
    const Vector<GraphElement*> selected_elements = get_graph()->get_selected<GraphElement>();
    return selected_elements.size() == 1 && selected_elements[0] == this;
}

bool OrchestratorEditorGraphNode::is_add_pin_button_visible() const
{
    const Ref<OScriptEditablePinNode> editable_node = _node;
    if (editable_node.is_valid())
        return editable_node->can_add_dynamic_pin();

    return false;
}

void OrchestratorEditorGraphNode::set_show_type_icons(bool p_show_type_icons)
{
    if (_show_type_icons != p_show_type_icons)
    {
        _show_type_icons = p_show_type_icons;
        _update_pin_widgets();
    }
}

void OrchestratorEditorGraphNode::set_show_advanced_tooltips(bool p_show_advanced_tooltips)
{
    if (_show_advanced_tooltips != p_show_advanced_tooltips)
    {
        _show_advanced_tooltips = p_show_advanced_tooltips;

        _update_pin_widgets();
        set_tooltip_text(SceneUtils::create_wrapped_tooltip_text(_get_tooltip_text()));
    }
}

void OrchestratorEditorGraphNode::set_slot_color_opacity(float p_opacity, EPinDirection p_direction)
{
    if (p_direction == PD_Input || p_direction == PD_MAX)
    {
        for (int i = 0; i < get_input_port_count(); i++)
        {
            if (is_slot_enabled_left(i))
                set_slot_color_left(i, Color(get_input_port_color(i), p_opacity));
        }
    }

    if (p_direction == PD_Output || p_direction == PD_MAX)
    {
        for (int i = 0; i < get_output_port_count(); i++)
        {
            if (is_slot_enabled_right(i))
                set_slot_color_right(i, Color(get_output_port_color(i), p_opacity));
        }
    }
}

void OrchestratorEditorGraphNode::update()
{
    // No need to update deleted nodes
    if (is_queued_for_deletion())
        return;

    if (ORCHESTRATOR_GET("ui/nodes/resize_to_content", false))
        _resize_to_content();

    _slots.clear();

    SAFE_REMOVE_CHILDREN(_indicators_hbox);
    SAFE_REMOVE_CHILDREN(this);

    clear_all_slots();

    _update_titlebar();
    _create_indicators();
    _create_pin_widgets();
    _create_add_button_widgets();

    if (get_position_offset() != _node->get_position())
        set_position_offset(_node->get_position());

    set_tooltip_text(SceneUtils::create_wrapped_tooltip_text(_get_tooltip_text()));
}

void OrchestratorEditorGraphNode::redraw_connections()
{
    for (uint32_t slot_index = 0; slot_index < _slots.size(); slot_index++)
    {
        const Slot& slot = _slots[slot_index];
        if (slot.left)
        {
            const OrchestratorEditorGraphPinSlotInfo slot_info = slot.left->get_slot_info();
            if (get_slot_type_left(slot.slot) != slot_info.type)
                set_slot_type_left(slot.slot, slot_info.type);
            if (get_slot_color_left(slot.slot) != slot_info.color)
                set_slot_color_left(slot.slot, slot_info.color);
        }

        if (slot.right)
        {
            const OrchestratorEditorGraphPinSlotInfo slot_info = slot.right->get_slot_info();
            if (get_slot_type_right(slot.slot) != slot_info.type)
                set_slot_type_right(slot.slot, slot_info.type);
            if (get_slot_color_right(slot.slot) != slot_info.color)
                set_slot_color_right(slot.slot, slot_info.color);
        }
    }
}

void OrchestratorEditorGraphNode::notify_connections_changed()
{
    // No-op - is there anything to do here?
}

void OrchestratorEditorGraphNode::notify_bookmarks_changed()
{
    SAFE_REMOVE_CHILDREN(_indicators_hbox);
    _create_indicators();
}

void OrchestratorEditorGraphNode::notify_breakpoints_changed()
{
    SAFE_REMOVE_CHILDREN(_indicators_hbox);
    _create_indicators();
}

void OrchestratorEditorGraphNode::_notification(int p_what)
{
}

void OrchestratorEditorGraphNode::_bind_methods()
{
    ADD_SIGNAL(MethodInfo("node_pins_changed", PropertyInfo(Variant::OBJECT, "node")));
    ADD_SIGNAL(MethodInfo("context_menu_requested", PropertyInfo(Variant::OBJECT, "node"), PropertyInfo(Variant::VECTOR2, "position")));
    ADD_SIGNAL(MethodInfo("double_click_jump_request", PropertyInfo(Variant::OBJECT, "node")));
    ADD_SIGNAL(MethodInfo("add_node_pin_requested", PropertyInfo(Variant::OBJECT, "node")));
}

OrchestratorEditorGraphNode::OrchestratorEditorGraphNode()
{
    // GraphNode titlebar_hbox is designed to hold exactly one component, a Label.
    // This Label is set with SIZE_EXPAND_FILL

    _indicators_hbox = memnew(HBoxContainer);
    _indicators_hbox->set_h_size_flags(SIZE_SHRINK_END);
    _indicators_hbox->set_v_size_flags(SIZE_SHRINK_CENTER);
    get_titlebar_hbox()->add_child(_indicators_hbox);

    connect("node_selected", callable_mp_this(_node_selected));
}

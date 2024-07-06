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
#include "graph_node_pin.h"

#include "common/callable_lambda.h"
#include "common/scene_utils.h"
#include "common/settings.h"
#include "common/variant_utils.h"
#include "graph_edit.h"
#include "graph_node.h"
#include "script/nodes/data/coercion_node.h"
#include "script/nodes/data/dictionary.h"
#include "script/nodes/editable_pin_node.h"
#include "script/nodes/functions/call_function.h"
#include "script/nodes/variables/variable_get.h"
#include "script/nodes/variables/variable_set.h"

#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

OrchestratorGraphNodePin::OrchestratorGraphNodePin(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin)
{
    // Data elements
    _node = p_node;
    _pin = p_pin;

    _update_tooltip();
}

void OrchestratorGraphNodePin::_bind_methods()
{
}

String OrchestratorGraphNodePin::_get_color_name() const
{
    String type_name = VariantUtils::get_friendly_type_name(get_value_type()).to_lower();
    if (type_name.match("nil"))
        type_name = "any";

    return vformat("ui/connection_colors/%s", type_name);
}

void OrchestratorGraphNodePin::_notification(int p_what)
{
    if (p_what == NOTIFICATION_READY)
    {
        _create_widgets();

        _context_menu = memnew(PopupMenu);
        _context_menu->connect("id_pressed", callable_mp(this, &OrchestratorGraphNodePin::_on_context_menu_selection));
        add_child(_context_menu);
    }
}

void OrchestratorGraphNodePin::_gui_input(const Ref<InputEvent>& p_event)
{
    Ref<InputEventMouseButton> event = p_event;
    if (event.is_null() || !event->is_pressed() || event->get_button_mask() != MOUSE_BUTTON_RIGHT)
        return;

    // Show menu
    _show_context_menu(event->get_position());

    // Accept event so it doesn't bubble up
    accept_event();
}

OrchestratorGraphEdit* OrchestratorGraphNodePin::get_graph()
{
    return _node->get_graph();
}

OrchestratorGraphNode* OrchestratorGraphNodePin::get_graph_node()
{
    return _node;
}

Color OrchestratorGraphNodePin::get_color() const
{
    OrchestratorSettings* os = OrchestratorSettings::get_singleton();

    const String color_name = _get_color_name();
    if (os->has_setting(color_name))
        return os->get_setting(color_name);

    // Use fallback
    return os->get_setting("ui/connection_colors/any");
}

int OrchestratorGraphNodePin::get_slot_type() const
{
    // By default, type is 1, for data pins.
    return 1;
}

String OrchestratorGraphNodePin::get_slot_icon_name() const
{
    // By default, type is GuiGraphNodePort
    return "GuiGraphNodePort";
}

ResolvedType OrchestratorGraphNodePin::resolve_type() const
{
    ResolvedType resolved_type;

    if (get_value_type() != Variant::NIL && get_value_type() != Variant::OBJECT)
    {
        resolved_type.type = get_value_type();
        return resolved_type;
    }

    if (!_pin->get_target_class().is_empty())
    {
        resolved_type.class_name = _pin->get_target_class();
    }
    else if (_node) // For Objects and NULL, attempt to ask the node
    {
        // When consulting the node, we need to traverse down to the actual script
        // node implementation as this tends to be specific for each node, so we
        // skip delegation to the OrchestratorGraphNode and instead jump right to the
        // ScriptNode and call it directly.
        resolved_type.class_name = _node->get_script_node()->resolve_type_class(_pin);
    }
    else
        resolved_type.class_name = "Object";

    // Primarily used by SceneNode to get scene attributes about the target
    resolved_type.object = _pin->resolve_target();

    return resolved_type;
}

bool OrchestratorGraphNodePin::is_input() const
{
    return _pin->is_input();
}

bool OrchestratorGraphNodePin::is_output() const
{
    return _pin->is_output();
}

bool OrchestratorGraphNodePin::is_connectable() const
{
    return _pin->is_connectable();
}

bool OrchestratorGraphNodePin::is_connected() const
{
    return _pin->has_any_connections();
}

bool OrchestratorGraphNodePin::is_hidden() const
{
    return _pin->is_hidden();
}

bool is_numeric(Variant::Type p_type)
{
    return p_type == Variant::INT || p_type == Variant::FLOAT;
}

bool OrchestratorGraphNodePin::can_accept(OrchestratorGraphNodePin* p_pin)
{
    return _pin->can_accept(p_pin->_pin);
}

void OrchestratorGraphNodePin::link(OrchestratorGraphNodePin* p_pin)
{
    _pin->link(p_pin->_pin);
}

void OrchestratorGraphNodePin::unlink(OrchestratorGraphNodePin* p_pin)
{
    _pin->unlink(p_pin->_pin);
}

void OrchestratorGraphNodePin::unlink_all()
{
    _pin->unlink_all();
}

bool OrchestratorGraphNodePin::is_coercion_required(OrchestratorGraphNodePin* p_other_pin) const
{
    if (is_execution() && p_other_pin->is_execution())
        return false;

    return get_value_type() != p_other_pin->get_value_type();
}

Variant::Type OrchestratorGraphNodePin::get_value_type() const
{
    return _pin->get_type();
}

Variant OrchestratorGraphNodePin::get_default_value() const
{
    return _pin->get_effective_default_value();
}

void OrchestratorGraphNodePin::set_default_value(const Variant& p_default_value)
{
    _pin->set_default_value(p_default_value);
}

void OrchestratorGraphNodePin::set_default_value_control_visibility(bool p_visible)
{
    if (_default_value)
        _default_value->set_visible(p_visible);
}

void OrchestratorGraphNodePin::show_icon(bool p_visible)
{
    if (_icon)
        _icon->set_visible(p_visible);
}

void OrchestratorGraphNodePin::_remove_editable_pin()
{
    Ref<OScriptEditablePinNode> editable = _node->get_script_node();
    if (editable.is_valid())
        editable->remove_dynamic_pin(_pin);

    Ref<OScriptNodeCallFunction> function_call = _node->get_script_node();
    if (function_call.is_valid() && function_call->is_vararg())
        function_call->remove_dynamic_pin(_pin);
}

void OrchestratorGraphNodePin::_promote_as_variable()
{
    Orchestration* orchestation = _node->get_script_node()->get_orchestration();

    Ref<OScriptVariable> variable = orchestation->create_variable(_create_promoted_variable_name(), _pin->get_type());
    if (!variable.is_valid())
        return;

    variable->set_default_value(_pin->get_effective_default_value());

    OScriptNodeInitContext context;
    context.variable_name = variable->get_variable_name();

    Vector2 offset = Vector2(200, 25);
    Vector2 position = _node->get_script_node()->get_position();

    if (is_input())
    {
        position -= offset;
        get_graph()->spawn_node<OScriptNodeVariableGet>(context, position,
            callable_mp_lambda(this, [&, this](const Ref<OScriptNodeVariableGet>& p_node) {
                p_node->find_pin(0, PD_Output)->link(_pin);
            }));
    }
    else
    {
        position += offset + Vector2(25, 0);
        get_graph()->spawn_node<OScriptNodeVariableSet>(context, position,
            callable_mp_lambda(this, [&, this](const Ref<OScriptNodeVariableSet>& p_node) {
                _pin->link(p_node->find_pin(1, PD_Input));
            }));
    }
}

String OrchestratorGraphNodePin::_create_promoted_variable_name()
{
    Orchestration* orchestration = _node->get_script_node()->get_orchestration();

    int index = 0;
    String name = _pin->get_pin_name() + itos(index++);
    while (orchestration->has_variable(name))
        name = _pin->get_pin_name() + itos(index++);

    return name;
}

void OrchestratorGraphNodePin::_create_widgets()
{
    _default_value = nullptr;

    set_h_size_flags(SIZE_FILL);
    set_v_size_flags(SIZE_SHRINK_CENTER);
    set_alignment(ALIGNMENT_CENTER);

    OrchestratorSettings* os = OrchestratorSettings::get_singleton();
    bool show_icons = os->get_setting("ui/nodes/show_type_icons");

    if (is_input())
    {
        if (_render_default_value_below_label())
        {
            VBoxContainer* vbox = memnew(VBoxContainer);
            add_child(vbox);

            HBoxContainer* row0 = memnew(HBoxContainer);
            vbox->add_child(row0);

            if (!is_execution())
                row0->add_child(_create_type_icon(true));

            Label* label = _create_label();
            label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_LEFT);
            label->set_h_size_flags(SIZE_FILL);
            label->set_v_size_flags(SIZE_SHRINK_CENTER);
            row0->add_child(label);

            if (!is_execution() && !_pin->is_default_ignored())
            {
                _default_value = _get_default_value_widget();
                if (_default_value)
                {
                    _default_value->set_visible(!_pin->has_any_connections());
                    vbox->add_child(_default_value);
                }
            }
        }
        else
        {
            if (!is_execution())
                add_child(_create_type_icon(show_icons));

            Label* label = _create_label();
            label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_LEFT);
            label->set_h_size_flags(SIZE_FILL);
            label->set_v_size_flags(SIZE_SHRINK_CENTER);
            add_child(label);

            if (!is_execution() && !_pin->is_default_ignored())
            {
                _default_value = _get_default_value_widget();
                if (_default_value)
                {
                    _default_value->set_visible(!_pin->has_any_connections());
                    add_child(_default_value);
                }
            }
        }
    }
    else
    {
        Label* label = _create_label();
        label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
        label->set_h_size_flags(SIZE_FILL);
        label->set_v_size_flags(SIZE_SHRINK_CENTER);
        add_child(label);

        if (!is_execution())
            add_child(_create_type_icon(show_icons));
    }
}

TextureRect* OrchestratorGraphNodePin::_create_type_icon(bool p_visible)
{
    _icon = memnew(TextureRect);
    String value_type_name = _pin->get_pin_type_name();
    _icon->set_texture(SceneUtils::get_editor_icon(value_type_name));
    _icon->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);

    if (_pin->is_hidden() || !p_visible)
        _icon->set_visible(false);

    return _icon;
}

Label* OrchestratorGraphNodePin::_create_label()
{
    Label* label = memnew(Label);

    if (_pin->is_label_visible())
    {
        String text = _pin->get_label();
        if (text.is_empty())
            text = _pin->get_pin_name();

        if (_pin->use_pretty_labels())
            text = text.capitalize();

        label->set_text(text);
    }
    else
    {
        label->set_custom_minimum_size(Vector2(50, 0));
    }

    return label;
}

void OrchestratorGraphNodePin::_update_tooltip()
{
    const String label = _pin->get_label();
    const String tooltip = "";
    Variant::Type pin_type = _pin->get_type();
    if (!is_execution())
    {
        String tooltip_text = label.capitalize();
        tooltip_text += "\n" + VariantUtils::get_friendly_type_name(pin_type, true).capitalize();

        if (!tooltip.is_empty())
            tooltip_text += "\n\n" + tooltip.capitalize();

        set_tooltip_text(tooltip_text);
    }
}

void OrchestratorGraphNodePin::_populate_graph_node_in_sub_menu(int p_id, const String& p_prefix, PopupMenu* p_menu,
                                                          const Vector<Ref<OScriptNodePin>>& p_pins)
{
    for (int i = 0; i < p_pins.size(); i++)
    {
        const Ref<OScriptNodePin>& pin = p_pins[i];

        const int node_id = pin->get_owning_node()->get_id();
        if (GraphNode* gn = Object::cast_to<GraphNode>(get_graph()->find_child(itos(node_id), true, false)))
        {
            String title = vformat("%s %s", p_prefix, gn->get_title().strip_edges());
            if (title.is_empty())
            {
                Ref<OScriptNodeCoercion> node = get_graph_node()->get_script_node();
                if (node.is_valid())
                    title = node->get_tooltip_text();
            }
            p_menu->add_item(title, p_id + i);
            p_menu->set_item_metadata(p_menu->get_item_index(p_id + i), i);
        }
    }
}

void OrchestratorGraphNodePin::_show_context_menu(const Vector2& p_position)
{
    // When showing the context-menu, if the current node is not selected, we should clear the
    // selection and the operation will only be applicable for this node and its pin.
    if (!_node->is_selected())
    {
        get_graph()->clear_selection();
        _node->set_selected(true);
    }

    _context_menu->clear();

    // clear submenus
    while (_context_menu->get_child_count() > 0)
    {
        Node* child = _context_menu->get_child(0);
        _context_menu->remove_child(child);
        memdelete(child);
    }

    // Pin Actions
    _context_menu->add_separator("Pin Actions");

    Ref<OScriptNode> owner_node = get_graph_node()->get_script_node();
    bool has_connections = _pin->has_any_connections();

    if (has_connections && is_execution())
    {
        String text = vformat("Select all %s nodes", is_input() ? "Input" : "Output");
        _context_menu->add_item(text, CM_SELECT_NODES);
    }

    Ref<OScriptEditablePinNode> editable = owner_node;
    bool editable_pin_removable = editable.is_valid() && editable->can_remove_dynamic_pin(_pin);

    Ref<OScriptNodeCallFunction> function_call = owner_node;
    bool function_call_pin_removable = function_call.is_valid() && function_call->can_remove_dynamic_pin(_pin);

    if (editable_pin_removable || function_call_pin_removable)
    {
        String text = "Remove pin";

        Ref<OScriptNodeMakeDictionary> make_dict = owner_node;
        if (make_dict.is_valid())
            text = "Remove key/value pair";

        _context_menu->add_item(text, CM_REMOVE);
    }

    if (owner_node->can_change_pin_type())
    {
        const Vector<Variant::Type> options = owner_node->get_possible_pin_types();
        if (!options.is_empty())
        {
            PopupMenu* sub_menu = memnew(PopupMenu);
            sub_menu->set_name("change_pin_type_options");
            sub_menu->connect("id_pressed", callable_mp(this, &OrchestratorGraphNodePin::_on_context_menu_change_pin_type));

            for (int i = 0; i < options.size(); i++)
            {
                const String type = VariantUtils::get_friendly_type_name(options[i]).capitalize();
                sub_menu->add_item(type, CM_CHANGE_PIN_TYPE + i);
                sub_menu->set_item_metadata(sub_menu->get_item_index(CM_CHANGE_PIN_TYPE + i), options[i]);
            }

            _context_menu->add_child(sub_menu);
            _context_menu->add_submenu_item("Change Pin Type", sub_menu->get_name(), CM_CHANGE_PIN_TYPE);
        }
    }

    Vector<Ref<OScriptNodePin>> connections = _pin->get_connections();
    if (connections.size() <= 1)
    {
        _context_menu->add_icon_item(SceneUtils::get_editor_icon("Unlinked"), "Break This Link", CM_BREAK_LINKS);
        _context_menu->set_item_disabled(_context_menu->get_item_index(CM_BREAK_LINKS), !has_connections);
    }
    else
    {
        _context_menu->add_icon_item(SceneUtils::get_editor_icon("Unlinked"), "Break All Pin Links", CM_BREAK_LINKS);

        PopupMenu* sub_menu = memnew(PopupMenu);
        sub_menu->set_name("break_pin");
        sub_menu->connect("id_pressed", callable_mp(this, &OrchestratorGraphNodePin::_on_context_menu_break_pin));
        _populate_graph_node_in_sub_menu(CM_BREAK_LINK, "Break Pin Link to", sub_menu, connections);
        _context_menu->add_child(sub_menu);
        _context_menu->add_submenu_item("Break Link to...", sub_menu->get_name(), CM_BREAK_LINK);
    }

    if (has_connections)
    {
        PopupMenu* sub_menu = memnew(PopupMenu);
        sub_menu->set_name("node_jump");
        sub_menu->connect("id_pressed", callable_mp(this, &OrchestratorGraphNodePin::_on_context_menu_jump_node));
        _populate_graph_node_in_sub_menu(CM_JUMP_NODE, "Jump to", sub_menu, connections);
        _context_menu->add_child(sub_menu);
        _context_menu->add_submenu_item("Jump to connected node...", sub_menu->get_name(), CM_JUMP_NODE);
    }

    // todo: add pin decomposition/recombination feature

    if (_can_promote_to_variable())
        _context_menu->add_item("Promote to Variable", CM_PROMOTE_TO_VARIABLE);

    if (!is_execution() && !has_connections && is_connectable() && is_input())
        _context_menu->add_item("Reset to Default Value", CM_RESET_TO_DEFAULT);

    // Documentation
    _context_menu->add_separator("Documentation");
    _context_menu->add_icon_item(SceneUtils::get_editor_icon("Help"), "View Documentation", CM_VIEW_DOCUMENTATION);

    _context_menu->set_position(get_screen_position() + (p_position * (real_t) get_graph()->get_zoom()));
    _context_menu->reset_size();
    _context_menu->popup();
}

void OrchestratorGraphNodePin::_select_nodes_for_pin(const Ref<OScriptNodePin>& p_pin)
{
    Vector<Ref<OScriptNodePin>> connections = get_graph()->get_orchestration()->get_connections(p_pin.ptr());
    for (const Ref<OScriptNodePin>& connection : connections)
    {
        Ref<OScriptNode> node = connection->get_owning_node();
        if (node.is_valid())
        {
            if (Node* child = get_graph()->find_child(itos(node->get_id()), true, false))
                _select_nodes_for_pin(p_pin, (OrchestratorGraphNode*) child);
        }
    }
}

void OrchestratorGraphNodePin::_select_nodes_for_pin(const Ref<OScriptNodePin>& p_pin, OrchestratorGraphNode* p_node)
{
    p_node->set_selected(true);
    for (const Ref<OScriptNodePin>& node_pin : p_node->get_script_node()->get_all_pins())
    {
        if (node_pin->is_execution() && node_pin->get_direction() == p_pin->get_direction())
            _select_nodes_for_pin(node_pin);
    }
}

Variant OrchestratorGraphNodePin::_get_context_sub_menu_item_metadata(int p_menu_id, int p_id)
{
    const String menu_name = _context_menu->get_item_submenu(_context_menu->get_item_index(p_menu_id));

    PopupMenu* menu = Object::cast_to<PopupMenu>(_context_menu->find_child(menu_name, true, false));
    return menu ? menu->get_item_metadata(menu->get_item_index(p_id)) : Variant();
}

Ref<OScriptNodePin> OrchestratorGraphNodePin::_get_connected_pin_by_sub_menu_metadata(int p_menu_id, int p_id)
{
    Variant metadata = _get_context_sub_menu_item_metadata(p_menu_id, p_id);
    if (metadata.get_type() == Variant::INT)
    {
        const int index = metadata;
        Vector<Ref<OScriptNodePin>> connections = _pin->get_connections();
        if (index >= 0 && index < connections.size())
            return connections[index];
    }
    return {};
}

void OrchestratorGraphNodePin::_on_context_menu_selection(int p_id)
{
    // Handle others
    switch (p_id)
    {
        case CM_SELECT_NODES:
        {
            _select_nodes_for_pin(_pin);
            break;
        }
        case CM_BREAK_LINKS:
        {
            _pin->unlink_all(true);
            break;
        }
        case CM_RESET_TO_DEFAULT:
        {
            // force node redraw
            _pin->set_default_value(_pin->get_generated_default_value());
            _node->get_script_node()->emit_changed();
            break;
        }
        case CM_VIEW_DOCUMENTATION:
        {
            get_graph()->goto_class_help(get_graph_node()->get_script_node()->get_help_topic());
            break;
        }
        case CM_REMOVE:
        {
            _remove_editable_pin();
            break;
        }
        case CM_PROMOTE_TO_VARIABLE:
        {
            if (_can_promote_to_variable())
                _promote_as_variable();
            break;
        }
        default:
            // no-op
            break;
    }
}

void OrchestratorGraphNodePin::_on_context_menu_change_pin_type(int p_id)
{
    Variant metadata = _get_context_sub_menu_item_metadata(CM_CHANGE_PIN_TYPE, p_id);
    if (metadata.get_type() == Variant::INT)
    {
        const int type = metadata;
        get_graph_node()->get_script_node()->change_pin_types(VariantUtils::to_type(type));
    }
}

void OrchestratorGraphNodePin::_on_context_menu_break_pin(int p_id)
{
    const Ref<OScriptNodePin> connection = _get_connected_pin_by_sub_menu_metadata(CM_BREAK_LINK, p_id);
    if (connection.is_valid())
        _pin->unlink(connection);
}

void OrchestratorGraphNodePin::_on_context_menu_jump_node(int p_id)
{
    const Ref<OScriptNodePin> connection = _get_connected_pin_by_sub_menu_metadata(CM_JUMP_NODE, p_id);
    if (connection.is_valid())
    {
        const int node_id = connection->get_owning_node()->get_id();
        get_graph()->focus_node(node_id);
    }
}
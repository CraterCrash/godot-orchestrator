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
#include "graph_edit.h"

#include "common/dictionary_utils.h"
#include "common/logger.h"
#include "editor/graph/factories/graph_node_factory.h"
#include "editor/graph/graph_node_pin.h"
#include "editor/graph/graph_node_spawner.h"
#include "editor/graph/nodes/graph_node_comment.h"
#include "plugin/plugin.h"
#include "script/language.h"
#include "script/nodes/script_nodes.h"
#include "script/script.h"

#include <godot_cpp/classes/center_container.hpp>
#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/method_tweener.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/script_editor.hpp>
#include <godot_cpp/classes/tween.hpp>

OrchestratorGraphEdit::Clipboard* OrchestratorGraphEdit::_clipboard = nullptr;

OrchestratorGraphEdit::DragContext::DragContext()
    : node_name("")
    , node_port(-1)
    , output_port(false)
    , dragging(false)
{

}

void OrchestratorGraphEdit::DragContext::reset()
{
    node_name = "";
    node_port = -1;
    output_port = false;
    dragging = false;
}

void OrchestratorGraphEdit::DragContext::start_drag(const StringName& p_name, int p_from_port, bool p_output)
{
    node_name = p_name;
    node_port = p_from_port;
    output_port = p_output;
    dragging = true;
}

void OrchestratorGraphEdit::DragContext::end_drag()
{
    dragging = false;
}

bool OrchestratorGraphEdit::DragContext::should_autowire() const
{
    return !node_name.is_empty();
}

EPinDirection OrchestratorGraphEdit::DragContext::get_direction() const
{
    return output_port ? PD_Output : PD_Input;
}

OrchestratorGraphEdit::OrchestratorGraphEdit(OrchestratorPlugin* p_plugin, Ref<OScript> p_script, const String& p_name)
{
    set_name(p_name);
    set_minimap_enabled(false);
    set_right_disconnects(true);
    set_show_arrange_button(false);

    _plugin = p_plugin;

    _script = p_script;
    _script_graph = _script->get_graph(p_name);

    // Add action menu
    _action_menu = memnew(OrchestratorGraphActionMenu(this));
    add_child(_action_menu);

    set_zoom(_script_graph->get_viewport_zoom());
    set_scroll_offset(_script_graph->get_viewport_offset());
    set_show_zoom_label(true);

    _context_menu = memnew(PopupMenu);
    add_child(_context_menu);
}

void OrchestratorGraphEdit::initialize_clipboard()
{
    if (!_clipboard)
        _clipboard = memnew(Clipboard);
}

void OrchestratorGraphEdit::free_clipboard()
{
    if (_clipboard)
    {
        memdelete(_clipboard);
        _clipboard = nullptr;
    }
}

void OrchestratorGraphEdit::_notification(int p_what)
{
    if (p_what == NOTIFICATION_READY)
    {
        _drag_hint = memnew(Label);
        _drag_hint->set_anchor_and_offset(SIDE_TOP, ANCHOR_END, 0);
        _drag_hint->set_anchor_and_offset(SIDE_BOTTOM, ANCHOR_END, -50);
        _drag_hint->set_anchor_and_offset(SIDE_RIGHT, ANCHOR_END, 0);
        _drag_hint->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
        _drag_hint->set_vertical_alignment(VERTICAL_ALIGNMENT_BOTTOM);
        add_child(_drag_hint);

        Label* label = memnew(Label);
        label->set_text("Use Right Mouse Button To Add New Nodes");
        label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
        label->add_theme_font_size_override("font_size", 24);

        _status = memnew(CenterContainer);
        _status->set_anchors_preset(Control::PRESET_FULL_RECT);
        _status->add_child(label);
        add_child(_status);

        // When graph has nodes, hide right-click suggestion
        if (!_script_graph->get_nodes().is_empty())
            _status->hide();

        _drag_hint_timer = memnew(Timer);
        _drag_hint_timer->set_wait_time(5);
        _drag_hint_timer->connect("timeout", callable_mp(this, &OrchestratorGraphEdit::_hide_drag_hint));
        add_child(_drag_hint_timer);

        Button* show_script_details = memnew(Button);
        show_script_details->set_text("Script Details");
        show_script_details->set_tooltip_text("Shows script details in the Inspector");
        show_script_details->set_focus_mode(Control::FOCUS_NONE);
        show_script_details->connect("pressed", callable_mp(this, &OrchestratorGraphEdit::_on_inspect_script));
        get_menu_hbox()->add_child(show_script_details);

        Button* validate_and_build = memnew(Button);
        validate_and_build->set_text("Validate");
        validate_and_build->set_tooltip_text("Validates the script for errors");
        validate_and_build->set_focus_mode(Control::FOCUS_NONE);
        validate_and_build->connect("pressed", callable_mp(this, &OrchestratorGraphEdit::_on_validate_and_build));
        get_menu_hbox()->add_child(validate_and_build);

        _confirm_window = memnew(ConfirmationDialog);
        _confirm_window->set_hide_on_ok(true);
        _confirm_window->get_cancel_button()->hide();
        _confirm_window->set_initial_position(Window::WINDOW_INITIAL_POSITION_CENTER_SCREEN_WITH_MOUSE_FOCUS);
        add_child(_confirm_window);

        _script->connect("connections_changed", callable_mp(this, &OrchestratorGraphEdit::_on_graph_connections_changed));
        _script_graph->connect("node_added", callable_mp(this, &OrchestratorGraphEdit::_on_graph_node_added));
        _script_graph->connect("node_removed", callable_mp(this, &OrchestratorGraphEdit::_on_graph_node_removed));

        // Wire up action menu
        _action_menu->connect("canceled", callable_mp(this, &OrchestratorGraphEdit::_on_action_menu_cancelled));
        _action_menu->connect("action_selected", callable_mp(this, &OrchestratorGraphEdit::_on_action_menu_action_selected));

        // Wire up our signals
        connect("connection_from_empty", callable_mp(this, &OrchestratorGraphEdit::_on_attempt_connection_from_empty));
        connect("connection_to_empty", callable_mp(this, &OrchestratorGraphEdit::_on_attempt_connection_to_empty));
        connect("connection_request", callable_mp(this, &OrchestratorGraphEdit::_on_connection));
        connect("disconnection_request", callable_mp(this, &OrchestratorGraphEdit::_on_disconnection));
        connect("popup_request", callable_mp(this, &OrchestratorGraphEdit::_on_right_mouse_clicked));
        connect("node_selected", callable_mp(this, &OrchestratorGraphEdit::_on_node_selected));
        connect("node_deselected", callable_mp(this, &OrchestratorGraphEdit::_on_node_deselected));
        connect("delete_nodes_request", callable_mp(this, &OrchestratorGraphEdit::_on_delete_nodes_requested));
        connect("connection_drag_started", callable_mp(this, &OrchestratorGraphEdit::_on_connection_drag_started));
        connect("connection_drag_ended", callable_mp(this, &OrchestratorGraphEdit::_on_connection_drag_ended));
        connect("copy_nodes_request", callable_mp(this, &OrchestratorGraphEdit::_on_copy_nodes_request));
        connect("duplicate_nodes_request", callable_mp(this, &OrchestratorGraphEdit::_on_duplicate_nodes_request));
        connect("paste_nodes_request", callable_mp(this, &OrchestratorGraphEdit::_on_paste_nodes_request));

        _context_menu->connect("id_pressed", callable_mp(this, &OrchestratorGraphEdit::_on_context_menu_selection));

        ProjectSettings* ps = ProjectSettings::get_singleton();
        ps->connect("settings_changed", callable_mp(this, &OrchestratorGraphEdit::_on_project_settings_changed));

        _synchronize_graph_with_script(_deferred_tween_node == -1);
        _focus_node(_deferred_tween_node);
    }
}

void OrchestratorGraphEdit::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("_synchronize_child_order"), &OrchestratorGraphEdit::_synchronize_child_order);

    ADD_SIGNAL(MethodInfo("nodes_changed"));
    ADD_SIGNAL(MethodInfo("focus_requested", PropertyInfo(Variant::OBJECT, "target")));
}

void OrchestratorGraphEdit::clear_selection()
{
    set_selected(nullptr);

    for_each_graph_node([](OrchestratorGraphNode* node) {
        if (node->is_selected())
            node->set_selected(false);
    });
}

void OrchestratorGraphEdit::focus_node(int p_node_id)
{
    if (is_inside_tree() && is_node_ready())
        _focus_node(p_node_id);
    else
        _deferred_tween_node = p_node_id;
}

void OrchestratorGraphEdit::request_focus(Object* p_object)
{
    emit_signal("focus_requested", p_object);
}

void OrchestratorGraphEdit::apply_changes()
{
    // During save update the graph-specific data points
    _script_graph->set_viewport_offset(get_scroll_offset());
    _script_graph->set_viewport_zoom(get_zoom());
}

void OrchestratorGraphEdit::post_apply_changes()
{
}

void OrchestratorGraphEdit::spawn_node(const Ref<OScriptNode>& p_node, const Vector2& p_position)
{
    Logger::debug("Spawning node ", p_node->get_class(), " at ", p_position);
    p_node->set_position(p_position);

    // Node initialized, add to script
    _script->add_node(_script_graph, p_node);

    // Notify node has been placed
    p_node->post_placed_new_node();

    // Based on the connection drag details, attempt to auto-connect
    if (_drag_context.should_autowire())
    {
        Ref<OScriptNode> other_node = _script->get_node(_drag_context.node_name.to_int());
        if (other_node.is_valid())
        {
            Logger::debug("Auto-wire placed node ", p_node->get_class(), " with ", other_node->get_class());
            _attempt_autowire(p_node, other_node);
        }
    }

    _drag_context.reset();
    emit_signal("nodes_changed");
}

void OrchestratorGraphEdit::goto_class_help(const String& p_class_name)
{
    _plugin->get_editor_interface()->set_main_screen_editor("Script");
    _plugin->get_editor_interface()->get_script_editor()->call("_help_class_open", p_class_name);
}

void OrchestratorGraphEdit::for_each_graph_node(std::function<void(OrchestratorGraphNode*)> p_func)
{
    for (int i = 0; i < get_child_count(); i++)
    {
        if (OrchestratorGraphNode* node = Object::cast_to<OrchestratorGraphNode>(get_child(i)))
            p_func(node);
    }
}

bool OrchestratorGraphEdit::_can_drop_data(const Vector2& p_position, const Variant& p_data) const
{
    if (p_data.get_type() != Variant::DICTIONARY)
        return false;

    Dictionary data = p_data;
    if (!data.has("type"))
        return false;

    const String type = data["type"];

    PackedStringArray allowed_types;
    allowed_types.push_back("nodes");
    allowed_types.push_back("files");
    allowed_types.push_back("obj_property");
    allowed_types.push_back("variable");
    allowed_types.push_back("signal");
    allowed_types.push_back("function");

    if (allowed_types.has(type))
    {
        if (type == "variable")
            _show_drag_hint("Hint: Use Ctrl to drop a Setter, Shift to drop a Getter");

        return true;
    }

    return false;
}

void OrchestratorGraphEdit::_drop_data(const Vector2& p_position, const Variant& p_data)
{
    OScriptLanguage* language = OScriptLanguage::get_singleton();
    Dictionary data = p_data;

    _update_saved_mouse_position(p_position);

    const String type = data["type"];
    if (type == "nodes")
    {
        const Array nodes = data["nodes"];
        if (Node* root = get_tree()->get_edited_scene_root())
        {
            if (Node* dropped_node = root->get_node_or_null(nodes[0]))
            {
                NodePath path = root->get_path_to(dropped_node);
                Ref<OScriptNodeSceneNode> node = language->create_node_from_type<OScriptNodeSceneNode>(_script);

                OScriptNodeInitContext context;
                context.node_path = path;
                node->initialize(context);

                spawn_node(node, _saved_mouse_position);
            }
        }
    }
    else if (type == "files")
    {
        const Array files = data["files"];

        // Create context-menu to specify variable get or set choice
        _context_menu->clear();
        _context_menu->add_separator("File " + String(files[0]));
        _context_menu->add_item("Get Path", CM_FILE_GET_PATH);
        _context_menu->add_item("Preload ", CM_FILE_PRELOAD);
        _context_menu->set_item_metadata(_context_menu->get_item_index(CM_FILE_GET_PATH), String(files[0]));
        _context_menu->set_item_metadata(_context_menu->get_item_index(CM_FILE_PRELOAD), String(files[0]));
        _context_menu->reset_size();
        _context_menu->set_position(get_screen_position() + p_position);
        _context_menu->popup();
    }
    else if (type == "obj_property")
    {
        const Object* object = data["object"];
        const StringName property_name = data["property"];
        const TypedArray<Dictionary> properties = object->get_property_list();

        NodePath path;
        if (Node* root = get_tree()->get_edited_scene_root())
            if (const Node* object_node = Object::cast_to<Node>(object))
                path = root->get_path_to(const_cast<Node*>(object_node));

        PropertyInfo pi;
        bool found = false;
        for (int i = 0; i < properties.size(); i++)
        {
            pi = DictionaryUtils::to_property(properties[i]);
            if (pi.name.match(property_name))
            {
                found = true;
                break;
            }
        }

        if (found)
        {
            // Get the property's current value and seed that as the pin's value
            const Variant value = object->get(property_name);

            // Create context-menu handlers
            Ref<OrchestratorGraphActionHandler> get_handler(memnew(OrchestratorGraphNodeSpawnerPropertyGet(pi, path)));
            Ref<OrchestratorGraphActionHandler> set_handler(memnew(OrchestratorGraphNodeSpawnerPropertySet(pi, path, value)));

            // Create context-menu to specify variable get or set choice
            _context_menu->clear();
            _context_menu->add_separator("Property " + pi.name);
            _context_menu->add_item("Get " + pi.name, CM_PROPERTY_GET);
            _context_menu->add_item("Set " + pi.name, CM_PROPERTY_SET);
            _context_menu->set_item_metadata(_context_menu->get_item_index(CM_PROPERTY_GET), get_handler);
            _context_menu->set_item_metadata(_context_menu->get_item_index(CM_PROPERTY_SET), set_handler);
            _context_menu->reset_size();
            _context_menu->set_position(get_screen_position() + p_position);
            _context_menu->popup();
        }
    }
    else if (type == "function")
    {
        const StringName function_name = StringName(Array(data["functions"])[0]);
        const Ref<OScriptFunction> function = _script->find_function(function_name);
        if (function.is_valid())
        {
            Ref<OScriptNodeCallFunction> node = language->create_node_from_type<OScriptNodeCallScriptFunction>(_script);

            OScriptNodeInitContext context;
            context.method = function->get_method_info();
            node->initialize(context);

            spawn_node(node, _saved_mouse_position);
        }
    }
    else if (type == "variable")
    {
        _hide_drag_hint();

        const String variable_name = String(Array(data["variables"])[0]);
        if (Input::get_singleton()->is_key_pressed(Key::KEY_CTRL))
        {
            OrchestratorGraphNodeSpawnerVariableSet setter(variable_name);
            setter.execute(this, _saved_mouse_position);
        }
        else if (Input::get_singleton()->is_key_pressed(Key::KEY_SHIFT))
        {
            OrchestratorGraphNodeSpawnerVariableGet getter(variable_name);
            getter.execute(this, _saved_mouse_position);
        }
        else
        {
            // Create context-menu handlers
            Ref<OrchestratorGraphActionHandler> get_handler(memnew(OrchestratorGraphNodeSpawnerVariableGet(variable_name)));
            Ref<OrchestratorGraphActionHandler> set_handler(memnew(OrchestratorGraphNodeSpawnerVariableSet(variable_name)));

            // Create context-menu to specify variable get or set choice
            _context_menu->clear();
            _context_menu->add_separator("Variable " + variable_name);
            _context_menu->add_item("Get " + variable_name, CM_VARIABLE_GET);
            _context_menu->add_item("Set " + variable_name, CM_VARIABLE_SET);
            _context_menu->set_item_metadata(_context_menu->get_item_index(CM_VARIABLE_GET), get_handler);
            _context_menu->set_item_metadata(_context_menu->get_item_index(CM_VARIABLE_SET), set_handler);
            _context_menu->reset_size();
            _context_menu->set_position(get_screen_position() + p_position);
            _context_menu->popup();
        }
    }
    else if (type == "signal")
    {
        const String signal_name = String(Array(data["signals"])[0]);
        const Ref<OScriptSignal> signal = _script->get_custom_signal(signal_name);
        if (signal.is_valid())
        {
            Ref<OScriptNodeEmitSignal> node = language->create_node_from_type<OScriptNodeEmitSignal>(_script);

            OScriptNodeInitContext context;
            context.method = signal->get_method_info();
            node->initialize(context);

            spawn_node(node, _saved_mouse_position);
        }
    }
}

void OrchestratorGraphEdit::_focus_node(int p_node_id, bool p_animated)
{
    if (p_node_id >= 0)
    {
        if (OrchestratorGraphNode* node = _get_node_by_id(p_node_id))
        {
            // Clear and re-select the node
            clear_selection();
            node->set_selected(true);

            // Calculate position
            Vector2 position = (node->get_position_offset() * get_zoom()) - (get_viewport_rect().get_center() / 2);
            if (!p_animated)
            {
                set_scroll_offset(position);
                return;
            }

            const float duration = 0.2f;
            {
                Ref<Tween> tween = get_tree()->create_tween();
                tween->tween_method(Callable(this, "set_scroll_offset"), get_scroll_offset(), position, duration);
                tween->set_ease(Tween::EASE_OUT_IN);
                tween->play();
            }
            {
                Ref<Tween> tween = get_tree()->create_tween();
                tween->tween_method(Callable(this, "set_zoom"), get_zoom(), 1.f, duration);
                tween->set_ease(Tween::EASE_OUT_IN);
                tween->play();
            }
        }
    }
}

bool OrchestratorGraphEdit::_is_node_hover_valid(const StringName& p_from, int p_from_port, const StringName& p_to, int p_to_port)
{
    if (OrchestratorGraphNode* source = _get_node_by_name(p_from))
    {
        if (OrchestratorGraphNode* target = _get_node_by_name(p_to))
        {
            OrchestratorGraphNodePin* source_pin = source->get_output_pin(p_from_port);
            OrchestratorGraphNodePin* target_pin = target->get_input_pin(p_to_port);

            return target_pin->can_accept(source_pin);
        }
    }
    return false;
}

OrchestratorGraphNode* OrchestratorGraphEdit::_get_node_by_id(int p_id)
{
    return _get_node_by_name(itos(p_id));
}

OrchestratorGraphNode* OrchestratorGraphEdit::_get_node_by_name(const StringName& p_name)
{
    return Object::cast_to<OrchestratorGraphNode>(get_node_or_null(NodePath(p_name)));
}

void OrchestratorGraphEdit::_remove_all_nodes()
{
    // Remove all nodes from the graph.
    List<GraphNode*> removables;
    for_each_graph_node([&removables](OrchestratorGraphNode* node) {
        removables.push_back(node);
    });

    for (GraphNode* node : removables)
    {
        remove_child(node);
        node->queue_free();
    }
}

void OrchestratorGraphEdit::_synchronize_graph_with_script(bool p_apply_position)
{
    _remove_all_nodes();

    Array nodes = _script_graph->get_nodes();
    for (int i = 0; i < nodes.size(); i++)
        _synchronize_graph_node(_script->get_node(nodes[i]));

    _synchronize_graph_connections_with_script();

    call_deferred("_synchronize_child_order");

    if (p_apply_position)
    {
        // These must be deferred, don't change.
        call_deferred("set_zoom", _script_graph->get_viewport_zoom());
        call_deferred("set_scroll_offset", _script_graph->get_viewport_offset());
    }
}

void OrchestratorGraphEdit::_synchronize_graph_connections_with_script()
{
    // Remove all connections
    clear_connections();

    // Re-assign connections
    Array nodes = _script_graph->get_nodes();
    for (const OScriptConnection& E : _script->get_connections())
    {
        // Only apply nodes that are part of this graph
        if (!(nodes.has(E.from_node) || nodes.has(E.to_node)))
            continue;

        connect_node(itos(E.from_node), E.from_port, itos(E.to_node), E.to_port);
    }
}

void OrchestratorGraphEdit::_synchronize_graph_node(Ref<OScriptNode> p_node)
{
    const String node_id = itos(p_node->get_id());

    if (has_node(node_id))
    {
        WARN_PRINT("_synchronize_graph_node cannot update existing nodes; likely a bug!?");
        return;
    }

    const Vector2 node_size = p_node->get_size();

    OrchestratorGraphNode* graph_node = OrchestratorGraphNodeFactory::create_node(this, p_node);
    graph_node->set_title(p_node->get_node_title());
    graph_node->set_position_offset(p_node->get_position());
    graph_node->set_size(node_size.is_zero_approx() ? graph_node->get_size() : node_size);
    add_child(graph_node);
}

void OrchestratorGraphEdit::_synchronize_child_order()
{
    // Always place comment nodes above the nodes that are contained within their rects.
    for_each_graph_node([&](OrchestratorGraphNode* node) {
        if (OrchestratorGraphNodeComment* comment = Object::cast_to<OrchestratorGraphNodeComment>(node))
        {
            // Raises the child
            move_child(comment, -1);
            comment->call_deferred("raise_request_node_reorder");
        }
    });
}

void OrchestratorGraphEdit::_attempt_autowire(const Ref<OScriptNode>& p_new_node, const Ref<OScriptNode>& p_existing_node)
{
    // User dragging a connection either an input or output port
    Ref<OScriptNodePin> existing_pin = p_existing_node->find_pin(_drag_context.node_port, _drag_context.get_direction());
    if (!existing_pin.is_valid())
        return;

    for (const Ref<OScriptNodePin>& pin : p_new_node->get_all_pins())
    {
        // Invalid/hidden pins are skipped
        if (!pin.is_valid() || pin->get_flags().has_flag(OScriptNodePin::Flags::HIDDEN))
            continue;

        // If pin direction matches drag, skip
        if (_drag_context.get_direction() == pin->get_direction())
            continue;

        // If the existing pin is an execution, only match against executions
        if (existing_pin->is_execution() && !pin->is_execution())
            continue;

        // If the existing pin is an data port, only match data ports
        if (!existing_pin->is_execution() && pin->is_execution())
            continue;

        if (!existing_pin->is_execution() && !pin->is_execution())
        {
            // For data flows, match types
            if (existing_pin->get_type() != pin->get_type())
                continue;
        }

        // For OScriptNodePin the link must be done where the argument pin is the input.
        if (_drag_context.output_port)
            existing_pin->link(pin);
        else
            pin->link(existing_pin);

        p_new_node->post_node_autowired(p_existing_node, _drag_context.get_direction());
        break;
    }
}

void OrchestratorGraphEdit::_show_action_menu(const Vector2& p_position, const OrchestratorGraphActionFilter& p_filter)
{
    _update_saved_mouse_position(p_position);

    _action_menu->set_position(get_screen_position() + p_position);
    _action_menu->apply_filter(p_filter);
}

void OrchestratorGraphEdit::_update_saved_mouse_position(const Vector2& p_position)
{
    _saved_mouse_position = (p_position + get_scroll_offset()) / get_zoom();
}

bool OrchestratorGraphEdit::_can_duplicate_node(OrchestratorGraphNode* p_node) const
{
    Ref<OScriptNode> node = p_node->get_script_node();
    Ref<OScriptNodeEvent> event = node;
    Ref<OScriptNodeFunctionEntry> function_entry = node;
    Ref<OScriptNodeFunctionResult> function_result = node;
    Ref<OScriptNodeLocalVariable> local_variable = node;
    return !event.is_valid() && !function_entry.is_valid() && !function_result.is_valid() && !local_variable.is_valid();
}

void OrchestratorGraphEdit::_show_drag_hint(const godot::String& p_message) const
{
    _drag_hint->set_text(p_message);
    _drag_hint->show();
    _drag_hint_timer->start();
}

void OrchestratorGraphEdit::_hide_drag_hint()
{
    _drag_hint->hide();
}

void OrchestratorGraphEdit::_on_connection_drag_started(const StringName& p_from, int p_from_port, bool p_output)
{
    _drag_context.start_drag(p_from, p_from_port, p_output);

    if (OrchestratorGraphNode* source = _get_node_by_name(p_from))
    {
        if (p_output)
        {
            // From port is an output
            OrchestratorGraphNodePin* pin = source->get_output_pin(p_from_port);
            for_each_graph_node([&](OrchestratorGraphNode* node) {
                node->set_inputs_for_accept_opacity(0.3f, pin);
                node->set_all_outputs_opacity(0.3f);

                if (node->get_inputs_with_opacity() == 0 && node != source)
                    node->set_modulate(Color(1, 1, 1, 0.5));
            });
        }
        else
        {
            // From port is an input
            OrchestratorGraphNodePin* pin = source->get_input_pin(p_from_port);
            for_each_graph_node([&](OrchestratorGraphNode* node) {
                node->set_all_inputs_opacity(0.3f);
                node->set_outputs_for_accept_opacity(0.3f, pin);

                if (node->get_outputs_with_opacity() == 0 && node != source)
                    node->set_modulate(Color(1, 1, 1, 0.5));
            });
        }
    }
}

void OrchestratorGraphEdit::_on_connection_drag_ended()
{
    _drag_context.end_drag();

    for_each_graph_node([](OrchestratorGraphNode* node) {
        node->set_modulate(Color(1, 1, 1, 1));
        node->set_all_inputs_opacity(1.f);
        node->set_all_outputs_opacity(1.f);
    });
}

void OrchestratorGraphEdit::_on_action_menu_cancelled()
{
    _drag_context.reset();
}

void OrchestratorGraphEdit::_on_action_menu_action_selected(OrchestratorGraphActionHandler* p_handler)
{
    // When the user has not used the right mouse button in the graph area, any created
    // node, i.e. overrides, should be placed in the center of the existing viewport
    // rather than at 0,0 which may be out of visible range of the current area.
    if (_saved_mouse_position.is_equal_approx(Vector2()))
        _saved_mouse_position = get_global_rect().get_center();

    p_handler->execute(this, _saved_mouse_position);
}

void OrchestratorGraphEdit::_on_connection(const StringName& p_from_node, int p_from_port, const StringName& p_to_node,
                                 int p_to_port)
{
    _drag_context.reset();

    if (OrchestratorGraphNode* source = _get_node_by_name(p_from_node))
    {
        if (OrchestratorGraphNode* target = _get_node_by_name(p_to_node))
        {
            OrchestratorGraphNodePin* source_pin = source->get_output_pin(p_from_port);
            OrchestratorGraphNodePin* target_pin = target->get_input_pin(p_to_port);
            if (!source_pin || !target_pin)
                return;

            // Connect the two pins
            source_pin->link(target_pin);
        }
    }
}

void OrchestratorGraphEdit::_on_disconnection(const StringName& p_from_node, int p_from_port, const StringName& p_to_node,
                                    int p_to_port)
{
    if (OrchestratorGraphNode* source = _get_node_by_name(p_from_node))
    {
        if (OrchestratorGraphNode* target = _get_node_by_name(p_to_node))
        {
            OrchestratorGraphNodePin* source_pin = source->get_output_pin(p_from_port);
            OrchestratorGraphNodePin* target_pin = target->get_input_pin(p_to_port);
            if (!source_pin || !target_pin)
                return;

            // Disconnect the two pins
            source_pin->unlink(target_pin);
        }
    }
}

void OrchestratorGraphEdit::_on_attempt_connection_from_empty(const StringName& p_to_node, int p_to_port, const Vector2& p_position)
{
    OrchestratorGraphNode* node = _get_node_by_name(p_to_node);
    ERR_FAIL_COND_MSG(!node, "Unable to find graph node with name " + p_to_node);

    OrchestratorGraphNodePin* pin = node->get_input_pin(p_to_port);
    if (!pin)
        return;

    OrchestratorGraphActionFilter filter;
    filter.context.graph = this;
    filter.context.pins.push_back(pin);

    ResolvedType resolved_type = pin->resolve_type();
    filter.target_type = resolved_type.type;

    if (resolved_type.object)
    {
        // When a resolved object isp provided, it takes precedence
        filter.target_classes.push_back(resolved_type.object->get_class());
        filter.target_object = resolved_type.object;
        filter.context.pins.clear();
    }
    if (resolved_type.is_class_type())
    {
        filter.target_classes.push_back(resolved_type.class_name);
        filter.context.pins.clear();
    }

    _show_action_menu(p_position, filter);
}

void OrchestratorGraphEdit::_on_attempt_connection_to_empty(const StringName& p_from_node, int p_from_port,
                                                  const Vector2& p_position)
{
    OrchestratorGraphNode* node = _get_node_by_name(p_from_node);
    ERR_FAIL_COND_MSG(!node, "Unable to find graph node with name " + p_from_node);

    OrchestratorGraphNodePin* pin = node->get_output_pin(p_from_port);
    if (!pin)
        return;

    OrchestratorGraphActionFilter filter;
    filter.context.graph = this;
    filter.context.pins.push_back(pin);

    ResolvedType resolved_type = pin->resolve_type();
    filter.target_type = resolved_type.type;

    if (resolved_type.object)
    {
        // When a resolved object is provided, it takes precedence
        filter.target_classes.push_back(resolved_type.object->get_class());
        filter.target_object = resolved_type.object;
        filter.context.pins.clear();
    }
    else if (resolved_type.is_class_type())
    {
        filter.target_classes.push_back(resolved_type.class_name);
        filter.context.pins.clear();
    }

    _show_action_menu(p_position, filter);
}

void OrchestratorGraphEdit::_on_node_selected(Node* p_node)
{
    if (!p_node)
        return;

    OrchestratorGraphNode* graph_node = Object::cast_to<OrchestratorGraphNode>(p_node);
    if (!graph_node)
        return;

    Ref<OScriptNode> node = p_node->get_meta("__script_node");
    if (node.is_null())
        return;

    if (!node->can_inspect_node_properties())
        return;

    // NOTE:
    // If the InspectorDock creates an empty copy of an object initially, this is
    // because EditorPropertyRevert::get_property_revert_value checks whether the
    // object implements the "property_can_revert" method.
    //
    // If the object passed to the InspectorDock does not implement that method,
    // the Editor will use PropertyUtils to create a temporary instance of the
    // object to resolve whether the object has any property default values so
    // it can properly revert values accordingly with the rollback button.
    //
    _plugin->get_editor_interface()->edit_resource(node->get_inspect_object());
}

void OrchestratorGraphEdit::_on_node_deselected(Node* p_node)
{
    _plugin->get_editor_interface()->inspect_object(nullptr);
}

void OrchestratorGraphEdit::_on_delete_nodes_requested(const PackedStringArray& p_node_names)
{
    for (const String& node_name : p_node_names)
    {
        if (OrchestratorGraphNode* node = _get_node_by_name(node_name))
        {
            if (!node->get_script_node()->can_user_delete_node())
            {
                const String message = "Node %d with the title '%s' cannot be deleted.\n"
                    "It may be that this node represents a function entry or some other node type that requires "
                    "deletion via the component menu instead.";

                _confirm_window->set_initial_position(godot::Window::WINDOW_INITIAL_POSITION_CENTER_SCREEN_WITH_MOUSE_FOCUS);
                _confirm_window->set_text(vformat(message, node->get_script_node_id(), node->get_title().strip_edges()));
                _confirm_window->set_title("Delete canceled");
                _confirm_window->reset_size();
                _confirm_window->show();
                return;
            }
        }
    }

    for (const String& node_name : p_node_names)
    {
        OrchestratorGraphNode* node = _get_node_by_name(node_name);
        if (!node)
        {
            ERR_PRINT("Cannot find node with name " + node_name + " to delete");
            continue;
        }

        if (node->is_selected())
            node->set_selected(false);

        _script->remove_node(node->get_script_node_id());
        node->queue_free();
    }

    if (!p_node_names.is_empty())
        emit_signal("nodes_changed");
}

void OrchestratorGraphEdit::_on_right_mouse_clicked(const Vector2& p_position)
{
    OrchestratorGraphActionFilter filter;
    filter.context.graph = this;

    _show_action_menu(p_position, filter);
}

void OrchestratorGraphEdit::_on_graph_node_added(int p_node_id)
{
    Ref<OScriptNode> node = _script->get_node(p_node_id);
    _synchronize_graph_node(node);

    // When node is added to graph, show right-click suggestion
    _status->hide();
}

void OrchestratorGraphEdit::_on_graph_node_removed(int p_node_id)
{
    if (OrchestratorGraphNode* node = _get_node_by_id(p_node_id))
    {
        remove_child(node);
        node->queue_free();
    }
    _synchronize_graph_connections_with_script();

    // When last node is removed from graph, show right-click suggestion
    if (_script_graph->get_nodes().is_empty())
        _status->show();
}

void OrchestratorGraphEdit::_on_graph_connections_changed(const String& p_caller)
{
    _synchronize_graph_connections_with_script();
}

void OrchestratorGraphEdit::_on_context_menu_selection(int p_id)
{
    switch (p_id)
    {
        case CM_VARIABLE_GET:
        case CM_VARIABLE_SET:
        case CM_PROPERTY_GET:
        case CM_PROPERTY_SET:
        {
            int index = _context_menu->get_item_index(p_id);
            Ref<OrchestratorGraphActionHandler> handler = _context_menu->get_item_metadata(index);
            if (handler.is_valid())
                handler->execute(this, _saved_mouse_position);
            break;
        }
        case CM_FILE_GET_PATH:
        case CM_FILE_PRELOAD:
        {
            const int index = _context_menu->get_item_index(p_id);
            const String file = _context_menu->get_item_metadata(index);

            OScriptLanguage* language = OScriptLanguage::get_singleton();

            Ref<OScriptNode> node;
            if (p_id == CM_FILE_GET_PATH)
                node = language->create_node_from_type<OScriptNodeResourcePath>(_script);
            else if (p_id == CM_FILE_PRELOAD)
                node = language->create_node_from_type<OScriptNodePreload>(_script);

            if (node.is_valid())
            {
                OScriptNodeInitContext context;
                context.resource_path = file;
                node->initialize(context);
                spawn_node(node, _saved_mouse_position);
            }
            break;
        }
        default:
            // no-op
            break;
    }
}

void OrchestratorGraphEdit::_on_project_settings_changed()
{
    _synchronize_graph_with_script();
}

void OrchestratorGraphEdit::_on_inspect_script()
{
    _plugin->get_editor_interface()->inspect_object(_script.ptr());
}

void OrchestratorGraphEdit::_on_validate_and_build()
{
    if(!_script->validate_and_build())
    {
        _confirm_window->set_text("There are build failures, please check the output.");
        _confirm_window->set_title("Validation Failed");
    }
    else
    {
        _confirm_window->set_text("Orchestration validation successful.");
        _confirm_window->set_title("Validation OK");
    }
    _confirm_window->reset_size();
    _confirm_window->show();
}

void OrchestratorGraphEdit::_on_copy_nodes_request()
{
    _clipboard->reset();

    for_each_graph_node([&](OrchestratorGraphNode* node) {
        if (node->is_selected())
        {
            Ref<OScriptNode> script_node = node->get_script_node();

            if (!_can_duplicate_node(node))
            {
                WARN_PRINT_ONCE_ED("There are some nodes that cannot be copied, they were not placed on the clipboard.");
                return;
            }

            const int id = script_node->get_id();
            _clipboard->positions[id] = script_node->get_position();
            // Be sure to clone to create new copies of pins
            _clipboard->nodes[id] = script_node->duplicate(true);
        }
    });

    for (const OScriptConnection& E : _script->get_connections())
        if (_clipboard->nodes.has(E.from_node) && _clipboard->nodes.has(E.to_node))
            _clipboard->connections.insert(E);
}

void OrchestratorGraphEdit::_on_duplicate_nodes_request()
{
    Vector<int> duplications;
    for_each_graph_node([&](OrchestratorGraphNode* node) {
        if (node->is_selected())
        {
            if (!_can_duplicate_node(node))
            {
                WARN_PRINT_ONCE_ED("There are some nodes that cannot be copied, they were not placed on the clipboard.");
                return;
            }
            duplications.push_back(node->get_script_node_id());
        }
    });

    if (duplications.is_empty())
        return;

    Vector<int> selections;
    HashMap<int, int> bindings;
    for (const int node_id : duplications)
    {
        Ref<OScriptNode> node = _script->get_node(node_id);

        // Duplicate sub-resources to handle copying pins
        Ref<OScriptNode> duplicate = node->duplicate(true);

        // Initialize the duplicate node
        // While the ID and Position are obvious, the other two maybe are not.
        // Setting OScript is necessary because the OScript pointer is not stored/persisted.
        // Additionally, there are other node properties that are maybe references to other objects or data
        // that is post-processed after placement but before rendering, and this allows this step to handle
        // that cleanly.
        duplicate->set_id(_script->get_available_id());
        duplicate->set_position(node->get_position() + Vector2(20, 20));
        duplicate->set_owning_script(_script.ptr());
        duplicate->post_initialize();

        _script->add_node(_script_graph, duplicate);
        duplicate->post_placed_new_node();

        selections.push_back(duplicate->get_id());
        bindings[node->get_id()] = duplicate->get_id();
    }

    for (const OScriptConnection& E : _script->get_connections())
        if (duplications.has(E.from_node) && duplications.has(E.to_node))
            _script->connect_nodes(bindings[E.from_node], E.from_port, bindings[E.to_node], E.to_port);

    _synchronize_graph_with_script();

    for (const int selected_id : selections)
    {
        if (OrchestratorGraphNode* node = _get_node_by_id(selected_id))
            node->set_selected(true);
    }
}

void OrchestratorGraphEdit::_on_paste_nodes_request()
{
    if (_clipboard->is_empty())
        return;

    Vector<int> duplications;
    RBSet<Vector2> existing_positions;
    for (const KeyValue<int, Vector2>& E : _clipboard->positions)
    {
        existing_positions.insert(E.value.snapped(Vector2(2, 2)));
        duplications.push_back(E.key);
    }

    Vector2 mouse_up_position = get_screen_position() + get_local_mouse_position();
    Vector2 position_offset = (get_scroll_offset() + (mouse_up_position - get_global_position())) / get_zoom();
    if (is_snapping_enabled())
    {
        int snap = get_snapping_distance();
        position_offset = position_offset.snapped(Vector2(snap, snap));
    }

    for (const KeyValue<int, Ref<OScriptNode>>& E : _clipboard->nodes)
    {
        position_offset -= _clipboard->positions[E.key];
        break;
    }

    Vector<int> selections;
    HashMap<int, int> bindings;
    for (const KeyValue<int, Ref<OScriptNode>>& E : _clipboard->nodes)
    {
        Ref<OScriptNode> node = E.value->duplicate();

        node->set_id(_script->get_available_id());
        node->set_position(_clipboard->positions[E.key] + position_offset);
        node->set_owning_script(_script.ptr());
        node->post_initialize();

        _script->add_node(_script_graph, node);

        node->post_placed_new_node();
        selections.push_back(node->get_id());

        bindings[E.key] = node->get_id();
    }

    for (const OScriptConnection& E : _clipboard->connections)
        _script->connect_nodes(bindings[E.from_node], E.from_port, bindings[E.to_node], E.to_port);

    _synchronize_graph_with_script();

    for (const int selected_id : selections)
    {
        if (OrchestratorGraphNode* node = _get_node_by_id(selected_id))
            node->set_selected(true);
    }
}
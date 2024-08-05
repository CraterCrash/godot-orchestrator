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

#include "autowire_selections.h"
#include "common/callable_lambda.h"
#include "common/dictionary_utils.h"
#include "common/logger.h"
#include "common/name_utils.h"
#include "common/scene_utils.h"
#include "common/settings.h"
#include "common/string_utils.h"
#include "common/version.h"
#include "editor/graph/graph_knot.h"
#include "editor/graph/graph_node_pin.h"
#include "editor/graph/graph_node_spawner.h"
#include "editor/graph/nodes/graph_node_comment.h"
#include "editor/plugins/orchestrator_editor_plugin.h"
#include "nodes/graph_node_factory.h"
#include "script/language.h"
#include "script/nodes/script_nodes.h"
#include "script/script.h"
#include "script/script_server.h"

#include <godot_cpp/classes/center_container.hpp>
#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/editor_inspector.hpp>
#include <godot_cpp/classes/geometry2d.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_action.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/method_tweener.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/script_editor.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/tween.hpp>
#include <godot_cpp/classes/v_separator.hpp>

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

OrchestratorGraphEdit::OrchestratorGraphEdit(OrchestratorPlugin* p_plugin, const Ref<OScriptGraph>& p_graph)
{
    internal::gdextension_interface_get_godot_version(&_version);
    _is_43p = _version.major == 4 && _version.minor >= 3;

    set_name(p_graph->get_graph_name());
    set_minimap_enabled(OrchestratorSettings::get_singleton()->get_setting("ui/graph/show_minimap", false));
    set_show_arrange_button(OrchestratorSettings::get_singleton()->get_setting("ui/graph/show_arrange_button", false));
    set_right_disconnects(true);

    _plugin = p_plugin;
    _script_graph = p_graph;

    _cache_connection_knots();

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
        _update_theme();

        get_menu_hbox()->add_child(memnew(VSeparator));
        get_menu_hbox()->move_child(get_menu_hbox()->get_child(-1), 4);

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

        _theme_update_timer = memnew(Timer);
        _theme_update_timer->set_wait_time(.5);
        _theme_update_timer->set_one_shot(true);
        add_child(_theme_update_timer);

        #if GODOT_VERSION >= 0x040300
        _grid_pattern = memnew(OptionButton);
        _grid_pattern->add_item("Lines");
        _grid_pattern->set_item_metadata(0, GRID_PATTERN_LINES);
        _grid_pattern->add_item("Dots");
        _grid_pattern->set_item_metadata(1, GRID_PATTERN_DOTS);
        _grid_pattern->connect("item_selected", callable_mp(this, &OrchestratorGraphEdit::_on_grid_style_selected));
        get_menu_hbox()->add_child(_grid_pattern);
        get_menu_hbox()->move_child(_grid_pattern, 5);

        set_grid_pattern(GRID_PATTERN_LINES);
        #endif

        get_menu_hbox()->add_child(memnew(VSeparator));

        _base_type_button = memnew(Button);
        _base_type_button->set_tooltip_text("Adjust the base type of the orchestration");
        _base_type_button->set_focus_mode(FOCUS_NONE);
        _base_type_button->connect("pressed", callable_mp(this, &OrchestratorGraphEdit::_on_inspect_script));
        _on_script_changed();
        get_menu_hbox()->add_child(_base_type_button);

        Button* validate_and_build = memnew(Button);
        validate_and_build->set_text("Validate");
        validate_and_build->set_button_icon(SceneUtils::get_editor_icon("TransitionSyncAuto"));
        validate_and_build->set_tooltip_text("Validates the script for errors");
        validate_and_build->set_focus_mode(FOCUS_NONE);
        validate_and_build->connect("pressed", callable_mp(this, &OrchestratorGraphEdit::_on_validate_and_build));
        get_menu_hbox()->add_child(validate_and_build);

        const Ref<Resource> self = get_orchestration()->get_self();
        self->connect("connections_changed", callable_mp(this, &OrchestratorGraphEdit::_on_graph_connections_changed));
        self->connect("changed", callable_mp(this, &OrchestratorGraphEdit::_on_script_changed));

        _script_graph->connect("node_added", callable_mp(this, &OrchestratorGraphEdit::_on_graph_node_added));
        _script_graph->connect("node_removed", callable_mp(this, &OrchestratorGraphEdit::_on_graph_node_removed));
        _script_graph->connect("knots_updated", callable_mp(this, &OrchestratorGraphEdit::_synchronize_graph_knots));
        _script_graph->connect("connection_knots_removed", callable_mp(this, &OrchestratorGraphEdit::_remove_connection_knots));

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
        callable_mp(this, &OrchestratorGraphEdit::_synchronize_graph_knots).call_deferred();
    }
    else if (p_what == NOTIFICATION_THEME_CHANGED)
    {
        if (PanelContainer* pc = Object::cast_to<PanelContainer>(get_menu_hbox()->get_parent()))
        {
            // Refreshes the panel changes on theme adjustments
            Ref<StyleBoxFlat> hbox_panel = pc->get_theme_stylebox("panel")->duplicate();
            hbox_panel->set_shadow_size(1);
            hbox_panel->set_shadow_offset(Vector2(2.f, 2.f));
            hbox_panel->set_bg_color(hbox_panel->get_bg_color() + Color(0, 0, 0, .3));
            hbox_panel->set_border_width(SIDE_LEFT, 1);
            hbox_panel->set_border_width(SIDE_TOP, 1);
            hbox_panel->set_border_color(hbox_panel->get_shadow_color());
            pc->add_theme_stylebox_override("panel", hbox_panel);
        }
    }
}

void OrchestratorGraphEdit::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("_synchronize_child_order"), &OrchestratorGraphEdit::_synchronize_child_order);

    ADD_SIGNAL(MethodInfo("nodes_changed"));
    ADD_SIGNAL(MethodInfo("focus_requested", PropertyInfo(Variant::OBJECT, "target")));
    ADD_SIGNAL(MethodInfo("collapse_selected_to_function"));
    ADD_SIGNAL(MethodInfo("expand_node", PropertyInfo(Variant::INT, "node_id")));
    ADD_SIGNAL(MethodInfo("validation_requested"));
}

void OrchestratorGraphEdit::clear_selection()
{
    set_selected(nullptr);

    for_each_graph_node([](OrchestratorGraphNode* node) {
        if (node->is_selected())
            node->set_selected(false);
    });
}

Vector<OrchestratorGraphNode*> OrchestratorGraphEdit::get_selected_nodes()
{
    Vector<OrchestratorGraphNode*> selected;
    for_each_graph_node([&](OrchestratorGraphNode* node) {
        if (node->is_selected())
            selected.push_back(node);
    });
    return selected;
}

Vector<Ref<OScriptNode>> OrchestratorGraphEdit::get_selected_script_nodes()
{
    Vector<Ref<OScriptNode>> selected;
    for_each_graph_node([&](OrchestratorGraphNode* node) {
        if (node->is_selected())
            selected.push_back(node->get_script_node());
    });
    return selected;
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
    _store_connection_knots();
}

void OrchestratorGraphEdit::post_apply_changes()
{
}

void OrchestratorGraphEdit::set_spawn_position_center_view()
{
    _saved_mouse_position = (get_scroll_offset() + get_rect().get_center()) / get_zoom();
}

void OrchestratorGraphEdit::goto_class_help(const String& p_class_name)
{
    #if GODOT_VERSION >= 0x040300
    _plugin->get_editor_interface()->get_script_editor()->goto_help(p_class_name);
    #else
    _plugin->get_editor_interface()->set_main_screen_editor("Script");
    _plugin->get_editor_interface()->get_script_editor()->call("_help_class_open", p_class_name);
    #endif
}

void OrchestratorGraphEdit::for_each_graph_node(std::function<void(OrchestratorGraphNode*)> p_func)
{
    for (int i = 0; i < get_child_count(); i++)
    {
        if (OrchestratorGraphNode* node = Object::cast_to<OrchestratorGraphNode>(get_child(i)))
            p_func(node);
    }
}

void OrchestratorGraphEdit::execute_action(const String& p_action_name)
{
    Ref<InputEventAction> action = memnew(InputEventAction);
    action->set_action(p_action_name);
    action->set_pressed(true);

    Input::get_singleton()->parse_input_event(action);
}

void OrchestratorGraphEdit::spawn_node(const StringName& p_type, const OScriptNodeInitContext& p_context, const Vector2& p_position, const Callable& p_callback)
{
    const Ref<OScriptNode> spawned = _script_graph->create_node(p_type, p_context, p_position);
    if (spawned.is_valid())
    {
        if (_drag_context.should_autowire())
        {
            const Ref<OScriptNode> other = _script_graph->get_node(_drag_context.node_name.to_int());
            if (other.is_valid())
            {
                const Ref<OScriptNodePin> pin = other->find_pin(_drag_context.node_port, _drag_context.get_direction());
                _queue_autowire(pin, spawned, p_callback);
            }
        }
        else
        {
            _complete_spawn(spawned, p_callback);
        }
    }
}

void OrchestratorGraphEdit::sync()
{
    _synchronize_graph_connections_with_script();
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
    else if (d >= 1.0f)
        return p_segment[1]; // After first point.
    else
        return p_segment[0] + n * d; // Inside.
}

static float get_distance_to_segment(const Vector2& p_point, const Vector2* p_segment)
{
    return p_point.distance_to(get_closest_point_to_segment(p_point, p_segment));
}

Dictionary OrchestratorGraphEdit::get_closest_connection_at_point(const Vector2& p_position, float p_max_distance)
{
    Vector2 transformed_point = p_position + get_scroll_offset();

    Dictionary closest_connection;
    float closest_distance = p_max_distance;

    TypedArray<Dictionary> connections = get_connection_list();
    for (int i = 0; i < connections.size(); i++)
    {
        const Dictionary& connection = connections[i];

        OrchestratorGraphNode* source = _get_by_name<OrchestratorGraphNode>(String(connection["from_node"]));
        if (!source)
            continue;

        OrchestratorGraphNode* target = _get_by_name<OrchestratorGraphNode>(String(connection["to_node"]));
        if (!target)
            continue;

        // What is cached
        Vector2 from_pos = source->get_output_port_position(int32_t(connection["from_port"])) + source->get_position_offset();
        Vector2 to_pos = target->get_input_port_position(int32_t(connection["to_port"])) + target->get_position_offset();

        if (_is_43p)
        {
            from_pos *= get_zoom();
            to_pos *= get_zoom();
        }

        // This function is called during both draw and this logic, and so the results need to be handled
        // differently based on the context of the call in Godot 4.2.
        PackedVector2Array points = get_connection_line(from_pos, to_pos);
        if (points.is_empty())
            continue;

        if (!_is_43p)
        {
            for (int j = 0; j < points.size(); j++)
                points[j] *= get_zoom();
        }

        Rect2 aabb(points[0], Vector2());
        for (int j = 0; j < points.size(); j++)
            aabb = aabb.expand(points[j]);
        aabb.grow_by(get_connection_lines_thickness() * 0.5);

        if (aabb.distance_to(transformed_point) > p_max_distance)
            continue;

        for (int j = 0; j < points.size(); j++)
        {
            float distance = get_distance_to_segment(transformed_point, &points[j]);
            if (distance <= get_connection_lines_thickness() * 0.5 + p_max_distance && distance < closest_distance)
            {
                closest_distance = distance;
                closest_connection = connection;
            }
        }
    }
    return closest_connection;
}
#endif

void OrchestratorGraphEdit::_gui_input(const Ref<InputEvent>& p_event)
{
    // In Godot 4.2, the UI delete events only apply to GraphNode and not GraphElement objects
    if (!_is_43p)
    {
        if (p_event->is_pressed() && p_event->is_action_pressed("ui_graph_delete", true))
        {
            TypedArray<StringName> nodes;
            for (int i = 0; i < get_child_count(); i++)
            {
                OrchestratorGraphKnot* knot = Object::cast_to<OrchestratorGraphKnot>(get_child(i));
                if (!knot || !knot->is_selected())
                    continue;

                nodes.push_back(knot->get_name());
            }
            emit_signal("delete_nodes_request", nodes);
        }
    }

    // todo:
    // There is a bug where if the mouse hovers a connection and a node concurrently,
    // the connection color is changed, even if the mouse is inside the node.
    GraphEdit::_gui_input(p_event);

    // This is to avoid triggering the display text or our internal hover_connection logic.
    Ref<InputEventMouse> me = p_event;
    if (me.is_valid() && !_is_position_within_node_rect(me->get_position()))
    {
        Ref<InputEventMouseMotion> mm = p_event;
        if (mm.is_valid())
        {
            _hovered_connection = get_closest_connection_at_point(mm->get_position());
            if (!_hovered_connection.is_empty())
            {
                _show_drag_hint("Use Ctrl+LMB to add a knot to the connection.\n"
                    "Hover over an existing knot and pressing Ctrl+LMB will remove it.");
            }
        }

        Ref<InputEventMouseButton> mb = p_event;
        if (mb.is_valid())
        {
            if (mb->get_button_index() == MOUSE_BUTTON_LEFT && mb->is_pressed())
            {
                if (mb->get_modifiers_mask().has_flag(KEY_MASK_CTRL))
                {
                    // CTRL+LMB adds a knot to the connection that can then be moved.
                    if (!_hovered_connection.is_empty())
                        _create_connection_knot(_hovered_connection, mb->get_position());
                }
            }
        }
    }

    Ref<InputEventKey> key_event = p_event;
    if (key_event.is_valid() && key_event->is_pressed() && key_event->get_keycode() == KEY_F9)
    {
        for_each_graph_node([](OrchestratorGraphNode* node) {
            if (node->is_selected())
                node->toggle_breakpoint();
        });
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
            _show_drag_hint("Use Ctrl to drop a Setter, Shift to drop a Getter");

        return true;
    }

    return false;
}

void OrchestratorGraphEdit::_drop_data(const Vector2& p_position, const Variant& p_data)
{
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
                OScriptNodeInitContext context;
                context.node_path = root->get_path_to(dropped_node);

                const Ref<Script> node_script = dropped_node->get_script();

                String global_name;
                if (node_script.is_valid())
                    global_name = ScriptServer::get_global_name(node_script);

                context.class_name = StringUtils::default_if_empty(global_name, dropped_node->get_class());

                spawn_node<OScriptNodeSceneNode>(context, _saved_mouse_position);
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
        if (data.has("functions"))
        {
            MethodInfo method = DictionaryUtils::to_method(data["functions"]);

            // Create context-menu to specify variable get or set choice
            _context_menu->clear();
            _context_menu->add_separator("Function " + method.name);
            _context_menu->add_item("Add Call Function", CM_FUNCTION_CALL);
            _context_menu->add_item("Add Callable", CM_FUNCTION_CALLABLE);
            _context_menu->set_item_metadata(_context_menu->get_item_index(CM_FUNCTION_CALL), data["functions"]);
            _context_menu->set_item_metadata(_context_menu->get_item_index(CM_FUNCTION_CALLABLE), data["functions"]);
            _context_menu->reset_size();
            _context_menu->set_position(get_screen_position() + p_position);
            _context_menu->popup();
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
            const bool validated = get_orchestration()->get_variable(variable_name)->get_variable_type() == Variant::OBJECT;

            // Create context-menu handlers
            Ref<OrchestratorGraphActionHandler> get_handler(memnew(OrchestratorGraphNodeSpawnerVariableGet(variable_name)));
            Ref<OrchestratorGraphActionHandler> set_handler(memnew(OrchestratorGraphNodeSpawnerVariableSet(variable_name)));
            Ref<OrchestratorGraphActionHandler> get_validated_handler(memnew(OrchestratorGraphNodeSpawnerVariableGet(variable_name, true)));

            // Create context-menu to specify variable get or set choice
            _context_menu->clear();
            _context_menu->add_separator("Variable " + variable_name);
            _context_menu->add_item("Get " + variable_name, CM_VARIABLE_GET);
            if (validated)
                _context_menu->add_item("Get " + variable_name + " with validation", CM_VARIABLE_GET_VALIDATED);
            _context_menu->add_item("Set " + variable_name, CM_VARIABLE_SET);
            _context_menu->set_item_metadata(_context_menu->get_item_index(CM_VARIABLE_GET), get_handler);
            if (validated)
                _context_menu->set_item_metadata(_context_menu->get_item_index(CM_VARIABLE_GET_VALIDATED), get_validated_handler);
            _context_menu->set_item_metadata(_context_menu->get_item_index(CM_VARIABLE_SET), set_handler);
            _context_menu->reset_size();
            _context_menu->set_position(get_screen_position() + p_position);
            _context_menu->popup();
        }
    }
    else if (type == "signal")
    {
        if (data.has("signals"))
        {
            OScriptNodeInitContext context;
            context.method = DictionaryUtils::to_method(data["signals"]);
            spawn_node<OScriptNodeEmitSignal>(context, _saved_mouse_position);
        }
    }
}

void OrchestratorGraphEdit::_confirm_yes_no(const String& p_text, const String& p_title, Callable p_confirm_callback)
{
    ConfirmationDialog* dialog = memnew(ConfirmationDialog);
    dialog->set_title(p_title);
    dialog->set_text(p_text);
    dialog->set_ok_button_text("Yes");
    dialog->set_cancel_button_text("No");
    dialog->set_initial_position(Window::WINDOW_INITIAL_POSITION_CENTER_SCREEN_WITH_KEYBOARD_FOCUS);
    add_child(dialog);

    dialog->connect("confirmed", p_confirm_callback);
    dialog->connect("close_requested", callable_mp_lambda(this, [dialog] { dialog->queue_free(); }));

    dialog->popup_centered();
}

void OrchestratorGraphEdit::_notify(const String& p_text, const String& p_title)
{
    ConfirmationDialog* dialog = memnew(ConfirmationDialog);
    dialog->set_title(p_title);
    dialog->set_text(p_text);
    dialog->set_ok_button_text("OK");
    dialog->get_cancel_button()->hide();
    dialog->set_initial_position(Window::WINDOW_INITIAL_POSITION_CENTER_SCREEN_WITH_KEYBOARD_FOCUS);
    add_child(dialog);

    dialog->connect("close_requested", callable_mp_lambda(this, [dialog] { dialog->queue_free(); }));

    dialog->popup_centered();
}

bool OrchestratorGraphEdit::_is_position_within_node_rect(const Vector2& p_position) const
{
    for (int i = 0; i < get_child_count(); ++i)
    {
        GraphNode* child = Object::cast_to<GraphNode>(get_child(i));
        if (child && child->get_rect().has_point(p_position))
            return true;
    }
    return false;
}

void OrchestratorGraphEdit::_cache_connection_knots()
{
    _knots.clear();
    for (const KeyValue<uint64_t, PackedVector2Array>& E : _script_graph->get_knots())
    {
        Vector<Ref<KnotPoint>> points;
        for (const Vector2& point : E.value)
        {
            Ref<KnotPoint> knot(memnew(KnotPoint));
            knot->point = point;
            points.push_back(knot);
        }
        _knots[E.key] = points;
    }
}

void OrchestratorGraphEdit::_store_connection_knots()
{
    HashMap<uint64_t, PackedVector2Array> knots;
    for (const KeyValue<uint64_t, Vector<Ref<KnotPoint>>>& E : _knots)
    {
        // Ensure that if the connection is no longer valid, the knot is not stored.
        const OScriptConnection C(E.key);
        if (!is_node_connected(itos(C.from_node), C.from_port, itos(C.to_node), C.to_port))
        {
            WARN_PRINT("Orphan knot for connection " + C.to_string() + " removed.");
            continue;
        }

        PackedVector2Array array;
        for (int i = 0; i < E.value.size(); i++)
            array.push_back(E.value[i]->point);

        // No need to serialize empty arrays
        if (!array.is_empty())
            knots[E.key] = array;
    }

    _script_graph->set_knots(knots);
}

PackedVector2Array OrchestratorGraphEdit::_get_connection_knot_points(const OScriptConnection& p_connection, bool p_apply_zoom) const
{
    PackedVector2Array array;
    if (_knots.has(p_connection.id))
    {
        const Vector<Ref<KnotPoint>>& points = _knots[p_connection.id];
        for (int i = 0; i < points.size(); i++)
            array.push_back(points[i]->point * (p_apply_zoom ? get_zoom() : 1.0));
    }
    return array;
}

void OrchestratorGraphEdit::_create_connection_knot(const Dictionary& p_connection, const Vector2& p_position)
{
    // Knots should be stored within any zoom applied.
    const Vector2 position = p_position / get_zoom();
    const Vector2 transformed_position = position + (get_scroll_offset() / get_zoom());

    const OScriptConnection connection = OScriptConnection::from_dict(p_connection);
    const PackedVector2Array knot_points = _get_connection_knot_points(connection);

    OrchestratorGraphNode* source = _get_node_by_id(connection.from_node);
    OrchestratorGraphNode* target = _get_node_by_id(connection.to_node);
    if (!source || !target)
        return;

    PackedVector2Array points;
    int knot_position = 0;

    const Vector2 from_position = source->get_output_port_position(connection.from_port) + source->get_position_offset();
    const Vector2 to_position = target->get_input_port_position(connection.to_port) + target->get_position_offset();

    points.push_back(from_position);
    points.append_array(knot_points);
    points.push_back(to_position);

    Vector<Ref<Curve2D>> curves = _get_connection_curves(points);

    float closest_distance = INFINITY;
    for (int i = 0; i < curves.size(); i++)
    {
        const Ref<Curve2D>& curve = curves[i];

        const Vector2 closest_point = curve->get_closest_point(transformed_position);
        float distance = closest_point.distance_to(transformed_position);
        if (distance < closest_distance)
        {
            closest_distance = distance;
            knot_position = i;
        }
    }

    if (!_knots.has(connection.id))
        _knots[connection.id] = Vector<Ref<KnotPoint>>();

    Ref<KnotPoint> knot(memnew(KnotPoint));
    knot->point = transformed_position;

    _knots[connection.id].insert(knot_position, knot);

    _store_connection_knots();
    _synchronize_graph_knots();

    if (_is_43p)
        _synchronize_graph_connections_with_script();
}

void OrchestratorGraphEdit::_update_theme()
{
    Ref<Font> label_font = SceneUtils::get_editor_font("main_msdf");
    Ref<Font> label_bold_font = SceneUtils::get_editor_font("main_bold_msdf");

    Ref<Theme> theme(memnew(Theme));
    theme->set_font("font", "Label", label_font);
    theme->set_font("font", "GraphNodeTitleLabel", label_bold_font);
    theme->set_font("font", "LineEdit", label_font);
    theme->set_font("font", "Button", label_font);

    set_theme(theme);
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
            Vector2 position = (node->get_position_offset()) - (get_viewport_rect().get_center() / 2);
            if (!p_animated)
            {
                set_scroll_offset(position);
                return;
            }

            const float duration = 0.2f;
            Ref<Tween> tween = get_tree()->create_tween();
            if (!UtilityFunctions::is_equal_approx(1.f, get_zoom()))
                tween->tween_method(Callable(this, "set_zoom"), get_zoom(), 1.f, duration);

            Ref<MethodTweener> scroll_tween = tween->tween_method(Callable(this, "set_scroll_offset"), get_scroll_offset(), position, duration);
            if (!UtilityFunctions::is_equal_approx(1.f, get_zoom()))
                scroll_tween->set_delay(duration);

            tween->set_ease(Tween::EASE_IN_OUT);
            tween->play();
        }
    }
}

bool OrchestratorGraphEdit::_is_node_hover_valid(const StringName& p_from, int p_from_port, const StringName& p_to, int p_to_port)
{
    if (OrchestratorGraphNode* source = _get_by_name<OrchestratorGraphNode>(p_from))
    {
        if (OrchestratorGraphNode* target = _get_by_name<OrchestratorGraphNode>(p_to))
        {
            OrchestratorGraphNodePin* source_pin = source->get_output_pin(p_from_port);
            OrchestratorGraphNodePin* target_pin = target->get_input_pin(p_to_port);

            return target_pin->can_accept(source_pin);
        }
    }
    return false;
}

PackedVector2Array OrchestratorGraphEdit::_get_connection_line(const Vector2& p_from_position, const Vector2& p_to_position) const
{
    // Create array of points from the from position to the to position, including all existing knots
    PackedVector2Array points;
    points.push_back(p_from_position);

    OScriptConnection c;
    if (_get_connection_for_points(p_from_position, p_to_position, c))
        points.append_array(_get_connection_knot_points(c, _is_43p));

    points.push_back(p_to_position);

    const Vector<Ref<Curve2D>> curves = _get_connection_curves(points);

    PackedVector2Array curve_points;
    for (const Ref<Curve2D>& curve : curves)
    {
        if (get_connection_lines_curvature() > 0)
            curve_points.append_array(curve->tessellate(5, 2.0));
        else
            curve_points.append_array(curve->tessellate(1));
    }

    return curve_points;
}

bool OrchestratorGraphEdit::_get_connection_for_points(const Vector2& p_from_position,
                                                       const Vector2& p_to_position,
                                                       OScriptConnection& r_connection) const
{
    // Godot 4.2 does not provide the from/to position affected by zoom when this method is called for drawing
    // Godot 4.3 does provide the values multipled by the zoom regardless, so we need to handle that here.
    Vector2 from_position = p_from_position * (_is_43p ? 1.0 : get_zoom());
    Vector2 to_position = p_to_position * (_is_43p ? 1.0 : get_zoom());

    // Calculate the from node and port from the from position
    int from_node = -1;
    int32_t from_port = -1;
    for (int i = 0; i < get_child_count() && from_port == -1; i++)
    {
        if (OrchestratorGraphNode* node = Object::cast_to<OrchestratorGraphNode>(get_child(i)))
        {
            from_port = node->get_port_at_position(from_position / get_zoom(), PD_Output);
            if (from_port != -1)
                from_node = node->get_script_node_id();
        }
    }

    // Calculate the to node and port from the to position
    int to_node = -1;
    int32_t to_port = -1;
    for (int i = 0; i < get_child_count() && to_port == -1; i++)
    {
        if (OrchestratorGraphNode* node = Object::cast_to<OrchestratorGraphNode>(get_child(i)))
        {
            to_port = node->get_port_at_position(to_position / get_zoom(), PD_Input);
            if (to_port != -1)
                to_node = node->get_script_node_id();
        }
    }

    // Create array of points from the from position to the to position, including all existing knots
    if (from_port != -1 && to_port != -1)
    {
        r_connection.from_node = from_node;
        r_connection.from_port = from_port;
        r_connection.to_node = to_node;
        r_connection.to_port = to_port;
        return true;
    }

    return false;
}

Vector<Ref<Curve2D>> OrchestratorGraphEdit::_get_connection_curves(const PackedVector2Array& p_points) const
{
    Vector<Ref<Curve2D>> curves;

    // For all points calculate the curve from point to point
    for (int i = 0; i < p_points.size() - 1; i++)
    {
        float xdiff = (p_points[i].x - p_points[i + 1].x);
        float cp_offset = xdiff * get_connection_lines_curvature();
        if (xdiff < 0)
            cp_offset *= -1;

        // Curvature is only applied between the first two points and last two points.
        if (i > 0 && i < (p_points.size() - 2))
            cp_offset = 0;

        Ref<Curve2D> curve(memnew(Curve2D));
        curve->add_point(p_points[i]);
        curve->set_point_out(0, Vector2(cp_offset, 0));
        curve->add_point(p_points[i + 1]);
        curve->set_point_in(1, Vector2(-cp_offset, 0));
        curves.append(curve);
    }

    return curves;
}

OrchestratorGraphNode* OrchestratorGraphEdit::_get_node_by_id(int p_id)
{
    return _get_by_name<OrchestratorGraphNode>(itos(p_id));
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

    _script_graph->sanitize_nodes();

    for (const Ref<OScriptNode>& node : _script_graph->get_nodes())
        _synchronize_graph_node(node);

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
    for (const OScriptConnection& E : _script_graph->get_connections())
        connect_node(itos(E.from_node), E.from_port, itos(E.to_node), E.to_port);
}

void OrchestratorGraphEdit::_synchronize_graph_knots()
{
    // Remove all nodes from the graph.
    List<GraphElement*> removables;
    for (int i = 0; i < get_child_count(); i++)
    {
        if (OrchestratorGraphKnot* knot = Object::cast_to<OrchestratorGraphKnot>(get_child(i)))
            removables.push_back(knot);
    }

    for (GraphElement* knot : removables)
    {
        remove_child(knot);
        knot->queue_free();
    }

    _cache_connection_knots();

    for (const KeyValue<uint64_t, Vector<Ref<KnotPoint>>>& E : _knots)
    {
        OScriptConnection connection(E.key);

        OrchestratorGraphNode* source = _get_node_by_id(connection.from_node);
        if (!source)
            continue;

        for (int i = 0; i < E.value.size(); i++)
        {
            const Ref<KnotPoint>& point = E.value[i];

            OrchestratorGraphKnot* graph_knot = memnew(OrchestratorGraphKnot);
            graph_knot->set_graph(_script_graph);
            graph_knot->set_connection(connection);
            graph_knot->set_knot(point);
            graph_knot->set_color(source->get_output_port_color(connection.from_port));
            add_child(graph_knot);

            graph_knot->connect("knot_position_changed", callable_mp_lambda(this, [&](const Vector2& position) {
                _synchronize_graph_connections_with_script();
            }));
            graph_knot->connect("knot_delete_requested", callable_mp_lambda(this, [&](const String& name) {
               _on_delete_nodes_requested(Array::make(name));
            }));
        }
    }
}

void OrchestratorGraphEdit::_remove_connection_knots(uint64_t p_connection_id)
{
    if (_knots.erase(p_connection_id))
    {
        _store_connection_knots();
        _synchronize_graph_knots();
    }
}

void OrchestratorGraphEdit::_synchronize_graph_node(Ref<OScriptNode> p_node)
{
    if (!p_node.is_valid())
        return;

    const String node_id = itos(p_node->get_id());
    if (!has_node(node_id))
    {
        const Vector2 node_size = p_node->get_size();

        OrchestratorGraphNode* graph_node = OrchestratorGraphNodeFactory::create_node(this, p_node);
        graph_node->set_title(p_node->get_node_title());
        graph_node->set_position_offset(p_node->get_position());
        graph_node->set_size(node_size.is_zero_approx() ? graph_node->get_size() : node_size);
        add_child(graph_node);
    }
    else
    {
        p_node->reconstruct_node();
    }
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

void OrchestratorGraphEdit::_complete_spawn(const Ref<OScriptNode>& p_spawned, const Callable& p_callback)
{
    if (p_spawned.is_valid())
    {
        if (p_callback != Callable())
            p_callback.callv(Array::make(p_spawned));

        emit_signal("nodes_changed");
    }
}

void OrchestratorGraphEdit::_queue_autowire(const Ref<OScriptNodePin>& p_source, const Ref<OScriptNode>& p_spawned, const Callable& p_callback)
{
    if (!p_source.is_valid() || !p_spawned.is_valid())
        return;

    const Vector<Ref<OScriptNodePin>> choices = p_spawned->get_eligible_autowire_pins(p_source);

    _autowire = memnew(OrchestratorScriptAutowireSelections);
    _autowire->set_source(p_source);
    _autowire->set_spawned(p_spawned);

    add_child(_autowire);

    _autowire->connect("confirmed", callable_mp(this, &OrchestratorGraphEdit::_complete_autowire).bind(p_spawned, p_callback, true));
    _autowire->connect("canceled", callable_mp(this, &OrchestratorGraphEdit::_complete_autowire).bind(p_spawned, p_callback, false));

    // This must be deferred to avoid conflicts with exclusive windows
    callable_mp(_autowire, &OrchestratorScriptAutowireSelections::popup_autowire).call_deferred();
}

void OrchestratorGraphEdit::_complete_autowire(const Ref<OScriptNode>& p_spawned, const Callable& p_callback, bool p_confirmed)
{
    if (p_confirmed)
    {
        const Ref<OScriptNodePin> choice = _autowire->get_autowire_choice();
        if (choice.is_valid())
        {
            if (_drag_context.output_port)
                _autowire->get_source()->link(choice);
            else
                choice->link(_autowire->get_source());

            p_spawned->post_node_autowired(_autowire->get_source()->get_owning_node(), _drag_context.get_direction());
        }
    }

    _autowire->queue_free();
    _drag_context.reset();

    _complete_spawn(p_spawned, p_callback);
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

    if (is_snapping_enabled())
        _saved_mouse_position = _saved_mouse_position.snappedf(get_snapping_distance());
}

void OrchestratorGraphEdit::_show_drag_hint(const godot::String& p_message) const
{
    OrchestratorSettings* os = OrchestratorSettings::get_singleton();
    if (!os->get_setting("ui/graph/show_overlay_action_tooltips", true))
        return;

    _drag_hint->set_text("Hint:\n" + p_message);
    _drag_hint->show();
    _drag_hint_timer->start();
}

void OrchestratorGraphEdit::_hide_drag_hint()
{
    _drag_hint->hide();
}

void OrchestratorGraphEdit::_create_script_function_callable(const StringName& p_function_name)
{
    BuiltInType callable_type = ExtensionDB::get_builtin_type(Variant::CALLABLE);

    int ctor_index = 0;
    bool found = false;
    for (; ctor_index < callable_type.constructors.size(); ++ctor_index)
    {
        const ConstructorInfo& ctor = callable_type.constructors[ctor_index];
        if (ctor.arguments.size() == 2
            && ctor.arguments[0].type == Variant::OBJECT
            && ctor.arguments[1].type == Variant::STRING_NAME)
        {
            found = true;
            break;
        }
    }

    if (!found)
        return;

    const Vector<PropertyInfo>& properties = callable_type.constructors[ctor_index].arguments;
    const Array arguments = DictionaryUtils::from_properties(properties);

    Dictionary spawn_data;
    spawn_data["type"] = Variant::CALLABLE;
    spawn_data["constructor_args"] = arguments;

    OScriptNodeInitContext compose_context;
    compose_context.user_data = spawn_data;

    spawn_node<OScriptNodeComposeFrom>(
        compose_context,
        _saved_mouse_position,
        callable_mp_lambda(this, [=](OScriptNodeComposeFrom* compose) {
            compose->find_pin(1, PD_Input)->set_default_value(p_function_name);
            compose->reconstruct_node();

            const Vector2 self_position = _saved_mouse_position - Vector2(200, 0);

            OScriptNodeInitContext self_context;
            spawn_node<OScriptNodeSelf>(
                self_context,
                self_position,
                callable_mp_lambda(this, [=](OScriptNodeSelf* self) {
                    self->find_pin(0, PD_Output)->link(compose->find_pin(0, PD_Input));
                }));
        }));
}

void OrchestratorGraphEdit::_on_connection_drag_started(const StringName& p_from, int p_from_port, bool p_output)
{
    OrchestratorSettings* os = OrchestratorSettings::get_singleton();
    const bool flow_disconnect_on_drag = os->get_setting("ui/graph/disconnect_control_flow_when_dragged", true);

    _drag_context.start_drag(p_from, p_from_port, p_output);

    if (OrchestratorGraphNode* source = _get_by_name<OrchestratorGraphNode>(p_from))
    {
        if (p_output)
        {
            // From port is an output
            OrchestratorGraphNodePin* pin = source->get_output_pin(p_from_port);
            if (pin && pin->is_execution() && flow_disconnect_on_drag)
                pin->unlink_all();

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

    if (OrchestratorGraphNode* source = _get_by_name<OrchestratorGraphNode>(p_from_node))
    {
        if (OrchestratorGraphNode* target = _get_by_name<OrchestratorGraphNode>(p_to_node))
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
    if (OrchestratorGraphNode* source = _get_by_name<OrchestratorGraphNode>(p_from_node))
    {
        if (OrchestratorGraphNode* target = _get_by_name<OrchestratorGraphNode>(p_to_node))
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
    OrchestratorGraphNode* node = _get_by_name<OrchestratorGraphNode>(p_to_node);
    ERR_FAIL_COND_MSG(!node, "Unable to find graph node with name " + p_to_node);

    OrchestratorGraphNodePin* pin = node->get_input_pin(p_to_port);
    if (!pin)
        return;

    OrchestratorGraphActionFilter filter;
    filter.context.graph = this;
    filter.context.pins.push_back(pin);

    ResolvedType resolved_type = pin->resolve_type();
    filter.target_type = resolved_type.type;

    if (resolved_type.has_target_object())
    {
        // When a resolved object isp provided, it takes precedence
        filter.target_classes.push_back(resolved_type.get_target_class());
        filter.target_object = resolved_type.object;
        filter.use_cache = false;
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
    OrchestratorGraphNode* node = _get_by_name<OrchestratorGraphNode>(p_from_node);
    ERR_FAIL_COND_MSG(!node, "Unable to find graph node with name " + p_from_node);

    OrchestratorGraphNodePin* pin = node->get_output_pin(p_from_port);
    if (!pin)
        return;

    OrchestratorGraphActionFilter filter;
    filter.flags = OrchestratorGraphActionFilter::Filter_RejectEvents;
    filter.context.graph = this;
    filter.context.pins.push_back(pin);

    ResolvedType resolved_type = pin->resolve_type();
    filter.target_type = resolved_type.type;

    if (resolved_type.has_target_object())
    {
        // When a resolved object is provided, it takes precedence
        filter.target_classes.push_back(resolved_type.get_target_class());
        filter.target_object = resolved_type.object;
        filter.use_cache = false;
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

    OrchestratorSettings* os = OrchestratorSettings::get_singleton();
    if (os->get_setting("ui/nodes/highlight_selected_connections", false))
    {
        // Get list of all selected nodes
        List<Ref<OScriptNode>> selected_nodes;
        for_each_graph_node([&](OrchestratorGraphNode* other) {
            if (other && other->is_selected())
                selected_nodes.push_back(other->get_script_node());
        });

        if (!selected_nodes.is_empty())
        {
            for_each_graph_node([&](OrchestratorGraphNode* loop_node) {
                loop_node->set_all_inputs_opacity(0.3f);
                loop_node->set_all_outputs_opacity(0.3f);
            });
        }

        List<Ref<OScriptNode>> linked_nodes;
        for (const Ref<OScriptNode>& selected : selected_nodes)
        {
            Vector<Ref<OScriptNodePin>> pins = selected->get_all_pins();
            for (const Ref<OScriptNodePin>& pin : pins)
            {
                const Vector<Ref<OScriptNodePin>> connections = pin->get_connections();
                for (const Ref<OScriptNodePin>& connection : connections)
                {
                    if (!selected_nodes.find(connection->get_owning_node()))
                        linked_nodes.push_back(connection->get_owning_node());
                }
            }
        }
        for_each_graph_node([&](OrchestratorGraphNode* other) {
            other->set_modulate(Color(1, 1, 1, 0.5));
            if (selected_nodes.find(other->get_script_node()) || linked_nodes.find(other->get_script_node()))
                other->set_modulate(Color(1, 1, 1, 1));
        });
    }

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

    OrchestratorSettings* os = OrchestratorSettings::get_singleton();
    if (os->get_setting("ui/nodes/highlight_selected_connections", false))
    {
        // Get list of all selected nodes
        List<Ref<OScriptNode>> selected_nodes;
        for_each_graph_node([&](OrchestratorGraphNode* other) {
            if (other && other->is_selected())
                selected_nodes.push_back(other->get_script_node());
        });

        if (selected_nodes.is_empty())
        {
            for_each_graph_node([&](OrchestratorGraphNode* other) {
                other->set_modulate(Color(1, 1, 1, 1.0));
                other->set_all_inputs_opacity();
                other->set_all_outputs_opacity();
            });
        }
        else
        {
            List<Ref<OScriptNode>> linked_nodes;
            for (const Ref<OScriptNode>& selected : selected_nodes)
            {
                Vector<Ref<OScriptNodePin>> pins = selected->get_all_pins();
                for (const Ref<OScriptNodePin>& pin : pins)
                {
                    const Vector<Ref<OScriptNodePin>> connections = pin->get_connections();
                    for (const Ref<OScriptNodePin>& connection : connections)
                    {
                        if (!selected_nodes.find(connection->get_owning_node()))
                            linked_nodes.push_back(connection->get_owning_node());
                    }
                }
            }
            for_each_graph_node([&](OrchestratorGraphNode* other) {
                other->set_modulate(Color(1, 1, 1, 0.5));
                if (selected_nodes.find(other->get_script_node()) || linked_nodes.find(other->get_script_node()))
                    other->set_modulate(Color(1, 1, 1, 1));
            });
        }
    }
}

void OrchestratorGraphEdit::_on_delete_nodes_requested(const PackedStringArray& p_node_names)
{
    // In Godot 4.2, there is a use case where this callback fires twice, once with no node names.
    // As sanity sake, guard against that by doing nothing if the node names array is empty.
    if (p_node_names.is_empty())
        return;

    OrchestratorSettings* settings = OrchestratorSettings::get_singleton();
    if (settings->get_setting("ui/graph/confirm_on_delete", true))
    {
        const String message = vformat("Do you wish to delete %d node(s)?", p_node_names.size());
        _confirm_yes_no(message, "Confirm deletion", callable_mp(this, &OrchestratorGraphEdit::_delete_nodes).bind(p_node_names));
    }
    else
        _delete_nodes(p_node_names);
}

void OrchestratorGraphEdit::_delete_nodes(const PackedStringArray& p_node_names)
{
    for (const String& node_name : p_node_names)
    {
        if (OrchestratorGraphNode* node = _get_by_name<OrchestratorGraphNode>(node_name))
        {
            if (!node->get_script_node()->can_user_delete_node())
            {
                const String message = "Node %d with the title '%s' cannot be deleted.\n"
                    "It may be that this node represents a function entry or some other node type that requires "
                    "deletion via the component menu instead.";

                _notify(vformat(message, node->get_script_node_id(), node->get_script_node()->get_node_title()), "Delete canceled");
                return;
            }
        }
    }

    PackedStringArray knot_names;

    for (const String& node_name : p_node_names)
    {
        if (OrchestratorGraphKnot* knot = _get_by_name<OrchestratorGraphKnot>(node_name))
        {
            knot_names.push_back(node_name);

            if (knot->is_selected())
                knot->set_selected(false);

            const OScriptConnection connection = knot->get_connection();
            if (_knots.has(connection.id))
                _knots[connection.id].erase(knot->get_knot());

            knot->queue_free();
            continue;
        }

        OrchestratorGraphNode* node = _get_by_name<OrchestratorGraphNode>(node_name);
        if (!node)
        {
            ERR_PRINT("Cannot find node with name " + node_name + " to delete");
            continue;
        }

        if (node->is_selected())
            node->set_selected(false);

        const Ref<OScriptNodeEvent> event_node = node->get_script_node();
        if (event_node.is_valid())
            _script_graph->get_orchestration()->remove_function(event_node->get_function()->get_function_name());
        else
            _script_graph->get_orchestration()->remove_node(node->get_script_node_id());

        node->queue_free();
    }

    if (!p_node_names.is_empty())
        emit_signal("nodes_changed");

    if (!knot_names.is_empty())
        _synchronize_graph_connections_with_script();
}

void OrchestratorGraphEdit::_on_right_mouse_clicked(const Vector2& p_position)
{
    OrchestratorGraphActionFilter filter;
    filter.context.graph = this;

    _show_action_menu(p_position, filter);
}

void OrchestratorGraphEdit::_on_graph_node_added(int p_node_id)
{
    Ref<OScriptNode> node = _script_graph->get_node(p_node_id);
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
        case CM_VARIABLE_GET_VALIDATED:
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
        case CM_FUNCTION_CALL:
        {
            int index = _context_menu->get_item_index(p_id);
            OScriptNodeInitContext context;
            context.method = DictionaryUtils::to_method(_context_menu->get_item_metadata(index));
            spawn_node<OScriptNodeCallScriptFunction>(context, _saved_mouse_position);
            break;
        }
        case CM_FUNCTION_CALLABLE:
        {
            int index = _context_menu->get_item_index(p_id);
            MethodInfo method = DictionaryUtils::to_method(_context_menu->get_item_metadata(index));
            _create_script_function_callable(method.name);
            break;
        }
        case CM_FILE_GET_PATH:
        case CM_FILE_PRELOAD:
        {
            const int index = _context_menu->get_item_index(p_id);

            OScriptNodeInitContext context;
            context.resource_path = _context_menu->get_item_metadata(index);

            if (p_id == CM_FILE_GET_PATH)
                spawn_node<OScriptNodeResourcePath>(context, _saved_mouse_position);
            else if (p_id == CM_FILE_PRELOAD)
                spawn_node<OScriptNodePreload>(context, _saved_mouse_position);

            break;
        }
        default:
            // no-op
            break;
    }
}

void OrchestratorGraphEdit::_on_project_settings_changed()
{
    if (_theme_update_timer->is_stopped())
    {
        _theme_update_timer->start();

        OrchestratorSettings* os = OrchestratorSettings::get_singleton();
        bool show_icons = os->get_setting("ui/nodes/show_type_icons", true);
        bool node_resizable = os->get_setting("ui/nodes/resizable_by_default", false);

        set_minimap_enabled(os->get_setting("ui/graph/show_minimap", false));
        set_show_arrange_button(os->get_setting("ui/graph/show_arrange_button", false));

        for_each_graph_node([&](OrchestratorGraphNode* node) {
            node->show_icons(show_icons);
            node->set_resizable(node_resizable);
        });
    }
}

void OrchestratorGraphEdit::_on_inspect_script()
{
    _plugin->get_editor_interface()->inspect_object(get_orchestration()->get_self().ptr());

    EditorInspector* inspector = _plugin->get_editor_interface()->get_inspector();

    TypedArray<Node> fields = inspector->find_children("*", "EditorPropertyClassName", true, false);
    if (!fields.is_empty())
    {
        if (Node* node = Object::cast_to<Node>(fields[0]))
        {
            TypedArray<Button> buttons = node->find_children("*", "Button", true, false);
            if (!buttons.is_empty())
            {
                if (Button* button = Object::cast_to<Button>(buttons[0]))
                    button->emit_signal("pressed");
            }
        }
    }
}

void OrchestratorGraphEdit::_on_validate_and_build()
{
    emit_signal("validation_requested");
}

void OrchestratorGraphEdit::_on_copy_nodes_request()
{
    _clipboard->reset();

    for_each_graph_node([&](OrchestratorGraphNode* node) {
        if (node->is_selected())
        {
            Ref<OScriptNode> script_node = node->get_script_node();

            if (!script_node->can_duplicate())
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

    for (const OScriptConnection& E : get_orchestration()->get_connections())
        if (_clipboard->nodes.has(E.from_node) && _clipboard->nodes.has(E.to_node))
            _clipboard->connections.insert(E);
}

void OrchestratorGraphEdit::_on_duplicate_nodes_request()
{
    Vector<int> duplications;
    for_each_graph_node([&](OrchestratorGraphNode* node) {
        if (node->is_selected())
        {
            if (!node->get_script_node()->can_duplicate())
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
        const Ref<OScriptNode> duplicate = _script_graph->duplicate_node(node_id, Vector2(20, 20), true);
        if (!duplicate.is_valid())
            continue;

        selections.push_back(duplicate->get_id());
        bindings[node_id] = duplicate->get_id();
    }

    for (const OScriptConnection& E : get_orchestration()->get_connections())
        if (duplications.has(E.from_node) && duplications.has(E.to_node))
            _script_graph->link(bindings[E.from_node], E.from_port, bindings[E.to_node], E.to_port);

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
    Vector2 position_offset = (get_scroll_offset() + (mouse_up_position - get_screen_position())) / get_zoom();
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
        const Ref<OScriptNode> node = _script_graph->paste_node(E.value, _clipboard->positions[E.key] + position_offset);
        selections.push_back(node->get_id());
        bindings[E.key] = node->get_id();
    }

    for (const OScriptConnection& E : _clipboard->connections)
        _script_graph->link(bindings[E.from_node], E.from_port, bindings[E.to_node], E.to_port);

    _synchronize_graph_with_script();

    for (const int selected_id : selections)
    {
        if (OrchestratorGraphNode* node = _get_node_by_id(selected_id))
            node->set_selected(true);
    }
}

void OrchestratorGraphEdit::_on_script_changed()
{
    _base_type_button->set_button_icon(SceneUtils::get_editor_icon(get_orchestration()->get_base_type()));
    _base_type_button->set_text(vformat("Base Type: %s", get_orchestration()->get_base_type()));
}

#if GODOT_VERSION >= 0x040300
void OrchestratorGraphEdit::_on_show_grid(bool p_current_state)
{
    _grid_pattern->set_disabled(!p_current_state);
}

void OrchestratorGraphEdit::_on_grid_style_selected(int p_index)
{
    const GridPattern pattern = static_cast<GridPattern>(int(_grid_pattern->get_item_metadata(p_index)));
    set_grid_pattern(pattern);
}
#endif
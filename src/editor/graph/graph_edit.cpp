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
#include "graph_edit.h"

#include "common/callable_lambda.h"
#include "common/dictionary_utils.h"
#include "common/macros.h"
#include "common/method_utils.h"
#include "common/name_utils.h"
#include "common/property_utils.h"
#include "common/scene_utils.h"
#include "common/settings.h"
#include "common/string_utils.h"
#include "common/version.h"
#include "editor/actions/filter_engine.h"
#include "editor/actions/menu.h"
#include "editor/actions/registry.h"
#include "editor/autowire_connection_dialog.h"
#include "editor/context_menu.h"
#include "editor/graph/graph_knot.h"
#include "editor/graph/graph_node_pin.h"
#include "editor/graph/nodes/graph_node_comment.h"
#include "nodes/graph_node_factory.h"
#include "script/nodes/script_nodes.h"
#include "script/script.h"
#include "script/script_server.h"

#include <godot_cpp/classes/center_container.hpp>
#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/editor_inspector.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
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

OrchestratorGraphEdit::OrchestratorGraphEdit(const Ref<OScriptGraph>& p_graph)
{
    _is_43p = _version.at_least(4, 3);

    set_name(p_graph->get_graph_name());
    set_minimap_enabled(OrchestratorSettings::get_singleton()->get_setting("ui/graph/show_minimap", false));
    set_show_arrange_button(OrchestratorSettings::get_singleton()->get_setting("ui/graph/show_arrange_button", false));
    set_right_disconnects(true);

    _script_graph = p_graph;

    _cache_connection_knots();

    set_zoom(_script_graph->get_viewport_zoom());
    set_scroll_offset(_script_graph->get_viewport_offset());
    set_show_zoom_label(true);
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

        OrchestratorSettings* settings = OrchestratorSettings::get_singleton();

        #if GODOT_VERSION >= 0x040300
        _grid_pattern = memnew(OptionButton);
        _grid_pattern->add_item("Lines");
        _grid_pattern->set_item_metadata(0, GRID_PATTERN_LINES);
        _grid_pattern->add_item("Dots");
        _grid_pattern->set_item_metadata(1, GRID_PATTERN_DOTS);
        _grid_pattern->connect("item_selected", callable_mp(this, &OrchestratorGraphEdit::_on_grid_style_selected));
        get_menu_hbox()->add_child(_grid_pattern);
        get_menu_hbox()->move_child(_grid_pattern, 5);

        if (String(settings->get_setting("ui/graph/grid_pattern", "Lines")) == "Lines")
        {
            _grid_pattern->select(0);
            set_grid_pattern(GRID_PATTERN_LINES);
        }
        else
        {
            _grid_pattern->select(1);
            set_grid_pattern(GRID_PATTERN_DOTS);
        }
        #endif

        set_show_grid(settings->get_setting("ui/graph/grid_enabled", true));
        set_snapping_enabled(settings->get_setting("ui/graph/grid_snapping_enabled", true));

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

        // Wire up our signals
        connect("child_entered_tree", callable_mp(this, &OrchestratorGraphEdit::_resort_child_nodes_on_add));
        connect("connection_from_empty", callable_mp(this, &OrchestratorGraphEdit::_on_connection_from_empty));
        connect("connection_to_empty", callable_mp(this, &OrchestratorGraphEdit::_on_connection_to_empty));
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

        if (is_visible_in_tree() && is_node_ready())
            _synchronize_graph_with_script();
    }
}

void OrchestratorGraphEdit::_bind_methods()
{
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
    EditorInterface::get_singleton()->get_script_editor()->goto_help(p_class_name);
    #else
    EditorInterface::get_singleton()->set_main_screen_editor("Script");
    EditorInterface::get_singleton()->get_script_editor()->call("_help_class_open", p_class_name);
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

void OrchestratorGraphEdit::for_each_graph_element(const std::function<void(GraphElement*)>& p_func, bool p_nodes, bool p_knots)
{
    const int child_count = get_child_count();
    for (int index = 0; index < child_count; index++)
    {
        GraphElement* element = Object::cast_to<GraphElement>(get_child(index));
        if (!element)
            continue;

        const OrchestratorGraphNode* node = Object::cast_to<OrchestratorGraphNode>(element);
        const OrchestratorGraphKnot* knot = Object::cast_to<OrchestratorGraphKnot>(element);

        if ((p_nodes && node) || (p_knots && knot))
            p_func(element);
    }
}

void OrchestratorGraphEdit::execute_action(const String& p_action_name)
{
    Ref<InputEventAction> action = memnew(InputEventAction);
    action->set_action(p_action_name);
    action->set_pressed(true);

    Input::get_singleton()->parse_input_event(action);
}

OrchestratorGraphNode* OrchestratorGraphEdit::spawn_node(const NodeSpawnOptions& p_options)
{
    ERR_FAIL_COND_V_MSG(p_options.node_class.is_empty(), nullptr, "No node class specified, cannot spawn node");
    ERR_FAIL_COND_V_MSG(!_script_graph.is_valid(), nullptr, "Cannot spawn into an invalid graph");

    const OScriptNodeInitContext& context = p_options.context;
    const Vector2& position = p_options.position;

    const Ref<OScriptNode> spawned_node = _script_graph->create_node(p_options.node_class, context, position);
    ERR_FAIL_COND_V_MSG(!spawned_node.is_valid(), nullptr, "Failed to spawn node");

    emit_signal("nodes_changed");

    OrchestratorGraphNode* spawned_graph_node = _get_node_by_id(spawned_node->get_id());
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

void OrchestratorGraphEdit::center_node(OrchestratorGraphNode* p_node)
{
    GUARD_NULL(p_node);

    clear_selection();
    p_node->set_selected(true);

    scroll_to_position(p_node->get_node_rect().get_center());
}

void OrchestratorGraphEdit::scroll_to_position(const Vector2& p_position, float p_time)
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

void OrchestratorGraphEdit::sync()
{
    _synchronize_graph_connections_with_script();
}

void OrchestratorGraphEdit::show_override_function_action_menu()
{
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
    context.script = _script_graph->get_orchestration()->get_self();
    context.class_hierarchy = Array::make(_script_graph->get_orchestration()->get_base_type());

    OrchestratorEditorActionMenu* menu = memnew(OrchestratorEditorActionMenu);
    menu->set_title("Select a graph action");
    menu->set_suffix("graph_editor_overrides");
    menu->set_close_on_focus_lost(ORCHESTRATOR_GET("ui/actions_menu/close_on_focus_lost", false));
    menu->set_show_filter_option(false);
    menu->set_start_collapsed(false);
    menu->connect("action_selected", callable_mp_this(_on_action_menu_selection));

    menu->popup_centered(
        OrchestratorEditorActionRegistry::get_singleton()->get_actions(_script_graph->get_orchestration()->get_self()),
        filter_engine,
        context);
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

void OrchestratorGraphEdit::_move_selected(const Vector2& p_delta)
{
    for (int i = 0; i < get_child_count(); i++)
    {
        GraphElement* element = Object::cast_to<GraphElement>(get_child(i));
        if (!element || !element->is_selected())
            continue;

        if (OrchestratorGraphNode* node = Object::cast_to<OrchestratorGraphNode>(element))
        {
            node->set_position_offset(node->get_position_offset() + p_delta);
            node->get_script_node()->set_position(node->get_position_offset());
        }
        else if (OrchestratorGraphKnot* knot = Object::cast_to<OrchestratorGraphKnot>(element))
        {
            knot->set_position_offset(knot->get_knot()->point + p_delta);
        }
    }
}

void OrchestratorGraphEdit::_resort_child_nodes_on_add(Node* p_node)
{
    if (_is_comment_node(p_node))
    {
        const int position = _get_connection_layer_index();

        // Comment nodes should always be before the "_connection_layer"
        // This needs to be deferred, don't change.
        call_deferred("move_child", p_node, position);
    }
}

int OrchestratorGraphEdit::_get_connection_layer_index() const
{
    int index = 0; // generally this is the first child; however, comments will causes resorts
    for (; index < get_child_count(); index++)
    {
        if (get_child(index)->get_name().match("_connection_layer"))
            break;
    }
    return index;
}

bool OrchestratorGraphEdit::_is_comment_node(Node* p_node) const
{
    return Object::cast_to<OrchestratorGraphNodeComment>(p_node);
}

OrchestratorGraphNodePin* OrchestratorGraphEdit::_resolve_pin_from_handle(const PinHandle& p_handle, bool p_input)
{
    if (OrchestratorGraphNode* node = _get_node_by_id(p_handle.node_id))
        return p_input ? node->get_input_pin(p_handle.pin_port) : node->get_output_pin(p_handle.pin_port);

    return nullptr;
}

void OrchestratorGraphEdit::_drop_data_files(const String& p_node_type, const Array& p_files, const Vector2& p_at_position)
{
    Vector2 position = p_at_position;

    for (int i = 0; i < p_files.size(); i++)
    {
        NodeSpawnOptions options;
        options.node_class = p_node_type;
        options.context.resource_path = p_files[i];
        options.position = position;

        OrchestratorGraphNode* spawned_node = spawn_node(options);
        if (spawned_node)
            position.y += spawned_node->get_size().height + 20;
    }
}

void OrchestratorGraphEdit::_drop_data_property(const Dictionary& p_property, const Vector2& p_at_position, const String& p_path, bool p_setter)
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

void OrchestratorGraphEdit::_drop_data_function(const Dictionary& p_function, const Vector2& p_at_position, bool p_as_callable)
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

            OrchestratorGraphNode* compose_node = spawn_node(options);
            if (compose_node)
            {
                compose_node->get_input_pin(1)->set_default_value(method.name);

                options.node_class = OScriptNodeSelf::get_class_static();
                options.context.user_data.reset();
                options.position = options.position - Vector2(200, 0);

                OrchestratorGraphNode* self = spawn_node(options);
                if (self)
                    self->get_output_pin(0)->link(compose_node->get_input_pin(0));
            }
        }
    }
}

void OrchestratorGraphEdit::_drop_data_variable(const String& p_name, const Vector2& p_at_position, bool p_validated, bool p_setter)
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
    if (me.is_valid() && !_is_position_valid_for_knot(me->get_position()))
    {
        Ref<InputEventMouseMotion> mm = p_event;
        if (mm.is_valid())
        {
            _hovered_connection = get_closest_connection_at_point(mm->get_position());
            if (!_hovered_connection.is_empty())
            {
                _show_drag_hint("Use Ctrl + left click to add a knot to the connection.\n"
                    "Hover over an existing knot and pressing Ctrl + left click will remove it.");
            }
        }

        Ref<InputEventMouseButton> mb = p_event;
        if (mb.is_valid())
        {
            if (mb->get_button_index() == MOUSE_BUTTON_LEFT && mb->is_pressed())
            {
                if (mb->get_modifiers_mask().has_flag(KEY_MASK_CTRL))
                {
                    // CTRL + left click adds a knot to the connection that can then be moved.
                    if (!_hovered_connection.is_empty())
                        _create_connection_knot(_hovered_connection, mb->get_position());
                }
            }
        }
    }

    Ref<InputEventMouseButton> mb = p_event;
    if (mb.is_valid())
    {
        if (mb->get_button_index() == MOUSE_BUTTON_LEFT && mb->is_pressed())
        {
            // This checks whether the left click should trigger box-selection
            //
            // While GraphEdit manages this, this information isn't directly exposed as signals, and our
            // implementation needs this detail to know if we should ignore selecting specific custom
            // graph elements, like GraphEdit does for GraphFrame in Godot 4.3.
            GraphElement* element = nullptr;
            for (int i = get_child_count() - 1; i >= 0; i--)
            {
                // Only interested in graph elements
                GraphElement* selected = Object::cast_to<GraphElement>(get_child(i));
                if (!selected)
                    continue;

                const Rect2 rect2(Point2(), selected->get_size());
                if (rect2.has_point((mb->get_position() - selected->get_position()) / get_zoom()))
                {
                    OrchestratorGraphNodeComment* comment = Object::cast_to<OrchestratorGraphNodeComment>(selected);
                    if (comment && comment->_has_point((mb->get_position() - selected->get_position()) / get_zoom()))
                    {
                        element = selected;
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

    // Our implementation needs to detect box selection and its rect to know whether the selection
    // fully encloses our comment node implementations, similar to GraphFrame in Godot 4.3
    Ref<InputEventMouseMotion> mm = p_event;
    if (mm.is_valid() && _box_selection)
    {
        const Vector2 selection_to = mm->get_position();
        const Rect2 select_rect = Rect2(_box_selection_from.min(selection_to), (_box_selection_from - selection_to).abs());

        for (int i = get_child_count() - 1; i >= 0; i--)
        {
            GraphElement* element = Object::cast_to<GraphElement>(get_child(i));
            if (!element)
                continue;

            const bool is_comment = _is_comment_node(element);
            const Rect2 r = element->get_rect();
            const bool should_be_selected = is_comment ? select_rect.encloses(r) : select_rect.intersects(r);

            // This must be deferred, don't change
            if (is_comment && !should_be_selected)
                element->call_deferred("set_selected", false);
        }
    }

    const Ref<InputEventKey> key = p_event;
    if (key.is_valid() && key->is_pressed())
    {
        // todo: Submitted https://github.com/godotengine/godot/pull/95614
        // Can eventually rely on the "cut_nodes_request" signal rather than this approach
        if (key->is_action("ui_cut", true))
        {
            _on_cut_nodes_request();
            accept_event();
        }

        if (key->is_action("ui_left", true))
        {
            _move_selected(Vector2(is_snapping_enabled() ? -get_snapping_distance() : -1, 0));
            accept_event();
        }
        else if (key->is_action("ui_right", true))
        {
            _move_selected(Vector2(is_snapping_enabled() ? get_snapping_distance() : 1, 0));
            accept_event();
        }
        else if (key->is_action("ui_up", true))
        {
            _move_selected(Vector2(0, is_snapping_enabled() ? -get_snapping_distance() : -1));
            accept_event();
        }
        else if (key->is_action("ui_down", true))
        {
            _move_selected(Vector2(0, is_snapping_enabled() ? get_snapping_distance() : 1));
            accept_event();
        }
        else if (key->get_keycode() == KEY_F9)
        {
            for_each_graph_node([](OrchestratorGraphNode* node) {
                if (node->is_selected())
                    node->toggle_breakpoint();
            });
            accept_event();
        }
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
        {
            const String variable_name = String(Array(data["variables"])[0]);
            const Ref<OScriptVariable> variable = _script_graph->get_orchestration()->get_variable(variable_name);
            if (variable.is_valid() && !variable->is_constant())
                _show_drag_hint("Use Ctrl to drop a Setter, Shift to drop a Getter");
        }
        return true;
    }

    return false;
}

void OrchestratorGraphEdit::_drop_data(const Vector2& p_position, const Variant& p_data)
{
    // No need to let the hint continue to be visible when dropped
    _drag_hint->hide();

    Dictionary data = p_data;

    _update_saved_mouse_position(p_position);

    Vector2 spawn_position = _saved_mouse_position;
    Vector2 popup_position = p_position + get_screen_position();

    const String type = data["type"];
    if (type == "nodes")
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
            options.context.node_path = path;
            options.context.class_name = StringUtils::default_if_empty(global_name, dropped_node->get_class());
            options.position = spawn_position;

            OrchestratorGraphNode* spawned_node = spawn_node(options);
            if (spawned_node)
                spawn_position.y += spawned_node->get_size().height + 20;
        }
    }
    else if (type == "files")
    {
        const Array files = data["files"];

        OrchestratorEditorContextMenu* menu = memnew(OrchestratorEditorContextMenu);
        menu->set_auto_destroy(true);
        add_child(menu);

        menu->add_separator(files.size() == 1 ? vformat("File %s", files[0]) : vformat("%d files", files.size()));
        menu->add_item("Get Path", callable_mp_this(_drop_data_files).bind(OScriptNodeResourcePath::get_class_static(), files, spawn_position));
        menu->add_item("Preload", callable_mp_this(_drop_data_files).bind(OScriptNodePreload::get_class_static(), files, spawn_position));

        menu->set_position(popup_position);
        menu->popup();
    }
    else if (type == "obj_property")
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
    else if (type == "function")
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
    else if (type == "variable")
    {
        const Array& variables = data["variables"];
        if (variables.is_empty())
            return;

        const String variable_name = variables[0];
        const Ref<OScriptVariable> variable = _script_graph->get_orchestration()->get_variable(variable_name);
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
    else if (type == "signal")
    {
        NodeSpawnOptions options;
        options.node_class = OScriptNodeEmitSignal::get_class_static();
        options.context.method = DictionaryUtils::to_method(data["signals"]);
        options.position = spawn_position;

        spawn_node(options);
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

bool OrchestratorGraphEdit::_is_position_valid_for_knot(const Vector2& p_position) const
{
    for (int i = 0; i < get_child_count(); ++i)
    {
        GraphNode* child = Object::cast_to<GraphNode>(get_child(i));

        // Skip/ignore any comment nodes from knot logic validity
        if (_is_comment_node(child))
            continue;

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

void OrchestratorGraphEdit::_queue_autowire(OrchestratorGraphNode* p_spawned_node, OrchestratorGraphNodePin* p_origin_pin)
{
    ERR_FAIL_NULL_MSG(p_spawned_node, "Cannot initiate an autowire operation with an invalid node reference");
    ERR_FAIL_NULL_MSG(p_origin_pin, "Cannot initiate an autowire operation with an invalid pin reference");

    const Vector<OrchestratorGraphNodePin*> choices = p_spawned_node->get_eligible_autowire_pins(p_origin_pin);

    // Do nothing if there are no eligible choices
    if (choices.size() == 0)
        return;

    if (choices.size() == 1)
    {
        // When there is only one choice, there is no need for the autowire dialog.
        p_origin_pin->link(choices[0]);
        return;
    }

    // Compute exact matches for class types
    Vector<OrchestratorGraphNodePin*> exact_matches;
    for (OrchestratorGraphNodePin* choice : choices)
    {
        if (choice->get_property_info().class_name.match(p_origin_pin->get_property_info().class_name))
            exact_matches.push_back(choice);
    }

    // Handle cases where class matches rank higher and have precedence
    if (exact_matches.size() == 1)
    {
        p_origin_pin->link(exact_matches[0]);
        return;
    }

    // For operator nodes, always auto-wire the first eligible pin.
    if (cast_to<OScriptNodeOperator>(p_spawned_node->get_script_node().ptr()))
    {
        p_origin_pin->link(choices[0]);
        return;
    }

    // At this point no auto-resolution could be made, show the dialog if enabled
    const bool autowire_dialog_enabled = ORCHESTRATOR_GET("ui/graph/show_autowire_selection_dialog", true);
    if (!autowire_dialog_enabled)
        return;

    OrchestratorAutowireConnectionDialog* autowire = memnew(OrchestratorAutowireConnectionDialog);

    autowire->connect("confirmed", callable_mp_lambda(this, [autowire, p_origin_pin, this] {
        OrchestratorGraphNodePin* selected = autowire->get_autowire_choice();
        if (selected && p_origin_pin)
            p_origin_pin->link(selected);
    }));

    autowire->popup_autowire(choices);
}

void OrchestratorGraphEdit::_update_saved_mouse_position(const Vector2& p_position)
{
    _saved_mouse_position = (p_position + get_scroll_offset()) / get_zoom();

    if (is_snapping_enabled())
    {
        #if GODOT_VERSION >= 0x040300
        _saved_mouse_position = _saved_mouse_position.snappedf(get_snapping_distance());
        #else
        const float step = static_cast<float>(get_snapping_distance());
        _saved_mouse_position = _saved_mouse_position.snapped(Vector2(step, step));
        #endif
    }
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

void OrchestratorGraphEdit::_connect_with_menu(PinHandle p_handle, const Vector2& p_position, bool p_input)
{
    OrchestratorGraphNodePin* pin = _resolve_pin_from_handle(p_handle, p_input);
    ERR_FAIL_NULL_MSG(pin, "Failed to resolve pin from context");

    _update_saved_mouse_position(p_position);

    _drag_from_pin = pin;

    // Resolve the drag pin target if one is available
    Object* target = nullptr;
    const ResolvedType resolved_type = _drag_from_pin->resolve_type();
    if (resolved_type.has_target_object() && resolved_type.object.is_valid() && resolved_type.object->has_target())
        target = resolved_type.object->get_target();

    Ref<OrchestratorEditorActionPortRule> port_rule;
    port_rule.instantiate();
    port_rule->configure(_drag_from_pin, target);

    Ref<OrchestratorEditorActionGraphTypeRule> graph_type_rule;
    graph_type_rule.instantiate();
    graph_type_rule->set_graph_type(_script_graph->get_flags().has_flag(OScriptGraph::GraphFlags::GF_FUNCTION)
        ? OrchestratorEditorActionDefinition::GRAPH_FUNCTION
        : OrchestratorEditorActionDefinition::GRAPH_EVENT);

    GraphEditorFilterContext context;
    context.script = _script_graph->get_orchestration()->get_self();
    context.port_type = pin->get_property_info();
    context.output = pin->is_output();
    context.class_hierarchy = Array::make(_script_graph->get_orchestration()->get_base_type());

    OrchestratorEditorActionMenu* menu = memnew(OrchestratorEditorActionMenu);
    menu->set_title("Select a graph action");
    menu->set_suffix("graph_editor");
    menu->set_close_on_focus_lost(ORCHESTRATOR_GET("ui/actions_menu/close_on_focus_lost", false));
    menu->set_show_filter_option(false);
    menu->set_start_collapsed(true);
    menu->connect("action_selected", callable_mp_this(_on_action_menu_selection));

    Ref<OrchestratorEditorActionFilterEngine> filter_engine;
    filter_engine.instantiate();
    filter_engine->add_rule(memnew(OrchestratorEditorActionSearchTextRule));
    filter_engine->add_rule(graph_type_rule);
    filter_engine->add_rule(port_rule);

    if (_drag_from_pin->is_execution())
        filter_engine->add_rule(memnew(OrchestratorEditorActionClassHierarchyScopeRule));

    const Ref<Script> source_script = _script_graph->get_orchestration()->get_self();
    OrchestratorEditorActionRegistry* action_registry = OrchestratorEditorActionRegistry::get_singleton();

    Vector<Ref<OrchestratorEditorActionDefinition>> actions;
    if (target)
        actions = action_registry->get_actions(target);
    else if (resolved_type.is_class_type())
        actions = action_registry->get_actions(resolved_type.class_name);

    if (actions.is_empty())
        actions = action_registry->get_actions(source_script);

    menu->popup(p_position + get_screen_position(), actions, filter_engine, context);
}

void OrchestratorGraphEdit::_on_connection_drag_started(const StringName& p_from, int p_from_port, bool p_output)
{
    OrchestratorSettings* os = OrchestratorSettings::get_singleton();
    const bool flow_disconnect_on_drag = os->get_setting("ui/graph/disconnect_control_flow_when_dragged", true);

    ERR_FAIL_COND_MSG(!p_from.is_valid_int(), "Drag from node name is expected to be an integer value");

    PinHandle handle;
    handle.node_id = p_from.to_int();
    handle.pin_port = p_from_port;

    OrchestratorGraphNodePin* pin = _resolve_pin_from_handle(handle, !p_output);
    ERR_FAIL_NULL_MSG(pin, "Failed to resolve drag from pin");

    _drag_from_pin = pin;

    if (p_output && flow_disconnect_on_drag && _drag_from_pin->is_execution())
        _drag_from_pin->unlink_all();

    if (p_output)
    {
        for_each_graph_node([&](OrchestratorGraphNode* node) {
            node->set_inputs_for_accept_opacity(0.3f, pin);
            node->set_all_outputs_opacity(0.3f);

            if (node->get_inputs_with_opacity() == 0 && node != pin->get_graph_node())
                node->set_modulate(Color(1, 1, 1, 0.5));
        });
    }
    else
    {
        // From port is an input
        for_each_graph_node([&](OrchestratorGraphNode* node) {
            node->set_all_inputs_opacity(0.3f);
            node->set_outputs_for_accept_opacity(0.3f, pin);

            if (node->get_outputs_with_opacity() == 0 && node != pin->get_graph_node())
                node->set_modulate(Color(1, 1, 1, 0.5));
        });
    }
}

void OrchestratorGraphEdit::_on_action_menu_selection(const Ref<OrchestratorEditorActionDefinition>& p_action)
{
    ERR_FAIL_COND_MSG(!p_action.is_valid(), "Cannot execute the action, it is invalid.");

    const Vector2 spawn_position = _saved_mouse_position;

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

            AcceptDialog* dialog = memnew(AcceptDialog);
            dialog->set_text(message);
            dialog->set_title("Failed to spawn node");
            dialog->set_exclusive(false);

            dialog->connect("canceled", callable_mp_cast(dialog, Node, queue_free));
            dialog->connect("confirmed", callable_mp_cast(dialog, Node, queue_free));

            EI->popup_dialog_centered(dialog);

            break;
        }
    }
}

void OrchestratorGraphEdit::_on_connection_drag_ended()
{
    for_each_graph_node([](OrchestratorGraphNode* node) {
        node->set_modulate(Color(1, 1, 1, 1));
        node->set_all_inputs_opacity(1.f);
        node->set_all_outputs_opacity(1.f);
    });
}

void OrchestratorGraphEdit::_on_connection(const StringName& p_from_node, int p_from_port, const StringName& p_to_node,
                                 int p_to_port)
{
    ERR_FAIL_COND_MSG(!p_from_node.is_valid_int(), "Connection from name is expected to be an integer value");
    ERR_FAIL_COND_MSG(!p_to_node.is_valid_int(), "Connection to name is expected to be an integer value");

    PinHandle from_handle;
    from_handle.node_id = p_from_node.to_int();
    from_handle.pin_port = p_from_port;

    PinHandle to_handle;
    to_handle.node_id = p_to_node.to_int();
    to_handle.pin_port = p_to_port;

    OrchestratorGraphNodePin* source = _resolve_pin_from_handle(from_handle, false);
    OrchestratorGraphNodePin* target = _resolve_pin_from_handle(to_handle, true);
    ERR_FAIL_COND_MSG(!source || !target, "Could not resolve one of the connection pins");

    source->link(target);
}

void OrchestratorGraphEdit::_on_disconnection(const StringName& p_from_node, int p_from_port, const StringName& p_to_node,
                                    int p_to_port)
{
    ERR_FAIL_COND_MSG(!p_from_node.is_valid_int(), "Connection from name is expected to be an integer value");
    ERR_FAIL_COND_MSG(!p_to_node.is_valid_int(), "Connection to name is expected to be an integer value");

    PinHandle from_handle;
    from_handle.node_id = p_from_node.to_int();
    from_handle.pin_port = p_from_port;

    PinHandle to_handle;
    to_handle.node_id = p_to_node.to_int();
    to_handle.pin_port = p_to_port;

    OrchestratorGraphNodePin* source = _resolve_pin_from_handle(from_handle, false);
    OrchestratorGraphNodePin* target = _resolve_pin_from_handle(to_handle, true);
    ERR_FAIL_COND_MSG(!source || !target, "Could not resolve one of the connection pins");

    source->unlink(target);
}

void OrchestratorGraphEdit::_on_connection_from_empty(const StringName& p_to_node, int p_to_port, const Vector2& p_position)
{
    PinHandle handle;
    handle.node_id = p_to_node.to_int();
    handle.pin_port = p_to_port;

    _connect_with_menu(handle, p_position, true);
}

void OrchestratorGraphEdit::_on_connection_to_empty(const StringName& p_from_node, int p_from_port, const Vector2& p_position)
{
    PinHandle handle;
    handle.node_id = p_from_node.to_int();
    handle.pin_port = p_from_port;

    _connect_with_menu(handle, p_position, false);
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
    EditorInterface::get_singleton()->edit_resource(node->get_inspect_object());
}

void OrchestratorGraphEdit::_on_node_deselected(Node* p_node)
{
    EditorInterface::get_singleton()->inspect_object(nullptr);

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
    if (!_disable_delete_confirmation && settings->get_setting("ui/graph/confirm_on_delete", true))
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
    _update_saved_mouse_position(p_position);

    Ref<OrchestratorEditorActionGraphTypeRule> graph_type_rule;
    graph_type_rule.instantiate();
    graph_type_rule->set_graph_type(_script_graph->get_flags().has_flag(OScriptGraph::GraphFlags::GF_FUNCTION)
        ? OrchestratorEditorActionDefinition::GRAPH_FUNCTION
        : OrchestratorEditorActionDefinition::GRAPH_EVENT);

    Ref<OrchestratorEditorActionFilterEngine> filter_engine;
    filter_engine.instantiate();
    filter_engine->add_rule(memnew(OrchestratorEditorActionSearchTextRule));
    filter_engine->add_rule(memnew(OrchestratorEditorActionClassHierarchyScopeRule));
    filter_engine->add_rule(graph_type_rule);

    GraphEditorFilterContext context;
    context.script = _script_graph->get_orchestration()->get_self();
    context.class_hierarchy = Array::make(_script_graph->get_orchestration()->get_base_type());

    OrchestratorEditorActionMenu* menu = memnew(OrchestratorEditorActionMenu);
    menu->set_title("Select a graph action");
    menu->set_suffix("graph_editor");
    menu->set_close_on_focus_lost(ORCHESTRATOR_GET("ui/actions_menu/close_on_focus_lost", false));
    menu->set_show_filter_option(false);
    menu->set_start_collapsed(true);
    menu->connect("action_selected", callable_mp_this(_on_action_menu_selection));

    menu->popup(
        p_position + get_screen_position(),
        OrchestratorEditorActionRegistry::get_singleton()->get_actions(_script_graph->get_orchestration()->get_self()),
        filter_engine,
        context);
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
            node->update_pins(show_icons);
            node->set_resizable(node_resizable);
        });
    }
}

void OrchestratorGraphEdit::_on_inspect_script()
{
    EditorInterface::get_singleton()->inspect_object(get_orchestration()->get_self().ptr());

    EditorInspector* inspector = EditorInterface::get_singleton()->get_inspector();

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

    const Vector<Ref<OScriptNode>> selected = get_selected_script_nodes();
    if (selected.is_empty())
    {
        _notify("No nodes selected, nothing copied to clipboard.", "Clipboard error");
        return;
    }

    // Check if any selected nodes cannot be copied, showing message if not.
    for (const Ref<OScriptNode>& node : selected)
    {
        if (!node->can_duplicate())
        {
            _notify(vformat("Cannot duplicate node '%s' (%d).", node->get_node_title(), node->get_id()), "Clipboard error");
            return;
        }
    }

    // Local cache of copied objects
    // Prevents creating multiple instances on paste of the same function, variable, or signal
    RBSet<Ref<OScriptFunction>> functions_cache;
    RBSet<Ref<OScriptVariable>> variables_cache;
    RBSet<Ref<OScriptSignal>> signals_cache;

    // Perform copy to clipboard
    for (const Ref<OScriptNode>& node : selected)
    {
        const Ref<OScriptNodeCallScriptFunction> call_function_node = node;
        if (call_function_node.is_valid())
        {
            const Ref<OScriptFunction> function = call_function_node->get_function();
            if (!functions_cache.has(function))
            {
                functions_cache.insert(function);
                _clipboard->functions.insert(function->duplicate());
            }
        }

        const Ref<OScriptNodeVariable> variable_node = node;
        if (variable_node.is_valid())
        {
            const Ref<OScriptVariable> variable = variable_node->get_variable();
            if (!variables_cache.has(variable))
            {
                variables_cache.insert(variable);
                _clipboard->variables.insert(variable->duplicate());
            }
        }

        const Ref<OScriptNodeEmitSignal> emit_signal_node = node;
        if (emit_signal_node.is_valid())
        {
            const Ref<OScriptSignal> signal = emit_signal_node->get_signal();
            if (!signals_cache.has(signal))
            {
                signals_cache.insert(signal);
                _clipboard->signals.insert(signal->duplicate());
            }
        }

        const int node_id = node->get_id();
        _clipboard->positions[node_id] = node->get_position();
        _clipboard->nodes[node_id] = _script_graph->copy_node(node_id, true);
    }

    // Connections between pasted nodes, copy connections
    for (const OScriptConnection& E : get_orchestration()->get_connections())
    {
        if (_clipboard->nodes.has(E.from_node) && _clipboard->nodes.has(E.to_node))
            _clipboard->connections.insert(E);
    }
}

void OrchestratorGraphEdit::_on_cut_nodes_request()
{
    _clipboard->reset();

    _on_copy_nodes_request();

    PackedStringArray selected_names;
    for (int index = 0; index < get_child_count(); index++)
    {
        GraphElement* element = Object::cast_to<GraphElement>(get_child(index));
        if (element && element->is_selected())
            selected_names.push_back(element->get_name());
    }

    _disable_delete_confirmation = true;
    _on_delete_nodes_requested(selected_names);
    _disable_delete_confirmation = false;
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

    Orchestration* orchestration = _script_graph->get_orchestration();

    // Iterate copied function declarations and assert if paste is invalid
    // Functions are unique in that we do not clone their nodes or structure, so the function must exist
    // in the target orchestration with the same signature for the paste to be valid.
    for (const Ref<OScriptFunction>& E : _clipboard->functions)
    {
        if (!orchestration->has_function(E->get_function_name()))
        {
            _notify(vformat("Function '%s' does not exist in this orchestration.", E->get_function_name()), "Clipboard error");
            return;
        }

        // Exists, verify if its identical
        const Ref<OScriptFunction> other = orchestration->find_function(E->get_function_name());
        if (!MethodUtils::has_same_signature(E->get_method_info(), other->get_method_info()))
        {
            _notify(vformat("Function '%s' exists with a different definition.", E->get_function_name()), "Clipboard error");
            return;
        }
    }

    // Iterate copied variable declarations and assert if paste is invalid
    Vector<Ref<OScriptVariable>> variables_to_create;
    for (const Ref<OScriptVariable>& E : _clipboard->variables)
    {
        if (!orchestration->has_variable(E->get_variable_name()))
        {
            variables_to_create.push_back(E);
            continue;
        }

        // Exists, verify if its identical
        const Ref<OScriptVariable> other = orchestration->get_variable(E->get_variable_name());
        if (!PropertyUtils::are_equal(E->get_info(), other->get_info()))
        {
            _notify(vformat("Variable '%s' exists with a different definition.", E->get_variable_name()), "Clipboard error");
            return;
        }
    }

    // Iterate copied signal declarations and assert if paste is invalid
    Vector<Ref<OScriptSignal>> signals_to_create;
    for (const Ref<OScriptSignal>& E : _clipboard->signals)
    {
        if (!orchestration->has_custom_signal(E->get_signal_name()))
        {
            signals_to_create.push_back(E);
            continue;
        }

        // When signal exists, verify whether the signal has the same signature and fail if it doesn't.
        const Ref<OScriptSignal> other = orchestration->get_custom_signal(E->get_signal_name());
        if (!MethodUtils::has_same_signature(E->get_method_info(), other->get_method_info()))
        {
            _notify(vformat("Signal '%s' exists with a different definition.", E->get_signal_name()), "Clipboard error");
            return;
        }
    }

    for (const KeyValue<int, Ref<OScriptNode>>& E : _clipboard->nodes)
    {
        const Ref<OScriptNodeCallScriptFunction> call_script_function_node = E.value;
        if (call_script_function_node.is_valid())
        {
            const StringName function_name = call_script_function_node->get_function()->get_function_name();
            const Ref<OScriptFunction> this_function = get_orchestration()->find_function(function_name);
            if (this_function.is_valid())
            {
                // Since source OScriptFunction matches this OScriptFunction declaration, copy the
                // GUID from this orchestrations script function and set it on the node
                call_script_function_node->set("guid", this_function->get_guid().to_string());
            }
        }
    }

    // Iterate variables to be created
    for (const Ref<OScriptVariable>& E : variables_to_create)
    {
        Ref<OScriptVariable> new_variable = orchestration->create_variable(E->get_variable_name());
        new_variable->copy_persistent_state(E);
    }

    // Iterate signals to be created
    for (const Ref<OScriptSignal>& E : signals_to_create)
    {
        Ref<OScriptSignal> new_signal = orchestration->create_custom_signal(E->get_signal_name());
        new_signal->copy_persistent_state(E);
    }

    const Vector2 mouse_up_position = get_screen_position() + get_local_mouse_position();
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

    emit_signal("nodes_changed");
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

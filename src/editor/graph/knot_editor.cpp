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
#include "editor/graph/knot_editor.h"

#include "common/macros.h"
#include "core/godot/core_string_names.h"
#include "editor/graph/graph_panel.h"
#include "editor/graph/nodes/comment_graph_node.h"
#include "editor/graph/nodes/knot_node.h"

#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>

bool OrchestratorEditorGraphPanelKnotEditor::_are_knot_maps_equal(const KnotMap& p_left, const KnotMap& p_right) {
    if (p_left.size() != p_right.size()) {
        return false;
    }

    for (const KeyValue<uint64_t, PointArray> &pair : p_left) {
        const uint64_t &key = pair.key;
        const PointArray &value_a = pair.value;

        if (!p_right.has(key)) {
            return false;
        }

        const PointArray &value_b = p_right[key];

        if (value_a.size() != value_b.size()) {
            return false;
        }

        for (int i = 0; i < value_a.size(); ++i) {
            if (value_a[i] != value_b[i]) {
                return false;
            }
        }
    }

    return true;
}

bool OrchestratorEditorGraphPanelKnotEditor::_are_point_arrays_equal(const PointArray& p_left, const PointArray& p_right) {
    if (p_left.size() != p_right.size()) {
        return false;
    }

    for (int i = 0; i < p_left.size(); ++i) {
        if (p_left[i] != p_right[i]) {
            return false;
        }
    }
    return true;
}

void OrchestratorEditorGraphPanelKnotEditor::_knot_dragged(
    const Vector2& p_old_position, const Vector2& p_new_position, GraphElement* p_knot) {
    // When a GraphElement finishes being dragged, this event is emitted.
    //
    // The KnotEditor uses this to effectively debounce the position changes so that
    // the underlying graph model is only updated when the user finishes dragging.
    _notify_changed();
    _notify_graph_to_refresh_connections();
}

void OrchestratorEditorGraphPanelKnotEditor::_knot_position_offset_changed(GraphElement* p_knot) {
    OrchestratorEditorGraphNodeKnot* knot = cast_to<OrchestratorEditorGraphNodeKnot>(p_knot);
    if (!knot) {
        return;
    }

    // Update the KnotInfo's position details as its moved.
    // This makes sure that when we request a redraw of the connection lines, the position
    // state provided to the graph is accurate and represents the knot positions.
    Vector<KnotInfo>& entries = _knots[knot->get_connection_id()];
    for (KnotInfo& entry : entries) {
        if (entry.guid == knot->get_guid()) {
            entry.position = knot->get_position_offset();
        }
    }

    _notify_graph_to_refresh_connections();
}

void OrchestratorEditorGraphPanelKnotEditor::_update_knots(const KnotMap& p_current_state) {
    // Quick exit, comparing state
    if (_previous_state.size() == p_current_state.size()) {
        bool all_matched = true;
        for (const KeyValue<uint64_t, PointArray>& E : _previous_state) {
            if (!p_current_state.has(E.key) || !_are_point_arrays_equal(E.value, p_current_state[E.key])) {
                all_matched = false;
                break;
            }
        }
        if (all_matched) {
            return;
        }
    }

    // Incrementally rebuild changed knots
    HashSet<uint64_t> ids_to_remove;
    HashSet<uint64_t> ids_to_add_or_update;

    for (const KeyValue<uint64_t, PointArray>& E : _previous_state) {
        const uint64_t& key = E.key;
        if (!p_current_state.has(key)) {
            ids_to_remove.insert(key);
        } else if (!_are_point_arrays_equal(E.value, p_current_state[key])) {
            ids_to_add_or_update.insert(key);
        }
    }

    for (const KeyValue<uint64_t, PointArray>& E : p_current_state) {
        const uint64_t& key = E.key;
        if (!_previous_state.has(key)) {
            ids_to_add_or_update.insert(key);
        }
    }

    for (uint64_t connection_id : ids_to_remove) {
        remove_knots_for_connection(connection_id);
    }

    for (uint64_t connection_id : ids_to_add_or_update) {
        _recreate_knots_for_connection(connection_id, p_current_state[connection_id]);
    }

    _previous_state = p_current_state;
}

void OrchestratorEditorGraphPanelKnotEditor::_recreate_knots_for_connection(uint64_t p_id, const PointArray& p_points)
{
    // Remove all knots for the connection
    remove_knots_for_connection(p_id);

    // New set of points for a connection
    // It's easier to rebuild all knots for a connection
    for (const Vector2& point : p_points) {
        _create_knot(p_id, point);
    }
}

void OrchestratorEditorGraphPanelKnotEditor::_create_knot(uint64_t p_connection_id, const Vector2& p_point, int64_t p_index) {
    if (!_knots.has(p_connection_id)) {
        _knots[p_connection_id] = Vector<KnotInfo>();
    }

    KnotInfo info;
    info.connection_id = p_connection_id;
    info.guid = Guid::create_guid();
    info.position = p_point;

    if (p_index == -1) {
        _knots[p_connection_id].push_back(info);
    } else {
        _knots[p_connection_id].insert(p_index, info);
    }

    OrchestratorEditorGraphNodeKnot* knot = memnew(OrchestratorEditorGraphNodeKnot);
    knot->set_connection_id(info.connection_id);
    knot->set_guid(info.guid);
    knot->set_position_offset(p_point);
    knot->set_selected_color(_selected_color);

    // Notifies GraphPanel to add the knot to the graph
    get_parent()->add_child(knot);

    connect("selection_color_changed", callable_mp(knot, &OrchestratorEditorGraphNodeKnot::set_selected_color));
    connect("remove_connection_knots_requested", callable_mp(knot, &OrchestratorEditorGraphNodeKnot::remove_knots_for_connection));

    // As knot is repositions, keeps the editor synchronized
    knot->connect("dragged", callable_mp_this(_knot_dragged).bind(knot));
    knot->connect("position_offset_changed", callable_mp_this(_knot_position_offset_changed).bind(knot));
}

void OrchestratorEditorGraphPanelKnotEditor::_notify_changed() {
    emit_signal(CoreStringName(changed));
}

void OrchestratorEditorGraphPanelKnotEditor::_notify_knot_nodes_selection_color_changed() {
    emit_signal("selection_color_changed", _selected_color);
}

void OrchestratorEditorGraphPanelKnotEditor::_notify_graph_to_refresh_connections() {
    emit_signal("refresh_connections_requested");
}

void OrchestratorEditorGraphPanelKnotEditor::set_selected_color(const Color& p_color) {
    if (_selected_color != p_color) {
        _selected_color = p_color;
        _notify_knot_nodes_selection_color_changed();
    }
}

bool OrchestratorEditorGraphPanelKnotEditor::is_create_knot_keybind(const Ref<InputEvent>& p_event) const {
    const Ref<InputEventMouseButton> mb = p_event;
    return mb.is_valid()
        && mb->is_pressed()
        && mb->get_modifiers_mask().has_flag(KEY_MASK_CTRL)
        && mb->get_button_index() == MOUSE_BUTTON_LEFT;
}

bool OrchestratorEditorGraphPanelKnotEditor::is_remove_knot_keybind(const Ref<InputEvent>& p_event) const {
    return is_create_knot_keybind(p_event);
}

PackedVector2Array OrchestratorEditorGraphPanelKnotEditor::get_knots_for_connection(uint64_t p_connection_id) const {
    PackedVector2Array results;
    if (_knots.has(p_connection_id)) {
        for (const KnotInfo& entry : _knots[p_connection_id]) {
            results.push_back(entry.position);
        }
    }
    return results;
}

Vector<Ref<Curve2D>> OrchestratorEditorGraphPanelKnotEditor::get_curves_for_points(
    const PackedVector2Array& p_points, float p_curvature) const {
    Vector<Ref<Curve2D>> curves;

    // For all points calculate the curve from point to point
    for (int i = 0; i < p_points.size() - 1; i++) {
        float xdiff = (p_points[i].x - p_points[i + 1].x);
        float cp_offset = xdiff * p_curvature;
        if (xdiff < 0) {
            cp_offset *= -1;
        }

        // Curvature is only applied between the first two points and last two points.
        if (i > 0 && i < (p_points.size() - 2)) {
            cp_offset = 0;
        }

        Ref<Curve2D> curve;
        curve.instantiate();

        curve->add_point(p_points[i]);
        curve->set_point_out(0, Vector2(cp_offset, 0));
        curve->add_point(p_points[i + 1]);
        curve->set_point_in(1, Vector2(-cp_offset, 0));

        curves.append(curve);
    }

    return curves;
}

void OrchestratorEditorGraphPanelKnotEditor::flush_knot_cache(const Ref<OrchestrationGraph>& p_graph) {
    RBSet<Connection> connections = p_graph->get_connections();

    KnotMap knots;
    for (const KeyValue<uint64_t, Vector<KnotInfo>>& E : _knots) {
        for (const KnotInfo& knot : E.value) {
            const Connection C(knot.connection_id);
            if (!connections.has(C)) {
                continue;
            }
            knots[E.key].push_back(knot.position);
        }
    }

    p_graph->set_knots(knots);
}

void OrchestratorEditorGraphPanelKnotEditor::remove_knots_for_connection(uint64_t p_id) {
    emit_signal("remove_connection_knots_requested", p_id);

    _knots.erase(p_id);
}

String OrchestratorEditorGraphPanelKnotEditor::get_hint_message() const {
    return "Use Ctrl + Left Click (LMB) to add a knot to the connection.\n"
           "Hover over an existing knot and pressing Ctrl + Left Click (LMB) will remove it.";
}

bool OrchestratorEditorGraphPanelKnotEditor::is_knot(const GraphElement* p_element) {
    return cast_to<OrchestratorEditorGraphNodeKnot>(p_element) != nullptr;
}

void OrchestratorEditorGraphPanelKnotEditor::remove_knots(const TypedArray<GraphElement>& p_knot_elements) {
    for (int i = 0; i < p_knot_elements.size(); ++i) {
        OrchestratorEditorGraphNodeKnot* knot = cast_to<OrchestratorEditorGraphNodeKnot>(p_knot_elements[i]);
        if (knot) {
            if (knot->is_selected()) {
                knot->set_selected(false);
            }

            const uint64_t connection_id = knot->get_connection_id();
            if (_knots.has(connection_id)) {
                Vector<KnotInfo>& data = _knots[connection_id];
                for (int j = 0; j < data.size(); j++) {
                    const KnotInfo& info = data[j];
                    if (info.guid == knot->get_guid()) {
                        data.remove_at(j);
                        break;
                    }
                }
            }

            knot->queue_free();
        }
    }

    _notify_changed();
    callable_mp_this(_notify_graph_to_refresh_connections).call_deferred();
}

void OrchestratorEditorGraphPanelKnotEditor::create_knot(
    const Connection& p_connection, const Vector2& p_position, GraphNode* p_from, GraphNode* p_to, float p_curvature) {
    if (!p_from || !p_to) {
        return;
    }

    const int32_t from_port = static_cast<int32_t>(p_connection.from_port);
    const int32_t to_port = static_cast<int32_t>(p_connection.to_port);

    const Vector2 from_position = p_from->get_output_port_position(from_port) + p_from->get_position_offset();
    const Vector2 to_position = p_to->get_input_port_position(to_port) + p_to->get_position_offset();

    PackedVector2Array points;
    points.push_back(from_position);
    points.append_array(get_knots_for_connection(p_connection.id));
    points.push_back(to_position);

    Vector<Ref<Curve2D>> curves = get_curves_for_points(points, p_curvature);

    int knot_position = 0;
    float closest_distance = INFINITY;
    for (int i = 0; i < curves.size(); i++) {
        const Ref<Curve2D>& curve = curves[i];

        const Vector2 closest_point = curve->get_closest_point(p_position);
        float distance = closest_point.distance_to(p_position);
        if (distance < closest_distance) {
            closest_distance = distance;
            knot_position = i;
        }
    }

    _create_knot(p_connection.id, p_position, knot_position);

    if (_godot_version.at_least(4, 3)) {
        _notify_graph_to_refresh_connections();
    }
}

void OrchestratorEditorGraphPanelKnotEditor::update(const KnotMap& p_knots, bool p_force) {
    if (p_force) {
        _previous_state.clear();
    }

    if (!p_force && _are_knot_maps_equal(_previous_state, p_knots)) {
        return;
    }
    _update_knots(p_knots);
}

void OrchestratorEditorGraphPanelKnotEditor::_bind_methods() {
    // Knot nodes listen for this and update their selection color when emitted
    ADD_SIGNAL(MethodInfo("selection_color_changed", PropertyInfo(Variant::COLOR, "color")));

     // Requests GraphPanel to refresh/rebuild connections
    ADD_SIGNAL(MethodInfo("refresh_connections_requested"));

    // Notifies observers the KnotManager state has changed
    // Could this be the same as flush_knots but using a pull versus push for the data?
    ADD_SIGNAL(MethodInfo(CoreStringName(changed)));

    // Notifies nodes to self-delete themselves if they're associated with the connection
    ADD_SIGNAL(MethodInfo("remove_connection_knots_requested", PropertyInfo(Variant::INT, "connection_id")));
}

OrchestratorEditorGraphPanelKnotEditor::OrchestratorEditorGraphPanelKnotEditor(const GodotVersionInfo& p_godot_version) {
    _godot_version = p_godot_version;
}
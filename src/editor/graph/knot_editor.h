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
#ifndef ORCHESTRATOR_EDITOR_GRAPH_PANEL_KNOT_EDITOR_H
#define ORCHESTRATOR_EDITOR_GRAPH_PANEL_KNOT_EDITOR_H

#include "common/godot_version.h"
#include "common/guid.h"
#include "script/connection.h"
#include "script/graph.h"

#include <memory>

#include <godot_cpp/classes/curve2d.hpp>
#include <godot_cpp/classes/graph_edit.hpp>
#include <godot_cpp/classes/graph_node.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>

using namespace godot;

/// Forward declarations
class OrchestratorEditorGraphNodeKnot;
class OrchestratorEditorGraphPanel;

/// This provides functionality about placing and managing knots in a GraphEdit.
///
/// The behavior was extracted from the OrchestratorGraphEditor with the intent that the use of
/// knots would eventually be replaced by reroute nodes similar to the Godot Visual Shader
/// plugin and other visual scripting tools.
///
class OrchestratorEditorGraphPanelKnotEditor : public Node {
    GDCLASS(OrchestratorEditorGraphPanelKnotEditor, Node);

    struct KnotInfo {
        uint64_t connection_id = 0;
        Guid guid;
        Vector2 position;
        int64_t index = 0;
    };

    using PointArray = PackedVector2Array;
    using KnotMap = HashMap<uint64_t, PointArray>;
    using KnotInfoMap = HashMap<uint64_t, Vector<KnotInfo>>;
    using Connection = OScriptConnection;

    GodotVersionInfo _godot_version;
    KnotMap _previous_state;
    KnotInfoMap _knots;
    Color _selected_color;
    Dictionary _hovered_connection;

    // Needed for Godot
    OrchestratorEditorGraphPanelKnotEditor() = default;

    static bool _are_knot_maps_equal(const KnotMap& p_left, const KnotMap& p_right);
    static bool _are_point_arrays_equal(const PointArray& p_left, const PointArray& p_right);

    void _knot_dragged(const Vector2& p_old_position, const Vector2& p_new_position, GraphElement* p_knot);
    void _knot_position_offset_changed(GraphElement* p_knot);

    void _update_knots(const KnotMap& p_current_state);
    void _recreate_knots_for_connection(uint64_t p_id, const PointArray& p_points);
    void _create_knot(uint64_t p_connection_id, const Vector2& p_point, int64_t p_index = -1);

    void _notify_changed();
    void _notify_knot_nodes_selection_color_changed();
    void _notify_graph_to_refresh_connections();

protected:
    static void _bind_methods();

public:

    // Used by GraphPanel to set settings colors
    void set_selected_color(const Color& p_color);

    bool is_create_knot_keybind(const Ref<InputEvent>& p_event) const;
    bool is_remove_knot_keybind(const Ref<InputEvent>& p_event) const;

    PointArray get_knots_for_connection(uint64_t p_connection_id) const;
    Vector<Ref<Curve2D>> get_curves_for_points(const PointArray& p_points, float p_curvature) const;

    void flush_knot_cache(const Ref<OrchestrationGraph>& p_graph);

    void remove_knots_for_connection(uint64_t p_id);

    String get_hint_message() const;

    void create_knot(const Connection& p_connection, const Vector2& p_position, GraphNode* p_from, GraphNode* p_to, float p_curvature);

    bool is_knot(const GraphElement* p_element);
    void remove_knots(const TypedArray<GraphElement>& p_knot_elements);

    // Called by GraphPanel to update knots
    void update(const KnotMap& p_knots);

    explicit OrchestratorEditorGraphPanelKnotEditor(const GodotVersionInfo& p_godot_version);
    ~OrchestratorEditorGraphPanelKnotEditor() override = default;
};

#endif // ORCHESTRATOR_EDITOR_GRAPH_PANEL_KNOT_EDITOR_H
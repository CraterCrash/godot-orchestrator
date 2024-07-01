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
#ifndef ORCHESTRATOR_GRAPH_KNOT_H
#define ORCHESTRATOR_GRAPH_KNOT_H

#include "editor/graph/graph_edit.h"
#include "script/script.h"

#include <godot_cpp/classes/graph_element.hpp>
#include <godot_cpp/classes/texture_rect.hpp>

using namespace godot;

/// Represents a simple overlay at a connection knot allowing the user to move the position.
class OrchestratorGraphKnot : public GraphElement
{
    GDCLASS(OrchestratorGraphKnot, GraphElement);
    static void _bind_methods();

protected:
    OScriptConnection _connection;     //! The connection this knot belongs
    Ref<OScriptGraph> _graph;          //! The owning graph
    Ref<OrchestratorKnotPoint> _knot;  //! The knot
    TextureRect* _icon;                //! The icon
    Color _color;                      //! The knot color

    //~ Begin Signal Handlers
    void _connections_changed(const String& p_caller);
    void _position_changed();
    void _node_selected();
    void _node_deselected();
    //~ End Signal Handlers

    const Vector2 RENDER_OFFSET{ 8, 8 };
    const Vector2 RENDER_ICON_SIZE = RENDER_OFFSET * 2;

public:
    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

    //~ Begin Control Interface
    void _gui_input(const Ref<InputEvent>& p_event) override;
    //~ End Control Interface

    /// Set the owning graph
    /// @param p_graph the graph that owns this knot
    void set_graph(const Ref<OScriptGraph>& p_graph);

    /// Gets the owning connection
    /// @return the connection
    OScriptConnection get_connection() const { return _connection; }

    /// Sets the owning connection for this knot.
    /// @param p_connection the connection
    void set_connection(OScriptConnection p_connection) { _connection = p_connection; }

    /// Get the knot reference
    /// @return the knot reference
    Ref<OrchestratorKnotPoint> get_knot() const { return _knot; }

    /// Sets the knot reference
    /// @param p_knot the knot reference
    void set_knot(const Ref<OrchestratorKnotPoint>& p_knot);

    /// Set the knot's color
    /// @param p_color the color
    void set_color(const Color& p_color) { _color = p_color; }
};

#endif  // ORCHESTRATOR_GRAPH_KNOT_H
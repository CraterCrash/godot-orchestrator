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
#ifndef ORCHESTRATOR_EDITOR_GRAPH_NODE_KNOT_H
#define ORCHESTRATOR_EDITOR_GRAPH_NODE_KNOT_H

#include "editor/graph/knot_editor.h"

#include <godot_cpp/classes/graph_element.hpp>

using namespace godot;

/// A special implementation of <code>GraphElement</code> that places a small pin on a wire connection
/// allowing the user to specify the drawing angle and position of the connection that it belongs.
///
class OrchestratorEditorGraphNodeKnot : public GraphElement {
    GDCLASS(OrchestratorEditorGraphNodeKnot, GraphElement)

    Color _color;
    Color _selected_color;
    Ref<Texture2D> _icon;
    uint64_t _connection_id;
    Guid _guid;

    void _position_offset_changed();
    Color _calculate_knot_color();

protected:
    static void _bind_methods();

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

public:
    //~ Begin Control Interface
    void _gui_input(const Ref<InputEvent>& p_event) override;
    bool _has_point(const Vector2& p_point) const override;
    //~ End Control Interface

    uint64_t get_connection_id() const { return _connection_id; }
    void set_connection_id(uint64_t p_connection_id);

    Guid get_guid() const { return _guid; }
    void set_guid(const Guid& p_guid);

    void set_selected_color(const Color& p_color);

    void remove_knots_for_connection(uint64_t p_connection_id);

    OrchestratorEditorGraphNodeKnot();
};

#endif // ORCHESTRATOR_EDITOR_GRAPH_NODE_KNOT_H
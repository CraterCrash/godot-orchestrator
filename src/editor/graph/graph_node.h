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
#ifndef ORCHESTRATOR_EDITOR_GRAPH_NODE_H
#define ORCHESTRATOR_EDITOR_GRAPH_NODE_H

#include "script/node.h"

#include <godot_cpp/classes/graph_node.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/templates/hash_map.hpp>

using namespace godot;

class OrchestratorEditorGraphPanel;
class OrchestratorEditorGraphPin;

/// A node represents a logical step within the visual graph, which can act as a function entry or return,
/// a function call, mathematical operation, or more complex behaviors like for loops, and more. The node
/// acts as a container that houses one or more <code>OrchestratorEditorGraphPin</code> widgets.
///
class OrchestratorEditorGraphNode : public GraphNode {
    friend class OrchestratorEditorGraphPanel;

    GDCLASS(OrchestratorEditorGraphNode, GraphNode);

    struct Slot {
        int64_t slot = 0;
        Control* row = nullptr;
        OrchestratorEditorGraphPin* left = nullptr;
        OrchestratorEditorGraphPin* right = nullptr;
    };

    typedef HashMap<int, Slot> SlotMap;
    typedef KeyValue<int, Slot> SlotMapKeyValue;

    Ref<OrchestrationGraphNode> _node;
    HBoxContainer* _indicators_hbox = nullptr;
    SlotMap _slots;
    bool _show_type_icons = true;
    bool _show_advanced_tooltips = false;

protected:
    static void _bind_methods();

    //~ Begin Wrapped Interface
    void _notification(int p_what); // NOLINT
    //~ End Wrapped Interface

    const Ref<OrchestrationGraphNode> get_graph_node() const { return _node; }

    void _resize_to_content();

    void _node_selected();
    void _pin_connection_status_changed(int p_type, int p_index, bool p_connected);

    void _update_titlebar();

    virtual void _create_pin_widgets();
    void _update_pin_widgets();

    virtual void _create_add_button_widgets();
    void _add_pin_requested();

    virtual void _create_indicators();
    void _add_indicator(const String& p_icon_name, const String& p_tooltip_text = String());

    virtual void _update_styles();

    virtual String _get_tooltip_text();

    void _draw_port2(int32_t p_slot_index, const Vector2i& p_position, bool p_left, const Color& p_color, const Color& p_rim_color);

public:
    //~ Begin Control Interface
    void _gui_input(const Ref<InputEvent>& p_event) override;
    //~ End Control Interface

    //~ Begin GraphNode Interface
    void _draw_port(int32_t p_slot_index, const Vector2i &p_position, bool p_left, const Color &p_color) override;
    //~ End GraphNode Interface

    void set_node(const Ref<OrchestrationGraphNode>& p_node);

    OrchestratorEditorGraphPanel* get_graph() const;

    int get_id() const;

    bool can_user_delete_node() const;
    bool is_bookmarked() const;
    bool is_breakpoint() const;

    bool can_jump_to_definition() const;
    Object* get_definition_object() const;

    Vector<OrchestratorEditorGraphPin*> get_pins() const;

    Vector<OrchestratorEditorGraphPin*> get_eligible_autowire_pins(OrchestratorEditorGraphPin* p_pin) const;

    // These expect you to use get_input_port_slot/get_output_port_slot methods first
    OrchestratorEditorGraphPin* get_input_pin(int32_t p_slot);
    OrchestratorEditorGraphPin* get_output_pin(int32_t p_slot);
    OrchestratorEditorGraphPin* get_pin(int32_t p_slot, EPinDirection p_direction = PD_Input);

    // Because we overload these in this class, we pull them back in
    using GraphNode::get_input_port_slot;
    using GraphNode::get_output_port_slot;

    // Helper methods to locate pin slots by pin names
    int32_t get_input_port_slot(const String& p_pin_name);
    int32_t get_output_port_slot(const String& p_pin_name);
    int32_t get_port_slot(const String& p_pin_name, EPinDirection p_direction = PD_Input);
    int32_t get_port_slot(int32_t p_port, EPinDirection p_direction = PD_Input);

    Vector2 get_port_position_for_pin(OrchestratorEditorGraphPin* p_pin);
    int32_t get_port_at_position(const Vector2& p_position);
    int32_t get_slot_at_position(const Vector2& p_position);

    int32_t get_pin_port(OrchestratorEditorGraphPin* p_pin);

    Vector<GraphElement*> get_overlapping_elements() const;

    Rect2 get_graph_rect() const;

    bool is_selected_exclusively() const;

    bool is_add_pin_button_visible() const;

    void set_show_type_icons(bool p_show_type_icons);
    void set_show_advanced_tooltips(bool p_show_advanced_tooltips);
    void set_slot_color_opacity(float p_opacity, EPinDirection p_direction = PD_MAX);

    // Provides a way for an external actor to refresh the node
    // It's also virtual to allow derived classes to change how nodes are composed
    virtual void update();
    virtual void redraw_connections();

    virtual void notify_connections_changed();
    virtual void notify_pin_default_value_changed(OrchestratorEditorGraphPin* p_pin) {}
    virtual void notify_bookmarks_changed();
    virtual void notify_breakpoints_changed();

    OrchestratorEditorGraphNode();
};

#endif // ORCHESTRATOR_EDITOR_GRAPH_NODE_H
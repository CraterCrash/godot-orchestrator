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
#ifndef ORCHESTRATOR_EDITOR_GRAPH_PANEL_H
#define ORCHESTRATOR_EDITOR_GRAPH_PANEL_H

#include "common/godot_version.h"
#include "core/godot/object/weak_ref.h"
#include "editor/graph/graph_node.h"
#include "editor/graph/graph_panel_styler.h"

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/graph_edit.hpp>
#include <godot_cpp/classes/h_flow_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/style_box.hpp>
#include <godot_cpp/classes/timer.hpp>
#include <godot_cpp/templates/hash_set.hpp>

using namespace godot;

/// Forward declarations
class OrchestratorEditorActionDefinition;
class OrchestratorEditorGraphPanelKnotEditor;

/// A graph panel is a widget that allows the placement of pins that contain ports (aka pins) that provide a
/// visual way to define logic behavior that can be used for scripting.
///
class OrchestratorEditorGraphPanel : public GraphEdit {
    friend class OrchestratorEditorGraphPanelKnotEditor;

    using KnotHelper = OrchestratorEditorGraphPanelKnotEditor;

    GDCLASS(OrchestratorEditorGraphPanel, GraphEdit);

public:

    struct NodeSpawnOptions {
        StringName node_class;
        OrchestratorEditorGraphPin* drag_pin = nullptr;
        OScriptNodeInitContext context;
        Vector2 position;
        bool select_on_spawn = false;
        bool center_on_spawn = false;
    };

private:

    enum GraphNodeAlignment {
        ALIGN_TOP,
        ALIGN_MIDDLE,
        ALIGN_BOTTOM,
        ALIGN_LEFT,
        ALIGN_CENTER,
        ALIGN_RIGHT,
        ALIGN_MAX
    };

    struct ScopedThemeGuard {
        bool& flag;

        explicit ScopedThemeGuard(bool& p_flag) : flag(p_flag) { flag = true; }
        ~ScopedThemeGuard() { flag = false; }
    };

    struct ThemeCache {
        Ref<Font> label_font;
        Ref<Font> label_bold_font;
        Ref<StyleBox> panel;
    } _theme_cache;

    struct PinHandle {
        uint64_t node_id;
        int32_t pin_port;
    };

    struct CopyItem {
        int id;
        Ref<OrchestrationGraphNode> node;
        Vector2 position;
        Vector2 size;
    };

    struct CopyBuffer {
        List<CopyItem> nodes;
        List<uint64_t> connections;
        Orchestration* orchestration = nullptr;
        HashSet<StringName> variable_names;
        HashSet<StringName> function_names;
        HashSet<StringName> signal_names;

        bool is_empty() const {
            return nodes.is_empty();
        }
    };

    static CopyBuffer _copy_buffer;

    GodotVersionInfo _godot_version;

    Ref<OrchestrationGraph> _graph;
    Ref<OrchestratorEditorGraphPanelStyler> _styler;

    // Defines as a weak reference so that in the event the graph is redrawn or if the pin is
    // no longer valid, any future use will return null if the pin object no longer exists.
    WeakRef<OrchestratorEditorGraphPin> _drag_from_pin;

    KnotHelper* _knot_editor = nullptr;

    HFlowContainer* _toolbar_hflow = nullptr;
    Control* _center_status = nullptr;
    Label* _drag_hint = nullptr;
    Timer* _drag_hint_timer = nullptr;
    Timer* _theme_update_timer = nullptr;
    Timer* _idle_timer = nullptr;
    OptionButton* _grid_pattern = nullptr;
    Dictionary _hovered_connection;

    bool _in_theme_update = false;
    bool _show_type_icons = true;
    bool _show_advanced_tooltips = false;
    bool _resizable_by_default = true;
    bool _show_overlay_action_tooltips = true;
    bool _disconnect_control_flow_when_dragged = true;
    bool _moving_selection = false;
    bool _pending_nodes_changed_event = false;
    bool _edited = false;
    bool _treat_call_member_as_override = false;

    bool _box_selection;
    Vector2 _box_selection_from;

    float _idle_time = 0.f;
    float _idle_time_with_errors = 0.f;

    Vector2 _menu_position;

    HashMap<int, bool> _breakpoint_state;
    PackedInt64Array _breakpoints;
    int _breakpoints_index = -1;

    PackedInt64Array _bookmarks;
    int _bookmarks_index = -1;

protected:
    static void _bind_methods();

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

    GodotVersionInfo& get_godot_version() { return _godot_version; }

    //~ Begin GraphEdit Signals
    void _child_entered_tree(Node* p_node);
    void _child_exiting_tree(Node* p_node);
    void _connection_from_empty(const StringName& p_name, int p_port, const Vector2& p_position);
    void _connection_to_empty(const StringName& p_name, int p_port, const Vector2& p_position);
    void _connection_request(const StringName& p_from, int p_from_port, const StringName& p_to, int p_to_port);
    void _disconnection_request(const StringName& p_from, int p_from_port, const StringName& p_to, int p_to_port);
    void _popup_request(const Vector2& p_position);
    void _node_selected(Node* p_node);
    void _node_deselected(Node* p_node);
    void _delete_nodes_request(const PackedStringArray& p_names);
    void _connection_drag_started(const StringName& p_from, int p_port, bool p_output);
    void _connection_drag_ended();
    void _copy_nodes_request();
    void _cut_nodes_request();
    void _duplicate_nodes_request();
    void _paste_nodes_request();
    void _begin_node_move();
    void _end_node_move();
    void _scroll_offset_changed(const Vector2& p_scroll_offset);
    void _update_panel_hint();
    //~ End GraphEdit Signals

    //~ Begin OrchestratorEditorGraphNode Signals
    void _connect_graph_node_pin_signals(OrchestratorEditorGraphNode* p_node);
    void _disconnect_graph_node_pin_signals(OrchestratorEditorGraphNode* p_node);
    void _double_click_node_jump_request(OrchestratorEditorGraphNode* p_node);
    void _show_node_context_menu(OrchestratorEditorGraphNode* p_node, const Vector2& p_position);
    void _node_position_changed(const Vector2& p_old_position, const Vector2& p_new_position, OrchestratorEditorGraphNode* p_node);
    void _node_resized(OrchestratorEditorGraphNode* p_node);
    void _node_resize_end(const Vector2& p_size, OrchestratorEditorGraphNode* p_node);
    //~ End OrchestratorEditorGraphNode Signals

    //~ Begin OrchestratorEditorGraphPin Signals
    void _show_pin_context_menu(OrchestratorEditorGraphPin* p_pin, const Vector2& p_position);
    void _pin_default_value_changed(OrchestratorEditorGraphPin* p_pin, const Variant& p_value);
    //~ End OrchestratorEditorGraphPin Signals

    //~ Begin OrchestrationGraph Signals
    void _node_added(int p_node_id);
    void _node_removed(int p_node_id);
    void _graph_changed();
    //~ End OrchestrationGraph Signals

    //~ Begin KnotEditor Signals
    void _knots_changed();
    //~ End KnotEditor Signals

    void _clear_copy_buffer();
    void _toggle_resizer_for_selected_nodes();
    void _resize_node_to_content();
    void _refresh_selected_nodes();
    void _add_node_pin(OrchestratorEditorGraphNode* p_node);
    void _expand_node(OrchestratorEditorGraphNode* p_node);
    void _collapse_selected_nodes_to_function();
    bool _create_new_function(const String& p_name, bool p_has_return = false);
    bool _create_new_function_override(const MethodInfo& p_method);
    void _create_call_to_parent_function(OrchestratorEditorGraphNode* p_node);
    void _align_nodes(OrchestratorEditorGraphNode* p_anchor, int p_alignment);
    void _toggle_node_bookmark(OrchestratorEditorGraphNode* p_node);
    bool _has_breakpoint_support() const;
    void _toggle_node_breakpoint(OrchestratorEditorGraphNode* p_node);
    void _set_node_breakpoint(OrchestratorEditorGraphNode* p_node, bool p_breaks);
    void _set_node_breakpoint_enabled(OrchestratorEditorGraphNode* p_node, bool p_enabled);
    void _set_variable_node_validation(OrchestratorEditorGraphNode* p_node, bool p_validated);

    void _select_connected_execution_pins(OrchestratorEditorGraphPin* p_pin);
    void _remove_node_pin(OrchestratorEditorGraphPin* p_pin);
    void _change_node_pin_type(OrchestratorEditorGraphPin* p_pin, int p_type);
    bool _can_promote_pin_to_variable(OrchestratorEditorGraphPin* p_pin);
    void _promote_pin_to_variable(OrchestratorEditorGraphPin* p_pin);
    void _reset_pin_to_generated_default_value(OrchestratorEditorGraphPin* p_pin);
    void _view_documentation(const String& p_topic);

    void _connect_graph_node_signals(OrchestratorEditorGraphNode* p_node);
    void _disconnect_graph_node_signals(OrchestratorEditorGraphNode* p_node);

    OrchestratorEditorGraphPin* _resolve_pin_from_handle(const PinHandle& p_handle, bool p_input);

    void _connect_with_menu(const PinHandle& p_handle, const Vector2& p_position, bool p_input);
    void _popup_menu(const Vector2& p_position);
    void _action_menu_selection(const Ref<OrchestratorEditorActionDefinition>& p_action);
    void _action_menu_canceled();

    void _idle_timeout();
    void _grid_pattern_changed(int p_index);
    void _settings_changed();
    void _show_drag_hint(const String& p_hint_text) const;
    bool _is_delete_confirmation_enabled();
    bool _can_duplicate_nodes(const Vector<OrchestratorEditorGraphNode*>& p_nodes, bool p_error_dialog = true);
    void _set_scroll_offset_and_zoom(const Vector2& p_scroll_offset, float p_zoom = 1.f, const Callable& p_callback = Callable());

    void _queue_autowire(OrchestratorEditorGraphNode* p_spawned_node, OrchestratorEditorGraphPin* p_origin_pin);

    Vector2 _get_center() const;

    virtual void _update_theme_item_cache();
    virtual void _update_menu_theme();

    virtual void _refresh_panel_with_model();
    virtual void _refresh_panel_connections_with_model();

    void _update_box_selection_state(const Ref<InputEvent>& p_event);

    void _drop_data_files(const String& p_node_type, const Array& p_files, const Vector2& p_at_position);
    void _drop_data_files_as_exported_variables(const Array& p_files);
    void _drop_data_property(const Dictionary& p_property, const Vector2& p_at_position, const String& p_path, bool p_setter);
    void _drop_data_function(const Dictionary& p_function, const Vector2& p_at_position, bool p_as_callable);
    void _drop_data_variable(const String& p_name, const Vector2& p_at_position, bool p_validated, bool p_setter);

    bool _is_in_port_hotzone(const Vector2& p_pos, const Vector2& p_mouse_pos, const Vector2i& p_port_size, bool p_left);

    void _set_edited(bool p_edited);

    void _get_graph_node_and_port(const Vector2& p_position, int& r_id, int& r_port_index) const;
    bool _is_point_inside_node(const Vector2& p_point) const;

    void _disconnect_connection(const Dictionary& p_connection);
    void _create_connection_reroute(const Dictionary& p_connection, const Vector2& p_position);

public:
    //~ Begin Control Interface
    void _gui_input(const Ref<InputEvent>& p_event) override;
    bool _can_drop_data(const Vector2& p_at_position, const Variant& p_data) const override;
    void _drop_data(const Vector2& p_at_position, const Variant& p_data) override;
    //~ End Control Interface

    //~ Begin GraphEdit Interface
    PackedVector2Array _get_connection_line(const Vector2& p_from_position, const Vector2& p_to_position) const override;
    bool _is_node_hover_valid(const StringName& p_from_node, int32_t p_from_port, const StringName& p_to_node, int32_t p_to_port) override;
    bool _is_in_input_hotzone(Object* p_in_node, int32_t p_in_port, const Vector2& p_mouse_position) override;
    bool _is_in_output_hotzone(Object* p_in_node, int32_t p_in_port, const Vector2& p_mouse_position) override;
    //~ End GraphEdit Interface

    #if GODOT_VERSION < 0x040300
    Dictionary get_closest_connection_at_point(const Vector2& p_position, float p_max_distance = 4.0f);
    #endif

    void set_graph(const Ref<OrchestrationGraph>& p_graph);
    void reloaded_from_file();

    Control* get_menu_control() const;
    Node* get_connection_layer_node() const;

    bool is_bookmarked(const OrchestratorEditorGraphNode* p_node) const;
    void set_bookmarked(OrchestratorEditorGraphNode* p_node, bool p_bookmarked);
    void goto_next_bookmark();
    void goto_previous_bookmark();

    bool is_breakpoint(const OrchestratorEditorGraphNode* p_node) const;
    void set_breakpoint(OrchestratorEditorGraphNode* p_node, bool p_breakpoint);
    bool get_breakpoint(OrchestratorEditorGraphNode* p_node);
    void goto_next_breakpoint();
    void goto_previous_breakpoint();
    PackedInt32Array get_breakpoints() const;
    void clear_breakpoints();

    void show_override_function_action_menu(const Callable& p_callback = Callable());

    bool are_pins_compatible(OrchestratorEditorGraphPin* p_source, OrchestratorEditorGraphPin* p_target) const;

    void link(OrchestratorEditorGraphPin* p_source, OrchestratorEditorGraphPin* p_target);
    void unlink(OrchestratorEditorGraphPin* p_source, OrchestratorEditorGraphPin* p_target);
    void unlink_all(OrchestratorEditorGraphPin* p_target, bool p_notify = false);
    void unlink_node_all(OrchestratorEditorGraphNode* p_node);

    HashSet<OrchestratorEditorGraphNode*> get_connected_nodes(OrchestratorEditorGraphNode* p_node);
    HashSet<OrchestratorEditorGraphPin*> get_connected_pins(OrchestratorEditorGraphPin* p_pin);

    OrchestratorEditorGraphNode* find_node(int p_id);
    OrchestratorEditorGraphNode* find_node(const StringName& p_name);

    void remove_node(OrchestratorEditorGraphNode* p_node, bool p_confirm = true);
    void remove_nodes(const TypedArray<OrchestratorEditorGraphNode>& p_nodes, bool p_confirm = true);

    void clear_selections();
    void select_nodes(const PackedInt64Array& p_ids);

    int64_t get_selection_count();

    Rect2 get_bounds_for_nodes(bool p_only_selected, bool p_padding = 0.f);
    Rect2 get_bounds_for_nodes(const Vector<OrchestratorEditorGraphNode*>& p_nodes, bool p_padding = 0.f);

    void scroll_to_position(const Vector2& p_position, float p_time = 0.2f);

    void center_node_id(int p_id);
    void center_node(OrchestratorEditorGraphNode* p_node);

    template <typename T, typename P> Vector<T*> predicate_find(P&& p_predicate);
    template <typename T, typename F> void for_each(F&& p_function, bool p_selected = false);
    template <typename T> Vector<T*> get_selected();
    template <typename T> Vector<T*> get_all(bool p_only_selected);

    template <typename NodeType> OrchestratorEditorGraphNode* spawn_node(NodeSpawnOptions& p_options);
    OrchestratorEditorGraphNode* spawn_node(const NodeSpawnOptions& p_options);

    void validate();

    // Editor State API
    Variant get_edit_state() const;
    void set_edit_state(const Variant& p_state, const Callable& p_completion_callback);

    OrchestratorEditorGraphPanel();
    ~OrchestratorEditorGraphPanel() override;
};

using NodeSpawnOptions = OrchestratorEditorGraphPanel::NodeSpawnOptions;

template <typename T, typename P>
_FORCE_INLINE_ Vector<T*> OrchestratorEditorGraphPanel::predicate_find(P&& p_predicate) {
    Vector<T*> results;
    for (int i = 0; i < get_child_count(); i++) {
        T* object = cast_to<T>(get_child(i));
        if (object && p_predicate(object)) {
            results.push_back(object);
        }
    }
    return results;
}

template <typename T, typename F>
_FORCE_INLINE_ void OrchestratorEditorGraphPanel::for_each(F&& p_function, bool p_selected) {
    for (int i = 0; i < get_child_count(); i++) {
        if (T* object = cast_to<T>(get_child(i))) {
            if ((p_selected && object->is_selected()) || !p_selected) {
                p_function(object);
            }
        }
    }
}

template <typename T>
_FORCE_INLINE_ Vector<T*> OrchestratorEditorGraphPanel::get_selected() {
    Vector<T*> selected;
    for (int i = 0; i < get_child_count(); i++) {
        T* selectable = cast_to<T>(get_child(i));
        if (selectable && selectable->is_selected()) {
            selected.push_back(selectable);
        }
    }
    return selected;
}

template <typename T>
_FORCE_INLINE_ Vector<T*> OrchestratorEditorGraphPanel::get_all(bool p_only_selected) {
    Vector<T*> objects;
    for (int i = 0; i < get_child_count(); i++) {
        T* child = cast_to<T>(get_child(i));
        if (child && (!p_only_selected || (p_only_selected && child->is_selected()))) {
            objects.push_back(child);
        }
    }
    return objects;
}

template <typename NodeType>
_FORCE_INLINE_ OrchestratorEditorGraphNode* OrchestratorEditorGraphPanel::spawn_node(NodeSpawnOptions& p_options) {
    p_options.node_class = NodeType::get_class_static();
    return spawn_node(p_options);
}


#endif // ORCHESTRATOR_EDITOR_GRAPH_PANEL_H
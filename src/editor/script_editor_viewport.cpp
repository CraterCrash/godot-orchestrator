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
#include "editor/script_editor_viewport.h"

#include "api/extension_db.h"
#include "common/name_utils.h"
#include "editor/component_panels/functions_panel.h"
#include "editor/component_panels/graphs_panel.h"
#include "editor/component_panels/macros_panel.h"
#include "editor/component_panels/signals_panel.h"
#include "editor/component_panels/variables_panel.h"
#include "editor/graph/graph_edit.h"
#include "plugins/orchestrator_editor_plugin.h"
#include "script/nodes/functions/call_script_function.h"
#include "script/nodes/functions/event.h"
#include "script/nodes/functions/function_result.h"
#include "script/script.h"

#include <godot_cpp/classes/scene_tree.hpp>

void OrchestratorScriptEditorViewport::_update_components()
{
    _graphs->update();
    _functions->update();
    _macros->update();
    _variables->update();
    _signals->update();
}

bool OrchestratorScriptEditorViewport::_can_graph_be_closed(OrchestratorGraphEdit* p_graph)
{
    // The event graph cannot be closed
    return p_graph != _event_graph;
}

void OrchestratorScriptEditorViewport::_focus_object(Object* p_object)
{
    Ref<OScriptFunction> function = Object::cast_to<OScriptFunction>(p_object);
    if (function.is_valid())
    {
        if (OrchestratorGraphEdit* graph = _get_or_create_tab(function->get_function_name(), true, true))
            graph->focus_node(function->get_owning_node_id());
    }
}

void OrchestratorScriptEditorViewport::_graph_opened(OrchestratorGraphEdit* p_graph)
{
    OrchestratorEditorViewport::_graph_opened(p_graph);

    p_graph->connect("collapse_selected_to_function", callable_mp(this, &OrchestratorScriptEditorViewport::_collapse_selected_to_function).bind(p_graph));
    p_graph->connect("expand_node", callable_mp(this, &OrchestratorScriptEditorViewport::_expand_node).bind(p_graph));
}

void OrchestratorScriptEditorViewport::_save_state()
{
    Ref<OrchestratorEditorCache> cache = OrchestratorPlugin::get_singleton()->get_editor_cache();
    if (cache.is_valid())
    {
        Dictionary state;

        PackedStringArray open_graphs;
        for (int i = 0; i < _tabs->get_tab_count(); i++)
            open_graphs.push_back(_tabs->get_tab_control(i)->get_name());

        /// For each graph, record its current transient state
        Dictionary graph_states;
        state["graphs"] = graph_states;
        for (const Ref<OScriptGraph>& graph : _orchestration->get_graphs())
        {
            Dictionary graph_state;
            graph_state["viewport_offset"] = graph->get_viewport_offset();
            graph_state["zoom"] = graph->get_viewport_zoom();
            graph_state["open"] = open_graphs.has(graph->get_graph_name());
            graph_state["active"] = _tabs->get_current_tab_control()->get_name().match(graph->get_graph_name());
            graph_states[graph->get_graph_name()] = graph_state;
        }

        // Save panel collapse state
        Dictionary panel_states;
        state["panels"] = panel_states;
        panel_states["graphs"] = _graphs->is_collapsed();
        panel_states["functions"] = _functions->is_collapsed();
        panel_states["macros"] = _macros->is_collapsed();
        panel_states["variables"] = _variables->is_collapsed();
        panel_states["signals"] = _signals->is_collapsed();

        cache->set_script_state(_orchestration->get_self()->get_path(), state);
        cache->save();
    }
}

void OrchestratorScriptEditorViewport::_restore_state()
{
    Ref<OrchestratorEditorCache> cache = OrchestratorPlugin::get_singleton()->get_editor_cache();
    if (cache.is_valid())
    {
        const Dictionary& state = cache->get_script_state(_orchestration->get_self()->get_path());

        // Restores graph state
        if (state.has("graphs"))
        {
            const Dictionary& graphs = state["graphs"];
            for (int i = 0; i < graphs.keys().size(); i++)
            {
                const String graph_name = graphs.keys()[i];
                const Dictionary& graph_data = graphs[graph_name];

                // Restore graph transient data
                Ref<OScriptGraph> graph = _orchestration->find_graph(graph_name);
                if (graph.is_valid())
                {
                    graph->set_viewport_offset(graph_data.get("viewport_offset", Vector2()));
                    graph->set_viewport_zoom(graph_data.get("zoom", 1.0));
                }

                if (graph_data.get("open", false))
                    _get_or_create_tab(graph_name, graph_data.get("active", false), true);
            }
        }

        // Restores panel collapse state
        if (state.has("panels"))
        {
            const Dictionary& panel_state = state["panels"];
            _graphs->set_collapsed(panel_state.get("graphs", false));
            _functions->set_collapsed(panel_state.get("functions", false));
            _macros->set_collapsed(panel_state.get("macros", false));
            _variables->set_collapsed(panel_state.get("variables", false));
            _signals->set_collapsed(panel_state.get("signals", false));
        }
    }
}

Ref<OScriptFunction> OrchestratorScriptEditorViewport::_create_new_function(const String& p_name, bool p_has_return)
{
    ERR_FAIL_COND_V_MSG(_orchestration->has_graph(p_name), {}, "Script already has graph named " + p_name);

    Ref<OScriptGraph> graph = _orchestration->create_graph(p_name, OScriptGraph::GF_FUNCTION | OScriptGraph::GF_DEFAULT);
    ERR_FAIL_COND_V_MSG(!graph.is_valid(), {}, "Failed to create new function graph named " + p_name);

    MethodInfo mi;
    mi.name = p_name;
    mi.flags = METHOD_FLAG_NORMAL;
    mi.return_val.type = Variant::NIL;
    mi.return_val.hint = PROPERTY_HINT_NONE;
    mi.return_val.usage = PROPERTY_USAGE_DEFAULT;

    OScriptNodeInitContext context;
    context.method = mi;

    const Ref<OScriptNodeFunctionEntry> entry = graph->create_node<OScriptNodeFunctionEntry>(context);
    if (!entry.is_valid())
    {
        _orchestration->remove_graph(graph->get_graph_name());
        ERR_FAIL_V_MSG({}, "Failed to create function entry node for function " + p_name);
    }

    if (p_has_return)
    {
        const Vector2 position = entry->get_position() + Vector2(300, 0);
        const Ref<OScriptNodeFunctionResult> result = graph->create_node<OScriptNodeFunctionResult>(context, position);
        if (!result.is_valid())
            ERR_PRINT(vformat("Failed to spawn result node for function '%s'.", p_name));
    }

    _functions->update();

    return entry->get_function();
}

void OrchestratorScriptEditorViewport::_show_graph(const String& p_name)
{
    _get_or_create_tab(p_name, true, true);
}

void OrchestratorScriptEditorViewport::_close_graph(const String& p_name)
{
    const int tab_index = _get_tab_index_by_name(p_name);
    if (tab_index >= 0)
        _close_tab(tab_index);
}

void OrchestratorScriptEditorViewport::_graph_renamed(const String& p_old_name, const String& p_new_name)
{
    _rename_tab(p_old_name, p_new_name);
}

void OrchestratorScriptEditorViewport::_focus_node(const String& p_graph_name, int p_node_id)
{
    if (OrchestratorGraphEdit* graph = _get_or_create_tab(p_graph_name, true, true))
        graph->focus_node(p_node_id);
}

void OrchestratorScriptEditorViewport::_scroll_to_item(TreeItem* p_item)
{
    if (p_item && _scroll_container)
    {
        Tree* tree = p_item->get_tree();

        const Rect2 item_rect = tree->get_item_area_rect(p_item);
        const Rect2 tree_rect = tree->get_global_rect();
        const Rect2 view_rect = _scroll_container->get_rect();

        const float offset = tree_rect.get_position().y + item_rect.get_position().y;
        if (offset > view_rect.get_size().y)
            _scroll_container->set_v_scroll(static_cast<int32_t>(offset));
    }
}

void OrchestratorScriptEditorViewport::_override_godot_function()
{
    OrchestratorGraphEdit* current_graph = _get_current_tab();
    if (!current_graph || !current_graph->is_event_graph())
        current_graph = _get_or_create_tab(EVENT_GRAPH_NAME, false, true);

    if (current_graph)
    {
        current_graph->set_spawn_position_center_view();
        current_graph->show_override_function_action_menu();
    }
}

void OrchestratorScriptEditorViewport::_collapse_selected_to_function(OrchestratorGraphEdit* p_graph)
{
    Vector<Ref<OScriptNode>> selected = p_graph->get_selected_script_nodes();
    if (selected.is_empty())
        return;

    for (const Ref<OScriptNode>& node : selected)
    {
        if (!node->can_duplicate())
        {
            ERR_FAIL_MSG("Cannot collapse because node " + itos(node->get_id()) + " cannot be duplicated.");
        }
    }

    // Capture connections based on the selected nodes
    NodeSetConnections connections;
    _resolve_node_set_connections(selected, connections);

    ERR_FAIL_COND_EDMSG(connections.input_executions > 1, "Cannot collapse to function with more than one external input execution wire.");
    ERR_FAIL_COND_EDMSG(connections.output_executions > 1, "Cannot collapse to function with more than one external output execution wire.");
    ERR_FAIL_COND_EDMSG(connections.output_data > 1, "Cannot collapse to function with more than one output data wire.");
    ERR_FAIL_COND_EDMSG(connections.outputs.size() > 2, "Cannot output more than one execution and one data pin.");

    const String new_function_name = NameUtils::create_unique_name("NewFunction", _orchestration->get_function_names());
    Ref<OScriptFunction> function = _create_new_function(new_function_name, !connections.outputs.is_empty());
    if (!function.is_valid())
        return;

    const Ref<OScriptGraph> source_graph = p_graph->get_owning_graph();
    const Ref<OScriptGraph> target_graph = function->get_function_graph();

    // Calculate the area of the original nodes
    const Rect2 area = _get_node_set_rect(selected);

    // Before we move the nodes, we need to severe their connections to the outside world
    for (const OScriptConnection& E : connections.inputs)
        source_graph->unlink(E.from_node, E.from_port, E.to_node, E.to_port);
    for (const OScriptConnection& E : connections.outputs)
        source_graph->unlink(E.from_node, E.from_port, E.to_node, E.to_port);

    // Move node between the two graphs
    for (const Ref<OScriptNode>& E : selected)
        source_graph->move_node_to(E, target_graph);

    OScriptNodeInitContext context;
    context.method = function->get_method_info();

    Ref<OScriptNode> call_node = source_graph->create_node<OScriptNodeCallScriptFunction>(context, area.get_center());

    const Ref<OScriptNodeFunctionEntry> entry = _orchestration->get_node(function->get_owning_node_id());
    const Ref<OScriptNodeFunctionResult> result = function->get_return_node();

    int input_index = 1;
    int call_input_index = 1;
    bool input_execution_wired = false;
    bool call_execution_wired = false;
    bool entry_positioned = false;
    for (const OScriptConnection& E : connections.inputs)
    {
        // The exterior node connected to the selected node
        const Ref<OScriptNode> source = _orchestration->get_node(E.from_node);
        const Ref<OScriptNodePin> source_pin = source->find_pins(PD_Output)[E.from_port];
        if (source_pin->is_execution() && !call_execution_wired)
        {
            source_graph->link(E.from_node, E.from_port, call_node->get_id(), 0);
            call_execution_wired = true;
        }
        else if (!source_pin->is_execution())
        {
            source_graph->link(E.from_node, E.from_port, call_node->get_id(), call_input_index++);
        }

        // The selected node that is connected from the outside
        const Ref<OScriptNode> target = _orchestration->get_node(E.to_node);
        const Ref<OScriptNodePin> target_pin = target->find_pins(PD_Input)[E.to_port];

        if (!entry_positioned)
        {
            entry->set_position(target->get_position() - Vector2(250, 0));
            entry->emit_changed();
            entry_positioned = true;
        }

        if (!target_pin->is_execution())
        {
            const size_t size = function->get_argument_count() + 1;
            function->resize_argument_list(size);

            PropertyInfo property = target_pin->get_property_info();
            if (!target_pin->get_label().is_empty() && property.name != target_pin->get_label())
                property.name = target_pin->get_label();

            PackedStringArray names;
            for (const PropertyInfo& argument : function->get_method_info().arguments)
                if (!names.has(argument.name))
                    names.push_back(argument.name);

            if (names.has(property.name))
                property.name = NameUtils::create_unique_name(property.name, names);

            function->set_argument(size - 1, property);

            // Wire entry data output to this connection
            target_graph->link(entry->get_id(), input_index++, E.to_node, E.to_port);
        }
        else if (!input_execution_wired)
        {
            // Wire entry execution output to this connection
            target_graph->link(entry->get_id(), 0, E.to_node, E.to_port);
            input_execution_wired = true;
        }
    }

    if(result.is_valid())
    {
        bool output_execution_wired = false;
        bool output_data_wired = false;
        bool positioned = false;
        for (const OScriptConnection& E : connections.outputs)
        {
            // The selected node that is connected from the ouside world
            const Ref<OScriptNode> source = _orchestration->get_node(E.from_node);
            const Ref<OScriptNodePin> source_pin = source->find_pins(PD_Output)[E.from_port];

            if (!positioned)
            {
                result->set_position(source->get_position() + Vector2(250, 0));
                result->emit_changed();
                positioned = true;
            }

            if (source_pin->is_execution() && !output_execution_wired) // Connect execution
            {
                target_graph->link(E.from_node, E.from_port, result->get_id(), 0);
                output_execution_wired = true;
            }
            else if (!source_pin->is_execution() && !output_data_wired) // Connect data
            {
                function->set_has_return_value(true);
                function->set_return_type(source_pin->get_type());

                target_graph->link(E.from_node, E.from_port, result->get_id(), 1);
                output_data_wired = true;
            }
        }

        const Ref<OScriptNodePin> result_exec = result->find_pin(0, PD_Input);
        if (result_exec.is_valid() && !result_exec->has_any_connections())
        {
            const Ref<OScriptNodePin> entry_exec = entry->find_pin(0, PD_Output);
            if (entry_exec.is_valid() && !entry_exec->has_any_connections())
            {
                entry_exec->link(result_exec);
                if (entry->find_pins(PD_Output).size() == 1)
                {
                    entry->set_position(result->get_position() - Vector2(250, 0));
                    entry->emit_changed();
                }
            }
        }
    }

    // Wire call node
    int call_output_index = 1;
    call_execution_wired = false;
    for (const OScriptConnection& E : connections.outputs)
    {
        // The exterior node connected to the selected node
        const Ref<OScriptNode> target = _orchestration->get_node(E.to_node);
        const Ref<OScriptNodePin> target_pin = target->find_pins(PD_Input)[E.to_port];
        if (target_pin->is_execution() && !call_execution_wired)
        {
            source_graph->link(call_node->get_id(), 0, E.to_node, E.to_port);
            call_execution_wired = true;
        }
        else if (!target_pin->is_execution())
        {
            source_graph->link(call_node->get_id(), call_output_index++, E.to_node, E.to_port);
        }
    }

    call_node->emit_changed();

    _functions->find_and_edit(function->get_function_name());
}

void OrchestratorScriptEditorViewport::_expand_node(int p_node_id, OrchestratorGraphEdit* p_graph)
{
    const Ref<OScriptNodeCallScriptFunction> call_node = _orchestration->get_node(p_node_id);
    if (!call_node.is_valid())
        return;

    const Ref<OScriptFunction> function = call_node->get_function();
    if (!function.is_valid())
        return;

    const Ref<OScriptGraph> function_graph = function->get_function_graph();

    Vector<Ref<OScriptNode>> selected;
    for (const Ref<OScriptNode>& graph_node : function_graph->get_nodes())
    {
        Ref<OScriptNodeFunctionEntry> entry = graph_node;
        Ref<OScriptNodeFunctionResult> result = graph_node;
        if (!entry.is_valid() && !result.is_valid() && graph_node->can_duplicate())
            selected.push_back(graph_node);
    }

    if (selected.is_empty())
        return;

    const Rect2 area = _get_node_set_rect(selected);
    const Vector2 pos_delta = call_node->get_position() - area.get_center();

    HashMap<int, int> node_remap;
    for (const Ref<OScriptNode>& node : selected)
    {
        const Ref<OScriptNode> duplicate = p_graph->get_owning_graph()->duplicate_node(
            node->get_id(),
            pos_delta,
            true);

        // Record mapping between old and new nodes
        node_remap[node->get_id()] = duplicate->get_id();
    }

    // Record connections
    NodeSetConnections connections;
    _resolve_node_set_connections(selected, connections);

    // Reapply connections among pasted nodes
    for (const OScriptConnection& E : connections.connections)
        p_graph->get_owning_graph()->link(node_remap[E.from_node], E.from_port, node_remap[E.to_node], E.to_port);

    // Remove call node
    p_graph->get_orchestration()->remove_node(call_node->get_id());
}

void OrchestratorScriptEditorViewport::apply_changes()
{
    OrchestratorEditorViewport::apply_changes();
    _save_state();
}

void OrchestratorScriptEditorViewport::add_script_function(Object* p_object, const String& p_function_name, const PackedStringArray& p_args)
{
    // Check whether a function already exists with the given name
    if (_orchestration->has_function(p_function_name))
    {
        // This could be the user relinking an existing function to a signal.
        // In this case the component viewport needs a redraw.
        _update_components();
        return;
    }

    OrchestratorPlugin::get_singleton()->make_active();

    MethodInfo method;
    method.name = p_function_name;
    method.return_val.type = Variant::NIL; // Signals do not have return values

    for (const String& argument : p_args)
    {
        PackedStringArray bits = argument.split(":");

        if (ClassDB::get_class_list().has(bits[1]))
        {
            // Type represents a registered class.
            PropertyInfo property;
            property.name = bits[0];
            property.class_name = bits[1];
            property.type = Variant::OBJECT;
            method.arguments.push_back(property);
        }
        else if (ExtensionDB::get_builtin_type_names().has(bits[1]))
        {
            // Built-In Type
            PropertyInfo property;
            property.name = bits[0];
            property.type = ExtensionDB::get_builtin_type(bits[1]).type;
            method.arguments.push_back(property);
        }
        else
        {
            ERR_PRINT("Failed to resolve argument type for argument '" + argument + "'.  Function not added.");
            return;
        }
    }

    OScriptNodeInitContext context;
    context.method = method;

    OrchestratorGraphEdit* event_graph = _get_or_create_tab(EVENT_GRAPH_NAME, true, false);
    if (!event_graph)
        return;

    const Ref<OScriptNodeEvent> node = event_graph->get_owning_graph()->create_node<OScriptNodeEvent>(context);
    if (node.is_valid())
    {
        _update_components();
        event_graph->focus_node(node->get_id());
    }
}

void OrchestratorScriptEditorViewport::_notification(int p_what)
{
    #if GODOT_VERSION < 0x040202
    OrchestratorEditorViewport::_notification(p_what);
    #endif

    if (p_what == NOTIFICATION_READY)
    {
        _graphs = memnew(OrchestratorScriptGraphsComponentPanel(_orchestration));
        _graphs->connect("show_graph_requested", callable_mp(this, &OrchestratorScriptEditorViewport::_show_graph));
        _graphs->connect("close_graph_requested", callable_mp(this, &OrchestratorScriptEditorViewport::_close_graph));
        _graphs->connect("focus_node_requested", callable_mp(this, &OrchestratorScriptEditorViewport::_focus_node));
        _graphs->connect("graph_renamed", callable_mp(this, &OrchestratorScriptEditorViewport::_graph_renamed));
        _graphs->connect("scroll_to_item", callable_mp( this, &OrchestratorScriptEditorViewport::_scroll_to_item));
        _component_container->add_child(_graphs);

        Callable create_func = callable_mp(this, &OrchestratorScriptEditorViewport::_create_new_function);
        _functions = memnew(OrchestratorScriptFunctionsComponentPanel(_orchestration, create_func));
        _functions->connect("show_graph_requested", callable_mp(this, &OrchestratorScriptEditorViewport::_show_graph));
        _functions->connect("close_graph_requested", callable_mp(this, &OrchestratorScriptEditorViewport::_close_graph));
        _functions->connect("focus_node_requested", callable_mp(this, &OrchestratorScriptEditorViewport::_focus_node));
        _functions->connect("override_function_requested", callable_mp(this, &OrchestratorScriptEditorViewport::_override_godot_function));
        _functions->connect("graph_renamed", callable_mp(this, &OrchestratorScriptEditorViewport::_graph_renamed));
        _functions->connect("scroll_to_item", callable_mp(this, &OrchestratorScriptEditorViewport::_scroll_to_item));
        _component_container->add_child(_functions);

        _macros = memnew(OrchestratorScriptMacrosComponentPanel(_orchestration));
        _macros->connect("scroll_to_item", callable_mp(this, &OrchestratorScriptEditorViewport::_scroll_to_item));
        _component_container->add_child(_macros);

        _variables = memnew(OrchestratorScriptVariablesComponentPanel(_orchestration));
        _variables->connect("scroll_to_item", callable_mp(this, &OrchestratorScriptEditorViewport::_scroll_to_item));
        _component_container->add_child(_variables);

        _signals = memnew(OrchestratorScriptSignalsComponentPanel(_orchestration));
        _signals->connect("scroll_to_item", callable_mp(this, &OrchestratorScriptEditorViewport::_scroll_to_item));
        _component_container->add_child(_signals);

        // Always open the event graph
        _event_graph = _get_or_create_tab(EVENT_GRAPH_NAME);

        _update_components();
        _restore_state();
    }
    else if (p_what == NOTIFICATION_EXIT_TREE)
        _save_state();
}

void OrchestratorScriptEditorViewport::_bind_methods()
{
}

OrchestratorScriptEditorViewport::OrchestratorScriptEditorViewport(const Ref<OScript>& p_script) : OrchestratorEditorViewport(p_script)
{
    if (p_script.is_valid())
        _orchestration = p_script->get_orchestration();
}

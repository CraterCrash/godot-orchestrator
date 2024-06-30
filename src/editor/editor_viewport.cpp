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
#include "editor/editor_viewport.h"

#include "common/scene_utils.h"
#include "editor/graph/graph_edit.h"
#include "orchestration/orchestration.h"
#include "plugins/orchestrator_editor_debugger_plugin.h"
#include "plugins/orchestrator_editor_plugin.h"

#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

Rect2 OrchestratorEditorViewport::_get_node_set_rect(const Vector<Ref<OScriptNode>>& p_nodes)
{
    if (p_nodes.is_empty())
        return Rect2();

    Rect2 area(p_nodes[0]->get_position(), Vector2());
    for (const Ref<OScriptNode>& E : p_nodes)
        area.expand_to(E->get_position());

    return area;
}

void OrchestratorEditorViewport::_graph_opened(OrchestratorGraphEdit* p_graph)
{
    p_graph->connect("nodes_changed", callable_mp(this, &OrchestratorEditorViewport::_graph_nodes_changed));
    p_graph->connect("focus_requested", callable_mp(this, &OrchestratorEditorViewport::_graph_focus_requested));
    p_graph->connect("validation_requested", callable_mp(this, &OrchestratorEditorViewport::build).bind(true));
}

void OrchestratorEditorViewport::_resolve_node_set_connections(const Vector<Ref<OScriptNode>>& p_nodes, NodeSetConnections& r_connections)
{
    // Create a map of the nodes
    HashMap<int, Ref<OScriptNode>> node_map;
    for (const Ref<OScriptNode>& node : p_nodes)
    {
        node_map[node->get_id()] = node;

        const Vector<Ref<OScriptNodePin>> inputs = node->find_pins(PD_Input);
        for (const Ref<OScriptNodePin>& input : inputs)
        {
            for (const Ref<OScriptNodePin>& E : input->get_connections())
            {
                if (!p_nodes.has(E->get_owning_node()))
                {
                    if (input->is_execution())
                        r_connections.input_executions++;
                    else
                        r_connections.input_data++;
                }
            }
        }

        const Vector<Ref<OScriptNodePin>> outputs = node->find_pins(PD_Output);
        for (const Ref<OScriptNodePin>& output : outputs)
        {
            for (const Ref<OScriptNodePin>& E : output->get_connections())
            {
                if (!p_nodes.has(E->get_owning_node()))
                {
                    if (output->is_execution())
                        r_connections.output_executions++;
                    else
                        r_connections.output_data++;
                }
            }
        }
    }

    for (const OScriptConnection& E : _orchestration->get_connections())
    {
        if (node_map.has(E.from_node) && node_map.has(E.to_node))
            r_connections.connections.insert(E);

        if (!node_map.has(E.from_node) && node_map.has(E.to_node))
            r_connections.inputs.insert(E);

        if (node_map.has(E.from_node) && !node_map.has(E.to_node))
            r_connections.outputs.insert(E);
    }
}

void OrchestratorEditorViewport::_close_tab(int p_tab_index)
{
    if (OrchestratorGraphEdit* graph = Object::cast_to<OrchestratorGraphEdit>(_tabs->get_tab_control(p_tab_index)))
    {
        if (!_can_graph_be_closed(graph))
            return;

        if (Node* parent = graph->get_parent())
        {
            parent->remove_child(graph);
            memdelete(graph);
        }
    }
}

void OrchestratorEditorViewport::_close_tab_requested(int p_tab_index)
{
    if (p_tab_index >= 0 && p_tab_index < _tabs->get_tab_count())
        _close_tab(p_tab_index);
}

void OrchestratorEditorViewport::_graph_nodes_changed()
{
    _update_components();
}

void OrchestratorEditorViewport::_graph_focus_requested(Object* p_object)
{
    _focus_object(p_object);
}

int OrchestratorEditorViewport::_get_tab_index_by_name(const String& p_name) const
{
    for (int i = 0; i < _tabs->get_tab_count(); ++i)
    {
        if (OrchestratorGraphEdit* graph = Object::cast_to<OrchestratorGraphEdit>(_tabs->get_child(i)))
        {
            if (graph->get_name().match(p_name))
                return i;
        }
    }
    return -1;
}

OrchestratorGraphEdit* OrchestratorEditorViewport::_get_or_create_tab(const StringName& p_name, bool p_focus, bool p_create)
{
    const int tab_index = _get_tab_index_by_name(p_name);
    if (tab_index >= 0)
    {
        // Tab already exists
        if (p_focus)
            _tabs->get_tab_bar()->set_current_tab(tab_index);

        return Object::cast_to<OrchestratorGraphEdit>(_tabs->get_tab_control(tab_index));
    }

    if (!p_create)
        return nullptr;

    const Ref<OScriptGraph> script_graph = _orchestration->get_graph(p_name);
    if (!script_graph.is_valid())
        return nullptr;

    OrchestratorGraphEdit* graph = memnew(OrchestratorGraphEdit(OrchestratorPlugin::get_singleton(), script_graph));
    _tabs->add_child(graph);

    const String tab_icon = graph->is_function() ? "MemberMethod" : "ClassList";
    _tabs->set_tab_icon(_get_tab_index_by_name(p_name), SceneUtils::get_editor_icon(tab_icon));

    _graph_opened(graph);

    if (p_focus)
        _tabs->get_tab_bar()->set_current_tab(_tabs->get_tab_count() - 1);

    return graph;
}

void OrchestratorEditorViewport::_rename_tab(const String& p_old_name, const String& p_new_name)
{
    if (OrchestratorGraphEdit* graph = _get_or_create_tab(p_old_name, false, false))
        graph->set_name(p_new_name);
}

void OrchestratorEditorViewport::_meta_clicked(const Variant& p_meta)
{
    _build_errors_dialog->hide();

    const Dictionary value = JSON::parse_string(String(p_meta));
    if (value.has("goto_node"))
        goto_node(String(value["goto_node"]).to_int());
}

void OrchestratorEditorViewport::apply_changes()
{
    for (const Ref<OScriptNode>& node : _orchestration->get_nodes())
        node->pre_save();

    for (int i = 0; i < _tabs->get_tab_count(); i++)
        if (OrchestratorGraphEdit* graph = Object::cast_to<OrchestratorGraphEdit>(_tabs->get_child(i)))
            graph->apply_changes();

    if (ResourceSaver::get_singleton()->save(_resource, _resource->get_path()) != OK)
        OS::get_singleton()->alert(vformat("Failed to save %s", _resource->get_path()), "Error");

    _update_components();

    for (int i = 0; i < _tabs->get_tab_count(); i++)
        if (OrchestratorGraphEdit* graph = Object::cast_to<OrchestratorGraphEdit>(_tabs->get_child(i)))
            graph->post_apply_changes();

    for (const Ref<OScriptNode>& node : _orchestration->get_nodes())
        node->post_save();
}

void OrchestratorEditorViewport::reload_from_disk()
{
    const Ref<Script> script = _resource;
    ERR_FAIL_COND_MSG(!script.is_valid(), "Cannot reload resource type: " + _resource->get_class());

    script->reload();
}

void OrchestratorEditorViewport::rename(const String& p_new_file_name)
{
    _resource->set_path(p_new_file_name);
}

bool OrchestratorEditorViewport::save_as(const String& p_file_name)
{
    if (ResourceSaver::get_singleton()->save(_resource, p_file_name) != OK)
        return false;

    _resource->set_path(p_file_name);
    return true;
}

bool OrchestratorEditorViewport::is_same_script(const Ref<Script>& p_script) const
{
    const Ref<Script> script = _resource;
    return script.is_valid() && (p_script == script);
}

bool OrchestratorEditorViewport::is_modified() const
{
    return _orchestration->is_edited();
}

bool OrchestratorEditorViewport::build(bool p_show_success)
{
    BuildLog log;
    _orchestration->validate_and_build(log);

    const bool has_errors = log.has_errors() || log.has_warnings();

    _build_errors->clear();
    _build_errors->append_text(vformat("[b]File:[/b] %s\n\n", _resource->get_path()));

    if (has_errors)
    {
        _build_errors_dialog->set_title("Orchestration Build Errors");
        for (const String& E : log.get_messages())
            _build_errors->append_text(vformat("* %s\n", E));

        _build_errors_dialog->popup_centered_ratio(0.5);
        return false;
    }

    if (p_show_success)
    {
        _build_errors_dialog->set_title("Orchestration Validation Results");
        _build_errors->append_text(vformat("* [color=green]OK[/color]: Script is valid."));
        _build_errors_dialog->popup_centered_ratio(0.25);
    }

    return true;
}

#if GODOT_VERSION >= 0x040300
void OrchestratorEditorViewport::clear_breakpoints()
{
    OrchestratorEditorDebuggerPlugin* debugger = OrchestratorEditorDebuggerPlugin::get_singleton();
    for (const Ref<OScriptNode>& node : _orchestration->get_nodes())
    {
        node->set_breakpoint_flag(OScriptNode::BREAKPOINT_NONE);
        debugger->set_breakpoint(_orchestration->get_self()->get_path(), node->get_id(), false);
    }
}

void OrchestratorEditorViewport::set_breakpoint(int p_node_id, bool p_enabled)
{
    const Ref<OScriptNode> node = _orchestration->get_node(p_node_id);
    if (node.is_valid())
        node->set_breakpoint_flag(p_enabled ? OScriptNode::BREAKPOINT_ENABLED : OScriptNode::BREAKPOINT_NONE);
}

PackedStringArray OrchestratorEditorViewport::get_breakpoints() const
{
    PackedStringArray breakpoints;
    for (const Ref<OScriptNode>& node : _orchestration->get_nodes())
    {
        if (node->has_breakpoint())
            breakpoints.push_back(vformat("%s:%d", _orchestration->get_self()->get_path(), node->get_id()));
    }
    return breakpoints;
}
#endif

void OrchestratorEditorViewport::goto_node(int p_node_id)
{
    Ref<OScriptNode> node = _orchestration->get_node(p_node_id);
    if (!node.is_valid())
        return;

    for (const Ref<OScriptGraph>& graph : _orchestration->get_graphs())
    {
        if (graph->has_node(p_node_id))
        {
            if (OrchestratorGraphEdit* graph_edit = _get_or_create_tab(graph->get_graph_name(), true, true))
                graph_edit->focus_node(p_node_id);
            break;
        }
    }
}

void OrchestratorEditorViewport::notify_scene_tab_changed()
{
    _update_components();
}

void OrchestratorEditorViewport::notify_component_panel_visibility_changed(bool p_visible)
{
    if (_scroll_container)
        _scroll_container->set_visible(p_visible);
}

void OrchestratorEditorViewport::_notification(int p_what)
{
    if (p_what == NOTIFICATION_READY)
    {
        VBoxContainer* panel = memnew(VBoxContainer);
        panel->set_h_size_flags(SIZE_EXPAND_FILL);
        add_child(panel);

        MarginContainer* margin = memnew(MarginContainer);
        margin->set_v_size_flags(SIZE_EXPAND_FILL);
        panel->add_child(margin);

        _tabs = memnew(TabContainer);
        _tabs->get_tab_bar()->set_tab_close_display_policy(TabBar::CLOSE_BUTTON_SHOW_ACTIVE_ONLY);
        _tabs->get_tab_bar()->connect("tab_close_pressed", callable_mp(this, &OrchestratorEditorViewport::_close_tab_requested));
        margin->add_child(_tabs);

        _scroll_container = memnew(ScrollContainer);
        _scroll_container->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
        _scroll_container->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
        add_child(_scroll_container);

        _component_container = memnew(VBoxContainer);
        _component_container->set_h_size_flags(SIZE_EXPAND_FILL);
        _scroll_container->add_child(_component_container);

        _build_errors = memnew(RichTextLabel);
        _build_errors->set_use_bbcode(true);
        _build_errors->connect("meta_clicked", callable_mp(this, &OrchestratorEditorViewport::_meta_clicked));

        _build_errors_dialog = memnew(AcceptDialog);
        _build_errors_dialog->set_title("Orchestrator Build Errors");
        _build_errors_dialog->add_child(_build_errors);
        add_child(_build_errors_dialog);
    }
}

void OrchestratorEditorViewport::_bind_methods()
{
}

OrchestratorEditorViewport::OrchestratorEditorViewport(const Ref<Resource>& p_resource) : _resource(p_resource)
{
    set_v_size_flags(SIZE_EXPAND_FILL);
    set_h_size_flags(SIZE_EXPAND_FILL);
}
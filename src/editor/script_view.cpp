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
#include "api/extension_db.h"
#include "common/scene_utils.h"
#include "editor/graph/graph_edit.h"
#include "editor/main_view.h"
#include "plugin/plugin.h"
#include "script/nodes/functions/event.h"
#include "script/nodes/functions/function_entry.h"
#include "script_connections.h"
#include "script_view.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/scene_tree_timer.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>

OrchestratorScriptViewSection::OrchestratorScriptViewSection(const String& p_section_name)
{
    _section_name = p_section_name;
}

void OrchestratorScriptViewSection::_bind_methods()
{
    ADD_SIGNAL(MethodInfo("scroll_to_item", PropertyInfo(Variant::OBJECT, "item")));
}

void OrchestratorScriptViewSection::_notification(int p_what)
{
    if (p_what == NOTIFICATION_READY)
    {
        set_v_size_flags(Control::SIZE_SHRINK_BEGIN);
        set_h_size_flags(Control::SIZE_EXPAND_FILL);
        add_theme_constant_override("separation", 0);
        set_custom_minimum_size(Vector2i(300, 0));

        _panel_hbox = memnew(HBoxContainer);
        _panel_hbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        _panel_hbox->set_tooltip_text(SceneUtils::create_wrapped_tooltip_text(_get_tooltip_text()));

        _collapse_button = memnew(Button);
        _collapse_button->set_focus_mode(Control::FOCUS_NONE);
        _collapse_button->set_flat(true);
        _collapse_button->connect("pressed", callable_mp(this, &OrchestratorScriptViewSection::_toggle));
        _update_collapse_button_icon();
        _panel_hbox->add_child(_collapse_button);

        Label* label = memnew(Label);
        label->set_text(_section_name);
        label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        _panel_hbox->add_child(label);

        Button* add_button = memnew(Button);
        add_button->set_focus_mode(Control::FOCUS_NONE);
        add_button->connect("pressed", callable_mp(this, &OrchestratorScriptViewSection::_on_add_pressed));
        add_button->set_button_icon(SceneUtils::get_editor_icon("Add"));
        add_button->set_tooltip_text("Add a new " + _get_section_item_name());
        _panel_hbox->add_child(add_button);

        _panel = memnew(PanelContainer);
        _panel->set_mouse_filter(Control::MOUSE_FILTER_PASS);
        _panel->add_child(_panel_hbox);
        add_child(_panel);

        _tree = memnew(Tree);
        _tree->set_columns(1);
        _tree->set_allow_rmb_select(true);
        _tree->set_select_mode(Tree::SELECT_ROW);
        _tree->set_h_scroll_enabled(false);
        _tree->set_v_scroll_enabled(false);
        _tree->set_custom_minimum_size(Vector2i(300, 40));
        _tree->set_v_size_flags(SIZE_FILL);
        _tree->set_hide_root(true);
        _tree->set_focus_mode(Control::FOCUS_NONE);
        _tree->set_drag_forwarding(callable_mp(this, &OrchestratorScriptViewSection::_on_tree_drag_data), Callable(), Callable());
        _tree->connect("item_activated", callable_mp(this, &OrchestratorScriptViewSection::_on_item_activated));
        _tree->connect("item_edited", callable_mp(this, &OrchestratorScriptViewSection::_on_item_edited));
        _tree->connect("item_selected", callable_mp(this, &OrchestratorScriptViewSection::_on_item_selected));
        _tree->connect("item_mouse_selected", callable_mp(this, &OrchestratorScriptViewSection::_on_item_mouse_selected));
        _tree->connect("item_collapsed", callable_mp(this, &OrchestratorScriptViewSection::_on_item_collapsed));
        _tree->connect("button_clicked", callable_mp(this, &OrchestratorScriptViewSection::_on_button_clicked));
        _tree->create_item()->set_text(0, "Root"); // creates the root item
        add_child(_tree);

        _context_menu = memnew(PopupMenu);
        _context_menu->connect("id_pressed", callable_mp(this, &OrchestratorScriptViewSection::_on_menu_id_pressed));
        add_child(_context_menu);

        _confirm = memnew(ConfirmationDialog);
        _confirm->set_title("Please confirm...");
        _confirm->connect("confirmed", callable_mp(this, &OrchestratorScriptViewSection::_on_remove_confirmed));
        add_child(_confirm);

        _notify = memnew(AcceptDialog);
        _notify->set_title("Message");
        add_child(_notify);
    }
    else if (p_what == NOTIFICATION_THEME_CHANGED)
    {
        _theme_changing = true;
        callable_mp(this, &OrchestratorScriptViewSection::_update_theme).call_deferred();
    }
}

void OrchestratorScriptViewSection::_gui_input(const Ref<godot::InputEvent>& p_event)
{
    Ref<InputEventMouseButton> mb = p_event;
    if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == MOUSE_BUTTON_LEFT)
    {
        _toggle();
        get_viewport()->set_input_as_handled();
    }
}

void OrchestratorScriptViewSection::update()
{
    if (_expanded)
    {
        // A simple hack to redraw the tree based on content height
        _tree->set_visible(false);
        _tree->set_visible(true);
    }
}

void OrchestratorScriptViewSection::_update_theme()
{
    if (!_theme_changing)
        return;

    Ref<Theme> theme = OrchestratorPlugin::get_singleton()->get_editor_interface()->get_editor_theme();
    if (theme.is_valid() && _panel)
    {
        Ref<StyleBoxFlat> sb = theme->get_stylebox("panel", "ItemList")->duplicate();
        sb->set_corner_radius(CORNER_BOTTOM_LEFT, 0);
        sb->set_corner_radius(CORNER_BOTTOM_RIGHT, 0);
        _panel->add_theme_stylebox_override("panel", sb);
    }

    Ref<StyleBoxFlat> sb = _tree->get_theme_stylebox("panel");
    if (sb.is_valid())
    {
        Ref<StyleBoxFlat> new_style = sb->duplicate();
        new_style->set_corner_radius(CORNER_TOP_LEFT, 0);
        new_style->set_corner_radius(CORNER_TOP_RIGHT, 0);
        _tree->add_theme_stylebox_override("panel", new_style);
    }

    queue_redraw();

    _theme_changing = false;
}

void OrchestratorScriptViewSection::_clear_tree()
{
    _tree->clear();
    _tree->create_item();
}

void OrchestratorScriptViewSection::_update_collapse_button_icon()
{
    const String icon_name = _expanded ? "CodeFoldDownArrow" : "CodeFoldedRightArrow";
    _collapse_button->set_button_icon(SceneUtils::get_editor_icon(icon_name));
}

void OrchestratorScriptViewSection::_toggle()
{
    _expanded = !_expanded;
    _update_collapse_button_icon();
    _tree->set_visible(_expanded);
}

void OrchestratorScriptViewSection::_show_notification(const String& p_message)
{
    _notify->set_text(p_message);
    _notify->reset_size();
    _notify->popup_centered();
}

void OrchestratorScriptViewSection::_confirm_removal(TreeItem* p_item)
{
    _confirm->set_text(_get_remove_confirm_text(p_item));
    _confirm->reset_size();
    _confirm->popup_centered();
}

String OrchestratorScriptViewSection::_create_unique_name_with_prefix(const String& p_prefix)
{
    const PackedStringArray child_names = _get_existing_names();
    if (!child_names.has(p_prefix))
        return p_prefix;

    for (int i = 0; i < std::numeric_limits<int>::max(); i++)
    {
        const String name = vformat("%s_%s", p_prefix, i);
        if (child_names.has(name))
            continue;

        return name;
    }

    return {};
}

bool OrchestratorScriptViewSection::_find_child_and_activate(const String& p_name, bool p_edit)
{
    TreeItem* root = _tree->get_root();

    for (int i = 0; i < root->get_child_count(); i++)
    {
        if (TreeItem* child = Object::cast_to<TreeItem>(root->get_child(i)))
        {
            if (child->get_text(0).match(p_name))
            {
                emit_signal("scroll_to_item", child);
                _tree->call_deferred("set_selected", child, 0);

                if (p_edit)
                {
                    Ref<SceneTreeTimer> timer = get_tree()->create_timer(0.1f);
                    if (timer.is_valid())
                        timer->connect("timeout", callable_mp(_tree, &Tree::edit_selected).bind(true));
                }

                return true;
            }
        }
    }
    return false;
}

HBoxContainer* OrchestratorScriptViewSection::_get_panel_hbox()
{
    return _panel_hbox;
}

void OrchestratorScriptViewSection::_on_add_pressed()
{
    _handle_add_new_item();
}

void OrchestratorScriptViewSection::_on_item_activated()
{
    TreeItem* item = _tree->get_selected();
    ERR_FAIL_COND_MSG(!item, "Cannot activate when no item selected");

    _handle_item_activated(item);
}

void OrchestratorScriptViewSection::_on_item_edited()
{
    TreeItem* item = _tree->get_selected();
    ERR_FAIL_COND_MSG(!item, "Cannot edit item when no item selected");

    const String old_name = item->get_meta("__name");
    const String new_name = item->get_text(0);

    // Nothing to edit if they're identical
    if (old_name.match(new_name))
        return;

    _handle_item_renamed(old_name, new_name);
}

void OrchestratorScriptViewSection::_on_item_selected()
{
    _handle_item_selected();
}

void OrchestratorScriptViewSection::_on_item_mouse_selected(const godot::Vector2& p_position, int p_button)
{
    if (p_button != MouseButton::MOUSE_BUTTON_RIGHT)
        return;

    TreeItem* item = _tree->get_selected();
    if (!item)
        return;

    _context_menu->clear();
    _context_menu->reset_size();

    if (_populate_context_menu(item))
    {
        _context_menu->set_position(_tree->get_screen_position() + p_position);
        _context_menu->reset_size();
        _context_menu->popup();
    }
}

void OrchestratorScriptViewSection::_on_item_collapsed(TreeItem* p_item)
{
    if (_expanded)
    {
        _tree->set_visible(false);
        _tree->set_visible(true);
    }
}

void OrchestratorScriptViewSection::_on_menu_id_pressed(int p_id)
{
    _handle_context_menu(p_id);
}

void OrchestratorScriptViewSection::_on_remove_confirmed()
{
    OrchestratorPlugin::get_singleton()->get_editor_interface()->inspect_object(nullptr);
    _handle_remove(_tree->get_selected());
}

void OrchestratorScriptViewSection::_on_button_clicked(TreeItem* p_item, int p_column, int p_id, int p_mouse_button)
{
    _handle_button_clicked(p_item, p_column, p_id, p_mouse_button);
}

Variant OrchestratorScriptViewSection::_on_tree_drag_data(const Vector2& p_position)
{
    const Dictionary data = _handle_drag_data(p_position);
    if (data.keys().is_empty())
        return {};

    PanelContainer* container = memnew(PanelContainer);
    container->set_anchors_preset(Control::PRESET_TOP_LEFT);
    container->set_v_size_flags(Control::SIZE_SHRINK_BEGIN);

    HBoxContainer* hbc = memnew(HBoxContainer);
    hbc->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
    container->add_child(hbc);

    TextureRect* rect = memnew(TextureRect);
    rect->set_texture(_tree->get_selected()->get_icon(0));
    rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
    rect->set_h_size_flags(Control::SIZE_SHRINK_CENTER);
    rect->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
    hbc->add_child(rect);

    Label* label = memnew(Label);
    label->set_text(_tree->get_selected()->get_text(0));
    hbc->add_child(label);

    set_drag_preview(container);
    return data;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OrchestratorScriptViewGraphsSection::OrchestratorScriptViewGraphsSection(const Ref<OScript>& p_script)
    : OrchestratorScriptViewSection("Graphs")
    , _script(p_script)
{
}

void OrchestratorScriptViewGraphsSection::_bind_methods()
{
    ADD_SIGNAL(MethodInfo("show_graph_requested",
                          PropertyInfo(Variant::STRING, "graph_name")));
    ADD_SIGNAL(MethodInfo("close_graph_requested",
                          PropertyInfo(Variant::STRING, "graph_name")));
    ADD_SIGNAL(MethodInfo("graph_renamed",
                          PropertyInfo(Variant::STRING, "old_name"),
                          PropertyInfo(Variant::STRING, "new_name")));
    ADD_SIGNAL(MethodInfo("focus_node_requested",
                          PropertyInfo(Variant::STRING, "graph_name"),
                          PropertyInfo(Variant::INT, "node_id")));
}

void OrchestratorScriptViewGraphsSection::_show_graph_item(TreeItem* p_item)
{
    const String graph_name = p_item->get_text(0);

    // Graph
    emit_signal("show_graph_requested", graph_name);
    _tree->deselect_all();
}

void OrchestratorScriptViewGraphsSection::_focus_graph_function(TreeItem* p_item)
{
    const String graph_name = p_item->get_parent()->get_text(0);
    const int node_id = _script->get_function_node_id(p_item->get_text(0));

    // Specific event node
    emit_signal("focus_node_requested", graph_name, node_id);
    _tree->deselect_all();
}

void OrchestratorScriptViewGraphsSection::_remove_graph(TreeItem* p_item)
{
    const String graph_name = p_item->get_text(0);
    emit_signal("close_graph_requested", graph_name);

    _script->remove_graph(graph_name);
    update();
}

void OrchestratorScriptViewGraphsSection::_remove_graph_function(TreeItem* p_item)
{
    const String function_name = p_item->get_text(0);

    _script->remove_function(function_name);
    update();
}

PackedStringArray OrchestratorScriptViewGraphsSection::_get_existing_names() const
{
    PackedStringArray result;
    for (const Ref<OScriptGraph>& graph : _script->get_graphs())
        result.push_back(graph->get_graph_name());
    return result;
}

String OrchestratorScriptViewGraphsSection::_get_tooltip_text() const
{
    return "A graph allows you to place many types of nodes to create various behaviors. "
           "Event graphs are flexible and can control multiple event nodes that start execution, "
           "nodes that may take time, react to signals, or call functions and macro nodes.\n\n"
           "While there is always one event graph called \"EventGraph\", you can create new "
           "event graphs to better help organize event logic.";
}

String OrchestratorScriptViewGraphsSection::_get_remove_confirm_text(TreeItem* p_item) const
{
    // Only confirm graphs, not event functions
    if (p_item->get_parent() == _tree->get_root())
        return "Removing a graph removes all nodes within the graph.\nDo you want to continue?";

    return OrchestratorScriptViewSection::_get_remove_confirm_text(p_item);
}

bool OrchestratorScriptViewGraphsSection::_populate_context_menu(TreeItem* p_item)
{
    if (p_item->get_parent() == _tree->get_root())
    {
        // Graph
        Ref<OScriptGraph> graph = _script->get_graph(p_item->get_text(0));
        bool rename_disabled = !graph->get_flags().has_flag(OScriptGraph::GraphFlags::GF_RENAMABLE);
        bool delete_disabled = !graph->get_flags().has_flag(OScriptGraph::GraphFlags::GF_DELETABLE);
        _context_menu->add_item("Open Graph", CM_OPEN_GRAPH);
        _context_menu->add_icon_item(SceneUtils::get_editor_icon("Rename"), "Rename", CM_RENAME_GRAPH);
        _context_menu->set_item_disabled(_context_menu->get_item_count() - 1, rename_disabled);
        _context_menu->add_icon_item(SceneUtils::get_editor_icon("Remove"), "Remove", CM_REMOVE_GRAPH);
        _context_menu->set_item_disabled(_context_menu->get_item_count() - 1, delete_disabled);
    }
    else
    {
        // Graph Functions
        _context_menu->add_item("Focus", CM_FOCUS_FUNCTION);
        _context_menu->add_icon_item(SceneUtils::get_editor_icon("Remove"), "Remove", CM_REMOVE_FUNCTION);
    }
    return true;
}

void OrchestratorScriptViewGraphsSection::_handle_context_menu(int p_id)
{
    switch (p_id)
    {
        case CM_OPEN_GRAPH:
            _show_graph_item(_tree->get_selected());
            break;
        case CM_RENAME_GRAPH:
            _tree->edit_selected(true);
            break;
        case CM_REMOVE_GRAPH:
            _confirm_removal(_tree->get_selected());
            break;
        case CM_FOCUS_FUNCTION:
            _focus_graph_function(_tree->get_selected());
            break;
        case CM_REMOVE_FUNCTION:
            _remove_graph_function(_tree->get_selected());
            break;
    }
}

void OrchestratorScriptViewGraphsSection::_handle_add_new_item()
{
    const String name = _create_unique_name_with_prefix("NewEventGraph");

    // Add the new graph and update the components display
    int flags = OScriptGraph::GF_EVENT | OScriptGraph::GF_DEFAULT;
    Ref<OScriptGraph> graph = _script->create_graph(name, flags);

    update();

    _find_child_and_activate(name);
}

void OrchestratorScriptViewGraphsSection::_handle_item_activated(TreeItem* p_item)
{
    if (p_item->get_parent() == _tree->get_root())
        _show_graph_item(p_item);
    else
        _focus_graph_function(p_item);
}

void OrchestratorScriptViewGraphsSection::_handle_item_renamed(const String& p_old_name, const String& p_new_name)
{
    if (_get_existing_names().has(p_new_name))
    {
        _show_notification("A graph with the name '" + p_new_name + "' already exists.");
        return;
    }

    _script->rename_graph(p_old_name, p_new_name);
    emit_signal("graph_renamed", p_old_name, p_new_name);

    update();
}

void OrchestratorScriptViewGraphsSection::_handle_remove(TreeItem* p_item)
{
    if (p_item->get_parent() == _tree->get_root())
        _remove_graph(p_item);
}

void OrchestratorScriptViewGraphsSection::_handle_button_clicked(TreeItem* p_item, int p_column, int p_id,
                                                                 int p_mouse_button)
{
    const Vector<Node*> nodes = SceneUtils::find_all_nodes_for_script_in_edited_scene(_script);

    OrchestratorScriptConnectionsDialog* dialog = memnew(OrchestratorScriptConnectionsDialog);
    add_child(dialog);
    dialog->popup_connections(p_item->get_text(0), nodes);
}

void OrchestratorScriptViewGraphsSection::update()
{
    _clear_tree();

    Vector<Ref<OScriptGraph>> graphs = _script->get_graphs();
    if (graphs.is_empty())
    {
        TreeItem* item = _tree->get_root()->create_child();
        item->set_text(0, "No graphs defined");
        item->set_selectable(0, false);
        return;
    }

    const Vector<Node*> script_nodes = SceneUtils::find_all_nodes_for_script_in_edited_scene(_script);
    const String base_type = _script->get_instance_base_type();

    const PackedStringArray functions = _script->get_function_names();
    for (const Ref<OScriptGraph>& graph : graphs)
    {
        if (!(graph->get_flags().has_flag(OScriptGraph::GraphFlags::GF_EVENT)))
            continue;

        TreeItem* item = _tree->get_root()->create_child();
        item->set_text(0, graph->get_graph_name());
        item->set_meta("__name", graph->get_graph_name()); // Used for renames
        item->set_icon(0, SceneUtils::get_editor_icon("ClassList"));

        TypedArray<int> nodes = graph->get_nodes();
        for (const String& function_name : functions)
        {
            int function_id = _script->get_function_node_id(function_name);
            if (nodes.has(function_id))
            {
                TreeItem* func = item->create_child();
                func->set_text(0, function_name);
                func->set_icon(0, SceneUtils::get_editor_icon("PlayStart"));

                if (SceneUtils::has_any_signals_connected_to_function(function_name, base_type, script_nodes))
                    func->add_button(0, SceneUtils::get_editor_icon("Slot"));
            }
        }
    }

    OrchestratorScriptViewSection::update();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OrchestratorScriptViewFunctionsSection::OrchestratorScriptViewFunctionsSection(const Ref<OScript>& p_script)
    : OrchestratorScriptViewSection("Functions")
    , _script(p_script)
{
}

void OrchestratorScriptViewFunctionsSection::_bind_methods()
{
    ADD_SIGNAL(MethodInfo("show_graph_requested",
                          PropertyInfo(Variant::STRING, "graph_name")));
    ADD_SIGNAL(MethodInfo("close_graph_requested",
                          PropertyInfo(Variant::STRING, "graph_name")));
    ADD_SIGNAL(MethodInfo("graph_renamed",
                          PropertyInfo(Variant::STRING, "old_name"),
                          PropertyInfo(Variant::STRING, "new_name")));
    ADD_SIGNAL(MethodInfo("focus_node_requested",
                          PropertyInfo(Variant::STRING, "graph_name"),
                          PropertyInfo(Variant::INT, "node_id")));
    ADD_SIGNAL(MethodInfo("override_function_requested"));
}


void OrchestratorScriptViewFunctionsSection::_on_override_virtual_function()
{
    emit_signal("override_function_requested");
}

void OrchestratorScriptViewFunctionsSection::_notification(int p_what)
{
    // Godot does not dispatch to parent (shrugs)
    OrchestratorScriptViewSection::_notification(p_what);

    if (p_what == NOTIFICATION_READY)
    {
        HBoxContainer* container = _get_panel_hbox();

        Button* override_button = memnew(Button);
        override_button->set_focus_mode(Control::FOCUS_NONE);
        override_button->connect(
            "pressed", callable_mp(this, &OrchestratorScriptViewFunctionsSection::_on_override_virtual_function));
        override_button->set_button_icon(SceneUtils::get_editor_icon("Override"));
        override_button->set_tooltip_text("Override a Godot virtual function");
        container->add_child(override_button);
    }
}

void OrchestratorScriptViewFunctionsSection::_show_function_graph(TreeItem* p_item)
{
    // Function name and graph names are synonymous
    const String function_name = p_item->get_text(0);
    emit_signal("show_graph_requested", function_name);
    emit_signal("focus_node_requested", function_name, _script->get_function_node_id(function_name));
    _tree->deselect_all();
}

PackedStringArray OrchestratorScriptViewFunctionsSection::_get_existing_names() const
{
    return _script->get_function_names();
}

String OrchestratorScriptViewFunctionsSection::_get_tooltip_text() const
{
    return "A function graph allows the encapsulation of functionality for re-use. Function graphs have "
           "a single input with an optional output node. Function graphs have a single execution pin "
           "with multiple input data pins and the result node may return a maximum of one data value to "
           "the caller.\n\n"
           "Functions can be called by selecting the action in the action menu or by dragging the "
           "function from this component view onto the graph area.";
}

String OrchestratorScriptViewFunctionsSection::_get_remove_confirm_text(TreeItem* p_item) const
{
    return "Removing a function removes all nodes that participate in the function and any nodes\n"
           "that call that function from the event graphs.\n"
           "Do you want to continue?";
}

bool OrchestratorScriptViewFunctionsSection::_populate_context_menu(TreeItem* p_item)
{
    _context_menu->add_item("Open in Graph", CM_OPEN_FUNCTION_GRAPH);
    _context_menu->add_icon_item(SceneUtils::get_editor_icon("Rename"), "Rename", CM_RENAME_FUNCTION);
    _context_menu->add_icon_item(SceneUtils::get_editor_icon("Remove"), "Remove", CM_REMOVE_FUNCTION);
    return true;
}

void OrchestratorScriptViewFunctionsSection::_handle_context_menu(int p_id)
{
    switch (p_id)
    {
        case CM_OPEN_FUNCTION_GRAPH:
            _show_function_graph(_tree->get_selected());
            break;
        case CM_RENAME_FUNCTION:
            _tree->edit_selected(true);
            break;
        case CM_REMOVE_FUNCTION:
            _confirm_removal(_tree->get_selected());
            break;
    }
}

void OrchestratorScriptViewFunctionsSection::_handle_add_new_item()
{
    const String name = _create_unique_name_with_prefix("NewFunction");

    // User-defined functions are also graphs
    int flags = OScriptGraph::GF_FUNCTION | OScriptGraph::GF_DEFAULT;
    Ref<OScriptGraph> graph = _script->create_graph(name, flags);

    // Create the node
    OScriptLanguage* language = OScriptLanguage::get_singleton();
    Ref<OScriptNode> node = language->create_node_from_type<OScriptNodeFunctionEntry>(_script);

    // Create method information details
    MethodInfo mi;
    mi.name = name;

    // Initialize the node and register it with the script
    OScriptNodeInitContext context;
    context.method = mi;
    node->initialize(context);

    _script->add_node(graph, node);
    node->post_placed_new_node();

    // Link the node to the graph
    graph->add_function(node->get_id());
    graph->add_node(node->get_id());

    update();

    _find_child_and_activate(name);
}

void OrchestratorScriptViewFunctionsSection::_handle_item_activated(TreeItem* p_item)
{
    _show_function_graph(p_item);
}

void OrchestratorScriptViewFunctionsSection::_handle_item_renamed(const String& p_old_name, const String& p_new_name)
{
    if (_get_existing_names().has(p_new_name))
    {
        _show_notification("A function with the name '" + p_new_name + "' already exists.");
        return;
    }

    _script->rename_function(p_old_name, p_new_name);
    emit_signal("graph_renamed", p_old_name, p_new_name);

    update();
}

void OrchestratorScriptViewFunctionsSection::_handle_remove(TreeItem* p_item)
{
    // Function name and graph names are synonymous
    const String function_name = p_item->get_text(0);
    emit_signal("close_graph_requested", function_name);

    _script->remove_function(function_name);
    update();
}

void OrchestratorScriptViewFunctionsSection::_handle_button_clicked(TreeItem* p_item, int p_column, int p_id,
                                                                    int p_mouse_button)
{
    const Vector<Node*> nodes = SceneUtils::find_all_nodes_for_script_in_edited_scene(_script);

    OrchestratorScriptConnectionsDialog* dialog = memnew(OrchestratorScriptConnectionsDialog);
    add_child(dialog);
    dialog->popup_connections(p_item->get_text(0), nodes);
}

Dictionary OrchestratorScriptViewFunctionsSection::_handle_drag_data(const Vector2& p_position)
{
    Dictionary data;

    TreeItem* selected = _tree->get_selected();
    if (selected)
    {
        data["type"] = "function";
        data["functions"] = Array::make(selected->get_text(0));
    }
    return data;
}

void OrchestratorScriptViewFunctionsSection::update()
{
    _clear_tree();

    const Vector<Node*> script_nodes = SceneUtils::find_all_nodes_for_script_in_edited_scene(_script);
    const String base_type = _script->get_instance_base_type();

    for (const Ref<OScriptGraph>& graph : _script->get_graphs())
    {
        if (!(graph->get_flags().has_flag(OScriptGraph::GraphFlags::GF_FUNCTION)))
            continue;

        TreeItem* item = _tree->get_root()->create_child();
        item->set_text(0, graph->get_graph_name());
        item->set_meta("__name", graph->get_graph_name()); // Used for renames
        item->set_icon(0, SceneUtils::get_editor_icon("MemberMethod"));

        if (SceneUtils::has_any_signals_connected_to_function(graph->get_graph_name(), base_type, script_nodes))
            item->add_button(0, SceneUtils::get_editor_icon("Slot"));
    }

    if (_tree->get_root()->get_child_count() == 0)
    {
        TreeItem* item = _tree->get_root()->create_child();
        item->set_text(0, "No functions defined");
        item->set_selectable(0, false);
        return;
    }

    OrchestratorScriptViewSection::update();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OrchestratorScriptViewMacrosSection::OrchestratorScriptViewMacrosSection(const Ref<OScript>& p_script)
    : OrchestratorScriptViewSection("Macros")
    , _script(p_script)
{
}

String OrchestratorScriptViewMacrosSection::_get_tooltip_text() const
{
    return "A macro graph allows for the encapsulation of functionality for re-use. Macros have both a "
           "singular input and output node, but these nodes can have as many input or output data "
           "values needed for logic. Macros can contain nodes that take time, such as delays, but are "
           "not permitted to contain event nodes, such as a node that reacts to '_ready'.\n\n"
           "This feature is currently disabled and will be available in a future release.";
}

void OrchestratorScriptViewMacrosSection::_notification(int p_what)
{
    // Godot does not dispatch to parent (shrugs)
    OrchestratorScriptViewSection::_notification(p_what);

    if (p_what == NOTIFICATION_READY)
    {
        HBoxContainer* container = _get_panel_hbox();

        Button* button = Object::cast_to<Button>(container->get_child(-1));
        if (button)
            button->set_disabled(true);
    }
}

void OrchestratorScriptViewMacrosSection::update()
{
    if (_tree->get_root()->get_child_count() == 0)
    {
        TreeItem* item = _tree->get_root()->create_child();
        item->set_text(0, "No macros defined");
        item->set_selectable(0, false);
        return;
    }

    OrchestratorScriptViewSection::update();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OrchestratorScriptViewVariablesSection::OrchestratorScriptViewVariablesSection(const Ref<OScript>& p_script)
    : OrchestratorScriptViewSection("Variables")
    , _script(p_script)
{
}

void OrchestratorScriptViewVariablesSection::_on_variable_changed()
{
    update();
}

void OrchestratorScriptViewVariablesSection::_create_item(TreeItem* p_parent, const Ref<OScriptVariable>& p_variable)
{
    TreeItem* category = nullptr;
    for (TreeItem* child = p_parent->get_first_child(); child; child = child->get_next())
    {
        if (child->get_text(0).match(p_variable->get_category()))
        {
            category = child;
            break;
        }
    }

    if (p_variable->is_grouped_by_category())
    {
        if (!category)
        {
            category = p_parent->create_child();
            category->set_text(0, p_variable->get_category());
            category->set_selectable(0, false);
        }
    }
    else
        category = p_parent;

    TreeItem* item = category->create_child();
    item->set_text(0, p_variable->get_variable_name());
    item->set_icon(0, SceneUtils::get_editor_icon("MemberProperty"));
    item->set_meta("__name", p_variable->get_variable_name());
    item->add_button(0, SceneUtils::get_editor_icon(p_variable->get_variable_type_name()));

    if (!p_variable->get_description().is_empty())
    {
        const String tooltip = p_variable->get_variable_name() + "\n\n" + p_variable->get_description();
        item->set_tooltip_text(0, SceneUtils::create_wrapped_tooltip_text(tooltip));
    }

    if (p_variable->is_exported())
    {
        item->add_button(0, SceneUtils::get_editor_icon("GuiVisibilityVisible"));
        item->set_button_tooltip_text(0, 1, "Variable is visible outside the orchestration.");
    }
    else
    {
        item->add_button(0, SceneUtils::get_editor_icon("GuiVisibilityHidden"));
        item->set_button_tooltip_text(0, 1, "Variable is private.");
    }

    if (p_variable->is_exported() && p_variable->get_variable_name().begins_with("_"))
    {
        item->add_button(0, SceneUtils::get_editor_icon("NodeWarning"));
        item->set_button_tooltip_text(0, 2, "Variable is exported but defined as private using underscore prefix.");
        item->set_button_disabled(0, 2, true);
    }
}

PackedStringArray OrchestratorScriptViewVariablesSection::_get_existing_names() const
{
    return _script->get_variable_names();
}

String OrchestratorScriptViewVariablesSection::_get_tooltip_text() const
{
    return "A variable represents some data that will be stored and managed by the orchestration.\n\n"
           "Drag a variable from the component view onto the graph area to select whether to create "
           "a get/set node or use the action menu to find the get/set option for the variable.\n\n"
           "Selecting a variable in the component view displays the variable details in the inspector.";
}

String OrchestratorScriptViewVariablesSection::_get_remove_confirm_text(TreeItem* p_item) const
{
    return "Removing a variable will remove all nodes that get or set the variable.\nDo you want to continue?";
}

bool OrchestratorScriptViewVariablesSection::_populate_context_menu(TreeItem* p_item)
{
    _context_menu->add_icon_item(SceneUtils::get_editor_icon("Rename"), "Rename", CM_RENAME_VARIABLE);
    _context_menu->add_icon_item(SceneUtils::get_editor_icon("Remove"), "Remove", CM_REMOVE_VARIABLE);
    return true;
}

void OrchestratorScriptViewVariablesSection::_handle_context_menu(int p_id)
{
    switch (p_id)
    {
        case CM_RENAME_VARIABLE:
            _tree->edit_selected(true);
            break;
        case CM_REMOVE_VARIABLE:
            _confirm_removal(_tree->get_selected());
            break;
    }
}

void OrchestratorScriptViewVariablesSection::_handle_add_new_item()
{
    const String name = _create_unique_name_with_prefix("NewVar");

    // Add the new variable and update the components display
    _script->create_variable(name);

    update();

    _find_child_and_activate(name);
}

void OrchestratorScriptViewVariablesSection::_handle_item_selected()
{
    TreeItem* item = _tree->get_selected();

    Ref<OScriptVariable> variable = _script->get_variable(item->get_text(0));
    OrchestratorPlugin::get_singleton()->get_editor_interface()->edit_resource(variable);
}

void OrchestratorScriptViewVariablesSection::_handle_item_activated(TreeItem* p_item)
{
    Ref<OScriptVariable> variable = _script->get_variable(p_item->get_text(0));
    OrchestratorPlugin::get_singleton()->get_editor_interface()->edit_resource(variable);
}

void OrchestratorScriptViewVariablesSection::_handle_item_renamed(const String& p_old_name, const String& p_new_name)
{
    if (_get_existing_names().has(p_new_name))
    {
        _show_notification("A variable with the name '" + p_new_name + "' already exists.");
        return;
    }

    _script->rename_variable(p_old_name, p_new_name);
    update();
}

void OrchestratorScriptViewVariablesSection::_handle_remove(TreeItem* p_item)
{
    const String variable_name = p_item->get_text(0);
    _script->remove_variable(variable_name);
    update();
}

Dictionary OrchestratorScriptViewVariablesSection::_handle_drag_data(const Vector2& p_position)
{
    Dictionary data;

    TreeItem* selected = _tree->get_selected();
    if (selected)
    {
        data["type"] = "variable";
        data["variables"] = Array::make(selected->get_text(0));
    }
    return data;
}

void OrchestratorScriptViewVariablesSection::update()
{
    _clear_tree();

    PackedStringArray variable_names = _script->get_variable_names();
    if (!variable_names.is_empty())
    {
        HashMap<String, Ref<OScriptVariable>> categorized;
        HashMap<String, Ref<OScriptVariable>> uncategorized;
        HashMap<String, String> categorized_names;
        for (const String& variable_name : variable_names)
        {
            Ref<OScriptVariable> variable = _script->get_variable(variable_name);
            if (variable->is_grouped_by_category())
            {
                const String category = variable->get_category().to_lower();
                const String sort_name = vformat("%s/%s", category, variable_name.to_lower());

                categorized[variable_name] = variable;
                categorized_names[sort_name] = variable_name;
            }
            else
                uncategorized[variable_name] = variable;
        }

        // Sort categorized
        PackedStringArray sorted_categorized_names;
        for (const KeyValue<String, String>& E : categorized_names)
            sorted_categorized_names.push_back(E.key);
        sorted_categorized_names.sort();

        // Sort uncategorized
        PackedStringArray sorted_uncategorized_names;
        for (const KeyValue<String, Ref<OScriptVariable>>& E : uncategorized)
            sorted_uncategorized_names.push_back(E.key);
        sorted_uncategorized_names.sort();

        auto callable = callable_mp(this, &OrchestratorScriptViewVariablesSection::_on_variable_changed);

        TreeItem* root = _tree->get_root();
        for (const String& sort_categorized_name : sorted_categorized_names)
        {
            const String variable_name = categorized_names[sort_categorized_name];
            const Ref<OScriptVariable>& variable = categorized[variable_name];

            if (variable.is_valid() && !variable->is_connected("changed", callable))
                variable->connect("changed", callable);

            _create_item(root, variable);
        }

        for (const String& sort_uncategorized_name : sorted_uncategorized_names)
        {
            const Ref<OScriptVariable>& variable = uncategorized[sort_uncategorized_name];
            if (variable.is_valid() && !variable->is_connected("changed", callable))
                variable->connect("changed", callable);

            _create_item(root, variable);
        }
    }

    if (_tree->get_root()->get_child_count() == 0)
    {
        TreeItem* item = _tree->get_root()->create_child();
        item->set_text(0, "No variables defined");
        item->set_selectable(0, false);
        return;
    }

    OrchestratorScriptViewSection::update();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OrchestratorScriptViewSignalsSection::OrchestratorScriptViewSignalsSection(const Ref<OScript>& p_script)
    : OrchestratorScriptViewSection("Signals")
    , _script(p_script)
{
}

PackedStringArray OrchestratorScriptViewSignalsSection::_get_existing_names() const
{
    return _script->get_custom_signal_names();
}

String OrchestratorScriptViewSignalsSection::_get_tooltip_text() const
{
    return "A signal is used to send a notification synchronously to any number of observers that have "
           "connected to the defined signal on the orchestration. Signals allow for a variable number "
           "of arguments to be passed to the observer.\n\n"
           "Selecting a signal in the component view displays the signal details in the inspector.";
}

String OrchestratorScriptViewSignalsSection::_get_remove_confirm_text(TreeItem* p_item) const
{
    return "Removing a signal will remove all nodes that emit the signal.\nDo you want to continue?";
}

bool OrchestratorScriptViewSignalsSection::_populate_context_menu(TreeItem* p_item)
{
    _context_menu->add_icon_item(SceneUtils::get_editor_icon("Rename"), "Rename", CM_RENAME_SIGNAL);
    _context_menu->add_icon_item(SceneUtils::get_editor_icon("Remove"), "Remove", CM_REMOVE_SIGNAL);
    return true;
}

void OrchestratorScriptViewSignalsSection::_handle_context_menu(int p_id)
{
    switch (p_id)
    {
        case CM_RENAME_SIGNAL:
            _tree->edit_selected(true);
            break;
        case CM_REMOVE_SIGNAL:
            _confirm_removal(_tree->get_selected());
            break;
    }
}

void OrchestratorScriptViewSignalsSection::_handle_add_new_item()
{
    const String name = _create_unique_name_with_prefix("NewSignal");

    // Add the new signal and update the components display
    _script->create_custom_signal(name);

    update();

    _find_child_and_activate(name);
}

void OrchestratorScriptViewSignalsSection::_handle_item_selected()
{
    TreeItem* item = _tree->get_selected();

    Ref<OScriptSignal> signal = _script->get_custom_signal(item->get_text(0));
    OrchestratorPlugin::get_singleton()->get_editor_interface()->edit_resource(signal);
}

void OrchestratorScriptViewSignalsSection::_handle_item_activated(TreeItem* p_item)
{
    Ref<OScriptSignal> signal = _script->get_custom_signal(p_item->get_text(0));
    OrchestratorPlugin::get_singleton()->get_editor_interface()->edit_resource(signal);
}

void OrchestratorScriptViewSignalsSection::_handle_item_renamed(const String& p_old_name, const String& p_new_name)
{
    if (_get_existing_names().has(p_new_name))
    {
        _show_notification("A signal with the name '" + p_new_name + "' already exists.");
        return;
    }

    _script->rename_custom_user_signal(p_old_name, p_new_name);
    update();
}

void OrchestratorScriptViewSignalsSection::_handle_remove(TreeItem* p_item)
{
    const String signal_name = p_item->get_text(0);
    _script->remove_custom_signal(signal_name);
    update();
}

Dictionary OrchestratorScriptViewSignalsSection::_handle_drag_data(const Vector2& p_position)
{
    Dictionary data;

    TreeItem* selected = _tree->get_selected();
    if (selected)
    {
        data["type"] = "signal";
        data["signals"] = Array::make(selected->get_text(0));
    }
    return data;
}

void OrchestratorScriptViewSignalsSection::update()
{
    _clear_tree();

    PackedStringArray signal_names = _script->get_custom_signal_names();
    if (!signal_names.is_empty())
    {
        signal_names.sort();
        for (const String& signal_name : signal_names)
        {
            Ref<OScriptSignal> signal = _script->get_custom_signal(signal_name);

            TreeItem* item = _tree->get_root()->create_child();
            item->set_text(0, signal_name);
            item->set_meta("__name", signal_name);
            item->set_icon(0, SceneUtils::get_editor_icon("MemberSignal"));
        }
    }

    if (_tree->get_root()->get_child_count() == 0)
    {
        TreeItem* item = _tree->get_root()->create_child();
        item->set_text(0, "No signals defined");
        item->set_selectable(0, false);
        return;
    }

    OrchestratorScriptViewSection::update();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OrchestratorScriptView::OrchestratorScriptView(OrchestratorPlugin* p_plugin, OrchestratorMainView* p_main_view, const Ref<OScript>& p_script)
{
    _plugin = p_plugin;
    _main_view = p_main_view;
    _script = p_script;

    // When scripts are first opened, this adds the event graph if it doesn't exist.
    // This graph cannot be renamed or deleted.
    if (!_script->has_graph("EventGraph"))
        _script->create_graph("EventGraph", OScriptGraph::GF_EVENT);

    set_v_size_flags(SIZE_EXPAND_FILL);
    set_h_size_flags(SIZE_EXPAND_FILL);
}

void OrchestratorScriptView::_notification(int p_what)
{
    if (p_what == NOTIFICATION_READY)
    {
        _main_view->connect("toggle_component_panel", callable_mp(this, &OrchestratorScriptView::_on_toggle_component_panel));

        Node* editor_node = get_tree()->get_root()->get_child(0);
        editor_node->connect("script_add_function_request", callable_mp(this, &OrchestratorScriptView::_add_callback));

        VBoxContainer* panel = memnew(VBoxContainer);
        panel->set_h_size_flags(SIZE_EXPAND_FILL);
        add_child(panel);

        MarginContainer* margin = memnew(MarginContainer);
        margin->set_v_size_flags(SIZE_EXPAND_FILL);
        panel->add_child(margin);

        _tabs = memnew(TabContainer);
        _tabs->get_tab_bar()->set_tab_close_display_policy(TabBar::CLOSE_BUTTON_SHOW_ACTIVE_ONLY);
        _tabs->get_tab_bar()->connect("tab_close_pressed",
                                      callable_mp(this, &OrchestratorScriptView::_on_close_tab_requested));
        margin->add_child(_tabs);

        _scroll_container = memnew(ScrollContainer);
        _scroll_container->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
        _scroll_container->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
        add_child(_scroll_container);

        VBoxContainer* vbox = memnew(VBoxContainer);
        vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        _scroll_container->add_child(vbox);

        _graphs = memnew(OrchestratorScriptViewGraphsSection(_script));
        _graphs->connect("show_graph_requested", callable_mp(this, &OrchestratorScriptView::_on_show_graph));
        _graphs->connect("close_graph_requested", callable_mp(this, &OrchestratorScriptView::_on_close_graph));
        _graphs->connect("focus_node_requested", callable_mp(this, &OrchestratorScriptView::_on_focus_node));
        _graphs->connect("graph_renamed", callable_mp(this, &OrchestratorScriptView::_on_graph_renamed));
        _graphs->connect("scroll_to_item", callable_mp(this, &OrchestratorScriptView::_on_scroll_to_item));
        vbox->add_child(_graphs);

        _functions = memnew(OrchestratorScriptViewFunctionsSection(_script));
        _functions->connect("show_graph_requested", callable_mp(this, &OrchestratorScriptView::_on_show_graph));
        _functions->connect("close_graph_requested", callable_mp(this, &OrchestratorScriptView::_on_close_graph));
        _functions->connect("focus_node_requested", callable_mp(this, &OrchestratorScriptView::_on_focus_node));
        _functions->connect("override_function_requested", callable_mp(this, &OrchestratorScriptView::_on_override_function));
        _functions->connect("graph_renamed", callable_mp(this, &OrchestratorScriptView::_on_graph_renamed));
        _functions->connect("scroll_to_item", callable_mp(this, &OrchestratorScriptView::_on_scroll_to_item));
        vbox->add_child(_functions);

        _macros = memnew(OrchestratorScriptViewMacrosSection(_script));
        _macros->connect("scroll_to_item", callable_mp(this, &OrchestratorScriptView::_on_scroll_to_item));
        vbox->add_child(_macros);

        _variables = memnew(OrchestratorScriptViewVariablesSection(_script));
        _variables->connect("scroll_to_item", callable_mp(this, &OrchestratorScriptView::_on_scroll_to_item));
        vbox->add_child(_variables);

        _signals = memnew(OrchestratorScriptViewSignalsSection(_script));
        _signals->connect("scroll_to_item", callable_mp(this, &OrchestratorScriptView::_on_scroll_to_item));
        vbox->add_child(_signals);

        // The base event graph tab
        _event_graph = _get_or_create_tab("EventGraph");

        _update_components();
    }
}

void OrchestratorScriptView::goto_node(int p_node_id)
{
    Ref<OScriptNode> node = _script->get_node(p_node_id);
    if (node.is_valid())
    {
        for (const Ref<OScriptGraph>& graph : _script->get_graphs())
        {
            if (graph->has_node(p_node_id))
            {
                OrchestratorGraphEdit* ed_graph = _get_or_create_tab(graph->get_graph_name(), true, true);
                if (ed_graph)
                {
                    ed_graph->focus_node(p_node_id);
                    break;
                }
            }
        }
    }
}

void OrchestratorScriptView::scene_tab_changed()
{
    _update_components();
}

bool OrchestratorScriptView::is_modified() const
{
    return _script->is_edited();
}

void OrchestratorScriptView::reload_from_disk()
{
    _script->reload();
}

void OrchestratorScriptView::apply_changes()
{
    for (const Ref<OScriptNode>& node : _script->get_nodes())
        node->pre_save();

    for (int i = 0; i < _tabs->get_tab_count(); i++)
        if (OrchestratorGraphEdit* graph = Object::cast_to<OrchestratorGraphEdit>(_tabs->get_child(i)))
            graph->apply_changes();

    if (ResourceSaver::get_singleton()->save(_script, _script->get_path()) != OK)
        OS::get_singleton()->alert(vformat("Failed to save %s", _script->get_path()), "Error");

    _update_components();

    for (int i = 0; i < _tabs->get_tab_count(); i++)
        if (OrchestratorGraphEdit* graph = Object::cast_to<OrchestratorGraphEdit>(_tabs->get_child(i)))
            graph->post_apply_changes();

    for (const Ref<OScriptNode>& node : _script->get_nodes())
        node->post_save();
}

void OrchestratorScriptView::rename(const String& p_new_file)
{
    _script->set_path(p_new_file);
}

bool OrchestratorScriptView::save_as(const String& p_new_file)
{
    if (ResourceSaver::get_singleton()->save(_script, p_new_file) == OK)
    {
        _script->set_path(p_new_file);
        return true;
    }
    return false;
}

bool OrchestratorScriptView::build()
{
    return _script->validate_and_build();
}

void OrchestratorScriptView::_update_components()
{
    _graphs->update();
    _functions->update();
    _macros->update();
    _variables->update();
    _signals->update();
}

int OrchestratorScriptView::_get_tab_index_by_name(const String& p_name) const
{
    for (int i = 0; i < _tabs->get_tab_count(); i++)
    {
        if (OrchestratorGraphEdit* graph = Object::cast_to<OrchestratorGraphEdit>(_tabs->get_child(i)))
        {
            if (p_name.match(graph->get_name()))
                return i;
        }
    }
    return -1;
}

OrchestratorGraphEdit* OrchestratorScriptView::_get_or_create_tab(const StringName& p_tab_name, bool p_focus, bool p_create)
{
    // Lookup graph tab
    int tab_index = _get_tab_index_by_name(p_tab_name);
    if (tab_index >= 0)
    {
        // Tab found
        if (p_focus)
            _tabs->get_tab_bar()->set_current_tab(tab_index);

        return Object::cast_to<OrchestratorGraphEdit>(_tabs->get_tab_control(tab_index));
    }

    if (!p_create)
        return nullptr;

    // Create the graph and add it as a tab
    OrchestratorGraphEdit* graph = memnew(OrchestratorGraphEdit(_plugin, _script, p_tab_name));
    _tabs->add_child(graph);

    String tab_icon = "ClassList";
    if (graph->is_function())
        tab_icon = "MemberMethod";

    _tabs->set_tab_icon(_get_tab_index_by_name(p_tab_name), SceneUtils::get_editor_icon(tab_icon));

    // Setup connections
    graph->connect("nodes_changed", callable_mp(this, &OrchestratorScriptView::_on_graph_nodes_changed));
    graph->connect("focus_requested", callable_mp(this, &OrchestratorScriptView::_on_graph_focus_requested));

    if (p_focus)
        _tabs->get_tab_bar()->set_current_tab(_tabs->get_tab_count() - 1);

    return graph;
}

void OrchestratorScriptView::_show_available_function_overrides()
{
    if (OrchestratorGraphEdit* graph = _get_or_create_tab("EventGraph", false, false))
    {
        graph->set_spawn_position_center_view();

        OrchestratorGraphActionFilter filter;
        filter.context_sensitive = true;
        filter.context.graph = graph;
        filter.flags = OrchestratorGraphActionFilter::Filter_OverridesOnly;

        OrchestratorGraphActionMenu* menu = graph->get_action_menu();
        menu->set_initial_position(Window::WINDOW_INITIAL_POSITION_CENTER_SCREEN_WITH_MOUSE_FOCUS);
        menu->apply_filter(filter);
    }
}

void OrchestratorScriptView::_close_tab(int p_tab_index)
{
    if (OrchestratorGraphEdit* graph = Object::cast_to<OrchestratorGraphEdit>(_tabs->get_tab_control(p_tab_index)))
    {
        // Don't allow closing the main event graph tab
        if (graph->get_name().match("EventGraph"))
            return;

        Node* parent = graph->get_parent();
        parent->remove_child(graph);
        memdelete(graph);
    }
}

void OrchestratorScriptView::_on_close_tab_requested(int p_tab_index)
{
    if (p_tab_index >= 0 && p_tab_index < _tabs->get_tab_count())
        _close_tab(p_tab_index);
}

void OrchestratorScriptView::_on_graph_nodes_changed()
{
    _update_components();
}

void OrchestratorScriptView::_on_graph_focus_requested(Object* p_object)
{
    Ref<OScriptFunction> function = Object::cast_to<OScriptFunction>(p_object);
    if (function.is_valid())
        if (OrchestratorGraphEdit* graph = _get_or_create_tab(function->get_function_name(), true, true))
            graph->focus_node(function->get_owning_node_id());
}

void OrchestratorScriptView::_on_show_graph(const String& p_graph_name)
{
    _get_or_create_tab(p_graph_name, true, true);
}

void OrchestratorScriptView::_on_close_graph(const String& p_graph_name)
{
    const int tab_index = _get_tab_index_by_name(p_graph_name);
    if (tab_index >= 0)
        _close_tab(tab_index);
}

void OrchestratorScriptView::_on_graph_renamed(const String& p_old_name, const String& p_new_name)
{
    if (OrchestratorGraphEdit* graph = _get_or_create_tab(p_old_name, false, false))
        graph->set_name(p_new_name);
}

void OrchestratorScriptView::_on_focus_node(const String& p_graph_name, int p_node_id)
{
    if (OrchestratorGraphEdit* graph = _get_or_create_tab(p_graph_name, true, true))
        graph->focus_node(p_node_id);
}

void OrchestratorScriptView::_on_override_function()
{
    _show_available_function_overrides();
}

void OrchestratorScriptView::_on_toggle_component_panel(bool p_visible)
{
    _scroll_container->set_visible(p_visible);
}

void OrchestratorScriptView::_on_scroll_to_item(TreeItem* p_item)
{
    if (p_item && _scroll_container)
    {
        Tree* tree = p_item->get_tree();

        const Rect2 item_rect = tree->get_item_area_rect(p_item);
        const Rect2 tree_rect = tree->get_global_rect();
        const Rect2 view_rect = _scroll_container->get_rect();

        const float offset = tree_rect.get_position().y + item_rect.get_position().y;
        if (offset > view_rect.get_size().y)
            _scroll_container->set_v_scroll(offset);
    }
}

void OrchestratorScriptView::_add_callback(Object* p_object, const String& p_function_name, const PackedStringArray& p_args)
{
    // Get the script attached to the object
    Ref<Script> edited_script = p_object->get_script();
    if (!edited_script.is_valid())
        return;

    // Make sure that we're only applying the callback to the right resource
    if (edited_script.ptr() != _script.ptr())
        return;

    // Check if the method already exists and return if it does.
    if (_script->has_function(p_function_name))
        return;

    OScriptLanguage* language = OScriptLanguage::get_singleton();
    Ref<OScriptNode> node = language->create_node_from_type<OScriptNodeEvent>(_script);

    MethodInfo mi;
    mi.name = p_function_name;
    mi.return_val.type = Variant::NIL;

    for (const String& argument : p_args)
    {
        PackedStringArray bits = argument.split(":");
        const BuiltInType type = ExtensionDB::get_builtin_type(bits[1]);

        PropertyInfo pi;
        pi.name = bits[0];
        pi.class_name = bits[1];
        pi.type = type.type;
        mi.arguments.push_back(pi);
    }

    OScriptNodeInitContext context;
    context.method = mi;
    node->initialize(context);

    OrchestratorGraphEdit* editor_graph = _get_or_create_tab("EventGraph", true, false);
    if (editor_graph)
    {
        Ref<OScriptGraph> graph = editor_graph->get_owning_graph();
        _script->add_node(graph, node);
        node->post_placed_new_node();

        graph->add_function(node->get_id());
        graph->add_node(node->get_id());

        _update_components();

        editor_graph->focus_node(node->get_id());
    }
}
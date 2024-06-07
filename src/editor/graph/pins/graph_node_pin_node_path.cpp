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
#include "graph_node_pin_node_path.h"

#include "common/string_utils.h"
#include "common/scene_utils.h"
#include "script/script.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

void OrchestratorSceneTreeDialog::_bind_methods()
{
    ADD_SIGNAL(MethodInfo("node_selected", PropertyInfo(Variant::NODE_PATH, "node_path")));
}

void OrchestratorSceneTreeDialog::_on_close_requested()
{
    hide();
    queue_free();
}

void OrchestratorSceneTreeDialog::_on_confirmed()
{
    TreeItem* selected = _tree->get_selected();
    if (selected)
    {
        const NodePath node_path = selected->get_metadata(0);
        emit_signal("node_selected", node_path);
    }

    hide();
    queue_free();
}

void OrchestratorSceneTreeDialog::_on_filter_changed(const String& p_text)
{
    _update_tree(_tree->get_root());
}

void OrchestratorSceneTreeDialog::_on_item_activated()
{
    _on_confirmed();
}

void OrchestratorSceneTreeDialog::_on_item_selected()
{
    get_ok_button()->set_disabled(!_tree->get_selected());
}

bool OrchestratorSceneTreeDialog::_update_tree(TreeItem* p_item)
{
    bool has_filter = !_filter->get_text().strip_edges().is_empty();

    p_item->set_selectable(0, true);
    p_item->set_visible(true);
    p_item->clear_custom_color(0);

    if (has_filter)
        p_item->set_custom_color(0, Color(0.6, 0.6, 0.6));

    bool keep_for_children = false;
    for (TreeItem* child = p_item->get_first_child(); child; child = child->get_next())
        keep_for_children = _update_tree(child) || keep_for_children;

    bool keep = false;

    String text = p_item->get_text(0).to_lower();
    if ((has_filter && text.contains(_filter->get_text().strip_edges().to_lower())) || !has_filter)
    {
        keep = true;

        if (p_item->has_meta("__node"))
        {
            Node* p_node = Object::cast_to<Node>(p_item->get_meta("__node"));
            if (p_node->can_process())
                p_item->clear_custom_color(0);
        }
    }

    p_item->set_visible(keep || keep_for_children);

    if (!keep)
    {
        if (p_item->is_selected(0))
            p_item->deselect(0);

        p_item->set_selectable(0, false);
    }

    return p_item->is_visible();
}

void OrchestratorSceneTreeDialog::_populate_tree()
{
    _tree->clear();

    SceneTree* st = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());

    TreeItem* root = _tree->create_item();
    _tree->set_hide_root(true);

    // Don't allow showing the tree unless the scene that the script is attached is opened.
    Node* found = SceneUtils::get_node_with_script(_script, st->get_edited_scene_root(), st->get_edited_scene_root());
    if (found)
    {
        const String found_scene = SceneUtils::get_relative_scene_root(found)->get_scene_file_path();
        if (st->get_edited_scene_root()->get_scene_file_path() != found_scene)
            return;
    }

    _script_node = found;
    _populate_tree(root, st->get_edited_scene_root(), st->get_edited_scene_root());
}

void OrchestratorSceneTreeDialog::_populate_tree(TreeItem* p_parent, Node* p_node, Node* p_root)
{
    if (!(p_node == p_root || p_node->get_owner() == p_root))
        return;

    if (!_script_node)
        return;

    TreeItem* child = p_parent->create_child();
    child->set_text(0, p_node->get_name());
    child->set_icon(0, SceneUtils::get_editor_icon(p_node->get_class()));
    child->set_metadata(0, _script_node->get_path_to(p_node));
    child->set_meta("__node", p_node);
    if (!p_node->can_process())
        child->set_custom_color(0, Color(0.6, 0.6, 0.6));

    if (_script_node->get_path_to(p_node) == _node_path)
        child->select(0);

    for (int i = 0; i < p_node->get_child_count(); i++)
    {
        Node* child_node = p_node->get_child(i);
        _populate_tree(child, child_node, p_root);
    }
}

void OrchestratorSceneTreeDialog::_notification(int p_what)
{
    if (p_what == NOTIFICATION_READY)
    {
        set_title("Select a node");

        VBoxContainer* content = memnew(VBoxContainer);
        add_child(content);

        HBoxContainer* filter_container = memnew(HBoxContainer);
        content->add_child(filter_container);

        _filter = memnew(LineEdit);
        _filter->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        _filter->set_placeholder("Filter Nodes");
        _filter->set_clear_button_enabled(true);
        _filter->add_theme_constant_override("minimum_character_width", 0);
        _filter->connect("text_changed", callable_mp(this, &OrchestratorSceneTreeDialog::_on_filter_changed));
        filter_container->add_child(_filter);

        _tree = memnew(Tree);
        _tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);
        content->add_child(_tree);

        get_ok_button()->set_disabled(!_tree->get_selected());

        connect("confirmed", callable_mp(this, &OrchestratorSceneTreeDialog::_on_confirmed));
        connect("canceled", callable_mp(this, &OrchestratorSceneTreeDialog::_on_close_requested));

        _tree->connect("item_activated", callable_mp(this, &OrchestratorSceneTreeDialog::_on_item_activated));
        _tree->connect("item_selected", callable_mp(this, &OrchestratorSceneTreeDialog::_on_item_selected));

        _populate_tree();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OrchestratorGraphNodePinNodePath::OrchestratorGraphNodePinNodePath(OrchestratorGraphNode* p_node, const Ref<OScriptNodePin>& p_pin)
    : OrchestratorGraphNodePin(p_node, p_pin)
{
}

void OrchestratorGraphNodePinNodePath::_bind_methods()
{
}

void OrchestratorGraphNodePinNodePath::_on_show_scene_tree_dialog()
{
    OrchestratorSceneTreeDialog* dialog = memnew(OrchestratorSceneTreeDialog);
    dialog->set_min_size(Size2(475, 700));
    dialog->set_node_path(_pin->get_effective_default_value());
    dialog->connect("node_selected", callable_mp(this, &OrchestratorGraphNodePinNodePath::_on_node_selected));

    const Ref<OScript> script = _pin->get_owning_node()->get_orchestration()->get_self();
    if (script.is_valid())
        dialog->set_script(script);

    add_child(dialog);

    dialog->popup_centered();
}

void OrchestratorGraphNodePinNodePath::_on_node_selected(const NodePath& p_node_path)
{
    _pin->set_default_value(p_node_path);
    _button->set_text(p_node_path);
}

Control* OrchestratorGraphNodePinNodePath::_get_default_value_widget()
{
    // Create button
    _button = memnew(Button);
    _button->set_focus_mode(FocusMode::FOCUS_NONE);
    _button->set_custom_minimum_size(Vector2(28, 0));
    _button->set_text(StringUtils::default_if_empty(_pin->get_effective_default_value(), "Assign..."));
    _button->connect("pressed", callable_mp(this, &OrchestratorGraphNodePinNodePath::_on_show_scene_tree_dialog));
    return _button;
}
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
#include "action_menu.h"

#include "common/dictionary_utils.h"
#include "common/scene_utils.h"
#include "common/string_utils.h"
#include "editor/graph/graph_edit.h"
#include "editor/graph/graph_node_pin.h"
#include "editor/graph/graph_node_spawner.h"
#include "default_action_registrar.h"

#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

void OrchestratorGraphActionMenu::_bind_methods()
{
    ADD_SIGNAL(MethodInfo("action_selected", PropertyInfo(Variant::OBJECT, "handler")));
}

void OrchestratorGraphActionMenu::_notification(int p_what)
{
    if (p_what == NOTIFICATION_READY)
    {
        set_title("All Actions");

        VBoxContainer* vbox = memnew(VBoxContainer);
        add_child(vbox);

        HBoxContainer* hbox = memnew(HBoxContainer);
        hbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        hbox->set_alignment(BoxContainer::ALIGNMENT_END);
        vbox->add_child(hbox);

        _context_sensitive = memnew(CheckBox);
        _context_sensitive->set_text("Context Sensitive");
        _context_sensitive->set_h_size_flags(Control::SizeFlags::SIZE_SHRINK_END);
        _context_sensitive->set_focus_mode(Control::FOCUS_NONE);
        _context_sensitive->connect("toggled", callable_mp(this, &OrchestratorGraphActionMenu::_on_context_sensitive_toggled));
        hbox->add_child(_context_sensitive);

        _collapse = memnew(Button);
        _collapse->set_button_icon(SceneUtils::get_icon(this, "CollapseTree"));
        _collapse->set_toggle_mode(true);
        _collapse->set_focus_mode(Control::FOCUS_NONE);
        _collapse->set_tooltip_text("Collapse the results tree");
        _collapse->connect("toggled", callable_mp(this, &OrchestratorGraphActionMenu::_on_collapse_tree));
        hbox->add_child(_collapse);

        _expand = memnew(Button);
        _expand->set_button_icon(SceneUtils::get_icon(this, "ExpandTree"));
        _expand->set_toggle_mode(true);
        _expand->set_pressed(true);
        _expand->set_focus_mode(Control::FOCUS_NONE);
        _expand->set_tooltip_text("Expand the results tree");
        _expand->connect("toggled", callable_mp(this, &OrchestratorGraphActionMenu::_on_expand_tree));
        hbox->add_child(_expand);

        _filters_text_box = memnew(LineEdit);
        _filters_text_box->set_placeholder("Search...");
        _filters_text_box->set_custom_minimum_size(Size2(700, 0));
        _filters_text_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        _filters_text_box->set_clear_button_enabled(true);
        _filters_text_box->connect("text_changed", callable_mp(this, &OrchestratorGraphActionMenu::_on_filter_text_changed));
        _filters_text_box->connect("text_submitted", callable_mp(this, &OrchestratorGraphActionMenu::_on_filter_text_changed));
        vbox->add_child(_filters_text_box);
        register_text_enter(_filters_text_box);

        _tree_view = memnew(Tree);
        _tree_view->set_v_size_flags(Control::SIZE_EXPAND_FILL);
        _tree_view->set_hide_root(true);
        _tree_view->set_hide_folding(false);
        _tree_view->set_columns(1);
        _tree_view->set_select_mode(Tree::SELECT_ROW);
        _tree_view->connect("item_activated", callable_mp(this, &OrchestratorGraphActionMenu::_on_tree_item_activated));
        _tree_view->connect("item_selected", callable_mp(this, &OrchestratorGraphActionMenu::_on_tree_item_selected));
        _tree_view->connect("nothing_selected", callable_mp(this, &OrchestratorGraphActionMenu::_on_tree_item_activated));
        vbox->add_child(_tree_view);

        set_ok_button_text("Add");
        set_hide_on_ok(false);
        get_ok_button()->set_disabled(true);

        connect("confirmed", callable_mp(this, &OrchestratorGraphActionMenu::_on_confirmed));
        connect("canceled", callable_mp(this, &OrchestratorGraphActionMenu::_on_close_requested));
        connect("close_requested", callable_mp(this, &OrchestratorGraphActionMenu::_on_close_requested));
    }
}

void OrchestratorGraphActionMenu::apply_filter(const OrchestratorGraphActionFilter& p_filter)
{
    _filter = p_filter;
    _context_sensitive->set_block_signals(true);
    _context_sensitive->set_pressed(_filter.context_sensitive);
    _context_sensitive->set_block_signals(false);

    _force_refresh = true;
    _collapse->set_pressed(false);
    _expand->set_pressed(true);

    _refresh_actions();

    set_size(Vector2(350, 700));
    popup();

    _tree_view->scroll_to_item(_tree_view->get_root());
    _filters_text_box->grab_focus();
}

void OrchestratorGraphActionMenu::_refresh_actions()
{
    if (_items.is_empty() || _force_refresh)
        _generate_actions(_filter.context);

    _generate_filtered_actions(_filter.context);
}

void OrchestratorGraphActionMenu::_generate_actions(const OrchestratorGraphActionContext& p_context)
{
    _force_refresh = false;
    _items.clear();

    OrchestratorGraphActionRegistrarContext ctx;
    ctx.graph = p_context.graph;
    ctx.script = _filter.context.graph->get_owning_script();
    ctx.list = &_items;
    ctx.filter = &_filter;

    for (const String &name : ClassDB::get_inheriters_from_class(OrchestratorGraphActionRegistrar::get_class_static()))
    {
        if (!ClassDB::can_instantiate(name))
            continue;

        Variant object = ClassDB::instantiate(name);
        if (OrchestratorGraphActionRegistrar* registrator = Object::cast_to<OrchestratorGraphActionRegistrar>(object))
            registrator->register_actions(ctx);
    }

    _items.sort_custom<OrchestratorGraphActionMenuItemComparator>();
}

void OrchestratorGraphActionMenu::_generate_filtered_actions([[maybe_unused]] const OrchestratorGraphActionContext& p_context)
{
    _tree_view->clear();

    _tree_view->create_item();
    _tree_view->set_columns(2);

    Ref<Texture2D> broken = SceneUtils::get_icon(this, "_not_found_");

    for (const Ref<OrchestratorGraphActionMenuItem>& item : _items)
    {
        if (item->get_handler().is_valid())
        {
            if (item->get_handler()->is_filtered(_filter, item->get_spec()))
                continue;
        }

        TreeItem* parent = _tree_view->get_root();

        const PackedStringArray categories = item->get_spec().category.split("/");

        for (int i = 0; i < categories.size() - 1; i++)
        {
            bool found = false;
            for (int j = 0; j < parent->get_child_count(); ++j)
            {
                if (TreeItem* child = Object::cast_to<TreeItem>(parent->get_child(j)))
                {
                    if (child->get_text(0).to_lower() == categories[i].to_lower())
                    {
                        parent = child;
                        found = true;
                        break;
                    }
                }
            }

            if (!found)
            {
                for (int j = i; j < categories.size() - 1; ++j)
                {
                    parent = parent->create_child();
                    parent->set_text(0, categories[j]);

                    Ref<Texture2D> icon;
                    if (categories[j] == "Integer")
                        icon = SceneUtils::get_icon(this, "int");
                    else
                        icon = SceneUtils::get_icon(this, categories[j]);

                    if (icon == broken)
                        icon = SceneUtils::get_icon(this, "Object");

                    parent->set_icon(0, icon);
                    parent->set_selectable(0, false);
                }
                break;
            }
        }

        TreeItem* node = parent->create_child();
        node->set_text(0, item->get_spec().text);
        node->set_icon(0, SceneUtils::get_icon(this, item->get_spec().icon));
        node->set_tooltip_text(0, item->get_spec().tooltip);
        node->set_selectable(0, item->get_handler().is_valid());

        node->set_text_alignment(1, HORIZONTAL_ALIGNMENT_RIGHT);
        node->set_expand_right(1, true);
        node->set_icon(1, SceneUtils::get_icon(this, item->get_spec().type_icon));
        node->set_tooltip_text(1, item->get_handler().is_valid() ? item->get_handler()->get_class() : "");

        if (item->get_handler().is_valid())
            node->set_meta("handler", item->get_handler());
    }

    _remove_empty_action_nodes(_tree_view->get_root());
}

void OrchestratorGraphActionMenu::_remove_empty_action_nodes(TreeItem* p_parent)
{
    TreeItem* child = p_parent->get_first_child();
    while (child)
    {
        TreeItem* next = child->get_next();

        _remove_empty_action_nodes(child);
        if (child->get_child_count() == 0 && !child->has_meta("handler"))
            memdelete(child);

        child = next;
    }
}

void OrchestratorGraphActionMenu::_notify_and_close(TreeItem* p_selected)
{
    if (p_selected)
    {
        Ref<OrchestratorGraphActionHandler> handler = p_selected->get_meta("handler");
        emit_signal("action_selected", handler.ptr());
    }

    emit_signal("close_requested");
}

void OrchestratorGraphActionMenu::_on_context_sensitive_toggled(bool p_new_state)
{
    _filter.context_sensitive = p_new_state;
    _generate_filtered_actions(_filter.context);
}

void OrchestratorGraphActionMenu::_on_filter_text_changed(const String& p_new_text)
{
    // Update filters
    _filter.keywords.clear();

    const String filter_text = p_new_text.trim_prefix(" ").trim_suffix(" ");
    if (!filter_text.is_empty())
        for (const String& element : filter_text.split(" "))
            _filter.keywords.push_back(element.to_lower());

    get_ok_button()->set_disabled(true);

    _generate_filtered_actions(_filter.context);
}

void OrchestratorGraphActionMenu::_on_tree_item_selected()
{
    // Disable the OK button if no item is selected
    get_ok_button()->set_disabled(false);
}

void OrchestratorGraphActionMenu::_on_tree_item_activated()
{
    _notify_and_close(_tree_view->get_selected());
}

void OrchestratorGraphActionMenu::_on_close_requested()
{
    _filters_text_box->set_text("");
    get_ok_button()->set_disabled(true);

    hide();

    set_initial_position(Window::WINDOW_INITIAL_POSITION_ABSOLUTE);
}

void OrchestratorGraphActionMenu::_on_confirmed()
{
    _notify_and_close(_tree_view->get_selected());
}

void OrchestratorGraphActionMenu::_on_collapse_tree(bool p_collapsed)
{
    if (p_collapsed)
    {
        _expand->set_pressed(false);

        TreeItem* child = _tree_view->get_root()->get_first_child();
        while (child)
        {
            child->set_collapsed_recursive(true);
            child = child->get_next();
        }
    }
}

void OrchestratorGraphActionMenu::_on_expand_tree(bool p_expanded)
{
    if (p_expanded)
    {
        _collapse->set_pressed(false);

        TreeItem* child = _tree_view->get_root()->get_first_child();
        while (child)
        {
            child->set_collapsed_recursive(false);
            child = child->get_next();
        }
    }
}
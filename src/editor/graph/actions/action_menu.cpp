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
#include "action_menu.h"

#include "common/dictionary_utils.h"
#include "common/scene_utils.h"
#include "common/settings.h"
#include "common/string_utils.h"
#include "default_action_registrar.h"
#include "editor/graph/graph_edit.h"
#include "editor/graph/graph_node_pin.h"
#include "editor/graph/graph_node_spawner.h"

#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

OrchestratorGraphActionMenu::OrchestratorGraphActionMenu(OrchestratorGraphEdit* p_graph_edit)
{
    _graph_edit = p_graph_edit;
}

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
        _collapse->set_button_icon(SceneUtils::get_editor_icon("CollapseTree"));
        _collapse->set_toggle_mode(true);
        _collapse->set_focus_mode(Control::FOCUS_NONE);
        _collapse->set_tooltip_text("Collapse the results tree");
        _collapse->connect("toggled", callable_mp(this, &OrchestratorGraphActionMenu::_on_collapse_tree));
        hbox->add_child(_collapse);

        _expand = memnew(Button);
        _expand->set_button_icon(SceneUtils::get_editor_icon("ExpandTree"));
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
        _tree_view->connect("nothing_selected", callable_mp(this, &OrchestratorGraphActionMenu::_on_tree_nothing_selected));
        _tree_view->connect("button_clicked", callable_mp(this, &OrchestratorGraphActionMenu::_on_tree_button_clicked));
        _tree_view->connect("item_collapsed", callable_mp(this, &OrchestratorGraphActionMenu::_on_tree_item_collapsed));
        vbox->add_child(_tree_view);

        set_ok_button_text("Add");
        set_hide_on_ok(false);
        get_ok_button()->set_disabled(true);

        connect("confirmed", callable_mp(this, &OrchestratorGraphActionMenu::_on_confirmed));
        connect("canceled", callable_mp(this, &OrchestratorGraphActionMenu::_on_close_requested));
        connect("close_requested", callable_mp(this, &OrchestratorGraphActionMenu::_on_close_requested));

        // When certain script elements change, this handles forcing a refresh
        const Ref<Resource> self = _graph_edit->get_orchestration()->get_self();
        self->connect("functions_changed", callable_mp(this, &OrchestratorGraphActionMenu::clear));
        self->connect("variables_changed", callable_mp(this, &OrchestratorGraphActionMenu::clear));
        self->connect("signals_changed", callable_mp(this, &OrchestratorGraphActionMenu::clear));

        // When user changes any project settings, this is used to force a refresh for autoloads
        ProjectSettings::get_singleton()->connect("settings_changed", callable_mp(this, &OrchestratorGraphActionMenu::clear));
    }
}

void OrchestratorGraphActionMenu::clear()
{
    _action_db.clear();
}

void OrchestratorGraphActionMenu::apply_filter(const OrchestratorGraphActionFilter& p_filter)
{
    _filter = p_filter;
    _context_sensitive->set_block_signals(true);
    _context_sensitive->set_pressed(_filter.context_sensitive);
    _context_sensitive->set_block_signals(false);

    _collapse->set_pressed(false);
    _expand->set_pressed(true);

    // Small hack to trigger clearing the temp cache when not using the cache
    // This allows us to generate the action list once so that as the user types search
    // items into the filter box, we do not regenerate the expensive list repeatedly.
    _action_db.use_temp(!_filter.use_cache);

    _action_db.load(_filter);
    _generate_filtered_actions();

    set_size(Vector2(350, 700));

    OrchestratorSettings* os = OrchestratorSettings::get_singleton();
    if (os && os->get_setting("ui/actions_menu/center_on_mouse"))
        set_position(get_position() - (get_size() / 2));

    _on_collapse_tree(true);

    popup();

    _tree_view->scroll_to_item(_tree_view->get_root());
    _filters_text_box->grab_focus();
}

void OrchestratorGraphActionMenu::_generate_filtered_actions()
{
    _tree_view->clear();

    _tree_view->set_columns(3);
    _tree_view->set_column_expand(0, true);
    _tree_view->set_column_expand(1, false);
    _tree_view->set_column_custom_minimum_width(1, 50);
    _tree_view->set_column_expand(2, false);
    _tree_view->create_item();

    Ref<Texture2D> broken = SceneUtils::get_editor_icon("_not_found_");

    PackedStringArray action_favorites = OrchestratorSettings::get_singleton()->get_action_favorites();

    /// todo: this should be moved to a one-time thing on start-up
    for (int index = 0; index < action_favorites.size(); ++index)
    {
        if (action_favorites[index].begins_with("Script/Variables/")
            || action_favorites[index].begins_with("Script/Call Function/")
            || action_favorites[index].begins_with("Class/Signals/")
            || action_favorites[index].begins_with("Class/Methods/")
            || action_favorites[index].begins_with("Class/Properties/"))
        {
            OrchestratorSettings::get_singleton()->remove_action_favorite(action_favorites[index]);
            PackedStringArray parts = action_favorites[index].split("/");
            parts.remove_at(0);
            action_favorites[index] = StringUtils::join("/", parts);
            OrchestratorSettings::get_singleton()->add_action_favorite(action_favorites[index]);
        }
        else if (action_favorites[index].begins_with("Script/Emit Signals/"))
        {
            OrchestratorSettings::get_singleton()->remove_action_favorite(action_favorites[index]);
            PackedStringArray parts = action_favorites[index].split("/");
            parts.remove_at(0);
            parts.remove_at(0);
            action_favorites[index] = "Signals/" + StringUtils::join("/", parts);
            OrchestratorSettings::get_singleton()->add_action_favorite(action_favorites[index]);
        }
    }

    TreeItem* favorites = nullptr;
    if (!action_favorites.is_empty())
    {
        OrchestratorGraphActionSpec spec;
        spec.category = "favorites";
        spec.text = "Favorites";
        spec.icon = "Favorites";

        Ref<OrchestratorGraphActionMenuItem> item(memnew(OrchestratorGraphActionMenuItem(spec)));
        favorites = _make_item(_tree_view->get_root(), item, spec.text);
        favorites->set_selectable(0, false);
    }

    List<Ref<OrchestratorGraphActionMenuItem>> items = _action_db.get_items();
    for (const Ref<OrchestratorGraphActionMenuItem>& item : items)
    {
        TreeItem* parent = _tree_view->get_root();

        const PackedStringArray categories = item->get_spec().category.split("/");

        // Don't show "Project/" top-level when dragging from pin
        if (!_filter.context.pins.is_empty() || !_filter.target_classes.is_empty())
            if (categories.size() >= 1 && categories[0].to_lower().match("project"))
                continue;

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
                        icon = SceneUtils::get_editor_icon("int");
                    else
                        icon = SceneUtils::get_editor_icon(categories[j]);

                    if (icon == broken)
                        icon = SceneUtils::get_editor_icon("Object");

                    parent->set_icon(0, icon);
                    parent->set_selectable(0, false);
                }
                break;
            }
        }

        const bool is_favorite = action_favorites.has(item->get_spec().category);
        _make_item(parent, item, item->get_spec().text, true, is_favorite);

        if (is_favorite)
            _make_item(favorites, item, _create_favorite_item_text(parent, item));
    }

    _remove_empty_action_nodes(_tree_view->get_root());
}

TreeItem* OrchestratorGraphActionMenu::_traverse_tree(TreeItem* p_item, float& r_highest_score)
{
    if (p_item->get_child_count() > 0)
    {
        TreeItem* cached{ nullptr };
        for (int i = 0; i < p_item->get_child_count(); ++i)
        {
            TreeItem* item = _traverse_tree(p_item->get_child(i), r_highest_score);
            if (item != nullptr)
                cached = item;
        }
        return cached;
    }

    float score = _calculate_score(p_item);
    if (score > r_highest_score)
    {
        r_highest_score = score;
        return p_item;
    }

    return nullptr;
}

float OrchestratorGraphActionMenu::_calculate_score(TreeItem* p_item)
{
    if (!p_item->has_meta("item"))
        return 0.f;

    Ref<OrchestratorGraphActionMenuItem> item = p_item->get_meta("item");
    if (!item.is_valid())
        return 0.f;

    const String filter_text = _filters_text_box->get_text().trim_prefix(" ").trim_suffix(" ");
    const String item_text = item->get_spec().text;

    if (item_text.to_lower() == filter_text.to_lower())
        return 1.f;

    float inverse_length = 1.f / float(item_text.length());

    // Favor types where search term is substring close to start of search filter
    float w = 0.5f;
    int pos = item_text.findn(filter_text);
    float score = (pos > -1) ? 1.f - w * MIN(1, 3 * pos * inverse_length) : MAX(0.f, .9f - w);

    // Favor shorter items
    w = 0.9f;
    score *= (1 - w) + w * MIN(1.0f, filter_text.length() * inverse_length);

    // todo: add support for keywords

    if (!p_item->is_selectable(0))
        score *= 0.1f;

    return score;
}

TreeItem* OrchestratorGraphActionMenu::_make_item(TreeItem* p_parent,
                                                  const Ref<OrchestratorGraphActionMenuItem>& p_menu_item,
                                                  const String& p_text,
                                                  bool p_favorite_icon,
                                                  bool p_is_favorite)
{
    TreeItem* child = p_parent->create_child();
    child->set_text(0, p_text);
    child->set_expand_right(0, true);
    child->set_icon(0, SceneUtils::get_class_icon(p_menu_item->get_spec().icon));
    child->set_tooltip_text(0, p_menu_item->get_spec().tooltip);
    child->set_selectable(0, p_menu_item->get_handler().is_valid());

    child->set_text(1, StringUtils::default_if_empty(p_menu_item->get_spec().qualifiers, " "));
    child->set_text_alignment(1, HORIZONTAL_ALIGNMENT_RIGHT);

    if (!p_menu_item->get_spec().type_icon.is_empty())
    {
        child->add_button(2, SceneUtils::get_editor_icon(p_menu_item->get_spec().type_icon), -1, true);
        child->set_text_alignment(2, HORIZONTAL_ALIGNMENT_RIGHT);
        child->set_button_tooltip_text(2, 0, p_menu_item->get_handler().is_valid() ? p_menu_item->get_handler()->get_class() : "");
    }

    if (p_favorite_icon && p_menu_item->get_handler().is_valid())
    {
        child->add_button(2, SceneUtils::get_editor_icon(p_is_favorite ? "Favorites" : "NonFavorite"));
        child->set_button_tooltip_text(2, 1, p_is_favorite ? "Remove action from favorites." : "Add action to favorites.");
        child->set_meta("favorite", p_is_favorite);
    }

    child->set_meta("item", p_menu_item);

    if (p_menu_item->get_handler().is_valid())
        child->set_meta("handler", p_menu_item->get_handler());

    return child;
}

String OrchestratorGraphActionMenu::_create_favorite_item_text(TreeItem* p_parent,
                                                               const Ref<OrchestratorGraphActionMenuItem>& p_menu_item)
{
    String favorite_text;
    while (p_parent != _tree_view->get_root())
    {
        if (!favorite_text.is_empty())
            favorite_text = p_parent->get_text(0) + "/" + favorite_text;
        else
            favorite_text = p_parent->get_text(0);

        p_parent = p_parent->get_parent();
    }

    return "<" + favorite_text + "> " + p_menu_item->get_spec().text;
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

bool OrchestratorGraphActionMenu::_apply_selection(TreeItem* p_item)
{
    if (p_item->has_meta("item"))
    {
        Ref<OrchestratorGraphActionMenuItem> menu_item = p_item->get_meta("item");
        if (menu_item.is_valid() && menu_item->get_spec().category == _selection)
        {
            _tree_view->set_selected(p_item, 0);
            return true;
        }
    }

    TreeItem* child = p_item->get_first_child();
    while (child)
    {
        if (_apply_selection(child))
            return true;

        child = child->get_next();
    }

    return false;
}

void OrchestratorGraphActionMenu::_on_context_sensitive_toggled(bool p_new_state)
{
    _filter.context_sensitive = p_new_state;
    _action_db.load(_filter);

    _generate_filtered_actions();

    if (_collapse->is_pressed())
        _on_collapse_tree(true);
}

void OrchestratorGraphActionMenu::_on_filter_text_changed(const String& p_new_text)
{
    // Update filters
    _filter.keywords.clear();

    _on_expand_tree(true);

    const String filter_text = p_new_text.trim_prefix(" ").trim_suffix(" ");
    if (!filter_text.is_empty())
        for (const String& element : filter_text.split(" "))
            _filter.keywords.push_back(element.to_lower());

    get_ok_button()->set_disabled(true);

    _action_db.load(_filter);
    _generate_filtered_actions();

    float highest_score = 0;
    TreeItem* item = _traverse_tree(_tree_view->get_root(), highest_score);
    if (item != nullptr)
    {
        item->select(0);
        _tree_view->scroll_to_item(item, true);
    }
}

void OrchestratorGraphActionMenu::_on_tree_item_selected()
{
    // Disable the OK button if no item is selected
    get_ok_button()->set_disabled(false);

    TreeItem* selected = _tree_view->get_selected();
    if (selected && selected->has_meta("item"))
    {
        Ref<OrchestratorGraphActionMenuItem> menu_item = selected->get_meta("item");
        _selection = menu_item->get_spec().category;
    }
}

void OrchestratorGraphActionMenu::_on_tree_item_activated()
{
    _notify_and_close(_tree_view->get_selected());
}

void OrchestratorGraphActionMenu::_on_tree_nothing_selected()
{
    // Although Godot Tree dispatches nothing_selected, it does have a selected item,
    // so this needs to be cleared and the Add button needs to be disabled.
    _tree_view->deselect_all();
    get_ok_button()->set_disabled(true);
}

void OrchestratorGraphActionMenu::_on_tree_item_collapsed(TreeItem* p_item)
{
    if (!p_item || p_item->get_child_count() > 0)
    {
        _tree_view->deselect_all();
        get_ok_button()->set_disabled(true);
    }
}

void OrchestratorGraphActionMenu::_on_tree_button_clicked(TreeItem* p_item, int p_column, int p_id, int p_button_index)
{
    // There is currently only 1 button for marking favorites
    // No need to worry with button ids
    if (!p_item->get_meta("favorite", false))
    {
        p_item->set_button(p_column, 0, SceneUtils::get_editor_icon("Favorites"));
        p_item->set_meta("favorite", true);

        Ref<OrchestratorGraphActionMenuItem> menu_item = p_item->get_meta("item");
        if (menu_item.is_valid())
        {
            OrchestratorSettings* os = OrchestratorSettings::get_singleton();
            os->add_action_favorite(menu_item->get_spec().category);
        }
    }
    else
    {
        p_item->set_button(p_column, 0, SceneUtils::get_editor_icon("NonFavorite"));
        p_item->set_meta("favorite", false);

        Ref<OrchestratorGraphActionMenuItem> menu_item = p_item->get_meta("item");
        if (menu_item.is_valid())
        {
            OrchestratorSettings* os = OrchestratorSettings::get_singleton();
            os->remove_action_favorite(menu_item->get_spec().category);
        }
    }

    _action_db.load(_filter);
    _generate_filtered_actions();
}

void OrchestratorGraphActionMenu::_on_close_requested()
{
    _filters_text_box->set_text("");
    _selection = "";

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
        _expand->set_pressed_no_signal(false);

        TreeItem* child = _tree_view->get_root()->get_first_child();
        while (child)
        {
            child->set_collapsed_recursive(true);
            child = child->get_next();
        }
    }

    _collapse->set_pressed_no_signal(true);
}

void OrchestratorGraphActionMenu::_on_expand_tree(bool p_expanded)
{
    if (p_expanded)
    {
        _collapse->set_pressed_no_signal(false);

        bool applied = false;
        TreeItem* child = _tree_view->get_root()->get_first_child();
        while (child)
        {
            child->set_collapsed_recursive(false);

            if (!_selection.is_empty() && !applied)
                applied = _apply_selection(child);

            child = child->get_next();
        }
    }
    _expand->set_pressed_no_signal(true);
}
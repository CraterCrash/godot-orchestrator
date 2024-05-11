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
#include "editor/search/search_dialog.h"

#include "common/scene_utils.h"
#include "plugin/plugin.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_split_container.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/popup_panel.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/v_split_container.hpp>

void OrchestratorEditorSearchHelpBit::_notification(int p_what)
{
    if (p_what == NOTIFICATION_ENTER_TREE)
    {
        _help_bit = memnew(RichTextLabel);
        _help_bit->set_fit_content(true);
        _help_bit->set_use_bbcode(true);

        add_child(_help_bit);
        set_custom_minimum_size(Size2(0, 50));

        _help_bit->connect("meta_clicked", callable_mp(this, &OrchestratorEditorSearchHelpBit::_on_meta_clicked));
    }
    else if (p_what == NOTIFICATION_THEME_CHANGED)
    {
        if (_help_bit)
        {
            _help_bit->clear();
            _help_bit->add_theme_color_override("selection_color", get_theme_color("selection_color", "EditorHelp"));
            _add_text(_text);

            _help_bit->reset_size();
        }
    }
}

#include <godot_cpp/variant/utility_functions.hpp>

void OrchestratorEditorSearchHelpBit::_add_text(const String& p_bbcode)
{
    _help_bit->append_text(p_bbcode);
}

void OrchestratorEditorSearchHelpBit::set_disabled(bool p_disabled)
{
    _help_bit->set_modulate(Color(1, 1, 1, p_disabled ? 0.5f : 1.0f));
}

void OrchestratorEditorSearchHelpBit::set_text(const String& p_text)
{
    _text = p_text;
    _help_bit->clear();
    _add_text(_text);
}

void OrchestratorEditorSearchHelpBit::_on_meta_clicked()
{
    // no-op
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OrchestratorEditorSearchDialog::_bind_methods()
{
    ADD_SIGNAL(MethodInfo("selected"));
}

void OrchestratorEditorSearchDialog::_notification(int p_what)
{
    if (p_what == NOTIFICATION_ENTER_TREE)
    {
        HSplitContainer* hsplit_container = memnew(HSplitContainer);
        add_child(hsplit_container);

        VSplitContainer* vsplit_container = memnew(VSplitContainer);
        hsplit_container->add_child(vsplit_container);

        VBoxContainer* fav_vbox = memnew(VBoxContainer);
        fav_vbox->set_custom_minimum_size(Size2(150, 100));
        fav_vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
        vsplit_container->add_child(fav_vbox);

        _favorites = memnew(Tree);
        // set_auto_translate_mode
        _favorites->set_hide_root(true);
        _favorites->set_hide_folding(true);
        _favorites->set_allow_reselect(true);
        _favorites->set_focus_mode(Control::FOCUS_NONE);
        _favorites->connect("cell_selected", callable_mp(this, &OrchestratorEditorSearchDialog::_on_favorite_selected));
        _favorites->connect("item_activated", callable_mp(this, &OrchestratorEditorSearchDialog::_on_favorite_activated));
        _favorites->add_theme_constant_override("draw_guides", 1);
        SceneUtils::add_margin_child(fav_vbox, "Favorites:", _favorites, true);

        VBoxContainer* rec_vbox = memnew(VBoxContainer);
        vsplit_container->add_child(rec_vbox);
        rec_vbox->set_custom_minimum_size(Size2(150, 100));
        rec_vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);

        _recent = memnew(ItemList);
        // set_auto_translate_mode
        _recent->set_allow_reselect(true);
        _recent->set_focus_mode(Control::FOCUS_NONE);
        _recent->connect("item_selected", callable_mp(this, &OrchestratorEditorSearchDialog::_on_history_selected));
        _recent->connect("item_activated", callable_mp(this, &OrchestratorEditorSearchDialog::_on_history_activated));
        _recent->add_theme_constant_override("draw_guides", 1);
        SceneUtils::add_margin_child(rec_vbox, "Recent:", _recent, true);

        VBoxContainer* vbox = memnew(VBoxContainer);
        vbox->set_custom_minimum_size(Size2(300, 0));
        vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        hsplit_container->add_child(vbox);

        _search_box = memnew(LineEdit);
        _search_box->set_clear_button_enabled(true);
        _search_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        _search_box->connect("text_changed", callable_mp(this, &OrchestratorEditorSearchDialog::_on_search_changed));
        _search_box->connect("gui_input", callable_mp(this, &OrchestratorEditorSearchDialog::_on_search_input));

        HBoxContainer* search_hbox = memnew(HBoxContainer);
        search_hbox->add_child(_search_box);

        _favorite = memnew(Button);
        _favorite->set_toggle_mode(true);
        _favorite->set_tooltip_text("(Un)favorite selected item.");
        _favorite->set_focus_mode(Control::FOCUS_NONE);
        _favorite->connect("pressed", callable_mp(this, &OrchestratorEditorSearchDialog::_on_favorite_toggled));
        search_hbox->add_child(_favorite);

        const Vector<FilterOption> filters = _get_filters();
        if (!filters.is_empty())
        {
            _filters = memnew(OptionButton);
            for (const FilterOption& filter : filters)
                _filters->add_item(filter.text, filter.id);

            search_hbox->add_child(_filters);
            _filters->connect("item_selected", callable_mp(this, &OrchestratorEditorSearchDialog::_on_filter_selected));
        }

        SceneUtils::add_margin_child(vbox, "Search:", search_hbox);

        _search_options = memnew(Tree);
        // set_auto_translate_mode
        _search_options->connect("item_activated", callable_mp(this, &OrchestratorEditorSearchDialog::_on_confirmed));
        _search_options->connect("cell_selected", callable_mp(this, &OrchestratorEditorSearchDialog::_on_item_selected));
        SceneUtils::add_margin_child(vbox, "Matches:", _search_options, true);

        _help_bit = memnew(OrchestratorEditorSearchHelpBit);
        SceneUtils::add_margin_child(vbox, "Description:", _help_bit);

        // register_text_enter(_search_box);
        set_hide_on_ok(false);

        connect("confirmed", callable_mp(this, &OrchestratorEditorSearchDialog::_on_confirmed));
        connect("canceled", callable_mp(this, &OrchestratorEditorSearchDialog::_on_canceled));

        _search_box->set_right_icon(SceneUtils::get_editor_icon("Search"));
        _favorite->set_button_icon(SceneUtils::get_editor_icon("Favorites"));
    }
    else if (p_what == NOTIFICATION_EXIT_TREE)
    {
        disconnect("confirmed", callable_mp(this, &OrchestratorEditorSearchDialog::_on_confirmed));
        disconnect("canceled", callable_mp(this, &OrchestratorEditorSearchDialog::_on_canceled));
    }
    else if (p_what == NOTIFICATION_READY)
    {
        if (_filters)
            _filters->select(_get_default_filter());
    }
    else if (p_what == NOTIFICATION_VISIBILITY_CHANGED)
    {
        if (is_visible())
        {
            callable_mp((Control*) _search_box, &Control::grab_focus).call_deferred();
            _search_box->select_all();
        }
    }
    else if (p_what == NOTIFICATION_THEME_CHANGED)
    {
        const int icon_width = get_theme_constant("class_icon_size", "Editor");
        if (_search_options)
        {
            _search_options->add_theme_constant_override("icon_max_width", icon_width);
            _favorites->add_theme_constant_override("icon_max_width", icon_width);
            _recent->set_fixed_icon_size(Size2i(icon_width, icon_width));

            _search_box->set_right_icon(SceneUtils::get_editor_icon("Search"));
            _favorite->set_button_icon(SceneUtils::get_editor_icon("Favorites"));
        }
    }
}

bool OrchestratorEditorSearchDialog::_is_favorite(const Ref<SearchItem>& p_item) const
{
    return _favorite_list.has(p_item);
}

TreeItem* OrchestratorEditorSearchDialog::_populate_search_results()
{
    const String search_text = _search_box->get_text();

    // Generate list of possible candidates
    Vector<Ref<SearchItem>> candidates;
    for (const Ref<SearchItem>& item : _search_items)
    {
        if (search_text.is_empty() || search_text.is_subsequence_ofn(item->name) || search_text.is_subsequence_ofn(item->text))
        {
            if (!_filters || (_filters && !_is_filtered(item, search_text)))
                candidates.push_back(item);
        }
    }

    HashMap<String, TreeItem*> cache;
    for (const Ref<SearchItem>& candidate : candidates)
    {
        Vector<Ref<SearchItem>> item_hierarchy;
        item_hierarchy.push_back(candidate);

        Ref<SearchItem> hierarchy_parent = candidate->parent;
        while (hierarchy_parent.is_valid())
        {
            item_hierarchy.push_back(hierarchy_parent);
            hierarchy_parent = hierarchy_parent->parent;
        }

        item_hierarchy.reverse();

        TreeItem* parent = nullptr;
        while (!item_hierarchy.is_empty())
        {
            const String lookup = item_hierarchy.get(0)->path;
            if (cache.has(lookup))
            {
                parent = cache[lookup];
                item_hierarchy.remove_at(0);
                continue;
            }
            break;
        }

        // Create entries for all remaining hierarchy items
        for (const Ref<SearchItem>& item : item_hierarchy)
        {
            TreeItem* child = parent ? parent->create_child() : _search_options->create_item();
            child->set_text(0, item->text);
            child->set_icon(0, item->icon.is_valid() ? item->icon : nullptr);
            child->set_selectable(0, item->selectable);
            child->set_collapsed(false); //item->collapsed);
            child->set_meta("__item", item);

            // font_disabled_color
            if (item->disabled)
                child->set_custom_color(0, Color(0.875, 0.875, 0.875, 0.5));

            cache[item->path] = child;
            parent = child;

            // todo: move this to using item->collapsed
            _set_search_item_collapse_state(child);
        }
    }

    float highest_score = 0;
    int highest_index = 0;

    for (int i = 0; i < candidates.size(); i++)
    {
        float score = _calculate_score(candidates[i], search_text);
        if (score > highest_score)
        {
            highest_score = score;
            highest_index = i;
        }
    }

    if (!candidates.is_empty() && cache.has(candidates[highest_index]->path))
        return cache[candidates[highest_index]->path];

    return nullptr;
}

void OrchestratorEditorSearchDialog::_update_search_box(bool p_clear, bool p_replace, const String& p_text, bool p_focus)
{
    if (p_clear)
        _search_box->clear();
    else
        _search_box->select_all();

    if (p_replace)
        _search_box->set_text(p_text);

    if (p_focus)
        _search_box->grab_focus();
}

void OrchestratorEditorSearchDialog::_set_search_item_collapse_state(TreeItem* p_item)
{
    if (!_search_box->get_text().is_empty())
    {
        p_item->set_collapsed(false);
    }
    else if (p_item->get_parent())
    {
        bool should_collapse = _get_search_item_collapse_suggestion(p_item);

        Ref<EditorSettings> settings = OrchestratorPlugin::get_singleton()->get_editor_interface()->get_editor_settings();
        if (should_collapse && bool(settings->get_setting("docks/scene_tree/start_create_dialog_fully_expanded")))
            should_collapse = false;

        p_item->set_collapsed(should_collapse);
    }
}

void OrchestratorEditorSearchDialog::_load_favorites_and_history()
{
    Vector<Ref<SearchItem>> recent_items = _get_recent_items();
    for (const Ref<SearchItem>& item : recent_items)
    {
        int32_t index = _recent->add_item(item->text, item->icon);
        _recent->set_item_metadata(index, item);
    }

    _favorite_list = _get_favorite_items();
}

void OrchestratorEditorSearchDialog::_save_and_update_favorites_list()
{
    _favorites->clear();

    _save_favorite_items(_favorite_list);

    _favorite_list = _get_favorite_items();
    TreeItem* root = _favorites->create_item();
    for (const Ref<SearchItem>& favorite : _favorite_list)
    {
        TreeItem* item = _favorites->create_item(root);
        item->set_text(0, favorite->text);
        item->set_icon(0, favorite->icon.is_valid() ? favorite->icon : nullptr);
        item->set_meta("__item", favorite);
    }

    emit_signal("favorites_updated");
}

float OrchestratorEditorSearchDialog::_calculate_score(const Ref<SearchItem>& p_item, const String& p_search) const
{
    const String text = p_item->text;

    if (text == p_search)
    {
        // Always favor an exact match (case-sensitive), since clicking a favorite will set search text to type
        return 1.f;
    }

    float inverse_length = 1.f / float(text.length());

    // Favor types where search term is a substring close to the start of the type.
    float w = 0.5f;
    int pos = text.findn(p_search);
    float score = (pos > -1) ? 1.0f - w * MIN(1, 3 * pos * inverse_length) : MAX(0.f, .9f - w);

    // Favor shorter items: they resemble the search term more.
    w = 0.9f;
    score *= (1 - w) + w * MIN(1.0f, p_search.length() * inverse_length);

    score *= _is_preferred(text) ? 1.0f : 0.9f;

    // Add score for being a favorite type
    score *= (_is_favorite(p_item) ? 1.0f : 0.8f);

    // Look through at most 5 recent items
    bool in_recent = false;
    constexpr int RECENT_COMPLETION_SIZE = 5;
    for (int i = 0; i < MIN(RECENT_COMPLETION_SIZE - 1, _recent->get_item_count()); i++)
    {
        if (_recent->get_item_text(i) == p_item)
        {
            in_recent = true;
            break;
        }
    }
    score *= in_recent ? 1.f : 0.9f;

    // Significantly drop the item's score if its disabled
    if (p_item->disabled)
        score *= 0.1f;

    return score;
}

void OrchestratorEditorSearchDialog::_cleanup()
{
    _favorite_list.clear();
    _favorites->clear();
    _recent->clear();
}

void OrchestratorEditorSearchDialog::_select_item(TreeItem* p_item, bool p_center_on_item)
{
    if (!p_item)
        return;

    p_item->select(0);
    _search_options->scroll_to_item(p_item, p_center_on_item);

    Ref<SearchItem> item = p_item->get_meta("__item");

    _favorite->set_disabled(false);
    _favorite->set_pressed(_is_favorite(item));
    get_ok_button()->set_disabled(false);

    _update_help(item);
}

void OrchestratorEditorSearchDialog::_update_search()
{
    _search_options->clear();

    TreeItem* hit = _populate_search_results();

    if (_search_box->get_text().is_empty())
    {
        _search_options->scroll_to_item(_search_options->get_root());
        _search_options->deselect_all();
    }
    else if (hit)
    {
        _select_item(hit, true);
    }
    else
    {
        _favorite->set_disabled(true);
        _search_options->deselect_all();
        get_ok_button()->set_disabled(true);
    }
}

Ref<OrchestratorEditorSearchDialog::SearchItem> OrchestratorEditorSearchDialog::_get_search_item_by_name(const String& p_name) const
{
    for (const Ref<SearchItem>& item : _search_items)
        if (item->name.match(p_name))
            return item;

    return {};
}

void OrchestratorEditorSearchDialog::popup_create(bool p_dont_clear, bool p_replace_mode, const String& p_current_type, const String& p_current_name)
{
    _search_items = _get_search_items();

    Ref<SearchItem> initial_item = _get_search_item_by_name(p_current_type);
    String search_value = initial_item.is_valid() ? initial_item->text : p_current_type;

    _load_favorites_and_history();

    _update_search_box(!p_dont_clear, p_replace_mode, search_value, true);
    _update_search();

    _save_and_update_favorites_list();

    EditorInterface* ei = OrchestratorPlugin::get_singleton()->get_editor_interface();
    Rect2 saved_size = ei->get_editor_settings()->get_project_metadata("dialog_bounds", "create_new_node", Rect2());
    if (saved_size != Rect2())
        popup(saved_size);
    else
        popup_centered_clamped(Size2(900, 700), 0.8);
}

void OrchestratorEditorSearchDialog::_on_favorite_selected()
{
    TreeItem* item = _favorites->get_selected();
    if (!item)
        return;

    const Ref<SearchItem> search_item = item->get_meta("__item");
    if (search_item.is_valid() && !search_item->text.is_empty())
    {
        _search_box->set_text(search_item->text);
        _recent->deselect_all();
        _update_search();
    }
}

void OrchestratorEditorSearchDialog::_on_favorite_activated()
{
    _on_favorite_selected();
    _on_confirmed();
}

void OrchestratorEditorSearchDialog::_on_history_selected(int p_index)
{
    const Ref<SearchItem> item = _recent->get_item_metadata(p_index);
    if (item.is_valid())
    {
        _search_box->set_text(item->text);
        _favorites->deselect_all();
        _update_search();
    }
}

void OrchestratorEditorSearchDialog::_on_history_activated(int p_index)
{
    _on_history_selected(p_index);
    _on_confirmed();
}

void OrchestratorEditorSearchDialog::_on_search_changed(const String& p_text)
{
    _update_search();
}

void OrchestratorEditorSearchDialog::_on_search_input(const Ref<InputEvent>& p_event)
{
    Ref<InputEventKey> key = p_event;
    if (key.is_valid() && key->is_pressed())
    {
        switch (key->get_keycode())
        {
            case KEY_UP:
            case KEY_DOWN:
            case KEY_PAGEUP:
            case KEY_PAGEDOWN:
            {
                _search_options->_gui_input(p_event);
                _search_box->accept_event();
                break;
            }
            case KEY_SPACE:
            {
                TreeItem* item = _search_options->get_selected();
                if (item)
                    item->set_collapsed(!item->is_collapsed());
                _search_box->accept_event();
                break;
            }
            default:
                break;
        }
    }
}

void OrchestratorEditorSearchDialog::_on_favorite_toggled()
{
    TreeItem* item = _search_options->get_selected();
    if (!item)
        return;

    Ref<SearchItem> search_item = item->get_meta("__item");
    if (search_item.is_valid() && _favorite_list.has(search_item))
    {
        _favorite_list.erase(search_item);
        _favorite->set_pressed(false);
    }
    else
    {
        _favorite_list.push_back(search_item);
        _favorite->set_pressed(true);
    }

    _save_and_update_favorites_list();
}

void OrchestratorEditorSearchDialog::_on_confirmed()
{
    TreeItem* selected = _search_options->get_selected();
    if (!selected)
        return;

    Ref<SearchItem> search_item = selected->get_meta("__item");
    if (!search_item.is_valid())
        return;

    // todo: convert to SearchItem?
    if (!search_item->selectable)
        return;

    constexpr int RECENT_HISTORY_SIZE = 15;

    Vector<Ref<SearchItem>> to_be_saved;
    to_be_saved.push_back(search_item);

    for (int i = 0; i < MIN(RECENT_HISTORY_SIZE - 1, _recent->get_item_count()); i++)
    {
        const Ref<SearchItem> item = _recent->get_item_metadata(i);
        if (item.is_valid())
            to_be_saved.push_back(item);
    }

    _save_recent_items(to_be_saved);

    hide();

    emit_signal("selected");
    _cleanup();
}

void OrchestratorEditorSearchDialog::_on_canceled()
{
    _cleanup();
}

void OrchestratorEditorSearchDialog::_on_item_selected()
{
    TreeItem* item = _search_options->get_selected();
    if (!item)
        return;

    _select_item(item, false);
}

void OrchestratorEditorSearchDialog::_on_filter_selected(int p_index)
{
    _filter_type_changed(p_index);
    _update_search();
}
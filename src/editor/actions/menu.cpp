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
#include "editor/actions/menu.h"

#include "common/callable_lambda.h"
#include "common/file_utils.h"
#include "common/macros.h"
#include "common/scene_utils.h"
#include "common/settings.h"
#include "core/godot/scene_string_names.h"
#include "godot_cpp/templates/hash_map.hpp"
#include "help.h"
#include "introspector.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_split_container.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/v_split_container.hpp>
#include <godot_cpp/classes/worker_thread_pool.hpp>

bool OrchestratorEditorActionMenu::_is_favorite(const Variant& p_value, int& r_index) {
    const Ref<OrchestratorEditorActionDefinition> action = p_value;
    if (!action.is_valid()) {
        return false;
    }

    const String qualified_action_name = vformat("%s/%s", action->category, action->name);

    r_index = -1;
    for (int i = 0; i < _favorites->get_item_count(); i++) {
        const Variant& metadata = _favorites->get_item_metadata(i);
        if (metadata == qualified_action_name) {
            r_index = i;
            return true;
        }
    }
    return false;
}

void OrchestratorEditorActionMenu::_favorite_selected(int p_index) {
    const String text = _favorites->get_item_text(p_index);
    _search_box->set_text(text);
    _favorites->deselect_all();
    _update_search();
}

void OrchestratorEditorActionMenu::_favorite_activated(int p_index) {
    _favorite_selected(p_index);
    _confirmed();
}

void OrchestratorEditorActionMenu::_recent_selected(int p_index) {
    const String text = _recents->get_item_text(p_index);
    _search_box->set_text(text);
    _recents->deselect_all();
    _update_search();
}

void OrchestratorEditorActionMenu::_search_changed(const String& p_text) {
    _update_search();
}

void OrchestratorEditorActionMenu::_search_gui_input(const Ref<InputEvent>& p_event) {
    const Ref<InputEventKey> key = p_event;
    if (key.is_valid() && key->is_pressed()) {
        switch (key->get_keycode()) {
            case KEY_UP:
            case KEY_DOWN:
            case KEY_PAGEUP:
            case KEY_PAGEDOWN: {
                // Redirect these to the results pane
                _results->_gui_input(p_event);
                _search_box->accept_event();
                break;
            }
            default: {
                break;
            }
        }
    }
}

void OrchestratorEditorActionMenu::_item_selected() {
    TreeItem* item = _results->get_selected();
    if (item) {
        _select_item(item, false);
    }
}
void OrchestratorEditorActionMenu::_nothing_selected() {
    get_ok_button()->set_disabled(true);
}

void OrchestratorEditorActionMenu::_toggle_favorite() {
    TreeItem* item = _results->get_selected();
    if (item) {
        const Variant& value = item->get_metadata(0);
        if (value.get_type() == Variant::OBJECT) {
            const Ref<OrchestratorEditorActionDefinition> action = value;

            int index = -1;
            if (_is_favorite(value, index)) {
                _favorites->remove_item(index);
                _favorite_button->set_pressed(false);
            } else if (action.is_valid()) {
                const String qualified_name = vformat("%s/%s", action->category, action->name);

                index = _favorites->add_item(action->name, _get_cached_icon(action->icon));
                _favorites->set_item_metadata(index, qualified_name);

                _favorite_button->set_pressed(true);
            }
        }
    }
}

void OrchestratorEditorActionMenu::_add_recent(const Ref<OrchestratorEditorActionDefinition>& p_action) {
    if (!p_action.is_valid()) {
        return;
    }

    // Add selection to recents
    const String qualified_name = vformat("%s/%s", p_action->category, p_action->name);

    int index = -1;
    for (int i = 0; i < _recents->get_item_count(); i++) {
        if (_recents->get_item_metadata(i) == qualified_name) {
            index = i;
            break;
        }
    }

    // Add new
    if (index < 0) {
        index = _recents->add_item(p_action->name, _get_cached_icon(p_action->icon));
        _recents->set_item_metadata(index, qualified_name);
    }

    _recents->move_item(index, 0);
}

void OrchestratorEditorActionMenu::_filter_changed(int p_index) {
    _update_search();
}

void OrchestratorEditorActionMenu::_about_to_popup() {
    _perform_background_sort();
}

void OrchestratorEditorActionMenu::_visibility_changed() {
    if (!is_visible()) {
        PROJECT_SET("Orchestrator", "action_menu_bounds", Rect2(get_position(), get_size()));

        // Reset dialog state
        _favorites->clear();
        _recents->clear();
        _search_box->clear();
        _results->clear();
        _favorite_button->set_pressed(false);
    } else {
        _load_user_data();

        callable_mp_cast(_search_box, Control, grab_focus).call_deferred(false);
        _search_box->select_all();
    }
}

void OrchestratorEditorActionMenu::_focus_lost() {
    if (_close_on_focus_lost) {
        emit_signal(SceneStringName(canceled));
    }
}

void OrchestratorEditorActionMenu::_confirmed() {
    TreeItem* selected = _results->get_selected();
    if (selected) {
        Variant metadata = selected->get_metadata(0);

        const Ref<OrchestratorEditorActionDefinition> action = metadata;
        if (action.is_valid()) {
            _add_recent(action);
            emit_signal("action_selected", metadata);
        }
    }

    _save_user_data();

    hide();
    queue_free();
}

void OrchestratorEditorActionMenu::_select_item(TreeItem* p_item, bool p_center_on_item) {
    if (p_item) {
        p_item->select(0);
        _results->scroll_to_item(p_item, p_center_on_item);

        _favorite_button->set_disabled(false);

        int index = -1;
        Variant metadata = p_item->get_metadata(0);
        _favorite_button->set_pressed(_is_favorite(metadata, index));

        get_ok_button()->set_disabled(false);

        // todo: we need to improve the display of the help bit some more
        Ref<OrchestratorEditorActionDefinition> action = metadata;
        if (action.is_valid()) {
            _help->parse_action(action);
        }
    }
    else
        get_ok_button()->set_disabled(true);
}

void OrchestratorEditorActionMenu::_toggle_collapsed(bool p_collapsed) {
    _collapse_button->set_pressed_no_signal(p_collapsed);
    _expand_button->set_pressed_no_signal(!p_collapsed);

    if (_results->get_root()) {
        TreeItem* child = _results->get_root()->get_first_child();
        while (child) {
            child->set_collapsed_recursive(p_collapsed);
            child = child->get_next();
        }
    }
}

void OrchestratorEditorActionMenu::_recent_activated(int p_index) {
    _recent_selected(p_index);
    _confirmed();
}

struct ActionSortByCategoryAndName {
    _FORCE_INLINE_ bool operator()(const Ref<OrchestratorEditorActionDefinition>& a, const Ref<OrchestratorEditorActionDefinition>& b) const {
        const String a_name = a->category.is_empty() ? a->name : (a->category + "/" + a->name);
        const String b_name = b->category.is_empty() ? b->name : (b->category + "/" + b->name);

        const int a_priority = _get_priority(a_name);
        const int b_priority = _get_priority(b_name);
        if (a_priority != b_priority) {
            return a_priority < b_priority;
        }

        return a_name.naturalnocasecmp_to(b_name) < 0;
    }

private:
    _FORCE_INLINE_ static int _get_priority(const String& p_name) {
        if (p_name == "Project/") {
            return 0;
        }
        else if (p_name == "@OScript/") {
            return 1;
        }
        else {
            return 100;
        }
    }
};

TreeItem* OrchestratorEditorActionMenu::_find_first_selectable(TreeItem* p_item) {
    if (!p_item) {
        return nullptr;
    }

    if (p_item->is_selectable(0)) {
        return p_item;
    }

    TreeItem* child = p_item->get_first_child();
    while (child) {
        TreeItem* result = _find_first_selectable(child);
        if (result) {
            return result;
        }
        child = child->get_next();
    }

    return nullptr;
}

void OrchestratorEditorActionMenu::_prune_empty_categories(TreeItem* p_item) {
    if (!p_item || p_item->is_selectable(0) || p_item->get_first_child()) {
        return;
    }

    TreeItem* parent = p_item->get_parent();
    if (parent) {
        parent->remove_child(p_item);
    } else if (p_item == p_item->get_tree()->get_root()->get_first_child()) {
        p_item->get_tree()->get_root()->remove_child(p_item);
    }

    memdelete(p_item);

    // Recurse up
    _prune_empty_categories(parent);
}

void OrchestratorEditorActionMenu::_update_search() {
    // When the dialog first opens, the action list is sorted in a background thread.
    // If this method is called for any reason before sorting concludes, we skip it.
    // The _perform_background_sort worker thread will call this function when the
    // sort has completed on the main thread.
    if (_sorting) {
        return;
    }

    const String query = _search_box->get_text(); //.strip_edges(); //.trim_prefix(" ").trim_suffix(" ");

    FilterContext context;
    context.context_sensitive = true;
    context.query = query;
    context._filter_action_type = -1;

    // Filter actions
    Vector<ScoredAction> filtered_actions = _filter_engine->filter_actions(_actions, context);

    _results->clear();
    TreeItem* root = _results->create_item();
    root->set_selectable(0, false);

    // Hide the tree while we populate.
    // When adding lots of items with TreeItem::set_icon, there are lots of redraws
    // that get triggered, adding up to several seconds of latency.
    _results->hide();

    HashMap<String, TreeItem*> category_items;
    PackedStringArray sorted_keys;
    HashMap<String, Ref<OrchestratorEditorActionDefinition>> category_definitions;
    Vector<ScoredAction> leaf_items;

    // Pass 1: Sort into categories and leaf nodes
    for (int i = 0; i < filtered_actions.size(); i++) {
        const ScoredAction& scored_action = filtered_actions[i];
        const Ref<OrchestratorEditorActionDefinition>& action = filtered_actions[i].action;
        if (action->selectable) {
            leaf_items.push_back(scored_action);

            // Check if the leaf action's category is defined.
            // If it isn't, automatically inject a temporary one.
            // Should the user define one later with icons, the dummy one is replaced.
            if (!category_definitions.has(action->category) && !action->category.is_empty()) {
                Vector<Ref<OrchestratorEditorActionDefinition>> category_actions =
                    OrchestratorEditorIntrospector::generate_actions_from_category(action->category);

                for (const Ref<OrchestratorEditorActionDefinition>& category_action : category_actions) {
                    if (!category_definitions.has(category_action->category)) {
                        category_definitions[category_action->category] = category_action;
                        sorted_keys.push_back(category_action->category);
                    }
                }
            }
        } else if (!action->category.is_empty()) {
            category_definitions[action->category] = action;
            sorted_keys.push_back(action->category);
        }
    }

    // Pass 2: Create all category tree items in order
    const String separator = "/";
    HashMap<String, Ref<Texture2D>> icon_cache;
    for (const String& path : sorted_keys) {
        const Ref<OrchestratorEditorActionDefinition>& category = category_definitions[path];
        const PackedStringArray segments = path.split(separator);

        String cumulative_path;
        TreeItem* parent = root;

        for (int i = 0; i < segments.size(); i++) {
            if (i > 0) {
                cumulative_path += separator;
            }
            cumulative_path += segments[i];

            if (!category_items.has(cumulative_path)) {
                TreeItem* item = _results->create_item(parent);
                item->set_text(0, segments[i]);
                item->set_selectable(0, false);

                if (i == segments.size() - 1 && !category->icon.is_empty()) {
                    item->set_icon(0, SceneUtils::get_class_icon(category->icon));
                }

                category_items[cumulative_path] = item;
            }

            parent = category_items[cumulative_path];
        }
    }

    // Pass 3: Add selectable action items and track best match
    TreeItem* best_match = nullptr;
    float best_score = -1;
    for (int i = 0; i < leaf_items.size(); i++) {
        const ScoredAction& scored_action = leaf_items[i];
        const Ref<OrchestratorEditorActionDefinition>& leaf = scored_action.action;
        const String& path = leaf->category;

        TreeItem* parent = category_items.has(path) ? category_items[path] : root;

        TreeItem* item = _results->create_item(parent);
        item->set_text(0, leaf->no_capitalize ? leaf->name : leaf->name.capitalize());
        item->set_selectable(0, leaf->selectable);

        if (!leaf->icon.is_empty()) {
            item->set_icon(0, SceneUtils::get_class_icon(leaf->icon));
        }

        item->set_metadata(0, leaf);

        if (leaf->flags & OrchestratorEditorActionDefinition::FLAG_EXPERIMENTAL) {
            item->add_button(0, SceneUtils::get_editor_icon("NodeWarning"));
            item->set_button_tooltip_text(0, 0, "This is marked as experimental.");
        }

        float score = query.is_empty() ? 0.f : scored_action.score;
        if (score > best_score) {
            best_score = score;
            best_match = item;
        }
    }

    // Pass 4: Prune all unused categories
    for (const KeyValue<String, TreeItem*>& E : category_items) {
        TreeItem* item = E.value;
        if (!item->is_selectable(0) && item->get_first_child() == nullptr) {
            _prune_empty_categories(item);
        }
    }

    // The results are hidden during population
    _results->show();

    if (!query.is_empty() && best_match) {
        _results->set_selected(best_match, 0);
        _results->scroll_to_item(best_match);

        _toggle_collapsed(false);
    } else {
        if (TreeItem* first_selectable = _find_first_selectable(_results->get_root())) {
            _results->set_selected(first_selectable, 0);
        }
        if (_results->get_root()) {
            _results->scroll_to_item(root);
        }

        _toggle_collapsed(_start_collapsed);

        if (_start_collapsed) {
            _results->deselect_all();
            _help->parse_action(Ref<OrchestratorEditorActionDefinition>());
        }
    }

    get_ok_button()->set_disabled(!_results->get_selected());
}

void OrchestratorEditorActionMenu::_load_file_into_list(const String& p_filename, ItemList* p_list) {
    // User data is always store in an encoded way to make it easy to be reloaded.
    // Format is as follows:
    //
    //      [format version]
    //      [blank]
    //      [fully qualified action item]
    //      [description]
    //      [icon]
    //      [blank]
    //      starts next action item...
    //

    PackedStringArray items;
    const Ref<FileAccess> favorites = FileUtils::open_project_settings_file(p_filename, FileAccess::READ);
    FileUtils::for_each_line(favorites, [&](const String& line) {
        items.push_back(line.strip_edges());
    });

    if (!items.is_empty()) {
        const int64_t format = items[0].is_valid_int() ? items[0].to_int() : 0;
        if (format > 0) {
            // For now there is no difference in formats, so it's merely a formality
            for (int index = 2; (index + 2) < items.size(); index += 4) {
                const String& fully_qualified_action = items[index];
                const String& description = items[index + 1];
                const String& icon_name = items[index + 2];
                const Ref<Texture2D> icon = _get_cached_icon(icon_name);

                const int32_t id = p_list->add_item(description, icon, true);
                p_list->set_item_metadata(id, fully_qualified_action);
            }
        }
    }
}

void OrchestratorEditorActionMenu::_save_list_into_file(ItemList* p_list, const String& p_filename, int64_t p_max) {
    PackedStringArray items;
    items.push_back("1");
    items.push_back("");
    for (int i = 0; i < p_list->get_item_count(); i++) {
        items.push_back(p_list->get_item_metadata(i));
        items.push_back(p_list->get_item_text(i));
        items.push_back(_get_cached_icon_name(p_list->get_item_icon(i)));
        items.push_back("");

        if (p_max > 0 && i + 1 >= p_max) {
            break;
        }
    }

    const Ref<FileAccess> file = FileUtils::open_project_settings_file(p_filename, FileAccess::WRITE);
    if (file.is_valid()) {
        for (const String& line : items) {
            file->store_line(line);
        }
    }
}

void OrchestratorEditorActionMenu::_load_user_data() {
    _load_file_into_list(vformat("orchestrator_menu_favorites.%s", _suffix), _favorites);
    _load_file_into_list(vformat("orchestrator_menu_recents.%s", _suffix), _recents);
}

void OrchestratorEditorActionMenu::_save_user_data() {
    constexpr int64_t RECENT_HISTORY_MAX_SIZE = 15;

    _save_list_into_file(_favorites, vformat("orchestrator_menu_favorites.%s", _suffix));
    _save_list_into_file(_recents, vformat("orchestrator_menu_recents.%s", _suffix), RECENT_HISTORY_MAX_SIZE);
}

String OrchestratorEditorActionMenu::_get_cached_icon_name(const Ref<Texture2D>& p_texture) {
    for (const KeyValue<String, Ref<Texture2D>>& E : _icon_cache) {
        if (E.value == p_texture) {
            return E.key;
        }
    }

    return "Broken";
}

Ref<Texture2D> OrchestratorEditorActionMenu::_get_cached_icon(const String& p_icon_name) {
    if (_icon_cache.has(p_icon_name)) {
        return _icon_cache[p_icon_name];
    }

    Ref<Texture2D> icon = SceneUtils::get_class_icon(p_icon_name);
    _icon_cache[p_icon_name] = icon;

    return icon;
}

void OrchestratorEditorActionMenu::_perform_background_sort() {
    _sorting = true;

    WorkerThreadPool::get_singleton()->add_task(callable_mp_lambda(this, [&] {
        Vector<Ref<OrchestratorEditorActionDefinition>> sorted = _actions;
        sorted.sort_custom<OrchestratorEditorActionDefinitionComparator>();

        _actions = sorted;
        _sorting = false;

        callable_mp_this(_update_search).call_deferred();
    }));
}

void OrchestratorEditorActionMenu::set_start_collapsed(bool p_start_collapsed) {
    _start_collapsed = p_start_collapsed;

    if (_start_collapsed) {
        _collapse_button->set_pressed_no_signal(true);
    } else {
        _expand_button->set_pressed_no_signal(true);
    }
}

void OrchestratorEditorActionMenu::popup_centered(const Vector<Ref<OrchestratorEditorActionDefinition>>& p_actions,
                                                  const Ref<OrchestratorEditorActionFilterEngine>& p_filter_engine)
{
    _actions = p_actions;
    _filter_engine = p_filter_engine;

    if (_last_size.get_size() != Vector2()) {
        EI->popup_dialog_centered(this, _last_size.get_size());
    } else {
        EI->popup_dialog_centered_clamped(this, _default_rect.get_size());
    }
}

void OrchestratorEditorActionMenu::popup(const Vector2& p_position,
                                         const Vector<Ref<OrchestratorEditorActionDefinition>>& p_actions,
                                         const Ref<OrchestratorEditorActionFilterEngine>& p_filter_engine) {
    _actions = p_actions;
    _filter_engine = p_filter_engine;

    // If the last size has no size, use the default
    if (_last_size.get_size() == Vector2()) {
        _last_size.set_size(_default_rect.get_size());
    }

    bool center_at_mouse = ORCHESTRATOR_GET("ui/actions_menu/center_on_mouse", true);
    if (center_at_mouse) {
        _last_size.set_position(p_position - (_last_size.get_size() / 2.0));
    } else {
        _last_size.set_position(p_position);
    }

    EI->popup_dialog(this, _last_size);
}

void OrchestratorEditorActionMenu::_bind_methods() {
    ADD_SIGNAL(MethodInfo("action_selected", PropertyInfo(Variant::OBJECT, "action")));
}

OrchestratorEditorActionMenu::OrchestratorEditorActionMenu()
{
    // Separates the Favortes/Recents
    HSplitContainer* hsplit = memnew(HSplitContainer);
    add_child(hsplit);

    // Separates the Favorites/Recents from the Search/Help sections
    VSplitContainer* vsplit = memnew(VSplitContainer);
    hsplit->add_child(vsplit);

    VBoxContainer* fav_vbox = memnew(VBoxContainer);
    fav_vbox->set_custom_minimum_size(Size2(150, 100));
    fav_vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    vsplit->add_child(fav_vbox);

    VBoxContainer* recents_vbox = memnew(VBoxContainer);
    recents_vbox->set_custom_minimum_size(Size2(150, 100));
    recents_vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    vsplit->add_child(recents_vbox);

    _favorites = memnew(ItemList);
    _favorites->set_allow_reselect(true);
    _favorites->set_focus_mode(Control::FOCUS_NONE);
    _favorites->connect(SceneStringName(item_selected), callable_mp_this(_favorite_selected));
    _favorites->connect(SceneStringName(item_activated), callable_mp_this(_favorite_activated));
    _favorites->add_theme_constant_override("draw_guides", 1);

    SceneUtils::add_margin_child(fav_vbox, "Favorites:", _favorites, true);

    _recents = memnew(ItemList);
    _recents->set_allow_reselect(true);
    _recents->set_focus_mode(Control::FOCUS_NONE);
    _recents->connect(SceneStringName(item_selected), callable_mp_this(_recent_selected));
    _recents->connect(SceneStringName(item_activated), callable_mp_this(_recent_activated));
    _recents->add_theme_constant_override("draw_guides", 1);

    SceneUtils::add_margin_child(recents_vbox, "Recent:", _recents, true);

    VBoxContainer* vbox = memnew(VBoxContainer);
    vbox->set_custom_minimum_size(Size2(300, 0));
    vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    hsplit->add_child(vbox);

    _search_box = memnew(LineEdit);
    _search_box->set_clear_button_enabled(true);
    _search_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    _search_box->set_right_icon(SceneUtils::get_editor_icon("Search"));
    _search_box->connect(SceneStringName(text_changed), callable_mp_this(_search_changed));
    _search_box->connect(SceneStringName(gui_input), callable_mp_this(_search_gui_input));

    _favorite_button = memnew(Button);
    _favorite_button->set_toggle_mode(true);
    _favorite_button->set_tooltip_text("(Un)favorite selected item.");
    _favorite_button->set_focus_mode(Control::FOCUS_NONE);
    _favorite_button->set_button_icon(SceneUtils::get_editor_icon("Favorites"));
    _favorite_button->connect(SceneStringName(pressed), callable_mp_this(_toggle_favorite));

    _collapse_button = memnew(Button);
    _collapse_button->set_toggle_mode(true);
    _collapse_button->set_tooltip_text("Collapse search results");
    _collapse_button->set_focus_mode(Control::FOCUS_NONE);
    _collapse_button->set_button_icon(SceneUtils::get_editor_icon("CollapseTree"));
    _collapse_button->connect(SceneStringName(pressed), callable_mp_this(_toggle_collapsed).bind(true));

    _expand_button = memnew(Button);
    _expand_button->set_toggle_mode(true);
    _expand_button->set_tooltip_text("Expand search results");
    _expand_button->set_focus_mode(Control::FOCUS_NONE);
    _expand_button->set_button_icon(SceneUtils::get_editor_icon("ExpandTree"));
    _expand_button->connect(SceneStringName(pressed), callable_mp_this(_toggle_collapsed).bind(false));

    _filter_options = memnew(OptionButton);
    _filter_options->add_item("Display All");
    _filter_options->add_separator();
    _filter_options->connect(SceneStringName(item_selected), callable_mp_this(_filter_changed));

    HBoxContainer* search_hbox = memnew(HBoxContainer);
    search_hbox->add_child(_search_box);
    search_hbox->add_child(_favorite_button);
    search_hbox->add_child(_collapse_button);
    search_hbox->add_child(_expand_button);
    search_hbox->add_child(_filter_options);

    SceneUtils::add_margin_child(vbox, "Search:", search_hbox);

    _results = memnew(Tree);
    _results->set_hide_root(true);
    _results->add_theme_constant_override("icon_max_width", SceneUtils::get_editor_class_icon_size());
    _results->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
    _results->connect(SceneStringName(item_activated), callable_mp_this(_confirmed));
    _results->connect("cell_selected", callable_mp_this(_item_selected));
    _results->connect("nothing_selected", callable_mp_this(_nothing_selected));

    SceneUtils::add_margin_child(vbox, "Matches:", _results, true);

    _help = memnew(OrchestratorEditorActionHelp);
    _help->set_content_help_limits(80 * EDSCALE, 80 * EDSCALE);
    SceneUtils::add_margin_child(vbox, "Description:", _help);

    register_text_enter(_search_box);
    set_hide_on_ok(false);

    connect("about_to_popup", callable_mp_this(_about_to_popup));
    connect(SceneStringName(visibility_changed), callable_mp_this(_visibility_changed));
    connect(SceneStringName(confirmed), callable_mp_this(_confirmed));
    connect(SceneStringName(canceled), callable_mp_cast(this, Node, queue_free));
    connect(SceneStringName(focus_exited), callable_mp_this(_focus_lost));

    // Attempt to use Orchestrator bounds, falling back to Godot
    _last_size = PROJECT_GET("Orchestrator", "action_menu_bounds", Rect2());
    if (_last_size == Rect2()) {
        _last_size = PROJECT_GET("dialog_bounds", "create_new_node", Rect2());
    }
}

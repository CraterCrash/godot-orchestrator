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
#include "editor/actions/definition.h"
#include "editor/actions/introspector.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_split_container.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/v_split_container.hpp>
#include <godot_cpp/classes/worker_thread_pool.hpp>
#include <godot_cpp/templates/hash_map.hpp>

namespace {
    // Per-frame work budget for the search runner. A 60Hz frame is ~16ms; spending up
    // to ~8ms here keeps results filling quickly while leaving the UI responsive. The
    // build updates the tree in place (rather than clearing and rebuilding it), so even
    // when a search spans several frames the tree is never shown empty -- it refines
    // progressively instead of flickering.
    constexpr uint64_t SEARCH_TIME_SLICE_USEC = 8000;

    // Orders stale category items so the deepest are removed first. Combined with
    // removing leaves before any category, this guarantees each item is childless when
    // it is detached, so it can be pooled cleanly.
    struct SweepCategoryDepthDescending {
        _FORCE_INLINE_ bool operator()(TreeItem* p_a, TreeItem* p_b) const {
            return String(p_a->get_metadata(0)).count("/") > String(p_b->get_metadata(0)).count("/");
        }
    };
}

void OrchestratorEditorActionMenu::TreeItemCache::clear() {
    for (const KeyValue<String, TreeItem*>& E : items) {
        memdelete(E.value);
    }
    items.clear();
}

String OrchestratorEditorActionMenu::Runner::_cache_key(TreeItem* p_item) const {
    const Variant metadata = p_item->get_metadata(0);
    if (metadata.get_type() == Variant::OBJECT) {
        const Ref<OrchestratorEditorActionDefinition> action = metadata;
        if (action.is_valid()) {
            return LEAF + action->category + "/" + action->name;
        }
    }

    // Category items store their cumulative path as metadata.
    return CATEGORY + String(metadata);
}

TreeItem* OrchestratorEditorActionMenu::Runner::_obtain(TreeItem* p_parent, const String& p_parent_path, const String& p_key, bool& r_created) {
    HashMap<String, TreeItem*>& displayed = _menu->_displayed;
    HashMap<String, TreeItem*>& pool = _menu->_tree_cache.items;

    r_created = false;
    TreeItem* item = nullptr;

    if (displayed.has(p_key)) {
        // Already attached from a previous run: keep it in place.
        item = displayed[p_key];
    } else if (pool.has(p_key)) {
        // Re-attach a pooled item; cheaper than recreating it and reloading its icon.
        item = pool[p_key];
        pool.erase(p_key);
        p_parent->add_child(item);
        displayed[p_key] = item;
    } else {
        // Brand new item.
        item = _menu->_results->create_item(p_parent);
        displayed[p_key] = item;
        r_created = true;
    }

    // Keep children in source order without disturbing items that are already correctly placed.
    // In other words, position this item immediately after the previous sibling we placed under the parent.
    // When narrowing, surviving items are already in order, so these checks short-circuit and nothing moves.
    // This avoids any tree flickering.
    TreeItem* prev = _last_child.has(p_parent_path) ? _last_child[p_parent_path] : nullptr;
    if (!prev) {
        TreeItem* first = p_parent->get_first_child();
        if (first != item) {
            item->move_before(first);
        }
    } else if (prev->get_next() != item) {
        item->move_after(prev);
    }

    _last_child[p_parent_path] = item;
    _seen.insert(p_key);
    return item;
}

TreeItem* OrchestratorEditorActionMenu::Runner::_ensure_category(const String& p_path) {
    if (p_path.is_empty()) {
        return _root;
    }

    // Apply expected collapse state to each category, so they're initially rendered correctly.
    // Otherwise, categories would briefly render as expanded (the default) across frames, and
    // then be collapsed in _finish_search when the search query is empty, producing a flicker.
    //
    // Non-empty query: Always expand results
    // Empty query: Always collapse results
    const bool collapse = _query.is_empty() ? _menu->_start_collapsed : false;

    const PackedStringArray segments = p_path.split("/");
    String cumulative;
    TreeItem* parent = _root;

    for (int i = 0; i < segments.size(); i++) {
        const String parent_path = cumulative;  // path of the parent (empty == root)
        if (i > 0) {
            cumulative += "/";
        }
        cumulative += segments[i];

        if (_category_items.has(cumulative)) {
            parent = _category_items[cumulative];
            continue;
        }

        bool created = false;
        TreeItem* item = _obtain(parent, parent_path, CATEGORY + cumulative, created);
        if (created) {
            item->set_text(0, segments[i]);
            item->set_selectable(0, false);
            item->set_metadata(0, cumulative);

            if (_menu->_category_definitions.has(cumulative)) {
                const Ref<OrchestratorEditorActionDefinition>& definition = _menu->_category_definitions[cumulative];
                if (!definition->icon.is_empty()) {
                    item->set_icon(0, _menu->_get_cached_icon(definition->icon));
                }
            }
        }

        // Reused items carry a stale collapse state, so set it unconditionally.
        item->set_collapsed(collapse);

        _category_items[cumulative] = item;
        parent = item;
    }

    return parent;
}

void OrchestratorEditorActionMenu::Runner::_add_leaf(const Ref<OrchestratorEditorActionDefinition>& p_leaf, float p_score) {
    TreeItem* parent = _ensure_category(p_leaf->category);

    bool created = false;
    TreeItem* item = _obtain(parent, p_leaf->category, LEAF + p_leaf->category + "/" + p_leaf->name, created);
    if (created) {
        item->set_text(0, p_leaf->no_capitalize ? p_leaf->name : p_leaf->name.capitalize());
        item->set_selectable(0, p_leaf->selectable);

        if (!p_leaf->icon.is_empty()) {
            item->set_icon(0, _menu->_get_cached_icon(p_leaf->icon));
        }

        item->set_metadata(0, p_leaf);

        if (p_leaf->flags & OrchestratorEditorActionDefinition::FLAG_EXPERIMENTAL) {
            item->add_button(0, SceneUtils::get_editor_icon("NodeWarning"));
            item->set_button_tooltip_text(0, 0, "This is marked as experimental.");
        }
    }

    const float score = _query.is_empty() ? 0.f : p_score;
    if (score > _best_score) {
        _best_score = score;
        _best_match = item;
    }
}

void OrchestratorEditorActionMenu::Runner::_collect_sweep() {
    // Anything still displayed but not touched this run no longer matches and must be removed.
    // Detaches leaves first, then categories deepest-first, so every removed item is childless when detached.
    // This allows returning items to the pool cleanly.
    //
    // Items are NOT erased from _displayed here. The actual detach/erase/pool happens together, per item, in P2.
    // Otherwise, a sweep interrupted by a keystroke would leave items erased from _displayed yet still attached.
    // These become invisible to every future runner, so they are never reused ore removed.
    Vector<TreeItem*> categories;

    for (const KeyValue<String, TreeItem*>& E : _menu->_displayed) {
        if (_seen.has(E.key)) {
            continue;
        }

        if (E.key.begins_with(LEAF)) {
            _sweep.push_back(E.value);
        } else {
            categories.push_back(E.value);
        }
    }

    categories.sort_custom<SweepCategoryDepthDescending>();
    for (TreeItem* category : categories) {
        _sweep.push_back(category);
    }
}

bool OrchestratorEditorActionMenu::Runner::work(uint64_t p_time_slice_usec) {
    if (_done) {
        return true;
    }

    if (!_initialized) {
        // Reuse the existing root and update its contents in place; the tree is never
        // cleared, so it is never shown empty mid-build.
        TreeItem* root = _menu->_results->get_root();
        _root = root ? root : _menu->_results->create_item();
        _root->set_selectable(0, false);
        _initialized = true;
    }

    const uint64_t until = Time::get_singleton()->get_ticks_usec() + p_time_slice_usec;

    // Phase 1: filter the source, adding or keeping (in place) every matching item.
    while (!_build_done && _cursor < _source.size()) {
        const Ref<OrchestratorEditorActionDefinition>& action = _source[_cursor++];

        float score = 1.f;
        if (_menu->_filter_engine->filter_action(action, _context, score)) {
            // Track every match so a subsequent narrowed query can re-scan just these.
            _filtered.push_back(action);

            if (action->selectable) {
                _add_leaf(action, score);
            }
        }

        if (Time::get_singleton()->get_ticks_usec() > until) {
            return false;
        }
    }

    if (!_build_done) {
        _build_done = true;
        _collect_sweep();
    }

    // Phase 2: remove items that no longer match, pooling them for later reuse. Erase,
    // detach and pool together so the state stays consistent if a keystroke interrupts.
    while (_sweep_cursor < _sweep.size()) {
        TreeItem* item = _sweep[_sweep_cursor++];
        const String key = _cache_key(item);

        if (TreeItem* parent = item->get_parent()) {
            parent->remove_child(item);
        }
        _menu->_displayed.erase(key);
        _menu->_tree_cache.items.insert(key, item);

        if (Time::get_singleton()->get_ticks_usec() > until) {
            return false;
        }
    }

    _done = true;
    return true;
}

OrchestratorEditorActionMenu::Runner::Runner(
        OrchestratorEditorActionMenu* p_menu,
        const Vector<Ref<OrchestratorEditorActionDefinition>>& p_source,
        const String& p_query)
    : _menu(p_menu)
    , _source(p_source)
    , _query(p_query) {
    _context.context_sensitive = true;
    _context.query = _query;
    _context._filter_action_type = -1;
}

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
    _search_box->set_caret_column(text.length());
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
    _search_box->set_caret_column(text.length());
    _recents->deselect_all();
    _update_search();
}

void OrchestratorEditorActionMenu::_search_changed(const String& p_text) {
    _update_search();
}

void OrchestratorEditorActionMenu::_search_submitted(const String& p_text) {
    if (get_ok_button() && get_ok_button()->is_disabled()) {
        callable_mp_lambda(this, [this] {
            _search_box->release_focus();
            _search_box->grab_focus();
        }).call_deferred();
    }
}

void OrchestratorEditorActionMenu::_search_gui_input(const Ref<InputEvent>& p_event) {
    const Ref<InputEventKey> key = p_event;
    if (key.is_valid() && key->is_pressed()) {
        switch (key->get_keycode()) {
            case KEY_UP:
            case KEY_DOWN:
            case KEY_PAGEUP:
            case KEY_PAGEDOWN: {
                push_and_accept_event(p_event, _search_box, _results);
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

        // Stop and discard any in-flight search before tearing down the tree.
        set_process(false);
        if (_runner) {
            memdelete(_runner);
            _runner = nullptr;
        }

        // Reset dialog state
        _favorites->clear();
        _recents->clear();
        _search_box->clear();
        _results->clear();
        _favorite_button->set_pressed(false);

        // _results->clear() freed every attached item, so the displayed map now holds
        // dangling pointers; drop them (no free needed here).
        _displayed.clear();

        // Free items still pooled in the cache
        // These are detached from the tree, so _results->clear() called above does not reclaim them.
        _tree_cache.clear();
    } else {
        _load_user_data();

        #if GODOT_VERSION >= 0x040600
        callable_mp_cast(_search_box, Control, grab_focus).call_deferred(false);
        #else
        callable_mp_cast(_search_box, Control, grab_focus).call_deferred();
        #endif

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

void OrchestratorEditorActionMenu::_canceled() {
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

void OrchestratorEditorActionMenu::_update_search() {
    // When the dialog first opens, the action list is sorted in a background thread.
    // If this method is called for any reason before sorting concludes, we skip it.
    // The _perform_background_sort worker thread will call this function when the
    // sort has completed on the main thread.
    if (_sorting) {
        return;
    }

    const String query = _search_box->get_text();

    // Narrowing optimization: when the new query simply extends the previous (completed)
    // query, every result for the new query is necessarily a subset of the previous
    // results, so we only need to re-scan those rather than the full action set.
    const Vector<Ref<OrchestratorEditorActionDefinition>>& source =
        (!_last_query.is_empty() && query.begins_with(_last_query))
        ? _last_filtered_actions
        : _sorted_actions;

    // Replace any in-flight runner; this silently discards its remaining work. The
    // partially built tree it left behind is reclaimed into the cache by the new runner.
    if (_runner) {
        memdelete(_runner);
    }
    _runner = memnew(Runner(this, source, query));

    // Drive the runner from NOTIFICATION_PROCESS so no single frame blocks the UI.
    set_process(true);
}

void OrchestratorEditorActionMenu::_finish_search() {
    // Commit the completed snapshot so the next keystroke can narrow against it.
    _last_query = _runner->get_query();
    _last_filtered_actions = _runner->get_filtered();

    const String query = _last_query;
    TreeItem* best_match = _runner->get_best_match();

    if (!query.is_empty() && best_match) {
        _results->set_selected(best_match, 0);
        _results->scroll_to_item(best_match);

        _toggle_collapsed(false);
    } else {
        if (TreeItem* first_selectable = _find_first_selectable(_results->get_root())) {
            _results->set_selected(first_selectable, 0);
        }
        if (_results->get_root()) {
            _results->scroll_to_item(_results->get_root());
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
        Vector<Ref<OrchestratorEditorActionDefinition>> sorted;
        for (const Ref<OrchestratorEditorActionDefinition>& action : _actions) {
            sorted.push_back(action);
        }
        sorted.sort_custom<OrchestratorEditorActionDefinitionComparator>();

        _sorted_actions = sorted;

        _category_definitions.clear();
        _sorted_category_keys.clear();

        // First pass: collect categories from non-selectable category actions
        for (const Ref<OrchestratorEditorActionDefinition>& action : _sorted_actions) {
            if (!action->selectable && !action->category.is_empty()) {
                if (!_category_definitions.has(action->category)) {
                    _category_definitions[action->category] = action;
                    _sorted_category_keys.push_back(action->category);
                }
            }
        }

        // Second pass: inject categories for selectable actions whose category has no
        // corresponding non-selectable category action (e.g. "Constants", "Flow Control")
        for (const Ref<OrchestratorEditorActionDefinition>& action : _sorted_actions) {
            if (action->selectable && !action->category.is_empty()
                    && !_category_definitions.has(action->category)) {
                const Vector<Ref<OrchestratorEditorActionDefinition>> category_actions =
                    OrchestratorEditorIntrospector::generate_actions_from_category(action->category);
                for (const Ref<OrchestratorEditorActionDefinition>& cat : category_actions) {
                    if (!_category_definitions.has(cat->category)) {
                        _category_definitions[cat->category] = cat;
                        _sorted_category_keys.push_back(cat->category);
                    }
                }
            }
        }

        _sorted_category_keys.sort();

        _last_query = String();
        _last_filtered_actions.clear();

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

void OrchestratorEditorActionMenu::popup_centered(const OrchestratorEditorActionSet& p_actions,
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
                                         const OrchestratorEditorActionSet& p_actions,
                                         const Ref<OrchestratorEditorActionFilterEngine>& p_filter_engine) {
    _actions = p_actions;
    _filter_engine = p_filter_engine;

    // If the last size has no size, use the default
    if (_last_size.get_size() == Vector2()) {
        _last_size.set_size(_default_rect.get_size());
    }

    bool center_at_mouse = ORCHESTRATOR_GET("interface/editor/actions_menu/center_on_mouse", true);
    if (center_at_mouse) {
        _last_size.set_position(p_position - (_last_size.get_size() / 2.0));
    } else {
        _last_size.set_position(p_position);
    }

    EI->popup_dialog(this, _last_size);
}

void OrchestratorEditorActionMenu::_notification(int p_what) {
    if (p_what == NOTIFICATION_PROCESS) {
        if (!_runner) {
            set_process(false);
            return;
        }

        if (_runner->work(SEARCH_TIME_SLICE_USEC)) {
            _finish_search();
            set_process(false);
        }
    }
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
    // _search_box->connect(SceneStringName(text_submitted), callable_mp_this(_search_submitted));

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

    _help = memnew(OrchestratorEditorHelpBit);
    _help->set_content_help_limits(80 * EDSCALE, 80 * EDSCALE);
    SceneUtils::add_margin_child(vbox, "Description:", _help);

    register_text_enter(_search_box);
    set_hide_on_ok(false);

    connect("about_to_popup", callable_mp_this(_about_to_popup));
    connect(SceneStringName(visibility_changed), callable_mp_this(_visibility_changed));
    connect(SceneStringName(confirmed), callable_mp_this(_confirmed));
    connect(SceneStringName(canceled), callable_mp_this(_canceled));
    connect(SceneStringName(focus_exited), callable_mp_this(_focus_lost));

    // Attempt to use Orchestrator bounds, falling back to Godot
    _last_size = PROJECT_GET("Orchestrator", "action_menu_bounds", Rect2());
    if (_last_size == Rect2()) {
        _last_size = PROJECT_GET("dialog_bounds", "create_new_node", Rect2());
    }
}

OrchestratorEditorActionMenu::~OrchestratorEditorActionMenu() {
    if (_runner) {
        memdelete(_runner);
        _runner = nullptr;
    }

    // Any items still pooled in the cache are detached from the tree and must be freed
    // explicitly; the tree only owns the items currently attached to it.
    _tree_cache.clear();
}

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
#pragma once

#include "editor/actions/filter_engine.h"
#include "editor/doc/editor_help.h"

#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/item_list.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/classes/tree_item.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>

using namespace godot;

/// Displays the action dialog window that provides plug-in users the ability to search
/// and select actions to be performed by a listener.
class OrchestratorEditorActionMenu : public ConfirmationDialog {
    GDCLASS(OrchestratorEditorActionMenu, ConfirmationDialog);

    /// A pool of detached <code>TreeItem</code> instances, keyed by a stable identity
    /// string, that can be re-attached across searches instead of being destroyed and
    /// recreated. Avoids the redraw/icon-load churn of rebuilding the whole tree on
    /// every keystroke. Pooled items persist until the dialog is closed.
    struct TreeItemCache {
        HashMap<String, TreeItem*> items;
        void clear();
    };

    /// Time-sliced search runner.
    ///
    /// This mirrors <code>EditorHelpSearch::Runner</code>, a single keystroke instantiation
    /// of the runner and the menu drives from <code>NOTIFICATION_PROCESS</code>, doing bounded
    /// amounts of work per frame.
    ///
    /// Typing again simply replaces the runner, discarding any in-flight work. Because the action
    /// set is a flat, self-describing list (each action carries its own path), the runner needs
    /// only a single combined filter-and-build pass: categories are materialized on-demand as
    /// matching leaves are encountered, so no category is ever created only to be pruned.
    class Runner {
        const String CATEGORY = "C:";
        const String LEAF = "L:";

        OrchestratorEditorActionMenu* _menu = nullptr;
        Vector<Ref<OrchestratorEditorActionDefinition>> _source;  //! Snapshot iterated by this run
        String _query;                                            //! Search text for this run
        FilterContext _context;                                   //! Filter context for this run

        int _cursor = 0;                                          //! Next index into _source
        bool _initialized = false;                                //! Whether the tree root was resolved
        bool _build_done = false;                                 //! Whether the filter/build phase finished
        int _sweep_cursor = 0;                                    //! Next index into _sweep
        bool _done = false;                                       //! Whether the run has completed

        TreeItem* _root = nullptr;                                //! The (reused) tree root
        HashMap<String, TreeItem*> _category_items;               //! Categories resolved this run
        HashMap<String, TreeItem*> _last_child;                   //! Last sibling placed per parent path (for ordering)
        HashSet<String> _seen;                                    //! Keys kept/created this run
        Vector<TreeItem*> _sweep;                                 //! Stale items to detach, in safe removal order
        Vector<Ref<OrchestratorEditorActionDefinition>> _filtered;//! Everything that passed (for narrowing)

        TreeItem* _best_match = nullptr;                          //! Best scoring leaf so far
        float _best_score = -1;                                   //! Score of _best_match

        String _cache_key(TreeItem* p_item) const;
        TreeItem* _obtain(TreeItem* p_parent, const String& p_parent_path, const String& p_key, bool& r_created);
        TreeItem* _ensure_category(const String& p_path);
        void _add_leaf(const Ref<OrchestratorEditorActionDefinition>& p_leaf, float p_score);
        void _collect_sweep();

    public:
        /// Performs up to <code>p_time_slice_usec</code> microseconds of work.
        /// @return true when the search has completed, false if it should resume
        bool work(uint64_t p_time_slice_usec);

        const String& get_query() const { return _query; }
        TreeItem* get_best_match() const { return _best_match; }
        const Vector<Ref<OrchestratorEditorActionDefinition>>& get_filtered() const { return _filtered; }

        Runner(OrchestratorEditorActionMenu* p_menu,
               const Vector<Ref<OrchestratorEditorActionDefinition>>& p_source,
               const String& p_query);
    };

    Rect2 _default_rect = Rect2(0, 0, 900, 700);

    OrchestratorEditorHelpBit* _help = nullptr;
    LineEdit* _search_box = nullptr;
    ItemList* _favorites = nullptr;
    ItemList* _recents = nullptr;
    Tree* _results = nullptr;
    Button* _favorite_button = nullptr;
    Button* _collapse_button = nullptr;
    Button* _expand_button = nullptr;
    OptionButton* _filter_options = nullptr;
    String _suffix;
    Rect2 _last_size;
    bool _close_on_focus_lost;
    bool _start_collapsed;
    bool _sorting;

    HashMap<String, Ref<Texture2D>> _icon_cache;
    OrchestratorEditorActionSet _actions;
    Vector<Ref<OrchestratorEditorActionDefinition>> _sorted_actions;
    HashMap<String, Ref<OrchestratorEditorActionDefinition>> _category_definitions;
    PackedStringArray _sorted_category_keys;
    String _last_query;
    Vector<Ref<OrchestratorEditorActionDefinition>> _last_filtered_actions;
    Ref<OrchestratorEditorActionFilterEngine> _filter_engine;

    TreeItemCache _tree_cache;                                 //! Pool of detached, reusable items
    HashMap<String, TreeItem*> _displayed;                     //! Items currently attached to the tree, by key
    Runner* _runner = nullptr;

    bool _is_favorite(const Variant& p_value, int& r_index);
    void _favorite_selected(int p_index);
    void _favorite_activated(int p_index);
    void _recent_selected(int p_index);
    void _recent_activated(int p_index);
    void _search_changed(const String& p_text);
    void _search_submitted(const String& p_text);
    void _search_gui_input(const Ref<InputEvent>& p_event);
    void _item_selected();
    void _nothing_selected();
    void _toggle_favorite();
    void _add_recent(const Ref<OrchestratorEditorActionDefinition>& p_action);
    void _filter_changed(int p_index);
    void _about_to_popup();
    void _visibility_changed();
    void _focus_lost();
    void _confirmed();
    void _canceled();
    void _select_item(TreeItem* p_item, bool p_center_on_item);
    void _toggle_collapsed(bool p_collapsed);

    TreeItem* _find_first_selectable(TreeItem* p_item);
    void _update_search();
    void _finish_search();

    void _load_file_into_list(const String& p_filename, ItemList* p_list);
    void _save_list_into_file(ItemList* p_list, const String& p_filename, int64_t p_max = -1);

    void _load_user_data();
    void _save_user_data();

    String _get_cached_icon_name(const Ref<Texture2D>& p_texture);
    Ref<Texture2D> _get_cached_icon(const String& p_icon_name);

    void _perform_background_sort();

protected:
    static void _bind_methods();

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

public:

    void set_suffix(const String& p_suffix) { _suffix = p_suffix; }
    void set_close_on_focus_lost(bool p_close_on_focus_lost) { _close_on_focus_lost = p_close_on_focus_lost; }
    void set_show_filter_option(bool p_show_filter_option) { _filter_options->set_visible(p_show_filter_option); }
    void set_start_collapsed(bool p_start_collapsed);

    void popup(const Vector2& p_position, const OrchestratorEditorActionSet& p_actions,
               const Ref<OrchestratorEditorActionFilterEngine>& p_filter_engine);

    void popup_centered(const OrchestratorEditorActionSet& p_actions,
               const Ref<OrchestratorEditorActionFilterEngine>& p_filter_engine);

    OrchestratorEditorActionMenu();
    ~OrchestratorEditorActionMenu() override;
};

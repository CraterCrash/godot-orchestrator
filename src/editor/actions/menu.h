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
#ifndef ORCHESTRATOR_EDITOR_ACTIONS_MENU_H
#define ORCHESTRATOR_EDITOR_ACTIONS_MENU_H

#include "editor/actions/filter_engine.h"
#include "editor/actions/help.h"

#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/item_list.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/templates/hash_map.hpp>

using namespace godot;

/// Displays the action dialog window that provides plug-in users the ability to search
/// and select actions to be performed by a listener.
class OrchestratorEditorActionMenu : public ConfirmationDialog {
    GDCLASS(OrchestratorEditorActionMenu, ConfirmationDialog);

    Rect2 _default_rect = Rect2(0, 0, 900, 700);

    OrchestratorEditorActionHelp* _help = nullptr;
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
    Vector<Ref<OrchestratorEditorActionDefinition>> _actions;
    Ref<OrchestratorEditorActionFilterEngine> _filter_engine;

    bool _is_favorite(const Variant& p_value, int& r_index);
    void _favorite_selected(int p_index);
    void _favorite_activated(int p_index);
    void _recent_selected(int p_index);
    void _recent_activated(int p_index);
    void _search_changed(const String& p_text);
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
    void _select_item(TreeItem* p_item, bool p_center_on_item);
    void _toggle_collapsed(bool p_collapsed);

    TreeItem* _find_first_selectable(TreeItem* p_item);
    void _prune_empty_categories(TreeItem* p_item);
    void _update_search();

    void _load_file_into_list(const String& p_filename, ItemList* p_list);
    void _save_list_into_file(ItemList* p_list, const String& p_filename, int64_t p_max = -1);

    void _load_user_data();
    void _save_user_data();

    String _get_cached_icon_name(const Ref<Texture2D>& p_texture);
    Ref<Texture2D> _get_cached_icon(const String& p_icon_name);

    void _perform_background_sort();

protected:
    static void _bind_methods();

public:

    void set_suffix(const String& p_suffix) { _suffix = p_suffix; }
    void set_close_on_focus_lost(bool p_close_on_focus_lost) { _close_on_focus_lost = p_close_on_focus_lost; }
    void set_show_filter_option(bool p_show_filter_option) { _filter_options->set_visible(p_show_filter_option); }
    void set_start_collapsed(bool p_start_collapsed);

    void popup(const Vector2& p_position, const Vector<Ref<OrchestratorEditorActionDefinition>>& p_actions,
               const Ref<OrchestratorEditorActionFilterEngine>& p_filter_engine);

    void popup_centered(const Vector<Ref<OrchestratorEditorActionDefinition>>& p_actions,
               const Ref<OrchestratorEditorActionFilterEngine>& p_filter_engine);

    OrchestratorEditorActionMenu();
};

#endif // ORCHESTRATOR_EDITOR_ACTIONS_MENU_H
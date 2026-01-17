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
#ifndef ORCHESTRATOR_EDITOR_SEARCH_DIALOG_H
#define ORCHESTRATOR_EDITOR_SEARCH_DIALOG_H

#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/item_list.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>

using namespace godot;

/// Simple implementation of a RichTextLabel widget that can display help about selected items
class OrchestratorEditorSearchHelpBit : public MarginContainer {
    GDCLASS(OrchestratorEditorSearchHelpBit, MarginContainer);

    RichTextLabel* _help_bit = nullptr;
    String _text;

    void _add_text(const String& p_bbcode);
    void _meta_clicked();

protected:
    static void _bind_methods() { }

public:
    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

    void set_disabled(bool p_disabled);
    void set_text(const String& p_text);
};

/// Represents an item in the search dialog, which can be extended
class OrchestratorEditorSearchDialogItem : public RefCounted {
    GDCLASS(OrchestratorEditorSearchDialogItem, RefCounted);

protected:
    void static _bind_methods() { }

public:
    String path;                                     //! Defines the render path, allowing for nested hierarchies
    String name;                                     //! Logical name for the item
    String text;                                     //! Text shown for the item
    String script_filename;                          //! Name of the script that contributes type
    Ref<Texture2D> icon;                             //! The icon to be shown, if applicable
    bool selectable = true;                          //! Whether the item can be selected
    bool disabled = false;                           //! Whether the item is shaded as disabled
    bool collapsed = true;                           //! Whether the item should initially be collapsed
    Ref<OrchestratorEditorSearchDialogItem> parent;  //! The parent item, if applicable
};

/// Base class for Orchestrator's search dialogs
class OrchestratorEditorSearchDialog : public ConfirmationDialog {
    GDCLASS(OrchestratorEditorSearchDialog, ConfirmationDialog);

    //~ Begin Signal Handlers
    void _favorite_selected();
    void _favorite_activated();
    void _favorite_toggled();
    void _history_selected(int p_index);
    void _history_activated(int p_index);
    void _search_changed(const String& p_text);
    void _search_input(const Ref<InputEvent>& p_event);
    void _confirmed();
    void _canceled();
    void _item_selected();
    void _filter_selected(int p_index);
    //~ End Signal Handlers

protected:
    void static _bind_methods();

    typedef OrchestratorEditorSearchDialogItem SearchItem;

    struct FilterOption {
        int32_t id = -1;
        String text;
    };

    LineEdit* _search_box = nullptr;                        //! The user search box
    ItemList* _recent = nullptr;                            //! List of recently used items
    Tree* _favorites = nullptr;                             //! List of favorite items
    Tree* _search_options = nullptr;                        //! List of search results
    Button* _favorite = nullptr;                            //! Favorite toggle button
    OptionButton* _filters = nullptr;                       //! Filters
    Vector<Ref<SearchItem>> _favorite_list;                 //! List of favorite items
    Vector<Ref<SearchItem>> _search_items;                  //! List of searchable items
    Vector<FilterOption> _filter;                           //! List of filter options
    HashMap<String, TreeItem*> _search_options_hierarchy;   //! Hierarchy of created search items
    OrchestratorEditorSearchHelpBit* _help_bit = nullptr;   //! Reference to the help bit

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

    void _update_themes();

    /// Checks whether any favorite item has a matching meta descriptor.
    /// @param p_item the meta descriptor to check
    /// @return true if a favorite is found, false otherwise
    bool _is_favorite(const Ref<SearchItem>& p_item) const;

    /// Updates the search results based on the current dialog state
    void _update_search();

    /// Get the search item by name
    /// @param p_name the name
    /// @return the search item reference, invalid reference if not found
    Ref<SearchItem> _get_search_item_by_name(const String& p_name) const;

    /// Reads contents of the file line by line
    /// @param p_file_name the file name
    /// @return an array of recent items
    PackedStringArray _read_file_lines(const String& p_file_name) const;

    /// Writes the array of items line-by-line to the given file
    /// @param p_file_name the file name
    /// @param p_values the values to write
    void _write_file_lines(const String& p_file_name, const PackedStringArray& p_values);

    /// Get all filters
    /// @return the possible filters
    virtual Vector<FilterOption> _get_filters() const { return {}; }

    /// Get the default filter choice, defaults to first item
    /// @return the default filter choice
    virtual int _get_default_filter() const { return 0; }

    /// Called when the user changes the filter type
    /// @param p_index the selected item index
    virtual void _filter_type_changed(int p_index) { }

    /// Check whether the search item is to be filtered beyond the normal score calculation.
    /// @param p_item the search item
    /// @param p_text the search text
    /// @return true if the search item is to be filtered, false otherwse
    virtual bool _is_filtered(const Ref<SearchItem>& p_item, const String& p_text) const { return false; }

    /// Obtain a list of all searchable items.
    /// We use referenced items so that we can place these in the TreeItem as metadata.
    /// @return the searchable items this dialog will use.
    virtual Vector<Ref<SearchItem>> _get_search_items() { return {}; }

    /// Populates the search results
    /// @return the selectable item's descriptor
    virtual TreeItem* _populate_search_results();

    /// Updates the state of the search box
    /// @param p_clear whether to clear the search box contents
    /// @param p_replace whether to replace the search box contente with <code>p_text</code>
    /// @param p_text the text to replace, if provided
    /// @param p_focus whether to focus to the search box
    virtual void _update_search_box(bool p_clear, bool p_replace, const String& p_text, bool p_focus);

    /// Whether the dialog should collapse nodes on empty search terms
    /// @returns whether to collapse non-root tree nodes when search box is empty, defaults to false.
    virtual bool _should_collapse_on_empty_search() const { return false; }

    /// Sets the search item's collapse state
    /// @param p_item the tree item
    virtual void _set_search_item_collapse_state(TreeItem* p_item);

    /// Determines whether the specified item should be collapsed.
    /// @param p_item the tree item
    /// @return true if the item should be collapsed, otherwise false
    virtual bool _get_search_item_collapse_suggestion(TreeItem* p_item) const { return true; }

    /// Load the dialog window's favorites and recent history
    virtual void _load_favorites_and_history();

    /// Saves the dialog window's favorites and recent history
    virtual void _save_and_update_favorites_list();

    /// Gets the list of recent items to display for this dialog
    /// @return the recent items list, the default is an empty list
    virtual Vector<Ref<SearchItem>> _get_recent_items() const { return {}; }

    /// Gets the list of favorite items to display for this dialog
    /// @return the favorite items list, the default is an empty list
    virtual Vector<Ref<SearchItem>> _get_favorite_items() const { return {}; }

    /// Saves the list of recent items, the default behavior does not save anything
    /// @param p_recents the list of recent items to save
    virtual void _save_recent_items(const Vector<Ref<SearchItem>>& p_recents) { }

    /// Saves the list of favorites, the default behavior does not save anything
    /// @param p_favorites the list of favorites to save
    virtual void _save_favorite_items(const Vector<Ref<SearchItem>>& p_favorites) { }

    /// Determines whether the specified item is considered preferred in the score algorithm.
    /// By default, no item has preference.
    /// @return true if the item is preferred, otherwise false
    virtual bool _is_preferred(const String& p_item) const { return false; }

    /// Calculates the score of the specified tree item against the search string.
    /// @param p_item the tree item
    /// @param p_search the search text
    /// @return the score for the item based on the search text
    virtual float _calculate_score(const Ref<SearchItem>& p_item, const String& p_search) const;

    /// Performs clean-up tasks when the dialog is closed
    virtual void _cleanup();

    /// Update the help text for the specified item
    /// @param p_item the currently selected item
    virtual void _update_help(const Ref<SearchItem>& p_item) { }

    /// Selects the specified item, optionally centering the view on the item.
    /// @param p_item the item to select
    /// @param p_center_on_item whether to center the view on the item
    void _select_item(TreeItem* p_item, bool p_center_on_item);

public:
    /// Opens the dialog
    /// @param p_dont_clear whether the dialog should clear the search field
    /// @param p_replace_mode whether the dialog should replace the current type
    /// @param p_current_type the current type
    /// @param p_current_name the current name
    virtual void popup_create(bool p_dont_clear, bool p_replace_mode = false, const String& p_current_type = "", const String& p_current_name = "");
};

#endif  // ORCHESTRATOR_EDITOR_SEARCH_DIALOG_H
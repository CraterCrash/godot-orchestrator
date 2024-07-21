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
#ifndef ORCHESTRATOR_EDITOR_SELECT_TYPE_DIALOG_H
#define ORCHESTRATOR_EDITOR_SELECT_TYPE_DIALOG_H

#include "editor/search/search_dialog.h"

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>

/// Specialization of the OrchestratorEditorSearchDialog specifically for type selection
class OrchestratorSelectTypeSearchDialog : public OrchestratorEditorSearchDialog
{
    GDCLASS(OrchestratorSelectTypeSearchDialog, OrchestratorEditorSearchDialog);
    void static _bind_methods() { }

    enum FilterType
    {
        FT_ALL_TYPES = 1,
        FT_BASIC_TYPES = 2,
        FT_BITFIELDS = 3,
        FT_ENUMERATIONS = 4,
        FT_NODES = 5,
        FT_OBJECTS = 6,
        FT_RESOURCES = 7
    };

    /// Forward declaration
    struct SearchItemSortPath;

    HashSet<StringName> _exclusions;                  //! Set of types to be excluded
    Vector<String> _variant_type_names;               //! All valid variant type names
    bool _is_base_type_node{ false };                 //! Specifies if base type is a Node
    String _base_type;                                //! The base type
    String _fallback_icon{ "Object" };                //! The fallback icon
    String _preferred_search_result_type;             //! The preferred search result type
    String _data_suffix;                              //! Specifies the data suffix for history/favorite tracking
    String _title;                                    //! The desired title

protected:
    //~ Begin OrchestratorEditorSearchDialog Interface
    bool _is_preferred(const String& p_type) const override;
    bool _get_search_item_collapse_suggestion(TreeItem* p_item) const override;
    void _update_help(const Ref<SearchItem>& p_item) override;
    Vector<Ref<SearchItem>> _get_search_items() override;
    Vector<Ref<SearchItem>> _get_recent_items() const override;
    Vector<Ref<SearchItem>> _get_favorite_items() const override;
    void _save_recent_items(const Vector<Ref<SearchItem>>& p_recents) override;
    void _save_favorite_items(const Vector<Ref<SearchItem>>& p_favorites) override;
    Vector<FilterOption> _get_filters() const override;
    bool _is_filtered(const Ref<SearchItem>& p_item, const String& p_text) const override;
    int _get_default_filter() const override;
    void _filter_type_changed(int p_index) override;
    //~ End OrchestratorEditorSearchDialog Interface

    /// Creates the class hierarchy path, i.e. "Parent/Child/GrandChild"
    /// @param p_class the class
    /// @return the path
    String _create_class_hierarchy_path(const String& p_class);

    /// Gets the class hierarchy for the specified class.
    /// The results are ordered from eldest ancestor to the given class.
    /// @param p_class the class to get the hierarchy for
    /// @return the class hierarchy
    PackedStringArray _get_class_hierarchy(const String& p_class);

    /// Get the class hierarchy search items
    /// @param p_class the class
    /// @param r_cache the search item cache
    /// @param p_root the root search item
    Vector<Ref<SearchItem>> _get_class_hierarchy_search_items(const String& p_class, HashMap<String, Ref<SearchItem>>& r_cache, const Ref<SearchItem>& p_root);

public:
    //~ Begin OrchestratorEditorSearchDialog Interface
    void popup_create(bool p_dont_clear, bool p_replace_mode, const String& p_current_type, const String& p_current_name) override;
    //~ End OrchestratorEditorSearchDialog Interface

    /// Get the selected type from the dialog
    /// @return the selected type
    String get_selected_type() const;

    /// Sets the base type for the objects in the search dialog
    /// @param p_base_type
    void set_base_type(const String& p_base_type);

    /// Sets the data suffix for history and favorite tracking
    /// @param p_data_suffix the data file suffix
    void set_data_suffix(const String& p_data_suffix) { _data_suffix = p_data_suffix; }

    /// Sets the dialog's title
    /// @param p_title the title to be used
    void set_popup_title(const String& p_title) { _title = p_title; }
};

#endif  // ORCHESTRATOR_EDITOR_SELECT_TYPE_DIALOG_H
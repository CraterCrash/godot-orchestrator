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
#ifndef ORCHESTRATOR_EDITOR_SELECT_CLASS_DIALOG_H
#define ORCHESTRATOR_EDITOR_SELECT_CLASS_DIALOG_H

#include "editor/gui/search_dialog.h"

/// A specialization of Godot's <code>CreateDialog</code> specifically for displaying class types
/// that can be selected by the user, typically paired with <code>OrchestratorEditorPropertyClassName</code>
class OrchestratorSelectClassSearchDialog : public OrchestratorEditorSearchDialog {
    GDCLASS(OrchestratorSelectClassSearchDialog, OrchestratorEditorSearchDialog);

    /// Forward declaration
    struct SearchItemSortPath;

    bool _is_base_type_node = false;
    bool _allow_abstract_types = false;
    String _base_type;
    String _fallback_icon = "Object";
    String _preferred_search_result_type;
    String _data_suffix;
    String _title;

    /// Creates the class hierarchy path, i.e. "Parent/Child/GrandChild"
    /// @param p_class the class name
    /// @return the path
    String _create_class_hierarchy_path(const String& p_class);

    /// Get the class hierarchy for the specified class name as an array
    /// The results are ordered from the eldest ancestor to the given class.
    /// @param p_class the class name
    /// @return the class hierarchy as an array
    PackedStringArray _get_class_hierarchy(const String& p_class);

    /// Get the class hierarchy search items
    /// @param p_class the class name
    /// @param r_cache the seearch item cache
    /// @param p_root the root search item
    Vector<Ref<SearchItem>> _get_class_hierarchy_search_items(const String& p_class, HashMap<String, Ref<SearchItem>>& r_cache, const Ref<SearchItem>& p_root);

protected:
    static void _bind_methods() { }

    //~ Begin OrchestratorEditorSearchDialog Interface
    bool _is_preferred(const String& p_item) const override;
    bool _should_collapse_on_empty_search() const override;
    bool _get_search_item_collapse_suggestion(TreeItem* p_item) const override;
    void _update_help(const Ref<SearchItem>& p_item) override;
    Vector<Ref<SearchItem>> _get_search_items() override;
    Vector<Ref<SearchItem>> _get_recent_items() const override;
    Vector<Ref<SearchItem>> _get_favorite_items() const override;
    void _save_recent_items(const Vector<Ref<SearchItem>>& p_recents) override;
    void _save_favorite_items(const Vector<Ref<SearchItem>>& p_favorites) override;
    //~ End OrchestratorEditorSearchDialog Interface

public:
    //~ Begin OrchestratorEditorSearchDialog Interface
    void popup_create(bool p_dont_clear, bool p_replace_mode, const String& p_current_type, const String& p_current_name) override;
    //~ End OrchestratorEditorSearchDialog Interface

    /// Get the selected value
    /// @return the selected value
    String get_selected() const;

    /// Set the base type for objects in the search dialog
    /// @param p_base_type the base type to show hierarchies based upon
    void set_base_type(const String& p_base_type);

    /// Sets the data suffix for tracking recent history and favorites
    /// @param p_data_suffix the data file name suffix
    void set_data_suffix(const String& p_data_suffix);

    /// Sets whether user can select abstract types
    /// @param p_allow_abstracts whether to allow selection of abstract classes
    void set_allow_abstract_types(bool p_allow_abstracts);

    /// Sets the popup dialog title
    /// @param p_title the title to display
    void set_popup_title(const String& p_title);

};

#endif // ORCHESTRATOR_EDITOR_SELECT_CLASS_DIALOG_H
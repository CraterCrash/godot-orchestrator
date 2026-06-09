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

#include "editor/gui/search_dialog.h"

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>

/// Specialization of the OrchestratorEditorSearchDialog specifically for type selection
class OrchestratorSelectTypeSearchDialog : public OrchestratorEditorSearchDialog {
    GDCLASS(OrchestratorSelectTypeSearchDialog, OrchestratorEditorSearchDialog);

    enum FilterType {
        FT_ALL_TYPES = 1,
        FT_BASIC_TYPES = 2,
        FT_BITFIELDS = 3,
        FT_ENUMERATIONS = 4,
        FT_NODES = 5,
        FT_OBJECTS = 6,
        FT_RESOURCES = 7
    };

    HashSet<StringName> _exclusions;                  //! Set of types to be excluded
    bool _is_base_type_node = false;                  //! Specifies if base type is a Node
    bool _allow_abstract_types = false;               //! Allow selecting abstract types
    String _base_type;                                //! The base type
    String _fallback_icon = "Object";                 //! The fallback icon
    String _preferred_search_result_type;             //! The preferred search result type
    String _data_suffix;                              //! Specifies the data suffix for history/favorite tracking
    String _title;                                    //! The desired title

    PropertyInfo _string_to_property(const String& p_value) const;
    String _property_to_string(const PropertyInfo& p_property) const;
    String _decode_property_line(const String& p_value) const;

    String _get_icon_name(const String& p_name, const String& p_fallback = String());

    String _create_class_hierarchy_path(const String& p_class);
    PackedStringArray _get_class_hierarchy(const String& p_class);

    void _build_basic_type_items(Vector<Ref<SearchItem>>& r_items, const Ref<SearchItem>& p_parent);
    void _build_object_type_items(Vector<Ref<SearchItem>>& r_items, const Ref<SearchItem>& p_parent);
    void _build_class_children(const String& p_class_name, const Ref<SearchItem>& p_parent, const HashMap<String, PackedStringArray>& p_children_by_parent, Vector<Ref<SearchItem>>& r_items);
    void _build_global_enumeration_items(Vector<Ref<SearchItem>>& r_items, Ref<SearchItem>& r_parent, const Ref<SearchItem>& p_root);
    void _build_global_bitfield_items(Vector<Ref<SearchItem>>& r_items, Ref<SearchItem>& r_parent, const Ref<SearchItem>& p_root);

    static Ref<SearchItem> _make_item(
        const String& p_name,
        const String& p_text,
        const String& p_path,
        const Ref<SearchItem>& p_parent,
        const PropertyInfo&& p_property,
        const String& p_icon = String(),
        bool p_selectable = true);

    Ref<SearchItem> _get_search_item_by_property(const PropertyInfo& p_property) const;

protected:
    void static _bind_methods();

    //~ Begin OrchestratorEditorSearchDialog Interface
    bool _is_preferred(const String& p_type) const override;
    bool _should_collapse_on_empty_search() const override;
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

public:
    //~ Begin OrchestratorEditorSearchDialog Interface
    void popup_create(bool p_dont_clear, bool p_replace_mode, const String& p_current_type, const String& p_current_name) override;
    //~ End OrchestratorEditorSearchDialog Interface

    PropertyInfo get_selected() const;

    void set_base_type(const String& p_base_type);
    String get_base_type() const { return _base_type; }
    void set_data_suffix(const String& p_data_suffix) { _data_suffix = p_data_suffix; }
    String get_data_suffix() const { return _data_suffix; }
    void set_allow_abstract_types(bool p_allow_abstract_types) { _allow_abstract_types = p_allow_abstract_types; }
    bool is_allow_abstract_types() const { return _allow_abstract_types; }
    void set_popup_title(const String& p_title) { _title = p_title; }
    String get_popup_title() const { return _title; }
    void set_exclusions(const HashSet<StringName>& p_exclusions) { _exclusions = p_exclusions; }
    HashSet<StringName> get_exclusions() const { return _exclusions; }
};

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

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/templates/hash_map.hpp>

using namespace godot;

/// Forward declarations
class OrchestratorSelectTypeSearchDialog;

/// A reusable component for selecting types, including collection-based types.
class OrchestratorEditorTypeSelector : public HBoxContainer {
    GDCLASS(OrchestratorEditorTypeSelector, HBoxContainer);

    enum ContainerShape {
        NONE,
        ARRAY,
        DICTIONARY
    };

    struct DictionaryHintParts {
        String key;
        String value;

        static DictionaryHintParts parse(const String& p_hint_string);
    };

    Button* _left_type = nullptr;
    Button* _right_type = nullptr;
    OptionButton* _container_type = nullptr;
    HashMap<StringName, Variant::Type> _variant_named_types;
    PackedStringArray _user_exclusions;
    String _cache_name;
    bool _allow_abstract_types;
    PropertyInfo _property;

    static void _normalize_inbound(PropertyInfo& r_property);
    static void _normalize_outbound(PropertyInfo& r_property);

    bool _get_variant_name_to_type(const String& p_name, Variant::Type& r_type) const;

    String _pack_hint_string(const PropertyInfo& p_property);
    PropertyInfo _unpack_hint_string(const String& p_hint_string);

    ContainerShape _get_container_shape(const PropertyInfo& p_property) const;
    void _container_type_changed(int p_index);

    void _left_type_pressed();
    void _left_type_selected(OrchestratorSelectTypeSearchDialog* p_dialog);
    void _right_type_pressed();
    void _right_type_selected(OrchestratorSelectTypeSearchDialog* p_dialog);

    void _open_type_dialog(const String& p_title, const Callable& p_select_callback);
    void _type_dialog_closed(OrchestratorSelectTypeSearchDialog* p_dialog);

    void _emit_property_changed();
    void _update();

protected:
    static void _bind_methods();

public:

    void set_property(const PropertyInfo& p_property);
    PropertyInfo get_property() const;

    /// Set the read only mode
    /// @param p_read_only true if widget is in read only mode, false otherwise
    void set_read_only(bool p_read_only);
    void set_read_only(bool p_left_read_only, bool p_right_read_only);

    /// Set up the widget
    /// @param p_cache_suffix the cache file suffix, which stores favorites and/or recently used
    /// @param p_allow_abstract_types whether to show abstract types for selection
    /// @param p_exclusions what types to be excluded from selection
    void setup(const String& p_cache_suffix,
        bool p_allow_abstract_types = false,
        const PackedStringArray& p_exclusions = PackedStringArray());

    OrchestratorEditorTypeSelector();
};
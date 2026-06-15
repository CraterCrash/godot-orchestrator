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
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/timer.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/templates/hash_map.hpp>

using namespace godot;

/// Forward declarations
class OrchestratorEditorInputEventListenerLineEdit;
class OrchestratorEditorInputEventConfigurationDialog;

/// A copy of the editor's <code>EventSearchBar</code> class, that allows searching for shortcuts by
/// name or by their input binding.
class OrchestratorEditorEventSearchBar : public HBoxContainer {
    GDCLASS(OrchestratorEditorEventSearchBar, HBoxContainer);

    LineEdit* _search_by_name = nullptr;
    OrchestratorEditorInputEventListenerLineEdit* _search_by_event = nullptr;
    Button* _clear_all = nullptr;

    void _event_changed(const Ref<InputEvent>& p_event);
    void _clear();
    void _value_changed();

protected:
    static void _bind_methods();

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

public:
    LineEdit* get_name_search_box() const { return _search_by_name; }
    bool is_searching() const;

    String get_name() const;
    Ref<InputEvent> get_event() const;

    OrchestratorEditorEventSearchBar();
};

/// Class that represents the "Shortcuts" tab view in the <code>OrchestratorEditorSettingsDialog</code>.
class OrchestratorEditorSettingsShortcutsTab : public VBoxContainer {
    GDCLASS(OrchestratorEditorSettingsShortcutsTab, VBoxContainer);

    enum ShortcutButton {
        SHORTCUT_ADD,
        SHORTCUT_EDIT,
        SHORTCUT_ERASE,
        SHORTCUT_REVERT
    };

    OrchestratorEditorEventSearchBar* _shortcuts_search_bar = nullptr;
    OrchestratorEditorInputEventConfigurationDialog* _shortcut_editor = nullptr;
    Tree* _shortcuts = nullptr;
    Timer* _timer = nullptr;

    bool _is_editing_action = false;
    String _current_edited_identifier;
    Array _current_events;
    int _current_event_index = -1;

    //~ Begin Signal Handlers
    void _settings_changed();
    void _settings_save();

    void _shortcut_button_pressed(TreeItem* p_item, int p_column, int p_idx, int p_button = MOUSE_BUTTON_LEFT);
    void _shortcut_cell_double_clicked();

    void _event_config_confirmed();
    //~ End Signal Handlers

    //~ Begin Drag Forwarding
    Variant get_drag_data_fw(const Point2& p_point, Control* p_from);
    bool can_drop_data_fw(const Point2& p_point, const Variant& p_data, Control* p_from) const;
    void drop_data_fw(const Point2& p_point, const Variant& p_data, Control* p_from);
    //~ End Drag Forwarding

    bool _should_display_shortcut(const String& p_path, const Array& p_events, const String& p_name) const;
    TreeItem* _create_shortcut_item(TreeItem* p_parent, const String& p_identifier, const String& p_display, Array& p_events, bool p_allow_revert, bool p_is_action, bool p_is_collapsed);

    // Returns the TreeItem for a (possibly nested) section path, creating it and any missing
    // ancestors. Memoized in r_sections, keyed by the full path.
    TreeItem* _ensure_section(const String& p_section_path, HashMap<String, TreeItem*>& r_sections, TreeItem* p_root, const HashMap<String, bool>& p_collapsed);

    void _update_shortcuts();
    void _update_builtin_action(const String& p_path, const Array& p_events);
    void _update_shortcut_events(const String& p_path, const Array& p_events);

protected:
    static void _bind_methods();

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

public:
    OrchestratorEditorSettingsShortcutsTab();
};
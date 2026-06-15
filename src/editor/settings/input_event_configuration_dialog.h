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

#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/input_event_with_modifiers.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

using namespace godot;

enum OrchestratorEditorInputType {
    INPUT_KEY = 1,
    INPUT_MOUSE_BUTTON = 2,
    INPUT_JOY_BUTTON = 4,
    INPUT_JOY_MOTION = 8,
    INPUT_ALL = INPUT_KEY | INPUT_MOUSE_BUTTON | INPUT_JOY_BUTTON | INPUT_JOY_MOTION
};

/// Class that mimics the editor's <code>EditorEventListenerLineEdit</code>
class OrchestratorEditorInputEventListenerLineEdit : public LineEdit {
    GDCLASS(OrchestratorEditorInputEventListenerLineEdit, LineEdit);

public:
    // Taken from InputMap
    static constexpr int ALL_DEVICES = -1;

private:
    uint64_t _hold_next = 0;
    Ref<InputEvent> _hold_event;

    int _allowed_input_types = INPUT_ALL;
    bool _ignore_next_event = true;
    Ref<InputEvent> _event;

    static String _get_modifiers_text(const Ref<InputEventWithModifiers>& p_event);

    bool _is_over_clear_button(const Vector2& p_position);

    bool _is_event_allowed(const Ref<InputEvent>& p_event) const;
    void _text_changed(const String& p_text);

protected:
    static void _bind_methods();

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

public:
    //~ Begin Control Interface
    void _gui_input(const Ref<InputEvent>& p_event) override;
    //~ End Control Interface

    static String get_event_text(const Ref<InputEvent>& p_event, bool p_include_device);
    static String get_device_string(int p_device);

    Ref<InputEvent> get_event() const;
    void clear_event();

    void set_allowed_input_types(int p_type_masks);
    int get_allowed_input_types() const;

    void grab_focus();

    OrchestratorEditorInputEventListenerLineEdit();
};

/// Class that mimics the editor's <code>InputEventConfigurationDialog</code> class.
class OrchestratorEditorInputEventConfigurationDialog : public ConfirmationDialog {
    GDCLASS(OrchestratorEditorInputEventConfigurationDialog, ConfirmationDialog);

    struct IconCache {
        Ref<Texture2D> keyboard;
        Ref<Texture2D> mouse;
        Ref<Texture2D> joypad_button;
        Ref<Texture2D> joypad_axis;
    } icon_cache;

    Ref<InputEvent> _event;
    Ref<InputEvent> _original_event;
    Array _action_events;

    bool _in_tree_update = false;

    OrchestratorEditorInputEventListenerLineEdit* _event_listener = nullptr;
    Label* _event_as_text = nullptr;

    int _allowed_input_types;
    Tree* _input_list_tree = nullptr;
    LineEdit* _input_list_search = nullptr;

    VBoxContainer* _additional_options_container = nullptr;
    HBoxContainer* _device_container = nullptr;
    OptionButton* _device_id_option = nullptr;

    // Contains the subcontainer and the store command checkbox
    HBoxContainer* _mod_container = nullptr;

    enum ModCheckbox {
        MOD_ALT,
        MOD_SHIFT,
        MOD_CTRL,
        MOD_META,
        MOD_MAX
    };

    #ifdef MACOS_ENABLED
    String _mods[MOD_MAX] = { "Option", "Shift", "Ctrl", "Command" };
    #elifdef WINDOWS_ENABLED
    String _mods[MOD_MAX] = { "Alt", "Shift", "Ctrl", "Windows" };
    #else
    String _mods[MOD_MAX] = { "Alt", "Shift", "Ctrl", "Meta" };
    #endif

    String _mods_tip[MOD_MAX] = {
        "Alt or Option key",
        "Shift key",
        "Control key",
        "Meta/Windows or Command key",
    };

    CheckBox* _mod_checkboxes[MOD_MAX] = {};
    CheckBox* _autoremap_command_or_control_checkbox = nullptr;

    enum KeyMode {
        KEYMODE_KEYCODE,
        KEYMODE_PHY_KEYCODE,
        KEYMODE_UNICODE
    };

    OptionButton* _key_mode = nullptr;

    HBoxContainer* _location_container = nullptr;
    OptionButton* _key_location = nullptr;

    Label* _event_exists = nullptr;

    void _set_event(const Ref<InputEvent>& p_event, const Ref<InputEvent>& p_original_event, bool p_update_input_list_selection = true);
    void _listen_input_changed(const Ref<InputEvent>& p_event);

    void _search_term_updated(const String& p_term);
    void _update_input_list();
    void _input_list_item_activated();
    void _input_list_item_selected();

    void _mod_toggled(bool p_checked, int p_index);
    void _auto_remap_command_or_control_toggled(bool p_checked);
    void _key_mode_selected(int p_mode);
    void _key_location_selected(int p_location);

    void _device_selection_changed(int p_option_button_index);
    void _set_current_device(int p_device);
    int _get_current_device() const;

protected:
    static void _bind_methods();

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

public:
    void popup_and_configure(const Ref<InputEvent>& p_event = Ref<InputEvent>(), const String& p_current_action_name = "", const Dictionary& p_current_action = Dictionary());
    Ref<InputEvent> get_event() const;

    void set_allowed_input_types(int p_type_masks);

    OrchestratorEditorInputEventConfigurationDialog();
};
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
#include "editor/settings/input_event_configuration_dialog.h"

#include "common/macros.h"
#include "common/os_utils.h"
#include "common/scene_utils.h"
#include "core/godot/scene_string_names.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_joypad_button.hpp>
#include <godot_cpp/classes/input_event_joypad_motion.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/classes/v_separator.hpp>

struct KeyCodeText {
	Key code;
	const char *text;
};

static const KeyCodeText keycodes[] = {
	/* clang-format off */
	{KEY_ESCAPE                ,"Escape"},
	{KEY_TAB                   ,"Tab"},
	{KEY_BACKTAB               ,"Backtab"},
	{KEY_BACKSPACE             ,"Backspace"},
	{KEY_ENTER                 ,"Enter"},
	{KEY_KP_ENTER              ,"Kp Enter"},
	{KEY_INSERT                ,"Insert"},
	{KEY_DELETE                ,"Delete"},
	{KEY_PAUSE                 ,"Pause"},
	{KEY_PRINT                 ,"Print"},
	{KEY_SYSREQ                ,"SysReq"},
	{KEY_CLEAR                 ,"Clear"},
	{KEY_HOME                  ,"Home"},
	{KEY_END                   ,"End"},
	{KEY_LEFT                  ,"Left"},
	{KEY_UP                    ,"Up"},
	{KEY_RIGHT                 ,"Right"},
	{KEY_DOWN                  ,"Down"},
	{KEY_PAGEUP                ,"PageUp"},
	{KEY_PAGEDOWN              ,"PageDown"},
	{KEY_SHIFT                 ,"Shift"},
	{KEY_CTRL                  ,"Ctrl"},
#if defined(MACOS_ENABLED)
	{KEY_META                  ,"Command"},
	{KEY_CTRL                  ,"Command"},
	{KEY_ALT                   ,"Option"},
#elif defined(WINDOWS_ENABLED)
	{KEY_META                  ,"Windows"},
	{KEY_CTRL                  ,"Ctrl"},
	{KEY_ALT                   ,"Alt"},
#else
	{KEY_META                  ,"Meta"},
	{KEY_CTRL                  ,"Ctrl"},
	{KEY_ALT                   ,"Alt"},
#endif
	{KEY_CAPSLOCK              ,"CapsLock"},
	{KEY_NUMLOCK               ,"NumLock"},
	{KEY_SCROLLLOCK            ,"ScrollLock"},
	{KEY_F1                    ,"F1"},
	{KEY_F2                    ,"F2"},
	{KEY_F3                    ,"F3"},
	{KEY_F4                    ,"F4"},
	{KEY_F5                    ,"F5"},
	{KEY_F6                    ,"F6"},
	{KEY_F7                    ,"F7"},
	{KEY_F8                    ,"F8"},
	{KEY_F9                    ,"F9"},
	{KEY_F10                   ,"F10"},
	{KEY_F11                   ,"F11"},
	{KEY_F12                   ,"F12"},
	{KEY_F13                   ,"F13"},
	{KEY_F14                   ,"F14"},
	{KEY_F15                   ,"F15"},
	{KEY_F16                   ,"F16"},
	{KEY_F17                   ,"F17"},
	{KEY_F18                   ,"F18"},
	{KEY_F19                   ,"F19"},
	{KEY_F20                   ,"F20"},
	{KEY_F21                   ,"F21"},
	{KEY_F22                   ,"F22"},
	{KEY_F23                   ,"F23"},
	{KEY_F24                   ,"F24"},
	{KEY_F25                   ,"F25"},
	{KEY_F26                   ,"F26"},
	{KEY_F27                   ,"F27"},
	{KEY_F28                   ,"F28"},
	{KEY_F29                   ,"F29"},
	{KEY_F30                   ,"F30"},
	{KEY_F31                   ,"F31"},
	{KEY_F32                   ,"F32"},
	{KEY_F33                   ,"F33"},
	{KEY_F34                   ,"F34"},
	{KEY_F35                   ,"F35"},
	{KEY_KP_MULTIPLY           ,"Kp Multiply"},
	{KEY_KP_DIVIDE             ,"Kp Divide"},
	{KEY_KP_SUBTRACT           ,"Kp Subtract"},
	{KEY_KP_PERIOD             ,"Kp Period"},
	{KEY_KP_ADD                ,"Kp Add"},
	{KEY_KP_0                  ,"Kp 0"},
	{KEY_KP_1                  ,"Kp 1"},
	{KEY_KP_2                  ,"Kp 2"},
	{KEY_KP_3                  ,"Kp 3"},
	{KEY_KP_4                  ,"Kp 4"},
	{KEY_KP_5                  ,"Kp 5"},
	{KEY_KP_6                  ,"Kp 6"},
	{KEY_KP_7                  ,"Kp 7"},
	{KEY_KP_8                  ,"Kp 8"},
	{KEY_KP_9                  ,"Kp 9"},
	{KEY_MENU                  ,"Menu"},
	{KEY_HYPER                 ,"Hyper"},
	{KEY_HELP                  ,"Help"},
	{KEY_BACK                  ,"Back"},
	{KEY_FORWARD               ,"Forward"},
	{KEY_STOP                  ,"Stop"},
	{KEY_REFRESH               ,"Refresh"},
	{KEY_VOLUMEDOWN            ,"VolumeDown"},
	{KEY_VOLUMEMUTE            ,"VolumeMute"},
	{KEY_VOLUMEUP              ,"VolumeUp"},
	{KEY_MEDIAPLAY             ,"MediaPlay"},
	{KEY_MEDIASTOP             ,"MediaStop"},
	{KEY_MEDIAPREVIOUS         ,"MediaPrevious"},
	{KEY_MEDIANEXT             ,"MediaNext"},
	{KEY_MEDIARECORD           ,"MediaRecord"},
	{KEY_HOMEPAGE              ,"HomePage"},
	{KEY_FAVORITES             ,"Favorites"},
	{KEY_SEARCH                ,"Search"},
	{KEY_STANDBY               ,"StandBy"},
	{KEY_OPENURL               ,"OpenURL"},
	{KEY_LAUNCHMAIL            ,"LaunchMail"},
	{KEY_LAUNCHMEDIA           ,"LaunchMedia"},
	{KEY_LAUNCH0               ,"Launch0"},
	{KEY_LAUNCH1               ,"Launch1"},
	{KEY_LAUNCH2               ,"Launch2"},
	{KEY_LAUNCH3               ,"Launch3"},
	{KEY_LAUNCH4               ,"Launch4"},
	{KEY_LAUNCH5               ,"Launch5"},
	{KEY_LAUNCH6               ,"Launch6"},
	{KEY_LAUNCH7               ,"Launch7"},
	{KEY_LAUNCH8               ,"Launch8"},
	{KEY_LAUNCH9               ,"Launch9"},
	{KEY_LAUNCHA               ,"LaunchA"},
	{KEY_LAUNCHB               ,"LaunchB"},
	{KEY_LAUNCHC               ,"LaunchC"},
	{KEY_LAUNCHD               ,"LaunchD"},
	{KEY_LAUNCHE               ,"LaunchE"},
	{KEY_LAUNCHF               ,"LaunchF"},
	{KEY_GLOBE                 ,"Globe"},
	{KEY_KEYBOARD              ,"On-screen keyboard"},
	{KEY_JIS_EISU              ,"JIS Eisu"},
	{KEY_JIS_KANA              ,"JIS Kana"},
	{KEY_UNKNOWN               ,"Unknown"},
	{KEY_SPACE                 ,"Space"},
	{KEY_EXCLAM                ,"Exclam"},
	{KEY_QUOTEDBL              ,"QuoteDbl"},
	{KEY_NUMBERSIGN            ,"NumberSign"},
	{KEY_DOLLAR                ,"Dollar"},
	{KEY_PERCENT               ,"Percent"},
	{KEY_AMPERSAND             ,"Ampersand"},
	{KEY_APOSTROPHE            ,"Apostrophe"},
	{KEY_PARENLEFT             ,"ParenLeft"},
	{KEY_PARENRIGHT            ,"ParenRight"},
	{KEY_ASTERISK              ,"Asterisk"},
	{KEY_PLUS                  ,"Plus"},
	{KEY_COMMA                 ,"Comma"},
	{KEY_MINUS                 ,"Minus"},
	{KEY_PERIOD                ,"Period"},
	{KEY_SLASH                 ,"Slash"},
	{KEY_0                     ,"0"},
	{KEY_1                     ,"1"},
	{KEY_2                     ,"2"},
	{KEY_3                     ,"3"},
	{KEY_4                     ,"4"},
	{KEY_5                     ,"5"},
	{KEY_6                     ,"6"},
	{KEY_7                     ,"7"},
	{KEY_8                     ,"8"},
	{KEY_9                     ,"9"},
	{KEY_COLON                 ,"Colon"},
	{KEY_SEMICOLON             ,"Semicolon"},
	{KEY_LESS                  ,"Less"},
	{KEY_EQUAL                 ,"Equal"},
	{KEY_GREATER               ,"Greater"},
	{KEY_QUESTION              ,"Question"},
	{KEY_AT                    ,"At"},
	{KEY_A                     ,"A"},
	{KEY_B                     ,"B"},
	{KEY_C                     ,"C"},
	{KEY_D                     ,"D"},
	{KEY_E                     ,"E"},
	{KEY_F                     ,"F"},
	{KEY_G                     ,"G"},
	{KEY_H                     ,"H"},
	{KEY_I                     ,"I"},
	{KEY_J                     ,"J"},
	{KEY_K                     ,"K"},
	{KEY_L                     ,"L"},
	{KEY_M                     ,"M"},
	{KEY_N                     ,"N"},
	{KEY_O                     ,"O"},
	{KEY_P                     ,"P"},
	{KEY_Q                     ,"Q"},
	{KEY_R                     ,"R"},
	{KEY_S                     ,"S"},
	{KEY_T                     ,"T"},
	{KEY_U                     ,"U"},
	{KEY_V                     ,"V"},
	{KEY_W                     ,"W"},
	{KEY_X                     ,"X"},
	{KEY_Y                     ,"Y"},
	{KEY_Z                     ,"Z"},
	{KEY_BRACKETLEFT           ,"BracketLeft"},
	{KEY_BACKSLASH             ,"BackSlash"},
	{KEY_BRACKETRIGHT          ,"BracketRight"},
	{KEY_ASCIICIRCUM           ,"AsciiCircum"},
	{KEY_UNDERSCORE            ,"UnderScore"},
	{KEY_QUOTELEFT             ,"QuoteLeft"},
	{KEY_BRACELEFT             ,"BraceLeft"},
	{KEY_BAR                   ,"Bar"},
	{KEY_BRACERIGHT            ,"BraceRight"},
	{KEY_ASCIITILDE            ,"AsciiTilde"},
	{KEY_YEN                   ,"Yen"},
	{KEY_SECTION               ,"Section"},
	{KEY_NONE                  ,nullptr}
	/* clang-format on */
};

// Maps to 2*axis if value is neg, or 2*axis+1 if value is pos.
static const char *joy_axis_descriptions[JOY_AXIS_MAX * 2] = {
    "Left Stick Left, Joystick 0 Left",
    "Left Stick Right, Joystick 0 Right",
    "Left Stick Up, Joystick 0 Up",
    "Left Stick Down, Joystick 0 Down",
    "Right Stick Left, Joystick 1 Left",
    "Right Stick Right, Joystick 1 Right",
    "Right Stick Up, Joystick 1 Up",
    "Right Stick Down, Joystick 1 Down",
    "Joystick 2 Left",
    "Left Trigger, Sony L2, Xbox LT, Joystick 2 Right",
    "Joystick 2 Up",
    "Right Trigger, Sony R2, Xbox RT, Joystick 2 Down",
    "Joystick 3 Left",
    "Joystick 3 Right",
    "Joystick 3 Up",
    "Joystick 3 Down",
    "Joystick 4 Left",
    "Joystick 4 Right",
    "Joystick 4 Up",
    "Joystick 4 Down",
};

int keycode_get_value_by_index(int p_index) {
    return keycodes[p_index].code;
}

const char *keycode_get_name_by_index(int p_index) {
    return keycodes[p_index].text;
}

int keycode_get_count() {
    const KeyCodeText *kct = &keycodes[0];

    int count = 0;
    while (kct->text) {
        count++;
        kct++;
    }
    return count;
}

Key fix_keycode(char32_t p_char, Key p_key) {
    if (p_char >= 0x20 && p_char <= 0x7E) {
        return (Key)String::chr(p_char).to_upper()[0];
    }
    return p_key;
}

Key fix_key_label(char32_t p_char, Key p_key) {
    if (p_char >= 0x20 && p_char != 0x7F) {
        return (Key)String::chr(p_char).to_upper()[0];
    }
    return p_key;
}

String OrchestratorEditorInputEventListenerLineEdit::_get_modifiers_text(const Ref<InputEventWithModifiers>& p_event) {
    PackedStringArray mods;
    if (p_event.is_valid()) {
        OS* os = OS::get_singleton();
        if (p_event->is_ctrl_pressed()) {
            mods.push_back(os->get_keycode_string(KEY_CTRL));
        }
        if (p_event->is_shift_pressed()) {
            mods.push_back(os->get_keycode_string(KEY_SHIFT));
        }
        if (p_event->is_alt_pressed()) {
            mods.push_back(os->get_keycode_string(KEY_ALT));
        }
        if (p_event->is_meta_pressed()) {
            mods.push_back(os->get_keycode_string(KEY_META));
        }
    }
    return String("+").join(mods);
}

bool OrchestratorEditorInputEventListenerLineEdit::_is_over_clear_button(const Vector2& p_position) {
    if (!is_clear_button_enabled()) {
        return false;
    }

    if (!Rect2(Point2(), get_size()).has_point(p_position)) {
        return false;
    }

    const Ref<Texture2D> icon = SceneUtils::get_editor_icon("Clear");
    const Ref<StyleBox> normal = get_theme_stylebox("normal");

    if (is_layout_rtl()) {
        return p_position.x < normal->get_margin(SIDE_LEFT) + icon->get_width();
    } else {
        return p_position.x > get_size().width - icon->get_width() - normal->get_margin(SIDE_RIGHT);
    }
}

bool OrchestratorEditorInputEventListenerLineEdit::_is_event_allowed(const Ref<InputEvent>& p_event) const {
    const Ref<InputEventMouseButton> mb = p_event;
    const Ref<InputEventKey> k = p_event;
    const Ref<InputEventJoypadButton> jb = p_event;
    const Ref<InputEventJoypadMotion> jm = p_event;

    return mb.is_valid() && (_allowed_input_types & INPUT_MOUSE_BUTTON) ||
                k.is_valid() && (_allowed_input_types & INPUT_KEY) ||
                jb.is_valid() && (_allowed_input_types & INPUT_JOY_BUTTON) ||
                jm.is_valid() && (_allowed_input_types & INPUT_JOY_MOTION);
}

void OrchestratorEditorInputEventListenerLineEdit::_text_changed(const String& p_text) {
    if (p_text.is_empty()) {
        clear_event();
    }
}

void OrchestratorEditorInputEventListenerLineEdit::_gui_input(const Ref<InputEvent>& p_event) {
    const Ref<InputEventMouseMotion> mm = p_event;
    if (mm.is_valid()) {
        // This is always delegated to the LineEdit::gui_input upon return
        return;
    }

    const Ref<InputEventMouseButton> mb = p_event;
    if (mb.is_valid() && _is_over_clear_button(mb->get_position())) {
        // This is always delegated to the LineEdit::gui_input upon return
        return;
    }

    if (_ignore_next_event) {
        _ignore_next_event = false;
        return;
    }

    Ref<InputEvent> event_to_check = p_event;

    const uint64_t hold_to_unfocus_timeout = 3000;
    if (p_event->is_action_pressed("ui_cancel", true, true)) {
        if (Time::get_singleton()->get_ticks_msec() - _hold_next < hold_to_unfocus_timeout) {
            _hold_next = 0;
            Control* next = find_next_valid_focus();
            if (next) {
                next->grab_focus();
            }
        } else {
            _hold_next = Time::get_singleton()->get_ticks_msec();
            _hold_event = p_event;
        }
        accept_event();
        return;
    }

    if (p_event->is_action_released("ui_cancel", true)) {
        event_to_check = _hold_event;
        _hold_next = 0;
        _hold_event = Ref<InputEvent>();
    }

    accept_event();
    if (event_to_check.is_null() || !event_to_check->is_pressed() || event_to_check->is_echo() ||
            event_to_check->is_match(_event) || !_is_event_allowed(event_to_check)) {
        return;
    }

    _event = event_to_check;
    set_text(get_event_text(_event, false));
    emit_signal("event_changed", _event);
}

String OrchestratorEditorInputEventListenerLineEdit::get_event_text(const Ref<InputEvent>& p_event, bool p_include_device) {
    ERR_FAIL_COND_V_MSG(p_event.is_null(), {}, "Provided event is not a vaild instance of InputEvent");

    String text;
    Ref<InputEventKey> key = p_event;
    if (key.is_valid()) {
        String mods_text = _get_modifiers_text(key);
        mods_text = mods_text.is_empty() ? mods_text : mods_text + "+";

        if (key->is_command_or_control_autoremap()) {
            if (OSUtils::prefer_meta_over_ctrl()) {
                mods_text = mods_text.replace("Command", "Command/Ctrl");
            } else {
                mods_text = mods_text.replace("Ctrl", "Command/Ctrl");
            }
        }

        if (key->get_keycode() != KEY_NONE) {
            text += mods_text + OS::get_singleton()->get_keycode_string(key->get_keycode());
        }

        if (key->get_physical_keycode() != KEY_NONE) {
            if (!text.is_empty()) {
                text += " or ";
            }
            text += mods_text + OS::get_singleton()->get_keycode_string(key->get_physical_keycode()) + " (Physical";
            if (key->get_location() != KEY_LOCATION_UNSPECIFIED) {
                text += " " + key->as_text_location();
            }
            text += ")";
        }

        if (key->get_key_label() != KEY_NONE) {
            if (!text.is_empty()) {
                text += " or ";
            }
            text += mods_text + OS::get_singleton()->get_keycode_string(key->get_key_label()) + " (Unicode)";
        }

        if (text.is_empty()) {
            text = "(unset)";
        }
    } else {
        text = p_event->as_text();
    }

    Ref<InputEventMouse> mouse = p_event;
    Ref<InputEventJoypadMotion> jp_motion = p_event;
    Ref<InputEventJoypadButton> jp_button = p_event;
    if (jp_motion.is_valid()) {
        // Joypad motion events will display slightly differently than what the event->as_text() provides.
        // See the Godot issue #43660.
        String desc = "Unknown Joypad Axis";
        if (jp_motion->get_axis() < JOY_AXIS_MAX) {
            desc = joy_axis_descriptions[2 * (size_t)jp_motion->get_axis() + (jp_motion->get_axis_value() < 0 ? 0 : 1)];
        }
        text = vformat("Joypad Axis %d %s (%s)", jp_motion->get_axis(), jp_motion->get_axis_value() < 0 ? "-" : "+", desc);
    }

    if (p_include_device && (mouse.is_valid() || jp_button.is_valid() || jp_motion.is_valid())) {
        String device_string = get_device_string(p_event->get_device());
        text += vformat(" - %s", device_string);
    }

    return text;
}

String OrchestratorEditorInputEventListenerLineEdit::get_device_string(int p_device) {
    if (p_device == ALL_DEVICES) {
        return "All Devices";
    }
    return vformat("Device %d", p_device);
}

Ref<InputEvent> OrchestratorEditorInputEventListenerLineEdit::get_event() const {
    return _event;
}

void OrchestratorEditorInputEventListenerLineEdit::clear_event() {
    if (_event.is_valid()) {
        _event = Ref<InputEvent>();
        set_text("");
        emit_signal("event_changed", _event);
    }
}

void OrchestratorEditorInputEventListenerLineEdit::set_allowed_input_types(int p_type_masks) {
    _allowed_input_types = p_type_masks;
}

int OrchestratorEditorInputEventListenerLineEdit::get_allowed_input_types() const {
    return _allowed_input_types;
}

void OrchestratorEditorInputEventListenerLineEdit::grab_focus() {
    // If we grab focus through code, we don't need to ignore the first event
    _ignore_next_event = false;
    Control::grab_focus();
}

void OrchestratorEditorInputEventListenerLineEdit::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_THEME_CHANGED: {
            set_right_icon(SceneUtils::get_editor_icon("Keyboard"));
            break;
        }
        case NOTIFICATION_ENTER_TREE: {
            connect(SceneStringName(text_changed), callable_mp_this(_text_changed));
            set_clear_button_enabled(true);
            break;
        }
        case NOTIFICATION_FOCUS_ENTER: {
            AcceptDialog* dialog = cast_to<AcceptDialog>(get_window());
            if (dialog) {
                dialog->set_close_on_escape(false);
            }
            set_placeholder("Listening for Input");
            break;
        }
        case NOTIFICATION_FOCUS_EXIT: {
            AcceptDialog* dialog = cast_to<AcceptDialog>(get_window());
            if (dialog) {
                dialog->set_close_on_escape(true);
            }
            _ignore_next_event = true;
            set_placeholder("Filter by Event");
            break;
        }
    }
}

void OrchestratorEditorInputEventListenerLineEdit::_bind_methods() {
    ADD_SIGNAL(MethodInfo("event_changed", PropertyInfo(Variant::OBJECT, "event", PROPERTY_HINT_RESOURCE_TYPE, InputEvent::get_class_static())));
}

OrchestratorEditorInputEventListenerLineEdit::OrchestratorEditorInputEventListenerLineEdit() {
    set_caret_blink_enabled(false);
    set_placeholder("Filter by Event");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OrchestratorEditorInputEventConfigurationDialog::_set_event(const Ref<InputEvent>& p_event, const Ref<InputEvent>& p_original_event, bool p_update_input_list_selection) {
    if (p_event.is_valid()) {
		// If there is already a binding set, let enter or escape confirm/cancel the popup.
		if (_event.is_valid()) {
			Ref<InputEventKey> current_key = p_event;
			Ref<InputEventWithModifiers> modifiers = p_event;
			// Without this is_visible() check, the gui would not open if the already bound
			// keybind was escape.
			if (current_key.is_valid() && is_visible() && _event_listener->has_focus() && !modifiers->get_modifiers_mask()) {
				if (current_key->get_physical_keycode() == KEY_ENTER) {
				    get_ok_button()->set_pressed(true);
					return;
				}
				if (current_key->get_physical_keycode() == KEY_ESCAPE) {
				    get_cancel_button()->set_pressed(true);
					return;
				}
			}
		}

		// If the event is changed to something which is not the same as the listener,
		// clear out the event from the listener text box to avoid confusion.
		const Ref<InputEvent> listener_event = _event_listener->get_event();
		if (listener_event.is_valid() && !listener_event->is_match(p_event)) {
			_event_listener->clear_event();
		}

		_event = p_event;
		_original_event = p_original_event;
		// Update Label
		_event_as_text->set_text(OrchestratorEditorInputEventListenerLineEdit::get_event_text(_event, true));

		bool exists = false;
        for (int i = 0; i < _action_events.size(); i++) {
			Ref<InputEvent> ie = _action_events[i];
			if (ie.is_null()) {
				continue;
			} else if (ie->is_match(p_event)) {
				exists = true;
				break;
			}
		}

		_event_exists->set_visible(exists);
		get_ok_button()->set_disabled(exists);

		Ref<InputEventKey> k = p_event;
		Ref<InputEventMouseButton> mb = p_event;
		Ref<InputEventJoypadButton> joyb = p_event;
		Ref<InputEventJoypadMotion> joym = p_event;
		Ref<InputEventWithModifiers> mod = p_event;

		// Update option values and visibility
		bool show_mods = false;
		bool show_device = false;
		bool show_key = false;
		bool show_location = false;

		if (mod.is_valid()) {
			show_mods = true;

			_mod_checkboxes[MOD_ALT]->set_pressed_no_signal(mod->is_alt_pressed());
			_mod_checkboxes[MOD_SHIFT]->set_pressed_no_signal(mod->is_shift_pressed());
			_mod_checkboxes[MOD_CTRL]->set_pressed_no_signal(mod->is_ctrl_pressed());
			_mod_checkboxes[MOD_META]->set_pressed_no_signal(mod->is_meta_pressed());

			const bool autoremap = mod->is_command_or_control_autoremap();
			_autoremap_command_or_control_checkbox->set_pressed_no_signal(autoremap);

			_mod_checkboxes[MOD_CTRL]->set_visible(!autoremap);
			_mod_checkboxes[MOD_META]->set_visible(!autoremap);
		}

		if (k.is_valid()) {
			show_key = true;
			Key phys_key = k->get_physical_keycode();
			if (k->get_keycode() == KEY_NONE && phys_key == KEY_NONE && k->get_key_label() != KEY_NONE) {
				_key_mode->select(KEYMODE_UNICODE);
			} else if (k->get_keycode() != KEY_NONE) {
				_key_mode->select(KEYMODE_KEYCODE);
			} else if (phys_key != KEY_NONE) {
				_key_mode->select(KEYMODE_PHY_KEYCODE);
				if (phys_key == KEY_SHIFT || phys_key == KEY_CTRL || phys_key == KEY_ALT || phys_key == KEY_META) {
					_key_location->select(k->get_location());
					show_location = true;
				}
			} else {
				// Invalid key.
				_event = Ref<InputEvent>();
				_original_event = Ref<InputEvent>();
				_event_listener->clear_event();
				_event_as_text->set_text("No Event Configured");

				_additional_options_container->hide();
				_input_list_tree->deselect_all();
				_update_input_list();
				return;
			}
		} else if (joyb.is_valid() || joym.is_valid() || mb.is_valid()) {
			show_device = true;
			_set_current_device(_event->get_device());
		}

		_mod_container->set_visible(show_mods);
		_device_container->set_visible(show_device);
		_key_mode->set_visible(show_key);
		_location_container->set_visible(show_location);
		_additional_options_container->show();

		// Update mode selector based on original key event.
		Ref<InputEventKey> ko = p_original_event;
		if (ko.is_valid()) {
			if (ko->get_keycode() == KEY_NONE) {
				if (ko->get_physical_keycode() != KEY_NONE) {
					ko->set_keycode(ko->get_physical_keycode());
				}
				if (ko->get_key_label() != KEY_NONE) {
					ko->set_keycode(fix_keycode((char32_t)ko->get_key_label(), KEY_NONE));
				}
			}

			if (ko->get_physical_keycode() == KEY_NONE) {
				if (ko->get_keycode() != KEY_NONE) {
					ko->set_physical_keycode(ko->get_keycode());
				}
				if (ko->get_key_label() != KEY_NONE) {
					ko->set_physical_keycode(fix_keycode((char32_t)ko->get_key_label(), KEY_NONE));
				}
			}

			if (ko->get_key_label() == KEY_NONE) {
				if (ko->get_keycode() != KEY_NONE) {
					ko->set_key_label(fix_key_label((char32_t)ko->get_keycode(), KEY_NONE));
				}
				if (ko->get_physical_keycode() != KEY_NONE) {
					ko->set_key_label(fix_key_label((char32_t)ko->get_physical_keycode(), KEY_NONE));
				}
			}

			_key_mode->set_item_disabled(KEYMODE_KEYCODE, ko->get_keycode() == KEY_NONE);
			_key_mode->set_item_disabled(KEYMODE_PHY_KEYCODE, ko->get_physical_keycode() == KEY_NONE);
			_key_mode->set_item_disabled(KEYMODE_UNICODE, ko->get_key_label() == KEY_NONE);
		}

		// Update selected item in input list.
		if (p_update_input_list_selection && (k.is_valid() || joyb.is_valid() || joym.is_valid() || mb.is_valid())) {
			_in_tree_update = true;
			TreeItem* category = _input_list_tree->get_root()->get_first_child();
			while (category) {
				TreeItem* input_item = category->get_first_child();

				if (input_item != nullptr) {
					// input_type should always be > 0, unless the tree structure has been misconfigured.
					int input_type = input_item->get_parent()->get_meta("__type", 0);
					if (input_type == 0) {
						_in_tree_update = false;
						return;
					}

					// If event type matches input types of this category.
					if ((k.is_valid() && input_type == INPUT_KEY) || (joyb.is_valid() && input_type == INPUT_JOY_BUTTON) || (joym.is_valid() && input_type == INPUT_JOY_MOTION) || (mb.is_valid() && input_type == INPUT_MOUSE_BUTTON)) {
						// Loop through all items of this category until one matches.
						while (input_item) {
							bool key_match = k.is_valid() && (Variant(k->get_keycode()) == input_item->get_meta("__keycode") || Variant(k->get_physical_keycode()) == input_item->get_meta("__keycode"));
							bool joyb_match = joyb.is_valid() && Variant(joyb->get_button_index()) == input_item->get_meta("__index");
							bool joym_match = joym.is_valid() && Variant(joym->get_axis()) == input_item->get_meta("__axis") && joym->get_axis_value() == (float)input_item->get_meta("__value");
							bool mb_match = mb.is_valid() && Variant(mb->get_button_index()) == input_item->get_meta("__index");
							if (key_match || joyb_match || joym_match || mb_match) {
								category->set_collapsed(false);
								input_item->select(0);
								_input_list_tree->ensure_cursor_is_visible();
								_in_tree_update = false;
								return;
							}
							input_item = input_item->get_next();
						}
					}
				}

				category->set_collapsed(true); // Event not in this category, so collapse;
				category = category->get_next();
			}
			_in_tree_update = false;
		}
	} else {
		// Event is not valid, reset dialog
		_event = Ref<InputEvent>();
		_original_event = Ref<InputEvent>();
		_event_listener->clear_event();
		_event_as_text->set_text("No Event Configured");
		_event_exists->set_visible(false);
		get_ok_button()->set_disabled(false);

		_additional_options_container->hide();
		_input_list_tree->deselect_all();
		_update_input_list();
	}
}

void OrchestratorEditorInputEventConfigurationDialog::_listen_input_changed(const Ref<InputEvent>& p_event) {
    // Ignore if invalid, echo or not pressed
    if (p_event.is_null() || p_event->is_echo() || !p_event->is_pressed()) {
        return;
    }

    // Create an editable reference and a copy of full event.
    Ref<InputEvent> received_event = p_event;
    Ref<InputEvent> received_original_event = received_event->duplicate();

    // Check what the type is and if it is allowed.
    Ref<InputEventKey> k = received_event;
    Ref<InputEventJoypadButton> joyb = received_event;
    Ref<InputEventJoypadMotion> joym = received_event;
    Ref<InputEventMouseButton> mb = received_event;

    int type = 0;
    if (k.is_valid()) {
        type = INPUT_KEY;
    } else if (joyb.is_valid()) {
        type = INPUT_JOY_BUTTON;
    } else if (joym.is_valid()) {
        type = INPUT_JOY_MOTION;
    } else if (mb.is_valid()) {
        type = INPUT_MOUSE_BUTTON;
    }

    if (!(_allowed_input_types & type)) {
        return;
    }

    if (joym.is_valid()) {
        joym->set_axis_value(SIGN(joym->get_axis_value()));
    }

    if (k.is_valid()) {
        k->set_pressed(false); // To avoid serialization of 'pressed' property, doesn't matter for actions anyway.
        if (_key_mode->get_selected_id() == KEYMODE_KEYCODE) {
            k->set_physical_keycode(KEY_NONE);
            k->set_key_label(KEY_NONE);
        } else if (_key_mode->get_selected_id() == KEYMODE_PHY_KEYCODE) {
            k->set_keycode(KEY_NONE);
            k->set_key_label(KEY_NONE);
        } else if (_key_mode->get_selected_id() == KEYMODE_UNICODE) {
            k->set_physical_keycode(KEY_NONE);
            k->set_keycode(KEY_NONE);
        }
        if (_key_location->get_selected_id() == KEY_LOCATION_UNSPECIFIED) {
            k->set_location(KEY_LOCATION_UNSPECIFIED);
        }
    }

    Ref<InputEventWithModifiers> mod = received_event;
    if (mod.is_valid()) {
        mod->set_window_id(0);
    }

    // Maintain device selection.
    received_event->set_device(_get_current_device());

    _set_event(received_event, received_original_event);
}

void OrchestratorEditorInputEventConfigurationDialog::_search_term_updated(const String& p_term) {
    _update_input_list();
}

void OrchestratorEditorInputEventConfigurationDialog::_update_input_list() {
    _input_list_tree->clear();

	TreeItem* root = _input_list_tree->create_item();
	String search_term = _input_list_search->get_text();

	bool collapse = _input_list_search->get_text().is_empty();

	if (_allowed_input_types & INPUT_KEY) {
		TreeItem* kb_root = _input_list_tree->create_item(root);
		kb_root->set_text(0, "Keyboard Keys");
		kb_root->set_icon(0, icon_cache.keyboard);
		kb_root->set_collapsed(collapse);
		kb_root->set_meta("__type", INPUT_KEY);

		for (int i = 0; i < keycode_get_count(); i++) {
			String name = keycode_get_name_by_index(i);

			if (!search_term.is_empty() && !name.containsn(search_term)) {
				continue;
			}

			TreeItem* item = _input_list_tree->create_item(kb_root);
			item->set_text(0, name);
			item->set_meta("__keycode", keycode_get_value_by_index(i));
		}
	}

	if (_allowed_input_types & INPUT_MOUSE_BUTTON) {
		TreeItem *mouse_root = _input_list_tree->create_item(root);
		mouse_root->set_text(0, "Mouse Buttons");
		mouse_root->set_icon(0, icon_cache.mouse);
		mouse_root->set_collapsed(collapse);
		mouse_root->set_meta("__type", INPUT_MOUSE_BUTTON);

		MouseButton mouse_buttons[9] = {
		    MOUSE_BUTTON_LEFT,
		    MOUSE_BUTTON_RIGHT,
		    MOUSE_BUTTON_MIDDLE,
		    MOUSE_BUTTON_WHEEL_UP,
		    MOUSE_BUTTON_WHEEL_DOWN,
		    MOUSE_BUTTON_WHEEL_LEFT,
		    MOUSE_BUTTON_WHEEL_RIGHT,
		    MOUSE_BUTTON_XBUTTON1,
		    MOUSE_BUTTON_XBUTTON2
		};

		for (MouseButton mouse_button : mouse_buttons) {
			Ref<InputEventMouseButton> mb;
			mb.instantiate();
			mb->set_button_index(mouse_button);
			String desc = OrchestratorEditorInputEventListenerLineEdit::get_event_text(mb, false);

			if (!search_term.is_empty() && !desc.containsn(search_term)) {
				continue;
			}

			TreeItem* item = _input_list_tree->create_item(mouse_root);
			item->set_text(0, desc);
			item->set_meta("__index", mouse_button);
		}
	}

	if (_allowed_input_types & INPUT_JOY_BUTTON) {
		TreeItem* joyb_root = _input_list_tree->create_item(root);
		joyb_root->set_text(0, "Joypad Buttons");
		joyb_root->set_icon(0, icon_cache.joypad_button);
		joyb_root->set_collapsed(collapse);
		joyb_root->set_meta("__type", INPUT_JOY_BUTTON);

		for (int i = 0; i < JOY_BUTTON_MAX; i++) {
			Ref<InputEventJoypadButton> joyb;
			joyb.instantiate();
			joyb->set_button_index(static_cast<JoyButton>(i));
			String desc = OrchestratorEditorInputEventListenerLineEdit::get_event_text(joyb, false);

			if (!search_term.is_empty() && !desc.containsn(search_term)) {
				continue;
			}

			TreeItem* item = _input_list_tree->create_item(joyb_root);
			item->set_text(0, desc);
			item->set_meta("__index", i);
		}
	}

	if (_allowed_input_types & INPUT_JOY_MOTION) {
		TreeItem *joya_root = _input_list_tree->create_item(root);
		joya_root->set_text(0, "Joypad Axes");
		joya_root->set_icon(0, icon_cache.joypad_axis);
		joya_root->set_collapsed(collapse);
		joya_root->set_meta("__type", INPUT_JOY_MOTION);

		for (int i = 0; i < JOY_AXIS_MAX * 2; i++) {
			int axis = i / 2;
			int direction = (i & 1) ? 1 : -1;
			Ref<InputEventJoypadMotion> joym;
			joym.instantiate();
			joym->set_axis(static_cast<JoyAxis>(axis));
			joym->set_axis_value(direction);
			String desc = OrchestratorEditorInputEventListenerLineEdit::get_event_text(joym, false);

			if (!search_term.is_empty() && !desc.containsn(search_term)) {
				continue;
			}

			TreeItem* item = _input_list_tree->create_item(joya_root);
			item->set_text(0, desc);
			item->set_meta("__axis", i >> 1);
			item->set_meta("__value", (i & 1) ? 1 : -1);
		}
	}
}

void OrchestratorEditorInputEventConfigurationDialog::_input_list_item_activated() {
	TreeItem *selected = _input_list_tree->get_selected();
	selected->set_collapsed(!selected->is_collapsed());
}

void OrchestratorEditorInputEventConfigurationDialog::_input_list_item_selected() {
	TreeItem *selected = _input_list_tree->get_selected();

	// Called form _set_event, do not update for a second time.
	if (_in_tree_update) {
		return;
	}

	// Invalid tree selection - type only exists on the "category" items, which are not a valid selection.
	if (selected->has_meta("__type")) {
		return;
	}

	switch (static_cast<OrchestratorEditorInputType>(static_cast<int>(selected->get_parent()->get_meta("__type")))) {
		case INPUT_KEY: {
		    Key keycode = static_cast<Key>(static_cast<int>(selected->get_meta("__keycode")));
		    Ref<InputEventKey> k;
		    k.instantiate();

		    k->set_physical_keycode(keycode);
		    k->set_keycode(keycode);
		    k->set_key_label(keycode);

		    // Maintain modifier state from checkboxes.
		    k->set_alt_pressed(_mod_checkboxes[MOD_ALT]->is_pressed());
		    k->set_shift_pressed(_mod_checkboxes[MOD_SHIFT]->is_pressed());
		    if (_autoremap_command_or_control_checkbox->is_pressed()) {
		        k->set_command_or_control_autoremap(true);
		    } else {
		        k->set_ctrl_pressed(_mod_checkboxes[MOD_CTRL]->is_pressed());
		        k->set_meta_pressed(_mod_checkboxes[MOD_META]->is_pressed());
		    }

		    Ref<InputEventKey> ko = k->duplicate();

		    if (_key_mode->get_selected_id() == KEYMODE_UNICODE) {
		        _key_mode->select(KEYMODE_PHY_KEYCODE);
		    }

		    if (_key_mode->get_selected_id() == KEYMODE_KEYCODE) {
		        k->set_physical_keycode(KEY_NONE);
		        k->set_keycode(keycode);
		        k->set_key_label(KEY_NONE);
		    } else if (_key_mode->get_selected_id() == KEYMODE_PHY_KEYCODE) {
		        k->set_physical_keycode(keycode);
		        k->set_keycode(KEY_NONE);
		        k->set_key_label(KEY_NONE);
		    }
		    _set_event(k, ko, false);
		    break;
		}
		case INPUT_MOUSE_BUTTON: {
		    MouseButton idx = static_cast<MouseButton>(static_cast<int>(selected->get_meta("__index")));
			Ref<InputEventMouseButton> mb;
			mb.instantiate();
			mb->set_button_index(idx);
			// Maintain modifier state from checkboxes
			mb->set_alt_pressed(_mod_checkboxes[MOD_ALT]->is_pressed());
			mb->set_shift_pressed(_mod_checkboxes[MOD_SHIFT]->is_pressed());
			if (_autoremap_command_or_control_checkbox->is_pressed()) {
				mb->set_command_or_control_autoremap(true);
			} else {
				mb->set_ctrl_pressed(_mod_checkboxes[MOD_CTRL]->is_pressed());
				mb->set_meta_pressed(_mod_checkboxes[MOD_META]->is_pressed());
			}

			// Maintain selected device
			mb->set_device(_get_current_device());
			_set_event(mb, mb, false);
		    break;
		}
		case INPUT_JOY_BUTTON: {
			JoyButton idx = (JoyButton)(int)selected->get_meta("__index");
			// Maintain selected device

			Ref<InputEventJoypadButton> jb;
		    jb.instantiate();
		    jb->set_button_index(idx);
		    jb->set_device(_get_current_device());

			_set_event(jb, jb, false);
		} break;
		case INPUT_JOY_MOTION: {
			JoyAxis axis = (JoyAxis)(int)selected->get_meta("__axis");
			int value = selected->get_meta("__value");

			Ref<InputEventJoypadMotion> jm;
			jm.instantiate();
			jm->set_axis(axis);
			jm->set_axis_value(value);

			// Maintain selected device
			jm->set_device(_get_current_device());

			_set_event(jm, jm, false);
		} break;
	    default: {
	        break;
	    }
	}
}

void OrchestratorEditorInputEventConfigurationDialog::_mod_toggled(bool p_checked, int p_index) {
    Ref<InputEventWithModifiers> ie = _event;

    // Not event with modifiers
    if (ie.is_null()) {
        return;
    }

    if (p_index == 0) {
        ie->set_alt_pressed(p_checked);
    } else if (p_index == 1) {
        ie->set_shift_pressed(p_checked);
    } else if (p_index == 2) {
        if (!_autoremap_command_or_control_checkbox->is_pressed()) {
            ie->set_ctrl_pressed(p_checked);
        }
    } else if (p_index == 3) {
        if (!_autoremap_command_or_control_checkbox->is_pressed()) {
            ie->set_meta_pressed(p_checked);
        }
    }

    _set_event(ie, _original_event);
}

void OrchestratorEditorInputEventConfigurationDialog::_auto_remap_command_or_control_toggled(bool p_checked) {
    Ref<InputEventWithModifiers> ie = _event;
    if (ie.is_valid()) {
        ie->set_command_or_control_autoremap(p_checked);
        _set_event(ie, _original_event);
    }

    if (p_checked) {
        _mod_checkboxes[MOD_META]->hide();
        _mod_checkboxes[MOD_CTRL]->hide();
    } else {
        _mod_checkboxes[MOD_META]->show();
        _mod_checkboxes[MOD_CTRL]->show();
    }
}

void OrchestratorEditorInputEventConfigurationDialog::_key_mode_selected(int p_mode) {
    Ref<InputEventKey> k = _event;
    Ref<InputEventKey> ko = _original_event;
    if (k.is_null() || ko.is_null()) {
        return;
    }

    if (_key_mode->get_selected_id() == KEYMODE_KEYCODE) {
        k->set_keycode(ko->get_keycode());
        k->set_physical_keycode(KEY_NONE);
        k->set_key_label(KEY_NONE);
    } else if (_key_mode->get_selected_id() == KEYMODE_PHY_KEYCODE) {
        k->set_keycode(KEY_NONE);
        k->set_physical_keycode(ko->get_physical_keycode());
        k->set_key_label(KEY_NONE);
    } else if (_key_mode->get_selected_id() == KEYMODE_UNICODE) {
        k->set_physical_keycode(KEY_NONE);
        k->set_keycode(KEY_NONE);
        k->set_key_label(ko->get_key_label());
    }

    _set_event(k, _original_event);
}

void OrchestratorEditorInputEventConfigurationDialog::_key_location_selected(int p_location) {
    Ref<InputEventKey> k = _event;
    if (k.is_null()) {
        return;
    }

    k->set_location(static_cast<KeyLocation>(p_location));
    _set_event(k, _original_event);
}

void OrchestratorEditorInputEventConfigurationDialog::_device_selection_changed(int p_option_button_index) {
    // Subtract 1 as option index 0 corresponds to "All Devices" (value of -1)
    // and option index 1 corresponds to device 0, etc...
    _event->set_device(p_option_button_index - 1);
    _event_as_text->set_text(OrchestratorEditorInputEventListenerLineEdit::get_event_text(_event, true));
}

void OrchestratorEditorInputEventConfigurationDialog::_set_current_device(int p_device) {
    const int index = p_device + 1;
    if (index < 0 || index >= _device_id_option->get_item_count()) {
        _device_id_option->select(0);
        return;
    }

    _device_id_option->select(index);
}

int OrchestratorEditorInputEventConfigurationDialog::_get_current_device() const {
    return _device_id_option->get_selected() - 1;
}

void OrchestratorEditorInputEventConfigurationDialog::popup_and_configure(const Ref<InputEvent>& p_event, const String& p_current_action_name, const Dictionary& p_current_action) {
    _action_events = p_current_action.get("events", Array()).duplicate();

    if (p_event.is_valid()) {
        // Remove one instance of the InputEvent being edited
        // This avoids it being flagged as a duplicate in the dialog
        for (int i = 0; i < _action_events.size(); i++) {
            Ref<InputEvent> evt = _action_events[i];
            if (evt.is_null()) {
                continue;
            }
            if (evt->is_match(p_event)) {
                _action_events.remove_at(i);
                break;
            }
        }
        _set_event(p_event->duplicate(), p_event->duplicate());
    } else {
        // Clear the event
        _set_event(Ref<InputEvent>(), Ref<InputEvent>());

        // Clear Checkbox Values
        for (CheckBox* _mod_checkbox : _mod_checkboxes) {
            _mod_checkbox->set_pressed(false);
        }

        // Enable the Physical Key by default to encourage its use
        // Physical key should be used for most game inputs as it allows keys to work for non-QWERTY layouts OOTB.
        // This is especially important for WASD movement layouts
        _key_mode->select(KEYMODE_PHY_KEYCODE);
        _autoremap_command_or_control_checkbox->set_pressed(false);

        // Selects "All Devices"
        _device_id_option->select(0);
        // Selects "All Locations"
        _key_location->select(0);
    }

    if (!p_current_action_name.is_empty()) {
        set_title(vformat("Event Configuration for %s", p_current_action_name));
    } else {
        set_title("Event Configuration");
    }

    popup_centered(Size2(0, 400) * EDSCALE);
}

Ref<InputEvent> OrchestratorEditorInputEventConfigurationDialog::get_event() const {
    return _event;
}

void OrchestratorEditorInputEventConfigurationDialog::set_allowed_input_types(int p_type_masks) {
    _allowed_input_types = p_type_masks;
    _event_listener->set_allowed_input_types(p_type_masks);
}

void OrchestratorEditorInputEventConfigurationDialog::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_VISIBILITY_CHANGED: {
            _event_listener->grab_focus();
            break;
        }
        case NOTIFICATION_THEME_CHANGED: {
            _input_list_search->set_right_icon(SceneUtils::get_editor_icon("Search"));

            _key_mode->set_item_icon(KEYMODE_KEYCODE, SceneUtils::get_editor_icon("Keyboard"));
            _key_mode->set_item_icon(KEYMODE_PHY_KEYCODE, SceneUtils::get_editor_icon("KeyboardPhysical"));
            _key_mode->set_item_icon(KEYMODE_UNICODE, SceneUtils::get_editor_icon("KeyboardLabel"));

            icon_cache.keyboard = SceneUtils::get_editor_icon("Keyboard");
            icon_cache.mouse = SceneUtils::get_editor_icon("Mouse");
            icon_cache.joypad_button = SceneUtils::get_editor_icon("JoyButton");
            icon_cache.joypad_axis = SceneUtils::get_editor_icon("JoyAxis");

            _event_as_text->add_theme_font_override(SceneStringName(font), SceneUtils::get_editor_font("bold"));
            _event_exists->add_theme_color_override(SceneStringName(font_color), SceneUtils::get_editor_color("error_color"));

            _update_input_list();
            break;
        }
        case NOTIFICATION_TRANSLATION_CHANGED: {
            _key_location->set_item_text(_key_location->get_item_index(KEY_LOCATION_UNSPECIFIED), "Unspecified");
            _key_location->set_item_text(_key_location->get_item_index(KEY_LOCATION_LEFT), "Left");
            _key_location->set_item_text(_key_location->get_item_index(KEY_LOCATION_RIGHT), "Right");
            break;
        }
    }
}

void OrchestratorEditorInputEventConfigurationDialog::_bind_methods() {
}

OrchestratorEditorInputEventConfigurationDialog::OrchestratorEditorInputEventConfigurationDialog() {
    _allowed_input_types = INPUT_ALL;

    set_min_size(Size2i(800, 0) * EDSCALE);

    VBoxContainer* main_vbox = memnew(VBoxContainer);
    add_child(main_vbox);

    _event_as_text = memnew(Label);
    #if GODOT_VERSION >= 0x040500
    _event_as_text->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
    #endif
    _event_as_text->set_custom_minimum_size(Size2(500, 0) * EDSCALE);
    _event_as_text->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    _event_as_text->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
    _event_as_text->add_theme_font_size_override(SceneStringName(font_size), 18 * EDSCALE);
    main_vbox->add_child(_event_as_text);

    _event_listener = memnew(OrchestratorEditorInputEventListenerLineEdit);
    _event_listener->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    _event_listener->set_stretch_ratio(0.75);
    _event_listener->connect("event_changed", callable_mp_this(_listen_input_changed));
    main_vbox->add_child(_event_listener);

    main_vbox->add_child(memnew(HSeparator));

	// List of all input options to manually select from.
	VBoxContainer *manual_vbox = memnew(VBoxContainer);
	manual_vbox->set_name("Manual Selection");
	manual_vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	main_vbox->add_child(manual_vbox);

	_input_list_search = memnew(LineEdit);
	_input_list_search->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	_input_list_search->set_placeholder("Filter Inputs");
    #if GODOT_VERSION >= 0x040500
	_input_list_search->set_accessibility_name("Filter Inputs");
    #endif
	_input_list_search->set_clear_button_enabled(true);
	_input_list_search->connect(SceneStringName(text_changed), callable_mp_this(_search_term_updated));
	manual_vbox->add_child(_input_list_search);

	MarginContainer *mc = memnew(MarginContainer);
	mc->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	mc->set_theme_type_variation("NoBorderHorizontalWindow");
	manual_vbox->add_child(mc);

	_input_list_tree = memnew(Tree);
	_input_list_tree->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
    #if GODOT_VERSION >= 0x040600
	_input_list_tree->set_scroll_hint_mode(Tree::SCROLL_HINT_MODE_BOTH);
    #endif
	_input_list_tree->connect("item_activated", callable_mp_this(_input_list_item_activated));
	_input_list_tree->connect(SceneStringName(item_selected), callable_mp_this(_input_list_item_selected));
	mc->add_child(_input_list_tree);

	_input_list_tree->set_hide_root(true);
	_input_list_tree->set_columns(1);

	_update_input_list();

	// Additional Options
	_additional_options_container = memnew(VBoxContainer);
	_additional_options_container->hide();

	Label *opts_label = memnew(Label);
    opts_label->set_text("Additional Options");
	opts_label->set_theme_type_variation("HeaderSmall");
	_additional_options_container->add_child(opts_label);

	// Device Selection
	_device_container = memnew(HBoxContainer);
	_device_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);

	Label *device_label = memnew(Label);
    device_label->set_text("Device:");
	device_label->set_theme_type_variation("HeaderSmall");
	_device_container->add_child(device_label);

	_device_id_option = memnew(OptionButton);
	_device_id_option->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	for (int i = -1; i < 8; i++) {
		_device_id_option->add_item(OrchestratorEditorInputEventListenerLineEdit::get_device_string(i));
	}
	_device_id_option->connect(SceneStringName(item_selected), callable_mp_this(_device_selection_changed));
    #if GODOT_VERSION >= 0x040500
	_device_id_option->set_accessibility_name("Device:");
    #endif
	_set_current_device(OrchestratorEditorInputEventListenerLineEdit::ALL_DEVICES);
	_device_container->add_child(_device_id_option);

	_device_container->hide();
	_additional_options_container->add_child(_device_container);

	// Modifier Selection
	_mod_container = memnew(HBoxContainer);
	for (int i = 0; i < MOD_MAX; i++) {
		String name = _mods[i];
		_mod_checkboxes[i] = memnew(CheckBox);
	    _mod_checkboxes[i]->set_text(name);
		_mod_checkboxes[i]->connect(SceneStringName(toggled), callable_mp_this(_mod_toggled).bind(i));
		_mod_checkboxes[i]->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
		_mod_checkboxes[i]->set_tooltip_auto_translate_mode(AUTO_TRANSLATE_MODE_ALWAYS);
		_mod_checkboxes[i]->set_tooltip_text(_mods_tip[i]);
		_mod_container->add_child(_mod_checkboxes[i]);
	}

	_mod_container->add_child(memnew(VSeparator));

	_autoremap_command_or_control_checkbox = memnew(CheckBox);
    _autoremap_command_or_control_checkbox->set_text("Command / Control (auto)");
	_autoremap_command_or_control_checkbox->connect(SceneStringName(toggled), callable_mp_this(_auto_remap_command_or_control_toggled));
	_autoremap_command_or_control_checkbox->set_pressed(false);
	_autoremap_command_or_control_checkbox->set_tooltip_text("Automatically remaps between 'Meta' ('Command') and 'Control' depending on current platform.");
	_mod_container->add_child(_autoremap_command_or_control_checkbox);

	_mod_container->hide();
	_additional_options_container->add_child(_mod_container);

	// Key Mode Selection

	_key_mode = memnew(OptionButton);
	_key_mode->add_item("Keycode (Latin Equivalent)", KEYMODE_KEYCODE);
	_key_mode->add_item("Physical Keycode (Position on US QWERTY Keyboard)", KEYMODE_PHY_KEYCODE);
	_key_mode->add_item("Key Label (Unicode, Case-Insensitive)", KEYMODE_UNICODE);
	_key_mode->connect(SceneStringName(item_selected), callable_mp_this(_key_mode_selected));
	_key_mode->hide();
	_additional_options_container->add_child(_key_mode);

	// Key Location Selection

	_location_container = memnew(HBoxContainer);
	_location_container->hide();

    Label* physical_location = memnew(Label);
    physical_location->set_text("Physical Location");
	_location_container->add_child(physical_location);

	_key_location = memnew(OptionButton);
	_key_location->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	_key_location->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	// Item texts will be set in `NOTIFICATION_TRANSLATION_CHANGED`.
	_key_location->add_item(String(), KEY_LOCATION_UNSPECIFIED);
	_key_location->add_item(String(), KEY_LOCATION_LEFT);
	_key_location->add_item(String(), KEY_LOCATION_RIGHT);
	_key_location->connect(SceneStringName(item_selected), callable_mp_this(_key_location_selected));
    #if GODOT_VERSION >= 0x040500
	_key_location->set_accessibility_name("Physical Location");
    #endif

	_location_container->add_child(_key_location);
	_additional_options_container->add_child(_location_container);

	main_vbox->add_child(_additional_options_container);

	_event_exists = memnew(Label);
    _event_exists->set_text("Error: This action already contains this input event.");
	_event_exists->set_theme_type_variation("HeaderSmall");
	_event_exists->set_visible(false);
	main_vbox->add_child(_event_exists);
}
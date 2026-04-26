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
#include "core/godot/editor/settings/editor_settings.h"

#include "core/godot/os/os.h"

#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/os.hpp>

namespace godot {
    // taken from InputEventKey
    Ref<InputEventKey> create_input_key_reference(Key p_keycode, bool p_physical) {
        Ref<InputEventKey> ie;
        ie.instantiate();

        if (p_physical) {
            ie->set_physical_keycode(static_cast<Key>(static_cast<uint64_t>(p_keycode) & KEY_CODE_MASK));
        } else {
            ie->set_keycode(static_cast<Key>(static_cast<uint64_t>(p_keycode) & KEY_CODE_MASK));
        }

        char32_t ch = char32_t(static_cast<uint64_t>(p_keycode) & KEY_CODE_MASK);
        if (ch < 0xd800 || (ch > 0xdfff && ch <= 0x10ffff)) {
            ie->set_unicode(ch);
        }

        if ((static_cast<uint64_t>(p_keycode) & KEY_MASK_SHIFT) != KEY_NONE) {
            ie->set_shift_pressed(true);
        }

        if ((static_cast<uint64_t>(p_keycode) & KEY_MASK_ALT) != KEY_NONE) {
            ie->set_alt_pressed(true);
        }

        if ((static_cast<uint64_t>(p_keycode) & KEY_MASK_CMD_OR_CTRL) != KEY_NONE) {
            ie->set_command_or_control_autoremap(true);
            if ((static_cast<uint64_t>(p_keycode) & KEY_MASK_CTRL) != KEY_NONE || (static_cast<uint64_t>(p_keycode) & KEY_MASK_META) != KEY_NONE) {
                WARN_PRINT("Invalid Key Modifiers: Command or Control autoremapping is enabled, Meta and Control values are ignored!");
            }
        } else {
            if ((static_cast<uint64_t>(p_keycode) & KEY_MASK_CTRL) != KEY_NONE) {
                ie->set_ctrl_pressed(true);
            }
            if ((static_cast<uint64_t>(p_keycode) & KEY_MASK_META) != KEY_NONE) {
                ie->set_meta_pressed(true);
            }
        }

        return ie;
    }

    Ref<Shortcut> ED_SHORTCUT(const String& p_path, const String& p_name, Key p_keycode, bool p_physical) {
        PackedInt32Array array;
        array.push_back(p_keycode);
        return ED_SHORTCUT_ARRAY(p_path, p_name, array, p_physical);
    }

    Ref<Shortcut> ED_SHORTCUT_ARRAY(const String& p_path, const String& p_name, const PackedInt32Array& p_keycodes, bool p_physical) {
        Array events;

        for (int i = 0; i < p_keycodes.size(); i++) {
            Key keycode = static_cast<Key>(p_keycodes[i]);
            if (GDE::OS::prefer_meta_over_ctrl()) {
                // Use Cmd+Backspace as a general replacement for Delete shortcuts on macOS
                if (keycode == KEY_DELETE) {
                    keycode = static_cast<Key>(KEY_MASK_META | static_cast<uint64_t>(KEY_BACKSPACE));
                }
            }

            Ref<InputEventKey> ie;
            if (keycode != KEY_NONE) {
                ie = create_input_key_reference(keycode, p_physical);
                events.push_back(ie);
            }
        }

        if (!EI || !EI->get_editor_settings().is_valid()) {
            Ref<Shortcut> sc;
            sc.instantiate();
            sc->set_name(p_name);
            sc->set_events(events);
            sc->set_meta("original", events.duplicate(true));
            return sc;
        }

        Ref<Shortcut> sc = EI->get_editor_settings()->get_shortcut(p_path);
        if (sc.is_valid()) {
            sc->set_name(p_name); // keep name (the one that comes from disk have no name)
            sc->set_meta("original", events.duplicate(true)); // to compare against changes
            return sc;
        }

        sc.instantiate();
        sc->set_name(p_name);
        sc->set_events(events);
        sc->set_meta("original", events.duplicate(true)); // to compare against changes
        EI->get_editor_settings()->add_shortcut(p_path, sc);

        return sc;
    }

    void ED_SHORTCUT_OVERRIDE(const String& p_path, const String& p_feature, Key p_keycode, bool p_physical) {
        if (!EI || EI->get_editor_settings().is_null()) {
            return;
        }

        Ref<Shortcut> sc = EI->get_editor_settings()->get_shortcut(p_path);
        ERR_FAIL_COND_MSG(sc.is_null(), "Used ED_SHORTCUT_OVERRIDE with invalid shortcut: " + p_path);

        PackedInt32Array array;
        array.push_back(p_keycode);
        ED_SHORTCUT_OVERRIDE_ARRAY(p_path, p_feature, array, p_physical);
    }

    void ED_SHORTCUT_OVERRIDE_ARRAY(const String& p_path, const String& p_feature, const PackedInt32Array& p_keycodes, bool p_physical) {
        if (!EI || EI->get_editor_settings().is_null()) {
            return;
        }

        Ref<Shortcut> sc = EI->get_editor_settings()->get_shortcut(p_path);
        ERR_FAIL_COND_MSG(sc.is_null(), "Used ED_SHORTCUT_OVERRIDE_ARRAY with invalid shortcut: " + p_path);

        // Only add the override if the OS supports the provided feature.
        if (!OS::get_singleton()->has_feature(p_feature)) {
            if (!(p_feature == "macos" && (OS::get_singleton()->has_feature("web_macos") || OS::get_singleton()->has_feature("web_ios")))) {
                return;
            }
        }

        Array events;

        for (int i = 0; i < p_keycodes.size(); i++) {
            Key keycode = static_cast<Key>(p_keycodes[i]);

            if (GDE::OS::prefer_meta_over_ctrl()) {
                // Use Cmd+Backspace as a general replacement for Delete shortcuts on macOS
                if (keycode == KEY_DELETE) {
                    keycode = static_cast<Key>(KEY_MASK_META | static_cast<uint64_t>(KEY_BACKSPACE));
                }
            }

            Ref<InputEventKey> ie;
            if (keycode != KEY_NONE) {
                ie = create_input_key_reference(keycode, p_physical);
                events.push_back(ie);
            }
        }

        // Override the existing shortcut only if it wasn't customized by the user.
        if (!sc->has_meta("customized")) {
            sc->set_events(events);
        }

        sc->set_meta("original", events.duplicate(true));
    }

    Ref<Shortcut> ED_GET_SHORTCUT(const String& p_path) {
        ERR_FAIL_NULL_V_MSG(EI, nullptr, "EditorSettings are not available");
        ERR_FAIL_COND_V_MSG(EI->get_editor_settings().is_null(), nullptr, "EditorSettings are not available");

        Ref<Shortcut> sc = EI->get_editor_settings()->get_shortcut(p_path);
        ERR_FAIL_COND_V_MSG(sc.is_null(), sc, "Used ED_GET_SHORTCUT with invalid shortcut: " + p_path);

        return sc;
    }
}
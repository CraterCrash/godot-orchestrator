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
#ifndef ORCHESTRATOR_CORE_GODOT_EDITOR_SETTINGS_H
#define ORCHESTRATOR_CORE_GODOT_EDITOR_SETTINGS_H

#include "common/macros.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/shortcut.hpp>

namespace godot {
    Ref<Shortcut> ED_SHORTCUT(const String& p_path, const String& p_name, Key p_keycode = Key::KEY_NONE, bool p_physical = false);
    Ref<Shortcut> ED_SHORTCUT_ARRAY(const String& p_path, const String& p_name, const PackedInt32Array& p_keycodes, bool p_physical);
    void ED_SHORTCUT_OVERRIDE(const String& p_path, const String& p_feature, Key p_keycode = Key::KEY_NONE, bool p_physical = false);
    void ED_SHORTCUT_OVERRIDE_ARRAY(const String& p_path, const String& p_feature, const PackedInt32Array& p_keycodes, bool p_physical);
    Ref<Shortcut> ED_GET_SHORTCUT(const String& p_path);

    #if GODOT_VERSION >= 0x040600
    #define ED_IS_SHORTCUT(p_name, p_ev) (EI->get_editor_settings()->is_shortcut(p_name, p_ev))
    #define ED_HAS_SHORTCUT(p_name) (EI->get_editor_settings()->has_shortcut(p_name))
    #else
    #define ED_IS_SHORTCUT(p_name, p_ev) (false)
    #define ED_HAS_SHORTCUT(p_name) (false)
    #endif
}

#endif // ORCHESTRATOR_CORE_GODOT_EDITOR_SETTINGS_H
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

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/theme.hpp>

using namespace godot;

/// Forward declarations
class OrchestratorEditorThemeBuilder;

/// Theme management and coordinator between plugin settings and Godot editor
class OrchestratorEditorThemeManager : public Object {
    GDCLASS(OrchestratorEditorThemeManager, Object);

    const String DEFAULT_THEME = "Default";
    const String CUSTOM_THEME = "Custom";

    OrchestratorEditorThemeBuilder* _builder = nullptr;
    String _current_theme;
    bool _applying_preset = false;
    bool _applying_theme = false;
    bool _custom_flip_pending = false;

    void _project_settings_changed();
    void _editor_settings_changed();
    void _deferred_check_custom();

    bool _has_user_color_overrides() const;

    void _apply_preset(const String& p_theme_name, bool p_full_reset = false);
    void _write_preset_color(const String& p_key, const Color& p_value, bool p_full_reset = false);

protected:
    static void _bind_methods();

public:
    Ref<Theme> get_theme() const;

    void theme_changed();

    OrchestratorEditorThemeManager();
    ~OrchestratorEditorThemeManager() override;
};

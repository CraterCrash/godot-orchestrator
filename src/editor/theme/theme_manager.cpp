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
#include "editor/theme/theme_manager.h"

#include "common/callable_lambda.h"
#include "common/macros.h"
#include "common/scene_utils.h"
#include "common/settings.h"
#include "editor/theme/theme_builder.h"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

bool OrchestratorEditorThemeManager::_has_user_color_overrides() const {
    ProjectSettings* ps = ProjectSettings::get_singleton();

    for (const String& key : OrchestratorSettings::get_singleton()->get_settings_name_list()) {
        // Only check keys that _apply_preset manages, anything else (data connection, colors, etc.)
        // is user-owned and must not trigger the auto-Custom flip.
        const bool is_node_color = key.begins_with("orchestrator/interface/theme/node_colors/");
        const bool is_structural = key == "orchestrator/interface/theme/nodes/background_color"
            || key == "orchestrator/interface/theme/nodes/border_color"
            || key == "orchestrator/interface/theme/nodes/selected_border_color"
            || key == "orchestrator/interface/theme/nodes/border_radius"
            || key == "orchestrator/interface/theme/nodes/border_width";
        const bool is_exec = key == "orchestrator/interface/theme/connection_colors/execution";

        if (!is_node_color && !is_structural && !is_exec) {
            continue;
        }

        if (!ps->property_can_revert(key)) {
            continue;
        }

        // property_can_revert() uses exact Variant inequality, which can trip on sub-epsilon
        // float drift. Re-check with approximate comparison so only genuine user edits flip
        // the theme to "Custom".
        const Variant current = ps->get(key);
        const Variant initial = ps->property_get_revert(key);

        if (current.get_type() == Variant::COLOR && initial.get_type() == Variant::COLOR) {
            if (!Color(current).is_equal_approx(initial)) {
                return true;
            }
        } else if (current.get_type() == Variant::FLOAT && initial.get_type() == Variant::FLOAT) {
            if (!UtilityFunctions::is_equal_approx(current, initial)) {
                return true;
            }
        } else if (current != initial) {
            return true;
        }
    }
    return false;
}

void OrchestratorEditorThemeManager::_project_settings_changed() {
    if (_applying_preset) {
        return;
    }

    const String theme = ORCHESTRATOR_GET("interface/theme/color_theme", DEFAULT_THEME);
    if (theme != _current_theme) {
        _apply_preset(theme, true);
    } else if (theme != CUSTOM_THEME && _has_user_color_overrides() && !_custom_flip_pending) {
        // Defer to end-of-frame: any in-progress auto-adaptation
        // (e.g. theme_changed() then _apply_preset("Default", true)) may not have run yet,
        // so property_can_revert could be spuriously true right now. Re-check after all
        // handlers have settled.
        _custom_flip_pending = true;
        callable_mp_this(_deferred_check_custom).call_deferred();
    }

    _builder->queue_rebuild();
}

void OrchestratorEditorThemeManager::_deferred_check_custom() {
    _custom_flip_pending = false;
    if (_applying_preset || _current_theme == CUSTOM_THEME) {
        return;
    }
    if (_has_user_color_overrides()) {
        _applying_preset = true;
        ProjectSettings::get_singleton()->set("orchestrator/interface/theme/color_theme", CUSTOM_THEME);
        _current_theme = CUSTOM_THEME;
        _applying_preset = false;
    }
}

void OrchestratorEditorThemeManager::_editor_settings_changed() {
    if (_current_theme == DEFAULT_THEME) {
        _apply_preset(DEFAULT_THEME, false);
        _builder->queue_rebuild();
    }
}

void OrchestratorEditorThemeManager::_apply_preset(const String& p_theme_name, bool p_full_reset) {
    _applying_preset = true;
    _current_theme = p_theme_name;

    if (p_theme_name == DEFAULT_THEME) {
        const Color accent = SceneUtils::get_editor_color("accent_color");
        const Color base = SceneUtils::get_editor_color("base_color");
        const bool is_dark = base.get_luminance() < 0.5f;

        _write_preset_color("interface/theme/nodes/selected_border_color", accent, p_full_reset);
        _write_preset_color("interface/theme/nodes/background_color", is_dark ? base.darkened(0.15f) : base.lightened(0.05f), p_full_reset);
        _write_preset_color("interface/theme/nodes/border_color", is_dark ? base.darkened(0.35f) : base.darkened(0.15f), p_full_reset);

        if (p_full_reset) {
            String prefix = "orchestrator/interface/theme/nodes/";
            ProjectSettings::get_singleton()->set_initial_value(prefix + "border_radius", 4.0);
            ProjectSettings::get_singleton()->set(prefix + "border_radius", 4.0);
            ProjectSettings::get_singleton()->set_initial_value(prefix + "border_width", 2.0);
            ProjectSettings::get_singleton()->set(prefix + "border_width", 2.0);
        }

        // Category colors: derive from Legacy canonical hues, scale s/v for mode
        const float s = is_dark ? 0.65f : 0.50f;
        const float v = is_dark ? 0.45f : 0.72f;

        struct NodeColorEntry {
            String key;
            Color legacy;
        };

        static const NodeColorEntry node_colors[] = {
            { "interface/theme/node_colors/constants_and_literals",      Color(0.27f, 0.39f, 0.20f) },
            { "interface/theme/node_colors/dialogue",                    Color(0.31f, 0.43f, 0.83f) },
            { "interface/theme/node_colors/events",                      Color(0.46f, 0.00f, 0.00f) },
            { "interface/theme/node_colors/flow_control",                Color(0.13f, 0.26f, 0.27f) },
            { "interface/theme/node_colors/function_call",               Color(0.00f, 0.20f, 0.39f) },
            { "interface/theme/node_colors/orchestration_function_call", Color(0.00f, 0.31f, 0.60f) },
            { "interface/theme/node_colors/other_script_function_call",  Color(0.02f, 0.34f, 0.50f) },
            { "interface/theme/node_colors/pure_function_call",          Color(0.13f, 0.30f, 0.11f) },
            { "interface/theme/node_colors/function_terminator",         Color(0.29f, 0.00f, 0.50f) },
            { "interface/theme/node_colors/function_result",             Color(1.00f, 0.64f, 0.40f) },
            { "interface/theme/node_colors/math_operations",             Color(0.25f, 0.40f, 0.38f) },
            { "interface/theme/node_colors/memory",                      Color(0.35f, 0.33f, 0.13f) },
            { "interface/theme/node_colors/properties",                  Color(0.46f, 0.28f, 0.17f) },
            { "interface/theme/node_colors/resources",                   Color(0.26f, 0.27f, 0.35f) },
            { "interface/theme/node_colors/scene",                       Color(0.20f, 0.14f, 0.28f) },
            { "interface/theme/node_colors/signals",                     Color(0.35f, 0.00f, 0.00f) },
            { "interface/theme/node_colors/variable",                    Color(0.25f, 0.17f, 0.24f) },
            { "interface/theme/node_colors/type_cast",                   Color(0.00f, 0.22f, 0.20f) },
        };

        for (const NodeColorEntry& entry : node_colors) {
            // Preserve saturation=0 for fully desaturated legacy colors (avoids hue artifacts)
            const float entry_s = (entry.legacy.get_s() < 0.01f) ? 0.0f : s;
            _write_preset_color(entry.key, Color::from_hsv(entry.legacy.get_h(), entry_s, v), p_full_reset);
        }

        // Comment is always a neutral gray, no hue derivation
        const float gray = is_dark ? 0.35f : 0.65f;
        _write_preset_color("interface/theme/node_colors/comment", Color(gray, gray, gray), p_full_reset);

        // Handle execution control flow connections, which are white by default
        const Color font_color = SceneUtils::get_editor_color("font_color");
        _write_preset_color("interface/theme/connection_colors/execution", font_color, p_full_reset);
    }

    _applying_preset = false;
}

void OrchestratorEditorThemeManager::_write_preset_color(const String& p_key, const Color& p_value, bool p_full_reset) {
    const String key = p_key.begins_with("orchestrator/") ? p_key : vformat("orchestrator/%s", p_key);

    // All preset color properties are NO_ALPHA; force opaque so initial_value and value
    // agree on the alpha channel and property_can_revert doesn't spuriously report an override.
    Color value = p_value;
    value.a = 1.0f;

    if (!p_full_reset && ProjectSettings::get_singleton()->property_can_revert(key)) {
        return;
    }

    ProjectSettings::get_singleton()->set_initial_value(key, value);
    ProjectSettings::get_singleton()->set(key, value);
}

Ref<Theme> OrchestratorEditorThemeManager::get_theme() const {
    return _builder->get_theme();
}

void OrchestratorEditorThemeManager::theme_changed() {
    if (_applying_theme || _current_theme != DEFAULT_THEME) {
        return;
    }

    _apply_preset(DEFAULT_THEME, true);
    _builder->queue_rebuild();
}

void OrchestratorEditorThemeManager::_bind_methods() {
    ADD_SIGNAL(MethodInfo("theme_rebuilt"));
}

OrchestratorEditorThemeManager::OrchestratorEditorThemeManager() {
    _current_theme = ORCHESTRATOR_GET("interface/theme/color_theme", DEFAULT_THEME);
    _apply_preset(_current_theme, false);

    _builder = memnew(OrchestratorEditorThemeBuilder);
    _builder->connect("theme_rebuilt", callable_mp_lambda(this, [this] {
        _applying_theme = true;
        emit_signal("theme_rebuilt");
        _applying_theme = false;
    }));

    ProjectSettings::get_singleton()->connect("settings_changed", callable_mp_this(_project_settings_changed));
    EI->get_editor_settings()->connect("settings_changed", callable_mp_this(_editor_settings_changed));
}

OrchestratorEditorThemeManager::~OrchestratorEditorThemeManager() {
    memdelete(_builder);
}

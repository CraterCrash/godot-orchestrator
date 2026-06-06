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
#include "common/settings.h"

#include "common/dictionary_utils.h"
#include "common/macros.h"
#include "common/variant_utils.h"
#include "script/script_warning.h"

#include <godot_cpp/classes/project_settings.hpp>

#define GLOBAL_DEF(m_var, m_value) _define(m_var, m_value)
#define GLOBAL_DEF_RST(m_var, m_value) _define(m_var, m_value, true)
#define GLOBAL_DEF_NOVAL(m_var, m_value) _define(m_var, m_value, false, true)

#define GLOBAL_DEF_BASIC(m_var, m_value) _define(m_var, m_value, false, false, true)
#define GLOBAL_DEF_RST_BASIC(m_var, m_value) _define(m_var, m_value, true, false, true)
#define GLOBAL_DEF_NOVAL_BASIC(m_var, m_value) _define(m_var, m_value, false, true, true)
#define GLOBAL_DEF_RST_NOVAL_BASIC(m_var, m_value) _define(m_var, m_value, true, true, true)

#define GLOBAL_DEF_INTERNAL(m_var, m_value) _define(m_var, m_value, false, false, false, true)

OrchestratorSettings* OrchestratorSettings::_singleton = nullptr;

void OrchestratorSettings::_project_settings_changed() {
    if (_applying_preset) {
        return;
    }

    const String theme = get_setting("theme/color_theme", "Default");
    if (theme != _current_theme) {
        _current_theme = theme;
        _apply_color_theme_preset(theme);
    }
}

void OrchestratorSettings::_apply_color_theme_preset(const String& p_theme) {
    _applying_preset = true;

    if (p_theme == "Default" || p_theme == "Custom") {
        _applying_preset = false;
        return;
    }

    // Structural: explicit per-mode values, hue derivation doesn't apply here
    struct StructuralEntry { String key; Color legacy; Color light; Color dark; };
    static const StructuralEntry structural[] = {
        { "theme/nodes/background_color",      Color(0.09f, 0.11f, 0.13f), Color(0.90f, 0.90f, 0.92f), Color(0.05f, 0.06f, 0.07f) },
        { "theme/nodes/border_color",          Color(0.05f, 0.06f, 0.08f), Color(0.72f, 0.74f, 0.77f), Color(0.02f, 0.03f, 0.04f) },
        { "theme/nodes/selected_border_color", Color(0.68f, 0.43f, 0.09f), Color(0.60f, 0.35f, 0.05f), Color(0.75f, 0.50f, 0.12f) },
    };

    for (const StructuralEntry& e : structural) {
        const Color value = (p_theme == "Light") ? e.light : (p_theme == "Dark") ? e.dark : e.legacy;
        ProjectSettings::get_singleton()->set_initial_value(_make_key(e.key), value);
        ProjectSettings::get_singleton()->set(_make_key(e.key), value);
    }

    // Category node colors: single canonical (legacy) hue, s/v scaled per mode
    struct CategoryEntry { String key; Color legacy; };
    static const CategoryEntry categories[] = {
        { "theme/node_colors/constants_and_literals",      Color(0.27f, 0.39f, 0.20f) },
        { "theme/node_colors/dialogue",                    Color(0.31f, 0.43f, 0.83f) },
        { "theme/node_colors/events",                      Color(0.46f, 0.00f, 0.00f) },
        { "theme/node_colors/flow_control",                Color(0.13f, 0.26f, 0.27f) },
        { "theme/node_colors/function_call",               Color(0.00f, 0.20f, 0.39f) },
        { "theme/node_colors/orchestration_function_call", Color(0.00f, 0.31f, 0.60f) },
        { "theme/node_colors/other_script_function_call",  Color(0.02f, 0.34f, 0.50f) },
        { "theme/node_colors/pure_function_call",          Color(0.13f, 0.30f, 0.11f) },
        { "theme/node_colors/function_terminator",         Color(0.29f, 0.00f, 0.50f) },
        { "theme/node_colors/function_result",             Color(1.00f, 0.64f, 0.40f) },
        { "theme/node_colors/math_operations",             Color(0.25f, 0.40f, 0.38f) },
        { "theme/node_colors/memory",                      Color(0.35f, 0.33f, 0.13f) },
        { "theme/node_colors/properties",                  Color(0.46f, 0.28f, 0.17f) },
        { "theme/node_colors/resources",                   Color(0.26f, 0.27f, 0.35f) },
        { "theme/node_colors/scene",                       Color(0.20f, 0.14f, 0.28f) },
        { "theme/node_colors/signals",                     Color(0.35f, 0.00f, 0.00f) },
        { "theme/node_colors/variable",                    Color(0.25f, 0.17f, 0.24f) },
        { "theme/node_colors/type_cast",                   Color(0.00f, 0.22f, 0.20f) },
        { "theme/node_colors/comment",                     Color(0.40f, 0.40f, 0.40f) },
    };

    // Fixed s/v targets per mode, gives a consistent designed palette
    const float target_s = (p_theme == "Light") ? 0.50f : (p_theme == "Dark") ? 0.65f : 1.0f;
    const float target_v = (p_theme == "Light") ? 0.72f : (p_theme == "Dark") ? 0.45f : 1.0f;

    for (const CategoryEntry& e : categories) {
        Color value;
        if (p_theme == "Legacy") {
            value = e.legacy;
        } else {
            // Preserve saturation=0 for neutrals (e.g. comment gray), avoid hue artifacts
            const float s = (e.legacy.get_s() < 0.01f) ? 0.0f : target_s;
            const float v = (e.legacy.get_s() < 0.01f)
                ? ((p_theme == "Light")
                    ? 1.0f - e.legacy.get_v()
                    : e.legacy.get_v())
                : target_v;

            value = Color::from_hsv(e.legacy.get_h(), s, v);
        }

        ProjectSettings::get_singleton()->set_initial_value(_make_key(e.key), value);
        ProjectSettings::get_singleton()->set(_make_key(e.key), value);
    }

    _applying_preset = false;
}

Variant OrchestratorSettings::_define(const String& p_name, const Variant& p_default, bool p_restart_if_changed, bool p_ignore_in_docs, bool p_basic, bool p_internal) {
    const String name = _make_key(p_name);

    if (!ProjectSettings::get_singleton()->has_setting(name)) {
        ProjectSettings::get_singleton()->set(name, p_default);
    }

    if (!_registered_names.has(name)) {
        _registered_names.push_back(name);
    }

    const Variant ret = ProjectSettings::get_singleton()->get(name);

    ProjectSettings::get_singleton()->set_initial_value(name, p_default);
    ProjectSettings::get_singleton()->set_order(name, _builtin_order++);
    ProjectSettings::get_singleton()->set_as_basic(name, p_basic);
    ProjectSettings::get_singleton()->set_restart_if_changed(name, p_restart_if_changed);
    ProjectSettings::get_singleton()->set_as_internal(name, p_internal);
    // todo: set_ignore_value_in_docs

    return ret;
}

Variant OrchestratorSettings::_define(const PropertyInfo& p_property, const Variant& p_default, bool p_restart_if_changed, bool p_ignore_in_docs, bool p_basic, bool p_internal) {
    PropertyInfo property = p_property;
    property.name = _make_key(property.name);

    Variant ret = _define(property.name, p_default, p_restart_if_changed, p_ignore_in_docs, p_basic, p_internal);
    ProjectSettings::get_singleton()->add_property_info(DictionaryUtils::from_property(property, true));
    return ret;
}

void OrchestratorSettings::_rename(const String& p_old_name, const String& p_new_name) {
    const String old_name = _make_key(p_old_name);
    const String new_name = _make_key(p_new_name);

    if (ProjectSettings::get_singleton()->has_setting(old_name)) {
        if (ProjectSettings::get_singleton()->has_setting(new_name)) {
            ERR_FAIL_MSG("Setting '" + new_name + "' already exists, cannot rename '" + old_name + "'.");
        }

        Variant value = ProjectSettings::get_singleton()->get(old_name);

        ProjectSettings::get_singleton()->set(new_name, value);
        ProjectSettings::get_singleton()->clear(old_name);
    }
}

String OrchestratorSettings::_make_key(const String& p_key) {
    if (p_key.begins_with("orchestrator/")) {
        return p_key;
    }
    return vformat("orchestrator/%s", p_key);
}

void OrchestratorSettings::_initialize_settings() {
    // Settings
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::STRING, "settings/default_type", PROPERTY_HINT_RESOURCE_TYPE, "Object"), "Node");
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::STRING, "settings/storage_format", PROPERTY_HINT_ENUM, "Text,Binary"), "Text");
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "settings/updates/notify_about_preview_releases"), false);

    // Editor
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "editor/actions_menu/center_on_mouse"), true);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "editor/actions_menu/close_on_focus_lost"), false);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "editor/actions_menu/prefer_properties_over_methods"), false);

    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "editor/components_panel/show_function_friendly_names"), true);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "editor/components_panel/show_graph_friendly_names"), true);

    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "editor/graph/confirm_on_delete"), true);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "editor/graph/disconnect_control_flow_when_dragged"), true);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "editor/graph/grid_enabled"), true);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::STRING, "editor/graph/grid_pattern", PROPERTY_HINT_ENUM, "Dots,Lines"), "Dots");
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "editor/graph/grid_snapping_enabled"), true);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "editor/graph/show_advanced_tooltips"), false);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "editor/graph/show_autowire_selection_dialog"), true);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "editor/graph/show_minimap"), false);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "editor/graph/show_arrange_button"), false);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "editor/graph/show_overlay_action_tooltips"), true);

    GLOBAL_DEF_BASIC(PropertyInfo(Variant::STRING, "editor/graph_nodes/connection_hotzone_scale", PROPERTY_HINT_ENUM, "100%,150%,200%"), "100%");
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "editor/graph_nodes/highlight_selected_connections"), false);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "editor/graph_nodes/resizable_by_default"), false);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "editor/graph_nodes/resize_to_content"), false);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "editor/graph_nodes/show_type_icons"), true);

    // Debug
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::INT, "debug/settings/max_call_stack", PROPERTY_HINT_RANGE, "256,1024,256"), 1024);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::INT, "debug/settings/max_loop_iterations"), 1000000);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "debug/settings/always_track_call_stacks"), false);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "debug/settings/always_track_local_variables"), false);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "debug/settings/use_node_convergence"), true);

    #ifdef DEBUG_ENABLED
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "debug/warnings/enable"), true);
    for (int i = 0; i < static_cast<int>(OScriptWarning::WARNING_MAX); i++) {
        const OScriptWarning::Code code = static_cast<OScriptWarning::Code>(i);
        const Variant default_value = OScriptWarning::get_default_value(code);
        GLOBAL_DEF_BASIC(OScriptWarning::get_property_info(code), default_value);
    }
    #endif

    // Runtime
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::STRING, "runtime/dialogue/default_message_scene", PROPERTY_HINT_FILE, "*.tscn,*.scn"), "res://addons/orchestrator/scenes/dialogue_message.tscn");
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::STRING, "runtime/print_string/overlay_scale", PROPERTY_HINT_ENUM, "75%,100%,125%,150%,175%,200%,225%,250%,275%,300%,325%,350%,375%,400%"), "100%");

    // Theme
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::STRING, "theme/color_theme", PROPERTY_HINT_ENUM, "Default,Dark,Legacy,Custom"), "Default");

    struct TypeColor {
        Variant::Type type = Variant::NIL;
        Color color = Color(1, 1, 1);
    };

    Vector<TypeColor> type_colors;
    type_colors.push_back({ Variant::NIL, Color(0.41, 0.93, 0.74) });
    type_colors.push_back({ Variant::BOOL, Color(0.55, 0.65, 0.94) });
    type_colors.push_back({ Variant::INT, Color(0.59, 0.78, 0.94) });
    type_colors.push_back({ Variant::FLOAT, Color(0.38, 0.85, 0.96) });
    type_colors.push_back({ Variant::STRING, Color(0.42, 0.65, 0.93) });
    type_colors.push_back({ Variant::STRING_NAME, Color(0.42, 0.65, 0.93) });
    type_colors.push_back({ Variant::RECT2, Color(0.95, 0.57, 0.65) });
    type_colors.push_back({ Variant::RECT2I, Color(0.95, 0.57, 0.65) });
    type_colors.push_back({ Variant::VECTOR2, Color(0.74, 0.57, 0.95) });
    type_colors.push_back({ Variant::VECTOR2I, Color(0.74, 0.57, 0.95) });
    type_colors.push_back({ Variant::VECTOR3, Color(0.84, 0.49, 0.93) });
    type_colors.push_back({ Variant::VECTOR3I, Color(0.84, 0.49, 0.93) });
    type_colors.push_back({ Variant::VECTOR4, Color(0.84, 0.49, 0.94) });
    type_colors.push_back({ Variant::VECTOR4I, Color(0.84, 0.49, 0.94) });
    type_colors.push_back({ Variant::TRANSFORM2D, Color(0.77, 0.83, 0.41) });
    type_colors.push_back({ Variant::TRANSFORM3D, Color(0.96, 0.66, 0.43) });
    type_colors.push_back({ Variant::PLANE, Color(0.97, 0.44, 0.44) });
    type_colors.push_back({ Variant::QUATERNION, Color(0.93, 0.41, 0.64) });
    type_colors.push_back({ Variant::AABB, Color(0.93, 0.47, 0.57) });
    type_colors.push_back({ Variant::BASIS, Color(0.89, 0.93, 0.41) });
    type_colors.push_back({ Variant::PROJECTION, Color(0.302, 0.655, 0.271) });
    type_colors.push_back({ Variant::COLOR, Color(0.62, 1.00, 0.44) });
    type_colors.push_back({ Variant::NODE_PATH, Color(0.51, 0.58, 0.93) });
    type_colors.push_back({ Variant::RID, Color(0.41, 0.93, 0.60) });
    type_colors.push_back({ Variant::OBJECT, Color(0.47, 0.95, 0.91) });
    type_colors.push_back({ Variant::DICTIONARY, Color(0.47, 0.93, 0.69) });
    type_colors.push_back({ Variant::ARRAY, Color(0.88, 0.88, 0.88) });
    type_colors.push_back({ Variant::CALLABLE, Color(0.47, 0.95, 0.91) });
    type_colors.push_back({ Variant::SIGNAL, Color(1.0, 0.0, 0.0) });
    type_colors.push_back({ Variant::PACKED_BYTE_ARRAY, Color(0.55, 0.65, 0.94) });
    type_colors.push_back({ Variant::PACKED_STRING_ARRAY, Color(0.42, 0.65, 0.93) });
    type_colors.push_back({ Variant::PACKED_INT32_ARRAY, Color(0.59, 0.78, 0.94) });
    type_colors.push_back({ Variant::PACKED_INT64_ARRAY, Color(0.59, 0.78, 0.94) });
    type_colors.push_back({ Variant::PACKED_FLOAT32_ARRAY, Color(0.38, 0.85, 0.96) });
    type_colors.push_back({ Variant::PACKED_FLOAT64_ARRAY, Color(0.38, 0.85, 0.96) });
    type_colors.push_back({ Variant::PACKED_VECTOR2_ARRAY, Color(0.74, 0.57, 0.95) });
    type_colors.push_back({ Variant::PACKED_VECTOR3_ARRAY, Color(0.84, 0.49, 0.93) });
    type_colors.push_back({ Variant::PACKED_VECTOR4_ARRAY, Color(0.84, 0.49, 0.94) });
    type_colors.push_back({ Variant::PACKED_COLOR_ARRAY, Color(0.62, 1.00, 0.44) });

    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "theme/connection_colors/execution", PROPERTY_HINT_COLOR_NO_ALPHA), Color(1.0, 1.0, 1.0));
    for (const TypeColor& type_color: type_colors) {
        GLOBAL_DEF_BASIC(PropertyInfo(
            Variant::COLOR,
            vformat("theme/connection_colors/%s", VariantUtils::get_friendly_type_name(type_color.type, true).to_lower().replace(" ", "_")),
            PROPERTY_HINT_COLOR_NO_ALPHA),
            type_color.color);
    }

    // Check and handle new variant types not yet mapped
    if (type_colors.size() != Variant::VARIANT_MAX) {
        for (int i = 0; i < Variant::VARIANT_MAX; i++) {
            const Variant::Type type = VariantUtils::to_type(i);

            bool found = false;
            for (const TypeColor& color : type_colors) {
                if (color.type == type) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                GLOBAL_DEF_BASIC(PropertyInfo(
                Variant::COLOR,
                vformat("theme/connection_colors/%s", VariantUtils::get_friendly_type_name(type, true).to_lower().replace(" ", "_")),
                PROPERTY_HINT_COLOR_NO_ALPHA),
                Color(0.5, 0.5, 0.5));
            }
        }
    }

    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "theme/nodes/background_color", PROPERTY_HINT_COLOR_NO_ALPHA), Color::html("#191d23"));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "theme/nodes/border_color", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.059f, 0.067f, 0.082f));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "theme/nodes/selected_border_color", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.68f, 0.44f, 0.09f));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::FLOAT, "theme/nodes/border_radius", PROPERTY_HINT_RANGE, "0,16,1"), 4.0);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::FLOAT, "theme/nodes/border_width", PROPERTY_HINT_RANGE, "0.8,10,.2"), 2.0);

    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "theme/node_colors/constants_and_literals", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.271, 0.392, 0.2));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "theme/node_colors/dialogue", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.318, 0.435, 0.839));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "theme/node_colors/events", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.467, 0.0, 0.0));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "theme/node_colors/flow_control", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.132, 0.258, 0.266));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "theme/node_colors/function_call", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.0, 0.2, 0.396));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "theme/node_colors/orchestration_function_call", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.0, 0.316, 0.601));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "theme/node_colors/other_script_function_call", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.027, 0.341, 0.504));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "theme/node_colors/pure_function_call", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.133, 0.302, 0.114));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "theme/node_colors/function_terminator", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.294, 0.0, 0.506));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "theme/node_colors/function_result", PROPERTY_HINT_COLOR_NO_ALPHA), Color(1.0, 0.65, 0.4));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "theme/node_colors/math_operations", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.259, 0.408, 0.384));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "theme/node_colors/memory", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.351, .339, .133));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "theme/node_colors/properties", PROPERTY_HINT_COLOR_NO_ALPHA), Color(.467, 0.28, 0.175));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "theme/node_colors/resources", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.263, 0.275, 0.359));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "theme/node_colors/scene", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.208, 0.141, 0.282));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "theme/node_colors/signals", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.353, 0.0, 0.0));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "theme/node_colors/variable", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.259, 0.177, 0.249));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "theme/node_colors/type_cast", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.009, 0.221, 0.203));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "theme/node_colors/comment", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.4, 0.4, 0.4));
    
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "theme/tool_scripts/background_color"), Color(1.f, 1.f, 0.f, 0.10f));
}

void OrchestratorSettings::_rename_settings() {
    _rename("settings/notify_about_pre-releases", "settings/updates/notify_about_preview_releases");
    _rename("settings/runtime/max_call_stack", "debug/settings/max_call_stack");
    _rename("settings/runtime/max_loop_iterations", "debug/settings/max_loop_iterations");
    _rename("settings/runtime/use_node_convergence", "debug/settings/use_node_convergence");
    _rename("settings/dialogue/default_message_scene", "runtime/dialogue/default_message_scene");
    _rename("settings/runtime/print_string_scale", "runtime/print_string/overlay_scale");
    _rename("ui/actions_menu/center_on_mouse", "editor/actions_menu/center_on_mouse");
    _rename("ui/actions_menu/close_on_focus_lost", "editor/actions_menu/close_on_focus_lost");
    _rename("ui/actions_menu/prefer_properties_over_methods", "editor/actions_menu/prefer_properties_over_methods");
    _rename("ui/components_panel/show_graph_friendly_names", "editor/components_panel/show_graph_friendly_names");
    _rename("ui/components_panel/show_function_friendly_names", "editor/components_panel/show_function_friendly_names");
    _rename("ui/graph/confirm_on_delete", "editor/graph/confirm_on_delete");
    _rename("ui/graph/disconnect_control_flow_when_dragged", "editor/graph/disconnect_control_flow_when_dragged");
    _rename("ui/graph/grid_enabled", "editor/graph/grid_enabled");
    _rename("ui/graph/grid_pattern", "editor/graph/grid_pattern");
    _rename("ui/graph/show_advanced_tooltips", "editor/graph/show_advanced_tooltips");
    _rename("ui/graph/show_autowire_selection_dialog", "editor/graph/show_autowire_selection_dialog");
    _rename("ui/graph/show_minimap", "editor/graph/show_minimap");
    _rename("ui/graph/show_arrange_button", "editor/graph/show_arrange_button");
    _rename("ui/graph/show_overlay_action_tooltips", "editor/graph/show_overlay_action_tooltips");
    _rename("ui/nodes/connection_hotzone_scale", "editor/graph_nodes/connection_hotzone_scale");
    _rename("ui/nodes/show_type_icons", "editor/graph_nodes/show_type_icons");
    _rename("ui/nodes/resizable_by_default", "editor/graph_nodes/resizable_by_default");
    _rename("ui/nodes/resize_to_content", "editor/graph_nodes/resize_to_content");
    _rename("ui/nodes/highlight_selected_connections", "editor/graph_nodes/highlight_selected_connections");
    _rename("ui/graph/tool_script_background_color", "theme/tool_scripts/background_color");
    _rename("ui/connection_colors/execution", "theme/connection_colors/execution");
    _rename("ui/connection_colors/any", "theme/connection_colors/any");
    _rename("ui/connection_colors/boolean", "theme/connection_colors/boolean");
    _rename("ui/connection_colors/integer", "theme/connection_colors/integer");
    _rename("ui/connection_colors/float", "theme/connection_colors/float");
    _rename("ui/connection_colors/string", "theme/connection_colors/string");
    _rename("ui/connection_colors/string name", "theme/connection_colors/string name");
    _rename("ui/connection_colors/rect2", "theme/connection_colors/rect2");
    _rename("ui/connection_colors/rect2i", "theme/connection_colors/rect2i");
    _rename("ui/connection_colors/vector2", "theme/connection_colors/vector2");
    _rename("ui/connection_colors/vector2i", "theme/connection_colors/vector2i");
    _rename("ui/connection_colors/vector3", "theme/connection_colors/vector3");
    _rename("ui/connection_colors/vector3i", "theme/connection_colors/vector3i");
    _rename("ui/connection_colors/vector4", "theme/connection_colors/vector4");
    _rename("ui/connection_colors/vector4i", "theme/connection_colors/vector4i");
    _rename("ui/connection_colors/transform2d", "theme/connection_colors/transform2d");
    _rename("ui/connection_colors/transform3d", "theme/connection_colors/transform3d");
    _rename("ui/connection_colors/plane", "theme/connection_colors/plane");
    _rename("ui/connection_colors/quaternion", "theme/connection_colors/quaternion");
    _rename("ui/connection_colors/aabb", "theme/connection_colors/aabb");
    _rename("ui/connection_colors/basis", "theme/connection_colors/basis");
    _rename("ui/connection_colors/projection", "theme/connection_colors/projection");
    _rename("ui/connection_colors/color", "theme/connection_colors/color");
    _rename("ui/connection_colors/nodepath", "theme/connection_colors/nodepath");
    _rename("ui/connection_colors/rid", "theme/connection_colors/rid");
    _rename("ui/connection_colors/object", "theme/connection_colors/object");
    _rename("ui/connection_colors/dictionary", "theme/connection_colors/dictionary");
    _rename("ui/connection_colors/array", "theme/connection_colors/array");
    _rename("ui/connection_colors/callable", "theme/connection_colors/callable");
    _rename("ui/connection_colors/packed byte array", "theme/connection_colors/packed byte array");
    _rename("ui/connection_colors/packed string array", "theme/connection_colors/packed string array");
    _rename("ui/connection_colors/packed int32 array", "theme/connection_colors/packed int32 array");
    _rename("ui/connection_colors/packed int64 array", "theme/connection_colors/packed int64 array");
    _rename("ui/connection_colors/packed float32 array", "theme/connection_colors/packed float32 array");
    _rename("ui/connection_colors/packed float64 array", "theme/connection_colors/packed float64 array");
    _rename("ui/connection_colors/packed vector2 array", "theme/connection_colors/packed vector2 array");
    _rename("ui/connection_colors/packed vector3 array", "theme/connection_colors/packed vector3 array");
    _rename("ui/connection_colors/packed vector4 array", "theme/connection_colors/packed vector4 array");
    _rename("ui/connection_colors/packed color array", "theme/connection_colors/packed color array");
    _rename("ui/nodes/background_color", "theme/nodes/background_color");
    _rename("ui/nodes/border_color", "theme/nodes/border_color");
    _rename("ui/nodes/border_selected_color", "theme/nodes/selected_border_color");
    _rename("ui/nodes/border_radius", "theme/nodes/border_radius");
    _rename("ui/nodes/border_width", "theme/nodes/border_width");
    _rename("ui/node_colors/constants_and_literals", "theme/node_colors/constants_and_literals");
    _rename("ui/node_colors/dialogue", "theme/node_colors/dialogue");
    _rename("ui/node_colors/events", "theme/node_colors/events");
    _rename("ui/node_colors/flow_control", "theme/node_colors/flow_control");
    _rename("ui/node_colors/function_call", "theme/node_colors/function_call");
    _rename("ui/node_colors/orchestration_function_call", "theme/node_colors/orchestration_function_call");
    _rename("ui/node_colors/other_script_function_call", "theme/node_colors/other_script_function_call");
    _rename("ui/node_colors/pure_function_call", "theme/node_colors/pure_function_call");
    _rename("ui/node_colors/function_terminator", "theme/node_colors/function_terminator");
    _rename("ui/node_colors/function_result", "theme/node_colors/function_result");
    _rename("ui/node_colors/math_operations", "theme/node_colors/math_operations");
    _rename("ui/node_colors/memory", "theme/node_colors/memory");
    _rename("ui/node_colors/properties", "theme/node_colors/properties");
    _rename("ui/node_colors/resources", "theme/node_colors/resources");
    _rename("ui/node_colors/scene", "theme/node_colors/scene");
    _rename("ui/node_colors/signals", "theme/node_colors/signals");
    _rename("ui/node_colors/variable", "theme/node_colors/variable");
    _rename("ui/node_colors/type_cast", "theme/node_colors/type_cast");
    _rename("ui/node_colors/comment", "theme/node_colors/comment");
}

void OrchestratorSettings::_remove_deprecated_settings() {
    PackedStringArray deprecated_keys;

    // Orchestrator v1
    deprecated_keys.push_back("run/test_scene");
    deprecated_keys.push_back("nodes/colors/backgrond");
    deprecated_keys.push_back("nodes/colors/data");
    deprecated_keys.push_back("nodes/color/flow_control");
    deprecated_keys.push_back("nodes/colors/logic");
    deprecated_keys.push_back("nodes/colors/terminal");
    deprecated_keys.push_back("nodes/colors/utility");
    deprecated_keys.push_back("nodes/colors/custom");

    // Orchestrator v2
    deprecated_keys.push_back("ui/nodes/show_conversion_nodes");
    deprecated_keys.push_back("settings/save_copy_as_text_resource");
    deprecated_keys.push_back("settings/runtime/tickable");
    deprecated_keys.push_back("ui/graph/knot_selected_color");

    ProjectSettings* ps = ProjectSettings::get_singleton();

    // Remove settings that have been deprecated and no longer used.
    for (const String& deprecated_key : deprecated_keys) {
        const String key = _make_key(deprecated_key);
        if (ps->has_setting(key)) {
            ps->clear(key);
        }
    }
}

void OrchestratorSettings::create() {
    _singleton = memnew(OrchestratorSettings);
}

void OrchestratorSettings::destroy() {
    if (_singleton) {
        memdelete(_singleton);
        _singleton = nullptr;
    }
}

bool OrchestratorSettings::has_setting(const String& p_key) const {
    const String path = _make_key(p_key);
    const bool result = ProjectSettings::get_singleton()->has_setting(path);
    if (!result) {
        UtilityFunctions::print("Failed to find key ", path);
    }
    return result;
}

Variant OrchestratorSettings::get_setting(const String& p_key, const Variant& p_default_value) {
    return ProjectSettings::get_singleton()->get_setting(_make_key(p_key), p_default_value);
}

void OrchestratorSettings::set_setting(const String& p_key, const Variant& p_value) {
    ProjectSettings::get_singleton()->set_setting(_make_key(p_key), p_value);
}

PackedStringArray OrchestratorSettings::get_action_favorites() {
    return ProjectSettings::get_singleton()->get_setting(_make_key("settings/action_favorites"), PackedStringArray());
}

void OrchestratorSettings::add_action_favorite(const String& p_action_name) {
    ProjectSettings* ps = ProjectSettings::get_singleton();

    const String key = _make_key("orchestrator/settings/action_favorites");
    const PropertyInfo pi = PropertyInfo(Variant::PACKED_STRING_ARRAY, key);
    if (!ps->has_setting(key)) {
        ps->set_setting(key, PackedStringArray());
        ps->set_initial_value(key, PackedStringArray());
        ps->add_property_info(DictionaryUtils::from_property(pi, true));
        ps->set_as_basic(key, false);
    }

    PackedStringArray actions = get_action_favorites();
    if (!actions.has(p_action_name)) {
        actions.push_back(p_action_name);
        ps->set_setting(key, actions);
        ps->save();
    }
}

void OrchestratorSettings::remove_action_favorite(const String& p_action_name) {
    ProjectSettings* ps = ProjectSettings::get_singleton();

    const String key = _make_key("orchestrator/settings/action_favorites");
    if (!ps->has_setting(key)) {
        return;
    }

    PackedStringArray actions = get_action_favorites();
    if (actions.has(p_action_name)) {
        actions.remove_at(actions.find(p_action_name));
        ps->set_setting(key, actions);
        ps->save();
    }
}

bool OrchestratorSettings::is_notify_about_prereleases() {
    return get_setting("orchestrator/settings/updates/notify_about_preview_releases", true);
}

void OrchestratorSettings::set_notify_prerelease_builds(bool p_notify_about_prereleases) {
    ProjectSettings* ps = ProjectSettings::get_singleton();

    const String key = "orchestrator/settings/updates/notify_about_preview_releases";
    if (!ps->has_setting(key)) {
        return;
    }

    ps->set_setting(key, p_notify_about_prereleases);
    ps->save();
}

void OrchestratorSettings::_bind_methods() {

}

OrchestratorSettings::OrchestratorSettings() {
    // Initialize the current preferred theme
    _current_theme = get_setting("theme/color_theme", "Default");

    _rename_settings();
    _initialize_settings();
    _remove_deprecated_settings();

    _apply_color_theme_preset(_current_theme);

    ProjectSettings::get_singleton()->connect("settings_changed", callable_mp_this(_project_settings_changed));
}

OrchestratorSettings::~OrchestratorSettings() {
}

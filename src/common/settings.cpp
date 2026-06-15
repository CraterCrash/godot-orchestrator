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

#include "api/extension_db.h"
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

    const String theme = get_setting("interface/theme/color_theme", "Default");
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
        { "interface/theme/nodes/background_color",      Color(0.09f, 0.11f, 0.13f), Color(0.90f, 0.90f, 0.92f), Color(0.05f, 0.06f, 0.07f) },
        { "interface/theme/nodes/border_color",          Color(0.05f, 0.06f, 0.08f), Color(0.72f, 0.74f, 0.77f), Color(0.02f, 0.03f, 0.04f) },
        { "interface/theme/nodes/selected_border_color", Color(0.68f, 0.43f, 0.09f), Color(0.60f, 0.35f, 0.05f), Color(0.75f, 0.50f, 0.12f) },
    };

    for (const StructuralEntry& e : structural) {
        const Color value = (p_theme == "Light") ? e.light : (p_theme == "Dark") ? e.dark : e.legacy;
        ProjectSettings::get_singleton()->set_initial_value(_make_key(e.key), value);
        ProjectSettings::get_singleton()->set(_make_key(e.key), value);
    }

    // Category node colors: single canonical (legacy) hue, s/v scaled per mode
    struct CategoryEntry { String key; Color legacy; };
    static const CategoryEntry categories[] = {
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
        { "interface/theme/node_colors/comment",                     Color(0.40f, 0.40f, 0.40f) },
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

Variant OrchestratorSettings::_define(const String& p_name, const Variant& p_default, bool p_restart_if_changed, bool p_ignore_in_docs, bool p_basic, bool p_internal, const PropertyInfo* p_property_info) {
    const String name = _make_key(p_name);
    const String logical_name = name.trim_prefix("orchestrator/");

    if (!ProjectSettings::get_singleton()->has_setting(name)) {
        ProjectSettings::get_singleton()->set(name, p_default);
    }

    bool already_registered = false;
    for (const PropertyInfo& existing : _editor_properties) {
        if (existing.name == logical_name) {
            already_registered = true;
            break;
        }
    }

    if (!already_registered) {
        // Retain an editor-facing entry keyed by the logical name (no "orchestrator/" prefix) so the
        // Orchestrator settings dialog can render it even though it's internal in ProjectSettings.
        // Built complete here (single push), since the backing Vector is copy-on-write.
        PropertyInfo pi;
        if (p_property_info) {
            pi.type = p_property_info->type;
            pi.hint = p_property_info->hint;
            pi.hint_string = p_property_info->hint_string;
            pi.class_name = p_property_info->class_name;
        } else {
            pi.type = p_default.get_type();
        }
        pi.name = logical_name;
        pi.usage = PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_STORAGE;
        if (p_basic) {
            pi.usage |= PROPERTY_USAGE_EDITOR_BASIC_SETTING;
        }
        _editor_properties.push_back(pi);
    }

    const Variant ret = ProjectSettings::get_singleton()->get(name);

    ProjectSettings::get_singleton()->set_initial_value(name, p_default);
    ProjectSettings::get_singleton()->set_order(name, _builtin_order++);
    ProjectSettings::get_singleton()->set_as_basic(name, p_basic);
    ProjectSettings::get_singleton()->set_restart_if_changed(name, p_restart_if_changed);
    ProjectSettings::get_singleton()->set_as_internal(name, true /*p_internal*/);
    // todo: set_ignore_value_in_docs

    return ret;
}

Variant OrchestratorSettings::_define(const PropertyInfo& p_property, const Variant& p_default, bool p_restart_if_changed, bool p_ignore_in_docs, bool p_basic, bool p_internal) {
    PropertyInfo property = p_property;
    property.name = _make_key(property.name);

    // Pass the original (logical-named) PropertyInfo so the editor-facing entry is captured with its
    // full hint metadata in a single push, rather than mutating the copy-on-write Vector in place.
    Variant ret = _define(property.name, p_default, p_restart_if_changed, p_ignore_in_docs, p_basic, p_internal, &p_property);
    ProjectSettings::get_singleton()->add_property_info(DictionaryUtils::from_property(property, true));
    return ret;
}

void OrchestratorSettings::_get_property_list(List<PropertyInfo>* r_list) const {
    for (const PropertyInfo& pi : _editor_properties) {
        r_list->push_back(pi);
    }
}

bool OrchestratorSettings::_get(const StringName& p_name, Variant& r_value) const {
    ProjectSettings* ps = ProjectSettings::get_singleton();
    const String key = _make_key(p_name);
    if (!ps->has_setting(key)) {
        return false;
    }
    r_value = ps->get(key);
    return true;
}

bool OrchestratorSettings::_set(const StringName& p_name, const Variant& p_value) {
    ProjectSettings* ps = ProjectSettings::get_singleton();
    const String key = _make_key(p_name);
    if (!ps->has_setting(key)) {
        return false;
    }
    ps->set(key, p_value);
    return true;
}

bool OrchestratorSettings::_property_can_revert(const StringName& p_name) const {
    return ProjectSettings::get_singleton()->property_can_revert(_make_key(p_name));
}

bool OrchestratorSettings::_property_get_revert(const StringName& p_name, Variant& r_value) const {
    r_value = ProjectSettings::get_singleton()->property_get_revert(_make_key(p_name));
    return true;
}

PackedStringArray OrchestratorSettings::get_settings_name_list() const {
    // Returns the full, "orchestrator/"-prefixed storage keys (callers, e.g. theme code, match on
    // that form). Derived from the registered editor properties, which hold the logical names.
    PackedStringArray names;
    for (const PropertyInfo& pi : _editor_properties) {
        names.push_back(_make_key(pi.name));
    }
    return names;
}

String OrchestratorSettings::get_property_description(const String& p_property) {
    return _editor_property_descriptions.has(p_property) ? _editor_property_descriptions[p_property] : String();
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
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::STRING, "editor/settings/default_type", PROPERTY_HINT_RESOURCE_TYPE, "Object"), "Node");
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::STRING, "editor/settings/storage_format", PROPERTY_HINT_ENUM, "Text,Binary"), "Text");
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "editor/settings/updates/notify_about_preview_releases"), false);

    GLOBAL_DEF_BASIC(PropertyInfo(Variant::FLOAT, "editor/behavior/files/autosave_interval_secs"), 0);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "editor/behavior/files/restore_scripts_on_load"), true);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "editor/behavior/files/auto_reload_scripts_on_external_change"), true);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "editor/behavior/files/auto_reload_and_parse_scripts_on_save"), true);

    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "editor/script_list/highlight_scene_scripts"), true);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::INT, "editor/script_list/sort_scripts_by", PROPERTY_HINT_ENUM, "None:2,Name:0,Path:1"), 0);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::INT, "editor/script_list/list_script_names_as", PROPERTY_HINT_ENUM, "Name,Parent Directory And Name,Full Path"), 0);

    GLOBAL_DEF_BASIC(PropertyInfo(Variant::FLOAT, "editor/validation/idle_parse_delay", PROPERTY_HINT_RANGE, "0.1,10,0.01"), 1.5);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::FLOAT, "editor/validation/idle_parse_delay_with_errors_found", PROPERTY_HINT_RANGE, "0.1,5,0.01"), 0.5);

    // Editor
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "interface/editor/actions_menu/center_on_mouse"), true);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "interface/editor/actions_menu/close_on_focus_lost"), false);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "interface/editor/actions_menu/prefer_properties_over_methods"), false);

    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "interface/editor/components_panel/show_function_friendly_names"), true);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "interface/editor/components_panel/show_graph_friendly_names"), true);

    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "interface/editor/graph/confirm_on_delete"), true);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "interface/editor/graph/disconnect_control_flow_when_dragged"), true);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "interface/editor/graph/grid_enabled"), true);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::STRING, "interface/editor/graph/grid_pattern", PROPERTY_HINT_ENUM, "Dots,Lines"), "Dots");
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "interface/editor/graph/grid_snapping_enabled"), true);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "interface/editor/graph/show_advanced_tooltips"), false);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "interface/editor/graph/show_autowire_selection_dialog"), true);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "interface/editor/graph/show_minimap"), false);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "interface/editor/graph/show_arrange_button"), false);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "interface/editor/graph/show_overlay_action_tooltips"), true);

    GLOBAL_DEF_BASIC(PropertyInfo(Variant::STRING, "interface/editor/graph_nodes/connection_hotzone_scale", PROPERTY_HINT_ENUM, "100%,150%,200%"), "100%");
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "interface/editor/graph_nodes/highlight_selected_connections"), false);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "interface/editor/graph_nodes/resizable_by_default"), false);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "interface/editor/graph_nodes/resize_to_content"), false);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "interface/editor/graph_nodes/show_type_icons"), true);

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
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::STRING, "interface/theme/color_theme", PROPERTY_HINT_ENUM, "Default,Dark,Legacy,Custom"), "Default");

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

    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "interface/theme/connection_colors/execution", PROPERTY_HINT_COLOR_NO_ALPHA), Color(1.0, 1.0, 1.0));
    for (const TypeColor& type_color: type_colors) {
        GLOBAL_DEF_BASIC(PropertyInfo(
            Variant::COLOR,
            vformat("interface/theme/connection_colors/%s", VariantUtils::get_friendly_type_name(type_color.type, true).to_lower().replace(" ", "_")),
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
                vformat("interface/theme/connection_colors/%s", VariantUtils::get_friendly_type_name(type, true).to_lower().replace(" ", "_")),
                PROPERTY_HINT_COLOR_NO_ALPHA),
                Color(0.5, 0.5, 0.5));
            }
        }
    }

    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "interface/theme/nodes/background_color", PROPERTY_HINT_COLOR_NO_ALPHA), Color::html("#191d23"));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "interface/theme/nodes/border_color", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.059f, 0.067f, 0.082f));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "interface/theme/nodes/selected_border_color", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.68f, 0.44f, 0.09f));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::FLOAT, "interface/theme/nodes/border_radius", PROPERTY_HINT_RANGE, "0,16,1"), 4.0);
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::FLOAT, "interface/theme/nodes/border_width", PROPERTY_HINT_RANGE, "0.8,10,.2"), 2.0);

    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "interface/theme/node_colors/constants_and_literals", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.271, 0.392, 0.2));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "interface/theme/node_colors/dialogue", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.318, 0.435, 0.839));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "interface/theme/node_colors/events", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.467, 0.0, 0.0));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "interface/theme/node_colors/flow_control", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.132, 0.258, 0.266));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "interface/theme/node_colors/function_call", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.0, 0.2, 0.396));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "interface/theme/node_colors/orchestration_function_call", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.0, 0.316, 0.601));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "interface/theme/node_colors/other_script_function_call", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.027, 0.341, 0.504));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "interface/theme/node_colors/pure_function_call", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.133, 0.302, 0.114));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "interface/theme/node_colors/function_terminator", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.294, 0.0, 0.506));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "interface/theme/node_colors/function_result", PROPERTY_HINT_COLOR_NO_ALPHA), Color(1.0, 0.65, 0.4));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "interface/theme/node_colors/math_operations", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.259, 0.408, 0.384));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "interface/theme/node_colors/memory", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.351, .339, .133));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "interface/theme/node_colors/properties", PROPERTY_HINT_COLOR_NO_ALPHA), Color(.467, 0.28, 0.175));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "interface/theme/node_colors/resources", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.263, 0.275, 0.359));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "interface/theme/node_colors/scene", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.208, 0.141, 0.282));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "interface/theme/node_colors/signals", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.353, 0.0, 0.0));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "interface/theme/node_colors/variable", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.259, 0.177, 0.249));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "interface/theme/node_colors/type_cast", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.009, 0.221, 0.203));
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "interface/theme/node_colors/comment", PROPERTY_HINT_COLOR_NO_ALPHA), Color(0.4, 0.4, 0.4));
    
    GLOBAL_DEF_BASIC(PropertyInfo(Variant::COLOR, "interface/theme/tool_scripts/background_color"), Color(1.f, 1.f, 0.f, 0.10f));
}

void OrchestratorSettings::_rename_settings() {
    _rename("settings/default_type", "editor/settings/default_type");
    _rename("settings/storage_format", "editor/settings/storage_format");
    _rename("settings/notify_about_pre-releases", "editor/settings/updates/notify_about_preview_releases");
    _rename("settings/runtime/max_call_stack", "debug/settings/max_call_stack");
    _rename("settings/runtime/max_loop_iterations", "debug/settings/max_loop_iterations");
    _rename("settings/runtime/use_node_convergence", "debug/settings/use_node_convergence");
    _rename("settings/dialogue/default_message_scene", "runtime/dialogue/default_message_scene");
    _rename("settings/runtime/print_string_scale", "runtime/print_string/overlay_scale");
    _rename("ui/actions_menu/center_on_mouse", "interface/editor/actions_menu/center_on_mouse");
    _rename("ui/actions_menu/close_on_focus_lost", "interface/editor/actions_menu/close_on_focus_lost");
    _rename("ui/actions_menu/prefer_properties_over_methods", "interface/editor/actions_menu/prefer_properties_over_methods");
    _rename("ui/components_panel/show_graph_friendly_names", "interface/editor/components_panel/show_graph_friendly_names");
    _rename("ui/components_panel/show_function_friendly_names", "interface/editor/components_panel/show_function_friendly_names");
    _rename("ui/graph/confirm_on_delete", "interface/editor/graph/confirm_on_delete");
    _rename("ui/graph/disconnect_control_flow_when_dragged", "interface/editor/graph/disconnect_control_flow_when_dragged");
    _rename("ui/graph/grid_enabled", "interface/editor/graph/grid_enabled");
    _rename("ui/graph/grid_pattern", "interface/editor/graph/grid_pattern");
    _rename("ui/graph/show_advanced_tooltips", "interface/editor/graph/show_advanced_tooltips");
    _rename("ui/graph/show_autowire_selection_dialog", "interface/editor/graph/show_autowire_selection_dialog");
    _rename("ui/graph/show_minimap", "interface/editor/graph/show_minimap");
    _rename("ui/graph/show_arrange_button", "interface/editor/graph/show_arrange_button");
    _rename("ui/graph/show_overlay_action_tooltips", "interface/editor/graph/show_overlay_action_tooltips");
    _rename("ui/nodes/connection_hotzone_scale", "interface/editor/graph_nodes/connection_hotzone_scale");
    _rename("ui/nodes/show_type_icons", "interface/editor/graph_nodes/show_type_icons");
    _rename("ui/nodes/resizable_by_default", "interface/editor/graph_nodes/resizable_by_default");
    _rename("ui/nodes/resize_to_content", "interface/editor/graph_nodes/resize_to_content");
    _rename("ui/nodes/highlight_selected_connections", "interface/editor/graph_nodes/highlight_selected_connections");
    _rename("ui/graph/tool_script_background_color", "interface/theme/tool_scripts/background_color");
    _rename("ui/connection_colors/execution", "interface/theme/connection_colors/execution");
    _rename("ui/connection_colors/any", "interface/theme/connection_colors/any");
    _rename("ui/connection_colors/boolean", "interface/theme/connection_colors/boolean");
    _rename("ui/connection_colors/integer", "interface/theme/connection_colors/integer");
    _rename("ui/connection_colors/float", "interface/theme/connection_colors/float");
    _rename("ui/connection_colors/string", "interface/theme/connection_colors/string");
    _rename("ui/connection_colors/string name", "interface/theme/connection_colors/string_name");
    _rename("ui/connection_colors/rect2", "interface/theme/connection_colors/rect2");
    _rename("ui/connection_colors/rect2i", "interface/theme/connection_colors/rect2i");
    _rename("ui/connection_colors/vector2", "interface/theme/connection_colors/vector2");
    _rename("ui/connection_colors/vector2i", "interface/theme/connection_colors/vector2i");
    _rename("ui/connection_colors/vector3", "interface/theme/connection_colors/vector3");
    _rename("ui/connection_colors/vector3i", "interface/theme/connection_colors/vector3i");
    _rename("ui/connection_colors/vector4", "interface/theme/connection_colors/vector4");
    _rename("ui/connection_colors/vector4i", "interface/theme/connection_colors/vector4i");
    _rename("ui/connection_colors/transform2d", "interface/theme/connection_colors/transform2d");
    _rename("ui/connection_colors/transform3d", "interface/theme/connection_colors/transform3d");
    _rename("ui/connection_colors/plane", "interface/theme/connection_colors/plane");
    _rename("ui/connection_colors/quaternion", "interface/theme/connection_colors/quaternion");
    _rename("ui/connection_colors/aabb", "interface/theme/connection_colors/aabb");
    _rename("ui/connection_colors/basis", "interface/theme/connection_colors/basis");
    _rename("ui/connection_colors/projection", "interface/theme/connection_colors/projection");
    _rename("ui/connection_colors/color", "interface/theme/connection_colors/color");
    _rename("ui/connection_colors/nodepath", "interface/theme/connection_colors/nodepath");
    _rename("ui/connection_colors/rid", "interface/theme/connection_colors/rid");
    _rename("ui/connection_colors/object", "interface/theme/connection_colors/object");
    _rename("ui/connection_colors/dictionary", "interface/theme/connection_colors/dictionary");
    _rename("ui/connection_colors/array", "interface/theme/connection_colors/array");
    _rename("ui/connection_colors/callable", "interface/theme/connection_colors/callable");
    _rename("ui/connection_colors/packed byte array", "interface/theme/connection_colors/packed_byte_array");
    _rename("ui/connection_colors/packed string array", "interface/theme/connection_colors/packed_string_array");
    _rename("ui/connection_colors/packed int32 array", "interface/theme/connection_colors/packed_int32_array");
    _rename("ui/connection_colors/packed int64 array", "interface/theme/connection_colors/packed_int64_array");
    _rename("ui/connection_colors/packed float32 array", "interface/theme/connection_colors/packed_float32_array");
    _rename("ui/connection_colors/packed float64 array", "interface/theme/connection_colors/packed_float64_array");
    _rename("ui/connection_colors/packed vector2 array", "interface/theme/connection_colors/packed_vector2_array");
    _rename("ui/connection_colors/packed vector3 array", "interface/theme/connection_colors/packed_vector3_array");
    _rename("ui/connection_colors/packed vector4 array", "interface/theme/connection_colors/packed_vector4_array");
    _rename("ui/connection_colors/packed color array", "interface/theme/connection_colors/packed_color_array");
    _rename("ui/nodes/background_color", "interface/theme/nodes/background_color");
    _rename("ui/nodes/border_color", "interface/theme/nodes/border_color");
    _rename("ui/nodes/border_selected_color", "interface/theme/nodes/selected_border_color");
    _rename("ui/nodes/border_radius", "interface/theme/nodes/border_radius");
    _rename("ui/nodes/border_width", "interface/theme/nodes/border_width");
    _rename("ui/node_colors/constants_and_literals", "interface/theme/node_colors/constants_and_literals");
    _rename("ui/node_colors/dialogue", "interface/theme/node_colors/dialogue");
    _rename("ui/node_colors/events", "interface/theme/node_colors/events");
    _rename("ui/node_colors/flow_control", "interface/theme/node_colors/flow_control");
    _rename("ui/node_colors/function_call", "interface/theme/node_colors/function_call");
    _rename("ui/node_colors/orchestration_function_call", "interface/theme/node_colors/orchestration_function_call");
    _rename("ui/node_colors/other_script_function_call", "interface/theme/node_colors/other_script_function_call");
    _rename("ui/node_colors/pure_function_call", "interface/theme/node_colors/pure_function_call");
    _rename("ui/node_colors/function_terminator", "interface/theme/node_colors/function_terminator");
    _rename("ui/node_colors/function_result", "interface/theme/node_colors/function_result");
    _rename("ui/node_colors/math_operations", "interface/theme/node_colors/math_operations");
    _rename("ui/node_colors/memory", "interface/theme/node_colors/memory");
    _rename("ui/node_colors/properties", "interface/theme/node_colors/properties");
    _rename("ui/node_colors/resources", "interface/theme/node_colors/resources");
    _rename("ui/node_colors/scene", "interface/theme/node_colors/scene");
    _rename("ui/node_colors/signals", "interface/theme/node_colors/signals");
    _rename("ui/node_colors/variable", "interface/theme/node_colors/variable");
    _rename("ui/node_colors/type_cast", "interface/theme/node_colors/type_cast");
    _rename("ui/node_colors/comment", "interface/theme/node_colors/comment");
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
    return ProjectSettings::get_singleton()->has_setting(_make_key(p_key));
}

Variant OrchestratorSettings::get_setting(const String& p_key, const Variant& p_default_value) {
    return ProjectSettings::get_singleton()->get_setting(_make_key(p_key), p_default_value);
}

void OrchestratorSettings::set_setting(const String& p_key, const Variant& p_value) {
    ProjectSettings::get_singleton()->set_setting(_make_key(p_key), p_value);
}

bool OrchestratorSettings::is_notify_about_prereleases() {
    return get_setting("editor/settings/updates/notify_about_preview_releases", true);
}

void OrchestratorSettings::set_notify_prerelease_builds(bool p_notify_about_prereleases) {
    const String key = "editor/settings/updates/notify_about_preview_releases";
    if (!has_setting(key)) {
        return;
    }

    set_setting(key, p_notify_about_prereleases);
    ProjectSettings::get_singleton()->save();
}

void OrchestratorSettings::_bind_methods() {

}

OrchestratorSettings::OrchestratorSettings() {
    // Initialize the current preferred theme
    _current_theme = get_setting("interface/theme/color_theme", "Default");

    _rename_settings();
    _initialize_settings();
    _remove_deprecated_settings();

    _apply_color_theme_preset(_current_theme);

    ProjectSettings::get_singleton()->connect("settings_changed", callable_mp_this(_project_settings_changed));

    _editor_property_descriptions = ExtensionDB::get_setting_descriptions();
}

OrchestratorSettings::~OrchestratorSettings() {
}

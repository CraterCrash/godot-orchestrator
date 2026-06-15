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
#include "editor/theme/theme_builder.h"

#include "common/macros.h"
#include "common/scene_utils.h"
#include "common/settings.h"
#include "core/godot/scene_string_names.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/style_box_empty.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>

OrchestratorEditorThemeBuilder::ThemeParams OrchestratorEditorThemeBuilder::_read_theme_params() const {
    return {
        ORCHESTRATOR_GET("interface/theme/color_theme", "Default"),
        ORCHESTRATOR_GET("interface/theme/nodes/border_radius", 4),
        ORCHESTRATOR_GET("interface/theme/nodes/border_width", 2),
        ORCHESTRATOR_GET("interface/theme/nodes/border_color", Color(0, 0, 0)),
        ORCHESTRATOR_GET("interface/theme/nodes/selected_border_color", Color(0.68f, 0.44f, 0.09f)),
        ORCHESTRATOR_GET("interface/theme/nodes/background_color", Color::html("#191d23"))
    };
}

void OrchestratorEditorThemeBuilder::_build_graph_styles(const Ref<Theme>& p_theme, const ThemeParams& p_params) {

    // Base GraphNode: panel font color adapts to background brightness
    const float bg_lum = p_params.background_color.get_luminance();
    const Color panel_font_color = bg_lum > 0.45f ? Color(0.1f, 0.1f, 0.1f) : Color(1.f, 1.f, 1.f);
    p_theme->set_color("font_color", "GraphNode", panel_font_color);

    // Base GraphNode
    const Ref<StyleBoxFlat> sb = SceneUtils::get_editor_stylebox(SceneStringName(panel), "GraphNode");
    if (sb.is_valid()) {
        Ref<StyleBoxFlat> new_sb = sb->duplicate(true);
        new_sb->set_border_color(p_params.border_color);
        new_sb->set_border_width_all(p_params.border_width);
        new_sb->set_border_width(SIDE_TOP, 0);
        new_sb->set_content_margin_all(2);
        new_sb->set_content_margin(SIDE_BOTTOM, 6);
        new_sb->set_corner_radius_all(p_params.border_radius);
        new_sb->set_corner_radius(CORNER_TOP_LEFT, 0);
        new_sb->set_corner_radius(CORNER_TOP_RIGHT, 0);
        new_sb->set_bg_color(p_params.background_color);
        p_theme->set_stylebox(SceneStringName(panel), "GraphNode", new_sb);

        Ref<StyleBoxFlat> new_sb_selected = new_sb->duplicate();
        new_sb_selected->set_border_color(p_params.selected_border_color);
        p_theme->set_stylebox("panel_selected", "GraphNode", new_sb_selected);
    }

    // Colored GraphNode variations
    const PackedStringArray settings = OrchestratorSettings::get_singleton()->get_settings_name_list();
    for (const String& setting : settings) {
        if (!setting.begins_with("orchestrator/interface/theme/node_colors")) {
            continue;
        }

        const Color color = OrchestratorSettings::get_singleton()->get_setting(setting);
        const PackedStringArray parts = setting.split("/");
        const String type_name = vformat("OEGraphNode_%s", parts[parts.size() - 1]);

        // Titlebar: title text color adapts to category tile color brightness
        const float tile_lum = color.get_luminance();
        const Color title_color = tile_lum > 0.5f ? Color(0.1f, 0.1f, 0.1f) : Color(1.f, 1.f, 1.f);
        p_theme->set_color("title_color", type_name, title_color);

        p_theme->set_type_variation(type_name, "GraphNode");

        const Ref<StyleBoxFlat> sb_titlebar = SceneUtils::get_editor_stylebox("titlebar", "GraphNode");
        if (sb_titlebar.is_valid()) {
            const Ref<StyleBoxFlat> new_titlebar = sb_titlebar->duplicate(true);
            new_titlebar->set_bg_color(color);
            new_titlebar->set_border_width_all(p_params.border_width);
            new_titlebar->set_border_width(SIDE_BOTTOM, 0);
            new_titlebar->set_corner_radius_all(p_params.border_radius);
            new_titlebar->set_corner_radius(CORNER_BOTTOM_LEFT, 0);
            new_titlebar->set_corner_radius(CORNER_BOTTOM_RIGHT, 0);
            new_titlebar->set_content_margin_all(4);
            new_titlebar->set_content_margin(SIDE_LEFT, 12);
            new_titlebar->set_content_margin(SIDE_RIGHT, 12);
            new_titlebar->set_border_color(p_params.border_color);
            p_theme->set_stylebox("titlebar", type_name, new_titlebar);

            const Ref<StyleBoxFlat> new_titlebar_selected = new_titlebar->duplicate();
            new_titlebar_selected->set_border_color(p_params.selected_border_color);
            p_theme->set_stylebox("titlebar_selected", type_name, new_titlebar_selected);
        }
    }

    // Reroute nodes
    const String reroute_type_name = "OEGraphNode_reroute";
    p_theme->set_type_variation(reroute_type_name, "GraphNode");

    const Ref<StyleBox> reroute_sb = memnew(StyleBoxEmpty);
    reroute_sb->set_content_margin_all(-1);

    const Ref<StyleBox> reroute_titlebar = reroute_sb->duplicate();
    reroute_titlebar->set_content_margin_all(16 * EDSCALE);

    p_theme->set_stylebox(SceneStringName(panel), reroute_type_name, reroute_sb);
    p_theme->set_stylebox("panel_selected", reroute_type_name, reroute_sb);
    p_theme->set_stylebox("titlebar", reroute_type_name, reroute_titlebar);
    p_theme->set_stylebox("titlebar_selected", reroute_type_name, reroute_titlebar);
    p_theme->set_stylebox("slot", reroute_type_name, reroute_sb->duplicate());
    p_theme->set_constant("port_h_offset", reroute_type_name, 16.0 * EDSCALE);

}

void OrchestratorEditorThemeBuilder::_rebuild_theme() {
    _rebuilding = false;

    const ThemeParams params = _read_theme_params();
    Ref<Theme> theme = memnew(Theme);

    _build_graph_styles(theme, params);

    _theme = theme;
    emit_signal("theme_rebuilt");
}

void OrchestratorEditorThemeBuilder::queue_rebuild() {
    if (_rebuilding) {
        return;
    }

    _rebuilding = true;
    callable_mp_this(_rebuild_theme).call_deferred();
}

void OrchestratorEditorThemeBuilder::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_POSTINITIALIZE: {
            ProjectSettings::get_singleton()->connect("settings_changed", callable_mp_this(queue_rebuild));
            callable_mp_this(queue_rebuild).call_deferred();
            break;
        }
    }
}

void OrchestratorEditorThemeBuilder::_bind_methods() {
    ADD_SIGNAL(MethodInfo("theme_rebuilt"));
}

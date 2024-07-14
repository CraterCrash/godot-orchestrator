// This file is part of the Godot Orchestrator project.
//
// Copyright (c) 2023-present Vahera Studios LLC and its contributors.
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
#include "editor/theme/theme_cache.h"

#include "common/scene_utils.h"
#include "common/settings.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/graph_node.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>

void OrchestratorThemeCache::_settings_changed()
{
    // Connect to this signal if it hasn't been connected yet
    Callable handler = callable_mp(this, &OrchestratorThemeCache::_settings_changed);
    if (!ProjectSettings::get_singleton()->is_connected("settings_changed", handler))
        ProjectSettings::get_singleton()->connect("settings_changed", handler);

    OrchestratorSettings* settings = OrchestratorSettings::get_singleton();

    const Color border = settings->get_setting("ui/nodes/border_color", Color(0, 0, 0));
    const Color select = settings->get_setting("ui/nodes/border_selected_color", Color(0.68f, 0.44f, 0.09f));
    const Color bkgrnd = settings->get_setting("ui/nodes/background_color", Color::html("#191d23"));
    const int radius = settings->get_setting("ui/nodes/border_radius", 4);
    const int bwidth = settings->get_setting("ui/nodes/border_width", 2);

    Ref<StyleBoxFlat> panel = get_theme_stylebox("panel", "GraphNode");
    if (!panel.is_valid())
    {
        // Not yet primed.
        Ref<StyleBoxFlat> new_panel = _get_editor_theme_stylebox("panel", "GraphNode")->duplicate(true);
        if (new_panel.is_valid())
        {
            new_panel->set_border_color(border);
            new_panel->set_border_width_all(bwidth);
            new_panel->set_border_width(SIDE_TOP, 0);
            new_panel->set_content_margin_all(2);
            new_panel->set_content_margin(SIDE_BOTTOM, 6);
            new_panel->set_corner_radius_all(radius);
            new_panel->set_corner_radius(CORNER_TOP_LEFT, 0);
            new_panel->set_corner_radius(CORNER_TOP_RIGHT, 0);
            new_panel->set_bg_color(bkgrnd);

            add_theme_stylebox("panel", "GraphNode", new_panel);

            Ref<StyleBoxFlat> panel_selected = new_panel->duplicate();
            if (panel_selected.is_valid())
            {
                panel_selected->set_border_color(select);
                add_theme_stylebox("panel_selected", "GraphNode", panel_selected);
            }
        }
    }
    else
    {
        if (panel->get_border_color() != border)
            panel->set_border_color(border);

        if (panel->get_bg_color() != bkgrnd)
            panel->set_bg_color(bkgrnd);

        if (panel->get_corner_radius(CORNER_BOTTOM_LEFT) != radius)
        {
            panel->set_corner_radius_all(radius);
            panel->set_corner_radius(CORNER_TOP_LEFT, 0);
            panel->set_corner_radius(CORNER_TOP_RIGHT, 0);
        }

        if (panel->get_border_width(SIDE_LEFT) != bwidth)
        {
            panel->set_border_width_all(bwidth);
            panel->set_border_width(SIDE_TOP, 0);
        }

        Ref<StyleBoxFlat> panel_selected = get_theme_stylebox("panel_selected", "GraphNode");
        if (panel_selected.is_valid())
        {
            if (panel_selected->get_bg_color() != bkgrnd)
                panel_selected->set_bg_color(bkgrnd);

            if (panel_selected->get_border_color() != select)
                panel_selected->set_border_color(select);

            if (panel_selected->get_corner_radius(CORNER_BOTTOM_LEFT) != radius)
            {
                panel_selected->set_corner_radius_all(radius);
                panel_selected->set_corner_radius(CORNER_TOP_LEFT, 0);
                panel_selected->set_corner_radius(CORNER_TOP_RIGHT, 0);
            }

            if (panel_selected->get_border_width(SIDE_LEFT) != bwidth)
            {
                panel_selected->set_border_width_all(bwidth);
                panel_selected->set_border_width(SIDE_TOP, 0);
            }
        }
    }

    for (const OrchestratorSettings::Setting& setting : settings->get_settings())
    {
        if (!setting.info.name.begins_with("ui/node_colors/"))
            continue;

        const Color color = settings->get_setting(setting.info.name);
        const PackedStringArray parts = setting.info.name.split("/");
        const String type_name = vformat("GraphNode_%s", parts[parts.size() - 1]);

        Ref<StyleBoxFlat> titlebar = get_theme_stylebox("titlebar", type_name);
        if (!titlebar.is_valid())
        {
            Ref<StyleBoxFlat> new_titlebar = _get_editor_theme_stylebox("titlebar", "GraphNode")->duplicate(true);
            if (new_titlebar.is_valid())
            {
                new_titlebar->set_bg_color(color);
                new_titlebar->set_border_width_all(bwidth);
                new_titlebar->set_border_width(SIDE_BOTTOM, 0);
                new_titlebar->set_corner_radius_all(radius);
                new_titlebar->set_corner_radius(CORNER_BOTTOM_LEFT, 0);
                new_titlebar->set_corner_radius(CORNER_BOTTOM_RIGHT, 0);

                new_titlebar->set_content_margin_all(4);
                new_titlebar->set_content_margin(SIDE_LEFT, 12);
                new_titlebar->set_content_margin(SIDE_RIGHT, 12);
                new_titlebar->set_border_color(border);

                add_theme_stylebox("titlebar", type_name, new_titlebar);

                Ref<StyleBoxFlat> titlebar_selected = new_titlebar->duplicate();
                if (titlebar_selected.is_valid())
                {
                    titlebar_selected->set_border_color(select);
                    add_theme_stylebox("titlebar_selected", type_name, titlebar_selected);
                }
            }
        }
        else
        {
            if (titlebar->get_bg_color() != color || titlebar->get_border_color() != border)
            {
                // Primed, but color changed
                titlebar->set_bg_color(color);
                titlebar->set_border_color(border);
            }

            if (titlebar->get_corner_radius(CORNER_TOP_LEFT) != radius)
            {
                titlebar->set_corner_radius_all(radius);
                titlebar->set_corner_radius(CORNER_BOTTOM_LEFT, 0);
                titlebar->set_corner_radius(CORNER_BOTTOM_RIGHT, 0);
            }

            if (titlebar->get_border_width(SIDE_LEFT) != bwidth)
            {
                titlebar->set_border_width_all(bwidth);
                titlebar->set_border_width(SIDE_BOTTOM, 0);
            }

            const Ref<StyleBoxFlat> titlebar_selected = get_theme_stylebox("titlebar_selected", type_name);
            if (titlebar_selected.is_valid())
            {
                if (titlebar_selected->get_bg_color() != color)
                    titlebar_selected->set_bg_color(color);
                if (titlebar_selected->get_border_color() != select)
                    titlebar_selected->set_border_color(select);

                if (titlebar_selected->get_corner_radius(CORNER_TOP_LEFT) != radius)
                {
                    titlebar_selected->set_corner_radius_all(radius);
                    titlebar_selected->set_corner_radius(CORNER_BOTTOM_LEFT, 0);
                    titlebar_selected->set_corner_radius(CORNER_BOTTOM_RIGHT, 0);
                }

                if (titlebar_selected->get_border_width(SIDE_LEFT) != bwidth)
                {
                    titlebar_selected->set_border_width_all(bwidth);
                    titlebar_selected->set_border_width(SIDE_BOTTOM, 0);
                }
            }
        }
    }
}

void OrchestratorThemeCache::add_theme_stylebox(const StringName& p_name, const String& p_type_name, const Ref<StyleBox>& p_stylebox)
{
    if (!_stylebox_cache.has(p_type_name))
        _stylebox_cache[p_type_name] = HashMap<StringName, Ref<StyleBox>>();

    _stylebox_cache[p_type_name][p_name] = p_stylebox;
}

Ref<StyleBox> OrchestratorThemeCache::get_theme_stylebox(const StringName& p_name, const String& p_type_name) const
{
    if (_stylebox_cache.has(p_type_name))
    {
        const HashMap<StringName, Ref<StyleBox>>& second_level = _stylebox_cache[p_type_name];
        if (second_level.has(p_name))
            return second_level[p_name];
    }
    return nullptr;
}

Ref<StyleBox> OrchestratorThemeCache::_get_editor_theme_stylebox(const String& p_name, const String& p_type_name) const
{
    return SceneUtils::get_editor_stylebox(p_name, p_type_name);
}
void OrchestratorThemeCache::_notification(int p_what)
{
    if (p_what == NOTIFICATION_POSTINITIALIZE)
        callable_mp(this, &OrchestratorThemeCache::_settings_changed).call_deferred();
}


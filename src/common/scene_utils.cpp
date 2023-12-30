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
#include "scene_utils.h"

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/classes/window.hpp>

namespace SceneUtils
{
    Ref<Texture2D> get_icon(Control* p_control, const String& p_icon_name)
    {
        if (!p_icon_name.begins_with("res://"))
            return p_control->get_theme_icon(p_icon_name, "EditorIcons");

        return ResourceLoader::get_singleton()->load(p_icon_name);
    }

    Ref<Texture2D> get_icon(Window* p_window, const String& p_icon_name)
    {
        if (!p_icon_name.begins_with("res://"))
            return p_window->get_theme_icon(p_icon_name, "EditorIcons");

        return ResourceLoader::get_singleton()->load(p_icon_name);
    }

    String create_wrapped_tooltip_text(const String& p_tooltip_text, int p_width)
    {
        String wrapped = ""; // the wrapped text
        String current = ""; // current line

        Ref<Theme> theme = ThemeDB::get_singleton()->get_default_theme();
        if (!theme.is_valid())
            return p_tooltip_text;

        Ref<Font> font = theme->get_default_font();
        if (!font.is_valid())
            return p_tooltip_text;

        const PackedStringArray words = p_tooltip_text.split(" ", false);
        for (int i = 0; i < words.size(); i++)
        {
            const String word = words[i];

            // Check if word contains a new line
            int nl_pos = word.find("\n");
            if (nl_pos != -1)
            {
                // Split the word at the new line
                String before = word.substr(0, nl_pos);
                String after = word.substr(nl_pos + 1, word.length() - nl_pos - 1);
                if (font->get_string_size(current + before).width > p_width)
                {
                    wrapped += current + "\n";
                    current = "";
                }
                current += before + "\n";
                wrapped += current;
                current = after + " ";
            }
            else
            {
                // Checks if adding the next word exceeds the width
                if (font->get_string_size(current + word).width > p_width)
                {
                    wrapped += current + "\n";
                    current = "";
                }
                current += word + String(" ");
            }
        }

        // Add last line
        wrapped += current;
        return wrapped;
    }
}
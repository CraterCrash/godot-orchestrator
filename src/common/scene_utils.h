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
#ifndef ORCHESTRATOR_SCENE_UTILS_H
#define ORCHESTRATOR_SCENE_UTILS_H

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/texture2d.hpp>

using namespace godot;

namespace SceneUtils
{
    /// Load an icon.
    ///
    /// @attention This method inspects the provided icon name and if it is not referring to a
    /// resource on the file system, it is assumed to refer to an icon in the "EditorIcons"
    /// pack of the Editor.
    ///
    /// @param p_control the control to get the theme icon based on, typically "this".
    /// @param p_icon_name the icon name or fully qualified file path
    /// @return a reference to the texture or an invalid reference if the texture isn't loaded
    Ref<Texture2D> get_icon(Control* p_control, const String& p_icon_name);

    /// Load an icon.
    ///
    /// @attention This method inspects the provided icon name and if it is not referring to a
    /// resource on the file system, it is assumed to refer to an icon in the "EditorIcons"
    /// pack of the Editor.
    ///
    /// @param p_window the window, typically "this".
    /// @param p_icon_name the icon name or fully qualified file path
    /// @return a reference to the texture or an invalid reference if the texture isn't loaded
    Ref<Texture2D> get_icon(Window* p_window, const String& p_icon_name);

    /// Creates tooltip text that will automatically be wrapped at word boundaries and will not
    /// exceed the specified width. If text contains new lines, those will be preserved.
    ///
    /// @param p_tooltip_text the text to be wrapped
    /// @param p_width the maximum width of a line of text before wrapping
    /// @return the wrapped tooltip text
    String create_wrapped_tooltip_text(const String& p_tooltip_text, int p_width = 512);
}

#endif  // ORCHESTRATOR_SCENE_UTILS_H

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
#ifndef ORCHESTRATOR_WINDOW_WRAPPER_H
#define ORCHESTRATOR_WINDOW_WRAPPER_H

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/margin_container.hpp>

using namespace godot;

namespace godot
{
    class HBoxContainer;
    class Panel;
    class Popup;
}

/// A window wrapper implementation, based heavily off the Godot Editor's WindowWrapper class.
/// @see godot-engine/editor/window_wrapper.h
class OrchestratorWindowWrapper : public MarginContainer
{
    GDCLASS(OrchestratorWindowWrapper, MarginContainer);

    Control* _wrapped_control{ nullptr };
    MarginContainer* _margin{ nullptr };
    Window* _window{ nullptr };
    Panel* _window_background{ nullptr };

    Rect2 _get_default_window_rect() const;
    Node* _get_wrapped_control_parent() const;

    void _set_window_enabled_with_rect(bool p_visible, const Rect2 p_rect);
    void _set_window_rect(const Rect2 p_rect);

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    OrchestratorWindowWrapper();

    void set_wrapped_control(Control* p_control);
    Control* get_wrapped_control() const;
    Control* release_wrapped_control();

    bool is_window_available() const;

    bool get_window_enabled() const;
    void set_window_enabled(bool p_enabled);

    Rect2i get_window_rect() const;
    int get_window_screen() const;

    void restore_window(const Rect2i& p_rect, int p_screen = -1);
    void restore_window_from_saved_position(const Rect2 p_window_rect, int p_screen, const Rect2 p_screen_rect);
    void enable_window_on_screen(int p_screen = -1, bool p_auto_scale = false);

    void set_window_title(const String& p_title);
    void set_margins_enabled(bool p_enabled);

    void move_to_foreground();
};

/// A screen select button implementation, based heavily off the Godot Editor's ScreenSelect class.
/// @see godot-engine/editor/window_wrapper.h
class OrchestratorScreenSelect : public Button
{
    GDCLASS(OrchestratorScreenSelect, Button);

    Popup* _popup{ nullptr };
    Panel* _popup_background{ nullptr };
    HBoxContainer* _screen_list{ nullptr };

    void _build_advanced_menu();

    void _emit_screen_signal(int p_screen_index);
    void _handle_mouse_shortcut(const Ref<InputEvent>& p_event);
    void _show_popup();

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    OrchestratorScreenSelect();

    void _pressed() override;
};

#endif // ORCHESTRATOR_WINDOW_WRAPPER_H
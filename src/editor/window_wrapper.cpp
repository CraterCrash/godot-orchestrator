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
#include "window_wrapper.h"

#include "common/scene_utils.h"
#include "editor/plugins/orchestrator_editor_plugin.h"

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/panel.hpp>
#include <godot_cpp/classes/popup.hpp>
#include <godot_cpp/classes/style_box.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

OrchestratorWindowWrapper::OrchestratorWindowWrapper()
{
    _window = memnew(Window);
    _window->set_wrap_controls(true);

    add_child(_window);
    _window->hide();

    _window_background = memnew(Panel);
    _window_background->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
    _window->add_child(_window_background);

    // todo: what about progress bar?
}

void OrchestratorWindowWrapper::_bind_methods()
{
    ADD_SIGNAL(MethodInfo("window_visibility_changed", PropertyInfo(Variant::BOOL, "visible")));
    ADD_SIGNAL(MethodInfo("window_close_requested"));
}

void OrchestratorWindowWrapper::_notification(int p_what)
{
    if (!is_window_available())
        return;

    switch (p_what)
    {
        // Grab the focus when WindowWrapper::set_visible(true) is called and is shown.
        case NOTIFICATION_VISIBILITY_CHANGED:
            if (get_window_enabled() && is_visible())
                _window->grab_focus();
            break;

        case NOTIFICATION_ENTER_TREE:
            _window->connect("close_requested", callable_mp(this, &OrchestratorWindowWrapper::set_window_enabled).bind(false));
            break;

        case NOTIFICATION_READY:
            break;

        case NOTIFICATION_THEME_CHANGED:
            _window_background->add_theme_stylebox_override("panel",
                                                            get_theme_stylebox("PanelForeground", "EditorStyles"));
            break;
    }
}

Rect2 OrchestratorWindowWrapper::_get_default_window_rect() const
{
    // Assume that the control rect is the desired one for the window.
    Transform2D xform = _wrapped_control->get_screen_transform();
    return Rect2(xform.get_origin(), xform.get_scale() * get_size());
}

Node* OrchestratorWindowWrapper::_get_wrapped_control_parent() const
{
    if (_margin)
        return _margin;
    return _window;
}

void OrchestratorWindowWrapper::_set_window_enabled_with_rect(bool p_visible, const Rect2 p_rect)
{
    ERR_FAIL_NULL(_wrapped_control);

    if (!is_window_available())
        return;

    if (_window->is_visible() == p_visible)
    {
        if (p_visible)
            _window->grab_focus();
        return;
    }

    Node* parent = _get_wrapped_control_parent();

    if (_wrapped_control->get_parent() != parent)
    {
        // Move the control to the window
        _wrapped_control->reparent(parent, false);

        _set_window_rect(p_rect);
        _wrapped_control->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
    }
    else if (!p_visible)
    {
        // Remove control from window.
        _wrapped_control->reparent(this, false);
    }

    _window->set_visible(p_visible);
    if (!p_visible)
        emit_signal("window_close_requested");

    emit_signal("window_visibility_changed", p_visible);
}

void OrchestratorWindowWrapper::_set_window_rect(const Rect2 p_rect)
{
    // Set the window rect even when the window is maximized to have a good default size
    // when the user leaves maximized mode.
    _window->set_position(p_rect.position);
    _window->set_size(p_rect.size);

    Ref<EditorSettings> es = OrchestratorPlugin::get_singleton()->get_editor_interface()->get_editor_settings();
    if (es.is_valid() && es->get_setting("interface/multi_window/maximize_window"))
        _window->set_mode(Window::MODE_MAXIMIZED);
}

void OrchestratorWindowWrapper::set_wrapped_control(Control* p_control)
{
    ERR_FAIL_NULL(p_control);
    ERR_FAIL_COND(_wrapped_control);

    _wrapped_control = p_control;
    add_child(p_control);
}

Control* OrchestratorWindowWrapper::get_wrapped_control() const
{
    return _wrapped_control;
}

Control* OrchestratorWindowWrapper::release_wrapped_control()
{
    set_window_enabled(false);
    if (_wrapped_control)
    {
        Control* old = _wrapped_control;
        _wrapped_control->get_parent()->remove_child(_wrapped_control);
        _wrapped_control = nullptr;
        return old;
    }
    return nullptr;
}

bool OrchestratorWindowWrapper::is_window_available() const
{
    return _window != nullptr;
}

bool OrchestratorWindowWrapper::get_window_enabled() const
{
    return is_window_available() ? _window->is_visible() : false;
}

void OrchestratorWindowWrapper::set_window_enabled(bool p_enabled)
{
        _set_window_enabled_with_rect(p_enabled, _get_default_window_rect());
}

Rect2i OrchestratorWindowWrapper::get_window_rect() const
{
    ERR_FAIL_COND_V(!get_window_enabled(), Rect2i());
    return Rect2i(_window->get_position(), _window->get_size());
}

int OrchestratorWindowWrapper::get_window_screen() const
{
    ERR_FAIL_COND_V(!get_window_enabled(), -1);
    return _window->get_current_screen();
}

void OrchestratorWindowWrapper::restore_window(const Rect2i& p_rect, int p_screen)
{
    ERR_FAIL_COND(!is_window_available());
    ERR_FAIL_INDEX(p_screen, DisplayServer::get_singleton()->get_screen_count());

    _set_window_enabled_with_rect(true, p_rect);
    _window->set_current_screen(p_screen);
}

void OrchestratorWindowWrapper::restore_window_from_saved_position(const Rect2 p_window_rect, int p_screen, const Rect2 p_screen_rect)
{
    ERR_FAIL_COND(!is_window_available());

    Rect2 window_rect = p_window_rect;
    int screen = p_screen;
    Rect2 restored_screen_rect = p_screen_rect;

    if (screen < 0 || screen >= DisplayServer::get_singleton()->get_screen_count())
    {
        // Fallback to the main window screen if the saved screen is not available.
        screen = get_window()->get_window_id();
    }

    Rect2i real_screen_rect = DisplayServer::get_singleton()->screen_get_usable_rect(screen);

    if (Rect2i(restored_screen_rect) == Rect2i())
    {
        // Fallback to the target screen rect.
        restored_screen_rect = real_screen_rect;
    }

    if (Rect2i(window_rect) == Rect2i())
    {
        // Fallback to a standard rect.
        window_rect = Rect2i(restored_screen_rect.position + restored_screen_rect.size / 4, restored_screen_rect.size / 2);
    }

    // Adjust the window rect size in case the resolution changes.
    Vector2 screen_ratio = Vector2(real_screen_rect.size) / Vector2(restored_screen_rect.size);

    // The screen positioning may change, so remove the original screen position.
    window_rect.position -= restored_screen_rect.position;
    window_rect = Rect2i(window_rect.position * screen_ratio, window_rect.size * screen_ratio);
    window_rect.position += real_screen_rect.position;

    // All good, restore the window.
    _window->set_current_screen(p_screen);
    if (_window->is_visible())
        _set_window_rect(window_rect);
	else
        _set_window_enabled_with_rect(true, window_rect);
}

void OrchestratorWindowWrapper::enable_window_on_screen(int p_screen, bool p_auto_scale)
{
    int current_screen = Object::cast_to<Window>(get_viewport())->get_current_screen();
    int screen = p_screen < 0 ? current_screen : p_screen;

    Ref<EditorSettings> es = OrchestratorPlugin::get_singleton()->get_editor_interface()->get_editor_settings();
    bool auto_scale = p_auto_scale && !es->get_setting("interface/multi_window/maximize_window");

    if (auto_scale && current_screen != screen)
    {
        Rect2 control_rect = _get_default_window_rect();

        Rect2i source_screen_rect = DisplayServer::get_singleton()->screen_get_usable_rect(current_screen);
        Rect2i dest_screen_rect = DisplayServer::get_singleton()->screen_get_usable_rect(screen);

        // Adjust the window rect size in case the resolution changes.
        Vector2 screen_ratio = Vector2(source_screen_rect.size) / Vector2(dest_screen_rect.size);

        // The screen positioning may change, so remove the original screen position.
        control_rect.position -= source_screen_rect.position;
        control_rect = Rect2i(control_rect.position * screen_ratio, control_rect.size * screen_ratio);
        control_rect.position += dest_screen_rect.position;

        restore_window(control_rect, p_screen);
    }
    else
    {
        _window->set_current_screen(p_screen);
        set_window_enabled(true);
    }
}

void OrchestratorWindowWrapper::set_window_title(const String& p_title)
{
    if (!is_window_available())
        return;

    _window->set_title(p_title);
}

void OrchestratorWindowWrapper::set_margins_enabled(bool p_enabled)
{
    if (!is_window_available())
        return;

    if (!p_enabled && _margin)
    {
        _margin->queue_free();
        _margin = nullptr;
    }
    else if (p_enabled && !_margin)
    {
        Size2 borders = Size2(4, 4);
        _margin = memnew(MarginContainer);
        _margin->add_theme_constant_override("margin_right", borders.width);
        _margin->add_theme_constant_override("margin_top", borders.height);
        _margin->add_theme_constant_override("margin_left", borders.width);
        _margin->add_theme_constant_override("margin_bottom", borders.height);

        _window->add_child(_margin);
        _margin->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
    }
}

void OrchestratorWindowWrapper::move_to_foreground()
{
    _window->grab_focus();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OrchestratorScreenSelect::OrchestratorScreenSelect()
{
    set_tooltip_text("Make this panel floating.");
    set_button_mask(MouseButtonMask::MOUSE_BUTTON_MASK_RIGHT);
    set_flat(true);
    set_toggle_mode(true);
    set_focus_mode(FOCUS_NONE);
    set_action_mode(ACTION_MODE_BUTTON_PRESS);

    // Create the popup.
    const Size2 borders = Size2(4, 4);

    _popup = memnew(Popup);
    add_child(_popup);

    _popup_background = memnew(Panel);
    _popup_background->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
    _popup->add_child(_popup_background);

    MarginContainer* root = memnew(MarginContainer);
    root->add_theme_constant_override("margin_right", borders.width);
    root->add_theme_constant_override("margin_top", borders.height);
    root->add_theme_constant_override("margin_left", borders.width);
    root->add_theme_constant_override("margin_bottom", borders.height);
    _popup->add_child(root);

    VBoxContainer* vbox = memnew(VBoxContainer);
    vbox->set_alignment(BoxContainer::ALIGNMENT_CENTER);
    root->add_child(vbox);

    Label* description = memnew(Label);
    description->set_text("Select Screen");
    description->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
    vbox->add_child(description);

    _screen_list = memnew(HBoxContainer);
    _screen_list->set_alignment(BoxContainer::ALIGNMENT_CENTER);
    vbox->add_child(_screen_list);

    root->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
}

void OrchestratorScreenSelect::_bind_methods()
{
    ADD_SIGNAL(MethodInfo("request_open_in_screen", PropertyInfo(Variant::INT, "screen")));
}

void OrchestratorScreenSelect::_notification(int p_what)
{
    switch (p_what)
    {
        case NOTIFICATION_READY:
            _popup->connect("popup_hide", callable_mp(static_cast<BaseButton*>(this), &OrchestratorScreenSelect::set_pressed).bind(false));
            connect("gui_input", callable_mp(this, &OrchestratorScreenSelect::_handle_mouse_shortcut));
            break;

        case NOTIFICATION_THEME_CHANGED:
        {
            set_button_icon(SceneUtils::get_editor_icon("MakeFloating"));
            _popup_background->add_theme_stylebox_override("panel", get_theme_stylebox("PanelForeground", "EditorStyles"));

            const real_t popup_height = real_t(get_theme_font_size("font_size")) * 2.0;
            _popup->set_min_size(Size2(0, popup_height * 3));
            break;
        }
    }
}

void OrchestratorScreenSelect::_pressed()
{
    if (_popup->is_visible())
    {
        _popup->hide();
        return;
    }

    _build_advanced_menu();
    _show_popup();
}

void OrchestratorScreenSelect::_build_advanced_menu()
{
    // Clear old screen list.
    while (_screen_list->get_child_count(false) > 0)
    {
        Node* child = _screen_list->get_child(0);
        _screen_list->remove_child(child);
        child->queue_free();
    }

    // Populate screen list
    const real_t height = real_t(get_theme_font_size("font_size")) * 1.5;

    int current_screen = get_window()->get_current_screen();
    for (int i = 0; i < DisplayServer::get_singleton()->get_screen_count(); i++)
    {
        Button* button = memnew(Button);

        Size2 screen_size = Size2(DisplayServer::get_singleton()->screen_get_size(i));
        Size2 button_size = Size2(height * (screen_size.x / screen_size.y), height);
        button->set_custom_minimum_size(button_size);
        _screen_list->add_child(button);

        button->set_text(itos(i));
        button->set_text_alignment(HORIZONTAL_ALIGNMENT_CENTER);
        button->set_tooltip_text(vformat("Make this panel floating in the screen %d.", i));

        if (i == current_screen)
        {
            Color accent_color = get_theme_color("accent_color", "Editor");
            button->add_theme_color_override("font_color", accent_color);
        }

        button->connect("pressed", callable_mp(this, &OrchestratorScreenSelect::_emit_screen_signal).bind(i));
        button->connect("pressed", callable_mp(static_cast<BaseButton*>(this), &OrchestratorScreenSelect::set_pressed).bind(false));
        button->connect("pressed", callable_mp(static_cast<Window*>(_popup), &Popup::hide));
    }
}

void OrchestratorScreenSelect::_emit_screen_signal(int p_screen_index)
{
    emit_signal("request_open_in_screen", p_screen_index);
}

void OrchestratorScreenSelect::_handle_mouse_shortcut(const Ref<InputEvent>& p_event)
{
    const Ref<InputEventMouseButton> mb = p_event;
    if (mb.is_valid())
    {
        if (mb->is_pressed() && mb->get_button_index() == MouseButton::MOUSE_BUTTON_LEFT)
        {
            _emit_screen_signal(get_window()->get_current_screen());
            accept_event();
        }
    }
}

void OrchestratorScreenSelect::_show_popup()
{
    if (!get_viewport())
        return;

    Size2 size = get_size() * get_viewport()->get_canvas_transform().get_scale();

    _popup->set_size(Size2(size.width, 0));

    Point2 gp = get_screen_position();
    gp.y += size.y;
    if (is_layout_rtl())
        gp.x += size.width - _popup->get_size().width;

    _popup->set_position(gp);
    _popup->popup();
}


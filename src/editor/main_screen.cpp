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
#include "editor/main_screen.h"

#include "common/macros.h"
#include "editor/editor.h"
#include "editor/plugins/orchestrator_editor_plugin.h"

#if GODOT_VERSION < 0x040800
    #include "common/godot_version.h"
    #include "editor/gui/window_wrapper.h"

    #include <godot_cpp/classes/display_server.hpp>
    #include <godot_cpp/classes/tab_container.hpp>
    #include <godot_cpp/classes/viewport.hpp>
#else
    #include <godot_cpp/classes/window.hpp>
#endif

OrchestratorEditorMainScreen* OrchestratorEditorMainScreen::create() {
    #if GODOT_VERSION < 0x040800
    return memnew(OrchestratorEditorMainScreenLegacy);
    #else
    return memnew(OrchestratorEditorMainScreenDock);
    #endif
}

#if GODOT_VERSION < 0x040800

void OrchestratorEditorMainScreenLegacy::_select_main_screen(const String& p_name) {
    if (_dock_managed) {
        // Godot 4.8 lets the user close any main-screen dock from the Docks menu. set_main_screen_editor
        // only searches the main screen's open tabs and fails with "editor name not found" for a closed
        // dock, whereas EditorDock::make_visible reopens it first, which is how the 2D editor returns.
        // Neither API exists in the bindings this build targets, so they are called dynamically.
        Object* dock = EI->call("get_dock_by_name", p_name);
        if (dock) {
            dock->call("make_visible");

            // Until the forced-focus bypass, make_visible and set_main_screen_editor both defer to
            // the current tab's allow_switch_screen, which the Script, Game and Asset Store tabs
            // disable. The user asked for this editor, so select its tab directly.
            Control* dock_control = cast_to<Control>(dock);
            if (dock_control && !dock_control->is_visible_in_tree()) {
                if (TabContainer* tabs = cast_to<TabContainer>(dock_control->get_parent())) {
                    tabs->set_current_tab(tabs->get_tab_idx_from_control(dock_control));
                }
            }
            return;
        }
    }

    EI->set_main_screen_editor(p_name);
}

void OrchestratorEditorMainScreenLegacy::_focus_another_editor() {
    if (_window_wrapper->get_window_enabled()) {
        ERR_FAIL_COND(_last_editor.is_empty());

        EI->get_base_control()->get_viewport()->gui_release_focus();
        _select_main_screen(_last_editor);
    }
}

void OrchestratorEditorMainScreenLegacy::_main_screen_changed(const String& p_name) {
    if (p_name != _plugin->_get_plugin_name()) {
        _last_editor = p_name;
    }
}

void OrchestratorEditorMainScreenLegacy::_window_visibility_changed(bool p_visible) {
    if (p_visible) {
        _focus_another_editor();
    } else {
        activate();
    }
}

void OrchestratorEditorMainScreenLegacy::attach(EditorPlugin* p_plugin) {
    _plugin = p_plugin;

    // A build against an older API can still be loaded by Godot 4.8, which reparents the wrapper
    // into a generated EditorDock and owns its visibility from then on.
    _dock_managed = GodotVersionInfo().at_least(4, 8);

    _window_wrapper->connect("window_visibility_changed", callable_mp_this(_window_visibility_changed));
    _plugin->connect("main_screen_changed", callable_mp_this(_main_screen_changed));

    EI->get_editor_main_screen()->add_child(_window_wrapper);

    make_visible(false);
}

void OrchestratorEditorMainScreenLegacy::detach(EditorPlugin* p_plugin) {
    p_plugin->disconnect("main_screen_changed", callable_mp_this(_main_screen_changed));
    _plugin = nullptr;

    SAFE_MEMDELETE(_editor);

    if (Node* parent = _window_wrapper->get_parent()) {
        parent->remove_child(_window_wrapper);
    }
    SAFE_MEMDELETE(_window_wrapper);

    memdelete(this);
}

void OrchestratorEditorMainScreenLegacy::make_visible(bool p_visible) {
    if (p_visible) {
        if (_window_wrapper->get_window_enabled()) {
            // EditorPlugin::selected_notify is not exposed to GDExtension, but this method
            // is called just before "selected_notify" as a way to address this until the
            // method can be exposed.
            _focus_another_editor();
        }
        _window_wrapper->show();
    }
    else if (!_dock_managed) {
        // Before Godot 4.8, the main screen calls this when another main-screen tab is selected.
        // From 4.8 onward, the dock's tab container owns visibility and this call instead arrives
        // from EditorNode::edit_item whenever the inspected object changes, so hiding here would
        // blank the content while the Orchestrator tab remains selected.
        _window_wrapper->hide();
    }
}

void OrchestratorEditorMainScreenLegacy::activate() {
    _select_main_screen(_plugin->_get_plugin_name());
}

void OrchestratorEditorMainScreenLegacy::move_to_foreground() {
    _window_wrapper->move_to_foreground();
}

void OrchestratorEditorMainScreenLegacy::set_window_layout(const Ref<ConfigFile>& p_configuration) {
    if (!OrchestratorPlugin::get_singleton()->restore_windows_on_load()) {
        return;
    }

    if (_window_wrapper->is_window_available() && p_configuration->has_section_key("Orchestrator", "window_rect")) {
        _window_wrapper->restore_window_from_saved_position(
            p_configuration->get_value("Orchestrator", "window_rect", Rect2i()),
            p_configuration->get_value("Orchestrator", "window_screen", -1),
            p_configuration->get_value("Orchestrator", "window_screen_rect", Rect2i()));
    } else {
        _window_wrapper->set_window_enabled(false);
    }
}

void OrchestratorEditorMainScreenLegacy::get_window_layout(const Ref<ConfigFile>& p_configuration) {
    if (_window_wrapper->get_window_enabled()) {
        const int screen = _window_wrapper->get_window_screen();
        p_configuration->set_value("Orchestrator", "window_rect", _window_wrapper->get_window_rect());
        p_configuration->set_value("Orchestrator", "window_screen", screen);
        p_configuration->set_value("Orchestrator", "window_screen_rect", DisplayServer::get_singleton()->screen_get_usable_rect(screen));
    } else {
        if (p_configuration->has_section_key("Orchestrator", "window_rect")) {
            p_configuration->erase_section_key("Orchestrator", "window_rect");
        }
        if (p_configuration->has_section_key("Orchestrator", "window_screen")) {
            p_configuration->erase_section_key("Orchestrator", "window_screen");
        }
        if (p_configuration->has_section_key("Orchestrator", "window_screen_rect")) {
            p_configuration->erase_section_key("Orchestrator", "window_screen_rect");
        }
    }
}

void OrchestratorEditorMainScreenLegacy::_bind_methods() {
}

OrchestratorEditorMainScreenLegacy::OrchestratorEditorMainScreenLegacy() {
    _window_wrapper = memnew(OrchestratorWindowWrapper);
    _window_wrapper->set_window_title("Orchestrator - Godot Engine");
    _window_wrapper->set_margins_enabled(true);
    _window_wrapper->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    _window_wrapper->hide();

    _editor = memnew(OrchestratorEditor(_window_wrapper));
    _window_wrapper->set_wrapped_control(_editor);
}

#else

void OrchestratorEditorMainScreenDock::attach(EditorPlugin* p_plugin) {
    set_title(p_plugin->_get_plugin_name());
    set_icon_name(p_plugin->_get_plugin_name());
    set_dock_icon(p_plugin->_get_plugin_icon());

    p_plugin->add_dock(this);
}

void OrchestratorEditorMainScreenDock::detach(EditorPlugin* p_plugin) {
    p_plugin->remove_dock(this);

    SAFE_MEMDELETE(_editor);

    memdelete(this);
}

void OrchestratorEditorMainScreenDock::make_visible(bool p_visible) {
    // Godot calls this from EditorNode::edit_item as the inspected object changes, not from tab
    // selection. Hiding is the tab container's job; showing selects the dock, as the engine's own
    // main-screen plugins do.
    if (p_visible) {
        EditorDock::make_visible();
    }
}

void OrchestratorEditorMainScreenDock::activate() {
    EditorDock::make_visible();
}

void OrchestratorEditorMainScreenDock::move_to_foreground() {
    // When the dock floats this is its own window; when docked it is the editor's main window.
    if (Window* window = get_window()) {
        window->grab_focus();
    }
}

void OrchestratorEditorMainScreenDock::set_window_layout(const Ref<ConfigFile>& p_configuration) {
    // The dock manager persists slot, tab and floating state under the layout key.
}

void OrchestratorEditorMainScreenDock::get_window_layout(const Ref<ConfigFile>& p_configuration) {
    // The dock manager persists slot, tab and floating state under the layout key.
}

void OrchestratorEditorMainScreenDock::_bind_methods() {
}

OrchestratorEditorMainScreenDock::OrchestratorEditorMainScreenDock() {
    // The key, not the title, identifies the dock in the saved editor layout.
    set_layout_key("Orchestrator");
    set_default_slot(EditorDock::DOCK_SLOT_MAIN_SCREEN);
    set_available_layouts(EditorDock::DOCK_LAYOUT_MAIN_SCREEN | EditorDock::DOCK_LAYOUT_FLOATING);

    // Selecting a 2D or 3D node must not switch the main screen away from Orchestrator.
    set_allow_switch_screen(false);

    set_v_size_flags(Control::SIZE_EXPAND_FILL);

    _editor = memnew(OrchestratorEditor(nullptr));
    add_child(_editor);
}

#endif

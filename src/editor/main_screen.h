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
#pragma once

#include "common/version.h"

#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/object.hpp>

#if GODOT_VERSION >= 0x040800
    #include <godot_cpp/classes/editor_dock.hpp>
#endif

using namespace godot;

/// Forward declarations
class OrchestratorEditor;
class OrchestratorWindowWrapper;

/// Hosts the Orchestrator editor panel in Godot's main screen and owns the panel.
///
/// Godot 4.8 replaced main-screen plugins with EditorDock; earlier versions place a plugin control
/// into the main screen container and drive its visibility through EditorPlugin::_make_visible.
/// The plugin talks to this interface and never learns which host it runs against. The choice is
/// made at compile time because EditorDock only exists in the 4.8 bindings.
class OrchestratorEditorMainScreen {
public:
    /// Creates the host for the Godot API this build targets, along with the editor panel.
    static OrchestratorEditorMainScreen* create();

    virtual ~OrchestratorEditorMainScreen() = default;

    virtual OrchestratorEditor* get_editor() const = 0;

    /// Adds the editor panel to the main screen; called from the plugin's ENTER_TREE.
    virtual void attach(EditorPlugin* p_plugin) = 0;

    /// Removes the editor panel from the main screen and frees the panel and this host.
    /// The host pointer is invalid once this returns.
    virtual void detach(EditorPlugin* p_plugin) = 0;

    /// Forwards EditorPlugin::_make_visible.
    virtual void make_visible(bool p_visible) = 0;

    /// Selects the Orchestrator main screen.
    virtual void activate() = 0;

    /// Brings the editor panel to the front when it is floating.
    virtual void move_to_foreground() = 0;

    /// Forwards EditorPlugin::_set_window_layout for host-owned state.
    virtual void set_window_layout(const Ref<ConfigFile>& p_configuration) = 0;

    /// Forwards EditorPlugin::_get_window_layout for host-owned state.
    virtual void get_window_layout(const Ref<ConfigFile>& p_configuration) = 0;
};

#if GODOT_VERSION < 0x040800

/// Main-screen host for Godot versions before 4.8.
///
/// Wraps the editor panel in OrchestratorWindowWrapper, adds it to the legacy main screen container
/// and emulates the editor's own floating-window behavior: when the panel is detached, the main
/// screen falls back to the last non-Orchestrator editor.
class OrchestratorEditorMainScreenLegacy : public Object, public OrchestratorEditorMainScreen {
    GDCLASS(OrchestratorEditorMainScreenLegacy, Object);

    OrchestratorWindowWrapper* _window_wrapper = nullptr;
    OrchestratorEditor* _editor = nullptr;
    EditorPlugin* _plugin = nullptr;
    String _last_editor;
    bool _dock_managed = false;     //! Godot 4.8 hosts the wrapper inside a generated EditorDock

    void _select_main_screen(const String& p_name);
    void _focus_another_editor();

    //~ Begin Signals
    void _main_screen_changed(const String& p_name);
    void _window_visibility_changed(bool p_visible);
    //~ End Signals

protected:
    static void _bind_methods();

public:
    //~ Begin OrchestratorEditorMainScreen Interface
    OrchestratorEditor* get_editor() const override { return _editor; }
    void attach(EditorPlugin* p_plugin) override;
    void detach(EditorPlugin* p_plugin) override;
    void make_visible(bool p_visible) override;
    void activate() override;
    void move_to_foreground() override;
    void set_window_layout(const Ref<ConfigFile>& p_configuration) override;
    void get_window_layout(const Ref<ConfigFile>& p_configuration) override;
    //~ End OrchestratorEditorMainScreen Interface

    OrchestratorEditorMainScreenLegacy();
};

#else

/// Main-screen host for Godot 4.8 and later.
///
/// An EditorDock in the main-screen slot. Godot owns tab selection, floating and layout
/// persistence, so the plugin-side visibility bookkeeping of the legacy host does not exist here.
class OrchestratorEditorMainScreenDock : public EditorDock, public OrchestratorEditorMainScreen {
    GDCLASS(OrchestratorEditorMainScreenDock, EditorDock);

    OrchestratorEditor* _editor = nullptr;

protected:
    static void _bind_methods();

public:
    using EditorDock::make_visible;

    //~ Begin OrchestratorEditorMainScreen Interface
    OrchestratorEditor* get_editor() const override { return _editor; }
    void attach(EditorPlugin* p_plugin) override;
    void detach(EditorPlugin* p_plugin) override;
    void make_visible(bool p_visible) override;
    void activate() override;
    void move_to_foreground() override;
    void set_window_layout(const Ref<ConfigFile>& p_configuration) override;
    void get_window_layout(const Ref<ConfigFile>& p_configuration) override;
    //~ End OrchestratorEditorMainScreen Interface

    OrchestratorEditorMainScreenDock();
};

#endif

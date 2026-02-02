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
#ifndef ORCHESTRATOR_EDITOR_PLUGIN_H
#define ORCHESTRATOR_EDITOR_PLUGIN_H

#include "editor/debugger/script_debugger_plugin.h"

#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/editor_inspector_plugin.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/texture2d.hpp>

using namespace godot;

/// Forward declarations
class OrchestratorEditor;
class OrchestratorWindowWrapper;

/// The Orchestrator editor plug-in.
class OrchestratorPlugin : public EditorPlugin {
    GDCLASS(OrchestratorPlugin, EditorPlugin);

    static OrchestratorPlugin* _plugin;

    String _last_editor;                                      //! Last editor
    OrchestratorEditor* _editor_panel = nullptr;              //! Plugin's editor panel
    OrchestratorWindowWrapper* _window_wrapper = nullptr;     //! Window wrapper
    Vector<Ref<EditorInspectorPlugin>> _inspector_plugins;

    static String _get_orchestrator_metedata_path();

    void _focus_another_editor();
    bool _is_exiting() const;

    void _register_plugins();

    bool _is_plugin_just_installed() const;

    void _add_plugin_icon_to_editor_theme();

    template <typename T>
    void _register_inspector_plugin() {
        Ref<T> plugin;
        plugin.instantiate();
        _inspector_plugins.push_back(plugin);
        add_inspector_plugin(plugin);
    }

    template <typename T>
    void _register_export_plugin() {
        Ref<T> plugin;
        plugin.instantiate();
        add_export_plugin(plugin);
    }

    template <typename T>
    void _register_debugger_plugin() {
        Ref<T> plugin;
        plugin.instantiate();
        add_debugger_plugin(plugin);
    }

    //~ Begin Signals
    void _main_screen_changed(const String& p_name);
    void _window_visibility_changed(bool p_visible);
    //~ End Signals

protected:
    static void _bind_methods();

public:
    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

    //~ Begin EditorPlugin interface
    String get_plugin_version() const; // NOLINT
    void _edit(Object* p_object) override;
    bool _handles(Object* p_object) const override;
    bool _has_main_screen() const override;
    void _make_visible(bool p_visible) override;
    String _get_plugin_name() const override;
    Ref<Texture2D> _get_plugin_icon() const override;
    void _save_external_data() override;
    String _get_unsaved_status(const String& p_for_scene) const override;
    void _apply_changes() override;
    void _set_window_layout(const Ref<ConfigFile>& configuration) override;
    void _get_window_layout(const Ref<ConfigFile>& configuration) override;
    bool _build() override;
    void _enable_plugin() override;
    void _disable_plugin() override;
    PackedStringArray _get_breakpoints() const override;
    //~ End EditorPlugin interface

    static OrchestratorPlugin* get_singleton() { return _plugin; }

    static String get_github_issues_url();
    static String get_patreon_url();
    static String get_community_url();
    static String get_plugin_online_documentation_url();

    bool restore_windows_on_load();
    void request_editor_restart();

    Ref<Texture2D> get_plugin_icon_hires() const;

    Ref<ConfigFile> get_metadata();
    void save_metadata(const Ref<ConfigFile>& p_metadata);

    void make_active();

    template<typename T>
    Ref<T> get_editor_inspector_plugin() {
        for (Ref<EditorInspectorPlugin>& plugin : _inspector_plugins) {
            if (T* result = cast_to<T>(plugin.ptr())) {
                return result;
            }
        }
        return {};
    }

    OrchestratorPlugin();
};

#endif  // ORCHESTRATOR_EDITOR_PLUGIN_H
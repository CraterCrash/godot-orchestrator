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

#include "editor/build_output_panel.h"
#include "editor/editor_cache.h"
#include "editor/plugins/orchestrator_editor_debugger_plugin.h"
#include "editor/theme/theme_cache.h"

#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/editor_export_plugin.hpp>
#include <godot_cpp/classes/editor_inspector_plugin.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/texture2d.hpp>

using namespace godot;

/// Forward declarations
class OrchestratorEditorPanel;
class OrchestratorWindowWrapper;

/// The Orchestrator editor plug-in.
class OrchestratorPlugin : public EditorPlugin
{
    GDCLASS(OrchestratorPlugin, EditorPlugin);

    static void _bind_methods();

    static OrchestratorPlugin* _plugin;

    EditorInterface& _editor;                                 //! Godot editor interface reference
    OrchestratorEditorPanel* _editor_panel{ nullptr };        //! Plugin's editor panel
    OrchestratorWindowWrapper* _window_wrapper{ nullptr };    //! Window wrapper
    Vector<Ref<EditorInspectorPlugin>> _inspector_plugins;
    Vector<Ref<EditorExportPlugin>> _export_plugins;
    Ref<OrchestratorThemeCache> _theme_cache;
    Ref<OrchestratorEditorCache> _editor_cache;               //! Script editor cache
    OrchestratorBuildOutputPanel* _build_panel{ nullptr };    //! Build panel
    #if GODOT_VERSION >= 0x040300
    Ref<OrchestratorEditorDebuggerPlugin> _debugger_plugin;   //! Debugger plugin
    #endif

public:
    /// Constructor
    OrchestratorPlugin();

    /// Get the plugin instance, only valid within the Godot Editor.
    /// @return the plugin instance
    static OrchestratorPlugin* get_singleton() { return _plugin; }

    /// Handle Godot's notification callbacks
    /// @param p_what the notification type
    void _notification(int p_what);

    /// Get the plugin's online documentation URL
    /// @return the online documentation URL
    String get_plugin_online_documentation_url() const;

    String get_github_release_url() const;
    String get_github_release_tag_url(const String& p_tag);
    String get_github_release_notes_url(const String& p_tag);
    String get_github_issues_url() const;
    String get_patreon_url() const;
    String get_community_url() const;

    /// Returns whether windows are restored on load
    /// @return true if windows are to be restored, false otherwise
    bool restore_windows_on_load();

    /// Requests to restart the editor
    void request_editor_restart();

    /// Get the plugin's high-resolution logo / icon
    /// @return high-res texture
    Ref<Texture2D> get_plugin_icon_hires() const;

    /// Get the plugin's editor metadata configuration
    /// @return the metadata configuration file
    Ref<ConfigFile> get_metadata();

    /// Saves the metadata
    /// @param p_metadata the metadata to save
    void save_metadata(const Ref<ConfigFile>& p_metadata);

    /// Makes this plugin's view active, if it isn't already.
    void make_active();

    Ref<OrchestratorThemeCache> get_theme_cache() { return _theme_cache; }
    Ref<OrchestratorEditorCache> get_editor_cache() { return _editor_cache; }

    /// Sets the build panel as active
    void make_build_panel_active();

    /// Get a reference to the build output panel
    /// @return the build output panel, should never be <code>null</code>
    OrchestratorBuildOutputPanel* get_build_panel() const { return _build_panel; }

    //~ Begin EditorPlugin interface
    String get_plugin_version() const;
    void _edit(Object* p_object) override;
    bool _handles(Object* p_object) const override;
    bool _has_main_screen() const override;
    void _make_visible(bool p_visible) override;
    String _get_plugin_name() const override;
    Ref<Texture2D> _get_plugin_icon() const override;
    void _apply_changes() override;
    void _set_window_layout(const Ref<ConfigFile>& configuration) override;
    void _get_window_layout(const Ref<ConfigFile>& configuration) override;
    bool _build() override;
    void _enable_plugin() override;
    void _disable_plugin() override;
    PackedStringArray _get_breakpoints() const override;
    //~ End EditorPlugin interface

    /// Get the editor inspector plugin by type
    /// @return the editor inspector plugin reference or an invalid reference if not found
    template<typename T>
    Ref<T> get_editor_inspector_plugin()
    {
        for (Ref<EditorInspectorPlugin>& plugin : _inspector_plugins)
        {
            if (T* result = Object::cast_to<T>(plugin.ptr()))
                return result;
        }
        return {};
    }

private:
    void _on_window_visibility_changed(bool p_visible);
    void _on_editor_restart();
};

#endif  // ORCHESTRATOR_EDITOR_PLUGIN_H
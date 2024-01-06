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
#ifndef ORCHESTRATOR_PLUGIN_H
#define ORCHESTRATOR_PLUGIN_H

#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/texture2d.hpp>

using namespace godot;

/// Forward declarations
class OrchestratorMainView;
class OrchestratorWindowWrapper;

/// The Orchestrator editor plug-in.
class OrchestratorPlugin : public EditorPlugin
{
    GDCLASS(OrchestratorPlugin, EditorPlugin);

    static void _bind_methods();

    static OrchestratorPlugin* _plugin;

    EditorInterface& _editor;                                 //! Godot editor interface reference
    OrchestratorMainView* _main_view{ nullptr };              //! Plugin's main view
    OrchestratorWindowWrapper* _window_wrapper{ nullptr };    //! Window wrapper
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
    String get_patreon_url() const;

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
    //~ End EditorPlugin interface

private:
    void _on_window_visibility_changed(bool p_visible);
};

void register_plugin_classes();

#endif  // ORCHESTRATOR_PLUGIN_H
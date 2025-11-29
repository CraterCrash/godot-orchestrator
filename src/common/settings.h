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
#ifndef ORCHESTRATOR_SETTINGS_H
#define ORCHESTRATOR_SETTINGS_H

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/templates/vector.hpp>

using namespace godot;

class OrchestratorSettings : public Object
{
    GDCLASS(OrchestratorSettings, Object);

    static void _bind_methods() {}

public:
    struct Setting
    {
        PropertyInfo info;
        Variant value;

        explicit Setting(const PropertyInfo& p_info, const Variant& p_value) : info(p_info), value(p_value) {}
    };

private:
    static OrchestratorSettings* _singleton;
    std::vector<Setting> _removed;
    std::vector<Setting> _settings;
    int _builtin_order{ 1000 };

public:
    OrchestratorSettings();
    ~OrchestratorSettings() override;

    static OrchestratorSettings* get_singleton() { return _singleton; }

    /// Check whether the specified setting exists
    /// @param p_key the setting to lookup
    /// @return true if the setting exists, false otherwise
    bool has_setting(const String& p_key) const;

    /// Get the value of setting
    /// @param p_key the setting key to find
    /// @param p_default_value the default value to use if key does not exist
    /// @return the found value or the specified default
    Variant get_setting(const String& p_key, const Variant& p_default_value = Variant());

    /// Set the value of a setting
    /// @param p_key the setting key to set
    /// @param p_value the value to set
    void set_setting(const String& p_key, const Variant& p_value);

    /// Get all currently defined action favorites.
    /// @return A <code>PackedStringArray</code> of all action category favorites.
    PackedStringArray get_action_favorites();

    /// Add an action category favorite.
    /// @param p_action_name the action category to be added
    void add_action_favorite(const String& p_action_name);

    /// Removes an action category favorite.
    /// @param p_action_name the action category to be removed
    void remove_action_favorite(const String& p_action_name);

    /// Return whether to notify about pre-releases
    /// @return true to notify about pre-releases, false otherwise
    bool is_notify_about_prereleases() { return get_setting("settings/notify_about_pre-releases", true); }

    /// Set whether to notify pre-release builds
    /// @param p_notify_prerelease_builds true to notify about pre-releases, false for only stable releases
    void set_notify_prerelease_builds(bool p_notify_prerelease_builds);

    const std::vector<Setting>& get_settings() const { return _settings; }

private:

    /// Get the base settings key
    /// @return the base setting key
    String _get_base_key() const { return "orchestrator"; }

    /// Register deprecated settings
    void _register_deprecated_settings();

    /// Register current usable settings
    void _register_settings();

    /// Initializes the default settings
    /// This is useful when starting the plugin for the first time to seed the project settings
    void _initialize_settings();

    /// Performs any update operations on the settings
    /// This handles any migration of settings from an older version to the current version.
    void _update_default_settings();
};

#define ORCHESTRATOR_GET(x, y) OrchestratorSettings::get_singleton()->get_setting(x, y)

#endif  // ORCHESTRATOR_SETTINGS_H
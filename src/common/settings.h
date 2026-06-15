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

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/templates/vector.hpp>

using namespace godot;

class OrchestratorSettings : public Object {
    GDCLASS(OrchestratorSettings, Object);

    static OrchestratorSettings* _singleton;

    /// The registered settings, keyed by their *logical* name (without the "orchestrator/" storage
    /// prefix). This is the single source of truth for the set of Orchestrator settings: it backs the
    /// Orchestrator settings dialog (so the settings render even though they are internal in
    /// ProjectSettings, and thus hidden from the native Project Settings dialog) and the name list.
    Vector<PropertyInfo> _editor_properties;
    HashMap<String, String> _editor_property_descriptions;

    int _builtin_order = (1 << 16) + 1;

    String _current_theme;
    bool _applying_preset = false;

    void _project_settings_changed();

    void _apply_color_theme_preset(const String& p_theme);

    Variant _define(const String& p_name, const Variant& p_default, bool p_restart_if_changed = false, bool p_ignore_in_docs = false, bool p_basic = false, bool p_internal = false, const PropertyInfo* p_property_info = nullptr);
    Variant _define(const PropertyInfo& p_property, const Variant& p_default, bool p_restart_if_changed = false, bool p_ignore_in_docs = false, bool p_basic = false, bool p_internal = false);

    void _rename(const String& p_old_name, const String& p_new_name);

    static String _make_key(const String& p_key);

    void _initialize_settings();
    void _rename_settings();
    void _remove_deprecated_settings();

protected:
    static void _bind_methods();

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _property_can_revert(const StringName& p_name) const;
    bool _property_get_revert(const StringName& p_name, Variant& r_value) const;
    //~ End Wrapped Interface

public:
    static OrchestratorSettings* get_singleton() { return _singleton; }

    static void create();
    static void destroy();

    bool has_setting(const String& p_key) const;
    Variant get_setting(const String& p_key, const Variant& p_default_value = Variant());
    void set_setting(const String& p_key, const Variant& p_value);

    bool is_notify_about_prereleases();
    void set_notify_prerelease_builds(bool p_notify_prerelease_builds);

    PackedStringArray get_settings_name_list() const;

    String get_property_description(const String& p_property);

    OrchestratorSettings();
    ~OrchestratorSettings() override;
};

#define ORCHESTRATOR_GET(x, y) OrchestratorSettings::get_singleton()->get_setting(x, y)
#define ORCHESTRATOR_GET_ENUM(t, x, y) static_cast<t>(static_cast<int>(ORCHESTRATOR_GET(x, y)))

template <typename T>
_FORCE_INLINE_ bool ORCHESTRATOR_GET_TRACK(T& r_field, const String& p_key, T p_default) {
    T new_value = ORCHESTRATOR_GET(p_key, p_default);
    if (new_value != r_field) {
        r_field = new_value;
        return true;
    }
    return false;
}

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
#ifndef ORCHESTRATOR_CORE_GODOT_CONFIG_PROJECT_SETTINGS_CACHE_H
#define ORCHESTRATOR_CORE_GODOT_CONFIG_PROJECT_SETTINGS_CACHE_H

#include <godot_cpp/templates/hash_map.hpp>

using namespace godot;

/// A simple cache object that maintains state observed from Godot's <code>ProjectSettings</code> class.
///
/// This allows us to avoid several expensive operations across the plugin by caching changes into data
/// structures that are easily consumed by Orchestrator.
///
class OrchestratorProjectSettingsCache : public Object {
    GDCLASS(OrchestratorProjectSettingsCache, Object);

public:
    struct AutoloadInfo {
        StringName name;
        String path;
        String uid;
        bool singleton = false;
    };

    using Entry = KeyValue<StringName, AutoloadInfo>;

private:
    static OrchestratorProjectSettingsCache* _singleton;

    const AutoloadInfo EMPTY;
    HashMap<StringName, AutoloadInfo> _autoloads;

    void _settings_changed();

protected:
    static void _bind_methods();

public:
    _FORCE_INLINE_ static OrchestratorProjectSettingsCache* get_singleton() { return _singleton; }

    static void create();
    static void destroy();

    bool has_autoload(const StringName& p_name);
    bool has_singleton_autoload(const StringName& p_name);
    const AutoloadInfo& get_autoload(const StringName& p_name) const;

    PackedStringArray get_autoload_names() const;
    const HashMap<StringName, AutoloadInfo>& get_autoloads() const;

    OrchestratorProjectSettingsCache();
};

#endif // ORCHESTRATOR_CORE_GODOT_CONFIG_PROJECT_SETTINGS_CACHE_H
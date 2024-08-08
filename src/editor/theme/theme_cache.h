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
#ifndef ORCHESTRATOR_THEME_CACHE_H
#define ORCHESTRATOR_THEME_CACHE_H

#include "godot_cpp/templates/hash_map.hpp"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/style_box.hpp>

using namespace godot;

/// A simple class that manages the themes used by the Orchestrator plugin.
class OrchestratorThemeCache : public RefCounted
{
    GDCLASS(OrchestratorThemeCache, RefCounted);
    static void _bind_methods() {}

protected:
    HashMap<StringName, HashMap<StringName, Ref<StyleBox>>> _stylebox_cache;

    //~ Begin Signal Handlers
    void _settings_changed();
    //~ End Signal Handlers

public:
    //~ Begn Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

    /// Adds a theme stylebox to the cache
    /// @param p_name the name
    /// @param p_type_name the type name
    /// @param p_stylebox the stylebox to cache
    void add_theme_stylebox(const StringName& p_name, const String& p_type_name, const Ref<StyleBox>& p_stylebox);

    /// Get a theme stylebox from the cache
    /// @param p_name the name
    /// @param p_type_name the type name
    /// @returns the stylebox from the cache or an invalid stylebox reference if it doesn't exist
    Ref<StyleBox> get_theme_stylebox(const StringName& p_name, const String& p_type_name) const;

    /// Gets the editor theme stylebox
    /// @param p_name the item name
    /// @param p_type_name the type name
    /// @return the editor stylebox theme, should always be valid
    Ref<StyleBox>_get_editor_theme_stylebox(const String& p_name, const String& p_type_name) const;
};

#endif // ORCHESTRATOR_THEME_CACHE_H
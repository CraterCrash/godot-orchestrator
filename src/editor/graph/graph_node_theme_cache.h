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
#ifndef ORCHESTRATOR_EDITOR_GRAPH_NODE_THEME_CACHE_H
#define ORCHESTRATOR_EDITOR_GRAPH_NODE_THEME_CACHE_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/style_box.hpp>
#include <godot_cpp/templates/hash_map.hpp>

using namespace godot;

/// A cache that maintains common graph node theme state for all node types.
/// This allows for storing a set of lookups from ThemeDB by object, and reused by <code>GraphNode</code> instances.
class OrchestratorEditorGraphNodeThemeCache : public RefCounted {
    GDCLASS(OrchestratorEditorGraphNodeThemeCache, RefCounted);

    using StyleBoxMap = HashMap<StringName, Ref<StyleBox>>;
    HashMap<StringName, StyleBoxMap> _cache;

protected:
    static void _bind_methods();

    //~ Begin Signal Handlers
    void _settings_changed();
    //~ End Signal Handlers

public:
    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

    void add_theme_stylebox(const StringName& p_name, const String& p_type_name, const Ref<StyleBox>& p_stylebox);
    Ref<StyleBox> get_theme_stylebox(const StringName& p_name, const String& p_type_name);

};

#endif // ORCHESTRATOR_EDITOR_GRAPH_NODE_THEME_CACHE_H
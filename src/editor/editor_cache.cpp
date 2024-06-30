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
#include "editor/editor_cache.h"
#include "plugins/orchestrator_editor_debugger_plugin.h"
#include "plugins/orchestrator_editor_plugin.h"

#include <godot_cpp/classes/editor_paths.hpp>

#if GODOT_VERSION >= 0x040300
PackedInt64Array OrchestratorEditorCache::_get_breakpoints_for_path(const String& p_path, bool p_disabled) const
{
    return _cache->get_value(p_path, p_disabled ? "disabled_breakpoints" : "breakpoints", PackedInt64Array());
}
#endif

Error OrchestratorEditorCache::load()
{
    const EditorInterface* ei = OrchestratorPlugin::get_singleton()->get_editor_interface();

    _cache = Ref<ConfigFile>(memnew(ConfigFile));

    Error result = _cache->load(ei->get_editor_paths()->get_project_settings_dir().path_join(CACHE_FILE));
    if (result != OK)
        return result;

    #if GODOT_VERSION >= 0x040300
    if (OrchestratorEditorDebuggerPlugin* debugger = OrchestratorEditorDebuggerPlugin::get_singleton())
    {
        for (const String& section : _cache->get_sections())
        {
            if (_cache->has_section_key(section, "breakpoints"))
            {
                PackedInt64Array breakpoints = _get_breakpoints_for_path(section, false);
                for (int i = 0; i < breakpoints.size(); i++)
                    debugger->set_breakpoint(section, breakpoints[i], true);
            }
            if (_cache->has_section_key(section, "disabled_breakpoints"))
            {
                PackedInt64Array disabled_breakpoints = _get_breakpoints_for_path(section, true);
                for (int i = 0; i < disabled_breakpoints.size(); i++)
                    debugger->set_breakpoint(section, disabled_breakpoints[i], true);
            }
        }
    }
    #endif

    return OK;
}

Error OrchestratorEditorCache::save()
{
    if (_cache.is_valid())
    {
        const EditorInterface* ei = OrchestratorPlugin::get_singleton()->get_editor_interface();
        return _cache->save(ei->get_editor_paths()->get_project_settings_dir().path_join(CACHE_FILE));
    }

    return ERR_FILE_CANT_WRITE;
}

#if GODOT_VERSION >= 0x040300
void OrchestratorEditorCache::clear_all_breakpoints()
{
    for (const String& section : _cache->get_sections())
    {
        PackedInt64Array breakpoints = _get_breakpoints_for_path(section, false);
        if (!breakpoints.is_empty())
            _cache->set_value(section, "breakpoints", Variant());

        PackedInt64Array disabled_breakpoints = _get_breakpoints_for_path(section, true);
        if (!disabled_breakpoints.is_empty())
            _cache->set_value(section, "disabled_breakpoints", Variant());
    }
}

bool OrchestratorEditorCache::is_node_breakpoint(const String& p_path, int p_node_id) const
{
    return _get_breakpoints_for_path(p_path, false).has(p_node_id);
}

bool OrchestratorEditorCache::is_node_disabled_breakpoint(const String& p_path, int p_node_id) const
{
    return _get_breakpoints_for_path(p_path, true).has(p_node_id);
}

void OrchestratorEditorCache::set_breakpoint(const String& p_path, int p_node_id, bool p_enabled)
{
    PackedInt64Array breakpoints = _get_breakpoints_for_path(p_path, false);
    if (p_enabled && !breakpoints.has(p_node_id))
    {
        breakpoints.push_back(p_node_id);
        _cache->set_value(p_path, "breakpoints", breakpoints);
    }
    else if (!p_enabled && breakpoints.has(p_node_id))
    {
        breakpoints.remove_at(breakpoints.find(p_node_id));
        _cache->set_value(p_path, "breakpoints", breakpoints);
    }
}

void OrchestratorEditorCache::set_disabled_breakpoint(const String& p_path, int p_node_id, bool p_remove)
{
    PackedInt64Array breakpoints = _get_breakpoints_for_path(p_path, true);
    if (p_remove && breakpoints.has(p_node_id))
    {
        breakpoints.remove_at(breakpoints.find(p_node_id));
        _cache->set_value(p_path, "disabled_breakpoints", breakpoints);
    }
    else if (!p_remove && !breakpoints.has(p_node_id))
    {
        breakpoints.push_back(p_node_id);
        _cache->set_value(p_path, "disabled_breakpoints", breakpoints);
    }
}
#endif

void OrchestratorEditorCache::_bind_methods()
{
}
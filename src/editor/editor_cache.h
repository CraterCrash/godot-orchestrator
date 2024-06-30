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
#ifndef ORCHESTRATOR_EDITOR_CACHE_H
#define ORCHESTRATOR_EDITOR_CACHE_H

#include "common/version.h"

#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/ref_counted.hpp>

using namespace godot;

/// A simple cache that maintains editor state details for Orchestrator.
class OrchestratorEditorCache : public RefCounted
{
    GDCLASS(OrchestratorEditorCache, RefCounted);
    static void _bind_methods();

protected:
    const String CACHE_FILE = "orchestrator_editor_cache.cfg";  //! The cache file name
    Ref<ConfigFile> _cache;                                     //! Cache file

    #if GODOT_VERSION >= 0x040300
    /// Get breakpoints for the specified path
    /// @param p_path the path
    /// @param p_disabled when true, returns disabled breakpoints, false returns enabled breakpoints
    /// @return array of breakpoints
    PackedInt64Array _get_breakpoints_for_path(const String& p_path, bool p_disabled) const;
    #endif

public:
    /// Loads the script editor cache from disk.
    /// @return the error code based on the load operation
    Error load();

    /// Saves the script editor cache to disk.
    /// @return the error code based on the save operation
    Error save();

    #if GODOT_VERSION >= 0x040300
    /// Clears all breakpoints
    void clear_all_breakpoints();

    /// Check whether the node in a script is a breakpoint
    /// @param p_path the script path
    /// @param p_node_id the node id
    /// @return true if the node is a breakpoint, false otherwise
    bool is_node_breakpoint(const String& p_path, int p_node_id) const;

    /// Check whether the node in a script is a breakpoint and is disabled
    /// @param p_path the script path
    /// @param p_node_id the node id
    /// @return true if the node is a breakpoint that is disabled, false otherwise
    bool is_node_disabled_breakpoint(const String& p_path, int p_node_id) const;

    /// Set whether a breakpoint is enabled
    /// @param p_path the script path
    /// @param p_node_id the node id
    /// @param p_enabled whether the breakpoint is enabled or disabled
    void set_breakpoint(const String& p_path, int p_node_id, bool p_enabled);

    /// Set the breakpoint as disabled
    /// @param p_path the script path
    /// @param p_node_id the node id
    /// @param p_remove whether to remove the disabled entry
    void set_disabled_breakpoint(const String& p_path, int p_node_id, bool p_remove = false);
    #endif

};

#endif // ORCHESTRATOR_EDITOR_CACHE_H
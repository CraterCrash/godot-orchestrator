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
#ifndef ORCHESTRATOR_EDITOR_DEBUGGER_PLUGIN_H
#define ORCHESTRATOR_EDITOR_DEBUGGER_PLUGIN_H

#include "common/version.h"

#if GODOT_VERSION >= 0x040300
#include <godot_cpp/classes/editor_debugger_plugin.hpp>
#include <godot_cpp/classes/editor_debugger_session.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/templates/hash_map.hpp>

using namespace godot;

/// Provides Orchestrator with Godot editor debugger integration
class OrchestratorEditorDebuggerPlugin : public EditorDebuggerPlugin
{
    GDCLASS(OrchestratorEditorDebuggerPlugin, EditorDebuggerPlugin);
    static void _bind_methods();

protected:
    static OrchestratorEditorDebuggerPlugin* _singleton;  //! Singleton instance
    Ref<EditorDebuggerSession> _current_session;          //! Current debugger session

    //~ Begin Signal Handlers
    void _session_started(int32_t p_session_id);
    void _session_stopped(int32_t p_session_id);
    void _session_breaked(int32_t p_session_id);
    void _session_continued(int32_t p_session_id);
    //~ End Signal Handlers

public:
    //~ Begin EditorDebuggerPlugin Interface
    void _setup_session(int32_t p_session_id) override;
    void _goto_script_line(const Ref<Script>& p_script, int p_line) override;
    void _breakpoints_cleared_in_tree() override;
    void _breakpoint_set_in_tree(const Ref<Script>& p_script, int p_line, bool p_enabled) override;
    //~ End EditorDebuggerPlugin Interface

    /// Get the singleton instance for this plugin
    /// @return the singleton instance
    static OrchestratorEditorDebuggerPlugin* get_singleton() { return _singleton; }

    /// Set a specific breakpoint state for a given script file and line number
    /// @param p_file the script file
    /// @param p_line the line number, mapped to an Orchestrator script node ID
    /// @param p_enabled whether the breakpoint is enabled
    void set_breakpoint(const String& p_file, int32_t p_line, bool p_enabled);

    /// Constructor
    OrchestratorEditorDebuggerPlugin();

    /// Destructor
    ~OrchestratorEditorDebuggerPlugin() override;
};
#endif

#endif  // ORCHESTRATOR_EDITOR_DEBUGGER_PLUGIN_H
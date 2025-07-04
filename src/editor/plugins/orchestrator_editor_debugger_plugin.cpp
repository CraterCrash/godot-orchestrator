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
#include "editor/plugins/orchestrator_editor_debugger_plugin.h"

#if GODOT_VERSION >= 0x040300
OrchestratorEditorDebuggerPlugin* OrchestratorEditorDebuggerPlugin::_singleton = nullptr;

void OrchestratorEditorDebuggerPlugin::_session_started(int32_t p_session_id)
{
    // Session id is 0, when game starts.
}

void OrchestratorEditorDebuggerPlugin::_session_stopped(int32_t p_session_id)
{
    // Session id is 0, when game ends.
}

void OrchestratorEditorDebuggerPlugin::_session_breaked(bool p_can_debug, int32_t p_session_id)
{
    // Always reports session id with 1 when breakpoints happen
}

void OrchestratorEditorDebuggerPlugin::_session_continued(int32_t p_session_id)
{
    // Reports continuation of breakpoints with session id 0
    // Why is there a disconnect between break and continue?
}

void OrchestratorEditorDebuggerPlugin::_setup_session(int32_t p_session_id)
{
    Ref<EditorDebuggerSession> session = get_session(p_session_id);
    if (session.is_valid())
    {
        _current_session = session;

        session->connect("started", callable_mp(this, &OrchestratorEditorDebuggerPlugin::_session_started).bind(p_session_id));
        session->connect("stopped", callable_mp(this, &OrchestratorEditorDebuggerPlugin::_session_stopped).bind(p_session_id));
        session->connect("breaked", callable_mp(this, &OrchestratorEditorDebuggerPlugin::_session_breaked).bind(p_session_id));
        session->connect("continued", callable_mp(this, &OrchestratorEditorDebuggerPlugin::_session_continued).bind(p_session_id));
    }
}

void OrchestratorEditorDebuggerPlugin::_goto_script_line(const Ref<Script>& p_script, int p_line)
{
    emit_signal("goto_script_line", p_script, p_line + 1);
}

void OrchestratorEditorDebuggerPlugin::_breakpoints_cleared_in_tree()
{
    emit_signal("breakpoints_cleared_in_tree");
}

void OrchestratorEditorDebuggerPlugin::_breakpoint_set_in_tree(const Ref<Script>& p_script, int p_line, bool p_enabled)
{
    emit_signal("breakpoint_set_in_tree", p_script, p_line + 1, p_enabled);
}

void OrchestratorEditorDebuggerPlugin::set_breakpoint(const String& p_file, int32_t p_line, bool p_enabled)
{
    if (!_current_session.is_valid())
        return;

    _current_session->set_breakpoint(p_file, p_line, p_enabled);
}

void OrchestratorEditorDebuggerPlugin::reload_all_scripts()
{
    if (!_current_session.is_valid())
        return;

    _current_session->send_message("reload_all_scripts", Array());
}

void OrchestratorEditorDebuggerPlugin::reload_scripts(const Vector<String>& p_script_paths)
{
    if (!_current_session.is_valid())
        return;

    Array scripts;
    for (const String& value : p_script_paths)
        scripts.push_back(value);

    _current_session->send_message("reload_scripts", scripts);
}

void OrchestratorEditorDebuggerPlugin::_bind_methods()
{
    ADD_SIGNAL(MethodInfo("breaked", PropertyInfo(Variant::BOOL, "breaked"), PropertyInfo(Variant::BOOL, "debug")));
    ADD_SIGNAL(MethodInfo("goto_script_line", PropertyInfo(Variant::OBJECT, "script"), PropertyInfo(Variant::INT, "line")));
    ADD_SIGNAL(MethodInfo("breakpoints_cleared_in_tree"));
    ADD_SIGNAL(MethodInfo("breakpoint_set_in_tree", PropertyInfo(Variant::OBJECT, "script"), PropertyInfo(Variant::INT, "line"), PropertyInfo(Variant::BOOL, "enabled")));
}

OrchestratorEditorDebuggerPlugin::OrchestratorEditorDebuggerPlugin()
{
    _singleton = this;
}

OrchestratorEditorDebuggerPlugin::~OrchestratorEditorDebuggerPlugin()
{
    _singleton = nullptr;
}
#endif
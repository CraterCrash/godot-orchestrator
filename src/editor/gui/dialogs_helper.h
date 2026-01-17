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
#ifndef ORCHESTRATOR_EDITOR_DIALOGS_HELPER_H
#define ORCHESTRATOR_EDITOR_DIALOGS_HELPER_H

#include <godot_cpp/variant/variant.hpp>

using namespace godot;

/// Utility class for some common dialogs
class OrchestratorEditorDialogs {
public:
    static void accept(const String& p_message, const String& p_button = "OK");
    static void confirm(const String& p_message, const Callable& p_callback, const String& p_yes_label = "Yes", const String& p_no_label = "No");
    static void error(const String& p_message, const String& p_title = "Error", bool p_exclusive = true);
};

#define ORCHESTRATOR_ACCEPT(message) { OrchestratorEditorDialogs::accept(message); return; }
#define ORCHESTRATOR_ACCEPT_V(message, retval) { OrchestratorEditorDialogs::accept(message); return retval; }

#define ORCHESTRATOR_CONFIRM(message, callable) { OrchestratorEditorDialogs::confirm(message, callable); return; }
#define ORCHESTRATOR_CONFIRM_V(message, callable, retval) { OrchestratorEditorDialogs::confirm(message, callable); return retval; }

#define ORCHESTRATOR_ERROR(message) { OrchestratorEditorDialogs::error(message); return; }
#define ORCHESTRATOR_ERROR_TITLE(message, title) { OrchestratorEditorDialogs::error(message, title); return; }
#define ORCHESTRATOR_ERROR_V(message, retval) { OrchestratorEditorDialogs::error(message); return retval; }

#endif // ORCHESTRATOR_EDITOR_DIALOGS_HELPER_H
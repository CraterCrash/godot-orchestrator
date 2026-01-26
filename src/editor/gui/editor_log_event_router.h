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
#ifndef ORCHESTRATOR_EDITOR_LOG_EVENT_ROUTER_H
#define ORCHESTRATOR_EDITOR_LOG_EVENT_ROUTER_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>

using namespace godot;

class OrchestratorEditorLogEventRouter : public Node {
    GDCLASS(OrchestratorEditorLogEventRouter, Node);

    RichTextLabel* _locate_editor_output_log();

    void _meta_clicked(const String& p_meta);

protected:
    static void _bind_methods();

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

};

#endif // ORCHESTRATOR_EDITOR_LOG_EVENT_ROUTER_H
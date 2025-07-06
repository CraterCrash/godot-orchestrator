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
#ifndef ORCHESTRATOR_ACTIONS_HELP_H
#define ORCHESTRATOR_ACTIONS_HELP_H

#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

using namespace godot;

/// Forward declaration
class OrchestratorEditorActionDefinition;

/// Displays help information about a selected <code>OrchestratorEditorActionDefinition</code>
class OrchestratorEditorActionHelp : public VBoxContainer
{
    GDCLASS(OrchestratorEditorActionHelp, VBoxContainer);

    Size2 _content_size;
    RichTextLabel* _title = nullptr;
    RichTextLabel* _help = nullptr;
    String _text;

protected:
    static void _bind_methods() { }

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

    void _meta_clicked(const Variant& p_value);
    void _add_text(const String& p_text);

public:
    void set_disabled(bool p_disabled);
    void set_text(const String& p_text);

    void set_content_help_limits(float p_min, float p_max);
    void update_content_height();

    void parse_action(const Ref<OrchestratorEditorActionDefinition>& p_action);

    OrchestratorEditorActionHelp();
};

#endif // ORCHESTRATOR_ACTIONS_HELP_H
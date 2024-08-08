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
#ifndef ORCHESTRATOR_SCRIPT_MACROS_COMPONENT_PANEL_H
#define ORCHESTRATOR_SCRIPT_MACROS_COMPONENT_PANEL_H

#include "editor/component_panels/component_panel.h"

class OrchestratorScriptMacrosComponentPanel : public OrchestratorScriptComponentPanel
{
    GDCLASS(OrchestratorScriptMacrosComponentPanel, OrchestratorScriptComponentPanel);
    static void _bind_methods();

protected:
    //~ Begin OrchestratorScriptComponentPanel Interface
    String _get_unique_name_prefix() const override { return "NewMacro"; }
    String _get_tooltip_text() const override;
    String _get_item_name() const override { return "Macro"; }
    //~ End OrchestratorScriptComponentPanel Interface

    /// Default constructor
    OrchestratorScriptMacrosComponentPanel() = default;

public:
    //~ Begin OrchestratorScriptViewSection Interface
    void update() override;
    //~ End OrchestratorScriptViewSection Interface

    /// Godot's notification callback
    /// @param p_what the notification type
    void _notification(int p_what);

    /// Construct the function component panel
    /// @param p_orchestration the orchestration
    explicit OrchestratorScriptMacrosComponentPanel(Orchestration* p_orchestration);
};

#endif // ORCHESTRATOR_SCRIPT_MACROS_COMPONENT_PANEL_H
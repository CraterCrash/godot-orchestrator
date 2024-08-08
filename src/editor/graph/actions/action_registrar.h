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
#ifndef ORCHESTRATOR_GRAPH_ACTION_REGISTRAR_H
#define ORCHESTRATOR_GRAPH_ACTION_REGISTRAR_H

#include "action_menu_item.h"
#include "script/script.h"

/// Registrar context
struct OrchestratorGraphActionRegistrarContext
{
    OrchestratorGraphEdit* graph{ nullptr };
    List<Ref<OrchestratorGraphActionMenuItem>>* list{ nullptr };
    OrchestratorGraphActionFilter* filter{ nullptr };

    static OrchestratorGraphActionRegistrarContext from_filter(const OrchestratorGraphActionFilter& p_filter);
};

/// Contract that defines an OrchestratorGraphAction registrar.
///
/// This is a user extension hook that allows custom registrar classes to contribute custom
/// objects to the OrchestratorGraphEdit action window.
///
class OrchestratorGraphActionRegistrar : public RefCounted
{
    GDCLASS(OrchestratorGraphActionRegistrar, RefCounted)

protected:
    static void _bind_methods() {}

public:
    /// Register actions
    /// @param p_context the registrar context
    virtual void register_actions(const OrchestratorGraphActionRegistrarContext& p_context) { }
};

#endif // ORCHESTRATOR_EDITOR_GRAPH_ACTION_REGISTRAR_H
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
#ifndef ORCHESTRATOR_EDITOR_ACTIONS_REGISTRY_H
#define ORCHESTRATOR_EDITOR_ACTIONS_REGISTRY_H

#include "editor/actions/definition.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/classes/timer.hpp>

using namespace godot;

/// A singleton node that maintains a collection of all available actions
///
/// When Godot first starts, the registry will be populated with a set of actions that are specific to
/// all native classes in Godot. These are classes that will never change during the lifetime of the
/// Editor's execution.
///
/// In addition, the <code>FileSystemDock</code> will emit details about all the resources that have
/// been scanned, along with notifying when they're added or removed. These hooks are monitored by
/// this class and all resource-related object's actions are kept synchronized.
///
class OrchestratorEditorActionRegistry : public Node
{
    GDCLASS(OrchestratorEditorActionRegistry, Node);

    static OrchestratorEditorActionRegistry* _singleton;

    using Action = OrchestratorEditorActionDefinition;
    using ActionComparator = OrchestratorEditorActionDefinitionComparator;
    using ActionType = OrchestratorEditorActionDefinition::ActionType;
    using GraphType = OrchestratorEditorActionDefinition::GraphType;

    Vector<Ref<Action>> _actions;
    bool _building = false;
    Timer* _global_script_class_update_timer = nullptr;

    void _build_actions();
    void _global_script_classes_updated();
    void _resources_reloaded(const PackedStringArray& p_file_names);

protected:
    static void _bind_methods();

public:
    static OrchestratorEditorActionRegistry* get_singleton() { return _singleton; }

    Vector<Ref<Action>> get_actions();
    Vector<Ref<Action>> get_actions(const Ref<Script>& p_script, const Ref<Script>& p_other = Ref<Script>());
    Vector<Ref<Action>> get_actions(Object* p_target);
    Vector<Ref<Action>> get_actions(const StringName& p_class_name);

    OrchestratorEditorActionRegistry();
    ~OrchestratorEditorActionRegistry() override;
};

#endif // ORCHESTRATOR_EDITOR_ACTIONS_REGISTRY_H
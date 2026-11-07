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
#pragma once

#include "editor/actions/definition.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/script.hpp>

using namespace godot;

/// A singleton node that maintains a collection of all available actions
///
/// When Godot first starts, the registry is populated with the actions for every native class,
/// variant type, built-in function and script node. None of these introspect project resources,
/// and none of them change for the lifetime of the editor.
///
/// Actions derived from project scripts (global classes and autoloads) are built lazily on the
/// first request and rebuilt only after the <code>EditorFileSystem</code> or project settings
/// report a change. Introspecting a global class loads its script, and doing that while the
/// editor is still initializing other addons can load scripts ahead of the addon that owns them;
/// deferring the work until a menu needs it keeps this registry out of that ordering.
///
class OrchestratorEditorActionRegistry : public Node {
    GDCLASS(OrchestratorEditorActionRegistry, Node);

    static OrchestratorEditorActionRegistry* _singleton;

    using Action = OrchestratorEditorActionDefinition;
    using ActionComparator = OrchestratorEditorActionDefinitionComparator;
    using ActionType = OrchestratorEditorActionDefinition::ActionType;
    using GraphType = OrchestratorEditorActionDefinition::GraphType;

    OrchestratorEditorActionSet _base_actions;
    OrchestratorEditorActionSet _immutable_actions;
    OrchestratorEditorActionSet _global_class_actions;
    OrchestratorEditorActionSet _autoload_actions;

    bool _global_classes_dirty = true;
    bool _autoloads_dirty = true;

    void _build_immutable_actions();
    void _ensure_built();

    void _settings_changed();
    void _script_classes_updated();

protected:
    static void _bind_methods();

public:
    static OrchestratorEditorActionRegistry* get_singleton() { return _singleton; }

    /// Builds any stale project-derived actions now rather than on the next request, so the
    /// first menu opened after a change does not pay for the rebuild. Safe to call repeatedly;
    /// it is a no-op while nothing is stale.
    void prime();

    OrchestratorEditorActionSet get_actions();
    OrchestratorEditorActionSet get_actions(const Ref<Script>& p_script, const Ref<Script>& p_other = Ref<Script>());
    OrchestratorEditorActionSet get_actions(Object* p_target);
    OrchestratorEditorActionSet get_actions(const StringName& p_class_name);

    OrchestratorEditorActionRegistry();
    ~OrchestratorEditorActionRegistry() override;
};

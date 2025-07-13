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
#ifndef ORCHESTRATOR_EDITOR_ACTIONS_FILTER_PORT_RULE_H
#define ORCHESTRATOR_EDITOR_ACTIONS_FILTER_PORT_RULE_H

#include "editor/actions/rules/rule.h"

using namespace godot;

class OrchestratorGraphNodePin;

/// This rule is designed to match the details associated with the port that the dragged from. This
/// checks information such as port type, class, and object reference.
///
class OrchestratorEditorActionPortRule : public OrchestratorEditorActionFilterRule
{
    GDCLASS(OrchestratorEditorActionPortRule, OrchestratorEditorActionFilterRule);

    Variant::Type _type = Variant::NIL;
    PackedStringArray _target_classes;
    bool _output = false;
    bool _execution = false;

protected:
    static void _bind_methods() { }

public:
    //~ Begin ActionFilterRule Interface
    bool matches(const Ref<OrchestratorEditorActionDefinition>& p_action, const FilterContext& p_context) override;
    //~ End ActionFilterRule Interface

    // Configure the port based on the source drag pin
    void configure(const OrchestratorGraphNodePin* p_pin, const Object* p_target);
};

#endif // ORCHESTRATOR_EDITOR_ACTIONS_FILTER_PORT_RULE_H
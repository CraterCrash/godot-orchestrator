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
#ifndef ORCHESTRATOR_EDITOR_ACTIONS_FILTER_RULE_H
#define ORCHESTRATOR_EDITOR_ACTIONS_FILTER_RULE_H

#include "editor/actions/definition.h"

struct FilterContext;

/// This class provides the basis for all <code>OrchestratorEditorFilterEngine</code> rules.
///
class OrchestratorEditorActionFilterRule : public RefCounted {
    GDCLASS(OrchestratorEditorActionFilterRule, RefCounted);

protected:
    static void _bind_methods() { }

public:
    virtual bool is_context_sensitive() const { return false; }
    virtual bool matches(const Ref<OrchestratorEditorActionDefinition>& p_action, const FilterContext& p_context) { return true; }
    virtual float score(const Ref<OrchestratorEditorActionDefinition>& p_action, const FilterContext& p_context) { return 1.f; }

    _FORCE_INLINE_ bool operator==(const Ref<OrchestratorEditorActionFilterRule>& p_other) const {
        return p_other.is_null() ? false : get_class() == p_other->get_class();
    }

    ~OrchestratorEditorActionFilterRule() override = default;
};

#endif // ORCHESTRATOR_EDITOR_ACTIONS_FILTER_RULE_H
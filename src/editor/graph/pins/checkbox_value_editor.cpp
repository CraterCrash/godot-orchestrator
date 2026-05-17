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
#include "editor/graph/pins/checkbox_value_editor.h"

#include "common/callable_lambda.h"
#include "common/macros.h"
#include "core/godot/scene_string_names.h"

void OrchestratorEditorGraphPinValueEditorCheckbox::configure(const PropertyInfo& p_property) {
    if (_control) {
        return;
    }

    _control = memnew(CheckBox);
    _control->set_focus_mode(FOCUS_NONE);
    _control->set_h_size_flags(SIZE_EXPAND_FILL);
    _control->connect(SceneStringName(toggled), callable_mp_lambda(this, [this](const bool&) {
        _emit_value_changed(_control->is_pressed());
    }));
    add_child(_control);
}

void OrchestratorEditorGraphPinValueEditorCheckbox::set_value(const Variant& p_value) {
    GUARD_NULL(_control);

    _control->set_block_signals(true);
    _control->set_pressed(p_value.operator bool());
    _control->set_block_signals(false);
}

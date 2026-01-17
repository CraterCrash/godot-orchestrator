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
#include "editor/graph/pins/checkbox_pin.h"

#include "common/callable_lambda.h"
#include "core/godot/scene_string_names.h"

void OrchestratorEditorGraphPinCheckbox::_update_control_value(const Variant& p_value) {
    _control->set_pressed(p_value);
}

Variant OrchestratorEditorGraphPinCheckbox::_read_control_value() {
    return _control->is_pressed();
}

Control* OrchestratorEditorGraphPinCheckbox::_create_default_value_widget() {
    _control = memnew(CheckBox);
    _control->set_focus_mode(FOCUS_NONE);
    _control->set_h_size_flags(SIZE_EXPAND_FILL);
    _control->connect(SceneStringName(toggled), callable_mp_lambda(this, [&] (const bool&) { _default_value_changed(); }));

    return _control;
}

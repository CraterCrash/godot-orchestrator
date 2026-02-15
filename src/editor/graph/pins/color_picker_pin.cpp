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
#include "editor/graph/pins/color_picker_pin.h"

#include "common/callable_lambda.h"
#include "common/macros.h"

#include <godot_cpp/classes/editor_interface.hpp>

void OrchestratorEditorGraphPinColorPicker::_update_control_value(const Variant& p_value) {
    _control->set_pick_color(p_value);
}

Variant OrchestratorEditorGraphPinColorPicker::_read_control_value() {
    return _control->get_pick_color();
}

Control* OrchestratorEditorGraphPinColorPicker::_create_default_value_widget() {
    _control = memnew(ColorPickerButton);
    _control->set_focus_mode(FOCUS_NONE);
    _control->set_h_size_flags(SIZE_SHRINK_BEGIN);
    _control->set_v_size_flags(SIZE_SHRINK_CENTER);
    _control->set_custom_minimum_size(Vector2(24, 24) * EDSCALE);
    _control->connect("color_changed", callable_mp_lambda(this, [&] (const Color&) { _default_value_changed(); }));

    return _control;
}
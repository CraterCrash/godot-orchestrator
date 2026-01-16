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
#include "editor/graph/pins/text_edit_pin.h"

#include "common/callable_lambda.h"

void OrchestratorEditorGraphPinTextEdit::_update_control_value(const Variant& p_value) {
    _control->set_text(p_value);
}

Variant OrchestratorEditorGraphPinTextEdit::_read_control_value() {
    return _control->get_text();
}

Control* OrchestratorEditorGraphPinTextEdit::_create_default_value_widget() {
    _control = memnew(TextEdit);
    _control->set_placeholder("No value...");
    _control->set_h_size_flags(SIZE_EXPAND);
    _control->set_v_size_flags(SIZE_EXPAND);
    _control->set_h_grow_direction(GROW_DIRECTION_END);
    _control->set_custom_minimum_size(Vector2(350, 0));
    _control->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    _control->set_line_wrapping_mode(TextEdit::LINE_WRAPPING_BOUNDARY);
    _control->set_fit_content_height_enabled(true);
    _control->connect("text_changed", callable_mp_lambda(this, [&] { _default_value_changed(); }));

    return _control;
}
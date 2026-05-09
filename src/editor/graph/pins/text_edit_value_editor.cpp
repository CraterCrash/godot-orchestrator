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
#include "editor/graph/pins/text_edit_value_editor.h"

#include "common/callable_lambda.h"
#include "common/macros.h"
#include "core/godot/scene_string_names.h"

void OrchestratorEditorGraphPinValueEditorTextEdit::configure(const PropertyInfo& p_property) {
    if (_control) {
        return;
    }

    _control = memnew(TextEdit);
    _control->set_placeholder("No value...");
    _control->set_h_size_flags(SIZE_EXPAND);
    _control->set_v_size_flags(SIZE_EXPAND);
    _control->set_h_grow_direction(GROW_DIRECTION_END);
    _control->set_custom_minimum_size(Vector2(350, 0));
    _control->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    _control->set_line_wrapping_mode(TextEdit::LINE_WRAPPING_BOUNDARY);
    _control->set_fit_content_height_enabled(true);
    add_child(_control);

    _control->connect(SceneStringName(text_changed), callable_mp_lambda(this, [this] {
        _emit_value_changed(_control->get_text());
    }));
}

void OrchestratorEditorGraphPinValueEditorTextEdit::set_value(const Variant& p_value) {
    GUARD_NULL(_control);

    const String text = p_value;
    if (_control->get_text() != text) {
        _control->set_block_signals(true);
        _control->set_text(text);
        _control->set_block_signals(false);
    }
}

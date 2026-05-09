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
#include "editor/graph/pins/number_value_editor.h"

#include "common/callable_lambda.h"
#include "common/macros.h"
#include "core/godot/scene_string_names.h"

void OrchestratorEditorGraphPinValueEditorNumber::_commit() {
    const String text = _control->get_text();
    Variant value;

    switch (_type) {
        case Variant::FLOAT: {
            if (text.is_valid_float()) {
                value = text.to_float();
                _rollback_value = value;
                callable_mp_this(set_value).bind(value).call_deferred();
                _emit_value_changed(value);
                return;
            }
            break;
        }
        case Variant::INT: {
            if (text.is_valid_int()) {
                value = text.to_int();
                _rollback_value = value;
                callable_mp_this(set_value).bind(value).call_deferred();
                _emit_value_changed(value);
                return;
            }
            break;
        }
        default:
            break;
    }

    // Invalid input: restore
    _control->set_text(_rollback_value);
    _control->call_deferred("grab_focus");
    _control->call_deferred("select_all");
}

void OrchestratorEditorGraphPinValueEditorNumber::configure(const PropertyInfo& p_property) {
    if (_control) {
        return;
    }

    _type = p_property.type;

    _control = memnew(LineEdit);
    _control->set_expand_to_text_length_enabled(true);
    _control->set_h_size_flags(SIZE_EXPAND);
    _control->add_theme_constant_override("minimum_character_width", 0);
    _control->set_select_all_on_focus(true);
    _control->connect(SceneStringName(text_submitted), callable_mp_lambda(this, [this](const String&) { _control->release_focus(); }));
    _control->connect(SceneStringName(focus_exited), callable_mp_lambda(this, [this] { _commit(); }));
    add_child(_control);
}

void OrchestratorEditorGraphPinValueEditorNumber::set_value(const Variant& p_value) {
    GUARD_NULL(_control);

    _rollback_value = p_value;

    _control->set_block_signals(true);
    _control->set_text(p_value);
    _control->set_block_signals(false);
}

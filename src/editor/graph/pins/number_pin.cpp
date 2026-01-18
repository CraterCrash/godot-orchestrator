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
#include "editor/graph/pins/number_pin.h"

#include "common/callable_lambda.h"
#include "common/macros.h"
#include "core/godot/scene_string_names.h"

void OrchestratorEditorGraphPinNumber::_update_control_value(const Variant& p_value) {
    // Used in case the value entered is invalid
    _rollback_value = p_value;

    _control->set_text(p_value);
}

Variant OrchestratorEditorGraphPinNumber::_read_control_value() {
    const String text_value = _control->get_text();
    switch (get_property_info().type) {
        case Variant::FLOAT: {
            if (text_value.is_valid_float()) {
                const double value = text_value.to_float();
                callable_mp_this(_update_control_value).bind(value).call_deferred();
                return value;
            }
            break;
        }
        case Variant::INT: {
            if (text_value.is_valid_int()) {
                const int value = text_value.to_int();
                callable_mp_this(_update_control_value).bind(value).call_deferred();
                return value;
            }
            break;
        }
        default: {
            break;
        }
    }

    _control->set_text(_rollback_value);
    _control->call_deferred("grab_focus");
    _control->call_deferred("select_all");

    return _rollback_value;
}

Control* OrchestratorEditorGraphPinNumber::_create_default_value_widget() {
    _control = memnew(LineEdit);
    _control->set_expand_to_text_length_enabled(true);
    _control->set_h_size_flags(SIZE_EXPAND);
    _control->add_theme_constant_override("minimum_character_width", 0);
    _control->set_select_all_on_focus(true);
    _control->connect(SceneStringName(text_submitted), callable_mp_lambda(this, [&] (const String&) { _control->release_focus(); }));
    _control->connect(SceneStringName(focus_exited), callable_mp_lambda(this, [&] { _default_value_changed(); }));

    return _control;
}

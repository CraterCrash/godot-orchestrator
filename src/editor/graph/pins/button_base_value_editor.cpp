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
#include "editor/graph/pins/button_base_value_editor.h"

#include "common/macros.h"
#include "common/scene_utils.h"
#include "common/string_utils.h"
#include "core/godot/scene_string_names.h"

void OrchestratorEditorGraphPinValueEditorButtonBase::_clear_button_pressed() {
    _emit_value_changed(_clear_default);
    set_value(_clear_default);
}

void OrchestratorEditorGraphPinValueEditorButtonBase::_selector_button_pressed() {
    _handle_selector_button_pressed();
}

void OrchestratorEditorGraphPinValueEditorButtonBase::_set_button_visible(bool p_visible) {
    GUARD_NULL(_selector_button);
    _selector_button->set_visible(p_visible);
}

void OrchestratorEditorGraphPinValueEditorButtonBase::_set_button_disabled(bool p_disabled) {
    GUARD_NULL(_selector_button);
    _selector_button->set_disabled(p_disabled);
}

void OrchestratorEditorGraphPinValueEditorButtonBase::_set_button_tooltip(const String& p_tooltip) {
    GUARD_NULL(_selector_button);
    _selector_button->set_tooltip_text(p_tooltip);
}

void OrchestratorEditorGraphPinValueEditorButtonBase::_handle_selector_button_response(const Variant& p_value) {
    _button_value = p_value;
    _emit_value_changed(p_value);
    set_value(p_value);
}

void OrchestratorEditorGraphPinValueEditorButtonBase::configure(const PropertyInfo& p_property) {
    if (_selector_button) {
        return;
    }

    add_theme_constant_override("separation", 1);

    _selector_button = memnew(Button);
    _selector_button->set_focus_mode(FOCUS_NONE);
    _selector_button->set_custom_minimum_size(Vector2(28, 0));
    _selector_button->connect(SceneStringName(pressed), callable_mp_this(_selector_button_pressed));
    add_child(_selector_button);

    _clear_button = memnew(Button);
    _clear_button->set_focus_mode(FOCUS_NONE);
    _clear_button->set_button_icon(SceneUtils::get_editor_icon("Reload"));
    _clear_button->connect(SceneStringName(pressed), callable_mp_this(_clear_button_pressed));
    add_child(_clear_button);
}

void OrchestratorEditorGraphPinValueEditorButtonBase::set_value(const Variant& p_value) {
    GUARD_NULL(_selector_button);

    _button_value = p_value;

    const String str_value = _button_value.get_type() != Variant::NIL ? String(_button_value) : String();
    const String button_text = StringUtils::default_if_empty(str_value, _default_text);
    const bool is_default = button_text == _default_text;
    _selector_button->set_text(button_text);
    _clear_button->set_visible(!is_default);
}

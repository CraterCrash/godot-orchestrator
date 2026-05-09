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
#include "editor/graph/pins/line_edit_value_editor.h"

#include "common/callable_lambda.h"
#include "common/macros.h"
#include "core/godot/scene_string_names.h"

#include <godot_cpp/classes/input_event_key.hpp>

void OrchestratorEditorGraphPinValueEditorLineEdit::_focus_entered() {
    PackedStringArray suggestions;
    if (_pin.is_valid()) {
        suggestions = _pin->get_suggestions();
    }

    if (!suggestions.is_empty()) {
        _popup->clear();
        for (const String& item : suggestions) {
            _popup->add_item(item);
        }
        _popup->set_position(_control->get_screen_position() + Vector2(0, _control->get_size().height));
        _popup->popup();
    }

    _control->grab_focus();
    _control->select_all();
}

void OrchestratorEditorGraphPinValueEditorLineEdit::_popup_window_input(const Ref<InputEvent>& p_event) {
    const Ref<InputEventKey> key = p_event;
    if (key.is_valid() && key->is_pressed() && key->get_keycode() != KEY_ENTER) {
        _control->get_viewport()->push_input(p_event, false);
    }
}

void OrchestratorEditorGraphPinValueEditorLineEdit::_popup_index_pressed(int p_index) {
    const String popup_suggestion = _popup->get_item_text(p_index);
    _control->set_text(popup_suggestion);
    _emit_value_changed(popup_suggestion);

    _control->release_focus();
}

void OrchestratorEditorGraphPinValueEditorLineEdit::configure(const PropertyInfo& p_property) {
    if (_control) {
        return;
    }

    _control = memnew(LineEdit);
    _control->set_custom_minimum_size(Vector2(30, 0));
    _control->set_expand_to_text_length_enabled(true);
    _control->set_h_size_flags(SIZE_EXPAND);
    _control->set_select_all_on_focus(true);
    _control->set_deselect_on_focus_loss_enabled(true);
    _control->connect(SceneStringName(text_submitted), callable_mp_lambda(this, [this](const String&) { _control->release_focus(); }));
    _control->connect(SceneStringName(focus_entered), callable_mp_this(_focus_entered));
    _control->connect(SceneStringName(focus_exited), callable_mp_lambda(this, [this] { _emit_value_changed(_control->get_text()); }));

    _popup = memnew(PopupMenu);
    _popup->set_flag(PopupMenu::FLAG_NO_FOCUS, true);
    _popup->set_allow_search(true);
    _popup->connect(SceneStringName(window_input), callable_mp_this(_popup_window_input));
    _popup->connect("index_pressed", callable_mp_this(_popup_index_pressed));
    _popup->connect("popup_hide", callable_mp_lambda(this, [this] { _control->release_focus(); }));
    _control->add_child(_popup);

    add_child(_control);
}

void OrchestratorEditorGraphPinValueEditorLineEdit::set_value(const Variant& p_value) {
    GUARD_NULL(_control);

    _control->set_block_signals(true);
    _control->set_text(p_value);
    _control->set_block_signals(false);
}

void OrchestratorEditorGraphPinValueEditorLineEdit::set_pin_ref(const Ref<OrchestrationGraphPin>& p_pin) {
    _pin = p_pin;
}

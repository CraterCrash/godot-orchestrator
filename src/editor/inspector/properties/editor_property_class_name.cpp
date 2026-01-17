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
#include "editor/inspector/properties/editor_property_class_name.h"

#include "common/macros.h"
#include "editor/gui/select_class_dialog.h"

Variant OrchestratorEditorPropertyClassName::_get_edited_property_value() {
    ERR_FAIL_NULL_V(get_edited_object(), Variant());
    return get_edited_object()->get(get_edited_property());
}

void OrchestratorEditorPropertyClassName::_property_selected() {
    _dialog->popup_create(true, true, _get_edited_property_value(), get_edited_property());
}

void OrchestratorEditorPropertyClassName::_dialog_selected() {
    _selected_type = _dialog->get_selected();

    emit_changed(get_edited_property(), _selected_type);

    _property->set_text(_selected_type);
}

void OrchestratorEditorPropertyClassName::_update_property() {
    const String value = _get_edited_property_value();

    _property->set_text(value);
    _selected_type = value;
}

void OrchestratorEditorPropertyClassName::_set_read_only(bool p_read_only) {
    _property->set_disabled(p_read_only);
}

void OrchestratorEditorPropertyClassName::setup(const String& p_base_type, const String& p_selected_type, bool p_allow_abstract) {
    _base_type = p_base_type;
    _selected_type = p_selected_type;

    _dialog->set_base_type(_base_type);
    _dialog->set_allow_abstract_types(p_allow_abstract);

    _property->set_text(_selected_type);
}

void OrchestratorEditorPropertyClassName::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_READY: {
            _property->connect("pressed", callable_mp_this(_property_selected));
            _dialog->connect("selected", callable_mp_this(_dialog_selected));
            break;
        }
    }
}

void OrchestratorEditorPropertyClassName::_bind_methods() {
}

OrchestratorEditorPropertyClassName::OrchestratorEditorPropertyClassName() {
    _property = memnew(Button);
    _property->set_clip_text(true);
    _property->set_theme_type_variation("EditorInspectorButton");

    add_child(_property);
    add_focusable(_property);

    _property->set_text(_selected_type);

    _dialog = memnew(OrchestratorSelectClassSearchDialog);
    _dialog->set_base_type(_base_type);
    _dialog->set_data_suffix("class");
    _dialog->set_popup_title("Select Class");

    add_child(_dialog);
}
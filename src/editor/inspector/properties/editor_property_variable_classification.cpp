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
#include "editor/inspector/properties/editor_property_variable_classification.h"

#include "../../gui/select_type_dialog.h"
#include "common/callable_lambda.h"
#include "common/macros.h"
#include "common/scene_utils.h"
#include "core/godot/scene_string_names.h"

void OrchestratorEditorPropertyVariableClassification::_property_selected() {
    _dialog->popup_create(true, false, get_edited_object()->get(get_edited_property()), get_edited_property());
}

void OrchestratorEditorPropertyVariableClassification::_search_selected() {
    _selected_name = _dialog->get_selected_type();

    if (get_edited_object()->get(get_edited_property()) != _selected_name) {
        ConfirmationDialog* confirm = memnew(ConfirmationDialog);
        confirm->set_text("This could break connections and reset default values on variable set nodes.\nDo you want to change the variable type?");
        confirm->set_title("Change Variable Type");
        confirm->set_ok_button_text("Change Variable Type");
        add_child(confirm);

        confirm->connect(SceneStringName(confirmed), callable_mp_lambda(this, [confirm, this] {
            emit_changed(get_edited_property(), _selected_name);
            update_property();
            if (confirm)
                confirm->queue_free();
        }));

        confirm->connect(SceneStringName(canceled), callable_mp_lambda(this, [confirm] {
            if (confirm)
                confirm->queue_free();
        }));

        confirm->popup_centered();
    }
}

void OrchestratorEditorPropertyVariableClassification::_update_property() {
    String value = get_edited_object()->get(get_edited_property());
    _selected_name = value;

    if (value.contains(":")) {
        const PackedStringArray parts = value.split(":");
        if (parts[0].match("type")) {
            _property->set_text(parts[1] == "Nil" ? "Any" : parts[1]);
            _property->set_button_icon(SceneUtils::get_editor_icon(parts[1] == "Nil" ? "Variant" : _property->get_text()));
        } else if (parts[0].match("class")) {
            _property->set_text(parts[1]);
            _property->set_button_icon(SceneUtils::get_class_icon(parts[1]));
        } else {
            _property->set_text(parts[1]);
            _property->set_button_icon(SceneUtils::get_editor_icon("Enum"));
        }
    } else {
        _property->set_text(value);
    }
}

void OrchestratorEditorPropertyVariableClassification::edit() {
    _dialog->popup_create(true, false, get_edited_object()->get(get_edited_property()), get_edited_property());
}

void OrchestratorEditorPropertyVariableClassification::setup(const String& p_base_type, const String& p_selected_type) {
    _base_type = p_base_type;
    _dialog->set_base_type(p_base_type);

    _selected_name = p_selected_type;

    if (p_selected_type.contains(":") && p_selected_type.match("type:Nil")) {
        _property->set_text("Any");
    } else {
        _property->set_text(p_selected_type);
    }
}

void OrchestratorEditorPropertyVariableClassification::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_READY: {
            // Register button that triggers opening the search dialog
            _property = memnew(Button);
            _property->set_clip_text(true);
            _property->set_text(_selected_name);
            _property->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
            _property->add_theme_constant_override("icon_max_width", SceneUtils::get_editor_class_icon_size());
            add_child(_property);
            add_focusable(_property);

            _dialog = memnew(OrchestratorSelectTypeSearchDialog);
            _dialog->set_popup_title("Select variable type");
            _dialog->set_data_suffix("variable_type");
            _dialog->set_base_type(_base_type);
            _dialog->set_allow_abstract_types(true);
            add_child(_dialog);

            _dialog->connect("selected", callable_mp_this(_search_selected));
            _property->connect(SceneStringName(pressed), callable_mp_this(_property_selected));

            break;
        }
    }
}
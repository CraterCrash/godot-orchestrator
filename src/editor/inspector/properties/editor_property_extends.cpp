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
#include "editor/inspector/properties/editor_property_extends.h"

#include "common/callable_lambda.h"
#include "common/macros.h"
#include "common/scene_utils.h"
#include "common/version.h"
#include "core/godot/core_string_names.h"
#include "core/godot/scene_string_names.h"
#include "editor/gui/dialogs_helper.h"
#include "editor/gui/file_dialog.h"

#include <godot_cpp/classes/editor_file_dialog.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/h_box_container.hpp>

void OrchestratorEditorPropertyExtends::_select_extends_class() {
    if (_editor_property_class) {
        Control* button = cast_to<Button>(_editor_property_class->find_child("*Button*", true, false));
        if (button) {
            button->emit_signal("pressed");
        }
    }
}

void OrchestratorEditorPropertyExtends::_select_extends_path() {
    OrchestratorFileDialog* dialog = memnew(OrchestratorFileDialog);
    dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
    dialog->set_access(FileDialog::ACCESS_RESOURCES);
    dialog->set_hide_on_ok(true);
    dialog->clear_filters();
    dialog->add_filter("*.os,*.torch", "Orchestrations");
    #if GODOT_VERSION >= 0x040500
    dialog->set_customization_flag_enabled(FileDialog::CUSTOMIZATION_FILE_FILTER, false);
    #endif
    dialog->connect("canceled", callable_mp_lambda(this, [dialog] { dialog->queue_free(); }));
    dialog->connect("file_selected", callable_mp_this(_extends_path_selected));
    add_child(dialog);

    dialog->popup_file_dialog();
    dialog->set_title("Select Orchestration To Extend");
}

void OrchestratorEditorPropertyExtends::_extends_path_selected(const String& p_path) {
    const String extension = p_path.get_extension();
    if (!Array::make("os", "torch").has(extension)) {
        ORCHESTRATOR_ERROR("The selected file is not an orchestration.");
    }

    emit_changed("base_type", p_path);
}

void OrchestratorEditorPropertyExtends::_update_property() {
    if (get_edited_object()) {
        _selected_value = get_edited_object()->get(get_edited_property());
        _extends->set_text(_selected_value);
    }
}

void OrchestratorEditorPropertyExtends::setup(const String& p_base_type, bool p_allow_path) {
    _base_type = p_base_type;
    _allow_path = p_allow_path;
}

void OrchestratorEditorPropertyExtends::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_READY: {
            HBoxContainer* container = memnew(HBoxContainer);
            container->set_h_size_flags(SIZE_EXPAND_FILL);

            _extends = memnew(LineEdit);
            _extends->set_h_size_flags(SIZE_EXPAND_FILL);
            _extends->set_text(_base_type);
            _extends->connect(SceneStringName(focus_exited), callable_mp_lambda(this, [this] {
                emit_changed(get_edited_property(), _extends->get_text());
            }));
            _extends->connect(SceneStringName(text_submitted), callable_mp_lambda(this, [this] (const String& p_value) {
                emit_changed(get_edited_property(), p_value);
            }));
            container->add_child(_extends);

            _select_class_button = memnew(Button);
            _select_class_button->set_button_icon(SceneUtils::get_editor_icon("ClassList"));
            _select_class_button->set_tooltip_text("Extend from a native or Orchestration-defined class");
            _select_class_button->connect(SceneStringName(pressed), callable_mp_this(_select_extends_class));
            container->add_child(_select_class_button);

            if (_allow_path) {
                _select_path_button = memnew(Button);
                _select_path_button->set_button_icon(SceneUtils::get_editor_icon("Folder"));
                _select_path_button->set_tooltip_text("Extend from another Orchestration that is not a class");
                _select_path_button->connect(SceneStringName(pressed), callable_mp_this(_select_extends_path));
                container->add_child(_select_path_button);
            }

            add_child(container);
            add_focusable(_extends);

            int node_index = get_index();
            if (node_index >= 1) {
                // Allows us to reuse the same class selection behavior
                _editor_property_class = cast_to<Control>(get_parent()->get_child(node_index - 1));
                _editor_property_class->set_visible(false);
            }

            break;
        }
    }
}

void OrchestratorEditorPropertyExtends::_bind_methods() {

}
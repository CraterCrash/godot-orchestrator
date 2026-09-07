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
#include "editor/inspector/properties/editor_property_annotations.h"

#include "common/macros.h"
#include "common/scene_utils.h"
#include "core/godot/scene_string_names.h"
#include "editor/inspector/annotation_presentation.h"
#include "orchestration/annotation_registry.h"
#include "orchestration/function.h"
#include "orchestration/variable.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/editor_spin_slider.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

void OrchestratorEditorPropertyAnnotations::_get_owner(PropertyInfo& r_owner, uint32_t& r_target, bool& r_constant) {
    r_owner = PropertyInfo();
    r_target = OScriptAnnotationRegistry::TARGET_VARIABLE;
    r_constant = false;

    if (const OScriptVariable* variable = cast_to<OScriptVariable>(get_edited_object())) {
        r_owner = variable->get_info();
        r_constant = variable->is_constant();
        return;
    }

    if (const OScriptFunction* function = cast_to<OScriptFunction>(get_edited_object())) {
        r_owner = function->get_method_info().return_val;
        r_target = function->is_user_defined()
            ? OScriptAnnotationRegistry::TARGET_FUNCTION
            : OScriptAnnotationRegistry::TARGET_EVENT;
    }
}

String OrchestratorEditorPropertyAnnotations::_get_summary(const OScriptAnnotation& p_annotation) const {
    if (p_annotation.arguments.is_empty()) {
        return p_annotation.name;
    }

    PackedStringArray parts;
    for (int i = 0; i < p_annotation.arguments.size(); i++) {
        parts.push_back(UtilityFunctions::var_to_str(p_annotation.arguments[i]));
    }
    return vformat("%s(%s)", p_annotation.name, String(", ").join(parts));
}

Variant OrchestratorEditorPropertyAnnotations::_get_argument_default(const OScriptAnnotationDescriptor& p_descriptor, int p_index) const {
    const MethodInfo& info = p_descriptor.info;
    const int signature_index = _get_signature_index(p_descriptor, p_index);
    const int required = info.arguments.size() - info.default_arguments.size();
    if (signature_index >= required && signature_index - required < info.default_arguments.size()) {
        return info.default_arguments[signature_index - required];
    }

    switch (info.arguments[signature_index].type) {
        case Variant::INT:
            return 0;
        case Variant::FLOAT:
            return 0.0;
        case Variant::BOOL:
            return false;
        case Variant::STRING:
        default:
            return String();
    }
}

int OrchestratorEditorPropertyAnnotations::_get_signature_index(const OScriptAnnotationDescriptor& p_descriptor, int p_argument) const {
    const int last = p_descriptor.info.arguments.size() - 1;
    return p_argument > last ? last : p_argument;
}

void OrchestratorEditorPropertyAnnotations::_emit() {
    Array array;
    for (const OScriptAnnotation& annotation : _annotations) {
        array.push_back(annotation.to_dict());
    }

    _self_change = true;
    emit_changed(get_edited_property(), array);
}

void OrchestratorEditorPropertyAnnotations::_rebuild_chips() {
    if (!_chips) {
        return;
    }

    while (_chips->get_child_count() > 0) {
        Node* child = _chips->get_child(_chips->get_child_count() - 1);
        _chips->remove_child(child);
        child->queue_free();
    }

    for (int i = 0; i < _annotations.size(); i++) {
        const OScriptAnnotation& annotation = _annotations[i];
        const OrchestratorEditorAnnotationPresentation presentation = OrchestratorEditorAnnotationPresentation::get(annotation.name);

        HBoxContainer* chip = memnew(HBoxContainer);
        chip->add_theme_constant_override("separation", 0);

        Button* toggle = memnew(Button);
        toggle->set_text(_get_summary(annotation));
        toggle->set_tooltip_text(presentation.description.is_empty() ? presentation.label : presentation.description);
        toggle->set_toggle_mode(true);
        toggle->set_pressed_no_signal(i == _expanded);
        toggle->set_focus_mode(FOCUS_NONE);
        toggle->connect(SceneStringName(toggled), callable_mp_this(_chip_toggled).bind(i));
        chip->add_child(toggle);

        Button* remove = memnew(Button);
        remove->set_button_icon(SceneUtils::get_editor_icon("Remove"));
        remove->set_tooltip_text(vformat("Remove %s", annotation.name));
        remove->set_flat(true);
        remove->set_focus_mode(FOCUS_NONE);
        remove->set_disabled(_read_only);
        remove->connect(SceneStringName(pressed), callable_mp_this(_chip_remove_pressed).bind(i));
        chip->add_child(remove);

        _chips->add_child(chip);
    }
}

void OrchestratorEditorPropertyAnnotations::_rebuild_form() {
    if (!_form_box) {
        return;
    }

    while (_form_box->get_child_count() > 0) {
        Node* child = _form_box->get_child(_form_box->get_child_count() - 1);
        _form_box->remove_child(child);
        child->queue_free();
    }
    _form = nullptr;

    _form_stale = false;

    if (_expanded < 0 || _expanded >= _annotations.size()) {
        _form_box->hide();
        return;
    }

    const OScriptAnnotation& annotation = _annotations[_expanded];
    const OScriptAnnotationDescriptor* descriptor = OScriptAnnotationRegistry::find(annotation.name);
    if (descriptor == nullptr) {
        Label* unknown = memnew(Label);
        unknown->set_text(vformat("%s is not a known annotation and will fail to compile.", annotation.name));
        _form_box->add_child(unknown);
        _form_box->show();
        return;
    }

    const MethodInfo& info = descriptor->info;
    if (info.arguments.is_empty()) {
        _form_box->hide();
        return;
    }

    _form = memnew(GridContainer);
    _form->set_columns(3);
    _form->add_theme_constant_override("h_separation", 8);
    _form->add_theme_constant_override("v_separation", 4);
    _form_box->add_child(_form);

    // Fixed arguments come first; a vararg signature repeats its last argument for every extra value.
    const int fixed = descriptor->is_vararg() ? info.arguments.size() - 1 : info.arguments.size();
    const int provided = annotation.arguments.size();
    const int rows = descriptor->is_vararg() ? (provided > fixed ? provided : fixed) : fixed;

    for (int argument = 0; argument < rows; argument++) {
        const int signature_index = _get_signature_index(*descriptor, argument);
        const PropertyInfo& parameter = info.arguments[signature_index];

        Label* label = memnew(Label);
        label->set_text(String(parameter.name).capitalize());
        _form->add_child(label);

        const Variant value = argument < provided ? annotation.arguments[argument] : _get_argument_default(*descriptor, argument);
        Control* editor = _make_argument_editor(*descriptor, signature_index, value, argument);
        editor->set_h_size_flags(SIZE_EXPAND_FILL);
        _form->add_child(editor);

        if (descriptor->is_vararg() && argument >= fixed) {
            Button* remove = memnew(Button);
            remove->set_button_icon(SceneUtils::get_editor_icon("Remove"));
            remove->set_tooltip_text("Remove this value");
            remove->set_flat(true);
            remove->set_focus_mode(FOCUS_NONE);
            remove->set_disabled(_read_only);
            remove->connect(SceneStringName(pressed), callable_mp_this(_argument_remove_pressed).bind(argument));
            _form->add_child(remove);
        } else {
            _form->add_child(memnew(Control));
        }
    }

    if (descriptor->is_vararg()) {
        const PropertyInfo& parameter = info.arguments[info.arguments.size() - 1];

        Button* add = memnew(Button);
        add->set_button_icon(SceneUtils::get_editor_icon("Add"));
        add->set_text(vformat("Add %s", String(parameter.name).capitalize()));
        add->set_theme_type_variation("InspectorActionButton");
        add->set_h_size_flags(SIZE_SHRINK_CENTER);
        add->set_focus_mode(FOCUS_NONE);
        add->set_disabled(_read_only);
        add->connect(SceneStringName(pressed), callable_mp_this(_argument_add_pressed));
        _form_box->add_child(add);
    }

    _form_box->show();
}

void OrchestratorEditorPropertyAnnotations::_refresh_chip_text() {
    if (!_chips) {
        return;
    }

    for (int i = 0; i < _annotations.size() && i < _chips->get_child_count(); i++) {
        const HBoxContainer* chip = cast_to<HBoxContainer>(_chips->get_child(i));
        if (chip && chip->get_child_count() > 0) {
            Button* toggle = cast_to<Button>(chip->get_child(0));
            if (toggle) {
                toggle->set_text(_get_summary(_annotations[i]));
            }
        }
    }
}

Control* OrchestratorEditorPropertyAnnotations::_make_argument_editor(const OScriptAnnotationDescriptor& p_descriptor, int p_signature_index, const Variant& p_value, int p_argument) {
    const PropertyInfo& parameter = p_descriptor.info.arguments[p_signature_index];

    PackedStringArray labels;
    Array values;
    if (OrchestratorEditorAnnotationPresentation::get_argument_choices(p_descriptor.info.name, p_signature_index, labels, values)) {
        OptionButton* option = memnew(OptionButton);
        for (int i = 0; i < labels.size(); i++) {
            option->add_item(labels[i]);
            option->set_item_metadata(i, values[i]);
            if (values[i] == p_value) {
                option->select(i);
            }
        }
        option->set_disabled(_read_only);
        option->connect(SceneStringName(item_selected), callable_mp_this(_argument_item_selected).bind(p_argument));
        add_focusable(option);
        return option;
    }

    switch (parameter.type) {
        case Variant::INT:
        case Variant::FLOAT: {
            const bool integer = parameter.type == Variant::INT;

            EditorSpinSlider* spin = memnew(EditorSpinSlider);
            spin->set_min(-1000000);
            spin->set_max(1000000);
            spin->set_allow_greater(true);
            spin->set_allow_lesser(true);
            spin->set_step(integer ? 1.0 : 0.001);
            spin->set_hide_slider(true);
            spin->set_value(p_value);
            spin->set_read_only(_read_only);
            spin->connect(SceneStringName(value_changed), callable_mp_this(_argument_value_changed).bind(p_argument, integer));
            add_focusable(spin);
            return spin;
        }
        case Variant::BOOL: {
            CheckBox* check = memnew(CheckBox);
            check->set_pressed_no_signal(p_value);
            check->set_disabled(_read_only);
            check->connect(SceneStringName(toggled), callable_mp_this(_argument_toggled).bind(p_argument));
            add_focusable(check);
            return check;
        }
        default: {
            LineEdit* edit = memnew(LineEdit);
            edit->set_text(p_value);
            edit->set_editable(!_read_only);
            edit->connect(SceneStringName(text_submitted), callable_mp_this(_argument_text_submitted).bind(p_argument));
            edit->connect(SceneStringName(focus_exited), callable_mp_this(_argument_focus_exited).bind(p_argument));
            add_focusable(edit);
            return edit;
        }
    }
}

void OrchestratorEditorPropertyAnnotations::_set_argument(int p_argument, const Variant& p_value) {
    if (_expanded < 0 || _expanded >= _annotations.size()) {
        return;
    }

    const OScriptAnnotationDescriptor* descriptor = OScriptAnnotationRegistry::find(_annotations[_expanded].name);
    if (descriptor == nullptr) {
        return;
    }

    Array arguments = _annotations[_expanded].arguments.duplicate();
    while (arguments.size() <= p_argument) {
        arguments.push_back(_get_argument_default(*descriptor, arguments.size()));
    }

    if (arguments[p_argument] == p_value) {
        return;
    }

    arguments[p_argument] = p_value;
    _annotations.write[_expanded].arguments = arguments;
    _emit();
}

void OrchestratorEditorPropertyAnnotations::_add_menu_about_to_popup() {
    PopupMenu* popup = _add_button->get_popup();
    popup->clear();

    PropertyInfo owner;
    uint32_t target = 0;
    bool constant = false;
    _get_owner(owner, target, constant);

    const Vector<OScriptAnnotationDescriptor>& descriptors = OScriptAnnotationRegistry::get_descriptors();
    for (int i = 0; i < descriptors.size(); i++) {
        const OScriptAnnotationDescriptor& descriptor = descriptors[i];
        if (constant && descriptor.family == StringName(OScriptAnnotationRegistry::FAMILY_EXPORT)) {
            continue;
        }
        if (OScriptAnnotationRegistry::can_add(target, owner, _annotations, descriptor.info.name) != OK) {
            continue;
        }

        const OrchestratorEditorAnnotationPresentation presentation = OrchestratorEditorAnnotationPresentation::get(descriptor.info.name);
        popup->add_item(presentation.label, i);
        popup->set_item_tooltip(popup->get_item_count() - 1, vformat("%s\n%s", descriptor.info.name, presentation.description));
    }

    if (popup->get_item_count() == 0) {
        popup->add_item("No annotations apply to this type");
        popup->set_item_disabled(0, true);
    }
}

void OrchestratorEditorPropertyAnnotations::_add_menu_id_pressed(int p_id) {
    const Vector<OScriptAnnotationDescriptor>& descriptors = OScriptAnnotationRegistry::get_descriptors();
    if (p_id < 0 || p_id >= descriptors.size()) {
        return;
    }

    const OScriptAnnotationDescriptor& descriptor = descriptors[p_id];

    OScriptAnnotation annotation(descriptor.info.name);
    const int required = descriptor.info.arguments.size() - descriptor.info.default_arguments.size();
    for (int i = 0; i < required; i++) {
        annotation.arguments.push_back(_get_argument_default(descriptor, i));
    }

    _annotations.push_back(annotation);
    _expanded = _annotations.size() - 1;
    _form_stale = true;
    _emit();
}

void OrchestratorEditorPropertyAnnotations::_chip_toggled(bool p_pressed, int p_index) {
    _expanded = p_pressed ? p_index : -1;

    for (int i = 0; i < _chips->get_child_count(); i++) {
        const HBoxContainer* chip = cast_to<HBoxContainer>(_chips->get_child(i));
        if (chip && chip->get_child_count() > 0) {
            Button* toggle = cast_to<Button>(chip->get_child(0));
            if (toggle) {
                toggle->set_pressed_no_signal(i == _expanded);
            }
        }
    }

    _rebuild_form();
}

void OrchestratorEditorPropertyAnnotations::_chip_remove_pressed(int p_index) {
    if (p_index < 0 || p_index >= _annotations.size()) {
        return;
    }

    _annotations.remove_at(p_index);
    _expanded = -1;
    _form_stale = true;
    _emit();
}

void OrchestratorEditorPropertyAnnotations::_argument_value_changed(double p_value, int p_argument, bool p_integer) {
    if (p_integer) {
        _set_argument(p_argument, static_cast<int64_t>(p_value));
    } else {
        _set_argument(p_argument, p_value);
    }
}

void OrchestratorEditorPropertyAnnotations::_argument_text_submitted(const String& p_text, int p_argument) {
    _set_argument(p_argument, p_text);
}

void OrchestratorEditorPropertyAnnotations::_argument_focus_exited(int p_argument) {
    if (!_form || _form->get_child_count() <= p_argument * 3 + 1) {
        return;
    }

    const LineEdit* edit = cast_to<LineEdit>(_form->get_child(p_argument * 3 + 1));
    if (edit) {
        _set_argument(p_argument, edit->get_text());
    }
}

void OrchestratorEditorPropertyAnnotations::_argument_toggled(bool p_pressed, int p_argument) {
    _set_argument(p_argument, p_pressed);
}

void OrchestratorEditorPropertyAnnotations::_argument_item_selected(int p_item, int p_argument) {
    if (!_form || _form->get_child_count() <= p_argument * 3 + 1) {
        return;
    }

    const OptionButton* option = cast_to<OptionButton>(_form->get_child(p_argument * 3 + 1));
    if (option) {
        _set_argument(p_argument, option->get_item_metadata(p_item));
    }
}

void OrchestratorEditorPropertyAnnotations::_argument_remove_pressed(int p_argument) {
    if (_expanded < 0 || _expanded >= _annotations.size()) {
        return;
    }

    Array arguments = _annotations[_expanded].arguments.duplicate();
    if (p_argument < 0 || p_argument >= arguments.size()) {
        return;
    }

    arguments.remove_at(p_argument);
    _annotations.write[_expanded].arguments = arguments;
    _form_stale = true;
    _emit();
}

void OrchestratorEditorPropertyAnnotations::_argument_add_pressed() {
    if (_expanded < 0 || _expanded >= _annotations.size()) {
        return;
    }

    const OScriptAnnotationDescriptor* descriptor = OScriptAnnotationRegistry::find(_annotations[_expanded].name);
    if (descriptor == nullptr || !descriptor->is_vararg()) {
        return;
    }

    // Fill the fixed arguments first so the new value lands in the repeating slot.
    Array arguments = _annotations[_expanded].arguments.duplicate();
    const int fixed = descriptor->info.arguments.size() - 1;
    while (arguments.size() < fixed) {
        arguments.push_back(_get_argument_default(*descriptor, arguments.size()));
    }
    arguments.push_back(_get_argument_default(*descriptor, arguments.size()));

    _annotations.write[_expanded].arguments = arguments;
    _form_stale = true;
    _emit();
}

void OrchestratorEditorPropertyAnnotations::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_READY: {
            _add_button = memnew(MenuButton);
            _add_button->set_text("Add");
            _add_button->set_button_icon(SceneUtils::get_editor_icon("Add"));
            _add_button->set_tooltip_text("Add an annotation that applies to this type");
            _add_button->set_focus_mode(FOCUS_NONE);
            _add_button->set_disabled(_read_only);
            _add_button->set_h_size_flags(SIZE_EXPAND_FILL);
            _add_button->get_popup()->connect("about_to_popup", callable_mp_this(_add_menu_about_to_popup));
            _add_button->get_popup()->connect(SceneStringName(id_pressed), callable_mp_this(_add_menu_id_pressed));
            add_child(_add_button);

            _margin = memnew(MarginContainer);
            _margin->set_theme_type_variation("MarginContainer4px");
            set_bottom_editor(_margin);
            add_child(_margin);

            VBoxContainer* outer = memnew(VBoxContainer);
            outer->add_theme_constant_override("separation", 5);
            _margin->add_child(outer);

            _chips = memnew(HFlowContainer);
            _chips->add_theme_constant_override("h_separation", 4);
            _chips->add_theme_constant_override("v_separation", 4);
            outer->add_child(_chips);

            _form_box = memnew(VBoxContainer);
            _form_box->add_theme_constant_override("separation", 5);
            _form_box->hide();
            outer->add_child(_form_box);

            _rebuild_chips();
            _rebuild_form();
            break;
        }
    }
}

void OrchestratorEditorPropertyAnnotations::_update_property() {
    ERR_FAIL_NULL(get_edited_object());

    OScriptAnnotationList list;
    list.from_array(get_edited_object()->get(get_edited_property()));
    _annotations = list.get_items();

    if (_expanded >= _annotations.size()) {
        _expanded = -1;
    }

    // An edit made here rebuilds only what changed, so the control being dragged or typed
    // into survives. Anything else, such as undo or a type change, rebuilds everything.
    if (_self_change) {
        _self_change = false;
        if (_form_stale) {
            _rebuild_chips();
            _rebuild_form();
        } else {
            _refresh_chip_text();
        }
        return;
    }

    _rebuild_chips();
    _rebuild_form();
}

void OrchestratorEditorPropertyAnnotations::_set_read_only(bool p_read_only) {
    _read_only = p_read_only;
    if (_add_button) {
        _add_button->set_disabled(p_read_only);
    }
    _rebuild_chips();
    _rebuild_form();
}
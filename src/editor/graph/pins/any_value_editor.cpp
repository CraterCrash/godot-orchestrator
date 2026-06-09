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
#include "editor/graph/pins/any_value_editor.h"

#include "common/macros.h"
#include "common/property_utils.h"
#include "common/scene_utils.h"
#include "common/variant_utils.h"
#include "core/godot/scene_string_names.h"
#include "editor/graph/graph_pin_factory.h"
#include "orchestration/variable.h"

void OrchestratorEditorGraphPinValueEditorAny::_type_button_pressed() {
    ERR_FAIL_NULL(_type_dialog);

    const String current_type = _pin.is_valid() && _pin->has_type_override()
        ? (_pin->get_type_override().class_name.is_empty()
            ? Variant::get_type_name(_pin->get_type_override().type)
            : String(_pin->get_type_override().class_name))
        : "";

    _type_dialog->popup_create(false, false, current_type, "");
}

void OrchestratorEditorGraphPinValueEditorAny::_type_selected() {
    ERR_FAIL_COND(!_pin.is_valid());

    const String selected = _type_dialog->get_selected_type();

    if (selected == "type:Nil") {
        // Emit before clearing so _pin_default_value_changed sees old != new and marks dirty.
        _emit_value_changed(Variant());
        _pin->clear_type_override();
        return;
    }

    ClassificationParser parser;
    ERR_FAIL_COND(!parser.parse(selected));

    PropertyInfo parsed_property = parser.get_property();
    if ((parsed_property.usage & PROPERTY_USAGE_CLASS_IS_ENUM) || (parsed_property.usage & PROPERTY_USAGE_CLASS_IS_BITFIELD)) {
        parsed_property.hint_string = "";
        parsed_property.hint = 0;
    }

    // Emit the incoming effective default BEFORE applying the override.
    // _pin_default_value_changed compares old vs new; emitting before the model changes ensures it
    // sees old != new and calls _set_edited(true) correctly.
    const bool is_object_ref = parsed_property.type == Variant::OBJECT && !String(parsed_property.class_name).is_empty();
    _emit_value_changed(is_object_ref ? Variant() : VariantUtils::make_default(parsed_property.type));

    _pin->set_type_override(parsed_property);
}

void OrchestratorEditorGraphPinValueEditorAny::_rebuild_inner_editor(const PropertyInfo& p_override) {
    const Variant::Type new_type = p_override.type;
    const String new_class = p_override.class_name;

    if (new_type == _built_for_type && new_class == _built_for_class) {
        return;
    }

    _built_for_type = new_type;
    _built_for_class = new_class;

    if (_inner_editor) {
        _inner_container->remove_child(_inner_editor);
        _inner_editor->queue_free();
        _inner_editor = nullptr;
    }

    // NIL means no override, thus no inner editor needed
    if (new_type == Variant::NIL || !_is_input) {
        _inner_container->set_visible(false);
        return;
    }

    Ref<OrchestrationGraphPin> synthetic = OrchestrationGraphPin::create(nullptr, p_override);
    synthetic->set_direction(PD_Input);

    _inner_editor = OrchestratorEditorGraphPinFactory::create_value_editor(synthetic);
    if (!_inner_editor) {
        _inner_container->set_visible(false);
        return;
    }

    _inner_editor->set_pin_ref(synthetic);
    _inner_editor->configure(p_override);
    _inner_editor->connect("value_changed", callable_mp_this(_on_inner_value_changed));
    _inner_container->add_child(_inner_editor);
    _inner_container->set_visible(true);

    emit_signal("layout_changed");
}

void OrchestratorEditorGraphPinValueEditorAny::_on_inner_value_changed(const Variant& p_value) {
    _emit_value_changed(p_value);
}

void OrchestratorEditorGraphPinValueEditorAny::configure(const PropertyInfo& p_property) {
    // First-time setup
    if (!_type_dialog) {
        _type_dialog = memnew(OrchestratorSelectTypeSearchDialog);
        _type_dialog->set_popup_title("Select Type");
        _type_dialog->set_allow_abstract_types(true);
        _type_dialog->set_data_suffix("any_pin");
        add_child(_type_dialog);
        _type_dialog->connect("selected", callable_mp_this(_type_selected));

        _type_button = memnew(Button);
        _type_button->set_focus_mode(FOCUS_NONE);
        _type_button->set_button_icon(SceneUtils::get_editor_icon("Edit"));
        _type_button->set_tooltip_text("Override pin type");
        _type_button->set_v_size_flags(SIZE_SHRINK_CENTER);
        _type_button->connect(SceneStringName(pressed), callable_mp_this(_type_button_pressed));

        _inner_container = memnew(HBoxContainer);
        _inner_container->add_theme_constant_override("separation", 2);
        _inner_container->set_visible(false);

        if (_is_input) {
            add_child(_inner_container);
            add_child(_type_button);
        } else {
            add_child(_type_button);
        }
    }

    // Rebuild inner editor if the override type changed
    if (!PropertyUtils::is_variant(p_property)) {
        _rebuild_inner_editor(p_property);
    } else {
        // No override, clear inner editor
        _rebuild_inner_editor(PropertyInfo());
    }
}

void OrchestratorEditorGraphPinValueEditorAny::set_value(const Variant& p_value) {
    if (_inner_editor) {
        _inner_editor->set_value(p_value);
    }
}

void OrchestratorEditorGraphPinValueEditorAny::set_linked(bool p_linked) {
    if (_type_button) {
        _type_button->set_visible(!p_linked);
    }
    if (_inner_container) {
        _inner_container->set_visible(!p_linked && _inner_editor != nullptr);
    }
}

void OrchestratorEditorGraphPinValueEditorAny::set_pin_ref(const Ref<OrchestrationGraphPin>& p_pin) {
    _pin = p_pin;
    _is_input = _pin.is_valid() && _pin->is_input();
}

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
#include "editor/graph/graph_pin.h"

#include "../gui/context_menu.h"
#include "common/macros.h"
#include "common/property_utils.h"
#include "common/scene_utils.h"
#include "common/settings.h"
#include "common/string_utils.h"
#include "common/variant_utils.h"
#include "core/godot/core_string_names.h"
#include "editor/graph/graph_panel.h"
#include "script/nodes/functions/call_function.h"

#include <godot_cpp/classes/input_event_mouse_button.hpp>

PackedStringArray OrchestratorEditorGraphPin::_get_pin_suggestions() const {
    ERR_FAIL_COND_V(!_pin.is_valid(), {});
    return _pin->get_suggestions();
}

String OrchestratorEditorGraphPin::_get_pin_color_name() const {
    static String COLOR_ANY = "ui/connection_colors/any";

    ERR_FAIL_COND_V(!_pin.is_valid(), COLOR_ANY);

    const String type_name = VariantUtils::get_friendly_type_name(_pin->get_type(), true).to_lower();
    return vformat("ui/connection_colors/%s", type_name);
}

void OrchestratorEditorGraphPin::_default_value_changed() {
    // Subclasses call this function to synchronize the UI widget value to the Pin
    const Variant default_value = _read_control_value();
    _set_default_value(default_value);
}

Variant OrchestratorEditorGraphPin::_get_default_value() {
    return _pin->get_effective_default_value();
}

void OrchestratorEditorGraphPin::_set_default_value(const Variant& p_value) {
    emit_signal("default_value_changed", this, p_value);
    _node->notify_pin_default_value_changed(this);
}

void OrchestratorEditorGraphPin::_update_control() {
    if (!is_inside_tree()) {
        _dirty = true;
        return;
    }

    set_tooltip_text(_get_tooltip_text());

    if (_default_value && _pin.is_valid()) {
        _update_control_value(_pin->get_effective_default_value());
    }
}

void OrchestratorEditorGraphPin::_create_pin_layout() {
    HBoxContainer* container = memnew(HBoxContainer);
    container->set_h_size_flags(_pin->is_input() ? SIZE_SHRINK_BEGIN : SIZE_SHRINK_END);
    container->set_v_size_flags(SIZE_EXPAND_FILL);
    add_child(container);

    _label = memnew(Label);
    _label->set_horizontal_alignment(_pin->is_input() ? HORIZONTAL_ALIGNMENT_LEFT : HORIZONTAL_ALIGNMENT_RIGHT);
    _label->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
    _label->set_h_size_flags(SIZE_FILL);
    _label->set_v_size_flags(SIZE_SHRINK_CENTER);
    _label->set_text(_get_label_text());
    _label->set_custom_minimum_size(_label->get_text().is_empty() ? Vector2(10, 0) : Vector2());
    container->add_child(_label);

    if (!_pin->is_execution()) {
        const String type_name = _pin->get_pin_type_name();
        const int icon_width = SceneUtils::get_editor_class_icon_size();

        _icon = memnew(TextureRect);
        _icon->set_texture(SceneUtils::get_class_icon(type_name));
        _icon->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
        _icon->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
        _icon->set_custom_minimum_size(Vector2i(icon_width, icon_width));
        _icon->set_visible(false);

        if (ORCHESTRATOR_GET("ui/nodes/show_type_icons", true)) {
            set_icon_visible(true);
        }

        container->add_child(_icon);

        // For input pins, icon shows on the left of the text
        if (_pin->is_input()) {
            container->move_child(_icon, 0);
        }
    }

    if (!_pin->is_execution() && !_pin->is_default_ignored() && _pin->is_input()) {
        _default_value = _create_default_value_widget();
        if (_default_value) {
            _default_value->set_visible(!is_linked());

            // For multiline input, the default value widget is rendered on the second row of
            // the VBoxContainer, which is this class; otherwise, it's appended to the right
            // inside the HBoxContainer on the first row of this class.
            if (_is_default_value_below_label()) {
                add_child(_default_value);
            } else {
                container->add_child(_default_value);
            }
        }
    }
}

String OrchestratorEditorGraphPin::_get_label_text() {
    if (_pin->is_label_visible()) {
        String text = StringUtils::default_if_empty(_pin->get_label(), _pin->get_pin_name());
        if (_pin->use_pretty_labels()) {
            text = text.capitalize();
        }
        return text;
    }

    return "";
}

String OrchestratorEditorGraphPin::_get_tooltip_text() {
    if (is_execution()) {
        return "";
    }

    String tooltip_text = StringUtils::default_if_empty(_pin->get_label(), _pin->get_pin_name()).capitalize();
    tooltip_text += "\n" + VariantUtils::get_friendly_type_name(_pin->get_type(), true).capitalize();

    if (!_pin->get_property_info().class_name.is_empty()) {
        tooltip_text += "\nClass: " + _pin->get_property_info().class_name;
    }

    const bool advanced_tooltips = ORCHESTRATOR_GET("ui/graph/show_advanced_tooltips", false);
    if (advanced_tooltips) {
        const PropertyInfo property = _pin->get_property_info();
        tooltip_text += "\n\n";
        tooltip_text += "Property Name: " + property.name + "\n";
        tooltip_text += "Property Type: " + itos(property.type) + " - " + _pin->get_pin_type_name() + "\n";
        tooltip_text += "Property Class: " + property.class_name + "\n";
        tooltip_text += "Property Hint: " + property.class_name + "\n";
        tooltip_text += "Property Hint String: " + property.hint_string + "\n";
        tooltip_text += "Property Hint Flags: " + itos(property.hint) + "\n";
        tooltip_text += "Property Usage: " + PropertyUtils::usage_to_string(property.usage) + "\n\n";
        tooltip_text += "Default Value: " + vformat("%s", _pin->get_default_value()) + "\n";
        tooltip_text += "Generated Default Value: " + vformat("%s", _pin->get_generated_default_value()) + "\n";
        tooltip_text += "Effective Default Value: " + vformat("%s", _pin->get_effective_default_value());
    }

    return tooltip_text;
}

void OrchestratorEditorGraphPin::_gui_input(const Ref<InputEvent>& p_event) {
    // The OrchestratorEditorGraphPanel reacts to this if subscribed
    const Ref<InputEventMouseButton> mb = p_event;
    if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == MOUSE_BUTTON_RIGHT) {
        emit_signal("context_menu_requested", this, mb->get_position());
    }
}

OrchestratorEditorGraphPanel* OrchestratorEditorGraphPin::get_graph() {
    return _node->get_graph();
}

void OrchestratorEditorGraphPin::set_graph_node(OrchestratorEditorGraphNode* p_owner_node) {
    CRASH_COND(!p_owner_node);

    _node = p_owner_node;
}

void OrchestratorEditorGraphPin::set_pin(const Ref<OrchestrationGraphPin>& p_pin) {
    ERR_FAIL_COND_MSG(_pin.is_valid(), "A pin is already set on the editor graph pin instance.");

    _pin = p_pin;

    _create_pin_layout();
    _update_control();

    _pin->connect(CoreStringName(changed), callable_mp_this(_update_control));
}

String OrchestratorEditorGraphPin::get_pin_name() const {
    return _pin->get_pin_name();
}

EPinDirection OrchestratorEditorGraphPin::get_direction() const {
    return _pin->get_direction();
}

bool OrchestratorEditorGraphPin::is_execution() const {
    return _pin->is_execution();
}

bool OrchestratorEditorGraphPin::is_linked() const {
    return _pin->has_any_connections();
}

bool OrchestratorEditorGraphPin::is_hidden() const {
    return _pin->is_hidden();
}

bool OrchestratorEditorGraphPin::is_connectable() const {
    return _pin->is_connectable();
}

bool OrchestratorEditorGraphPin::is_target_self() const {
    return _pin->is_target_self();
}

bool OrchestratorEditorGraphPin::is_autowire_enabled() const {
    return _pin->can_autowire();
}

OrchestratorEditorGraphPinSlotInfo OrchestratorEditorGraphPin::get_slot_info() const {
    ERR_FAIL_COND_V_MSG(!_pin.is_valid(), {}, "Can't get slot info, pin invalid");

    OrchestratorEditorGraphPinSlotInfo info;
    info.enabled = _pin->is_connectable() && !_pin->is_hidden();
    info.type    = _pin->is_execution() ? 0 : 1;
    info.icon    = _pin->is_execution() ? "VisualShaderPort" : "GuiGraphNodePort";
    info.color   = ORCHESTRATOR_GET(_get_pin_color_name(), Color(1.0, 1.0, 1.0, 1.0));

    return info;
}

const PropertyInfo& OrchestratorEditorGraphPin::get_property_info() const {
    return _pin->get_property_info();
}

void OrchestratorEditorGraphPin::set_default_value_control_visible(bool p_visible) {
    GUARD_NULL(_default_value);
    _default_value->set_visible(p_visible);
}

void OrchestratorEditorGraphPin::set_icon_visible(bool p_visible) {
    GUARD_NULL(_icon);

    if (_icon->is_visible() != p_visible && !_pin->is_hidden()) {
        _icon->set_visible(p_visible);
    }
}

void OrchestratorEditorGraphPin::set_show_advanced_tooltips(bool p_show_advanced_tooltips) {
    set_tooltip_text(_get_tooltip_text());
}

void OrchestratorEditorGraphPin::_notification(int p_what) {
    if (p_what == NOTIFICATION_READY && _dirty) {
        _update_control();
        _dirty = false;
    }
}

void OrchestratorEditorGraphPin::_bind_methods() {
    ADD_SIGNAL(MethodInfo("context_menu_requested", PropertyInfo(Variant::OBJECT, "pin"), PropertyInfo(Variant::VECTOR2, "position")));
    ADD_SIGNAL(MethodInfo("default_value_changed", PropertyInfo(Variant::OBJECT, "pin"), PropertyInfo(Variant::NIL, "value")));
}

OrchestratorEditorGraphPin::OrchestratorEditorGraphPin() {
    set_h_size_flags(SIZE_EXPAND_FILL);
}
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

#include "common/macros.h"
#include "common/property_utils.h"
#include "common/scene_utils.h"
#include "common/settings.h"
#include "common/string_utils.h"
#include "common/variant_utils.h"
#include "core/godot/core_string_names.h"
#include "editor/graph/graph_node.h"
#include "editor/graph/graph_panel.h"
#include "editor/graph/graph_pin_factory.h"
#include "editor/graph/pins/any_value_editor.h"
#include "editor/graph/pins/pin_value_editor.h"
#include "editor/gui/context_menu.h"
#include "orchestration/nodes/reroute.h"

#include <godot_cpp/classes/input_event_mouse_button.hpp>

String OrchestratorEditorGraphPin::_get_pin_color_name() const {
    static String COLOR_ANY = "ui/connection_colors/any";

    ERR_FAIL_COND_V(!_pin.is_valid(), COLOR_ANY);

    if (_pin->is_execution()) {
        return "ui/connection_colors/execution";
    }

    // Any pin with a type override uses the override's color
    if (PropertyUtils::is_variant(_pin->get_property_info()) && _pin->has_type_override()) {
        const String type_name = VariantUtils::get_friendly_type_name(_pin->get_type_override().type, true).to_lower();
        return vformat("ui/connection_colors/%s", type_name);
    }

    const String type_name = VariantUtils::get_friendly_type_name(_pin->get_type(), true).to_lower();
    return vformat("ui/connection_colors/%s", type_name);
}

void OrchestratorEditorGraphPin::_update_control() {
    if (!is_inside_tree()) {
        _dirty = true;
        return;
    }

    set_tooltip_text(_get_tooltip_text());
    _update_icon_texture();

    if (_editor && _pin.is_valid()) {
        // Re-configure for any-pin type override changes
        if (PropertyUtils::is_variant(_pin->get_property_info())) {
            _editor->configure(_effective_property_info());
        }
        if (_pin->is_input()) {
            _editor->set_value(_pin->get_effective_default_value());
        }
        _editor->set_linked(is_linked());
    }

    // Refresh slot color after any type override change
    if (OrchestratorEditorGraphNode* node = get_graph_node()) {
        node->redraw_connections();
    }
}

void OrchestratorEditorGraphPin::_create_pin_layout() {
    _layout_container = memnew(HBoxContainer);
    _layout_container->set_h_size_flags(_pin->is_input() ? SIZE_SHRINK_BEGIN : SIZE_SHRINK_END);
    _layout_container->set_v_size_flags(SIZE_EXPAND_FILL);
    add_child(_layout_container);

    _label = memnew(Label);
    _label->set_horizontal_alignment(_pin->is_input() ? HORIZONTAL_ALIGNMENT_LEFT : HORIZONTAL_ALIGNMENT_RIGHT);
    _label->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
    _label->set_h_size_flags(SIZE_FILL);
    _label->set_v_size_flags(SIZE_SHRINK_CENTER);
    _label->set_text(_get_label_text());
    _label->set_custom_minimum_size(_label->get_text().is_empty() ? Vector2(10, 0) : Vector2());
    _layout_container->add_child(_label);

    if (!_pin->is_execution()) {
        const int icon_width = SceneUtils::get_editor_class_icon_size();

        _icon = memnew(TextureRect);
        _icon->set_texture(SceneUtils::get_class_icon(_get_icon_type_name()));
        _icon->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
        _icon->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
        _icon->set_custom_minimum_size(Vector2i(icon_width, icon_width));
        _icon->set_visible(false);

        if (ORCHESTRATOR_GET("ui/nodes/show_type_icons", true)) {
            set_icon_visible(true);
        }

        _layout_container->add_child(_icon);

        if (_pin->is_input()) {
            _layout_container->move_child(_icon, 0);
        }
    }

    if (!_pin->is_execution() && !_pin->is_default_ignored()) {
        const bool is_any = PropertyUtils::is_variant(_pin->get_property_info());

        // Create a value editor for input pins, or for output any-pins (pencil button).
        if (_pin->is_input() || is_any) {
            _editor = OrchestratorEditorGraphPinFactory::create_value_editor(_pin);
        }

        if (_editor) {
            _editor->set_pin_ref(_pin);
            _editor->configure(_effective_property_info());
            if (_pin->is_input()) {
                _editor->set_value(_pin->get_effective_default_value());
            }
            _editor->set_linked(is_linked());
            _editor->connect("value_changed", callable_mp_this(_pin_editor_value_changed));
            _editor->connect("layout_changed", callable_mp_this(_pin_editor_layout_changed));

            if (_editor->is_below_label()) {
                add_child(_editor);
            } else if (_editor->prefers_leading_placement()) {
                _layout_container->add_child(_editor);
                _layout_container->move_child(_editor, 0);
            } else {
                _layout_container->add_child(_editor);
            }
        }
    }

    _on_pin_layout_created();
}

String OrchestratorEditorGraphPin::_get_icon_type_name() const {
    // Any pin with a type override shows the override type's icon
    if (PropertyUtils::is_variant(_pin->get_property_info()) && _pin->has_type_override()) {
        const PropertyInfo& override = _pin->get_type_override();
        if (override.type == Variant::OBJECT && !override.class_name.is_empty()) {
            return String(override.class_name);
        }
        return Variant::get_type_name(override.type);
    }
    return _pin->get_pin_type_name();
}

String OrchestratorEditorGraphPin::_get_label_text() {
    // Object pins with a self-target show "[Self]"
    if (_pin->get_type() == Variant::OBJECT && is_target_self()) {
        return "[Self]";
    }

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

    // Any pin with a type override gets a specialized tooltip
    if (PropertyUtils::is_variant(_pin->get_property_info()) && _pin->has_type_override()) {
        const PropertyInfo& override_prop = _pin->get_type_override();
        const String effective_type = override_prop.class_name.is_empty()
            ? VariantUtils::get_friendly_type_name(override_prop.type, false)
            : String(override_prop.class_name);

        String tooltip_text = StringUtils::default_if_empty(_pin->get_label(), _pin->get_pin_name()).capitalize();
        tooltip_text += "\nAny Overridden As " + effective_type.capitalize();

        if (!override_prop.class_name.is_empty()) {
            tooltip_text += "\nClass: " + override_prop.class_name;
        }
        return tooltip_text;
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

void OrchestratorEditorGraphPin::_pin_editor_value_changed(const Variant& p_value) {
    emit_signal("default_value_changed", this, p_value);
    if (_node) {
        _node->notify_pin_default_value_changed(this);
    }
}

void OrchestratorEditorGraphPin::_pin_editor_layout_changed() {
    if (_node) {
        _node->notify_pin_layout_changed();
    }
}

PropertyInfo OrchestratorEditorGraphPin::_effective_property_info() const {
    if (PropertyUtils::is_variant(_pin->get_property_info()) && _pin->has_type_override()) {
        return _pin->get_type_override();
    }
    return _pin->get_property_info();
}

void OrchestratorEditorGraphPin::_update_icon_texture() {
    GUARD_NULL(_icon);
    _icon->set_texture(SceneUtils::get_class_icon(_get_icon_type_name()));
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

    const OScriptNodeReroute* reroute = cast_to<OScriptNodeReroute>(_pin->get_owning_node());
    if (reroute) {
        // Reroutes always use the round port icon regardless of whether they carry
        // execution or data flow. Slot type drives connection compatibility:
        //   2 = ANY  (connects to both exec and data)
        //   0 = CONTROL (execution)
        //   1 = DATA
        switch (reroute->get_reroute_type()) {
            case OScriptNodeReroute::REROUTE_CONTROL:
                info.type  = 0;
                info.color = ORCHESTRATOR_GET(_get_pin_color_name(), Color(1.0, 1.0, 1.0, 1.0));
                break;
            case OScriptNodeReroute::REROUTE_DATA:
                info.type  = 1;
                info.color = ORCHESTRATOR_GET(_get_pin_color_name(), Color(1.0, 1.0, 1.0, 1.0));
                break;
            default: // REROUTE_ANY
                info.type  = 2;
                info.color = Color(0.78f, 0.78f, 0.78f, 1.0f);
                break;
        }
        info.icon = "GuiGraphNodePort";
    } else {
        info.type  = _pin->is_execution() ? 0 : 1;
        info.icon  = _pin->is_execution() ? "VisualShaderPort" : "GuiGraphNodePort";
        info.color = ORCHESTRATOR_GET(_get_pin_color_name(), Color(1.0, 1.0, 1.0, 1.0));
    }

    return info;
}

const PropertyInfo& OrchestratorEditorGraphPin::get_property_info() const {
    return _pin->get_property_info();
}

void OrchestratorEditorGraphPin::set_default_value_control_visible(bool p_visible) {
    GUARD_NULL(_editor);
    _editor->set_visible(p_visible);
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

OrchestratorEditorGraphPin* OrchestratorEditorGraphPin::create(const Ref<OrchestrationGraphPin>& p_pin) {
    ERR_FAIL_COND_V_MSG(!p_pin.is_valid(), nullptr, "Cannot create pin widget for an invalid pin model");
    OrchestratorEditorGraphPin* pin = memnew(OrchestratorEditorGraphPin);
    pin->set_pin(p_pin);
    return pin;
}

OrchestratorEditorGraphPin::OrchestratorEditorGraphPin() {
    set_h_size_flags(SIZE_EXPAND_FILL);
}
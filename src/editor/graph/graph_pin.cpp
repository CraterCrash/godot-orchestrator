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
#include "core/godot/object/type_resolver.h"
#include "editor/graph/graph_panel.h"
#include "editor/gui/context_menu.h"
#include "orchestration/nodes/reroute.h"
#include "orchestration/nodes/self.h"
#include "orchestration/orchestration.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/style_box_empty.hpp>

void OrchestratorEditorGraphPin::IconCache::clear() {
    if (container) {
        container->queue_free();
        container = nullptr;
        primary = nullptr;
        element = nullptr;
        key = nullptr;
        value = nullptr;
    }
}

Vector2i OrchestratorEditorGraphPin::_get_icon_scaled_size(const Ref<Texture2D>& p_icon, int p_editor_icon_size) const {
    // Preserve the texture's aspect ratio so non-square icons (e.g. 16x13) are not skewed.
    // Scale so the largest dimension equals icon_width and let the smaller dimension follow.
    Vector2i icon_size = Vector2i(p_editor_icon_size, p_editor_icon_size);
    if (p_icon.is_valid()) {
        const Size2 texture_size = p_icon->get_size();
        const float largest = MAX(texture_size.x, texture_size.y);
        if (largest > 0) {
            const float scale = p_editor_icon_size / largest;
            icon_size = Vector2i(Math::round(texture_size.x * scale), Math::round(texture_size.y * scale));
        }
    }
    return icon_size;
}

void OrchestratorEditorGraphPin::_update_icon_control() {
    const int icon_width = SceneUtils::get_editor_class_icon_size();

    if (icon_cache.primary) {
        Ref<Texture2D> type_icon;
        if (_pin->get_owning_node()->is_type<OScriptNodeSelf>()) {
            // Hack for when a named script transitions from unnamed to named before save
            type_icon = _pin->get_owning_node()->get_orchestration()->get_icon();
        } else {
            type_icon = SceneUtils::get_class_icon(_pin->get_pin_type_name());
        }

        icon_cache.primary->set_texture(type_icon);
        icon_cache.primary->set_custom_minimum_size(_get_icon_scaled_size(type_icon, icon_width));
    }

    const PropertyInfo property = _pin->get_property_info();

    if (property.type == Variant::ARRAY && property.hint == PROPERTY_HINT_ARRAY_TYPE) {
        const Ref<Texture2D> element_icon = SceneUtils::get_class_icon(property.hint_string);
        if (icon_cache.element && element_icon.is_valid()) {
            icon_cache.element->set_texture(element_icon);
            icon_cache.element->set_custom_minimum_size(_get_icon_scaled_size(element_icon, icon_width));
        }
    }

    if (property.type == Variant::DICTIONARY && property.hint == PROPERTY_HINT_DICTIONARY_TYPE) {
        const PackedStringArray hints = property.hint_string.split(";");

        const Ref<Texture2D> key_icon = SceneUtils::get_class_icon(hints[0]);
        if (icon_cache.key && key_icon.is_valid()) {
            icon_cache.key->set_texture(key_icon);
            icon_cache.key->set_custom_minimum_size(_get_icon_scaled_size(key_icon, icon_width));
        }

        const Ref<Texture2D> value_icon = SceneUtils::get_class_icon(hints[1]);
        if (icon_cache.value && value_icon.is_valid()) {
            icon_cache.value->set_texture(value_icon);
            icon_cache.value->set_custom_minimum_size(_get_icon_scaled_size(value_icon, icon_width));
        }
    }
}

Control* OrchestratorEditorGraphPin::_create_icon_cache() {
    // Clear the container if it's asked to be recreated and exists.
    // This should free all the child nodes
    if (icon_cache.container) {
        icon_cache.container->queue_free();
    }

    icon_cache.container = memnew(HBoxContainer);
    icon_cache.container->add_theme_constant_override("separation", 0);
    icon_cache.container->set_visible(false);

    icon_cache.primary = memnew(TextureRect);
    icon_cache.primary->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
    icon_cache.primary->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
    icon_cache.primary->set_visible(true);
    icon_cache.container->add_child(icon_cache.primary);

    const PropertyInfo property = _pin->get_property_info();
    if (property.type == Variant::ARRAY && property.hint == PROPERTY_HINT_ARRAY_TYPE) {
        Ref<StyleBoxEmpty> left_empty = memnew(StyleBoxEmpty);
        left_empty->set_content_margin_all(4);
        left_empty->set_content_margin(SIDE_RIGHT, 0);

        Label* lbracket = memnew(Label);
        lbracket->set_text("[");
        lbracket->set_clip_text(false);
        lbracket->add_theme_font_size_override("font_size", 15);
        lbracket->add_theme_stylebox_override("normal", left_empty);
        icon_cache.container->add_child(lbracket);

        icon_cache.element = memnew(TextureRect);
        icon_cache.element->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
        icon_cache.element->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
        icon_cache.element->set_visible(true);
        icon_cache.container->add_child(icon_cache.element);

        Ref<StyleBoxEmpty> right_empty = memnew(StyleBoxEmpty);
        right_empty->set_content_margin_all(4);
        right_empty->set_content_margin(SIDE_LEFT, 0);

        Label* rbracket = memnew(Label);
        rbracket->set_text("]");
        rbracket->set_clip_text(false);
        rbracket->add_theme_font_size_override("font_size", 15);
        rbracket->add_theme_stylebox_override("normal", right_empty);
        rbracket->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
        icon_cache.container->add_child(rbracket);
    }

    if (property.type == Variant::DICTIONARY && property.hint == PROPERTY_HINT_DICTIONARY_TYPE) {
        Ref<StyleBoxEmpty> left_empty = memnew(StyleBoxEmpty);
        left_empty->set_content_margin_all(4);
        left_empty->set_content_margin(SIDE_RIGHT, 0);

        Label* lbracket = memnew(Label);
        lbracket->set_text("[");
        lbracket->set_clip_text(false);
        lbracket->add_theme_font_size_override("font_size", 15);
        lbracket->add_theme_stylebox_override("normal", left_empty);
        icon_cache.container->add_child(lbracket);

        icon_cache.key = memnew(TextureRect);
        icon_cache.key->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
        icon_cache.key->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
        icon_cache.key->set_visible(true);
        icon_cache.container->add_child(icon_cache.key);

        Control* c = memnew(Control);
        c->set_custom_minimum_size(Size2(8, 0) * EDSCALE);
        icon_cache.container->add_child(c);

        icon_cache.value = memnew(TextureRect);
        icon_cache.value->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
        icon_cache.value->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
        icon_cache.value->set_visible(true);
        icon_cache.container->add_child(icon_cache.value);

        Ref<StyleBoxEmpty> right_empty = memnew(StyleBoxEmpty);
        right_empty->set_content_margin_all(4);
        right_empty->set_content_margin(SIDE_LEFT, 0);

        Label* rbracket = memnew(Label);
        rbracket->set_text("]");
        rbracket->set_clip_text(false);
        rbracket->add_theme_font_size_override("font_size", 15);
        rbracket->add_theme_stylebox_override("normal", right_empty);
        rbracket->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
        icon_cache.container->add_child(rbracket);
    }

    // Updates icons
    _update_icon_control();

    return icon_cache.container;
}

PackedStringArray OrchestratorEditorGraphPin::_get_pin_suggestions() const {
    ERR_FAIL_COND_V(!_pin.is_valid(), {});
    return _pin->get_suggestions();
}

String OrchestratorEditorGraphPin::_get_pin_color_name() const {
    static String COLOR_ANY = "interface/theme/connection_colors/any";

    ERR_FAIL_COND_V(!_pin.is_valid(), COLOR_ANY);

    const String type_name = VariantUtils::get_friendly_type_name(_pin->get_type(), true).to_lower().replace(" ", "_");
    return vformat("interface/theme/connection_colors/%s", type_name);
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

    if (_pin->is_input()) {
        Ref<StyleBoxEmpty> empty_sb = memnew(StyleBoxEmpty);
        empty_sb->set_content_margin_all(4);
        empty_sb->set_content_margin(SIDE_RIGHT, 8);
        _label->add_theme_stylebox_override("normal", empty_sb);
    } else {
        Ref<StyleBoxEmpty> empty_sb = memnew(StyleBoxEmpty);
        empty_sb->set_content_margin_all(4);
        empty_sb->set_content_margin(SIDE_LEFT, 8);
        _label->add_theme_stylebox_override("normal", empty_sb);
    }

    if (!_pin->is_execution()) {
        _icon = _create_icon_cache();

        if (ORCHESTRATOR_GET("interface/editor/graph_nodes/show_type_icons", true)) {
            set_icon_visible(true);
        }
    }

    if (_pin->is_input()) {
        if (_icon) {
            container->add_child(_icon);
        }
        container->add_child(_label);
    } else {
        container->add_child(_label);
        if (_icon) {
            container->add_child(_icon);
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

    const PropertyInfo property = _pin->get_property_info();

    String tooltip_text = StringUtils::default_if_empty(_pin->get_label(), _pin->get_pin_name()).capitalize();
    tooltip_text += "\n" + VariantUtils::get_friendly_type_name(property.type, true).capitalize();

    if (property.type == Variant::ARRAY && property.hint == PROPERTY_HINT_ARRAY_TYPE) {
        tooltip_text += "\nElement: " + property.hint_string;
    } else if (property.type == Variant::DICTIONARY && property.hint == PROPERTY_HINT_DICTIONARY_TYPE) {
        const PackedStringArray hints = property.hint_string.split(";");
        tooltip_text += "\nKey: " + hints[0] + "\nValue: " + hints[1];
    }

    if (!_pin->get_property_info().class_name.is_empty()) {
        tooltip_text += "\nClass: " + _pin->get_property_info().class_name;
    }

    if (_pin->get_owning_node()->can_change_pin_type(_pin)) {
        tooltip_text += "\n\nRight click this pin to convert its type.";
    }

    const bool advanced_tooltips = ORCHESTRATOR_GET("interface/editor/graph/show_advanced_tooltips", false);
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

Object* OrchestratorEditorGraphPin::_make_custom_tooltip(const String& p_for_text) const {
    MarginContainer* mc = memnew(MarginContainer);
    mc->add_theme_constant_override("margin_left", 10);
    mc->add_theme_constant_override("margin_top", 10);
    mc->add_theme_constant_override("margin_right", 10);
    mc->add_theme_constant_override("margin_bottom", 10);

    HBoxContainer* hbox = memnew(HBoxContainer);
    mc->add_child(hbox);

    Control* icon = cast_to<Control>(_icon->duplicate());
    if (icon) {
        icon->set_v_size_flags(SIZE_SHRINK_BEGIN);
        icon->set_visible(true);

        const float texture_size = 32.0 * EDSCALE;
        const TypedArray<Node> textures = icon->find_children("*", TextureRect::get_class_static(), true, false);
        for (int i = 0; i < textures.size(); i++) {
            TextureRect* texture = cast_to<TextureRect>(textures[i]);
            if (texture) {
                texture->set_custom_minimum_size(_get_icon_scaled_size(texture->get_texture(), texture_size));
            }
        }

        const float label_size = 30.0 * EDSCALE;
        const TypedArray<Node> labels = icon->find_children("*", Label::get_class_static(), true, false);
        for (int i = 0; i < labels.size(); i++) {
            Label* label = cast_to<Label>(labels[i]);
            if (label) {
                label->add_theme_font_size_override("font_size", label_size);
            }
        }

        hbox->add_child(icon);
    }

    Label* label = memnew(Label);
    label->set_text(p_for_text);
    hbox->add_child(label);

    return mc;
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
    switch (p_what) {
        case NOTIFICATION_READY: {
            if (_dirty) {
                _update_control();
                _dirty = false;
            }
            break;
        }
        case NOTIFICATION_THEME_CHANGED: {
            _update_icon_control();
            if (_label) {
                _label->add_theme_color_override("font_color", get_theme_color("font_color", "GraphNode"));
            }
            break;
        }
    }
}

void OrchestratorEditorGraphPin::_bind_methods() {
    ADD_SIGNAL(MethodInfo("context_menu_requested", PropertyInfo(Variant::OBJECT, "pin"), PropertyInfo(Variant::VECTOR2, "position")));
    ADD_SIGNAL(MethodInfo("default_value_changed", PropertyInfo(Variant::OBJECT, "pin"), PropertyInfo(Variant::NIL, "value")));
}

OrchestratorEditorGraphPin::OrchestratorEditorGraphPin() {
    set_h_size_flags(SIZE_EXPAND_FILL);
}
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
#include "editor/inspector/properties/type_selector.h"

#include "api/extension_db.h"
#include "common/dictionary_utils.h"
#include "common/macros.h"
#include "common/scene_utils.h"
#include "common/variant_utils.h"
#include "core/godot/object/type_resolver.h"
#include "core/godot/scene_string_names.h"
#include "editor/gui/select_type_dialog.h"

OrchestratorEditorTypeSelector::DictionaryHintParts OrchestratorEditorTypeSelector::DictionaryHintParts::parse(const String& p_hint_string) {
    const PackedStringArray parts = p_hint_string.split(";");
    return {
        parts.size() >= 1 ? parts[0] : "Variant",
        parts.size() >= 2 ? parts[1] : "Variant"
    };
}

void OrchestratorEditorTypeSelector::_normalize_inbound(PropertyInfo& r_property) {
    // Internally the widget always works with typed arrays/dictionaries
    switch (r_property.type) {
        case Variant::ARRAY: {
            r_property.hint = PROPERTY_HINT_ARRAY_TYPE;
            if (r_property.hint_string.is_empty()) {
                r_property.hint_string = "Variant";
            }
            break;
        }
        case Variant::DICTIONARY: {
            r_property.hint = PROPERTY_HINT_DICTIONARY_TYPE;
            if (r_property.hint_string.is_empty()) {
                r_property.hint_string = "Variant;Variant";
            }
            break;
        }
    }
}

void OrchestratorEditorTypeSelector::_normalize_outbound(PropertyInfo& r_property) {
    // Externally, full variant typed array/dictionaries should be untyped
    switch (r_property.type) {
        case Variant::ARRAY: {
            if (r_property.hint == PROPERTY_HINT_ARRAY_TYPE &&
                    r_property.hint_string == "Variant") {
                r_property.hint = PROPERTY_HINT_NONE;
                r_property.hint_string = "";
            }
            break;
        }
        case Variant::DICTIONARY: {
            if (r_property.hint == PROPERTY_HINT_DICTIONARY_TYPE &&
                    r_property.hint_string == "Variant;Variant") {
                r_property.hint = PROPERTY_HINT_NONE;
                r_property.hint_string = "";
            }
            break;
        }
    }
}

bool OrchestratorEditorTypeSelector::_get_variant_name_to_type(const String& p_name, Variant::Type& r_type) const {
    const Variant::Type* type_ptr = _variant_named_types.getptr(p_name);
    if (type_ptr) {
        r_type = *type_ptr;
        return true;
    }
    return false;
}

String OrchestratorEditorTypeSelector::_pack_hint_string(const PropertyInfo& p_property) {
    // Packs a single property info into a hint_string canonical value
    // Useful when converting to typed array element or typed dictionary key

    if (p_property.type == Variant::OBJECT && !p_property.class_name.is_empty()) {
        return p_property.class_name;
    }

    if (p_property.type == Variant::INT &&
            p_property.usage & (PROPERTY_USAGE_CLASS_IS_ENUM | PROPERTY_USAGE_CLASS_IS_BITFIELD)) {
        return p_property.class_name;
    }

    if (p_property.type == Variant::NIL && p_property.usage & PROPERTY_USAGE_NIL_IS_VARIANT) {
        return "Variant";
    }

    return Variant::get_type_name(p_property.type);
}

PropertyInfo OrchestratorEditorTypeSelector::_unpack_hint_string(const String& p_hint_string) {
    // Unpacks a single canonical name (element, key, or value type) into a PropertyInfo

    PropertyInfo property;
    property.hint = PROPERTY_HINT_NONE;
    property.hint_string = "";
    property.class_name = "";
    property.usage = PROPERTY_USAGE_DEFAULT;

    if (p_hint_string.is_empty() || p_hint_string == "Variant") {
        property.usage |= PROPERTY_USAGE_NIL_IS_VARIANT;
        property.type = Variant::NIL;
        return property;
    }

    Variant::Type type;
    if (_get_variant_name_to_type(p_hint_string, type)) {
        property.type = type;
        return property;
    }

    const int64_t dot = p_hint_string.find(".");
    if (dot == -1) {
        if (ExtensionDB::get_global_enum_names().has(p_hint_string)) {
            property.type = Variant::INT;
            property.class_name = p_hint_string;

            const EnumInfo enum_info = ExtensionDB::get_global_enum(p_hint_string);
            if (enum_info.is_bitfield) {
                property.usage |= PROPERTY_USAGE_CLASS_IS_BITFIELD;
            } else {
                property.usage |= PROPERTY_USAGE_CLASS_IS_ENUM;
            }

            return property;
        }
    } else {
        const String class_name = p_hint_string.substr(0, dot);
        const String enum_name = p_hint_string.substr(dot + 1);
        const bool is_bitfield = ClassDB::is_class_enum_bitfield(class_name, enum_name);

        property.type = Variant::INT;
        property.class_name = p_hint_string;
        if (is_bitfield) {
            property.usage |= PROPERTY_USAGE_CLASS_IS_BITFIELD;
        } else {
            property.usage |= PROPERTY_USAGE_CLASS_IS_ENUM;
        }

        return property;
    }

    property.type = Variant::OBJECT;
    property.class_name = p_hint_string;

    return property;
}

OrchestratorEditorTypeSelector::ContainerShape OrchestratorEditorTypeSelector::_get_container_shape(const PropertyInfo& p_property) const {
    switch (p_property.type) {
        case Variant::ARRAY:
            return ARRAY;
        case Variant::DICTIONARY:
            return DICTIONARY;
        default:
            return NONE;
    }
}

void OrchestratorEditorTypeSelector::_container_type_changed(int p_index) {
    const ContainerShape source_type = _get_container_shape(_property);
    const ContainerShape target_type = static_cast<ContainerShape>(p_index);

    if (source_type == target_type) {
        return;
    }

    String primary;
    switch (source_type) {
        case NONE: {
            primary = _pack_hint_string(_property);
            break;
        }
        case ARRAY: {
            primary = _property.hint_string;
            break;
        }
        case DICTIONARY: {
            primary = DictionaryHintParts::parse(_property.hint_string).key;
            break;
        }
    }

    PropertyInfo property;
    switch (target_type) {
        case NONE: {
            property = _unpack_hint_string(primary);
            break;
        }
        case ARRAY: {
            property.type = Variant::ARRAY;
            property.hint = PROPERTY_HINT_ARRAY_TYPE;
            property.hint_string = primary;
            property.class_name = "";
            property.usage = PROPERTY_USAGE_DEFAULT;
            break;
        }
        case DICTIONARY: {
            property.type = Variant::DICTIONARY;
            property.hint = PROPERTY_HINT_DICTIONARY_TYPE;
            property.hint_string = primary + ";Variant";
            property.class_name = "";
            property.usage = PROPERTY_USAGE_DEFAULT;
            break;
        }
    }

    _property = property;

    _update();
    _emit_property_changed();
}

void OrchestratorEditorTypeSelector::_left_type_pressed() {
    String title;
    switch (_container_type->get_selected()) {
        case NONE: {
            title = "Select Type";
            break;
        }
        case ARRAY: {
            title = "Select Array Element Type";
            break;
        }
        case DICTIONARY: {
            title = "Select Dictionary Key Type";
            break;
        }
    }

    _open_type_dialog(title, callable_mp_this(_left_type_selected));
}

void OrchestratorEditorTypeSelector::_left_type_selected(OrchestratorSelectTypeSearchDialog* p_dialog) {
    if (!p_dialog) {
        return;
    }

    const PropertyInfo selection = p_dialog->get_selected();
    p_dialog->queue_free();

    PropertyInfo property;
    switch (_get_container_shape(_property)) {
        case NONE: {
            property = selection;
            break;
        }
        case ARRAY: {
            property = _property;
            property.hint_string = _pack_hint_string(selection);
            emit_signal("changed", DictionaryUtils::from_property(property));
            break;
        }
        case DICTIONARY: {
            property = _property;
            property.hint_string = vformat("%s;%s",
                _pack_hint_string(selection), DictionaryHintParts::parse(_property.hint_string).value);
            break;
        }
    }

    _property = property;

    _update();
    _emit_property_changed();
}

void OrchestratorEditorTypeSelector::_right_type_pressed() {
    _open_type_dialog("Select Dictionary Value Type", callable_mp_this(_right_type_selected));
}

void OrchestratorEditorTypeSelector::_right_type_selected(OrchestratorSelectTypeSearchDialog* p_dialog) {
    if (p_dialog) {
        const PropertyInfo selection = p_dialog->get_selected();
        p_dialog->queue_free();

        ERR_FAIL_COND(_get_container_shape(_property) != DICTIONARY);

        PropertyInfo property = _property;
        property.hint = PROPERTY_HINT_DICTIONARY_TYPE;
        property.hint_string = vformat("%s;%s", DictionaryHintParts::parse(_property.hint_string).key, _pack_hint_string(selection));
        property.class_name = "";
        property.usage = PROPERTY_USAGE_DEFAULT;

        _property = property;

        _update();
        _emit_property_changed();
    }
}

void OrchestratorEditorTypeSelector::_open_type_dialog(const String& p_title, const Callable& p_select_callback) {
    HashSet<StringName> exclusions;
    exclusions.insert("Array");
    exclusions.insert("Dictionary");

    for (const String& user_exclusion : _user_exclusions) {
        exclusions.insert(user_exclusion);
    }

    OrchestratorSelectTypeSearchDialog* dialog = memnew(OrchestratorSelectTypeSearchDialog);
    dialog->connect("selected", p_select_callback.bind(dialog));
    dialog->connect("closed", callable_mp_this(_type_dialog_closed).bind(dialog));
    dialog->set_data_suffix(_cache_name);
    dialog->set_base_type("");
    dialog->set_allow_abstract_types(_allow_abstract_types);
    dialog->set_exclusions(exclusions);
    dialog->set_popup_title(p_title);

    add_child(dialog);
    dialog->popup_create(true, true, "", "");
}

void OrchestratorEditorTypeSelector::_type_dialog_closed(OrchestratorSelectTypeSearchDialog* p_dialog) {
    if (p_dialog) {
        p_dialog->queue_free();
    }
}

void OrchestratorEditorTypeSelector::_emit_property_changed() {
    PropertyInfo outbound = _property;
    _normalize_outbound(outbound);
    emit_signal("changed", DictionaryUtils::to_property(outbound));
}

void OrchestratorEditorTypeSelector::_update() {
    const Vector<GDE::GodotType> type_infos = GDE::TypeResolver::resolve(_property);
    switch (_get_container_shape(_property)) {
        case NONE: {
            _container_type->select(0);
            _left_type->set_text(type_infos[0].name);
            _left_type->set_button_icon(SceneUtils::get_class_icon(type_infos[0].name));
            _left_type->set_tooltip_text("Set variable type");
            _right_type->set_visible(false);
            break;
        }
        case ARRAY: {
            _container_type->select(1);
            _left_type->set_text(type_infos[1].name);
            _left_type->set_button_icon(SceneUtils::get_class_icon(type_infos[1].name));
            _left_type->set_tooltip_text("Set array element type");
            _right_type->set_visible(false);
            break;
        }
        case DICTIONARY: {
            _container_type->select(2);
            _left_type->set_text(type_infos[1].name);
            _left_type->set_button_icon(SceneUtils::get_class_icon(type_infos[1].name));
            _left_type->set_tooltip_text("Set dictionary key type");
            _right_type->set_text(type_infos[2].name);
            _right_type->set_button_icon(SceneUtils::get_class_icon(type_infos[2].name));
            _right_type->set_tooltip_text("Set dictionary value type");
            _right_type->set_visible(true);
            break;
        }
    }
}

void OrchestratorEditorTypeSelector::set_property(const PropertyInfo& p_property) {
    _property = p_property;
    _normalize_inbound(_property);
    _update();
}

PropertyInfo OrchestratorEditorTypeSelector::get_property() const {
    PropertyInfo outbound = _property;
    _normalize_outbound(outbound);
    return outbound;
}

void OrchestratorEditorTypeSelector::set_read_only(bool p_read_only) {
    set_read_only(p_read_only, p_read_only);
    _container_type->set_disabled(p_read_only);
}

void OrchestratorEditorTypeSelector::set_read_only(bool p_left_read_only, bool p_right_read_only) {
    _left_type->set_disabled(p_left_read_only);
    _right_type->set_disabled(p_right_read_only);
}

void OrchestratorEditorTypeSelector::setup(const String& p_cache_suffix, bool p_allow_abstract_types, const PackedStringArray& p_exclusions) {
    _cache_name = p_cache_suffix;
    _allow_abstract_types = p_allow_abstract_types;
    _user_exclusions = p_exclusions;
}

void OrchestratorEditorTypeSelector::_bind_methods() {
    ADD_SIGNAL(MethodInfo("changed", PropertyInfo(Variant::DICTIONARY, "property")));
}

OrchestratorEditorTypeSelector::OrchestratorEditorTypeSelector() {

    for (int i = 0; i < Variant::VARIANT_MAX; i++) {
        const Variant::Type type = VariantUtils::to_type(i);
        _variant_named_types[Variant::get_type_name(type)] = type;
    }

    set_h_size_flags(SIZE_EXPAND_FILL);

    _left_type = memnew(Button);
    _left_type->set_clip_text(true);
    _left_type->add_theme_constant_override("icon_max_width", SceneUtils::get_editor_class_icon_size());
    _left_type->set_theme_type_variation("EditorInspectorButton");
    _left_type->set_icon_alignment(HORIZONTAL_ALIGNMENT_LEFT);
    _left_type->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
    _left_type->set_button_icon(SceneUtils::get_editor_icon("Variant"));
    _left_type->set_text("Variant");
    _left_type->set_h_size_flags(SIZE_EXPAND_FILL);
    _left_type->connect(SceneStringName(pressed), callable_mp_this(_left_type_pressed));
    add_child(_left_type);

    _container_type = memnew(OptionButton);
    _container_type->set_h_size_flags(SIZE_SHRINK_BEGIN);
    _container_type->set_tooltip_text("Set container type");
    _container_type->add_icon_item(SceneUtils::get_icon("ContainerNone"), "");
    _container_type->set_item_tooltip(0, "No container");
    _container_type->add_icon_item(SceneUtils::get_icon("ContainerArray"), "");
    _container_type->set_item_tooltip(1, "Array");
    _container_type->add_icon_item(SceneUtils::get_icon("ContainerDictionary"), "");
    _container_type->set_item_tooltip(2, "Dictionary");
    _container_type->connect(SceneStringName(item_selected), callable_mp_this(_container_type_changed));
    add_child(_container_type);

    _right_type = memnew(Button);
    _right_type->set_clip_text(true);
    _right_type->add_theme_constant_override("icon_max_width", SceneUtils::get_editor_class_icon_size());
    _right_type->set_theme_type_variation("EditorInspectorButton");
    _right_type->set_icon_alignment(HORIZONTAL_ALIGNMENT_LEFT);
    _right_type->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
    _right_type->set_button_icon(SceneUtils::get_editor_icon("Variant"));
    _right_type->set_text("Variant");
    _right_type->set_h_size_flags(SIZE_EXPAND_FILL);
    _right_type->set_tooltip_text("Set dictionary element type");
    _right_type->hide();
    _right_type->connect(SceneStringName(pressed), callable_mp_this(_right_type_pressed));
    add_child(_right_type);
}
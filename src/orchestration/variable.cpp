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
#include "orchestration/variable.h"

#include "api/extension_db.h"
#include "common/dictionary_utils.h"
#include "common/property_utils.h"
#include "common/string_utils.h"
#include "common/variant_utils.h"
#include "orchestration/annotation_registry.h"
#include "script/script_server.h"

class ClassificationParser {
protected:
    PropertyInfo _property;
    String _classification;
    bool _convert_default_value = false;

public:
    bool parse(const String& p_classification);
    PropertyInfo get_property() const { return _property; }
    bool is_default_value_converted() const { return _convert_default_value; }
};

bool ClassificationParser::parse(const String& p_classification) {
    _classification = p_classification;

    // Reset property state
    _property = PropertyInfo();
    _property.hint = PROPERTY_HINT_NONE;
    _property.usage = PROPERTY_USAGE_NO_EDITOR;

    // Make sure classification starts with expected prefixes
    if (!_classification.begins_with("type:")
        && !_classification.begins_with("enum:")
        && !_classification.begins_with("bitfield:")
        && !_classification.begins_with("class_enum:")
        && !_classification.begins_with("class_bitfield:")
        && !_classification.begins_with("class:")) {
        return false;
    }

    if (_classification.begins_with("type:")) {
        // basic types
        const String type_name = _classification.substr(_classification.find(":") + 1);
        for (int i = 0; i < Variant::VARIANT_MAX; i++) {
            const Variant::Type type = VariantUtils::to_type(i);
            if (Variant::get_type_name(type).match(type_name)) {
                if (_property.type != type) {
                    _convert_default_value = true;
                }

                _property.type = type;
                _property.hint = PROPERTY_HINT_NONE;
                _property.hint_string = "";
                _property.class_name = "";

                // These cannot have default values
                if (type == Variant::CALLABLE || type == Variant::SIGNAL || type == Variant::RID || type == Variant::NIL) {
                    _property.usage = PROPERTY_USAGE_STORAGE;
                } else {
                    _property.usage = PROPERTY_USAGE_DEFAULT;
                }

                if (type == Variant::NIL) {
                    _property.usage |= PROPERTY_USAGE_NIL_IS_VARIANT;
                }

                break;
            }
        }
    } else if (_classification.begins_with("enum:") || _classification.begins_with("bitfield:")) {
        // enum/bitfields
        const String name = _classification.substr(_classification.find(":") + 1);
        const EnumInfo& enum_info = ExtensionDB::get_global_enum(name);
        if (!enum_info.values.is_empty()) {
            PackedStringArray hints;
            for (const EnumValue& value : enum_info.values) {
                hints.push_back(value.name);
            }

            _property.type = Variant::INT;
            _property.hint = _classification.begins_with("bitfield:") ? PROPERTY_HINT_FLAGS : PROPERTY_HINT_ENUM;
            _property.hint_string = StringUtils::join(",", hints);
            _property.class_name = name;
            if (_classification.begins_with("bitfield:")) {
                _property.usage = PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_BITFIELD;
            } else {
                _property.usage = PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_ENUM;
            }
        }
    } else if (_classification.begins_with("class:")) {
        // class type
        const String class_name = _classification.substr(_classification.find(":") + 1);

        String native_class = class_name;
        if (ScriptServer::is_global_class(class_name)) {
            native_class = ScriptServer::get_global_class_native_base(class_name);
        }

        if (ClassDB::is_parent_class(native_class, "Resource")) {
            _property.type = Variant::OBJECT;
            _property.hint = PROPERTY_HINT_RESOURCE_TYPE;
            _property.hint_string = class_name;
            _property.class_name = class_name;
            _property.usage = PROPERTY_USAGE_DEFAULT;
        } else if (ClassDB::is_parent_class(native_class, "Node")) {
            _property.type = Variant::OBJECT;
            _property.hint = PROPERTY_HINT_NODE_TYPE;
            _property.hint_string = class_name;
            _property.class_name = class_name;
            _property.usage = PROPERTY_USAGE_DEFAULT;
        } else {
            _property.type = Variant::OBJECT;
            _property.hint = PROPERTY_HINT_NONE;
            _property.hint_string = "";
            _property.class_name = class_name;
            _property.usage = PROPERTY_USAGE_NO_EDITOR;
        }
    } else if (_classification.begins_with("class_enum:") || _classification.begins_with("class_bitfield:")) {
        const String class_enum_name = _classification.substr(_classification.find(":") + 1);
        const String class_name = class_enum_name.substr(0, class_enum_name.find("."));
        const String enum_name = class_enum_name.substr(class_enum_name.find(".") + 1);
        const String hint_string = StringUtils::join(",", ClassDB::class_get_enum_constants(class_name, enum_name, true));
        const bool bitfield = _classification.begins_with("class_bitfield:");

        _property.type = Variant::INT;
        _property.hint = bitfield ? PROPERTY_HINT_FLAGS : PROPERTY_HINT_ENUM;
        _property.hint_string = hint_string;
        _property.class_name = class_enum_name;
        _property.usage = PROPERTY_USAGE_DEFAULT | (bitfield ? PROPERTY_USAGE_CLASS_IS_BITFIELD : PROPERTY_USAGE_CLASS_IS_ENUM);
    } else if (_classification.begins_with("custom_enum:") || _classification.begins_with("custom_bitfield:")) {
        const bool bitfield = _classification.begins_with("custom_bitfield:");

        _property.type = Variant::INT;
        _property.hint = bitfield ? PROPERTY_HINT_FLAGS : PROPERTY_HINT_ENUM;
        _property.hint_string = "";
        _property.class_name = "";
        _property.usage = PROPERTY_USAGE_NO_EDITOR;
    }

    return true;
}

static bool parse_classification(const String& p_value, PropertyInfo& r_property, bool& r_convert) {
    ClassificationParser parser;
    if (!parser.parse(p_value)) {
        return false;
    }

    r_property = parser.get_property();
    r_convert = parser.is_default_value_converted();

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OScriptVariable

void OScriptVariable::_get_property_list(List<PropertyInfo>* r_properties) const {
    r_properties->push_back(PropertyInfo(Variant::STRING, "classification", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE));
    r_properties->push_back(PropertyInfo(Variant::INT, "type", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE));
}

bool OScriptVariable::_set(const StringName& p_name, const Variant& p_value) {
    if (p_name.match("classification")) {

        PropertyInfo property;
        bool converted;
        if (parse_classification(p_value, property, converted)) {
            if (converted && _initializer == INITIALIZER_LITERAL) {
                _convert_default_value(property.type);
            }
        }

        _info.type = property.type;
        _info.hint = property.hint;
        _info.hint_string = property.hint_string;
        _info.class_name = property.class_name;
        _info.usage = property.usage | PROPERTY_USAGE_SCRIPT_VARIABLE;

        _prune_annotations();
        _reset_initializer_if_needed();

        notify_property_list_changed();
        emit_changed();
        return true;

    } else if (p_name.match("type")) {
        Variant::Type value = VariantUtils::to_type(p_value);
        if (_info.type != value) {
            _info.type = value;

            _reset_initializer_if_needed();
            if (_initializer == INITIALIZER_LITERAL && _default_value.get_type() != _info.type) {
                set_default_value(VariantUtils::make_default(_info.type));
            }

            emit_changed();
            notify_property_list_changed();
        }
        return true;
    }
    return false;
}

void OScriptVariable::_validate_property(PropertyInfo& p_property) const {
    if (p_property.name.match("initializer")) {
        if (!is_node_path_initializer_allowed()) {
            p_property.usage &= ~PROPERTY_USAGE_EDITOR;
        }
        return;
    }

    if (p_property.name.match("default_value")) {
        if (_initializer == INITIALIZER_NODE_PATH) {
            // Picked relative to the edited scene root, matching the scene node graph node.
            p_property.type = Variant::NODE_PATH;
            p_property.hint = PROPERTY_HINT_NODE_PATH_VALID_TYPES;
            p_property.hint_string = ClassDB::class_exists(_info.class_name) ? String(_info.class_name) : String();
            p_property.class_name = StringName();
            p_property.usage = PROPERTY_USAGE_DEFAULT;
            return;
        }

        PropertyUtils::shape_default_value_property(p_property, _info);
    } else if (p_property.name.match("exported")) {
        if (is_exportable()) {
            p_property.usage &= ~PROPERTY_USAGE_READ_ONLY;
        } else {
            p_property.usage |= PROPERTY_USAGE_READ_ONLY;
        }
    }
}

bool OScriptVariable::_property_can_revert(const StringName& p_name) const {
    static Array properties = Array::make("name", "category", "exported", "default_value", "description", "constant", "info", "annotations", "initializer");
    return properties.has(p_name);
}

bool OScriptVariable::_property_get_revert(const StringName& p_name, Variant& r_property) {
    if (p_name.match("name")) {
        r_property = _info.name;
        return true;
    } else if (p_name.match("category")) {
        r_property = "Default";
        return true;
    } else if (p_name.match("exported")) {
        r_property = false;
        return true;
    } else if (p_name.match("default_value")) {
        r_property = VariantUtils::make_default(_info.type);
        return true;
    } else if (p_name.match("description")) {
        r_property = "";
        return true;
    } else if (p_name.match("constant")) {
        r_property = false;
        return true;
    } else if (p_name.match("info")) {
        r_property = DictionaryUtils::from_property(PropertyUtils::make_variant(_info.name), true);
        return true;
    } else if (p_name.match("annotations")) {
        r_property = Array();
        return true;
    } else if (p_name.match("initializer")) {
        r_property = INITIALIZER_LITERAL;
        return true;
    }
    return false;
}

void OScriptVariable::_set_property_info(const Dictionary& p_property) {
    set_info(DictionaryUtils::to_property(p_property));
}

Dictionary OScriptVariable::_get_property_info() const {
    return DictionaryUtils::from_property(_info, true);
}

void OScriptVariable::_set_annotations_array(const Array& p_annotations) {
    OScriptAnnotationList annotations;
    annotations.from_array(p_annotations);
    set_annotations(annotations.get_items());
}

Array OScriptVariable::_get_annotations_array() const {
    return _annotations.to_array();
}

void OScriptVariable::_set_initializer(int p_initializer) {
    set_initializer_kind(p_initializer == INITIALIZER_NODE_PATH ? INITIALIZER_NODE_PATH : INITIALIZER_LITERAL);
}

int OScriptVariable::_get_initializer() const {
    return _initializer;
}

void OScriptVariable::_reset_initializer_if_needed() {
    if (_initializer == INITIALIZER_NODE_PATH && !is_node_path_initializer_allowed()) {
        _initializer = INITIALIZER_LITERAL;
        _default_value = VariantUtils::make_default(_info.type);
    }
}

bool OScriptVariable::_convert_default_value(Variant::Type p_new_type) {
    PropertyInfo info = _info;
    info.type = p_new_type;

    set_default_value(PropertyUtils::make_declared_default_value(info, get_default_value()));
    return true;
}

bool OScriptVariable::_prune_annotations() {
    return _annotations.prune(OScriptAnnotationRegistry::TARGET_VARIABLE, _info) > 0;
}

Orchestration* OScriptVariable::get_orchestration() const {
    return _orchestration;
}

const PropertyInfo& OScriptVariable::get_info() const {
    return _info;
}

void OScriptVariable::set_info(const PropertyInfo& p_property) {
    _info.type = p_property.type;
    _info.class_name = p_property.class_name;
    _info.hint = p_property.hint;
    _info.hint_string = p_property.hint_string;
    _info.usage = p_property.usage;

    _reset_initializer_if_needed();
    if (_initializer == INITIALIZER_LITERAL) {
        _convert_default_value(_info.type);
    }
    _prune_annotations();

    notify_property_list_changed();
    emit_changed();
}

PropertyInfo OScriptVariable::get_export_info() const {
    return get_info();
}

void OScriptVariable::set_variable_name(const String& p_name) {
    if (!_info.name.match(p_name)) {
        _info.name = p_name;
        emit_changed();
    }
}

bool OScriptVariable::is_grouped_by_category() const {
    return !_category.is_empty() && !_category.to_lower().match("default") && !_category.to_lower().match("none");
}

void OScriptVariable::set_category(const String& p_category) {
    if (!_category.match(p_category)) {
        _category = p_category;
        emit_changed();
    }
}

String OScriptVariable::get_variable_type_name() const {
    return PropertyUtils::get_property_type_name(_info);
}

void OScriptVariable::set_description(const String& p_description) {
    if (_description != p_description) {
        _description = p_description;
        emit_changed();
    }
}

bool OScriptVariable::is_exported() const {
    return _annotations.has_family(OScriptAnnotationRegistry::FAMILY_EXPORT);
}

void OScriptVariable::set_exported(bool p_exported) {
    if (p_exported) {
        if (!is_exported()) {
            add_annotation(OScriptAnnotation("@export"));
        }
    } else if (_annotations.remove_family(OScriptAnnotationRegistry::FAMILY_EXPORT) > 0) {
        emit_changed();
    }
}

bool OScriptVariable::is_exportable() const {
    // Constants cannot be exported
    if (_constant) {
        return false;
    }

    // Nor can a variable carrying an annotation the export family conflicts with, such as @onready
    if (!OScriptAnnotationRegistry::find_conflict("@export", _annotations.get_items()).is_empty()) {
        return false;
    }

    switch (_info.type) {
        // These are all not exportable
        case Variant::CALLABLE:
        case Variant::SIGNAL:
        case Variant::RID:
            return false;

            // Object has specific circumstances depending on hint string
        case Variant::OBJECT: {
            if (!_info.class_name.is_empty()) {
                if (ScriptServer::is_global_class(_info.class_name)) {
                    const String native_class = ScriptServer::get_global_class_native_base(_info.class_name);
                    return ClassDB::is_parent_class(native_class, "Node")
                        || ClassDB::is_parent_class(native_class, "Resource");
                }

                return ClassDB::is_parent_class(_info.class_name, "Node")
                    || ClassDB::is_parent_class(_info.class_name, "Resource");
            }

            if (_info.hint_string.is_empty()) {
                return false;
            }

            if (!ClassDB::is_parent_class(_info.hint_string, "Node")
                    && !ClassDB::is_parent_class(_info.hint_string, "Resource")) {
                return false;
                    }

            break;
        }

        default:
            break;
    }
    return true;
}

void OScriptVariable::set_default_value(const Variant& p_default_value) {
    if (_default_value != p_default_value) {
        _default_value = p_default_value;
        emit_changed();
    }
}

void OScriptVariable::set_initializer_kind(InitializerKind p_initializer) {
    if (_initializer == p_initializer) {
        return;
    }

    if (p_initializer == INITIALIZER_NODE_PATH && !is_node_path_initializer_allowed()) {
        return;
    }

    _initializer = p_initializer;
    if (_initializer == INITIALIZER_NODE_PATH) {
        if (_default_value.get_type() != Variant::NODE_PATH) {
            _default_value = NodePath();
        }
    } else {
        _default_value = VariantUtils::make_default(_info.type);
    }

    notify_property_list_changed();
    emit_changed();
}

bool OScriptVariable::is_node_path_initializer_allowed() const {
    if (_constant) {
        return false;
    }

    if (PropertyUtils::is_variant(_info)) {
        return true;
    }

    if (_info.type != Variant::OBJECT || _info.class_name.is_empty()) {
        return false;
    }

    StringName native_class = _info.class_name;
    if (ScriptServer::is_global_class(native_class)) {
        native_class = ScriptServer::get_global_class_native_base(native_class);
    }
    return ClassDB::is_parent_class(native_class, "Node");
}

void OScriptVariable::set_constant(bool p_constant) {
    if (_constant != p_constant) {
        _constant = p_constant;

        // Constants cannot be exported or resolved from the scene tree
        if (_constant) {
            _annotations.remove_family(OScriptAnnotationRegistry::FAMILY_EXPORT);
            _reset_initializer_if_needed();
        }

        notify_property_list_changed();
        emit_changed();
    }
}

Error OScriptVariable::add_annotation(const OScriptAnnotation& p_annotation, String* r_reason) {
    const Error result = _annotations.add(OScriptAnnotationRegistry::TARGET_VARIABLE, _info, p_annotation, r_reason);
    if (result == OK) {
        emit_changed();
    }
    return result;
}

void OScriptVariable::remove_annotation(int p_index) {
    if (_annotations.remove_at(p_index)) {
        emit_changed();
    }
}

void OScriptVariable::set_annotation_arguments(int p_index, const Array& p_arguments) {
    if (_annotations.set_arguments(p_index, p_arguments)) {
        emit_changed();
    }
}

void OScriptVariable::set_annotations(const Vector<OScriptAnnotation>& p_annotations) {
    // Whole-list replacement bypasses the cardinality check so a snapshot restores verbatim;
    // the parser reports any conflict at compile time.
    OScriptAnnotationList replacement;
    replacement.set_items(p_annotations);

    if (_annotations != replacement) {
        _annotations = replacement;
        emit_changed();
    }
}

void OScriptVariable::copy_persistent_state(const Ref<OScriptVariable>& p_other) {
    if (p_other.is_valid()) {
        _category = p_other->_category;
        _constant = p_other->_constant;
        _default_value = p_other->_default_value;
        _description = p_other->_description;
        _annotations = p_other->_annotations;
        _initializer = p_other->_initializer;

        set_info(p_other->_info);
    }
}

String OScriptVariable::decode_property(const String& p_value) {
    if (p_value.begins_with("{")) {
        return p_value;
    }

    PropertyInfo property;
    bool converted = false;
    if (!parse_classification(p_value, property, converted)) {
        return "";
    }

    return UtilityFunctions::var_to_str(DictionaryUtils::from_property(property)).replace("\n", " ");
}

void OScriptVariable::_bind_methods() {
    // This is read-only to avoid name changes in the inspector, which creates cache issues with the owning script
    ClassDB::bind_method(D_METHOD("set_variable_name", "name"), &OScriptVariable::set_variable_name);
    ClassDB::bind_method(D_METHOD("get_variable_name"), &OScriptVariable::get_variable_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY), "set_variable_name", "get_variable_name");

    ClassDB::bind_method(D_METHOD("set_category", "category"), &OScriptVariable::set_category);
    ClassDB::bind_method(D_METHOD("get_category"), &OScriptVariable::get_category);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "category"), "set_category", "get_category");

    ClassDB::bind_method(D_METHOD("set_constant", "constant"), &OScriptVariable::set_constant);
    ClassDB::bind_method(D_METHOD("is_constant"), &OScriptVariable::is_constant);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "constant"), "set_constant", "is_constant");

    // Derived from the annotation list and not stored since format 5; the setter still migrates the
    // flag written by older formats into a plain "@export".
    ClassDB::bind_method(D_METHOD("set_exported", "exported"), &OScriptVariable::set_exported);
    ClassDB::bind_method(D_METHOD("is_exported"), &OScriptVariable::is_exported);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "exported", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR), "set_exported", "is_exported");

    ClassDB::bind_method(D_METHOD("set_property_info", "property"), &OScriptVariable::_set_property_info);
    ClassDB::bind_method(D_METHOD("get_property_info"), &OScriptVariable::_get_property_info);
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "info"), "set_property_info", "get_property_info");

    // Bound before "default_value" so a stored NodePath loads under the right initializer kind.
    ClassDB::bind_method(D_METHOD("set_initializer", "initializer"), &OScriptVariable::_set_initializer);
    ClassDB::bind_method(D_METHOD("get_initializer"), &OScriptVariable::_get_initializer);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "initializer", PROPERTY_HINT_ENUM, "Literal,Node Path"), "set_initializer", "get_initializer");

    ClassDB::bind_method(D_METHOD("set_default_value", "value"), &OScriptVariable::set_default_value);
    ClassDB::bind_method(D_METHOD("get_default_value"), &OScriptVariable::get_default_value);
    ADD_PROPERTY(PropertyInfo(Variant::NIL, "default_value", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "set_default_value", "get_default_value");

    ClassDB::bind_method(D_METHOD("set_description", "description"), &OScriptVariable::set_description);
    ClassDB::bind_method(D_METHOD("get_description"), &OScriptVariable::get_description);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "description", PROPERTY_HINT_MULTILINE_TEXT), "set_description", "get_description");

    // Bound after "exported" so a stored list replaces the plain "@export" an older file's flag applies on load.
    ClassDB::bind_method(D_METHOD("set_annotations", "annotations"), &OScriptVariable::_set_annotations_array);
    ClassDB::bind_method(D_METHOD("get_annotations"), &OScriptVariable::_get_annotations_array);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "annotations"), "set_annotations", "get_annotations");
}

OScriptVariable::OScriptVariable() {
    _info.type = Variant::NIL;
}
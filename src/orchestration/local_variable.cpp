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
#include "orchestration/local_variable.h"

#include "common/dictionary_utils.h"
#include "common/property_utils.h"
#include "common/variant_utils.h"
#include "orchestration/function.h"
#include "orchestration/orchestration.h"

void OScriptLocalVariable::_set_property_info(const Dictionary& p_property) {
    set_info(DictionaryUtils::to_property(p_property));
}

Dictionary OScriptLocalVariable::_get_property_info() const {
    return DictionaryUtils::from_property(_info, true);
}

void OScriptLocalVariable::_mark_edited() {
    if (_function && _function->get_orchestration()) {
        _function->get_orchestration()->set_edited(true);
    }
}

void OScriptLocalVariable::_validate_property(PropertyInfo& p_property) const {
    if (p_property.name == StringName("default_value")) {
        PropertyUtils::shape_default_value_property(p_property, _info);
    }
}

bool OScriptLocalVariable::_property_can_revert(const StringName& p_name) const {
    return p_name == StringName("name")
        || p_name == StringName("default_value")
        || p_name == StringName("description")
        || p_name == StringName("info");
}

bool OScriptLocalVariable::_property_get_revert(const StringName& p_name, Variant& r_property) {
    if (p_name == StringName("name")) {
        r_property = _info.name;
        return true;
    } else if (p_name == StringName("default_value")) {
        r_property = VariantUtils::make_default(_info.type);
        return true;
    } else if (p_name == StringName("description")) {
        r_property = "";
        return true;
    } else if (p_name == StringName("info")) {
        r_property = DictionaryUtils::from_property(PropertyUtils::make_variant(_info.name), true);
        return true;
    }
    return false;
}

OScriptFunction* OScriptLocalVariable::get_function() const {
    return _function;
}

const PropertyInfo& OScriptLocalVariable::get_info() const {
    return _info;
}

void OScriptLocalVariable::set_info(const PropertyInfo& p_property) {
    _info.type = p_property.type;
    _info.class_name = p_property.class_name;
    _info.hint = p_property.hint;
    _info.hint_string = p_property.hint_string;
    _info.usage = p_property.usage;

    // The name is the declaration's address within the function, never part of the type
    _default_value = PropertyUtils::make_declared_default_value(_info, _default_value);

    notify_property_list_changed();
    emit_changed();
    _mark_edited();
}

void OScriptLocalVariable::set_variable_name(const String& p_name) {
    if (_info.name != p_name) {
        _info.name = p_name;
        emit_changed();
        _mark_edited();
    }
}

String OScriptLocalVariable::get_variable_type_name() const {
    return PropertyUtils::get_property_type_name(_info);
}

void OScriptLocalVariable::set_default_value(const Variant& p_default_value) {
    if (_default_value != p_default_value) {
        _default_value = p_default_value;
        emit_changed();
        _mark_edited();

        // This is required so that variable value type is refreshed in inspector
        notify_property_list_changed();
    }
}

void OScriptLocalVariable::set_description(const String& p_description) {
    if (_description != p_description) {
        _description = p_description;
        emit_changed();
        _mark_edited();
    }
}

void OScriptLocalVariable::copy_persistent_state(const Ref<OScriptLocalVariable>& p_other) {
    if (p_other.is_valid()) {
        _default_value = p_other->_default_value;
        _description = p_other->_description;

        set_info(p_other->_info);
    }
}

void OScriptLocalVariable::_bind_methods() {
    // This is read-only to avoid name changes in the inspector; renames go through the owning function
    ClassDB::bind_method(D_METHOD("set_variable_name", "name"), &OScriptLocalVariable::set_variable_name);
    ClassDB::bind_method(D_METHOD("get_variable_name"), &OScriptLocalVariable::get_variable_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY), "set_variable_name", "get_variable_name");

    ClassDB::bind_method(D_METHOD("set_property_info", "property"), &OScriptLocalVariable::_set_property_info);
    ClassDB::bind_method(D_METHOD("get_property_info"), &OScriptLocalVariable::_get_property_info);
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "info"), "set_property_info", "get_property_info");

    ClassDB::bind_method(D_METHOD("set_default_value", "value"), &OScriptLocalVariable::set_default_value);
    ClassDB::bind_method(D_METHOD("get_default_value"), &OScriptLocalVariable::get_default_value);
    ADD_PROPERTY(PropertyInfo(Variant::NIL, "default_value", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "set_default_value", "get_default_value");

    ClassDB::bind_method(D_METHOD("set_description", "description"), &OScriptLocalVariable::set_description);
    ClassDB::bind_method(D_METHOD("get_description"), &OScriptLocalVariable::get_description);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "description", PROPERTY_HINT_MULTILINE_TEXT), "set_description", "get_description");
}

OScriptLocalVariable::OScriptLocalVariable() {
    _info.type = Variant::NIL;
    _info.usage = PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_NIL_IS_VARIANT;
}
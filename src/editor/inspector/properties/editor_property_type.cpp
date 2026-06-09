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
#include "editor/inspector/properties/editor_property_type.h"

#include "common/dictionary_utils.h"
#include "common/macros.h"
#include "common/property_utils.h"
#include "core/godot/core_string_names.h"
#include "editor/inspector/properties/type_selector.h"

void OrchestratorEditorPropertyType::_selector_type_changed(const Dictionary& p_property) {
    emit_changed(get_edited_property(), p_property);
}

void OrchestratorEditorPropertyType::_update_property() {
    ERR_FAIL_NULL(get_edited_object());

    Variant value = get_edited_object()->get(get_edited_property());
    if (value.get_type() != Variant::DICTIONARY) {
        value = Dictionary(PropertyUtils::make_variant(get_edited_property()));
    }

    _selector->set_property(DictionaryUtils::to_property(value));

    if (_provider) {
        _selector->set_read_only(_provider->is_key_locked(), _provider->is_value_locked());
    }
}

void OrchestratorEditorPropertyType::_set_read_only(bool p_read_only) {
    _selector->set_read_only(p_read_only);
}

void OrchestratorEditorPropertyType::setup(const String& p_cache_suffix, bool p_allow_abstract_types, const PackedStringArray& p_exclusions) {
    _selector->setup(p_cache_suffix, p_allow_abstract_types, p_exclusions);
}

void OrchestratorEditorPropertyType::set_constraint_provider(std::unique_ptr<OrchestratorEditorTypeConstraintProvider> p_provider) {
    _provider = std::move(p_provider);
}

OrchestratorEditorPropertyType::OrchestratorEditorPropertyType() {
    _selector = memnew(OrchestratorEditorTypeSelector);
    _selector->connect(CoreStringName(changed), callable_mp_this(_selector_type_changed));
    add_child(_selector);
}
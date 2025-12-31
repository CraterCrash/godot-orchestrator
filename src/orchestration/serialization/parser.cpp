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
#include "orchestration/serialization/parser.h"

#include "editor/plugins/orchestrator_editor_plugin.h"

#include <godot_cpp/core/class_db.hpp>


void OrchestrationParser::_set_resource_property(Ref<Resource>& r_resource, const MissingResource* p_missing_resource, const StringName& p_name, const Variant& p_value, Dictionary& r_missing_properties) {
    Variant value = p_value;

    bool is_valid = true;
    if (value.get_type() == Variant::OBJECT && p_missing_resource != nullptr) {
        // If the property being set is missing a resource (and the parent is not), then setting
        // it will likely not work, so this saves it as metadata instead.
        Ref<MissingResource> mr = value;
        if (mr.is_valid()) {
            r_missing_properties[p_name] = mr;
            is_valid = false;
        }
    }

    if (value.get_type() == Variant::ARRAY) {
        const Array set_array = value;
        const Variant get_value = r_resource->get(p_name);
        if (get_value.get_type() == Variant::ARRAY) {
            const Array& get_array = get_value;
            if (!set_array.is_same_typed(get_array)) {
                value = Array(set_array,
                    get_array.get_typed_builtin(),
                    get_array.get_typed_class_name(),
                    get_array.get_typed_script());
            }
        }
    }

    if (value.get_type() == Variant::DICTIONARY) {
        const Dictionary set_dict = value;
        const Variant get_value = r_resource->get(p_name);
        if (get_value.get_type() == Variant::DICTIONARY) {
            const Dictionary& get_dict = get_value;
            if (!set_dict.is_same_typed(get_dict)) {
                value = Dictionary(set_dict,
                    get_dict.get_typed_key_builtin(),
                    get_dict.get_typed_key_class_name(),
                    get_dict.get_typed_key_script(),
                    get_dict.get_typed_value_builtin(),
                    get_dict.get_typed_value_class_name(),
                    get_dict.get_typed_value_script());
            }
        }
    }

    if (is_valid) {
        r_resource->set(p_name, value);
    }
}

bool OrchestrationParser::_is_creating_missing_resources_if_class_unavailable_enabled() const {
    // EditorNode sets this to true, existence of our plugin should suffice.
    return OrchestratorPlugin::get_singleton() != nullptr;
}

Variant OrchestrationParser::_instantiate_resource(const String& p_resource_type) {
    // todo: centralized, we can override this behavior per serialized type
    return ClassDB::instantiate(p_resource_type);
}

void OrchestrationParser::_set_resource_edited(const Ref<Resource>& p_resource, bool p_edited) {
    #if TOOLS_ENABLED
    // todo: advocate for merging this upstream
    // p_resource->set_edited(p_edited);
    #endif
}

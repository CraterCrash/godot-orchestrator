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
#include "editor/inspector/orchestration_inspector_plugin.h"

#include "common/callable_lambda.h"
#include "editor/inspector/properties/editor_property_extends.h"
#include "orchestration/orchestration.h"

bool OrchestratorEditorInspectorPluginOrchestration::_can_handle(Object* p_object) const {
    return p_object && p_object->get_class() == Orchestration::get_class_static();
}

bool OrchestratorEditorInspectorPluginOrchestration::_parse_property(Object* p_object, Variant::Type p_type, const String& p_name,
    PropertyHint p_hint_type, const String& p_hint_string, BitField<PropertyUsageFlags> p_usage_flags, bool p_wide) {

    const Ref<Orchestration> orchestration = cast_to<Orchestration>(p_object);
    if (orchestration.is_valid() && p_name == "base_type") {
        OrchestratorEditorPropertyExtends* editor = memnew(OrchestratorEditorPropertyExtends);
        editor->setup(orchestration->get_base_type(), true);
        editor->connect("property_changed", callable_mp_lambda(
            this,
            [orchestration](const StringName& property, const Variant& value, const StringName& field, bool changing) {
                if (orchestration.is_valid()) {
                    orchestration->set_edited(true);
                }
            }));

        #if GODOT_VERSION >= 0x040300
        add_property_editor(p_name, editor, true, "Extends");
        #else
        add_property_editor(p_name, editor, true);
        #endif
        //return true;
    }

    return false;
}
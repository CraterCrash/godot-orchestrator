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
#include "editor/inspector/type_cast_inspector_plugin.h"

#include "editor/inspector/properties/editor_property_class_name.h"
#include "script/nodes/data/type_cast.h"

bool OrchestratorEditorInspectorPluginTypeCast::_can_handle(Object* p_object) const {
    return p_object && p_object->get_class() == OScriptNodeTypeCast::get_class_static();
}

bool OrchestratorEditorInspectorPluginTypeCast::_parse_property(Object* p_object, Variant::Type p_type, const String& p_name,
    PropertyHint p_hint_type, const String& p_hint_string, BitField<PropertyUsageFlags> p_usage_flags, bool p_wide) {

    if (cast_to<OScriptNodeTypeCast>(p_object)) {
        if (p_name.match("type")) {
            OrchestratorEditorPropertyClassName* editor = memnew(OrchestratorEditorPropertyClassName);
            editor->setup(p_hint_string, p_object->get("type"), true);
            add_property_editor(p_name, editor, true);
            return true;
        }
    }

    return false;
}
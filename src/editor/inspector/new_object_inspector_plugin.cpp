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
#include "editor/inspector/new_object_inspector_plugin.h"

#include "editor/inspector/properties/editor_property_class_name.h"
#include "orchestration/nodes/memory.h"

bool OrchestratorEditorInspectorPluginNewObjectNode::_can_handle(Object* p_object) const {
    return cast_to<OScriptNodeNew>(p_object) != nullptr;
}

bool OrchestratorEditorInspectorPluginNewObjectNode::_parse_property(Object* p_object, Variant::Type p_type, const String& p_name, PropertyHint p_hint_type, const String& p_hint_string, BitField<PropertyUsageFlags> p_usage_flags, bool p_wide) {
    const Ref<OScriptNodeNew> node = cast_to<OScriptNodeNew>(p_object);
    if (!node.is_valid()) {
        return false;
    }

    if (!p_name.match("class_name")) {
        return false;
    }

    String selected_type = node->get_allocated_class_name();
    if (selected_type.is_empty()) {
        selected_type = "Object";
    }

    OrchestratorEditorPropertyClassName* class_name = memnew(OrchestratorEditorPropertyClassName);
    class_name->setup("Object", selected_type, false);
    add_property_editor(p_name, class_name);
    return true;
}
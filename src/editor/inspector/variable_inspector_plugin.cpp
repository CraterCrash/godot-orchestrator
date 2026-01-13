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
#include "editor/inspector/variable_inspector_plugin.h"

#include "common/macros.h"
#include "editor/inspector/properties/editor_property_variable_classification.h"
#include "script/variable.h"

#include <godot_cpp/classes/editor_interface.hpp>

bool OrchestratorEditorInspectorPluginVariable::_can_handle(Object* p_object) const {
    return p_object->get_class() == OScriptVariable::get_class_static();
}

bool OrchestratorEditorInspectorPluginVariable::_parse_property(Object* p_object, Variant::Type p_type, const String& p_name,
    PropertyHint p_hint, const String& p_hint_string, BitField<PropertyUsageFlags> p_usage, bool p_wide) {

    const Ref<OScriptVariable> variable = cast_to<OScriptVariable>(p_object);
    if (variable.is_null()) {
        return false;
    }

    if (p_name.match("classification")) {
        OrchestratorEditorPropertyVariableClassification* editor = memnew(OrchestratorEditorPropertyVariableClassification);
        _classification = editor;
        #if GODOT_VERSION >= 0x040300
        add_property_editor(p_name, editor, true, "Variable Type");
        #else
        add_property_editor(p_name, editor, true);
        #endif
        return true;
    }

    return false;
}

void OrchestratorEditorInspectorPluginVariable::edit_classification(Object* p_object) {
    Ref<OScriptVariable> variable = cast_to<OScriptVariable>(p_object);
    if (variable.is_null()) {
        return;
    }

    // This is done to clear and reset the editor interface
    EI->edit_node(nullptr);
    EI->edit_resource(variable);

    _classification->edit();
}
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
#include "editor/graph/graph_pin_factory.h"

#include "common/property_utils.h"
#include "editor/graph/pins/value_editors.h"
#include "orchestration/nodes/call_function.h"
#include "orchestration/nodes/dialogue.h"

#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event.hpp>

static bool _is_input_action_pin(const Ref<OrchestrationGraphPin>& p_pin) {
    static PackedStringArray input_event_names = Array::make(
        "is_action_pressed", "is_action_released", "is_action", "get_action_strength");

    static PackedStringArray input_names = Array::make(
        "action_press", "action_release", "get_action_raw_strength", "get_action_strength",
        "is_action_just_pressed", "is_action_just_released", "is_action_pressed");

    const OScriptNodeCallMemberFunction* cmf =
        Object::cast_to<OScriptNodeCallMemberFunction>(p_pin->get_owning_node());

    if (cmf && p_pin->get_pin_name().match("action")) {
        const String target_class = cmf->get_target_class();
        const MethodInfo& method = cmf->get_function();

        if (InputEvent::get_class_static() == target_class && input_event_names.has(method.name)) {
            return true;
        }
        if (Input::get_class_static() == target_class && input_names.has(method.name)) {
            return true;
        }
    }

    return false;
}

OrchestratorEditorGraphPinValueEditor* OrchestratorEditorGraphPinFactory::create_value_editor(const Ref<OrchestrationGraphPin>& p_pin) {
    ERR_FAIL_COND_V(!p_pin.is_valid(), nullptr);

    if (PropertyUtils::is_variant(p_pin->get_property_info())) {
        return memnew(OrchestratorEditorGraphPinValueEditorAny);
    }

    if (p_pin->is_file()) {
        OrchestratorEditorGraphPinValueEditorFilePicker* fp = memnew(OrchestratorEditorGraphPinValueEditorFilePicker);
        fp->set_filters(Array::make(p_pin->get_file_types()));
        if (Object::cast_to<OScriptNodeDialogueMessage>(p_pin->get_owning_node())) {
            fp->set_default_text("Default Scene");
        }
        return fp;
    }

    if (p_pin->is_enum()) {
        return memnew(OrchestratorEditorGraphPinValueEditorEnum);
    }

    if (p_pin->is_bitfield()) {
        return memnew(OrchestratorEditorGraphPinValueEditorBitfield);
    }

    switch (p_pin->get_type()) {
        case Variant::BOOL:
            return memnew(OrchestratorEditorGraphPinValueEditorCheckbox);
        case Variant::STRING:
        case Variant::STRING_NAME: {
            if (_is_input_action_pin(p_pin)) {
                return memnew(OrchestratorEditorGraphPinValueEditorInputActionPicker);
            }
            if (p_pin->is_multiline_text()) {
                return memnew(OrchestratorEditorGraphPinValueEditorTextEdit);
            }
            return memnew(OrchestratorEditorGraphPinValueEditorLineEdit);
        }
        case Variant::COLOR:
            return memnew(OrchestratorEditorGraphPinValueEditorColorPicker);
        case Variant::INT:
        case Variant::FLOAT:
            return memnew(OrchestratorEditorGraphPinValueEditorNumber);
        case Variant::NODE_PATH:
            return memnew(OrchestratorEditorGraphPinValueEditorNodePath);
        case Variant::VECTOR2:
        case Variant::VECTOR2I:
        case Variant::VECTOR3:
        case Variant::VECTOR3I:
        case Variant::VECTOR4:
        case Variant::VECTOR4I:
        case Variant::RECT2:
        case Variant::RECT2I:
        case Variant::TRANSFORM2D:
        case Variant::TRANSFORM3D:
        case Variant::PLANE:
        case Variant::QUATERNION:
        case Variant::PROJECTION:
        case Variant::AABB:
        case Variant::BASIS:
            return memnew(OrchestratorEditorGraphPinValueEditorStruct);
        default:
            return nullptr;
    }
}

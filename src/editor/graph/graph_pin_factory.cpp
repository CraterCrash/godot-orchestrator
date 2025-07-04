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

#include "editor/graph/enum_resolver.h"
#include "editor/graph/pins/pins.h"
#include "script/nodes/dialogue/dialogue_message.h"
#include "script/nodes/functions/call_member_function.h"

#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event.hpp>

OrchestratorEditorGraphPin* OrchestratorEditorGraphPinFactory::_create_pin_widget_internal(const Ref<OrchestrationGraphPin>& p_pin)
{
    ERR_FAIL_COND_V(!p_pin.is_valid(), nullptr);

    // Handle special use cases.
    if (p_pin->is_execution())
        return memnew(OrchestratorEditorGraphPinExec);

    if (p_pin->is_file())
    {
        OrchestratorEditorGraphPinFilePicker* file_picker = memnew(OrchestratorEditorGraphPinFilePicker);
        file_picker->set_filters(Array::make(p_pin->get_file_types()));

        if (Object::cast_to<OScriptNodeDialogueMessage>(p_pin->get_owning_node()))
            file_picker->set_default_text("Default Scene");

        return file_picker;
    }

    if (p_pin->is_enum())
        return memnew(OrchestratorEditorGraphPinEnum);

    if (p_pin->is_bitfield())
        return memnew(OrchestratorEditorGraphPinBitfield);

    switch (p_pin->get_type())
    {
        case Variant::BOOL:
            return memnew(OrchestratorEditorGraphPinCheckbox);

        case Variant::STRING:
        case Variant::STRING_NAME:
        {
            if (is_input_action_pin(p_pin))
                return memnew(OrchestratorEditorGraphPinInputActionPicker);

            if (p_pin->is_multiline_text())
                return memnew(OrchestratorEditorGraphPinTextEdit);

            return memnew(OrchestratorEditorGraphPinLineEdit);
        }

        case Variant::COLOR:
            return memnew(OrchestratorEditorGraphPinColorPicker);

        case Variant::INT:
        case Variant::FLOAT:
            return memnew(OrchestratorEditorGraphPinNumber);

        case Variant::OBJECT:
            return memnew(OrchestratorEditorGraphPinObject);

        case Variant::NODE_PATH:
            return memnew(OrchestratorEditorGraphPinNodePath);

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
            return memnew(OrchestratorEditorGraphPinStruct);

        default:
            return memnew(OrchestratorEditorGraphPin);
    }
}

bool OrchestratorEditorGraphPinFactory::is_input_action_pin(const Ref<OrchestrationGraphPin>& p_pin)
{
    ERR_FAIL_COND_V(!p_pin.is_valid(), false);

    static PackedStringArray input_event_names = Array::make(
            "is_action_pressed", "is_action_released", "is_action", "get_action_strength");

    static PackedStringArray input_names = Array::make(
        "action_press", "action_release", "get_action_raw_strength", "get_action_strength",
        "is_action_just_pressed", "is_action_just_released", "is_action_pressed");

    const OScriptNodeCallMemberFunction* call_member_function_node =
        Object::cast_to<OScriptNodeCallMemberFunction>(p_pin->get_owning_node());

    if (call_member_function_node && p_pin->get_pin_name().match("action"))
    {
        const String target_class = call_member_function_node->get_target_class();
        const MethodInfo& method = call_member_function_node->get_function();

        if (InputEvent::get_class_static() == target_class && input_event_names.has(method.name))
            return true;

        if (Input::get_class_static() == target_class && input_names.has(method.name))
            return true;
    }

    return false;
}

OrchestratorEditorGraphPin* OrchestratorEditorGraphPinFactory::create_pin_widget(const Ref<OrchestrationGraphPin>& p_pin)
{
    ERR_FAIL_COND_V_MSG(!p_pin.is_valid(), nullptr, "Cannot create pin widget for an invalid pin model");

    OrchestratorEditorGraphPin* pin = _create_pin_widget_internal(p_pin);
    ERR_FAIL_NULL_V_MSG(pin, nullptr, "Failed to create pin widget");

    pin->set_pin(p_pin);

    return pin;
}

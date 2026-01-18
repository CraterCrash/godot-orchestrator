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
#include "editor/inspector/function_inspector_plugin.h"

#include "common/dictionary_utils.h"
#include "common/macros.h"
#include "editor/inspector/properties/editor_property_pin_properties.h"
#include "script/nodes/functions/function_entry.h"

void OrchestratorEditorInspectorPluginFunction::_move_up(int p_index, const Ref<OScriptFunction>& p_function) {
    _swap(p_index, 0, -1, p_function);
}

void OrchestratorEditorInspectorPluginFunction::_move_down(int p_index, const Ref<OScriptFunction>& p_function) {
    _swap(p_index, 2, 1, p_function);
}

void OrchestratorEditorInspectorPluginFunction::_swap(int p_index, int p_pin_offset, int p_argument_offset, const Ref<OScriptFunction>& p_function) {
    for (const Ref<OScriptGraph>& graph : p_function->get_orchestration()->get_graphs()) {
        for (const Ref<OScriptNode>& node : graph->get_nodes()) {
            Ref<OScriptNodeFunctionEntry> entry_node = node;
            if (entry_node.is_valid() && entry_node->get_function() == p_function) {
                // Offset by one because Emit Signal port 0 is the execution port
                Ref<OScriptNodePin> pin = entry_node->find_pin(p_index + 1, PD_Input);
                Ref<OScriptNodePin> other_pin = entry_node->find_pin(p_index + p_pin_offset, PD_Input);

                Vector<Ref<OScriptNodePin>> pin_sources = pin->get_connections();
                Vector<Ref<OScriptNodePin>> other_pin_sources = other_pin->get_connections();

                pin->unlink_all();
                other_pin->unlink_all();

                for (const Ref<OScriptNodePin>& pin_source : pin_sources) {
                    pin_source->link(other_pin);
                }

                for (const Ref<OScriptNodePin>& other_pin_source : other_pin_sources) {
                    other_pin_source->link(pin);
                }
            }
        }
    }

    MethodInfo method = p_function->get_method_info();
    PropertyInfo displaced = method.arguments[p_index + p_argument_offset];
    method.arguments[p_index + p_argument_offset] = method.arguments[p_index];
    method.arguments[p_index] = displaced;

    TypedArray<Dictionary> properties;
    for (const PropertyInfo& property : method.arguments) {
        properties.push_back(DictionaryUtils::from_property(property));
    }

    p_function->set("inputs", properties);
}

bool OrchestratorEditorInspectorPluginFunction::_can_handle(Object* p_object) const {
    return cast_to<OScriptFunction>(p_object) != nullptr;
}

bool OrchestratorEditorInspectorPluginFunction::_parse_property(Object* p_object, Variant::Type p_type, const String& p_name,
    PropertyHint p_hint_type, const String& p_hint_string, BitField<PropertyUsageFlags> p_usage_flags, bool p_wide)
{
    Ref<OScriptFunction> function = cast_to<OScriptFunction>(p_object);
    if (!function.is_valid()) {
        return false;
    }

    if (p_name.match("inputs")) {
        OrchestratorEditorPropertyPinProperties* inputs = memnew(OrchestratorEditorPropertyPinProperties);
        inputs->set_label("Inputs");
        inputs->set_allow_rearrange(false);
        inputs->setup(true);
        inputs->connect("move_up", callable_mp_this(_move_up).bind(function));
        inputs->connect("move_down", callable_mp_this(_move_down).bind(function));
        add_property_editor(p_name, inputs, true);
        return true;
    }

    if (p_name.match("outputs")) {
        OrchestratorEditorPropertyPinProperties* outputs = memnew(OrchestratorEditorPropertyPinProperties);
        outputs->set_label("Outputs");
        outputs->set_allow_rearrange(false);
        outputs->setup(false, 1);
        add_property_editor(p_name, outputs, true);
        return true;
    }

    return false;
}
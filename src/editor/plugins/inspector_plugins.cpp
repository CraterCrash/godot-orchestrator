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
#include "editor/plugins/inspector_plugins.h"

#include "common/dictionary_utils.h"
#include "common/version.h"
#include "editor/inspector/editor_property_class_name.h"
#include "editor/inspector/property_info_container_property.h"
#include "editor/inspector/property_type_button_property.h"
#include "orchestrator_editor_plugin.h"
#include "script/nodes/data/type_cast.h"
#include "script/nodes/functions/function_entry.h"
#include "script/nodes/functions/function_terminator.h"
#include "script/nodes/signals/emit_signal.h"

void OrchestratorEditorInspectorPluginFunction::_move_up(int p_index, const Ref<OScriptFunction>& p_function)
{
    _swap(p_index, 0, -1, p_function);
}

void OrchestratorEditorInspectorPluginFunction::_move_down(int p_index, const Ref<OScriptFunction>& p_function)
{
    _swap(p_index, 2, 1, p_function);
}

void OrchestratorEditorInspectorPluginFunction::_swap(int p_index, int p_pin_offset, int p_argument_offset, const Ref<OScriptFunction>& p_function)
{
    for (const Ref<OScriptGraph>& graph : p_function->get_orchestration()->get_graphs())
    {
        for (const Ref<OScriptNode>& node : graph->get_nodes())
        {
            Ref<OScriptNodeFunctionEntry> entry_node = node;
            if (entry_node.is_valid() && entry_node->get_function() == p_function)
            {
                // Offset by one because Emit Signal port 0 is the execution port
                Ref<OScriptNodePin> pin = entry_node->find_pin(p_index + 1, PD_Input);
                Ref<OScriptNodePin> other_pin = entry_node->find_pin(p_index + p_pin_offset, PD_Input);

                Vector<Ref<OScriptNodePin>> pin_sources = pin->get_connections();
                Vector<Ref<OScriptNodePin>> other_pin_sources = other_pin->get_connections();

                pin->unlink_all();
                other_pin->unlink_all();

                for (const Ref<OScriptNodePin>& pin_source : pin_sources)
                    pin_source->link(other_pin);

                for (const Ref<OScriptNodePin>& other_pin_source : other_pin_sources)
                    other_pin_source->link(pin);
            }
        }
    }

    MethodInfo method = p_function->get_method_info();
    PropertyInfo displaced = method.arguments[p_index + p_argument_offset];
    method.arguments[p_index + p_argument_offset] = method.arguments[p_index];
    method.arguments[p_index] = displaced;

    TypedArray<Dictionary> properties;
    for (const PropertyInfo& property : method.arguments)
        properties.push_back(DictionaryUtils::from_property(property));

    p_function->set("inputs", properties);
}

bool OrchestratorEditorInspectorPluginFunction::_can_handle(Object* p_object) const
{
    return Object::cast_to<OScriptFunction>(p_object) != nullptr;
}

bool OrchestratorEditorInspectorPluginFunction::_parse_property(Object* p_object, Variant::Type p_type, const String& p_name, PropertyHint p_hint_type, const String& p_hint_string, BitField<PropertyUsageFlags> p_usage_flags, bool p_wide)
{
    Ref<OScriptFunction> function = Object::cast_to<OScriptFunction>(p_object);
    if (!function.is_valid())
        return false;

    if (p_name.match("inputs"))
    {
        OrchestratorPropertyInfoContainerEditorProperty* inputs = memnew(OrchestratorPropertyInfoContainerEditorProperty);
        inputs->set_label("Inputs");
        inputs->set_allow_rearrange(false);
        inputs->setup(true);
        inputs->connect("move_up", callable_mp(this, &OrchestratorEditorInspectorPluginFunction::_move_up).bind(function));
        inputs->connect("move_down", callable_mp(this, &OrchestratorEditorInspectorPluginFunction::_move_down).bind(function));
        add_property_editor(p_name, inputs, true);
        return true;
    }

    if (p_name.match("outputs"))
    {
        OrchestratorPropertyInfoContainerEditorProperty* outputs = memnew(OrchestratorPropertyInfoContainerEditorProperty);
        outputs->set_label("Outputs");
        outputs->set_allow_rearrange(false);
        outputs->setup(false, 1);
        add_property_editor(p_name, outputs, true);
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OrchestratorEditorInspectorPluginSignal::_move_up(int p_index, const Ref<OScriptSignal>& p_signal)
{
    _swap(p_index, 0, -1, p_signal);
}

void OrchestratorEditorInspectorPluginSignal::_move_down(int p_index, const Ref<OScriptSignal>& p_signal)
{
    _swap(p_index, 2, 1, p_signal);
}

void OrchestratorEditorInspectorPluginSignal::_swap(int p_index, int p_pin_offset, int p_argument_offset, const Ref<OScriptSignal>& p_signal)
{
    for (const Ref<OScriptGraph>& graph : p_signal->get_orchestration()->get_graphs())
    {
        for (const Ref<OScriptNode>& node : graph->get_nodes())
        {
            Ref<OScriptNodeEmitSignal> signal_node = node;
            if (signal_node.is_valid() && signal_node->get_signal() == p_signal)
            {
                // Offset by one because Emit Signal port 0 is the execution port
                Ref<OScriptNodePin> pin = signal_node->find_pin(p_index + 1, PD_Input);
                Ref<OScriptNodePin> other_pin = signal_node->find_pin(p_index + p_pin_offset, PD_Input);

                Vector<Ref<OScriptNodePin>> pin_sources = pin->get_connections();
                Vector<Ref<OScriptNodePin>> other_pin_sources = other_pin->get_connections();

                pin->unlink_all();
                other_pin->unlink_all();

                for (const Ref<OScriptNodePin>& pin_source : pin_sources)
                    pin_source->link(other_pin);

                for (const Ref<OScriptNodePin>& other_pin_source : other_pin_sources)
                    other_pin_source->link(pin);
            }
        }
    }

    MethodInfo method = p_signal->get_method_info();
    PropertyInfo displaced = method.arguments[p_index + p_argument_offset];
    method.arguments[p_index + p_argument_offset] = method.arguments[p_index];
    method.arguments[p_index] = displaced;

    TypedArray<Dictionary> properties;
    for (const PropertyInfo& property : method.arguments)
        properties.push_back(DictionaryUtils::from_property(property));

    p_signal->set("inputs", properties);
}

bool OrchestratorEditorInspectorPluginSignal::_can_handle(Object* p_object) const
{
    return Object::cast_to<OScriptSignal>(p_object) != nullptr;
}

bool OrchestratorEditorInspectorPluginSignal::_parse_property(Object* p_object, Variant::Type p_type, const String& p_name, PropertyHint p_hint_type, const String& p_hint_string, BitField<PropertyUsageFlags> p_usage_flags, bool p_wide)
{
    Ref<OScriptSignal> signal = Object::cast_to<OScriptSignal>(p_object);
    if (!signal.is_valid())
        return false;

    if (p_name.match("inputs"))
    {
        OrchestratorPropertyInfoContainerEditorProperty* inputs = memnew(OrchestratorPropertyInfoContainerEditorProperty);
        inputs->set_label("Inputs");
        inputs->set_allow_rearrange(true);
        inputs->setup(true);
        inputs->connect("move_up", callable_mp(this, &OrchestratorEditorInspectorPluginSignal::_move_up).bind(signal));
        inputs->connect("move_down", callable_mp(this, &OrchestratorEditorInspectorPluginSignal::_move_down).bind(signal));
        add_property_editor(p_name, inputs, true);
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool OrchestratorEditorInspectorPluginVariable::_can_handle(Object* p_object) const
{
    return p_object->get_class() == OScriptVariable::get_class_static();
}

bool OrchestratorEditorInspectorPluginVariable::_parse_property(Object* p_object, Variant::Type p_type, const String& p_name, PropertyHint p_hint, const String& p_hint_string, BitField<PropertyUsageFlags> p_usage, bool p_wide)
{
    Ref<OScriptVariable> variable = Object::cast_to<OScriptVariable>(p_object);
    if (variable.is_null())
        return false;

    if (p_name.match("classification"))
    {
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

void OrchestratorEditorInspectorPluginVariable::edit_classification(Object* p_object)
{
    Ref<OScriptVariable> variable = Object::cast_to<OScriptVariable>(p_object);
    if (variable.is_null())
        return;

    // This is done to clear and reset the editor interface
    EditorInterface::get_singleton()->edit_node(nullptr);
    EditorInterface::get_singleton()->edit_resource(variable);

    _classification->edit();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool OrchestratorEditorInspectorPluginTypeCast::_can_handle(Object* p_object) const
{
    return p_object->get_class() == OScriptNodeTypeCast::get_class_static();
}

bool OrchestratorEditorInspectorPluginTypeCast::_parse_property(Object* p_object, Variant::Type p_type, const String& p_name, PropertyHint p_hint_type, const String& p_hint_string, BitField<PropertyUsageFlags> p_usage_flags, bool p_wide)
{
    if (Object::cast_to<OScriptNodeTypeCast>(p_object))
    {
        if (p_name.match("type"))
        {
            OrchestratorEditorPropertyClassName* editor = memnew(OrchestratorEditorPropertyClassName);
            editor->setup(p_hint_string, p_object->get("type"), true);
            add_property_editor(p_name, editor, true);
            return true;
        }
    }
    return false;
}


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
#include "function_terminator.h"

#include "common/dictionary_utils.h"
#include "common/macros.h"
#include "common/property_utils.h"
#include "common/variant_utils.h"

void OScriptNodeFunctionTerminator::_get_property_list(List<PropertyInfo>* r_list) const
{
    r_list->push_back(PropertyInfo(Variant::STRING, "function_id", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    r_list->push_back(PropertyInfo(Variant::STRING, "function_name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY | PROPERTY_USAGE_EDITOR));

    uint32_t usage = (!_is_inputs_outputs_mutable() ? PROPERTY_USAGE_READ_ONLY | PROPERTY_USAGE_EDITOR : PROPERTY_USAGE_EDITOR);
    r_list->push_back(PropertyInfo(Variant::STRING, "Inputs/Outputs", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_CATEGORY));
    r_list->push_back(PropertyInfo(Variant::DICTIONARY, "inputs", PROPERTY_HINT_NONE, "", usage));
    r_list->push_back(PropertyInfo(Variant::DICTIONARY, "outputs", PROPERTY_HINT_NONE, "", usage));
}

bool OScriptNodeFunctionTerminator::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("function_id"))
    {
        r_value = _guid.to_string();
        return true;
    }
    else if (p_name.match("function_name"))
    {
        Ref<OScriptFunction> function = get_function();
        r_value = function.is_valid() ? function->get_function_name() : "";
        return true;
    }
    else if (p_name.match("inputs"))
    {
        TypedArray<Dictionary> inputs;
        Ref<OScriptFunction> function = get_function();
        if (function.is_valid())
        {
            for (const PropertyInfo& property : function->get_method_info().arguments)
                inputs.push_back(DictionaryUtils::from_property(property));
        }
        r_value = inputs;
        return true;
    }
    else if (p_name.match("outputs"))
    {
        TypedArray<Dictionary> outputs;
        Ref<OScriptFunction> function = get_function();
        if (function.is_valid() && get_function()->has_return_type())
            outputs.push_back(DictionaryUtils::from_property(function->get_method_info().return_val));

        r_value = outputs;
        return true;
    }
    return false;
}

bool OScriptNodeFunctionTerminator::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("function_id"))
    {
        _guid = Guid(p_value);
        return true;
    }
    else if (p_name.match("inputs"))
    {
        Ref<OScriptFunction> function = get_function();
        if (function.is_valid())
        {
            TypedArray<Dictionary> value = p_value;
            const bool refresh_required = function->get_argument_count() != size_t(value.size());

            function->set_arguments(p_value);

            if (refresh_required)
                notify_property_list_changed();

            return true;
        }
    }
    else if (p_name.match("outputs"))
    {
        Ref<OScriptFunction> function = get_function();
        if (function.is_valid())
        {
            const TypedArray<Dictionary> value = p_value;
            if (value.is_empty())
                function->set_has_return_value(false);
            else
                function->set_return(DictionaryUtils::to_property(value[0]));
            return true;
        }
    }
    return false;
}

void OScriptNodeFunctionTerminator::_on_function_changed()
{
    reconstruct_node();
}

bool OScriptNodeFunctionTerminator::create_pins_for_function_entry_exit(const Ref<OScriptFunction>& p_function,
                                                                        bool p_function_entry)
{
    bool pins_good = true;
    if (p_function_entry)
    {
        for (const PropertyInfo& property : p_function->get_method_info().arguments)
        {
            if (find_pin(property.name).is_valid())
                continue;

            // The Godot framework does not permit output arguments on function calls and therefore those will
            // not be supported here. Additionally, this will also mean that only a single output pin will be
            // possible when creating return nodes.
            Ref<OScriptNodePin> pin = create_pin(PD_Output, PT_Data, property);
            pins_good = pin.is_valid() & pins_good;
        }
    }
    else
    {
        if (p_function->has_return_type())
        {
            const MethodInfo mi = p_function->get_method_info();
            Ref<OScriptNodePin> pin = create_pin(PD_Input, PT_Data, PropertyUtils::as("return_value", mi.return_val));
            if (!mi.return_val.name.is_empty())
                pin->set_label(mi.return_val.name);

            pins_good = pin.is_valid() & pins_good;

            // Create hidden output pin to transfer value to caller
            Ref<OScriptNodePin> out = create_pin(PD_Output, PT_Data, PropertyUtils::as("return_out", mi.return_val));
            out->set_flag(OScriptNodePin::Flags::HIDDEN);
            pins_good = out.is_valid() & pins_good;
        }
    }

    return pins_good;
}

void OScriptNodeFunctionTerminator::post_initialize()
{
    super::post_initialize();

    _function = get_orchestration()->find_function(_guid);
    if (_function.is_valid() && _is_in_editor())
        OCONNECT(_function, "changed", callable_mp(this, &OScriptNodeFunctionTerminator::_on_function_changed));

    // Always reconstruct entry/exit nodes
    reconstruct_node();
}

void OScriptNodeFunctionTerminator::post_placed_new_node()
{
    super::post_placed_new_node();

    if (_function.is_valid() && _is_in_editor())
        OCONNECT(_function, "changed", callable_mp(this, &OScriptNodeFunctionTerminator::_on_function_changed));
}

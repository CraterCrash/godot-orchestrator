// This file is part of the Godot Orchestrator project.
//
// Copyright (c) 2023-present Vahera Studios LLC and its contributors.
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

#include "common/variant_utils.h"

void OScriptNodeFunctionTerminator::_get_property_list(List<PropertyInfo>* r_list) const
{
    Ref<OScriptFunction> function = get_function();

    // Setup flags
    int32_t read_only_serialize = PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY;
    int32_t read_only_editor    = PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_READ_ONLY;

    int32_t usage = read_only_editor;
    if (function.is_valid() && function->is_user_defined())
        usage = PROPERTY_USAGE_EDITOR;

    r_list->push_back(PropertyInfo(Variant::STRING, "function_id", PROPERTY_HINT_NONE, "", read_only_serialize));
    r_list->push_back(PropertyInfo(Variant::STRING, "function_name", PROPERTY_HINT_NONE, "", read_only_editor));

    r_list->push_back(PropertyInfo(Variant::STRING, "description", PROPERTY_HINT_MULTILINE_TEXT));

    if (function.is_valid())
    {
        if (_supports_return_values())
        {
            r_list->push_back(PropertyInfo(Variant::BOOL, "has_return_value", PROPERTY_HINT_NONE, "", usage));
            if (function->has_return_type())
            {
                static String return_types = VariantUtils::to_enum_list();
                r_list->push_back(PropertyInfo(Variant::INT, "return_type", PROPERTY_HINT_ENUM, return_types));
            }
        }

        r_list->push_back(PropertyInfo(Variant::INT, "argument_count", PROPERTY_HINT_RANGE, "0,32", usage));

        static String types = VariantUtils::to_enum_list();
        const MethodInfo& mi = function->get_method_info();
        for (size_t i = 1; i <= mi.arguments.size(); i++)
        {
            r_list->push_back(PropertyInfo(Variant::INT, "argument_" + itos(i) + "/type", PROPERTY_HINT_ENUM, types, usage));
            r_list->push_back(PropertyInfo(Variant::STRING, "argument_" + itos(i) + "/name", PROPERTY_HINT_NONE, "", usage));
        }
    }
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
    else if (p_name.match("argument_count"))
    {
        Ref<OScriptFunction> function = get_function();
        r_value = function.is_valid() ? static_cast<int64_t>(function->get_argument_count()) : 0;
        return true;
    }
    else if (p_name.begins_with("argument_"))
    {
        Ref<OScriptFunction> function = get_function();
        if (function.is_valid())
        {
            const MethodInfo &mi = function->get_method_info();

            const size_t index = p_name.get_slicec('_', 1).get_slicec('/', 0).to_int() - 1;
            ERR_FAIL_INDEX_V(index, mi.arguments.size(), false);

            const String what = p_name.get_slicec('/', 1);
            if (what == "type")
            {
                r_value = mi.arguments[index].type;
                return true;
            }
            else if (what == "name")
            {
                r_value = mi.arguments[index].name;
                return true;
            }
        }
    }
    else if (_supports_return_values() && p_name.match("has_return_value"))
    {
        r_value = _return_value;
        return true;
    }
    else if (_supports_return_values() && p_name.match("return_type"))
    {
        Ref<OScriptFunction> function = get_function();
        if (function.is_valid())
        {
            r_value = function->get_return_type();
            return true;
        }
    }
    else if (p_name.match("description"))
    {
        Ref<OScriptFunction> function = get_function();
        if (function.is_valid())
        {
            r_value = function->get_description();
            return true;
        }
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
    else if (p_name.match("argument_count"))
    {
        Ref<OScriptFunction> function = get_function();
        if (function.is_valid())
        {
            if (function->resize_argument_list(static_cast<int64_t>(p_value)))
                notify_property_list_changed();
            return true;
        }
    }
    else if (p_name.begins_with("argument_"))
    {
        Ref<OScriptFunction> function = get_function();
        if (function.is_valid())
        {
            const MethodInfo &mi = function->get_method_info();

            const size_t index = p_name.get_slicec('_', 1).get_slicec('/', 0).to_int() - 1;
            ERR_FAIL_INDEX_V(index, mi.arguments.size(), false);

            const String what = p_name.get_slicec('/', 1);
            if (what == "type")
            {
                function->set_argument_type(index, VariantUtils::to_type(p_value));
                return true;
            }
            else if (what == "name")
            {
                function->set_argument_name(index, p_value);
                return true;
            }
        }
    }
    else if (_supports_return_values() && p_name.match("has_return_value"))
    {
        Ref<OScriptFunction> function = get_function();
        if (function.is_valid() && function->is_user_defined())
        {
            _return_value = p_value;
            function->set_has_return_value(_return_value);
            notify_property_list_changed();
            return true;
        }
    }
    else if (_supports_return_values() && p_name.match("return_type"))
    {
        Ref<OScriptFunction> function = get_function();
        if (function.is_valid() && function->is_user_defined())
        {
            function->set_return_type(VariantUtils::to_type(p_value));
            return true;
        }
    }
    else if (p_name.match("description"))
    {
        Ref<OScriptFunction> function = get_function();
        if (function.is_valid())
        {
            function->set_description(p_value);
            return true;
        }
    }
    return false;
}

void OScriptNodeFunctionTerminator::_validate_property(PropertyInfo& p_property) const
{
    if (p_property.name.match("return_type"))
    {
        Ref<OScriptFunction> function = get_function();
        if (function.is_valid())
            p_property.type = function->get_return_type();
    }
}

void OScriptNodeFunctionTerminator::_on_function_changed()
{
    if (_function.is_valid() && _supports_return_values())
        _return_value = _function->has_return_type();

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
            Ref<OScriptNodePin> pin = create_pin(PD_Output, property.name, property.type);
            pin->set_flags(OScriptNodePin::Flags::DATA | OScriptNodePin::Flags::NO_CAPITALIZE);
            pins_good = pin.is_valid() & pins_good;
        }
    }
    else
    {
        if (p_function->has_return_type())
        {
            Ref<OScriptNodePin> pin = create_pin(PD_Input, "return_value", p_function->get_return_type());
            pins_good = pin.is_valid() & pins_good;

            // Create hidden output pin to transfer value to caller
            Ref<OScriptNodePin> out = create_pin(PD_Output, "return_out", p_function->get_return_type());
            out->set_flags(OScriptNodePin::Flags::DATA | OScriptNodePin::Flags::HIDDEN);
            pins_good = out.is_valid() & pins_good;
        }
    }

    return pins_good;
}

void OScriptNodeFunctionTerminator::post_initialize()
{
    super::post_initialize();

    _function = get_orchestration()->find_function(_guid);
    if (_function.is_valid())
    {
        if (_is_in_editor())
            _function->connect("changed", callable_mp(this, &OScriptNodeFunctionTerminator::_on_function_changed));
        _return_value = _function->has_return_type();
    }
}

void OScriptNodeFunctionTerminator::post_placed_new_node()
{
    super::post_placed_new_node();

    if (_function.is_valid())
    {
        if (_is_in_editor())
            _function->connect("changed", callable_mp(this, &OScriptNodeFunctionTerminator::_on_function_changed));
        _return_value = _function->has_return_type();
    }
}

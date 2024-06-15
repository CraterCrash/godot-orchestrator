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
#include "call_member_function.h"

#include "common/dictionary_utils.h"
#include "common/variant_utils.h"

OScriptNodeCallMemberFunction::OScriptNodeCallMemberFunction()
{
    _flags = ScriptNodeFlags::CATALOGABLE;
}

void OScriptNodeCallMemberFunction::post_initialize()
{
    _reference.name = _reference.method.name;
    _reference.return_type = _reference.method.return_val.type;
    _function_flags.set_flag(FF_IS_SELF);

    super::post_initialize();
}

String OScriptNodeCallMemberFunction::get_tooltip_text() const
{
    if (!_reference.method.name.is_empty())
        return vformat("Calls the function '%s'", _reference.method.name);

    return "Calls the specified function";
}

String OScriptNodeCallMemberFunction::get_node_title() const
{
    if (!_reference.method.name.is_empty())
        return vformat("%s", _reference.method.name.capitalize());

    return super::get_node_title();
}

void OScriptNodeCallMemberFunction::initialize(const OScriptNodeInitContext& p_context)
{
    MethodInfo mi;
    StringName target_class = get_orchestration()->get_base_type();
    Variant::Type target_type = Variant::NIL;
    if (p_context.user_data)
    {
        const Dictionary data = p_context.user_data.value();
        if (data.has("target_type") && data.has("method"))
        {
            // In this case there is no target class
            target_class = "";
            target_type = VariantUtils::to_type(data["target_type"]);
            mi = DictionaryUtils::to_method(data["method"]);
        }
        else
            mi = DictionaryUtils::to_method(data);
    }
    else if (p_context.method)
        mi = p_context.method.value();

    ERR_FAIL_COND_MSG(mi.name.is_empty(), "Failed to initialize CallMemberFunction without a MethodInfo");

    _reference.method = mi;
    _reference.target_type = target_type;
    _reference.target_class_name = target_class;
    _reference.name = _reference.method.name;
    _reference.return_type = _reference.method.return_val.type;

    _set_function_flags(_reference.method);

    // Default to self, but if the target pin has a connection, it will override this.
    _function_flags.set_flag(FF_IS_SELF);

    super::initialize(p_context);
}

void OScriptNodeCallMemberFunction::on_pin_connected(const Ref<OScriptNodePin>& p_pin)
{
    if (p_pin->get_pin_name().match("target"))
    {
        _function_flags = int64_t(_function_flags) & ~FF_IS_SELF;

        const Ref<OScriptNodePin> supplier = p_pin->get_connections()[0];
        const String target_class = supplier->get_owning_node()->resolve_type_class(supplier);
        _reference.target_class_name = target_class;
    }
}

void OScriptNodeCallMemberFunction::on_pin_disconnected(const Ref<OScriptNodePin>& p_pin)
{
    if (p_pin->get_pin_name().match("target"))
    {
        _function_flags.set_flag(FF_IS_SELF);
        _reference.target_class_name = get_orchestration()->get_base_type();
    }
    reconstruct_node();
}

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

#include "api/extension_db.h"
#include "common/dictionary_utils.h"
#include "common/method_utils.h"
#include "common/property_utils.h"
#include "common/variant_utils.h"
#include "common/version.h"
#include "script/nodes/variables/variable_get.h"
#include "script/script_server.h"

#include <godot_cpp/classes/node3d.hpp>

OScriptNodeCallMemberFunction::OScriptNodeCallMemberFunction()
{
    _flags = ScriptNodeFlags::CATALOGABLE;
    _function_flags.set_flag(FF_IS_SELF);
}

void OScriptNodeCallMemberFunction::_upgrade(uint32_t p_version, uint32_t p_current_version)
{
    if (p_version == 1 && p_current_version >= 2)
    {
        Ref<OScriptNodePin> target = find_pin("target", PD_Input);
        if (target.is_valid() && PropertyUtils::is_nil_no_variant(target->get_property_info()))
        {
            bool reconstruct = false;
            if (target->has_any_connections())
            {
                // VariableGet missing class encoding - player.torch - _character property??

                Ref<OScriptNodePin> source = target->get_connections()[0];
                if (source.is_valid() && !source->get_property_info().class_name.is_empty())
                {
                    const String target_class = source->get_property_info().class_name;
                    if (ClassDB::class_has_method(target_class, _reference.method.name))
                    {
                        _reference.target_class_name = target_class;
                        _reference.target_type = Variant::OBJECT;
                        reconstruct = true;
                    }
                }
                else
                {
                    // Can the target be resolved by traversing the target_class_name hierarchy
                    // If the method match is found, update the reference details
                    const String class_name = _get_method_class_hierarchy_owner(_reference.target_class_name, _reference.method.name);
                    if (!class_name.is_empty())
                    {
                        _reference.target_class_name = class_name;
                        _reference.target_type = Variant::OBJECT;
                        reconstruct = true;
                    }
                }
            }
            else
            {
                // No connections, traverse base type hierarchy
                const String class_name = _get_method_class_hierarchy_owner(get_orchestration()->get_base_type(), _reference.method.name);
                if (!class_name.is_empty())
                {
                    _reference.target_class_name = class_name;
                    _reference.target_type = Variant::OBJECT;
                    reconstruct = true;
                }
            }

            if (reconstruct)
                reconstruct_node();
        }
    }

    super::_upgrade(p_version, p_current_version);
}

Ref<OScriptNodePin> OScriptNodeCallMemberFunction::_create_target_pin()
{
    PropertyInfo property;
    property.type = _reference.target_type;
    property.name = "target";
    property.hint = PROPERTY_HINT_NONE;
    property.usage = PROPERTY_USAGE_DEFAULT;

    if (ClassDB::is_parent_class(_reference.target_class_name, RefCounted::get_class_static()))
    {
        property.hint = PROPERTY_HINT_RESOURCE_TYPE;
        property.hint_string = _reference.target_class_name;
    }

    if (property.type == Variant::OBJECT)
        property.class_name = _reference.target_class_name;

    // Create target pin
    Ref<OScriptNodePin> target = create_pin(PD_Input, PT_Data, property);
    if (target.is_valid())
    {
        _function_flags.set_flag(FF_TARGET);
        if (property.type != Variant::OBJECT && !PropertyUtils::is_nil(property))
        {
            target->set_label(VariantUtils::get_friendly_type_name(property.type));
            target->no_pretty_format();

            // Target pins should never accept default values as they're implied to be provided an instance
            // as a result of another node's output. So for example, to use a member function such as the
            // "get_as_property_path" function on "NodePath", construct a NodePath and then connect that
            // node to a member function call for "get_as_property_path".
            target->set_flag(OScriptNodePin::Flags::IGNORE_DEFAULT);
        }
        else if (!property.class_name.is_empty())
        {
            target->set_label(property.class_name);
            target->no_pretty_format();
        }

        _chainable = true;
        notify_property_list_changed();
    }

    return target;
}

StringName OScriptNodeCallMemberFunction::_get_method_class_hierarchy_owner(const String& p_class_name, const String& p_method_name)
{
    String class_name = p_class_name;
    while (!class_name.is_empty())
    {
        TypedArray<Dictionary> methods;
        if (ScriptServer::is_global_class(class_name))
            methods = ScriptServer::get_global_class(class_name).get_method_list();
        else
            methods = ClassDB::class_get_method_list(class_name, true);

        for (int index = 0; index < methods.size(); ++index)
        {
            const Dictionary& dict = methods[index];
            if (dict.has("name") && p_method_name.match(dict["name"]))
                return class_name;
        }

        class_name = ClassDB::get_parent_class(class_name);
    }

    return "";
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

String OScriptNodeCallMemberFunction::get_node_title_color_name() const
{
    if (!ClassDB::class_exists(_reference.target_class_name))
        return "other_script_function_call";

    return "function_call";
}

String OScriptNodeCallMemberFunction::get_help_topic() const
{
    #if GODOT_VERSION >= 0x040300
    if (_reference.target_type != Variant::OBJECT)
    {
        BuiltInType type = ExtensionDB::get_builtin_type(_reference.target_type);
        return vformat("class_method:%s:%s", type.name, _reference.method.name);
    }
    else
    {
        const String class_name = MethodUtils::get_method_class(_reference.target_class_name, _reference.method.name);
        if (!class_name.is_empty())
            return vformat("class_method:%s:%s", class_name, _reference.method.name);
    }
    #endif
    return super::get_help_topic();
}

void OScriptNodeCallMemberFunction::initialize(const OScriptNodeInitContext& p_context)
{
    MethodInfo mi;
    StringName target_class = get_orchestration()->get_base_type();
    Variant::Type target_type = Variant::NIL;
    if (p_context.user_data)
    {
        // Built-in types supply target_type (Variant.Type) and "method" (dictionary)
        const Dictionary& data = p_context.user_data.value();
        if (!data.has("target_type") && !data.has("method"))
        {
            ERR_FAIL_MSG("Cannot initialize member function node, missing 'target_type' and 'method'");
        }

        target_type = VariantUtils::to_type(data["target_type"]);
        mi = DictionaryUtils::to_method(data["method"]);
        target_class = "";
    }
    else if (p_context.method && p_context.class_name)
    {
        // Class-type member function call, includes 'class_name' and 'method' (MethodInfo)
        mi = p_context.method.value();
        target_class = p_context.class_name.value();
        target_type = Variant::OBJECT;
    }
    else
    {
        ERR_FAIL_MSG("Cannot initialize member function node, missing attributes.");
    }

    ERR_FAIL_COND_MSG(mi.name.is_empty(), "Failed to initialize CallMemberFunction without a MethodInfo");

    _reference.method = mi;
    _reference.target_type = target_type;
    _reference.target_class_name = target_class;

    _set_function_flags(_reference.method);

    super::initialize(p_context);
}

void OScriptNodeCallMemberFunction::validate_node_during_build(BuildLog& p_log) const
{
    const Ref<OScriptNodePin> target = find_pin("target", PD_Input);
    if (target.is_valid())
    {
        String target_class = target->get_property_info().class_name;
        if (!target_class.is_empty() && !target->has_any_connections())
            if (!ClassDB::is_parent_class(get_orchestration()->get_base_type(), target_class))
                p_log.error(this, target, "Requires a connection.");
    }

    super::validate_node_during_build(p_log);
}

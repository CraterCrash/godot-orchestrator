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
#include "script/nodes/functions/call_static_function.h"

#include "api/extension_db.h"
#include "common/dictionary_utils.h"
#include "common/method_utils.h"
#include "common/property_utils.h"
#include "common/version.h"
#include "script/script_server.h"

class OScriptNodeCallStaticFunctionInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeCallStaticFunction);

    MethodInfo _method;
    StringName _class_name;

public:
    int step(OScriptExecutionContext& p_context) override
    {
        // We need to get the hash, stored from the extension_api.json
        const int64_t hash = ExtensionDB::get_static_function_hash(_class_name, _method.name);

        // Store a reference to the MethodBind structure
        GDExtensionMethodBindPtr mb = internal::gdextension_interface_classdb_get_method_bind(
            _class_name._native_ptr(), _method.name._native_ptr(), hash);

        if (!mb)
        {
            p_context.set_error(vformat("Failed to find method '%s'", _method.name));
            return -1 | STEP_FLAG_END;
        }

        Array args;
        for (size_t i = 0; i < _method.arguments.size(); i++)
            args.push_back(p_context.get_input(i));

        std::vector<const Variant*> call_args;
        call_args.resize(_method.arguments.size());

        for (int i = 0; i < args.size(); i++)
            call_args[i] = &args[i];

        Variant ret;
        GDExtensionCallError r_error;
        internal::gdextension_interface_object_method_bind_call(
            mb, nullptr, reinterpret_cast<GDExtensionConstVariantPtr*>(call_args.data()), call_args.size(), &ret, &r_error);

        if (r_error.error != GDEXTENSION_CALL_OK)
        {
            p_context.set_error(r_error);
            return -1 | STEP_FLAG_END;
        }

        if (MethodUtils::has_return_value(_method))
            p_context.set_output(0, ret);

        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void OScriptNodeCallStaticFunction::_get_property_list(List<PropertyInfo>* r_list) const
{
    r_list->push_back(PropertyInfo(Variant::STRING, "class_name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    r_list->push_back(PropertyInfo(Variant::STRING, "function_name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
}

bool OScriptNodeCallStaticFunction::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("class_name"))
    {
        r_value = _class_name;
        return true;
    }
    else if (p_name.match("function_name"))
    {
        r_value = _method_name;
        return true;
    }
    return false;
}

bool OScriptNodeCallStaticFunction::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("class_name"))
    {
        _class_name = p_value;
        return true;
    }
    else if (p_name.match("function_name"))
    {
        _method_name = p_value;
        return true;
    }
    return false;
}

void OScriptNodeCallStaticFunction::_resolve_method_info()
{
    // Lookup the MethodInfo
    TypedArray<Dictionary> methods;
    if (ScriptServer::is_global_class(_class_name))
        methods = ScriptServer::get_global_class(_class_name).get_method_list();
    else
        methods = ClassDB::class_get_method_list(_class_name, true);

    for (int i = 0; i < methods.size(); i++)
    {
        const Dictionary& dict = methods[i];
        if (_method_name.match(dict["name"]))
        {
            _method = DictionaryUtils::to_method(methods[i]);
            break;
        }
    }
}

void OScriptNodeCallStaticFunction::post_initialize()
{
    // We need to get the hash, stored from the extension_api.json
    const int64_t hash = ExtensionDB::get_static_function_hash(_class_name, _method_name);

    // Store a reference to the MethodBind structure
    _method_bind = internal::gdextension_interface_classdb_get_method_bind(
        _class_name._native_ptr(), _method_name._native_ptr(), hash);

    _resolve_method_info();

    reconstruct_node();

    super::post_initialize();
}

void OScriptNodeCallStaticFunction::post_placed_new_node()
{
    // We need to get the hash, stored from the extension_api.json
    const int64_t hash = ExtensionDB::get_static_function_hash(_class_name, _method_name);

    // Store a reference to the MethodBind structure
    _method_bind = internal::gdextension_interface_classdb_get_method_bind(
        _class_name._native_ptr(), _method_name._native_ptr(), hash);

    _resolve_method_info();

    super::post_placed_new_node();
}

void OScriptNodeCallStaticFunction::allocate_default_pins()
{
    create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"));
    create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("ExecOut"));

    const size_t default_start_index = _method.arguments.empty()
        ? 0
        : _method.arguments.size() - _method.default_arguments.size();

    size_t def_index = 0;
    for (size_t arg_index = 0; arg_index < _method.arguments.size(); arg_index++)
    {
        const PropertyInfo& pi = _method.arguments[arg_index];
        const Variant default_value = arg_index >= default_start_index ? _method.default_arguments[def_index++] : Variant();
        create_pin(PD_Input, PT_Data, pi, default_value);
    }

    if (MethodUtils::has_return_value(_method))
    {
        Ref<OScriptNodePin> rvalue = create_pin(PD_Output, PT_Data, PropertyUtils::as("return_value", _method.return_val));
        if (_method.return_val.type == Variant::OBJECT)
            rvalue->set_label(_method.return_val.class_name);
        else
            rvalue->hide_label();
    }

    super::allocate_default_pins();
}

String OScriptNodeCallStaticFunction::get_tooltip_text() const
{
    if (!_class_name.is_empty() && !_method_name.is_empty())
        return vformat("Calls the static function '%s.%s'", _class_name, _method_name);

    return "Calls a static function";
}

String OScriptNodeCallStaticFunction::get_node_title() const
{
    if (!_class_name.is_empty() && !_method_name.is_empty())
        return vformat("%s %s", _class_name, _method_name.capitalize());

    return "Call Static Function";
}

void OScriptNodeCallStaticFunction::validate_node_during_build(BuildLog& p_log) const
{
    const int args_no_defs = _method.arguments.size() - _method.default_arguments.size();
    for (int i = 0; i < args_no_defs; i++)
    {
        const PropertyInfo& pi = _method.arguments[i];
        const Ref<OScriptNodePin> pin = find_pin(pi.name, PD_Input);
        if (pin.is_valid() && !pin->has_any_connections())
            p_log.error(this, pin, "Requires a connection.");
    }

    return super::validate_node_during_build(p_log);
}

OScriptNodeInstance* OScriptNodeCallStaticFunction::instantiate()
{
    OScriptNodeCallStaticFunctionInstance* i = memnew(OScriptNodeCallStaticFunctionInstance);
    i->_node = this;
    i->_method = _method;
    i->_class_name = _class_name;
    return i;
}

void OScriptNodeCallStaticFunction::initialize(const OScriptNodeInitContext& p_context)
{
    ERR_FAIL_COND_MSG(!p_context.user_data, "Failed to initialize CallStaticFunction without user data");

    const Dictionary data = p_context.user_data.value();
    ERR_FAIL_COND_MSG(!data.has("class_name"), "Data is missing the class name.");
    ERR_FAIL_COND_MSG(!data.has("method_name"), "Data is missing the method name.");

    _class_name = data["class_name"];
    _method_name = data["method_name"];

    _resolve_method_info();

    super::initialize(p_context);
}

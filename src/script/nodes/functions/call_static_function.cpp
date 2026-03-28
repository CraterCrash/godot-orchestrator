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
#include "core/godot/gdextension_compat.h"
#include "script/script_server.h"

#include <godot_cpp/classes/resource_loader.hpp>

void OScriptNodeCallStaticFunction::_get_property_list(List<PropertyInfo>* r_list) const {
    r_list->push_back(PropertyInfo(Variant::STRING, "class_name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    r_list->push_back(PropertyInfo(Variant::STRING, "function_name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
}

bool OScriptNodeCallStaticFunction::_get(const StringName& p_name, Variant& r_value) const {
    if (p_name.match("class_name")) {
        r_value = _class_name;
        return true;
    } else if (p_name.match("function_name")) {
        r_value = _method_name;
        return true;
    }
    return false;
}

bool OScriptNodeCallStaticFunction::_set(const StringName& p_name, const Variant& p_value) {
    if (p_name.match("class_name")) {
        _class_name = p_value;
        return true;
    } else if (p_name.match("function_name")) {
        _method_name = p_value;
        return true;
    }
    return false;
}

void OScriptNodeCallStaticFunction::_resolve_method_info() {
    // Lookup the MethodInfo
    TypedArray<Dictionary> methods;
    if (ScriptServer::is_global_class(_class_name)) {
        methods = ScriptServer::get_global_class(_class_name).get_method_list();
    }
    else if (ExtensionDB::is_builtin_type(_class_name)) {
        BuiltInType type = ExtensionDB::get_builtin_type(_class_name);
        for (const MethodInfo& method : type.get_method_list()) {
            if (method.name.match(_method_name)) {
                _method = method;
                break;
            }
        }
        return;
    }
    else {
        methods = ClassDB::class_get_method_list(_class_name, true);
    }

    for (uint32_t i = 0; i < methods.size(); i++) {
        const Dictionary& dict = methods[i];
        if (_method_name.match(dict["name"])) {
            _method = DictionaryUtils::to_method(dict);
            break;
        }
    }
}

void OScriptNodeCallStaticFunction::post_initialize() {
    _resolve_method_info();
    reconstruct_node();
    super::post_initialize();
}

void OScriptNodeCallStaticFunction::post_placed_new_node() {
    _resolve_method_info();
    super::post_placed_new_node();
}

void OScriptNodeCallStaticFunction::allocate_default_pins() {
    create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"));
    create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("ExecOut"));

    const size_t default_start_index = is_vector_empty(_method.arguments)
        ? 0
        : _method.arguments.size() - _method.default_arguments.size();

    size_t def_index = 0;
    for (size_t arg_index = 0; arg_index < _method.arguments.size(); arg_index++) {
        const PropertyInfo& pi = _method.arguments[arg_index];
        const Variant default_value = arg_index >= default_start_index ? _method.default_arguments[def_index++] : Variant();
        create_pin(PD_Input, PT_Data, pi, default_value);
    }

    if (MethodUtils::has_return_value(_method)) {
        Ref<OScriptNodePin> rvalue = create_pin(PD_Output, PT_Data, PropertyUtils::as("return_value", _method.return_val));
        if (_method.return_val.type == Variant::OBJECT) {
            rvalue->set_label(_method.return_val.class_name);
        } else {
            rvalue->hide_label();
        }
    }

    super::allocate_default_pins();
}

String OScriptNodeCallStaticFunction::get_tooltip_text() const {
    if (!_class_name.is_empty() && !_method_name.is_empty()) {
        return vformat("Calls the static function '%s.%s'", _class_name, _method_name);
    }
    return "Calls a static function";
}

String OScriptNodeCallStaticFunction::get_node_title() const {
    if (!_class_name.is_empty() && !_method_name.is_empty()) {
        return vformat("%s %s", _class_name, _method_name.capitalize());
    }
    return "Call Static Function";
}

String OScriptNodeCallStaticFunction::get_help_topic() const {
    const String class_name = MethodUtils::get_method_class(_class_name, _method_name);
    if (!class_name.is_empty()) {
        return vformat("class_method:%s:%s", class_name, _method_name);
    }
    return super::get_help_topic();
}

void OScriptNodeCallStaticFunction::initialize(const OScriptNodeInitContext& p_context) {
    ERR_FAIL_COND_MSG(!p_context.user_data, "Failed to initialize CallStaticFunction without user data");

    const Dictionary data = p_context.user_data.value();
    ERR_FAIL_COND_MSG(!data.has("class_name"), "Data is missing the class name.");
    ERR_FAIL_COND_MSG(!data.has("method_name"), "Data is missing the method name.");

    _class_name = data["class_name"];
    _method_name = data["method_name"];

    _resolve_method_info();

    super::initialize(p_context);
}

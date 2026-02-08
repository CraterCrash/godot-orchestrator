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
#include "script/script_utility_callable.h"

#include "core/godot/variant/variant.h"
#include "script/utility_functions.h"

bool OScriptUtilityCallable::_compare_equal(const CallableCustom* p_a, const CallableCustom* p_b) {
    return p_a->hash() == p_b->hash();
}

bool OScriptUtilityCallable::_compare_less(const CallableCustom* p_a, const CallableCustom* p_b) {
    return p_a->hash() < p_b->hash();
}

uint32_t OScriptUtilityCallable::hash() const {
    return _h;
}

String OScriptUtilityCallable::get_as_text() const {
    String scope;
    switch (_type) {
        case TYPE_INVALID: {
            scope = "<invalid scope>";
            break;
        }
        case TYPE_GLOBAL: {
            scope = "@GlobalScope";
            break;
        }
        case TYPE_OSCRIPT: {
            scope = "@OScript";
            break;
        }
    }

    return vformat("%s::%s", scope, _function_name);
}

CallableCustom::CompareEqualFunc OScriptUtilityCallable::get_compare_equal_func() const {
    return _compare_equal;
}

CallableCustom::CompareLessFunc OScriptUtilityCallable::get_compare_less_func() const {
    return _compare_less;
}

bool OScriptUtilityCallable::is_valid() const {
    return _type != TYPE_INVALID;
}

ObjectID OScriptUtilityCallable::get_object() const {
    return {};
}

int OScriptUtilityCallable::get_argument_count(bool& r_is_valid) const {
    switch (_type) {
        case TYPE_INVALID: {
            r_is_valid = false;
            return 0;
        }
        case TYPE_GLOBAL: {
            r_is_valid = true;
            return GDE::Variant::get_utility_function_argument_count(_function_name);
        }
        case TYPE_OSCRIPT: {
            r_is_valid = true;
            return OScriptUtilityFunctions::get_function_argument_count(_function_name);
        }
    }
    ERR_FAIL_V_MSG(0, "Invalid type.");
}

void OScriptUtilityCallable::call(const Variant** p_arguments, int p_arg_count, Variant& r_return_value, GDExtensionCallError& r_call_error) const {
    switch (_type) {
        case TYPE_INVALID: {
            r_return_value = vformat(R"(Trying to call invalid utility function "%s".)", _function_name);
            r_call_error.error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;
            r_call_error.argument = 0;
            r_call_error.expected = 0;
            break;
        }
        case TYPE_GLOBAL: {
            GDE::Variant::call_utility_function(_function_name, r_return_value, p_arguments, p_arg_count, r_call_error);
            break;
        }
        case TYPE_OSCRIPT: {
            _function_ptr(&r_return_value, p_arguments, p_arg_count, r_call_error);
            break;
        }
    }
}

StringName OScriptUtilityCallable::get_method() const {
    return _function_name;
}

OScriptUtilityCallable::OScriptUtilityCallable(const StringName& p_function_name) : _function_name(p_function_name) {
    if (OScriptUtilityFunctions::function_exists(p_function_name)) {
        _type = TYPE_OSCRIPT;
        _function_ptr = OScriptUtilityFunctions::get_function(p_function_name);
    } else if (GDE::Variant::has_utility_function(p_function_name)) {
        _type = TYPE_GLOBAL;
    } else {
        ERR_PRINT(vformat(R"(Unknown utility function "%s".)", p_function_name));
    }

    _h = p_function_name.hash();
}
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
#include "script/utility_functions.h"

#include "common/varray.h"
#include "script/language.h"

#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/class_db.hpp>

#ifdef DEBUG_ENABLED
#define DEBUG_VALIDATE_ARG_COUNT(m_min_count, m_max_count)                          \
    if (unlikely(p_arg_count < m_min_count))                                        \
    {                                                                               \
        *r_ret = Variant();                                                         \
        r_error.error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS;                   \
        r_error.expected = m_min_count;                                             \
        return;                                                                     \
    }                                                                               \
    if (unlikely(p_arg_count > m_max_count))                                        \
    {                                                                               \
        *r_ret = Variant();                                                         \
        r_error.error = GDEXTENSION_CALL_ERROR_TOO_MANY_ARGUMENTS;                  \
        r_error.expected = m_max_count;                                             \
        return;                                                                     \
    }

#define DEBUG_VALIDATE_ARG_TYPE(m_arg, m_type)                                      \
    if (unlikely(!Variant::can_convert_strict(p_args[m_arg]->get_type(), m_type)))  \
    {                                                                               \
        *r_ret = Variant();                                                         \
        r_error.error = GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT;                    \
        r_error.argument = m_arg;                                                   \
        r_error.expected = m_type;                                                  \
        return;                                                                     \
    }
#else
#define DEBUG_VALIDATE_ARG_COUNT(m_min_count, m_max_count)
#define DEBUG_VALIDATE_ARG_TYPE(m_arg, m_type)
#endif // DEBUG_ENABLED

struct OScriptUtilityFunctionsDefinitions
{
    static inline void type_exists(Variant* r_ret, const Variant** p_args, int p_arg_count, GDExtensionCallError& r_error)
    {
        DEBUG_VALIDATE_ARG_COUNT(1, 1);
        DEBUG_VALIDATE_ARG_TYPE(0, Variant::STRING_NAME);
        *r_ret = ClassDB::class_exists(*p_args[0]);
    }

    #if GODOT_VERSION >= 0x040300
    static inline void print_debug(Variant* r_ret, const Variant** p_args, int p_arg_count, GDExtensionCallError& r_error)
    {
        String s;
        for (int i = 0; i < p_arg_count; i++)
            s += p_args[i]->operator String();

        const uint64_t caller_thread = OS::get_singleton()->get_thread_caller_id();
        if (caller_thread == OS::get_singleton()->get_main_thread_id())
        {
            OScriptLanguage* script = OScriptLanguage::get_singleton();
            if (script->_debug_get_stack_level_count() > 0)
            {
                s += "\n    At: "
                    + script->_debug_get_stack_level_source(0)
                    + ":"
                    + itos(script->_debug_get_stack_level_line(0))
                    + ":"
                    + script->_debug_get_stack_level_function(0)
                    + "()";
            }
        }
        else
        {
            s += "\n   At: Cannot retrieve debug info outside the main thread. Thread ID: " + itos(caller_thread);
        }

        UtilityFunctions::print(s);
        *r_ret = Variant();
    }

    static inline void print_stack(Variant* r_ret, const Variant** p_args, int p_arg_count, GDExtensionCallError& r_error)
    {
        DEBUG_VALIDATE_ARG_COUNT(0, 0);

        const uint64_t caller_thread = OS::get_singleton()->get_thread_caller_id();
        if (caller_thread != OS::get_singleton()->get_main_thread_id())
        {
            UtilityFunctions::print("Cannot retrieve debug info outside the main thread. Thread ID: " + itos(caller_thread));
            return;
        }

        OScriptLanguage* script = OScriptLanguage::get_singleton();
        for (int i = 0; i < script->_debug_get_stack_level_count(); i++)
        {
            UtilityFunctions::print(vformat("Frame %d - %s:%d in function '%s'",
                i,
                script->_debug_get_stack_level_source(i),
                script->_debug_get_stack_level_line(i),
                script->_debug_get_stack_level_function(i)));
        }

        *r_ret = Variant();
    }

    static inline void get_stack(Variant* r_ret, const Variant** p_args, int p_arg_count, GDExtensionCallError& r_error)
    {
        DEBUG_VALIDATE_ARG_COUNT(0, 0);

        const uint64_t caller_thread = OS::get_singleton()->get_thread_caller_id();
        if (caller_thread != OS::get_singleton()->get_main_thread_id())
        {
            *r_ret = TypedArray<Dictionary>();
            return;
        }

        OScriptLanguage* script = OScriptLanguage::get_singleton();

        TypedArray<Dictionary> ret;
        for (int i = 0; i < script->_debug_get_stack_level_count(); i++)
        {
            Dictionary frame;
            frame["source"] = script->_debug_get_stack_level_source(i);
            frame["function"] = script->_debug_get_stack_level_function(i);
            frame["line"] = script->_debug_get_stack_level_line(i);
            ret.push_back(frame);
        }

        *r_ret = ret;
    }
    #endif

    static inline void len(Variant* r_ret, const Variant** p_args, int p_arg_count, GDExtensionCallError& r_error)
    {
        DEBUG_VALIDATE_ARG_COUNT(1, 1);
        switch (p_args[0]->get_type())
        {
            case Variant::STRING:
            case Variant::STRING_NAME:
            {
                String d= *p_args[0];
                *r_ret = d.length();
                break;
            }
            case Variant::DICTIONARY:
            {
                Dictionary d= *p_args[0];
                *r_ret = d.size();
                break;
            }
            case Variant::ARRAY:
            {
                Array d = *p_args[0];
                *r_ret = d.size();
                break;
            }
            case Variant::PACKED_BYTE_ARRAY:
            {
                PackedByteArray d = *p_args[0];
                *r_ret = d.size();
                break;
            }
            case Variant::PACKED_INT32_ARRAY:
            {
                PackedInt32Array d = *p_args[0];
                *r_ret = d.size();
                break;
            }
            case Variant::PACKED_INT64_ARRAY:
            {
                PackedInt64Array d = *p_args[0];
                *r_ret = d.size();
                break;
            }
            case Variant::PACKED_FLOAT32_ARRAY:
            {
                PackedFloat32Array d = *p_args[0];
                *r_ret = d.size();
                break;
            }
            case Variant::PACKED_FLOAT64_ARRAY:
            {
                PackedFloat64Array d = *p_args[0];
                *r_ret = d.size();
                break;
            }
            case Variant::PACKED_STRING_ARRAY:
            {
                PackedStringArray d = *p_args[0];
                *r_ret = d.size();
                break;
            }
            case Variant::PACKED_VECTOR2_ARRAY:
            {
                PackedVector2Array d = *p_args[0];
                *r_ret = d.size();
                break;
            }
            case Variant::PACKED_VECTOR3_ARRAY:
            {
                PackedVector3Array d = *p_args[0];
                *r_ret = d.size();
                break;
            }
            #if GODOT_VERSION >= 0x040300
            case Variant::PACKED_VECTOR4_ARRAY:
            {
                PackedVector4Array d = *p_args[0];
                *r_ret = d.size();
                break;
            }
            #endif
            default:
            {
                *r_ret = vformat("Value of type '%s' cannot provide a length", Variant::get_type_name(p_args[0]->get_type()));
                r_error.error = GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT;
                r_error.argument = 0;
                r_error.expected = Variant::NIL;
                break;
            }
        }
    }
};

struct OScriptUtilityFunctionInfo
{
    OScriptUtilityFunctions::FunctionPtr function{ nullptr };
    MethodInfo info;
    bool is_const{ false };
};

static HashMap<StringName, OScriptUtilityFunctionInfo> utility_function_table;
static List<StringName> utility_function_name_table;

static void _register_function(const StringName& p_name, const MethodInfo& p_method, OScriptUtilityFunctions::FunctionPtr p_function, bool p_is_const)
{
    ERR_FAIL_COND(utility_function_table.has(p_name));

    OScriptUtilityFunctionInfo function;
    function.function = p_function;
    function.info = p_method;
    function.is_const = p_is_const;

    utility_function_table.insert(p_name, function);
    utility_function_name_table.push_back(p_name);
}

#define REGISTER_FUNC(m_func, m_is_const, m_return, m_args, m_is_vararg, m_default_args)        \
    {                                                                                           \
        String name(#m_func);                                                                   \
        if (name.begins_with("_"))                                                              \
            name = name.substr(1);                                                              \
                                                                                                \
        MethodInfo info = m_args;                                                               \
        info.name = name;                                                                       \
        info.return_val = m_return;                                                             \
        info.default_arguments = m_default_args;                                                \
        if (m_is_vararg)                                                                        \
            info.flags |= METHOD_FLAG_VARARG;                                                   \
                                                                                                \
        _register_function(name, info, OScriptUtilityFunctionsDefinitions::m_func, m_is_const); \
    }

#define RET(m_type)             PropertyInfo(Variant::m_type, "")
#define RETVAR                  PropertyInfo(Variant::NIL, "", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NIL_IS_VARIANT)
#define RETCLS(m_class)         PropertyInfo(Variant::OBJECT, "", PROPERTY_HINT_RESOURCE_TYPE, m_class)

#define NOARGS                  MethodInfo()
#define ARGS(...)               MethodInfo("", __VA_ARGS__)
#define ARG(m_name, m_type)     PropertyInfo(Variant::m_type, m_name)
#define ARGVAR(m_name)          PropertyInfo(Variant::NIL, m_name, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NIL_IS_VARIANT)
#define ARGTYPE(m_name)         PropertyInfo(Variant::INT, m_name, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_CLASS_IS_ENUM, "Variant.Type")

void OScriptUtilityFunctions::register_functions()
{
    REGISTER_FUNC(type_exists, true, RET(BOOL), ARGS(ARG("type", STRING_NAME)), false, varray());
    #if GODOT_VERSION >= 0x040300
    REGISTER_FUNC(print_debug, false, RET(NIL), NOARGS, true, varray());
    REGISTER_FUNC(print_stack, false, RET(NIL), NOARGS, false, varray());
    REGISTER_FUNC(get_stack, false, RET(ARRAY), NOARGS, false, varray());
    #endif
    REGISTER_FUNC(len, true, RET(INT), ARGS(ARGVAR("var")), false, varray());
}

void OScriptUtilityFunctions::unregister_functions()
{
    utility_function_name_table.clear();
    utility_function_table.clear();
}

OScriptUtilityFunctions::FunctionPtr OScriptUtilityFunctions::get_function(const StringName& p_function_name)
{
    OScriptUtilityFunctionInfo* info = utility_function_table.getptr(p_function_name);
    ERR_FAIL_NULL_V(info, nullptr);

    return info->function;
}

bool OScriptUtilityFunctions::function_exists(const StringName& p_function_name)
{
    return utility_function_table.has(p_function_name);
}

List<StringName> OScriptUtilityFunctions::get_function_list()
{
    List<StringName> result;
    for (const StringName& E : utility_function_name_table)
        result.push_back(E);

    return result;
}

MethodInfo OScriptUtilityFunctions::get_function_info(const StringName& p_function_name)
{
    OScriptUtilityFunctionInfo* info = utility_function_table.getptr(p_function_name);
    ERR_FAIL_NULL_V(info, MethodInfo());

    return info->info;
}



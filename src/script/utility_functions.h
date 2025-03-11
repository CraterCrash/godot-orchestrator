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
#ifndef ORCHESTRATOR_SCRIPT_UTILITY_FUNCTIONS_H
#define ORCHESTRATOR_SCRIPT_UTILITY_FUNCTIONS_H

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/templates/list.hpp>

using namespace godot;

/// Utility class that provides access to a variety of C++ pure functions that are accessible in
/// an Orchestration, much like how certain functions like <code>load</code> are accessible in GDScript.
class OScriptUtilityFunctions
{
public:
    typedef void (*FunctionPtr)(Variant* r_ret, const Variant** p_args, int p_arg_count, GDExtensionCallError& r_error);

    /**
     * Get the function pointer
     *
     * @param p_function_name the function name
     * @return the function pointer
     */
    static OScriptUtilityFunctions::FunctionPtr get_function(const StringName& p_function_name);

    /**
     * Check whether a language-specific utility function exists
     *
     * @param p_function_name the function name
     * @return true if the function exists, false otherwise
     */
    static bool function_exists(const StringName& p_function_name);

    /**
     * Get a list of all registered utility functions
     * @return list of function names
     */
    static List<StringName> get_function_list();

    /**
     * Get the method information about a function.
     *
     * @param p_function_name the function name
     * @return the function's method information structure
     */
    static MethodInfo get_function_info(const StringName& p_function_name);

    static void register_functions();
    static void unregister_functions();
};

#endif // ORCHESTRATOR_SCRIPT_UTILITY_FUNCTIONS_H
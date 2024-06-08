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
#ifndef ORCHESTRATOR_SCRIPT_DOC_DATA_H
#define ORCHESTRATOR_SCRIPT_DOC_DATA_H

#include "script/script.h"

/// Helper class that creates documentation for an Orchestration script
class OScriptDocData
{
    /// Get the property documentation
    /// @param p_property the property
    /// @return the property documentation
    static Dictionary _create_property_documentation(const PropertyInfo& p_property);

    /// Get the method argument documentation
    /// @param p_properties the properties
    /// @return the arguments documentation
    static TypedArray<Dictionary> _get_method_arguments_documentation(const std::vector<PropertyInfo>& p_properties);

    /// Get the method return type
    /// @param p_method the method information
    /// @return the return type
    static String _get_method_return_type(const MethodInfo& p_method);

    /// Creates documentation for a given <code>MethodInfo</code>.
    /// @param p_method the method information
    /// @param p_description the custom method description
    /// @return the method documentation
    static Dictionary _method_info_documentation(const MethodInfo& p_method, const String& p_description);

    /// Creates documentation for the script's properties
    /// @param p_script the script to create documentation about
    /// @return the properties documentation
    static TypedArray<Dictionary> _create_properties_documentation(const Ref<OScript>& p_script);

    /// Creates documentation for the script's signals
    /// @param p_script the script to create documentation about
    /// @return the signals documentation
    static TypedArray<Dictionary> _create_signals_documentation(const Ref<OScript>& p_script);

    /// Creates documentation for the script's functions
    /// @param p_script the script to create documentation about
    /// @return the function documentation
    static TypedArray<Dictionary> _create_functions_documentation(const Ref<OScript>& p_script);

    /// Intentially private
    OScriptDocData() = default;

public:
    /// Creates the documentation
    /// @param p_script the script to create documentation about
    /// @return typed array of documentation data
    static TypedArray<Dictionary> create_documentation(const Ref<OScript>& p_script);
};

#endif // ORCHESTRATOR_SCRIPT_DOC_DATA_H
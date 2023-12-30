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
#ifndef ORCHESTRATOR_DICTIONARY_UTILS_H
#define ORCHESTRATOR_DICTIONARY_UTILS_H

#include <godot_cpp/core/method_bind.hpp>
#include <godot_cpp/core/property_info.hpp>

using namespace godot;

namespace DictionaryUtils
{
    /// Convert a Dictionary to a PropertyInfo.
    ///
    /// @param p_dict the dictionary that represents a property
    /// @return the PropertyInfo struct
    PropertyInfo to_property(const Dictionary& p_dict);

    /// Convert a PropertyInfo to a Dictionary.
    ///
    /// @param p_property the property to convert to a dictionary
    /// @return the dictionary
    Dictionary from_property(const PropertyInfo& p_property);

    /// Convert a Dictionary to a GDExtensionPropertyInfo.
    ///
    /// @attention A GDExtensionPropertyInfo is unlike a normal PropertyInfo in that the data that
    /// is held by the StringName and String properties must be pointers as the object will be
    /// passed back to the Godot engine.
    ///
    /// @param p_dict the dictionary that represents a property
    /// @return the GDExtensionPropertyInfo object
    GDExtensionPropertyInfo to_extension_property(const Dictionary& p_dict);

    /// Convert a Dictionary to a MethodInfo.
    ///
    /// @param p_dict the dictionary that represents a method or signal
    /// @return the MethodInfo struct
    MethodInfo to_method(const Dictionary& p_dict);

    /// Convert a MethodInfo to a Dictionary.
    ////
    /// @param p_method the method or signal to convert to a dictionary
    /// @return the dictionary
    Dictionary from_method(const MethodInfo& p_method);

    /// Constructs a simple dictionary from a list of variant pair tuples
    ///
    /// @param p_values the key/value pairs to insert into a dictionary
    /// @return the constructed dictionary
    Dictionary of(std::initializer_list<std::pair<Variant, Variant>>&& p_values);
}

#endif  // ORCHESTRATOR_DICTIONARY_UTILS_H

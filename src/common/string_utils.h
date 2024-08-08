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
#ifndef ORCHESTRATOR_STRING_UTILS_H
#define ORCHESTRATOR_STRING_UTILS_H

#include <godot_cpp/variant/string.hpp>

using namespace godot;

namespace StringUtils
{
    /// Join the array elements with the delimiter
    /// @tparam T the array type
    /// @param p_delimiter the delimiter
    /// @param p_array the array
    /// @return the string result joined by the delimiter
    template<typename T> String join(const String& p_delimiter, const T& p_array)
    {
        String result;
        if (!p_array.is_empty())
        {
            result = p_array[0];
            for (int i = 1; i < p_array.size(); i++)
                result += (p_delimiter + String(p_array[i]));
        }
        return result;
    }

    /// Return the value unless its empty, and then return the default value.
    /// @param p_value the value to check
    /// @param p_default_value the default value
    /// @return the value unless its empty, and the default value if its empty
    String default_if_empty(const String& p_value, const String& p_default_value);

    /// Replace the first occurrence of the key with the with argument in the value
    ///
    /// @param p_value the string value to mutate
    /// @param p_key the value to look for
    /// @param p_with the value to replace the key with
    /// @return the mutated string after replacement
    String replace_first(const String& p_value, const String& p_key, const String& p_with);

    //~ Begin "taken from ustring.h"
    String path_to_file(const String &p_local, const String &p_path);
    String path_to(const String &p_local, const String &p_path);
    String property_name_encode(const String& p_name);
    String c_escape_multiline(const String& p_name);
    //~ End "taken from ustring.h"
}

#endif // ORCHESTRATOR_STRING_UTILS_H
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
#ifndef ORCHESTRATOR_GUID_H
#define ORCHESTRATOR_GUID_H

#include <godot_cpp/classes/random_number_generator.hpp>

using namespace godot;

/// A simple Globally Unique Identifier implementation.
class Guid
{
    uint32_t _a{ 0 };
    uint32_t _b{ 0 };
    uint32_t _c{ 0 };
    uint32_t _d{ 0 };

    /// Parses the Guid string into its respective components
    ///
    /// @param p_guid_str the string to be parsed
    /// @param r_a the output value of the first component
    /// @param r_b the output value of the second component
    /// @param r_c the output value of the third component
    /// @param r_d the output value of the fourth component
    /// @return true if the parse was successful; false otherwise
    static bool _parse(const String &p_guid_str, uint32_t &r_a, uint32_t &r_b, uint32_t &r_c, uint32_t &r_d);

public:
    /// Default constructor
    Guid();

    /// Creates the Guid from a text string
    /// @param p_guid the guid text string
    Guid(const String &p_guid);

    /// Creates the Guid from its individual components
    /// @param p_a the first component value
    /// @param p_b the second component value
    /// @param p_c the third component value
    /// @param p_d the fourth component value
    Guid(uint32_t p_a, uint32_t p_b, uint32_t p_c, uint32_t p_d);

    /// Invalidates the Guid
    void invalidate();

    /// Checks whether this Guid is valid
    /// @return true if the GUID is valid; false otherwise
    bool is_valid() const;

    /// Converts this Guid to its string representation
    /// @return the GUID as a formatted GUID string
    String to_string() const;

    /// Helper method to create a new guid
    /// @return a new GUID instance
    static Guid create_guid();

    operator Variant() const { return to_string(); }

    bool operator==(const Guid &p_o) const;
    bool operator!=(const Guid &p_o) const;
};

#endif  // ORCHESTRATOR_GUID_H

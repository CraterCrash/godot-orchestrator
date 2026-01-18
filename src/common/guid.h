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
#ifndef ORCHESTRATOR_GUID_H
#define ORCHESTRATOR_GUID_H

#include <godot_cpp/classes/random_number_generator.hpp>

using namespace godot;

/// A simple Globally Unique Identifier implementation.
class Guid {
    uint32_t _a{ 0 };
    uint32_t _b{ 0 };
    uint32_t _c{ 0 };
    uint32_t _d{ 0 };

    static Ref<RandomNumberGenerator>& _get_random_number_generator();

    /// Parses the Guid string into its respective components
    ///
    /// @param p_guid_str the string to be parsed
    /// @param r_a the output value of the first component
    /// @param r_b the output value of the second component
    /// @param r_c the output value of the third component
    /// @param r_d the output value of the fourth component
    /// @return true if the parse was successful; false otherwise
    static bool _parse(const String& p_guid_str, uint32_t& r_a, uint32_t& r_b, uint32_t& r_c, uint32_t& r_d);

public:
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

    /// Cleanup static resources
    static void cleanup();

    operator Variant() const { return to_string(); }

    bool operator==(const Guid &p_o) const;
    bool operator!=(const Guid &p_o) const;

    _FORCE_INLINE_ uint64_t hash() const {
        return (static_cast<uint64_t>(_a) << 32 | _b) ^ (static_cast<uint64_t>(_c) << 32 | _d);
    }

    Guid();
    explicit Guid(const String& p_guid);
    Guid(uint32_t p_a, uint32_t p_b, uint32_t p_c, uint32_t p_d);
};

_FORCE_INLINE_ uint64_t hash(const Guid& p_guid) { return p_guid.hash(); }

#endif  // ORCHESTRATOR_GUID_H

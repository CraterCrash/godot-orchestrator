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
#ifndef ORCHESTRATOR_GODOT_VERSION_H
#define ORCHESTRATOR_GODOT_VERSION_H

#include <godot_cpp/godot.hpp>

/// This class provides a way to cleanly query the Godot version at runtime. In addition, this also allows
/// for centralizing version specific methods, such as "at_least" so that differences in Godot behavior
/// can be handled dynamically rather than statically at build-time.
///
struct GodotVersionInfo
{
private:
    GDExtensionGodotVersion _version;

public:
    constexpr uint32_t major() const { return _version.major; }
    constexpr uint32_t minor() const { return _version.minor; }
    constexpr uint32_t patch() const { return _version.patch; }

    constexpr const char* string() const { return _version.string; }

    constexpr bool at_least(uint32_t maj, uint32_t min, uint32_t patch = 0) const
    {
        return (_version.major > maj) ||
               (_version.major == maj && _version.minor > min) ||
               (_version.major == maj && _version.minor == min && _version.patch >= patch);
    }

    constexpr bool equals(uint32_t maj, uint32_t min, uint32_t patch = 0) const
    {
        return _version.major == maj && _version.minor == min && _version.patch == patch;
    }

    explicit GodotVersionInfo() { godot::internal::gdextension_interface_get_godot_version(&_version); }

    // Should only be used in tests, runtime code should use the no-arg constructor
    constexpr GodotVersionInfo(const GDExtensionGodotVersion& v) : _version(v) {}
};

#endif // ORCHESTRATOR_GODOT_VERSION_H

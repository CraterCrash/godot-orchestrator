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
#ifndef ORCHESTRATOR_CORE_GODOT_GD_EXTENSION_INTERFACE_COMPAT_H
#define ORCHESTRATOR_CORE_GODOT_GD_EXTENSION_INTERFACE_COMPAT_H

#include "common/version.h"

#include <godot_cpp/godot.hpp>

/// In Godot 4.6, the GDExtension API namespace changed from 'godot::internal' with all methods prefixed with
/// 'gdextension_interface' to a new namespace 'godot::gdextension_interface' where the method prefix was
/// dropped since it's part of the namespace. This macro eases that transition.
#if GODOT_VERSION >= 0x040600
#define GDE_INTERFACE(m_func) ::godot::gdextension_interface::m_func
#else
#define GDE_INTERFACE(m_func) ::godot::internal::gdextension_interface_##m_func
#endif

// Utility helper for migration from std::vector to LocalVector use cases
template <typename T>
bool is_vector_empty(const T& p_container) {
    if constexpr (requires { p_container.is_empty(); }) {
        return p_container.is_empty();
    } else {
        return p_container.empty();
    }
}

#endif // ORCHESTRATOR_CORE_GODOT_GD_EXTENSION_INTERFACE_COMPAT_H
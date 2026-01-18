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
#ifndef ORCHESTRATOR_CORE_GODOT_HASHFUNCS_H
#define ORCHESTRATOR_CORE_GODOT_HASHFUNCS_H

#include "common/version.h"
#include <godot_cpp/templates/hashfuncs.hpp>

namespace godot {
    #if GODOT_VERSION < 0x040500
    template <typename T>
    struct THashableHasher {
        static _FORCE_INLINE_ uint32_t hash(const T& hashtable) { return hashtable.hash(); }
    };
    #else
    template <typename T>
    using THashableHasher = HashableHasher<T>;
    #endif
}

#endif // ORCHESTRATOR_CORE_GODOT_HASHFUNCS_H
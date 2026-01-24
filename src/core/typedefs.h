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
#ifndef ORCHESTRATOR_CORE_TYPE_DEFS_H
#define ORCHESTRATOR_CORE_TYPE_DEFS_H

#include <godot_cpp/core/defs.hpp>

// In some cases [[nodiscard]] will get false positives,
// we can prevent the warning in specific cases by preceding the call with a cast.
// Note: This is mainly required for compatibility with Godot 4.4 and earlier.
#ifndef _ALLOW_DISCARD_
#define _ALLOW_DISCARD_ (void)
#endif

template <typename T, size_t SIZE>
constexpr size_t std_size(const T(&)[SIZE]) {
    return SIZE;
}

#endif // ORCHESTRATOR_CORE_TYPE_DEFS_H
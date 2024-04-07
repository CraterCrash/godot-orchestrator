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
#ifndef ORCHESTRATOR_METHOD_UTILS_H
#define ORCHESTRATOR_METHOD_UTILS_H

#include <godot_cpp/core/method_bind.hpp>

using namespace godot;

namespace MethodUtils
{
    /// Checks whether the specified Godot <code>MethodInfo</code> has a return value.
    /// @param p_method the Godot method info structure
    /// @return <code>true</code> if the method returns a value; <code>false</code> otherwise
    bool has_return_value(const MethodInfo& p_method);
}

#endif // ORCHESTRATOR_METHOD_UTILS_H
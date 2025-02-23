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
#ifndef ORCHESTRATOR_VARRAY_H
#define ORCHESTRATOR_VARRAY_H

#include <godot_cpp/variant/variant.hpp>
#include <vector>

using namespace godot;

template<typename... VarArgs>
std::vector<Variant> varray(VarArgs... p_args)
{
    std::vector<Variant> v;
    const Variant args[sizeof...(p_args) + 1] = { p_args..., Variant() };
    const uint32_t argc = sizeof...(p_args);
    if (argc > 0)
    {
        v.resize(argc);
        for (uint32_t i = 0; i < argc; i++)
            v[i] = args[i];
    }

    return v;
}

#endif // ORCHESTRATOR_VARRAY_H
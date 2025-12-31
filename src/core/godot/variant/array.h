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
#ifndef ORCHESTRATOR_CORE_GODOT_VARIANT_ARRAY_H
#define ORCHESTRATOR_CORE_GODOT_VARIANT_ARRAY_H

#include <godot_cpp/variant/variant.hpp>

namespace GDE {

    struct Array {
        Array() = delete;

        using ArrayType = godot::Array;

        static ArrayType from_variant_ptrs(const Variant** p_variants, int p_size) {
            ArrayType result;
            result.resize(p_size);

            for (uint32_t i = 0; i < p_size; i++) {
                result[i] = *p_variants[i];
            }

            return result;
        }

        template <typename RangeLoopContainer>
        static ArrayType from_container(const RangeLoopContainer& p_container) {
            ArrayType result;
            for (const auto& item : p_container) {
                result.push_back(item);
            }
            return result;
        }
    };

}

#endif // ORCHESTRATOR_CORE_GODOT_VARIANT_ARRAY_H
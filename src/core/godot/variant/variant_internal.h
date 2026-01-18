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
#ifndef ORCHESTRATOR_CORE_GODOT_VARIANT_VARIANT_INTERNAL_H
#define ORCHESTRATOR_CORE_GODOT_VARIANT_VARIANT_INTERNAL_H

#include "core/godot/gdextension_compat.h"

#include <godot_cpp/variant/variant.hpp>

namespace GDE {
    struct VariantInternal {
        VariantInternal() = delete;

        /// Constructs a <code>Variant</code> in-place based on the supplied <code>Variant::Type</code>
        _FORCE_INLINE_ static void initialize(godot::Variant* p_value, godot::Variant::Type p_type) {
            GDExtensionCallError error;
            GDE_INTERFACE(variant_construct)(
                static_cast<GDExtensionVariantType>(p_type),
                p_value,
                nullptr,
                0,
                &error);
        }
    };
}

#endif // ORCHESTRATOR_CORE_GODOT_VARIANT_VARIANT_INTERNAL_H
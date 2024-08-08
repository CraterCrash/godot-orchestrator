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
#ifndef ORCHESTRATOR_VARIANT_OPERATORS_H
#define ORCHESTRATOR_VARIANT_OPERATORS_H

#include <godot_cpp/variant/variant.hpp>

using namespace godot;

/// There is a serious disconnect between the Variant::Operator enumeration that is managed by
/// the GDExtension (godot-cpp) project and the Godot Engine (godot) where the engine has an
/// extra operation {@code OP_POWER} injected that does not exist in the godot-cpp enum. This
/// creates a disconnect when using Variant::Operator with any engine code.
///
/// This namespace is meant to provide a bridge for this until this is eventually fixed.
///
/// Engine: https://github.com/godotengine/godot/blob/master/core/variant/variant.h#L511-L542
/// CPP   : https://github.com/godotengine/godot-cpp/blob/master/include/godot_cpp/variant/variant.hpp#L109-L139
namespace VariantOperators
{
    enum Code
    {
        OP_EQUAL,
        OP_NOT_EQUAL,
        OP_LESS,
        OP_LESS_EQUAL,
        OP_GREATER,
        OP_GREATER_EQUAL,

        // mathematic
        OP_ADD,
        OP_SUBTRACT,
        OP_MULTIPLY,
        OP_DIVIDE,
        OP_NEGATE,
        OP_POSITIVE,
        OP_MODULE,
        OP_POWER,

        // bitwise
        OP_SHIFT_LEFT,
        OP_SHIFT_RIGHT,
        OP_BIT_AND,
        OP_BIT_OR,
        OP_BIT_XOR,
        OP_BIT_NEGATE,

        // logic
        OP_AND,
        OP_OR,
        OP_XOR,
        OP_NOT,

        // containment
        OP_IN,
        OP_MAX
    };

    /// Takes an Orchestrator operator code and converts it to an engine code.
    /// NOTE: This should only be used when interfacing with the engine only!
    ///
    /// @param p_code the Orchestrator operator code
    /// @return the engine operator code
    Variant::Operator to_engine(Code p_code);

}

#endif // ORCHESTRATOR_VARIANT_OPERATORS_H
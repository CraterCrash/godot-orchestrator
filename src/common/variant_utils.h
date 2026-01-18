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
#ifndef ORCHESTRATOR_VARIANT_UTILS_H
#define ORCHESTRATOR_VARIANT_UTILS_H

#include <godot_cpp/variant/variant.hpp>

using namespace godot;

namespace VariantUtils {

    /// Get the appropriate article to be used based on variant type.
    /// @param p_type the variant type
    /// @param p_nil_to_any whether to treat "Nil" as "Any"
    /// @return the article, `a` or `an` depending on the type
    String get_type_name_article(Variant::Type p_type, bool p_nil_to_any = false);

    /// Godot's Variant class provides a method, "get_type_name"; however, this method returns
    /// several types such as Integer and Boolean with shortened values. This method aims to
    /// return all types with their full, not abbreviated names.
    ///
    /// @param p_type the variant type
    /// @param p_nil_to_any whether to report "Nil" as "Any"
    /// @return the full friendly type name
    String get_friendly_type_name(Variant::Type p_type, bool p_nil_to_any = false);

    /// Creates a property enum list, which is a comma-separated list of type names that can be
    /// used by the Godot Editor InspectorDock for selecting a type, optionally specifying if
    /// the list should or should not include the conversion of NIL as Any.
    ///
    /// @param p_include_any if true, Variant::NIL will be represented as Any, otherwise its omitted
    /// @return a comma-seperated list of type names for an enum inspector property
    String to_enum_list(bool p_include_any = true);

    /// Converts a numeric value to its appropriate type..
    ///
    /// @param p_type the variant type
    /// @return the calculated variant type
    Variant::Type to_type(int p_type);

    /// Creates a Variant with its default value based on the supplied type.
    /// @param p_type the variant type
    /// @return the default Variant value of the specified type
    Variant make_default(Variant::Type p_type);

    /// Converts the value to the specified type
    /// @param p_value the value to convert
    /// @param p_target_type the target type
    /// @return the converted type
    Variant convert(const Variant& p_value, Variant::Type p_target_type);

    /// Cast to a desired type.
    /// @param p_value the value to be cast
    /// @param T the cast type
    /// @return the value cast to the specified type <T>
    template<typename T> T cast_to(const Variant& p_value) { return T(p_value); }

    /// Evaluates two variants
    /// @param p_operator the operation to be performed
    /// @param p_left the left operand
    /// @param p_right the right operand
    /// @param r_value the return value
    /// @return true if the evaluation was successful, false otherwise
    bool evaluate(Variant::Operator p_operator, const Variant& p_left, const Variant& p_right, Variant& r_value);

    /// Evaluates two variants, returning the result.
    /// @param p_operator the operation to be performed
    /// @param p_left the left operand
    /// @param p_right the right operand
    /// @return the evaluated result value between the two operands
    Variant evaluate(Variant::Operator p_operator, const Variant& p_left, const Variant& p_right);
}

#endif  // ORCHESTRATOR_VARIANT_UTILS_H

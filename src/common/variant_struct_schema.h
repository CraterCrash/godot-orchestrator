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
#pragma once

#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

/// Describes the built-in Variant types that are composed of smaller parts, such as Vector2,
/// Rect2, Transform3D or Color.
///
/// Two views are exposed for each composite type:
///
/// - <b>Components</b> are the ordered parts that reconstruct the value through the type's own
///   constructor, i.e. <code>Type(c1, c2, ...)</code> yields the original value. This is the set
///   used by Make nodes, by the struct default-value widget, and by pin splitting.
/// - <b>Properties</b> are every readable member the type exposes, including derived ones such as
///   Rect2's <code>end</code> or Color's HSV members. This is the set used by Break nodes.
///
/// The schema is derived from ExtensionDB and is available once the database has been created.
///
namespace VariantStructSchema {

    /// Returns whether the type is a composite whose components reconstruct it via its constructor.
    /// @param p_type the variant type
    /// @return true if the type has constructor-ordered components, false otherwise
    bool is_composite(Variant::Type p_type);

    /// Get the constructor-ordered components of a composite type.
    /// @param p_type the variant type
    /// @return the component properties in constructor order, or an empty vector for non-composites
    const Vector<PropertyInfo>& get_components(Variant::Type p_type);

    /// Get the names of the constructor-ordered components of a composite type.
    /// @param p_type the variant type
    /// @return the component names in constructor order
    PackedStringArray get_component_names(Variant::Type p_type);

    /// Get every readable property the type exposes, in declaration order.
    /// @param p_type the variant type
    /// @return the readable properties, or an empty vector when the type exposes none
    const Vector<PropertyInfo>& get_properties(Variant::Type p_type);

    /// Get the names of every readable property the type exposes, in declaration order.
    /// @param p_type the variant type
    /// @return the property names
    PackedStringArray get_property_names(Variant::Type p_type);

    /// Get the dotted paths to every leaf component, recursing through composite components.
    /// For example, Rect2 yields <code>position.x</code>, <code>position.y</code>,
    /// <code>size.x</code> and <code>size.y</code>.
    /// @param p_type the variant type
    /// @return the leaf component paths in constructor order
    PackedStringArray get_component_paths(Variant::Type p_type);

    /// Construct a composite value from its components, supplied in constructor order.
    /// @param p_type the variant type
    /// @param p_components the component values, one per entry of <code>get_components</code>
    /// @param r_valid set to whether construction succeeded, may be null
    /// @return the constructed value, or a default-constructed value on failure
    Variant compose(Variant::Type p_type, const Vector<Variant>& p_components, bool* r_valid = nullptr);

    /// Verifies that every composite type reconstructs its default value from its components.
    /// Reports any type whose schema disagrees with the engine's constructors. Intended to be run
    /// once at startup in debug builds only.
    /// @return true if every composite type round-trips, false otherwise
    bool verify();
}
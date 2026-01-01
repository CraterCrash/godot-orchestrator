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
#ifndef ORCHESTRATOR_PROPERTY_UTILS_H
#define ORCHESTRATOR_PROPERTY_UTILS_H

#include <godot_cpp/core/property_info.hpp>

namespace PropertyUtils
{
    using namespace godot;

    /// Checks whether two property info structures are identical (Excluding name).
    /// @param p_left a property
    /// @param p_right a property
    /// @return true if the two are identical, false otherwise
    bool are_equal(const PropertyInfo& p_left, const PropertyInfo& p_right);

    /// Constructs a new property info with a new name from an existing property
    /// @param p_name the new property name
    /// @param p_property the property to source data from
    /// @return the newly constructed property with the new name
    PropertyInfo as(const String& p_name, const PropertyInfo& p_property);

    /// Create a simple exec property
    /// @param p_name the execution pin name
    /// @return the execution property
    PropertyInfo make_exec(const String& p_name);

    /// Make a variant-based property
    /// @param p_name the property pin name
    /// @return the newly constructed property with the specified type
    PropertyInfo make_variant(const String& p_name);

    /// Make an object-based property for a given class type
    /// @param p_name the property pin name
    /// @param p_class_name the class name, if unspecified the pin accepts any Object
    /// @return the newly constructed property with the specified class type
    PropertyInfo make_object(const String& p_name, const String& p_class_name = "Object");

    /// Make a file property
    /// @param p_name the property pin name
    /// @param p_filters the file filters, defaults to none
    /// @return the newly constructed property
    PropertyInfo make_file(const String& p_name, const String& p_filters = String());

    /// Makes a simple typed property.
    ///
    /// @note This should not be used to make complex types such as objects, enums, or bitfields, nor
    /// to construct properties that represent various hinted types such as files or multilined text.
    /// @param p_name the property pin name
    /// @param p_type the basic type
    /// @param p_variant_on_nil whether the property should be a variant if the type is NIL
    /// @return the newly constructed property with the specified type
    PropertyInfo make_typed(const String& p_name, Variant::Type p_type, bool p_variant_on_nil = false);

    /// Make a multiline text property.
    /// @param p_name the property pin name
    /// @return the newly constructed property
    PropertyInfo make_multiline(const String& p_name);

    /// Creates a property info for a global enum type (class has enum name)
    /// @param p_name the property name
    /// @param p_class_name the global enum class name
    /// @return the property info structure
    PropertyInfo make_enum_class(const String& p_name, const String& p_class_name);

    /// Creates a property info for a class-specific enumeration type
    /// @param p_name the property name
    /// @param p_class_name the class that owns the enum
    /// @param p_enum_name the name of the enumeration
    /// @return the property info structure
    PropertyInfo make_class_enum(const String& p_name, const String& p_class_name, const String& p_enum_name);

    /// Checks whether the property type is <code>NIL</code>
    /// @param p_property the property to check
    /// @return true if the property is NIL, false otherwise
    _FORCE_INLINE_ bool is_nil(const PropertyInfo& p_property) { return p_property.type == Variant::NIL; }

    /// Checks whether the property represents a variant.
    /// @param p_property the property to check
    /// @return true if the property is a variant, false otherwise
    _FORCE_INLINE_ bool is_variant(const PropertyInfo& p_property) { return is_nil(p_property) && (p_property.usage & PROPERTY_USAGE_NIL_IS_VARIANT); }

    /// Checks whether the property represents a class type.
    /// @param p_property the property to check
    /// @return true if the property is a class type, false otherwise
    _FORCE_INLINE_ bool is_class(const PropertyInfo& p_property) { return p_property.type == Variant::OBJECT && !p_property.class_name.is_empty() && !p_property.class_name.contains("."); }

    /// Checks whether the property represents an enum.
    /// @param p_property the property to check
    /// @return true if the property is an enumeration, false otherwise
    _FORCE_INLINE_ bool is_enum(const PropertyInfo& p_property)
    {
        if (p_property.type != Variant::INT)
            return false;

        return p_property.hint == PROPERTY_HINT_ENUM || p_property.usage & PROPERTY_USAGE_CLASS_IS_ENUM;
    }

    /// Checks whether the property is a bitfield.
    /// @param p_property the property to check
    /// @return true if the property is a bitfield, false otherwise
    _FORCE_INLINE_ bool is_bitfield(const PropertyInfo& p_property)
    {
        if (p_property.type != Variant::INT)
            return false;

        return p_property.hint == PROPERTY_HINT_FLAGS || p_property.usage & PROPERTY_USAGE_CLASS_IS_BITFIELD;
    }

    /// Checks whether the property is a class enum.
    /// @param p_property the property to check
    /// @return true if the property usage has <code>PROPERTY_USAGE_CLASS_IS_ENUM</code>
    _FORCE_INLINE_ bool is_class_enum(const PropertyInfo& p_property) { return p_property.usage & PROPERTY_USAGE_CLASS_IS_ENUM; }

    /// Checks whether the property is a class bitfield.
    /// @param p_property the property to check
    /// @return true if the property usage has <code>PROPERTY_USAGE_CLASS_IS_BITFIELD</code>
    _FORCE_INLINE_ bool is_class_bitfield(const PropertyInfo& p_property) { return p_property.usage & PROPERTY_USAGE_CLASS_IS_BITFIELD; }

    /// Checks whether the property type is <code>NIL</code> but the variant flag is not set.
    /// @param p_property the property to check
    /// @return true if the property is <code>NIL</code> but has no variant flag set
    bool is_nil_no_variant(const PropertyInfo& p_property);

    /// Returns whether the specified property uses pass-by-reference semantics.
    /// @param p_property the property
    /// @return true if the property is passed by reference, false if its passed by value
    bool is_passed_by_reference(const PropertyInfo& p_property);

    /// Get the type name for the specified property
    /// @param p_property the property
    /// @return the property type name
    String get_property_type_name(const PropertyInfo& p_property);

    /// Get the type name based on variant types only
    /// @param p_property the property
    /// @return the property type name
    String get_variant_type_name(const PropertyInfo& p_property);

    /// Converts a property info's <code>usage</code> bitfield to a string.
    /// @param p_usage the property usage flags bitfield value
    /// @return comma-separated string of property usage flags
    String usage_to_string(uint32_t p_usage);
}

#endif // ORCHESTRATOR_PROPERTY_UTILS_H
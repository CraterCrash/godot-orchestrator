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
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>

using namespace godot;

/// A single annotation applied to a script member, such as a variable or a function.
///
/// The model records only what the user asked for: the annotation name and its literal argument values.
/// Whether the annotation is legal for the member is decided by the OScriptAnnotationRegistry when it
/// is added, and again by the parser at compile time.
///
struct OScriptAnnotation {
    StringName name;    //! Annotation name, including the leading '@'
    Array arguments;    //! Literal argument values, in signature order

    bool operator==(const OScriptAnnotation& p_other) const;
    bool operator!=(const OScriptAnnotation& p_other) const;

    Dictionary to_dict() const;
    static OScriptAnnotation from_dict(const Dictionary& p_dict);

    OScriptAnnotation() = default;
    explicit OScriptAnnotation(const StringName& p_name, const Array& p_arguments = Array());
};

/// The ordered set of annotations applied to a single script member.
///
/// Owners embed this and forward the mutating calls, emitting their own change signals. All rule
/// checks are delegated to the OScriptAnnotationRegistry so that the model, the editor and the
/// parser enforce the same constraints.
///
class OScriptAnnotationList {
    Vector<OScriptAnnotation> _items;

public:
    const Vector<OScriptAnnotation>& get_items() const { return _items; }
    void set_items(const Vector<OScriptAnnotation>& p_items) { _items = p_items; }
    int size() const { return _items.size(); }
    bool is_empty() const { return _items.is_empty(); }
    const OScriptAnnotation& get(int p_index) const { return _items[p_index]; }

    int find(const StringName& p_name) const;
    bool has(const StringName& p_name) const;
    bool has_family(const StringName& p_family) const;

    /// Adds an annotation when the registry allows it for the given target and owner.
    /// @return OK on success, otherwise the registry's error with an optional reason
    Error add(uint32_t p_target, const PropertyInfo& p_owner, const OScriptAnnotation& p_annotation, String* r_reason = nullptr);

    bool remove_at(int p_index);

    /// Removes every annotation in the given family.
    /// @return the number of annotations removed
    int remove_family(const StringName& p_family);

    /// Removes annotations whose type constraint no longer holds for the owner.
    /// Unknown annotation names are kept so the compiler can report them.
    /// @return the number of annotations removed
    int prune(uint32_t p_target, const PropertyInfo& p_owner);

    bool set_arguments(int p_index, const Array& p_arguments);

    Array to_array() const;
    void from_array(const Array& p_array);

    bool operator==(const OScriptAnnotationList& p_other) const;
    bool operator!=(const OScriptAnnotationList& p_other) const;
};
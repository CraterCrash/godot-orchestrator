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

#include "orchestration/annotation.h"

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/templates/hash_map.hpp>

using namespace godot;

/// Describes one annotation the model accepts: its signature, where it may be applied, the family it
/// beglons to for cardinality, and the type constraint on its owner.
///
/// The parser pairs each descriptor with its compile-time apply callback by name; the editor pairs
/// it with presentation metadata. Neither is stored here.
///
struct OScriptAnnotationDescriptor {
    /// Type predicate evaluated against the owner's property information.
    /// The owner is a variable's declared type, or for functions the return value.
    typedef bool (*TypePredicate)(const PropertyInfo& p_owner);

    MethodInfo info;                    //! Signature, including default arguments and the vararg flag
    uint32_t targets = 0;               //! Bitmask of OScriptAnnotationRegistry::Target
    StringName family;                  //! Cardinality group, see OScriptAnnotationRegistry
    bool repeatable = false;            //! Whether more than one of this family may be applied
    Vector<StringName> conflicts;       //! Families this one cannot be combined with, checked symmetrically
    TypePredicate applies_to = nullptr; //! Owner type constraint, null accepts any type

    bool is_vararg() const { return (info.flags & METHOD_FLAG_VARARG) != 0; }
};

/// The single source of annotation rules.
///
/// The model consults it when an annotation is added or an owner's type changes, the editor consults
/// it to filter what can be offered, and the parser registers its apply callbacks  from it. The table
/// holds Variant-typed data, so it is created lazily on first use and  released from
/// unregister_orchestration_types().
///
class OScriptAnnotationRegistry {
    Vector<OScriptAnnotationDescriptor> _descriptors;
    HashMap<StringName, int> _index;

    static OScriptAnnotationRegistry* _instance;

    static OScriptAnnotationRegistry* _get();

    void _add(const OScriptAnnotationDescriptor& p_descriptor);
    void _add_export(const MethodInfo& p_info, OScriptAnnotationDescriptor::TypePredicate p_applies_to, const Vector<Variant>& p_defaults = Vector<Variant>(), bool p_vararg = false);

    void _register_export_annotations();
    void _register_function_annotations();

    OScriptAnnotationRegistry();

public:
    /// Targets an annotation may be applied to. Mapped explicitly by the parser onto its own kinds.
    /// Event is a built-in function override, such as _ready; the parser treats it as a function.
    enum Target {
        TARGET_VARIABLE = 1 << 0,
        TARGET_FUNCTION = 1 << 1,
        TARGET_CLASS = 1 << 2,
        TARGET_EVENT = 1 << 3,
    };

    /// Family names used for cardinality checks.
    static const char* FAMILY_EXPORT;
    static const char* FAMILY_RPC;
    static const char* FAMILY_ONREADY;
    static const char* FAMILY_WARNING;

    static const OScriptAnnotationDescriptor* find(const StringName& p_name);
    static const Vector<OScriptAnnotationDescriptor>& get_descriptors();

    static bool is_family(const StringName& p_name, const StringName& p_family);

    /// Whether the descriptor's target and type constraint accept the owner.
    static bool applies_to(const OScriptAnnotationDescriptor& p_descriptor, uint32_t p_target, const PropertyInfo& p_owner);

    /// Whether the named annotation's target and type constraint accept the owner.
    /// Unknown names return false.
    static bool applies_to(const StringName& p_name, uint32_t p_target, const PropertyInfo& p_owner);

    /// The applied annotation the named one cannot be combined with, if any.
    /// @return the conflicting annotation's name, or an empty name when there is no conflict
    static StringName find_conflict(const StringName& p_name, const Vector<OScriptAnnotation>& p_existing);

    /// The one rule entry point. Checks that the annotation exists, targets the owner kind,
    /// accepts the owner's type, and does not violate its family's cardinality.
    /// @return OK when the annotation may be added, otherwise an error with an optional reason
    static Error can_add(uint32_t p_target, const PropertyInfo& p_owner, const Vector<OScriptAnnotation>& p_existing, const StringName& p_name, String* r_reason = nullptr);

    /// Releases the lazily created table.
    static void cleanup();
};
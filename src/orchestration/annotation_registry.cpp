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
#include "orchestration/annotation_registry.h"

#include "common/property_utils.h"
#include "script/script_server.h"

#include <godot_cpp/core/class_db.hpp>

OScriptAnnotationRegistry* OScriptAnnotationRegistry::_instance = nullptr;

const char* OScriptAnnotationRegistry::FAMILY_EXPORT = "export";
const char* OScriptAnnotationRegistry::FAMILY_RPC = "rpc";
const char* OScriptAnnotationRegistry::FAMILY_ONREADY = "onready";
const char* OScriptAnnotationRegistry::FAMILY_WARNING = "warning";

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Type predicates
///
/// Each predicate sees the owner's declared PropertyInfo. Typed arrays and packed arrays are reduced to their
/// element type, mirroring how the parser validates export annotations. A Variant-typed owner is accepted
/// everywhere, as the parser infers from the initializer.

static Variant::Type _effective_type(const PropertyInfo& p_owner) {
    switch (p_owner.type) {
        case Variant::ARRAY: {
            if (p_owner.hint == PROPERTY_HINT_ARRAY_TYPE && !p_owner.hint_string.is_empty()) {
                Variant::Type builtin = Variant::NIL;
                StringName class_name;
                Variant script;
                PropertyUtils::get_element_type(p_owner.hint_string, builtin, class_name, script);
                return builtin;
            }
            return Variant::ARRAY;
        }
        case Variant::PACKED_BYTE_ARRAY:
        case Variant::PACKED_INT32_ARRAY:
        case Variant::PACKED_INT64_ARRAY:
            return Variant::INT;
        case Variant::PACKED_FLOAT32_ARRAY:
        case Variant::PACKED_FLOAT64_ARRAY:
            return Variant::FLOAT;
        case Variant::PACKED_STRING_ARRAY:
            return Variant::STRING;
        case Variant::PACKED_VECTOR2_ARRAY:
            return Variant::VECTOR2;
        case Variant::PACKED_VECTOR3_ARRAY:
            return Variant::VECTOR3;
        case Variant::PACKED_VECTOR4_ARRAY:
            return Variant::VECTOR4;
        case Variant::PACKED_COLOR_ARRAY:
            return Variant::COLOR;
        default:
            return p_owner.type;
    }
}

static bool _is_variant(const PropertyInfo& p_owner) {
    return PropertyUtils::is_variant(p_owner);
}

static bool _is_one_of(const PropertyInfo& p_owner, std::initializer_list<Variant::Type> p_types) {
    if (_is_variant(p_owner)) {
        return true;
    }

    const Variant::Type type = _effective_type(p_owner);
    for (const Variant::Type candidate : p_types) {
        if (type == candidate) {
            return true;
        }
    }
    return false;
}

static bool _is_node_or_resource(const StringName& p_class) {
    StringName native_class = p_class;
    if (ScriptServer::is_global_class(p_class)) {
        native_class = ScriptServer::get_global_class_native_base(p_class);
    }
    return ClassDB::is_parent_class(native_class, "Node") || ClassDB::is_parent_class(native_class, "Resource");
}

static bool _accepts_export(const PropertyInfo& p_owner) {
    if (_is_variant(p_owner)) {
        return true;
    }

    switch (p_owner.type) {
        case Variant::CALLABLE:
        case Variant::SIGNAL:
        case Variant::RID:
            return false;

        case Variant::OBJECT: {
            if (!p_owner.class_name.is_empty()) {
                return _is_node_or_resource(p_owner.class_name);
            }
            if (p_owner.hint_string.is_empty()) {
                return false;
            }
            return _is_node_or_resource(p_owner.hint_string);
        }

        default:
            return true;
    }
}

static bool _accepts_int(const PropertyInfo& p_owner) {
    return _is_one_of(p_owner, { Variant::INT });
}

static bool _accepts_int_or_float(const PropertyInfo& p_owner) {
    return _is_one_of(p_owner, { Variant::INT, Variant::FLOAT });
}

static bool _accepts_int_or_string(const PropertyInfo& p_owner) {
    return _is_one_of(p_owner, { Variant::INT, Variant::STRING });
}

static bool _accepts_float(const PropertyInfo& p_owner) {
    return _is_one_of(p_owner, { Variant::FLOAT });
}

static bool _accepts_string(const PropertyInfo& p_owner) {
    return _is_one_of(p_owner, { Variant::STRING });
}

static bool _accepts_string_or_dictionary(const PropertyInfo& p_owner) {
    return _is_one_of(p_owner, { Variant::STRING, Variant::DICTIONARY });
}

static bool _accepts_color(const PropertyInfo& p_owner) {
    return _is_one_of(p_owner, { Variant::COLOR });
}

static bool _accepts_node_path(const PropertyInfo& p_owner) {
    return _is_one_of(p_owner, { Variant::NODE_PATH });
}

static bool _accepts_callable(const PropertyInfo& p_owner) {
    return _is_one_of(p_owner, { Variant::CALLABLE });
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OScriptAnnotationRegistry

OScriptAnnotationRegistry* OScriptAnnotationRegistry::_get() {
    if (_instance == nullptr) {
        _instance = memnew(OScriptAnnotationRegistry);
    }
    return _instance;
}

void OScriptAnnotationRegistry::_add(const OScriptAnnotationDescriptor& p_descriptor) {
    ERR_FAIL_COND_MSG(_index.has(p_descriptor.info.name), vformat(R"(Annotation "%s" already registered.)", p_descriptor.info.name));

    _index[p_descriptor.info.name] = _descriptors.size();
    _descriptors.push_back(p_descriptor);
}

void OScriptAnnotationRegistry::_add_export(const MethodInfo& p_info, OScriptAnnotationDescriptor::TypePredicate p_applies_to, const Vector<Variant>& p_defaults, bool p_vararg) {
    OScriptAnnotationDescriptor descriptor;
    descriptor.info = p_info;
    for (const Variant& item : p_defaults) {
        descriptor.info.default_arguments.push_back(item);
    }
    if (p_vararg) {
        descriptor.info.flags |= METHOD_FLAG_VARARG;
    }
    descriptor.targets = TARGET_VARIABLE;
    descriptor.family = FAMILY_EXPORT;
    descriptor.repeatable = false;
    descriptor.applies_to = p_applies_to;
    _add(descriptor);
}

void OScriptAnnotationRegistry::_register_export_annotations() {
    // Mirrors the GDScript parser's export annotation table, in the same order.
    _add_export(MethodInfo("@export"), _accepts_export);
    _add_export(MethodInfo("@export_enum", PropertyInfo(Variant::STRING, "names")), _accepts_int_or_string, {}, true);
    _add_export(MethodInfo("@export_file", PropertyInfo(Variant::STRING, "filter")), _accepts_string, { "" }, true);
    _add_export(MethodInfo("@export_dir"), _accepts_string);
    _add_export(MethodInfo("@export_global_file", PropertyInfo(Variant::STRING, "filter")), _accepts_string, { "" }, true);
    _add_export(MethodInfo("@export_global_dir"), _accepts_string);
    _add_export(MethodInfo("@export_multiline"), _accepts_string_or_dictionary);
    _add_export(MethodInfo("@export_placeholder", PropertyInfo(Variant::STRING, "placeholder")), _accepts_string);
    _add_export(MethodInfo("@export_range", PropertyInfo(Variant::FLOAT, "min"), PropertyInfo(Variant::FLOAT, "max"), PropertyInfo(Variant::FLOAT, "step"), PropertyInfo(Variant::STRING, "extra_hints")), _accepts_int_or_float, { 1.0, "" }, true);
    _add_export(MethodInfo("@export_exp_easing", PropertyInfo(Variant::STRING, "hints")), _accepts_float, { "" }, true);
    _add_export(MethodInfo("@export_color_no_alpha"), _accepts_color);
    _add_export(MethodInfo("@export_node_path", PropertyInfo(Variant::STRING, "type")), _accepts_node_path, { "" }, true);
    _add_export(MethodInfo("@export_flags", PropertyInfo(Variant::STRING, "names")), _accepts_int, {}, true);
    _add_export(MethodInfo("@export_flags_2d_render"), _accepts_int);
    _add_export(MethodInfo("@export_flags_2d_physics"), _accepts_int);
    _add_export(MethodInfo("@export_flags_2d_navigation"), _accepts_int);
    _add_export(MethodInfo("@export_flags_3d_render"), _accepts_int);
    _add_export(MethodInfo("@export_flags_3d_physics"), _accepts_int);
    _add_export(MethodInfo("@export_flags_3d_navigation"), _accepts_int);
    _add_export(MethodInfo("@export_flags_avoidance"), _accepts_int);
    _add_export(MethodInfo("@export_storage"), nullptr);
    _add_export(MethodInfo("@export_custom", PropertyInfo(Variant::INT, "hint", PROPERTY_HINT_ENUM, ""), PropertyInfo(Variant::STRING, "hint_string"), PropertyInfo(Variant::INT, "usage", PROPERTY_HINT_FLAGS, "")), nullptr, { PROPERTY_USAGE_DEFAULT });
    _add_export(MethodInfo("@export_tool_button", PropertyInfo(Variant::STRING, "text"), PropertyInfo(Variant::STRING, "icon")), _accepts_callable, { "" });
}

void OScriptAnnotationRegistry::_register_function_annotations() {
    // GDScript accepts the @rpc tokens in any order; the model stores them positionally so the
    // editor can render a fixed form. The parser counts by category, so order does not matter to it.
    OScriptAnnotationDescriptor rpc;
    rpc.info = MethodInfo("@rpc", PropertyInfo(Variant::STRING, "mode"), PropertyInfo(Variant::STRING, "sync"), PropertyInfo(Variant::STRING, "transfer_mode"), PropertyInfo(Variant::INT, "transfer_channel"));
    rpc.info.default_arguments.push_back("authority");
    rpc.info.default_arguments.push_back("call_remote");
    rpc.info.default_arguments.push_back("unreliable");
    rpc.info.default_arguments.push_back(0);
    rpc.targets = TARGET_FUNCTION;
    rpc.family = FAMILY_RPC;
    _add(rpc);

    // Any variable may carry @onready; whether the initializer needs it is the compiler's call.
    // It never combines with an export: the ready-time assignment would overwrite the exported value,
    // which the compiler also reports for hand-written files.
    OScriptAnnotationDescriptor onready;
    onready.info = MethodInfo("@onready");
    onready.targets = TARGET_VARIABLE;
    onready.family = FAMILY_ONREADY;
    onready.conflicts.push_back(FAMILY_EXPORT);
    _add(onready);

    OScriptAnnotationDescriptor warning_ignore;
    warning_ignore.info = MethodInfo("@warning_ignore", PropertyInfo(Variant::STRING, "warning"));
    warning_ignore.info.flags |= METHOD_FLAG_VARARG;
    warning_ignore.targets = TARGET_VARIABLE | TARGET_FUNCTION | TARGET_EVENT;
    warning_ignore.family = FAMILY_WARNING;
    warning_ignore.repeatable = true;
    _add(warning_ignore);
}

OScriptAnnotationRegistry::OScriptAnnotationRegistry() {
    _register_export_annotations();
    _register_function_annotations();
}

const OScriptAnnotationDescriptor* OScriptAnnotationRegistry::find(const StringName& p_name) {
    const OScriptAnnotationRegistry* registry = _get();
    const int* index = registry->_index.getptr(p_name);
    if (index == nullptr) {
        return nullptr;
    }
    return &registry->_descriptors[*index];
}

const Vector<OScriptAnnotationDescriptor>& OScriptAnnotationRegistry::get_descriptors() {
    return _get()->_descriptors;
}

bool OScriptAnnotationRegistry::is_family(const StringName& p_name, const StringName& p_family) {
    const OScriptAnnotationDescriptor* descriptor = find(p_name);
    return descriptor != nullptr && descriptor->family == p_family;
}

bool OScriptAnnotationRegistry::applies_to(const OScriptAnnotationDescriptor& p_descriptor, uint32_t p_target, const PropertyInfo& p_owner) {
    if ((p_descriptor.targets & p_target) == 0) {
        return false;
    }
    if (p_descriptor.applies_to != nullptr && !p_descriptor.applies_to(p_owner)) {
        return false;
    }
    return true;
}

bool OScriptAnnotationRegistry::applies_to(const StringName& p_name, uint32_t p_target, const PropertyInfo& p_owner) {
    const OScriptAnnotationDescriptor* descriptor = find(p_name);
    return descriptor != nullptr && applies_to(*descriptor, p_target, p_owner);
}

StringName OScriptAnnotationRegistry::find_conflict(const StringName& p_name, const Vector<OScriptAnnotation>& p_existing) {
    const OScriptAnnotationDescriptor* descriptor = find(p_name);
    if (descriptor == nullptr) {
        return StringName();
    }

    for (const OScriptAnnotation& existing : p_existing) {
        const OScriptAnnotationDescriptor* other = find(existing.name);
        if (other == nullptr) {
            continue;
        }
        if (descriptor->conflicts.has(other->family) || other->conflicts.has(descriptor->family)) {
            return existing.name;
        }
    }
    return StringName();
}

Error OScriptAnnotationRegistry::can_add(uint32_t p_target, const PropertyInfo& p_owner, const Vector<OScriptAnnotation>& p_existing, const StringName& p_name, String* r_reason) {
    const OScriptAnnotationDescriptor* descriptor = find(p_name);
    if (descriptor == nullptr) {
        if (r_reason) {
            *r_reason = vformat(R"(Unknown annotation "%s".)", p_name);
        }
        return ERR_DOES_NOT_EXIST;
    }

    if ((descriptor->targets & p_target) == 0) {
        if (r_reason) {
            *r_reason = vformat(R"(Annotation "%s" cannot be applied here.)", p_name);
        }
        return ERR_INVALID_PARAMETER;
    }

    if (descriptor->applies_to != nullptr && !descriptor->applies_to(p_owner)) {
        if (r_reason) {
            *r_reason = vformat(R"(Annotation "%s" cannot be applied to type "%s".)", p_name, PropertyUtils::get_property_type_name(p_owner));
        }
        return ERR_INVALID_DATA;
    }

    if (!descriptor->repeatable) {
        for (const OScriptAnnotation& existing : p_existing) {
            if (is_family(existing.name, descriptor->family)) {
                if (r_reason) {
                    *r_reason = vformat(R"(Annotation "%s" cannot be used with "%s".)", p_name, existing.name);
                }
                return ERR_ALREADY_EXISTS;
            }
        }
    }

    const StringName conflict = find_conflict(p_name, p_existing);
    if (!conflict.is_empty()) {
        if (r_reason) {
            *r_reason = vformat(R"(Annotation "%s" cannot be combined with "%s".)", p_name, conflict);
        }
        return ERR_INVALID_DATA;
    }

    return OK;
}

void OScriptAnnotationRegistry::cleanup() {
    if (_instance != nullptr) {
        memdelete(_instance);
        _instance = nullptr;
    }
}
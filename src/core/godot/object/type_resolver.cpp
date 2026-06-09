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
#include "core/godot/object/type_resolver.h"

#include "api/extension_db.h"
#include "common/variant_utils.h"
#include "core/godot/io/resource_uid.h"
#include "script/script_server.h"

#include <godot_cpp/core/class_db.hpp>

GDE::GodotType GDE::TypeResolver::resolve(const String& p_type_name) {
    GodotType info;

    if (p_type_name.is_empty() || p_type_name == "Variant") {
        info.kind = GodotType::VARIANT;
        info.name = "Variant";
        info.icon = "Variant";
        return info;
    }

    if (ClassDB::class_exists(p_type_name)) {
        info.kind = GodotType::CLASS;
        info.type = Variant::Type::OBJECT;
        info.name = p_type_name;
        info.icon = p_type_name;
        return info;
    }

    if (ScriptServer::is_global_class(p_type_name)) {
        info.kind = GodotType::CLASS;
        info.type = Variant::Type::OBJECT;
        info.name = p_type_name;

        ScriptServer::GlobalClass global = ScriptServer::get_global_class(p_type_name);
        if (!global.icon_path.is_empty()) {
            if (global.icon_path.begins_with("uid://")) {
                info.icon = ResourceUID::uid_to_path(global.icon_path);
            } else {
                info.icon = global.icon_path;
            }
        } else {
            info.icon = global.language;
        }

        return info;
    }

    if (p_type_name.contains(".")) {
        const int64_t dot = p_type_name.find(".");
        const String class_name = p_type_name.substr(0, dot);
        const String class_sub_name = p_type_name.substr(dot + 1);

        if (ClassDB::class_exists(class_name)) {
            info.type = Variant::Type::INT;
            info.name = p_type_name;
            info.icon = "Enum";

            if (ClassDB::is_class_enum_bitfield(class_name, class_sub_name)) {
                info.kind = GodotType::BITFIELD;
            } else {
                info.kind = GodotType::ENUM;
            }

            return info;
        }

        if (ScriptServer::is_global_class(class_name)) {
            info.type = Variant::Type::INT;
            info.name = p_type_name;
            info.kind = GodotType::ENUM;
            info.icon = "Enum";
            return info;
        }

        info.kind = GodotType::UNKNOWN;
        return info;
    }

    if (ExtensionDB::get_global_enum_names().has(p_type_name)) {
        const EnumInfo ei = ExtensionDB::get_global_enum(p_type_name);

        info.type = Variant::Type::INT;
        info.name = p_type_name;
        info.kind = ei.is_bitfield ? GodotType::BITFIELD : GodotType::ENUM;
        info.icon = "Enum";

        return info;
    }

    for (int i = 0; i < Variant::VARIANT_MAX; i++) {
        const Variant::Type type = VariantUtils::to_type(i);
        const String type_name = Variant::get_type_name(type);
        if (p_type_name == type_name) {
            info.type = type;
            info.name = type_name;
            info.kind = GodotType::BUILTIN;
            info.icon = type == Variant::NIL ? "Variant" : type_name;
            return info;
        }
    }

    info.kind = GodotType::UNKNOWN;
    info.name = "<Unknonw>";
    return info;
}

Vector<GDE::GodotType> GDE::TypeResolver::resolve(const PropertyInfo& p_property) {
    Vector<GodotType> resolved_types;

    if (p_property.type == Variant::ARRAY) {
        GodotType container;
        container.kind = GodotType::BUILTIN;
        container.type = Variant::ARRAY;
        container.name = Variant::get_type_name(container.type);
        container.icon = container.name;
        resolved_types.push_back(container);

        if (p_property.hint == PROPERTY_HINT_ARRAY_TYPE) {
            if (!p_property.hint_string.is_empty()) {
                resolved_types.push_back(resolve(p_property.hint_string));
            }
        }
    } else if (p_property.type == Variant::DICTIONARY) {
        GodotType container;
        container.kind = GodotType::BUILTIN;
        container.type = Variant::DICTIONARY;
        container.name = Variant::get_type_name(container.type);
        container.icon = container.name;
        resolved_types.push_back(container);

        if (p_property.hint == PROPERTY_HINT_DICTIONARY_TYPE) {
            if (!p_property.hint_string.is_empty()) {
                const PackedStringArray parts = p_property.hint_string.split(";");
                resolved_types.push_back(resolve(parts.size() >= 1 ? parts[0] : ""));
                resolved_types.push_back(resolve(parts.size() >= 2 ? parts[1] : ""));
            }
        }
    } else if (p_property.type == Variant::OBJECT) {
        resolved_types.push_back(resolve(p_property.class_name));
    } else if (p_property.type == Variant::INT &&
            p_property.usage & (PROPERTY_USAGE_CLASS_IS_BITFIELD | PROPERTY_USAGE_CLASS_IS_ENUM)) {
        resolved_types.push_back(resolve(p_property.class_name));
    } else {
        if (p_property.type == Variant::NIL) {// && p_property.usage & PROPERTY_USAGE_NIL_IS_VARIANT) {
            resolved_types.push_back(resolve(""));
        } else {
            resolved_types.push_back(resolve(Variant::get_type_name(p_property.type)));
        }
    }

    return resolved_types;
}
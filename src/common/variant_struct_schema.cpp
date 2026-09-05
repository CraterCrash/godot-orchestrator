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
#include "common/variant_struct_schema.h"

#include "api/extension_db.h"
#include "common/variant_utils.h"
#include "core/godot/variant/variant.h"

#include <godot_cpp/templates/hash_map.hpp>

namespace VariantStructSchema {

    namespace {
        struct Schema {
            HashMap<Variant::Type, Vector<PropertyInfo>> components;
            HashMap<Variant::Type, Vector<PropertyInfo>> properties;
        };

        /// Returns whether a readable property participates in the type's constructor.
        /// Derived members are readable but are not constructor arguments.
        bool _is_component(Variant::Type p_type, const String& p_name) {
            switch (p_type) {
                case Variant::RECT2:
                case Variant::RECT2I:
                case Variant::AABB: {
                    // Constructed from position and size; end is derived
                    return p_name != "end";
                }
                case Variant::PLANE: {
                    // Constructed from x, y, z and d; normal is derived
                    return p_name != "normal";
                }
                case Variant::COLOR: {
                    // Constructed from r, g, b and a; 8-bit, HSV and OK HSL members are derived
                    return p_name == "r" || p_name == "g" || p_name == "b" || p_name == "a";
                }
                default: {
                    return true;
                }
            }
        }

        const Schema& _get_schema() {
            static Schema schema;
            static bool built = false;
            if (!built) {
                for (const BuiltInType& type : ExtensionDB::get_builtin_types()) {
                    if (type.properties.is_empty()) {
                        continue;
                    }

                    schema.properties[type.type] = type.properties;

                    Vector<PropertyInfo> components;
                    for (const PropertyInfo& property : type.properties) {
                        if (_is_component(type.type, property.name)) {
                            components.push_back(property);
                        }
                    }
                    schema.components[type.type] = components;
                }
                built = true;
            }
            return schema;
        }

        const Vector<PropertyInfo>& _empty() {
            static const Vector<PropertyInfo> empty;
            return empty;
        }

        PackedStringArray _names_of(const Vector<PropertyInfo>& p_properties) {
            PackedStringArray names;
            for (const PropertyInfo& property : p_properties) {
                names.push_back(property.name);
            }
            return names;
        }
    }

    bool is_composite(Variant::Type p_type) {
        return _get_schema().components.has(p_type);
    }

    const Vector<PropertyInfo>& get_components(Variant::Type p_type) {
        const Vector<PropertyInfo>* components = _get_schema().components.getptr(p_type);
        return components ? *components : _empty();
    }

    PackedStringArray get_component_names(Variant::Type p_type) {
        return _names_of(get_components(p_type));
    }

    const Vector<PropertyInfo>& get_properties(Variant::Type p_type) {
        const Vector<PropertyInfo>* properties = _get_schema().properties.getptr(p_type);
        return properties ? *properties : _empty();
    }

    PackedStringArray get_property_names(Variant::Type p_type) {
        return _names_of(get_properties(p_type));
    }

    PackedStringArray get_component_paths(Variant::Type p_type) {
        PackedStringArray paths;
        for (const PropertyInfo& component : get_components(p_type)) {
            const PackedStringArray sub_paths = get_component_paths(component.type);
            if (sub_paths.is_empty()) {
                paths.push_back(component.name);
                continue;
            }

            for (const String& sub_path : sub_paths) {
                paths.push_back(vformat("%s.%s", component.name, sub_path));
            }
        }
        return paths;
    }

    Variant compose(Variant::Type p_type, const Vector<Variant>& p_components, bool* r_valid) {
        Variant value;

        Vector<const Variant*> arguments;
        arguments.resize(p_components.size());
        for (int i = 0; i < p_components.size(); i++) {
            arguments.write[i] = &p_components[i];
        }

        const GDExtensionCallError error = GDE::Variant::construct(p_type, value, arguments.ptrw(), arguments.size());
        const bool valid = error.error == GDEXTENSION_CALL_OK;
        if (r_valid) {
            *r_valid = valid;
        }

        return valid ? value : VariantUtils::make_default(p_type);
    }

    bool verify() {
        bool all_valid = true;

        for (const KeyValue<Variant::Type, Vector<PropertyInfo>>& E : _get_schema().components) {
            const Variant::Type type = E.key;
            const Variant expected = VariantUtils::make_default(type);

            Vector<Variant> components;
            for (const PropertyInfo& component : E.value) {
                const Variant part = expected.get(component.name);
                if (part.get_type() != component.type) {
                    ERR_PRINT(vformat("Struct schema: %s.%s is declared as %s but reads as %s",
                        Variant::get_type_name(type), component.name,
                        Variant::get_type_name(component.type), Variant::get_type_name(part.get_type())));
                    all_valid = false;
                }
                components.push_back(part);
            }

            bool valid = false;
            const Variant actual = compose(type, components, &valid);
            if (!valid || actual != expected) {
                ERR_PRINT(vformat("Struct schema: %s cannot be reconstructed from (%s)",
                    Variant::get_type_name(type), String(", ").join(get_component_names(type))));
                all_valid = false;
            }
        }

        return all_valid;
    }
}
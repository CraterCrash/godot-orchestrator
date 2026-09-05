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
#include "orchestration/nodes/compose.h"

#include "common/dictionary_utils.h"
#include "common/property_utils.h"
#include "common/scene_utils.h"
#include "common/string_utils.h"
#include "common/variant_struct_schema.h"
#include "common/variant_utils.h"

void OScriptNodeCompose::post_initialize() {
    // Clone this from the output pin
    _type = find_pin("value", PD_Output)->get_type();
    super::post_initialize();
}

void OScriptNodeCompose::allocate_default_pins() {
    for (const PropertyInfo& component : VariantStructSchema::get_components(_type)) {
        create_pin(PD_Input, PT_Data, PropertyUtils::make_typed(component.name, component.type));
    }

    // This is the pin that will be constructed from its types
    create_pin(PD_Output, PT_Data, PropertyUtils::make_typed("value", _type));
}

String OScriptNodeCompose::get_tooltip_text() const {
    if (_type != Variant::NIL) {
        const String type_name = VariantUtils::get_friendly_type_name(_type);
        const String components = StringUtils::join(", ", VariantStructSchema::get_component_names(_type));
        return vformat("Make a %s from %s", type_name, components);
    }
    return "Construct a Godot built-in type, optionally from its sub-components.";
}

String OScriptNodeCompose::get_node_title() const {
    return "Make " + VariantUtils::get_friendly_type_name(_type);
}

String OScriptNodeCompose::get_icon() const {
    return SceneUtils::get_icon_path("Compose");
}

void OScriptNodeCompose::configure(const OScriptNodeInitContext& p_context) {
    super::configure(p_context);

    const Dictionary data = p_context.user_data.value_or(Dictionary());

    _type = VariantUtils::to_type(data.get("type", Variant::NIL));
}

void OScriptNodeCompose::initialize(const OScriptNodeInitContext& p_context) {
    ERR_FAIL_COND_MSG(!p_context.user_data, "A Compose node requires custom data");
    ERR_FAIL_COND_MSG(!p_context.user_data.value().has("type"), "Cannot properly initialize compose node, no type specified.");

    super::initialize(p_context);
}

bool OScriptNodeCompose::is_supported(Variant::Type p_type) {
    // Composite types are handled by OScriptNodeComposeFrom
    return !VariantStructSchema::is_composite(p_type);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OScriptNodeComposeFrom

void OScriptNodeComposeFrom::post_initialize() {
    // Clone this from the output pin
    _type = find_pin("value", PD_Output)->get_type();

    for (const Ref<OScriptNodePin>& pin : find_pins(PD_Input)) {
        if (!pin->is_execution()) {
            _constructor_args.push_back(PropertyInfo(pin->get_type(), pin->get_pin_name()));
        }
    }

    super::post_initialize();
}

void OScriptNodeComposeFrom::allocate_default_pins() {
    for (int i = 0; i < _constructor_args.size(); i++) {
        const PropertyInfo& property = _constructor_args[i];
        if (property.name.is_empty()) {
            create_pin(PD_Input, PT_Data, PropertyUtils::as("arg" + itos(i), property));
        } else {
            create_pin(PD_Input, PT_Data, property);
        }
    }

    // This is the pin that will be constructed from its types
    create_pin(PD_Output, PT_Data, PropertyUtils::make_typed("value", _type));
}

String OScriptNodeComposeFrom::get_tooltip_text() const {
    if (_type != Variant::NIL) {
        const String type_name = VariantUtils::get_friendly_type_name(_type);

        PackedStringArray component_names;
        for (const PropertyInfo& property : _constructor_args) {
            component_names.push_back(VariantUtils::get_friendly_type_name(property.type));
        }

        const String components = StringUtils::join(" and ", component_names);
        return vformat("Construct a %s from %s", type_name, components);
    }
    return "Construct a Godot built-in type, optionally from its sub-components.";
}

String OScriptNodeComposeFrom::get_node_title() const {
    return "Make " + VariantUtils::get_friendly_type_name(_type);
}

String OScriptNodeComposeFrom::get_icon() const {
    return SceneUtils::get_icon_path("Compose");
}

String OScriptNodeComposeFrom::get_help_topic() const {
    return vformat("class:%s", Variant::get_type_name(_type));
}

PackedStringArray OScriptNodeComposeFrom::get_keywords() const {
    return Array::make("combine", "compose", "create", "make", Variant::get_type_name(_type));
}

void OScriptNodeComposeFrom::configure(const OScriptNodeInitContext& p_context) {
    super::configure(p_context);

    const Dictionary data = p_context.user_data.value_or(Dictionary());

    _type = VariantUtils::to_type(data.get("type", Variant::NIL));

    _constructor_args.clear();
    if (data.has("constructor_args")) {
        const Array constructor_types = data["constructor_args"];
        for (int i = 0; i < constructor_types.size(); i++) {
            const PropertyInfo pi = DictionaryUtils::to_property(constructor_types[i]);
            _constructor_args.push_back(pi);
        }
    }
}

void OScriptNodeComposeFrom::initialize(const OScriptNodeInitContext& p_context) {
    ERR_FAIL_COND_MSG(!p_context.user_data, "A ComposeFrom node requires custom data");
    ERR_FAIL_COND_MSG(!p_context.user_data.value().has("type"), "Cannot properly initialize compose from node, no type specified.");

    super::initialize(p_context);
}

bool OScriptNodeComposeFrom::is_supported(Variant::Type p_type, const Vector<PropertyInfo>& p_args) {
    switch (p_type) {
        case Variant::NIL: // Unnecessary
        case Variant::ARRAY: // Makes use of custom MakeArray
        case Variant::DICTIONARY: // Makes use of custom MakeDictionary
        case Variant::RID: // Not necessary
        case Variant::SIGNAL: // Not necessary
            return false;
        case Variant::PACKED_BYTE_ARRAY:
        case Variant::PACKED_STRING_ARRAY:
        case Variant::PACKED_COLOR_ARRAY:
        case Variant::PACKED_FLOAT32_ARRAY:
        case Variant::PACKED_FLOAT64_ARRAY:
        case Variant::PACKED_INT32_ARRAY:
        case Variant::PACKED_INT64_ARRAY:
        case Variant::PACKED_VECTOR2_ARRAY:
        case Variant::PACKED_VECTOR3_ARRAY:
        case Variant::AABB:
        case Variant::BASIS:
        case Variant::CALLABLE:
        case Variant::PLANE:
        case Variant::TRANSFORM2D:
            // Single argument constructors with same types, ignore them.
            if (p_args.size() == 1 && p_args[0].type == p_type) {
                return false;
            }
            break;
        case Variant::TRANSFORM3D:
            // Single argument constructors with same types, ignore them.
            if (p_args.size() == 1 && (p_args[0].type == p_type || p_args[0].type == Variant::PROJECTION)) {
                return false;
            }
            if (p_args.size() == 2 && p_args[0].type == Variant::BASIS && p_args[1].type == Variant::VECTOR3) {
                return false;
            }
            break;
        case Variant::RECT2:
            // Single argument constructors with same types, ignore them.
            if (p_args.size() == 1 && (p_args[0].type == p_type || p_args[0].type == Variant::RECT2I)) {
                return false;
            }
            break;
        case Variant::RECT2I:
            // Single argument constructors with same types, ignore them.
            if (p_args.size() == 1 && (p_args[0].type == p_type || p_args[0].type == Variant::RECT2)) {
                return false;
            }
            break;
        case Variant::PROJECTION:
            // Single argument constructors with same types, ignore them.
            if (p_args.size() == 1 && (p_args[0].type == p_type || p_args[0].type == Variant::TRANSFORM3D)) {
                return false;
            }
            break;
        case Variant::QUATERNION:
            if (p_args.size() == 1 && p_args[0].type == Variant::BASIS) {
                return false;
            }
            break;
        default:
            break;
    }

    return true;
}
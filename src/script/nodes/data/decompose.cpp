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
#include "script/nodes/data/decompose.h"

#include "api/extension_db.h"
#include "common/property_utils.h"
#include "common/scene_utils.h"
#include "common/string_utils.h"
#include "common/variant_utils.h"

OScriptNodeDecompose::TypeMap OScriptNodeDecompose::_type_components;

PackedStringArray OScriptNodeDecompose::_get_components() const {
    PackedStringArray results;

    const Array &components = _type_components[_type];
    int64_t index_start = 0;
    int64_t index_end = components.size();
    if (_type == Variant::COLOR) {
        switch (_sub_type) {
            case ST_NONE:
            case ST_COLOR_RGBA: {
                index_start = 0;
                index_end = 4;
                break;
            }
            case ST_COLOR_RGBA8: {
                index_start = 4;
                index_end = 8;
                break;
            }
            case ST_COLOR_HSV: {
                index_start = 8;
                index_end = 11;
                break;
            }
            case ST_COLOR_OK_HSL: {
                index_start = 11;
                index_end = components.size();
                break;
            }
            default: {
                // no-op
                break;
            }
        }
    }

    for (int64_t i = index_start; i < index_end; i++) {
        results.push_back(components[i]);
    }

    return results;
}

void OScriptNodeDecompose::post_initialize() {
    // Clone this from the input pin
    _type = find_pin("value", PD_Input)->get_type();
    super::post_initialize();
}

void OScriptNodeDecompose::allocate_default_pins() {
    // Set the pin with value that will be broken
    create_pin(PD_Input, PT_Data, PropertyUtils::make_typed("value", _type))->set_flag(OScriptNodePin::Flags::IGNORE_DEFAULT);

    Variant value = VariantUtils::make_default(_type);

    for (const String& component : _get_components()) {
        const Variant bit = value.get(component);
        const Ref<OScriptNodePin> pin = create_pin(PD_Output, PT_Data, PropertyUtils::make_typed(component, bit.get_type()));
        if (_type == Variant::COLOR) {
            pin->set_flag(OScriptNodePin::NO_CAPITALIZE);
        }
    }
}

String OScriptNodeDecompose::get_tooltip_text() const {
    if (_type != Variant::NIL) {
        const String type_name = VariantUtils::get_friendly_type_name(_type);
        const String components = StringUtils::join(", ", _get_components());
        return vformat("Break a %s into %s", type_name, components);
    }
    return "Breaks a complex structure into its components";
}

String OScriptNodeDecompose::get_node_title() const {
    return "Break " + VariantUtils::get_friendly_type_name(_type);
}

String OScriptNodeDecompose::get_icon() const {
    return SceneUtils::get_icon_path("Decompose");
}

String OScriptNodeDecompose::get_help_topic() const {
    return vformat("class:%s", Variant::get_type_name(_type));
}

PackedStringArray OScriptNodeDecompose::get_keywords() const {
    return Array::make("break", "split", "separate", "decompose", Variant::get_type_name(_type));
}

void OScriptNodeDecompose::initialize(const OScriptNodeInitContext& p_context) {
    ERR_FAIL_COND_MSG(!p_context.user_data, "A Decompose node requires custom data");

    const Dictionary& data = p_context.user_data.value();
    ERR_FAIL_COND_MSG(!data.has("type"), "Cannot properly initialize decompose node, no type specified.");

    _type = VariantUtils::to_type(data["type"]);
    _sub_type = static_cast<SubType>(static_cast<int32_t>(data.get("sub_type", ST_NONE)));

    super::initialize(p_context);
}

void OScriptNodeDecompose::_bind_methods() {
    // Populate the type components
    for (const BuiltInType& type : ExtensionDB::get_builtin_types()) {
        if (!type.properties.is_empty()) {
            Array properties;
            for (const PropertyInfo& pi : type.properties) {
                properties.push_back(pi.name);
            }
            _type_components[type.type] = properties;
        }
    }

    ClassDB::bind_method(D_METHOD("_set_sub_type", "type"), &OScriptNodeDecompose::_set_sub_type);
    ClassDB::bind_method(D_METHOD("_get_sub_type"), &OScriptNodeDecompose::_get_sub_type);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "sub_type", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "_set_sub_type", "_get_sub_type");
}
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
#include "decompose.h"

#include "api/extension_db.h"
#include "common/property_utils.h"
#include "common/string_utils.h"
#include "common/variant_utils.h"

OScriptNodeDecompose::TypeMap OScriptNodeDecompose::_type_components;

class OScriptNodeDecomposeInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeDecompose);
    Array _components;

public:
    int step(OScriptExecutionContext& p_context) override
    {
        Variant& value = p_context.get_input(0);
        for (int i = 0; i < _components.size(); i++)
        {
            Variant component_value = value.get(_components[i]);
            p_context.set_output(i, component_value);
        }
        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeDecompose::_bind_methods()
{
    // Populate the type components
    for (const String& type_name : ExtensionDB::get_builtin_type_names())
    {
        const BuiltInType type = ExtensionDB::get_builtin_type(type_name);
        if (!type.properties.is_empty())
        {
            Array properties;
            for (const PropertyInfo& pi : type.properties)
                properties.push_back(pi.name);

            // Color exposes a variety of additional properties, we only concern ourselves
            // with the R,G,B,A properties and not R8,G8,B8,A8 nor H, S, or V.
            if (type.type == Variant::COLOR)
                properties.resize(4);

            // Plane exposes not only the X,Y,Z and distance but also the normal.
            // We want to express planes only via X, Y, Z, and distance.
            else if (type.type == Variant::PLANE)
                properties.resize(4);

            // AABB exposes position, size, and end.
            // We only want to express AABB via position and size only.
            else if (type.type == Variant::AABB)
                properties.resize(2);

            _type_components[type.type] = properties;
        }
    }
}

void OScriptNodeDecompose::post_initialize()
{
    // Clone this from the input pin
    _type = find_pin("value", PD_Input)->get_type();

    super::post_initialize();
}

void OScriptNodeDecompose::allocate_default_pins()
{
    // Set the pin with value that will be broken
    create_pin(PD_Input, PT_Data, PropertyUtils::make_typed("value", _type))->set_flag(OScriptNodePin::Flags::IGNORE_DEFAULT);

    Variant value = VariantUtils::make_default(_type);
    const Array &components = _type_components[_type];
    for (int i = 0; i < components.size(); i++)
    {
        const Variant bit = value.get(components[i]);
        create_pin(PD_Output, PT_Data, PropertyUtils::make_typed(components[i], bit.get_type()));
    }
}

String OScriptNodeDecompose::get_tooltip_text() const
{
    if (_type != Variant::NIL)
    {
        const String type_name = VariantUtils::get_friendly_type_name(_type);
        const String components = StringUtils::join(", ", _type_components[_type]);
        return vformat("Break a %s into %s", type_name, components);
    }
    return "Breaks a complex structure into its components";
}

String OScriptNodeDecompose::get_node_title() const
{
    return "Break " + VariantUtils::get_friendly_type_name(_type);
}

String OScriptNodeDecompose::get_icon() const
{
    return "Unlinked";
}

String OScriptNodeDecompose::get_help_topic() const
{
    #if GODOT_VERSION >= 0x040300
    return vformat("class:%s", Variant::get_type_name(_type));
    #else
    return vformat("%s", Variant::get_type_name(_type));
    #endif
}

PackedStringArray OScriptNodeDecompose::get_keywords() const
{
    return Array::make("break", "split", "separate", "decompose", Variant::get_type_name(_type));
}

OScriptNodeInstance* OScriptNodeDecompose::instantiate()
{
    OScriptNodeDecomposeInstance* i = memnew(OScriptNodeDecomposeInstance);
    i->_node = this;
    i->_components = _type_components[_type];
    return i;
}

void OScriptNodeDecompose::initialize(const OScriptNodeInitContext& p_context)
{
    ERR_FAIL_COND_MSG(!p_context.user_data, "A Decompose node requires custom data");

    const Dictionary& data = p_context.user_data.value();
    ERR_FAIL_COND_MSG(!data.has("type"), "Cannot properly initialize decompose node, no type specified.");

    _type = VariantUtils::to_type(data["type"]);
    super::initialize(p_context);
}

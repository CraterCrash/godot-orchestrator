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
#include "coercion_node.h"

#include "common/logger.h"
#include "common/property_utils.h"
#include "common/variant_utils.h"

class OScriptNodeCoercionInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeCoercion);

public:
    int step(OScriptExecutionContext& p_context) override
    {
        p_context.copy_input_to_output(0, 0);
        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OScriptNodeCoercion::OScriptNodeCoercion()
{
    _flags = ScriptNodeFlags::NONE;
}

void OScriptNodeCoercion::post_initialize()
{
    _add_source_target_listeners();

    Ref<OScriptNodePin> input = _get_input_pin();
    if (input.is_valid())
        _left = input->get_type();

    Ref<OScriptNodePin> output = _get_output_pin();
    if (output.is_valid())
        _right = output->get_type();

    super::post_initialize();
}

void OScriptNodeCoercion::post_placed_new_node()
{
    _add_source_target_listeners();
    super::post_placed_new_node();
}

void OScriptNodeCoercion::allocate_default_pins()
{
    create_pin(PD_Input, PT_Data, PropertyUtils::make_typed("input", _left))->hide_label();
    create_pin(PD_Output, PT_Data, PropertyUtils::make_typed("output", _right))->hide_label();
}

String OScriptNodeCoercion::get_tooltip_text() const
{
    const String left = VariantUtils::get_friendly_type_name(_left, true);
    const String right = VariantUtils::get_friendly_type_name(_right, true);
    return vformat("Converts %s %s to %s", VariantUtils::get_type_name_article(_left, true), left, right);
}

String OScriptNodeCoercion::get_node_title() const
{
    return " "; // todo: hack trick to avoid apply a title for now
}

OScriptNodeInstance* OScriptNodeCoercion::instantiate()
{
    OScriptNodeCoercionInstance* i = memnew(OScriptNodeCoercionInstance);
    i->_node = this;
    return i;
}

void OScriptNodeCoercion::initialize(const OScriptNodeInitContext& p_context)
{
    if (p_context.user_data)
    {
        const Dictionary& data = p_context.user_data.value();
        if (data.has("left_type"))
            _left = VariantUtils::to_type(int(data["left_type"]));
        if (data.has("right_type"))
            _right = VariantUtils::to_type(int(data["right_type"]));
    }
    super::initialize(p_context);
}

Ref<OScriptNodePin> OScriptNodeCoercion::_get_input_pin()
{
    return find_pin("input", PD_Input);
}

Ref<OScriptNodePin> OScriptNodeCoercion::_get_output_pin()
{
    return find_pin("output", PD_Output);
}

Ref<OScriptNodePin> OScriptNodeCoercion::_get_source_node_pin()
{
    Ref<OScriptNodePin> input = _get_input_pin();
    if (!input.is_valid())
        return {};

    Vector<Ref<OScriptNodePin>> connections = get_orchestration()->get_connections(input.ptr());
    if (connections.is_empty())
        return {};

    return connections[0];
}

Ref<OScriptNodePin> OScriptNodeCoercion::_get_target_node_pin()
{
    Ref<OScriptNodePin> output = _get_output_pin();
    if (!output.is_valid())
        return {};

    Vector<Ref<OScriptNodePin>> connections = get_orchestration()->get_connections(output.ptr());
    if (connections.is_empty())
        return {};

    return connections[0];
}

void OScriptNodeCoercion::_add_source_target_listeners()
{
    if (!_is_in_editor())
        return;

    _cache_pin_indices();

    Ref<OScriptNodePin> source = _get_source_node_pin();
    if (source.is_valid())
    {
        _on_source_pin_changed(source);
        source->connect("changed", callable_mp(this, &OScriptNodeCoercion::_on_source_pin_changed).bind(source));
    }

    Ref<OScriptNodePin> target = _get_target_node_pin();
    if (target.is_valid())
    {
        _on_target_pin_changed(target);
        target->connect("changed", callable_mp(this, &OScriptNodeCoercion::_on_target_pin_changed).bind(target));
    }
}

void OScriptNodeCoercion::_on_source_pin_changed(const Ref<OScriptNodePin>& p_pin)
{
    Ref<OScriptNodePin> input = _get_input_pin();
    if (input.is_valid())
        input->set_type(p_pin->get_type());

    // If source node is removed, remove this node
    if (!_get_source_node_pin().is_valid())
        get_orchestration()->remove_node(_id);
}

void OScriptNodeCoercion::_on_target_pin_changed(const Ref<OScriptNodePin>& p_pin)
{
    Ref<OScriptNodePin> output = _get_output_pin();
    if (output.is_valid())
        output->set_type(p_pin->get_type());

    // if target node is removed, remove this node
    if (!_get_target_node_pin().is_valid())
         get_orchestration()->remove_node(_id);
}
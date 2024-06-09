// This file is part of the Godot Orchestrator project.
//
// Copyright (c) 2023-present Vahera Studios LLC and its contributors.
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
#include "arrays.h"

#include "common/variant_utils.h"

class OScriptNodeMakeArrayInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeMakeArray);
    int _count{ 0 };

public:
    int step(OScriptExecutionContext& p_context) override
    {
        Array result;
        for (int i = 0; i < _count; i++)
            result.push_back(p_context.get_input(i));

        p_context.set_output(0, result);
        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class OScriptNodeArrayGetInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeArrayGet)

    Variant::Type _collection_type;
    Variant::Type _index_type;

    template<typename T>
    int _step_internal(OScriptExecutionContext& p_context)
    {
        T array = p_context.get_input(0);
        int index = p_context.get_input(1);

        if (array.size() > index)
            p_context.set_output(0, array[index]);
        else
            p_context.set_output(0, Variant());

        return 0;
    }

public:
    int step(OScriptExecutionContext& p_context) override
    {
        switch (_collection_type)
        {
            case Variant::ARRAY:
                return _step_internal<Array>(p_context);
            case Variant::PACKED_BYTE_ARRAY:
                return _step_internal<PackedByteArray>(p_context);
            case Variant::PACKED_INT32_ARRAY:
                return _step_internal<PackedInt32Array>(p_context);
            case Variant::PACKED_INT64_ARRAY:
                return _step_internal<PackedInt64Array>(p_context);
            case Variant::PACKED_FLOAT32_ARRAY:
                return _step_internal<PackedFloat32Array>(p_context);
            case Variant::PACKED_FLOAT64_ARRAY:
                return _step_internal<PackedFloat64Array>(p_context);
            case Variant::PACKED_STRING_ARRAY:
                return _step_internal<PackedStringArray>(p_context);
            case Variant::PACKED_VECTOR2_ARRAY:
                return _step_internal<PackedVector2Array>(p_context);
            case Variant::PACKED_VECTOR3_ARRAY:
                return _step_internal<PackedVector3Array>(p_context);
            case Variant::PACKED_COLOR_ARRAY:
                return _step_internal<PackedColorArray>(p_context);
            default:
                p_context.set_error(GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT,
                    "The collection type " + itos(_collection_type) + " is not supported.");
            return -1;
        }
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class OScriptNodeArraySetInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeArraySet)

    Variant::Type _collection_type;
    Variant::Type _index_type;

    int _invalid_index(OScriptExecutionContext& p_context, int p_index)
    {
        p_context.set_error(GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT,
            "Size is too small to index at index " + itos(p_index));
        return -1;
    }

    template<typename T>
    int _step_internal(OScriptExecutionContext& p_context)
    {
        T array = p_context.get_input(0);
        int index = p_context.get_input(1);
        Variant item = p_context.get_input(2);
        bool size_to_fit = p_context.get_input(3);

        const int size = array.size();
        if (size <= index && !size_to_fit)
        {
            p_context.set_error(GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT,
                "Collection size is too small to insert at index " + itos(index));
            return -1;
        }
        else if (size <= index)
        {
            array.resize(index + 1);
        }

        array[index] = item;
        p_context.set_output(0, array);
        return 0;
    }

public:
    int step(OScriptExecutionContext& p_context) override
    {
        switch (_collection_type)
        {
            case Variant::ARRAY:
                return _step_internal<Array>(p_context);
            case Variant::PACKED_BYTE_ARRAY:
                return _step_internal<PackedByteArray>(p_context);
            case Variant::PACKED_INT32_ARRAY:
                return _step_internal<PackedInt32Array>(p_context);
            case Variant::PACKED_INT64_ARRAY:
                return _step_internal<PackedInt64Array>(p_context);
            case Variant::PACKED_FLOAT32_ARRAY:
                return _step_internal<PackedFloat32Array>(p_context);
            case Variant::PACKED_FLOAT64_ARRAY:
                return _step_internal<PackedFloat64Array>(p_context);
            case Variant::PACKED_STRING_ARRAY:
                return _step_internal<PackedStringArray>(p_context);
            case Variant::PACKED_VECTOR2_ARRAY:
                return _step_internal<PackedVector2Array>(p_context);
            case Variant::PACKED_VECTOR3_ARRAY:
                return _step_internal<PackedVector3Array>(p_context);
            case Variant::PACKED_COLOR_ARRAY:
                return _step_internal<PackedColorArray>(p_context);
            default:
                p_context.set_error(GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT,
                    "The collection type " + itos(_collection_type) + " is not supported.");
                return -1;
        }
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class OScriptNodeArrayFindInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeArrayFind)

public:
    int step(OScriptExecutionContext& p_context) override
    {
        Array target_array = p_context.get_input(0);
        Variant item = p_context.get_input(1);

        p_context.set_output(0, target_array);
        p_context.set_output(1, target_array.find(item));

        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class OScriptNodeArrayClearInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeArrayClear)

public:
    int step(OScriptExecutionContext& p_context) override
    {
        Array target_array = p_context.get_input(0);
        target_array.clear();

        p_context.set_output(0, target_array);
        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class OScriptNodeArrayAppendInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeArrayAppend)

public:
    int step(OScriptExecutionContext& p_context) override
    {
        Array target_array = p_context.get_input(0);
        Array source_array = p_context.get_input(1);
        for (int i = 0; i < source_array.size(); i++)
            target_array.push_back(source_array[i]);

        p_context.set_output(0, target_array);
        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class OScriptNodeArrayAddElementInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeArrayAddElement)

public:
    int step(OScriptExecutionContext& p_context) override
    {
        Array target_array = p_context.get_input(0);
        Variant item = p_context.get_input(1);

        int index = target_array.size();
        target_array.push_back(item);

        p_context.set_output(0, target_array);
        p_context.set_output(1, index);

        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class OScriptNodeArrayRemoveElementInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeArrayRemoveElement)

public:
    int step(OScriptExecutionContext& p_context) override
    {
        Array target_array = p_context.get_input(0);
        Variant item = p_context.get_input(1);

        int remove_index = -1;
        for (int i = 0; i < target_array.size(); i++)
        {
            if (target_array[i] == item)
            {
                remove_index = i;
                target_array.remove_at(i);
                break;
            }
        }

        p_context.set_output(0, target_array);
        p_context.set_output(1, remove_index != -1);

        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class OScriptNodeArrayRemoveIndexInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeArrayRemoveIndex)

public:
    int step(OScriptExecutionContext& p_context) override
    {
        Array target_array = p_context.get_input(0);
        int index = p_context.get_input(1);

        if (target_array.size() > index)
            target_array.remove_at(index);

        p_context.set_output(0, target_array);
        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void OScriptNodeMakeArray::post_initialize()
{
    _element_count = find_pins(PD_Input).size();
    super::post_initialize();
}

void OScriptNodeMakeArray::allocate_default_pins()
{
    for (int i = 0; i < _element_count; i++)
    {
        Ref<OScriptNodePin> pin = create_pin(PD_Input, _get_pin_name_given_index(i), Variant::NIL);
        pin->set_flags(OScriptNodePin::Flags::DATA);
        pin->set_label(vformat("[%d]", i));
    }

    create_pin(PD_Output, "array", Variant::ARRAY)->set_flags(OScriptNodePin::Flags::DATA);

    super::allocate_default_pins();
}

String OScriptNodeMakeArray::get_tooltip_text() const
{
    return "Create an array from a series of items.";
}

String OScriptNodeMakeArray::get_node_title() const
{
    return "Make Array";
}

String OScriptNodeMakeArray::get_icon() const
{
    return "FileThumbnail";
}

void OScriptNodeMakeArray::pin_default_value_changed(const Ref<OScriptNodePin>& p_pin)
{

}

OScriptNodeInstance* OScriptNodeMakeArray::instantiate()
{
    OScriptNodeMakeArrayInstance* i = memnew(OScriptNodeMakeArrayInstance);
    i->_node = this;
    i->_count = _element_count;
    return i;
}

void OScriptNodeMakeArray::add_dynamic_pin()
{
    _element_count++;
    reconstruct_node();
}

bool OScriptNodeMakeArray::can_remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin) const
{
    return _element_count > 0 and p_pin->is_input();
}

void OScriptNodeMakeArray::remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin)
{
    if (p_pin.is_valid() && p_pin->is_input())
    {
        const int pin_offset = p_pin->get_pin_index();

        p_pin->unlink_all(true);
        remove_pin(p_pin);

        _adjust_connections(pin_offset, -1, PD_Input);

        _element_count--;
        reconstruct_node();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeArrayGet::post_initialize()
{
    _collection_type = find_pin("array", PD_Input)->get_type();
    _index_type = find_pin("element", PD_Output)->get_type();
    _collection_name = Variant::get_type_name(_collection_type);

    super::post_initialize();
}

void OScriptNodeArrayGet::allocate_default_pins()
{
    _collection_name = Variant::get_type_name(_collection_type);

    create_pin(PD_Input, "array", _collection_type)->set_flags(OScriptNodePin::Flags::DATA);
    create_pin(PD_Input, "index", Variant::INT)->set_flags(OScriptNodePin::Flags::DATA);
    create_pin(PD_Output, "element", _index_type)->set_flags(OScriptNodePin::Flags::DATA);

    super::allocate_default_pins();
}

String OScriptNodeArrayGet::get_tooltip_text() const
{
    return vformat("Given a %s and index, return the item at the specified index.", _collection_name);
}

String OScriptNodeArrayGet::get_node_title() const
{
    return "Get Element At Index";
}

String OScriptNodeArrayGet::get_icon() const
{
    return "FileThumbnail";
}

OScriptNodeInstance* OScriptNodeArrayGet::instantiate()
{
    OScriptNodeArrayGetInstance* i = memnew(OScriptNodeArrayGetInstance);
    i->_node = this;
    i->_collection_type = _collection_type;
    i->_index_type = _index_type;
    return i;
}

void OScriptNodeArrayGet::initialize(const OScriptNodeInitContext& p_context)
{
    if (p_context.user_data)
    {
        const Dictionary& data = p_context.user_data.value();
        _collection_type = VariantUtils::to_type(data["collection_type"]);
        _index_type = VariantUtils::to_type(data["index_type"]);
    }
    return super::initialize(p_context);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeArraySet::post_initialize()
{
    _collection_type = find_pin("array", PD_Input)->get_type();
    _index_type = find_pin("element", PD_Input)->get_type();
    _collection_name = Variant::get_type_name(_collection_type);

    super::post_initialize();
}

void OScriptNodeArraySet::allocate_default_pins()
{
    _collection_name = Variant::get_type_name(_collection_type);

    create_pin(PD_Input, "ExecIn")->set_flags(OScriptNodePin::Flags::EXECUTION);
    create_pin(PD_Input, "array", _collection_type)->set_flags(OScriptNodePin::Flags::DATA);
    create_pin(PD_Input, "index", Variant::INT)->set_flags(OScriptNodePin::Flags::DATA);
    create_pin(PD_Input, "element", _index_type)->set_flags(OScriptNodePin::Flags::DATA);
    create_pin(PD_Input, "size_to_fit", Variant::BOOL)->set_flags(OScriptNodePin::Flags::DATA);

    create_pin(PD_Output, "ExecOut")->set_flags(OScriptNodePin::Flags::EXECUTION);
    create_pin(PD_Output, "result", _collection_type)->set_flags(OScriptNodePin::Flags::DATA);

    super::allocate_default_pins();
}

String OScriptNodeArraySet::get_tooltip_text() const
{
    return vformat("Given a %s and index, assign the value at the specified index.", _collection_name);
}

String OScriptNodeArraySet::get_node_title() const
{
    return "Set Element At Index";
}

String OScriptNodeArraySet::get_icon() const
{
    return "FileThumbnail";
}

OScriptNodeInstance* OScriptNodeArraySet::instantiate()
{
    OScriptNodeArraySetInstance* i = memnew(OScriptNodeArraySetInstance);
    i->_node = this;
    i->_collection_type = _collection_type;
    i->_index_type = _index_type;
    return i;
}

void OScriptNodeArraySet::initialize(const OScriptNodeInitContext& p_context)
{
    if (p_context.user_data)
    {
        const Dictionary& data = p_context.user_data.value();
        _collection_type = VariantUtils::to_type(data["collection_type"]);
        _index_type = VariantUtils::to_type(data["index_type"]);
    }
    return super::initialize(p_context);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeArrayFind::allocate_default_pins()
{
    create_pin(PD_Input, "array", Variant::ARRAY)->set_flags(OScriptNodePin::Flags::DATA);
    create_pin(PD_Input, "item", Variant::NIL)->set_flags(OScriptNodePin::Flags::DATA);

    create_pin(PD_Output, "array", Variant::ARRAY)->set_flags(OScriptNodePin::Flags::DATA);
    create_pin(PD_Output, "index", Variant::INT)->set_flags(OScriptNodePin::Flags::DATA);

    super::allocate_default_pins();
}

String OScriptNodeArrayFind::get_tooltip_text() const
{
    return "Given an array and an item, returns the index of the item.";
}

String OScriptNodeArrayFind::get_node_title() const
{
    return "Find Array Element";
}

String OScriptNodeArrayFind::get_icon() const
{
    return "FileThumbnail";
}

OScriptNodeInstance* OScriptNodeArrayFind::instantiate()
{
    OScriptNodeArrayFindInstance* i = memnew(OScriptNodeArrayFindInstance);
    i->_node = this;
    return i;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeArrayClear::allocate_default_pins()
{
    create_pin(PD_Input, "ExecIn")->set_flags(OScriptNodePin::Flags::EXECUTION);
    create_pin(PD_Input, "array", Variant::ARRAY)->set_flags(OScriptNodePin::Flags::DATA);

    create_pin(PD_Output, "ExecOut")->set_flags(OScriptNodePin::Flags::EXECUTION);
    create_pin(PD_Output, "array", Variant::ARRAY)->set_flags(OScriptNodePin::Flags::DATA);

    super::allocate_default_pins();
}

String OScriptNodeArrayClear::get_tooltip_text() const
{
    return "Given an array, clears its contents.";
}

String OScriptNodeArrayClear::get_node_title() const
{
    return "Clear Array";
}

String OScriptNodeArrayClear::get_icon() const
{
    return "FileThumbnail";
}

OScriptNodeInstance* OScriptNodeArrayClear::instantiate()
{
    OScriptNodeArrayClearInstance* i = memnew(OScriptNodeArrayClearInstance);
    i->_node = this;
    return i;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeArrayAppend::allocate_default_pins()
{
    create_pin(PD_Input, "ExecIn")->set_flags(OScriptNodePin::Flags::EXECUTION);
    Ref<OScriptNodePin> target = create_pin(PD_Input, "target_array", Variant::ARRAY);
    target->set_flags(OScriptNodePin::Flags::DATA);
    target->set_label("Target");

    Ref<OScriptNodePin> source = create_pin(PD_Input, "source_array", Variant::ARRAY);
    source->set_flags(OScriptNodePin::Flags::DATA);
    source->set_label("Source");

    create_pin(PD_Output, "ExecOut")->set_flags(OScriptNodePin::Flags::EXECUTION);
    create_pin(PD_Output, "array", Variant::ARRAY)->set_flags(OScriptNodePin::Flags::DATA);

    super::allocate_default_pins();
}

String OScriptNodeArrayAppend::get_tooltip_text() const
{
    return "Append the source array into the target array";
}

String OScriptNodeArrayAppend::get_node_title() const
{
    return "Append Arrays";
}

String OScriptNodeArrayAppend::get_icon() const
{
    return "FileThumbnail";
}

OScriptNodeInstance* OScriptNodeArrayAppend::instantiate()
{
    OScriptNodeArrayAppendInstance* i = memnew(OScriptNodeArrayAppendInstance);
    i->_node = this;
    return i;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeArrayAddElement::allocate_default_pins()
{
    create_pin(PD_Input, "ExecIn")->set_flags(OScriptNodePin::Flags::EXECUTION);
    Ref<OScriptNodePin> target = create_pin(PD_Input, "target_array", Variant::ARRAY);
    target->set_flags(OScriptNodePin::Flags::DATA);
    target->set_label("Target");

    create_pin(PD_Input, "element", Variant::NIL)->set_flags(OScriptNodePin::Flags::DATA);

    create_pin(PD_Output, "ExecOut")->set_flags(OScriptNodePin::Flags::EXECUTION);
    create_pin(PD_Output, "array", Variant::ARRAY)->set_flags(OScriptNodePin::Flags::DATA);
    create_pin(PD_Output, "index", Variant::INT)->set_flags(OScriptNodePin::Flags::DATA);

    super::allocate_default_pins();
}

String OScriptNodeArrayAddElement::get_tooltip_text() const
{
    return "Given an array, append the item to the array.";
}

String OScriptNodeArrayAddElement::get_node_title() const
{
    return "Add Array Item";
}

String OScriptNodeArrayAddElement::get_icon() const
{
    return "FileThumbnail";
}

OScriptNodeInstance* OScriptNodeArrayAddElement::instantiate()
{
    OScriptNodeArrayAddElementInstance* i = memnew(OScriptNodeArrayAddElementInstance);
    i->_node = this;
    return i;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeArrayRemoveElement::allocate_default_pins()
{
    create_pin(PD_Input, "ExecIn")->set_flags(OScriptNodePin::Flags::EXECUTION);
    Ref<OScriptNodePin> target = create_pin(PD_Input, "target_array", Variant::ARRAY);
    target->set_flags(OScriptNodePin::Flags::DATA);
    target->set_label("Target");

    create_pin(PD_Input, "element", Variant::NIL)->set_flags(OScriptNodePin::Flags::DATA);

    create_pin(PD_Output, "ExecOut")->set_flags(OScriptNodePin::Flags::EXECUTION);
    create_pin(PD_Output, "array", Variant::ARRAY)->set_flags(OScriptNodePin::Flags::DATA);
    create_pin(PD_Output, "removed", Variant::BOOL)->set_flags(OScriptNodePin::Flags::DATA);

    super::allocate_default_pins();
}

String OScriptNodeArrayRemoveElement::get_tooltip_text() const
{
    return "Given an array, remove the item from the array if it exists.";
}

String OScriptNodeArrayRemoveElement::get_node_title() const
{
    return "Remove Array Item";
}

String OScriptNodeArrayRemoveElement::get_icon() const
{
    return "FileThumbnail";
}

OScriptNodeInstance* OScriptNodeArrayRemoveElement::instantiate()
{
    OScriptNodeArrayRemoveElementInstance* i = memnew(OScriptNodeArrayRemoveElementInstance);
    i->_node = this;
    return i;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeArrayRemoveIndex::allocate_default_pins()
{
    create_pin(PD_Input, "ExecIn")->set_flags(OScriptNodePin::Flags::EXECUTION);
    Ref<OScriptNodePin> target = create_pin(PD_Input, "target_array", Variant::ARRAY);
    target->set_flags(OScriptNodePin::Flags::DATA);
    target->set_label("Target");

    create_pin(PD_Input, "index", Variant::INT)->set_flags(OScriptNodePin::Flags::DATA);

    create_pin(PD_Output, "ExecOut")->set_flags(OScriptNodePin::Flags::EXECUTION);
    create_pin(PD_Output, "array", Variant::ARRAY)->set_flags(OScriptNodePin::Flags::DATA);

    super::allocate_default_pins();
}

String OScriptNodeArrayRemoveIndex::get_tooltip_text() const
{
    return "Given an array, removes an item from the array by index.";
}

String OScriptNodeArrayRemoveIndex::get_node_title() const
{
    return "Remove Array Item By Index";
}

String OScriptNodeArrayRemoveIndex::get_icon() const
{
    return "FileThumbnail";
}

OScriptNodeInstance* OScriptNodeArrayRemoveIndex::instantiate()
{
    OScriptNodeArrayRemoveIndexInstance* i = memnew(OScriptNodeArrayRemoveIndexInstance);
    i->_node = this;
    return i;
}
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
#include "constants.h"

#include "api/extension_db.h"
#include "common/property_utils.h"
#include "common/string_utils.h"
#include "common/variant_utils.h"
#include "common/version.h"
#include "script/script_server.h"

#include <godot_cpp/classes/engine.hpp>

class OScriptNodeGlobalConstantInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeGlobalConstant)
    Variant _value;

public:
    int step(OScriptExecutionContext& p_context) override
    {
        p_context.set_output(0, _value);
        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class OScriptNodeMathConstantInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeMathConstant)
    double _value;

public:
    int step(OScriptExecutionContext& p_context) override
    {
        p_context.set_output(0, _value);
        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class OScriptNodeTypeConstantInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeTypeConstant)
    Variant _value;

public:
    int step(OScriptExecutionContext& p_context) override
    {
        p_context.set_output(0, _value);
        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class OScriptNodeClassConstantInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeClassConstant)
    Variant _value;

public:
    int step(OScriptExecutionContext& p_context) override
    {
        p_context.set_output(0, _value);
        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class OScriptNodeSingletonConstantInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeSingletonConstant)
    Variant _value;

public:
    int step(OScriptExecutionContext& p_context) override
    {
        p_context.set_output(0, _value);
        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OScriptNodeConstant::OScriptNodeConstant()
{
    OScriptNode::_flags = CATALOGABLE | EXPERIMENTAL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

StringName OScriptNodeGlobalConstant::_get_default_constant_name() const
{
    if (_constant_name.is_empty())
    {
        PackedStringArray names = ExtensionDB::get_global_enum_value_names();
        if (!names.is_empty())
            return names[0];
        return {};
    }
    return _constant_name;
}

void OScriptNodeGlobalConstant::_get_property_list(List<PropertyInfo>* r_list) const
{
    const String constants = StringUtils::join(",", ExtensionDB::get_global_enum_value_names());
    r_list->push_front(PropertyInfo(Variant::STRING, "constant", PROPERTY_HINT_ENUM, constants));
}

bool OScriptNodeGlobalConstant::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("constant"))
    {
        r_value = _constant_name;
        return true;
    }
    return false;
}

bool OScriptNodeGlobalConstant::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("constant"))
    {
        _constant_name = p_value;
        _notify_pins_changed();
        return true;
    }
    return false;
}

bool OScriptNodeGlobalConstant::_property_can_revert(const StringName& p_name) const
{
    return p_name.match("constant");
}

bool OScriptNodeGlobalConstant::_property_get_revert(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("constant"))
    {
        const PackedStringArray enum_names = ExtensionDB::get_global_enum_value_names();
        if (!enum_names.is_empty())
        {
            r_value = StringName(enum_names[0]);
            return true;
        }
    }
    return false;
}

void OScriptNodeGlobalConstant::_upgrade(uint32_t p_version, uint32_t p_current_version)
{
    if (p_version == 1 && p_current_version >= 2)
    {
        // Fixup - make sure pin uses new enum semantics
        Ref<OScriptNodePin> constant = find_pin("constant", PD_Output);
        if (constant.is_valid() && !PropertyUtils::is_class_enum(constant->get_property_info()))
            reconstruct_node();
    }

    super::_upgrade(p_version, p_current_version);
}

void OScriptNodeGlobalConstant::post_initialize()
{
    // Initially set the value from the pin
    Ref<OScriptNodePin> constant = find_pin("constant", PD_Output);
    if (constant.is_valid())
        _constant_name = constant->get_label();

    // Seed from the list if pin had no value
    _constant_name = _get_default_constant_name();

    super::post_initialize();
}

void OScriptNodeGlobalConstant::allocate_default_pins()
{
    EnumInfo ei = ExtensionDB::get_global_enum_by_value(_constant_name);
    if (ei.values.is_empty())
    {
        ERR_FAIL_MSG("Failed to locate enum for " + _constant_name);
    }
    Ref<OScriptNodePin> constant = create_pin(PD_Output, PT_Data, PropertyUtils::make_enum_class("constant", ei.name));
    constant->set_label(_constant_name, false);

    super::allocate_default_pins();
}

String OScriptNodeGlobalConstant::get_tooltip_text() const
{
    return "Return the value of a global constant";
}

String OScriptNodeGlobalConstant::get_node_title() const
{
    return "Global Constant";
}

String OScriptNodeGlobalConstant::get_help_topic() const
{
    #if GODOT_VERSION >= 0x040300
    return vformat("class_constant:@GlobalScope:%s", _constant_name);
    #else
    return super::get_help_topic();
    #endif
}

String OScriptNodeGlobalConstant::get_icon() const
{
    return "MemberConstant";
}

PackedStringArray OScriptNodeGlobalConstant::get_keywords() const
{
    return PackedStringArray();
}

OScriptNodeInstance* OScriptNodeGlobalConstant::instantiate()
{
    OScriptNodeGlobalConstantInstance* i = memnew(OScriptNodeGlobalConstantInstance);
    i->_node = this;

    const EnumValue& ev = ExtensionDB::get_global_enum_value(_constant_name);
    i->_value = ev.value;

    return i;
}

void OScriptNodeGlobalConstant::initialize(const OScriptNodeInitContext& p_context)
{
    _constant_name = _get_default_constant_name();
    super::initialize(p_context);
}

void OScriptNodeGlobalConstant::validate_node_during_build(BuildLog& p_log) const
{
    super::validate_node_during_build(p_log);

    if (_constant_name.is_empty())
        p_log.error(this, "Constant node has no constant name specified.");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeMathConstant::_get_property_list(List<PropertyInfo>* r_list) const
{
    const String constants = StringUtils::join(",", ExtensionDB::get_math_constant_names());
    r_list->push_front(PropertyInfo(Variant::STRING, "constant", PROPERTY_HINT_ENUM, constants));
}

bool OScriptNodeMathConstant::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("constant"))
    {
        r_value = _constant_name;
        return true;
    }
    return false;
}

bool OScriptNodeMathConstant::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("constant"))
    {
        _constant_name = p_value;
        _notify_pins_changed();
        return true;
    }
    return false;
}

void OScriptNodeMathConstant::allocate_default_pins()
{
    Ref<OScriptNodePin> constant = create_pin(PD_Output, PT_Data, PropertyUtils::make_typed("constant", Variant::FLOAT));
    constant->set_label(_constant_name, false);
    super::allocate_default_pins();
}

String OScriptNodeMathConstant::get_tooltip_text() const
{
    return "Return the value of a math constant";
}

String OScriptNodeMathConstant::get_node_title() const
{
    return "Math Constant";
}

String OScriptNodeMathConstant::get_help_topic() const
{
    #if GODOT_VERSION >= 0x040300
    // todo: some math constants are not exposed to the documentation, i.e. "One"
    //       check if these can be exposed via OScriptLanguage instead?
    return vformat("class_constant:@GDScript:%s", _constant_name);
    #else
    return super::get_help_topic();
    #endif
}

String OScriptNodeMathConstant::get_icon() const
{
    return "MemberConstant";
}

PackedStringArray OScriptNodeMathConstant::get_keywords() const
{
    return ExtensionDB::get_math_constant_names();
}

OScriptNodeInstance* OScriptNodeMathConstant::instantiate()
{
    OScriptNodeMathConstantInstance* i = memnew(OScriptNodeMathConstantInstance);
    i->_node = this;
    i->_value = ExtensionDB::get_math_constant(_constant_name).value;
    return i;
}

void OScriptNodeMathConstant::validate_node_during_build(BuildLog& p_log) const
{
    super::validate_node_during_build(p_log);

    if (_constant_name.is_empty())
        p_log.error(this, "No constant name specified.");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HashMap<Variant::Type,HashMap<StringName,Variant>> OScriptNodeTypeConstant::_type_constants;
Vector<Variant::Type> OScriptNodeTypeConstant::_types;

void OScriptNodeTypeConstant::_get_property_list(List<PropertyInfo>* r_list) const
{
    PackedStringArray array;
    for (const KeyValue<Variant::Type, HashMap<StringName, Variant>>& E: _type_constants)
        array.push_back(VariantUtils::get_friendly_type_name(E.key));

    const String types = StringUtils::join(",", array);
    r_list->push_front(PropertyInfo(Variant::INT, "basic_type", PROPERTY_HINT_ENUM, types, PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_UPDATE_ALL_IF_MODIFIED));

    PackedStringArray array_values;
    if (_type_constants.has(_type))
    {
        for (const KeyValue<StringName, Variant>& E : _type_constants[_type])
            array_values.push_back(E.key);
    }
    const String values = StringUtils::join(",", array_values);
    r_list->push_back(PropertyInfo(Variant::STRING, "constant", PROPERTY_HINT_ENUM, values));
}

bool OScriptNodeTypeConstant::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("basic_type"))
    {
        r_value = _types.find(_type);
        return true;
    }
    else if (p_name.match("constant"))
    {
        r_value = _constant_name;
        return true;
    }
    return false;
}

bool OScriptNodeTypeConstant::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("basic_type"))
    {
        const int index = p_value;
        if (index >= 0 && !_types.is_empty() && _type != _types[p_value])
        {
            _type = _types[p_value];
            _constant_name = _type_constants[_type].begin()->key;
            _notify_pins_changed();
            notify_property_list_changed();
            return true;
        }
    }
    else if (p_name.match("constant"))
    {
        if (_constant_name != p_value)
        {
            _constant_name = p_value;
            _notify_pins_changed();
            return true;
        }
    }
    return false;
}

bool OScriptNodeTypeConstant::_property_can_revert(const StringName& p_name) const
{
    if (p_name.match("basic_type") || p_name.match("constant"))
        return true;

    return false;
}

bool OScriptNodeTypeConstant::_property_get_revert(const StringName& p_name, Variant& r_value) const
{
    if (!_type_constants.is_empty())
    {
        if (p_name.match("basic_type"))
        {
            r_value = _type_constants.begin()->key;
            return true;
        }
        else if (p_name.match("constant"))
        {
            r_value = String(_type_constants[_type].begin()->key);
            return true;
        }
    }
    return false;
}

void OScriptNodeTypeConstant::_bind_methods()
{
    PackedStringArray builtin_type_names = ExtensionDB::get_builtin_type_names();
    int type_index = 0;
    for (const String& type_name : builtin_type_names)
    {
        BuiltInType type = ExtensionDB::get_builtin_type(type_name);
        if (!type.constants.is_empty())
        {
            _types.push_back(VariantUtils::to_type(type_index));
            for (const ConstantInfo& ci : type.constants)
                _type_constants[VariantUtils::to_type(type_index)][ci.name] = ci.value;
        }
        type_index++;
    }
}

void OScriptNodeTypeConstant::_upgrade(uint32_t p_version, uint32_t p_current_version)
{
    if (p_version == 1 && p_current_version >= 2)
    {
        // Fixup - Make sure that the class enum details are encoded
        Ref<OScriptNodePin> constant = find_pin("constant", PD_Output);
        if (constant.is_valid() && !PropertyUtils::is_class_enum(constant->get_property_info()))
            reconstruct_node();
    }

    super::_upgrade(p_version, p_current_version);
}

PropertyInfo OScriptNodeTypeConstant::_create_pin_property_info()
{
    // Check whether constant is an enumeration
    BuiltInType type = ExtensionDB::get_builtin_type(_type);
    ERR_FAIL_COND_V_MSG(type.type != _type, PropertyInfo(), "Failed to resolve built-in type");

    for (const EnumInfo& E : type.enums)
    {
        for (const EnumValue& V : E.values)
        {
            if (V.name == _constant_name)
                return PropertyUtils::make_class_enum("constant", type.name, E.name);
        }
    }
    for (const ConstantInfo& C : type.constants)
    {
        if (C.name == _constant_name)
            return PropertyUtils::make_typed("constant", _type);
    }

    ERR_FAIL_V_MSG(PropertyInfo(), "Failed to find type constant " + _constant_name);
}

void OScriptNodeTypeConstant::allocate_default_pins()
{
    String label = VariantUtils::get_friendly_type_name(_type);
    if (!_constant_name.is_empty())
        label += "::" + _constant_name;

    Ref<OScriptNodePin> constant = create_pin(PD_Output, PT_Data, _create_pin_property_info());
    constant->set_label(label, false);
    super::allocate_default_pins();
}

String OScriptNodeTypeConstant::get_tooltip_text() const
{
    return "Return the value of a variant type constant";
}

String OScriptNodeTypeConstant::get_node_title() const
{
    return "Type Constant";
}

String OScriptNodeTypeConstant::get_help_topic() const
{
    #if GODOT_VERSION >= 0x040300
    return vformat("class_constant:%s:%s", Variant::get_type_name(_type), _constant_name);
    #else
    return super::get_help_topic();
    #endif
}

String OScriptNodeTypeConstant::get_icon() const
{
    return "MemberConstant";
}

OScriptNodeInstance* OScriptNodeTypeConstant::instantiate()
{
    OScriptNodeTypeConstantInstance* i = memnew(OScriptNodeTypeConstantInstance);
    i->_node = this;
    i->_value = _type_constants[_type][_constant_name];
    return i;
}

void OScriptNodeTypeConstant::initialize(const OScriptNodeInitContext& p_context)
{
    _type = _types[0];
    _constant_name = _type_constants[_type].begin()->key;

    super::initialize(p_context);
}

void OScriptNodeTypeConstant::validate_node_during_build(BuildLog& p_log) const
{
    super::validate_node_during_build(p_log);

    if (_constant_name.is_empty())
        p_log.error(this, "No constant name specified.");
    else if (_type == Variant::NIL)
        p_log.error(this, "No type specified.");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeClassConstantBase::_get_property_list(List<PropertyInfo>* r_list) const
{
    const PackedStringArray class_names = _get_class_names();
    if (!class_names.is_empty())
    {
        const String choices = StringUtils::join(",", class_names);
        r_list->push_back(PropertyInfo(Variant::STRING, "class_name", PROPERTY_HINT_ENUM, choices));
    }
    else
        r_list->push_back(PropertyInfo(Variant::STRING, "class_name", PROPERTY_HINT_TYPE_STRING, "Object"));


    const String constants = StringUtils::join(",", _get_class_constant_choices(_class_name));
    r_list->push_back(PropertyInfo(Variant::STRING, "constant", PROPERTY_HINT_ENUM, constants));
}

bool OScriptNodeClassConstantBase::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("class_name"))
    {
        r_value = _class_name;
        return true;
    }
    else if (p_name.match("constant"))
    {
        r_value = _constant_name;
        return true;
    }
    return false;
}

bool OScriptNodeClassConstantBase::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("class_name"))
    {
        if (_class_name != p_value)
        {
            _class_name = p_value;
            _constant_name = "";
            _notify_pins_changed();
            notify_property_list_changed();
            return true;
        }
    }
    else if (p_name.match("constant"))
    {
        if (_constant_name != p_value)
        {
            _constant_name = p_value;
            _notify_pins_changed();
            return true;
        }
    }
    return false;
}

void OScriptNodeClassConstantBase::_upgrade(uint32_t p_version, uint32_t p_current_version)
{
    if (p_version == 1 && p_current_version >= 2)
    {
        // Fixup - make sure pin uses new enum semantics
        Ref<OScriptNodePin> constant = find_pin("constant", PD_Output);
        if (constant.is_valid() && !PropertyUtils::is_class_enum(constant->get_property_info()))
            reconstruct_node();
    }

    super::_upgrade(p_version, p_current_version);
}

Ref<OScriptNodePin> OScriptNodeClassConstantBase::_create_constant_pin()
{
    if (ScriptServer::is_global_class(_class_name))
    {
        const StringName enum_name = ScriptServer::get_global_class(_class_name).get_integer_constant_enum(_constant_name);
        if (!enum_name.is_empty())
            return create_pin(PD_Output, PT_Data, PropertyUtils::make_class_enum("constant", _class_name, enum_name));
    }
    else
    {
        const StringName enum_name = ClassDB::class_get_integer_constant_enum(_class_name, _constant_name);
        if (!enum_name.is_empty())
            return create_pin(PD_Output, PT_Data, PropertyUtils::make_class_enum("constant", _class_name, enum_name));
    }

    return create_pin(PD_Output, PT_Data, PropertyUtils::make_typed("constant", Variant::INT));
}

void OScriptNodeClassConstantBase::allocate_default_pins()
{
    String label = _class_name;
    if (!_constant_name.is_empty())
        label += "::" + _constant_name;

    _create_constant_pin()->set_label(label, false);

    super::allocate_default_pins();
}

String OScriptNodeClassConstantBase::get_tooltip_text() const
{
    return "Return the value of a class-level constant";
}

String OScriptNodeClassConstantBase::get_node_title() const
{
    return "Class Constant";
}

String OScriptNodeClassConstantBase::get_help_topic() const
{
    #if GODOT_VERSION >= 0x040300
    String class_name = _class_name;
    while (!class_name.is_empty())
    {
        PackedStringArray values = ClassDB::class_get_integer_constant_list(class_name, true);
        if (values.has(_constant_name))
            return vformat("class_constant:%s:%s", class_name, _constant_name);
        class_name = ClassDB::get_parent_class(class_name);
    }
    #endif
    return super::get_help_topic();
}

String OScriptNodeClassConstantBase::get_icon() const
{
    return "MemberConstant";
}

void OScriptNodeClassConstantBase::validate_node_during_build(BuildLog& p_log) const
{
    super::validate_node_during_build(p_log);

    if (_class_name.is_empty())
        p_log.error(this, "No constant class name specified.");
    else if (_constant_name.is_empty())
        p_log.error(this, "No constant specified.");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OScriptNodeClassConstant::OScriptNodeClassConstant()
{
    _class_name = "Object";
}

PackedStringArray OScriptNodeClassConstant::_get_class_constant_choices(const String& p_class_name) const
{
    if (ScriptServer::is_global_class(p_class_name))
        return ScriptServer::get_global_class(p_class_name).get_integer_constant_list();

    return ClassDB::class_get_integer_constant_list(_class_name, false);
}

OScriptNodeInstance* OScriptNodeClassConstant::instantiate()
{
    OScriptNodeClassConstantInstance* i = memnew(OScriptNodeClassConstantInstance);
    i->_node = this;

    if (ScriptServer::is_global_class(_class_name))
        i->_value = ScriptServer::get_global_class(_class_name).get_integer_constant(_constant_name);
    else
        i->_value = ClassDB::class_get_integer_constant(_class_name, _constant_name);

    return i;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OScriptNodeSingletonConstant::OScriptNodeSingletonConstant()
{
    _class_name = "Input"; // Most common choice
    _singletons = _get_singletons_with_enum_constants();
}

PackedStringArray OScriptNodeSingletonConstant::_get_singletons_with_enum_constants() const
{
    // Initialize the singleton list once using static-init
    static const PackedStringArray singletons_with_enum_constants = []() {
        PackedStringArray array;
        for (const String& singleton : Engine::get_singleton()->get_singleton_list())
        {
            const PackedStringArray enums = ClassDB::class_get_enum_list(singleton, true);
            if (enums.is_empty())
                continue;
            array.push_back(singleton);
        }
        return array;
    }();

    return singletons_with_enum_constants;
}

PackedStringArray OScriptNodeSingletonConstant::_get_class_constant_choices(const String& p_class_name) const
{
    PackedStringArray results;
    const PackedStringArray enum_names = ClassDB::class_get_enum_list(p_class_name, true);
    for (const String& enum_name : enum_names)
    {
        const PackedStringArray constants = ClassDB::class_get_enum_constants(p_class_name, enum_name, true);
        for (const String& constant : constants)
            results.push_back(constant);
    }
    return results;
}

PackedStringArray OScriptNodeSingletonConstant::_get_class_names() const
{
    return _singletons;
}

OScriptNodeInstance* OScriptNodeSingletonConstant::instantiate()
{
    OScriptNodeSingletonConstantInstance* i = memnew(OScriptNodeSingletonConstantInstance);
    i->_node = this;
    i->_value = ClassDB::class_get_integer_constant(_class_name, _constant_name);
    return i;
}
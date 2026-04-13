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
#include "orchestration/nodes/call_function.h"

#include "api/extension_db.h"
#include "common/dictionary_utils.h"
#include "common/macros.h"
#include "common/method_utils.h"
#include "common/property_utils.h"
#include "common/string_utils.h"
#include "common/variant_utils.h"
#include "core/godot/gdextension_compat.h"
#include "orchestration/orchestration.h"
#include "script/script_server.h"
#include "script/utility_functions.h"

#include <godot_cpp/classes/expression.hpp>
#include <godot_cpp/classes/node.hpp>

void OScriptNodeCallFunction::_get_property_list(List<PropertyInfo>* r_list) const {
    r_list->push_back(PropertyInfo(Variant::STRING, "guid", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    r_list->push_back(PropertyInfo(Variant::STRING, "function_name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    r_list->push_back(PropertyInfo(Variant::STRING_NAME, "target_class_name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    r_list->push_back(PropertyInfo(Variant::INT, "target_type", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));

    const String flags = "Pure,Const,Is Bead,Is Self,Virtual,VarArg,Static,Object Core,Editor";
    r_list->push_back(PropertyInfo(Variant::INT, "flags", PROPERTY_HINT_FLAGS, flags, PROPERTY_USAGE_STORAGE));

    if (_is_method_info_serialized()) {
        r_list->push_back(PropertyInfo(Variant::DICTIONARY, "method", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    }
    if (_reference.method.flags & METHOD_FLAG_VARARG) {
        r_list->push_back(PropertyInfo(Variant::INT, "variable_arg_count", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    }
    if (_chainable) {
        r_list->push_back(PropertyInfo(Variant::BOOL, "chain"));
    }
}

bool OScriptNodeCallFunction::_get(const StringName& p_name, Variant& r_value) const {
    if (p_name.match("guid")) {
        r_value = _reference.guid;
        return true;
    } else if (p_name.match("function_name")) {
        r_value = _reference.method.name;
        return true;
    } else if (p_name.match("target_class_name")) {
        r_value = _reference.target_class_name;
        return true;
    } else if (p_name.match("target_type")) {
        r_value = _reference.target_type;
        return true;
    } else if (p_name.match("flags")) {
        r_value = _function_flags;
        return true;
    } else if (p_name.match("method")) {
        r_value = DictionaryUtils::from_method(_reference.method, true);
        return true;
    } else if (p_name.match("variable_arg_count")) {
        r_value = MAX(_vararg_count, 0);
        return true;
    } else if (p_name.match("chain")) {
        r_value = _chain;
        return true;
    }
    return false;
}

bool OScriptNodeCallFunction::_set(const StringName& p_name, const Variant& p_value) {
    if (p_name.match("guid")) {
        _reference.guid = Guid(p_value);
        return true;
    } else if (p_name.match("target_class_name")) {
        _reference.target_class_name = p_value;
        return true;
    } else if (p_name.match("target_type")) {
        _reference.target_type = VariantUtils::to_type(p_value);
        return true;
    } else if (p_name.match("flags")) {
        _function_flags = static_cast<int>(p_value);
        _notify_pins_changed();
        return true;
    } else if (p_name.match("method")) {
        _reference.method = DictionaryUtils::to_method(p_value);
        return true;
    } else if (p_name.match("variable_arg_count")) {
        _vararg_count = MAX(static_cast<int>(p_value), 0);
        _notify_pins_changed();
        return true;
    } else if (p_name.match("chain")) {
        _chain = p_value;
        if (!_chain) {
            Ref<OScriptNodePin> pin = find_pin("return_target", PD_Output);
            if (pin.is_valid() && pin->has_any_connections()) {
                pin->unlink_all();
            }
        }
        _notify_pins_changed();
        return true;
    }
    return false;
}

void OScriptNodeCallFunction::_upgrade(uint32_t p_version, uint32_t p_current_version) {
    if (p_version == 1 && p_current_version >= 2) {
        // Fixup - Address missing usage flags for certain method arguments
        for (PropertyInfo& pi : _reference.method.arguments) {
            if (PropertyUtils::is_nil_no_variant(pi)) {
                pi.usage |= PROPERTY_USAGE_NIL_IS_VARIANT;
            }
        }
    }

    super::_upgrade(p_version, p_current_version);
}

void OScriptNodeCallFunction::_create_pins_for_method(const MethodInfo& p_method) {
    if (_has_execution_pins(p_method)) {
        create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"));
        create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("ExecOut"));
    }

    _chainable = false;
    Ref<OScriptNodePin> target = _create_target_pin();

    const size_t default_start_index = is_vector_empty(p_method.arguments)
        ? 0
        : p_method.arguments.size() - p_method.default_arguments.size();

    size_t argument_index = 0;
    size_t default_index = 0;
    for (const PropertyInfo& pi : p_method.arguments) {
        Ref<OScriptNodePin> pin = create_pin(PD_Input, PT_Data, pi);

        if (pin.is_valid()) {
            const String arg_class_name = pin->get_property_info().class_name;
            if (!arg_class_name.is_empty() && _use_argument_class_name()) {
                if (arg_class_name.contains(".")) {
                    pin->set_label(arg_class_name.substr(arg_class_name.find(".") + 1));
                } else {
                    pin->set_label(arg_class_name);
                }
                pin->no_pretty_format();
            }

            if (argument_index >= default_start_index) {
                pin->set_generated_default_value(p_method.default_arguments[default_index]);
                pin->set_default_value(p_method.default_arguments[default_index++]);
            }
        }
        argument_index++;
    }

    if (_reference.method.flags & METHOD_FLAG_VARARG) {
        const uint32_t base_arg_count = _reference.method.arguments.size() + 1;
        for (int i = 0; i < _vararg_count; i++) {
            const String pin_name = vformat("%s%d", get_pin_prefix(), base_arg_count + i);
            create_pin(PD_Input, PT_Data, PropertyUtils::make_variant(pin_name));
        }
    }

    if (MethodUtils::has_return_value(p_method)) {
        Ref<OScriptNodePin> rv = create_pin(PD_Output, PT_Data, PropertyUtils::as("return_value", p_method.return_val));
        if (rv.is_valid()) {
            if (_is_return_value_labeled(rv)) {
                rv->set_label(p_method.return_val.class_name);
            } else {
                rv->hide_label();
            }
        }
    }

    if (_chainable && _chain && target.is_valid()) {
        create_pin(PD_Output, PT_Data, PropertyUtils::as("return_target", target->get_property_info()))->set_label("Target");
    }
}

bool OScriptNodeCallFunction::_has_execution_pins(const MethodInfo& p_method) const {
    if (MethodUtils::has_return_value(p_method) && is_vector_empty(p_method.arguments)) {
        const String method_name = p_method.name.capitalize();
        if (method_name.begins_with("Is ") || method_name.begins_with("Get ")) {
            return false;
        }
    }
    return true;
}

bool OScriptNodeCallFunction::_is_return_value_labeled(const Ref<OScriptNodePin>& p_pin) const {
    const bool is_enum = PropertyUtils::is_enum(p_pin->get_property_info());
    const bool is_bitfield = PropertyUtils::is_bitfield(p_pin->get_property_info());
    const bool is_object = p_pin->get_property_info().type == Variant::OBJECT;

    return is_object || is_enum || is_bitfield;
}

void OScriptNodeCallFunction::_set_function_flags(const MethodInfo& p_method) {
    if (p_method.flags & METHOD_FLAG_CONST) {
        _function_flags.set_flag(FF_CONST);
    }
    if (p_method.flags & METHOD_FLAG_VIRTUAL) {
        _function_flags.set_flag(FF_IS_VIRTUAL);
    }
    if (p_method.flags & METHOD_FLAG_STATIC) {
        _function_flags.set_flag(FF_STATIC);
    }
    if (p_method.flags & METHOD_FLAG_VARARG) {
        _function_flags.set_flag(FF_VARARG);
    }
}

int OScriptNodeCallFunction::get_argument_count() const {
    int arg_count = _reference.method.arguments.size();
    if (_reference.method.flags & METHOD_FLAG_VARARG) {
        arg_count += _vararg_count;
    }
    return arg_count;
}

void OScriptNodeCallFunction::reallocate_pins_during_reconstruction(const Vector<Ref<OScriptNodePin>>& p_old_pins) {
    super::reallocate_pins_during_reconstruction(p_old_pins);

    // Specifying default values on the call function node take precedence over the values defined on the
    // FunctionEntry node, and therefore since the create pins method does not set the default values on
    // the input pins, this allows us to copy the values from the old pins to the new ones created when
    // the node is reconstructed, so those values are not lost across load/save operations.
    //
    // This allows the pins to restore the default values without having to serialize those as any other
    // value held by this node.
    Vector<Ref<OScriptNodePin>> inputs = find_pins(PD_Input);
    for (int i = 2; i < inputs.size(); i++) {
        const Ref<OScriptNodePin>& input = inputs[i];
        for (const Ref<OScriptNodePin>& old_pin : p_old_pins) {
            if (old_pin->get_direction() == input->get_direction() && old_pin->get_pin_name() == input->get_pin_name()) {
                if (old_pin->get_property_info().type == input->get_property_info().type) {
                    input->set_generated_default_value(old_pin->get_generated_default_value());
                    input->set_default_value(old_pin->get_default_value());
                }
            }
        }
    }
}

void OScriptNodeCallFunction::post_initialize() {
    reconstruct_node();
    super::post_initialize();
}

void OScriptNodeCallFunction::allocate_default_pins() {
    _create_pins_for_method(get_method_info());
    super::allocate_default_pins();
}

void OScriptNodeCallFunction::initialize(const OScriptNodeInitContext& p_context) {
    ERR_FAIL_COND_MSG(_reference.method.name.is_empty(), "Function name not specified.");
    super::initialize(p_context);
}

bool OScriptNodeCallFunction::is_pure() const {
    // For now we say the node is pure when no execute pins are constructed
    OScriptNodeCallFunction* self = const_cast<OScriptNodeCallFunction*>(this);
    for (const Ref<OScriptNodePin>& pin : self->find_pins(PD_Input)) {
        if (pin->is_execution()) {
            return false;
        }
    }
    return true;
}

void OScriptNodeCallFunction::add_dynamic_pin() {
    _vararg_count++;
    reconstruct_node();
}

bool OScriptNodeCallFunction::can_remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin) const {
    if (p_pin.is_valid() && !p_pin->is_execution() && can_add_dynamic_pin()) {
        for (const PropertyInfo& pi : _reference.method.arguments) {
            if (pi.name.match(p_pin->get_pin_name())) {
                return false;
            }
        }
        return true;
    }
    return false;
}

void OScriptNodeCallFunction::remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin) {
    if (p_pin.is_valid() && can_remove_dynamic_pin(p_pin)) {
        int pin_offset = p_pin->get_pin_index();

        p_pin->unlink_all(true);
        remove_pin(p_pin);

        // Taken from OScriptNodeEditablePin::_adjust_connections
        get_orchestration()->adjust_connections(this, pin_offset, -1, PD_Input);

        _vararg_count--;
        reconstruct_node();
    }
}

void OScriptNodeCallFunction::_bind_methods() {
    BIND_ENUM_CONSTANT(FF_NONE)
    BIND_ENUM_CONSTANT(FF_PURE)
    BIND_ENUM_CONSTANT(FF_CONST)
    BIND_ENUM_CONSTANT(FF_IS_BEAD)
    BIND_ENUM_CONSTANT(FF_IS_SELF)
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OScriptNodeCallMemberFunction

void OScriptNodeCallMemberFunction::_upgrade(uint32_t p_version, uint32_t p_current_version) {
    if (p_version == 1 && p_current_version >= 2) {
        Ref<OScriptNodePin> target = find_pin("target", PD_Input);
        if (target.is_valid() && PropertyUtils::is_nil_no_variant(target->get_property_info())) {
            bool reconstruct = false;
            if (target->has_any_connections()) {
                // VariableGet missing class encoding - player.torch - _character property??

                Ref<OScriptNodePin> source = target->get_connections()[0];
                if (source.is_valid() && !source->get_property_info().class_name.is_empty()) {
                    const String target_class = source->get_property_info().class_name;
                    if (ClassDB::class_has_method(target_class, _reference.method.name)) {
                        _reference.target_class_name = target_class;
                        _reference.target_type = Variant::OBJECT;
                        reconstruct = true;
                    }
                } else {
                    // Can the target be resolved by traversing the target_class_name hierarchy
                    // If the method match is found, update the reference details
                    const String class_name = _get_method_class_hierarchy_owner(_reference.target_class_name, _reference.method.name);
                    if (!class_name.is_empty()) {
                        _reference.target_class_name = class_name;
                        _reference.target_type = Variant::OBJECT;
                        reconstruct = true;
                    }
                }
            } else {
                // No connections, traverse base type hierarchy
                const String class_name = _get_method_class_hierarchy_owner(get_orchestration()->get_base_type(), _reference.method.name);
                if (!class_name.is_empty()) {
                    _reference.target_class_name = class_name;
                    _reference.target_type = Variant::OBJECT;
                    reconstruct = true;
                }
            }

            if (reconstruct) {
                reconstruct_node();
            }
        }
    }

    super::_upgrade(p_version, p_current_version);
}

StringName OScriptNodeCallMemberFunction::_get_method_class_hierarchy_owner(const String& p_class_name, const String& p_method_name) {
    String class_name = p_class_name;
    while (!class_name.is_empty()) {
        TypedArray<Dictionary> methods;
        if (ScriptServer::is_global_class(class_name)) {
            methods = ScriptServer::get_global_class(class_name).get_method_list();
        }
        else {
            methods = ClassDB::class_get_method_list(class_name, true);
        }

        for (uint32_t i = 0; i < methods.size(); i++) {
            const Dictionary& dict = methods[i];
            if (dict.has("name") && p_method_name.match(dict["name"]))
                return class_name;
        }

        class_name = ClassDB::get_parent_class(class_name);
    }

    return "";
}

Ref<OScriptNodePin> OScriptNodeCallMemberFunction::_create_target_pin() {
    PropertyInfo property;
    property.type = _reference.target_type;
    property.name = "target";
    property.hint = PROPERTY_HINT_NONE;
    property.usage = PROPERTY_USAGE_DEFAULT;

    if (ClassDB::is_parent_class(_reference.target_class_name, RefCounted::get_class_static())) {
        property.hint = PROPERTY_HINT_RESOURCE_TYPE;
        property.hint_string = _reference.target_class_name;
    }

    if (property.type == Variant::OBJECT) {
        property.class_name = _reference.target_class_name;
    }

    // Create target pin
    Ref<OScriptNodePin> target = create_pin(PD_Input, PT_Data, property);
    if (target.is_valid()) {
        _function_flags.set_flag(FF_TARGET);
        if (property.type != Variant::OBJECT && !PropertyUtils::is_nil(property)) {
            target->set_label(VariantUtils::get_friendly_type_name(property.type));
            target->no_pretty_format();

            // Target pins should never accept default values as they're implied to be provided an instance
            // as a result of another node's output. So for example, to use a member function such as the
            // "get_as_property_path" function on "NodePath", construct a NodePath and then connect that
            // node to a member function call for "get_as_property_path".
            target->set_flag(OScriptNodePin::Flags::IGNORE_DEFAULT);
        } else if (!property.class_name.is_empty())
        {
            target->set_label(property.class_name);
            target->no_pretty_format();
        }

        _chainable = true;
        notify_property_list_changed();
    }

    return target;
}

String OScriptNodeCallMemberFunction::get_tooltip_text() const {
    if (!_reference.method.name.is_empty()) {
        return vformat("Calls the function '%s'", _reference.method.name);
    }
    return "Calls the specified function";
}

String OScriptNodeCallMemberFunction::get_node_title() const {
    if (!_reference.method.name.is_empty()) {
        return vformat("%s", _reference.method.name.capitalize());
    }
    return super::get_node_title();
}

String OScriptNodeCallMemberFunction::get_node_title_color_name() const {
    if (!ClassDB::class_exists(_reference.target_class_name)) {
        return "other_script_function_call";
    }
    return "function_call";
}

String OScriptNodeCallMemberFunction::get_help_topic() const {
    if (_reference.target_type != Variant::OBJECT) {
        BuiltInType type = ExtensionDB::get_builtin_type(_reference.target_type);
        return vformat("class_method:%s:%s", type.name, _reference.method.name);
    } else {
        const String class_name = MethodUtils::get_method_class(_reference.target_class_name, _reference.method.name);
        if (!class_name.is_empty()) {
            return vformat("class_method:%s:%s", class_name, _reference.method.name);
        }
    }
    return super::get_help_topic();
}

void OScriptNodeCallMemberFunction::initialize(const OScriptNodeInitContext& p_context) {
    MethodInfo mi;
    StringName target_class = get_orchestration()->get_base_type();
    Variant::Type target_type = Variant::NIL;
    if (p_context.user_data) {
        // Built-in types supply target_type (Variant.Type) and "method" (dictionary)
        const Dictionary& data = p_context.user_data.value();
        if (!data.has("target_type") && !data.has("method")) {
            ERR_FAIL_MSG("Cannot initialize member function node, missing 'target_type' and 'method'");
        }

        target_type = VariantUtils::to_type(data["target_type"]);
        mi = DictionaryUtils::to_method(data["method"]);
        target_class = "";
    } else if (p_context.method && p_context.class_name) {
        // Class-type member function call, includes 'class_name' and 'method' (MethodInfo)
        mi = p_context.method.value();
        target_class = p_context.class_name.value();
        target_type = Variant::OBJECT;
    } else {
        ERR_FAIL_MSG("Cannot initialize member function node, missing attributes.");
    }

    ERR_FAIL_COND_MSG(mi.name.is_empty(), "Failed to initialize CallMemberFunction without a MethodInfo");

    _reference.method = mi;
    _reference.target_type = target_type;
    _reference.target_class_name = target_class;

    _set_function_flags(_reference.method);

    super::initialize(p_context);
}

PackedStringArray OScriptNodeCallMemberFunction::get_suggestions(const Ref<OScriptNodePin>& p_pin) {
    if (p_pin.is_valid() && p_pin->is_input() && p_pin->get_pin_name().match("signal"))
    {
        const MethodInfo method = get_method_info();
        const bool is_object = Object::get_class_static().match(get_target_class());

        const bool object_connect = method.name.match("connect") && is_object;
        const bool object_disconnect = method.name.match("disconnect") && is_object;
        const bool object_is_connected = method.name.match("is_connected") && is_object;
        const bool object_emit_signal = method.name.match("emit_signal") && is_object;

        if (object_connect || object_disconnect || object_is_connected || object_emit_signal) {
            const Ref<OScriptNodePin> target_pin = find_pin("target", PD_Input);
            if (target_pin.is_valid()) {
                return target_pin->resolve_signal_names(true);
            }
        }
    }

    return super::get_suggestions(p_pin);
}

bool OScriptNodeCallMemberFunction::is_override() const {
    if (!_reference.target_class_name.is_empty()) {
        if (_reference.method.flags & METHOD_FLAG_VIRTUAL) {
            return true;
        }
    }
    return false;
}

OScriptNodeCallMemberFunction::OScriptNodeCallMemberFunction() {
    _flags = CATALOGABLE;
    _function_flags.set_flag(FF_IS_SELF);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OScriptNodeCallParentMemberFunction

Ref<OScriptNodePin> OScriptNodeCallParentMemberFunction::_create_target_pin() {
    // Calling parent is not chainable and target is implied as self
    _chainable = false;
    return nullptr;
}

String OScriptNodeCallParentMemberFunction::get_tooltip_text() const {
    if (!_reference.method.name.is_empty()) {
        return vformat("Calls the parent function '%s'", _reference.method.name);
    }
    return "Calls the specified parent function";
}

String OScriptNodeCallParentMemberFunction::get_node_title() const {
    return vformat("Parent: %s", super::get_node_title());
}

OScriptNodeCallParentMemberFunction::OScriptNodeCallParentMemberFunction() {
    _function_flags.set_flag(FF_SUPER);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OScriptNodeCallScriptFunction

void OScriptNodeCallScriptFunction::_on_function_changed() {
    if (_function.is_valid()) {
        _reference.method = _function->get_method_info();
    }
    reconstruct_node();
}

int OScriptNodeCallScriptFunction::get_argument_count() const {
    if (_function.is_valid()) {
        return _function->get_argument_count();
    }
    ERR_PRINT(vformat("Script function node has an invalid function %s", _reference.guid));
    return 0;
}

void OScriptNodeCallScriptFunction::post_initialize() {
    if (_reference.guid.is_valid()) {
        _function = get_orchestration()->find_function(_reference.guid);
        if (_function.is_valid()) {
            _reference.method = _function->get_method_info();
            _function_flags.set_flag(FF_IS_SELF);
            if (_is_in_editor()) {
                Callable callable = callable_mp_this(_on_function_changed);
                if (!_function->is_connected("changed", callable)) {
                    _function->connect("changed", callable);
                }
            }
        }
    }
    super::post_initialize();
}

void OScriptNodeCallScriptFunction::post_placed_new_node() {
    super::post_placed_new_node();
    if (_function.is_valid() && _is_in_editor()) {
        Callable callable = callable_mp_this(_on_function_changed);
        if (!_function->is_connected("changed", callable)) {
            _function->connect("changed", callable);
        }
    }
}

String OScriptNodeCallScriptFunction::get_tooltip_text() const {
    return vformat("Target is %s", get_orchestration()->get_base_type());
}

String OScriptNodeCallScriptFunction::get_node_title() const {
    if (_function.is_valid()) {
        return vformat("%s", _function->get_function_name().capitalize());
    }
    return super::get_node_title();
}

Object* OScriptNodeCallScriptFunction::get_jump_target_for_double_click() const {
    if (_function.is_valid()) {
        return _function.ptr();
    }
    return super::get_jump_target_for_double_click();
}

bool OScriptNodeCallScriptFunction::can_jump_to_definition() const {
    return get_jump_target_for_double_click() != nullptr;
}

bool OScriptNodeCallScriptFunction::can_inspect_node_properties() const {
    if (_function.is_valid() && !_function->get_function_name().is_empty()) {
        if (get_orchestration()->has_graph(_function->get_function_name())) {
            return true;
        }
    }
    return false;
}

void OScriptNodeCallScriptFunction::initialize(const OScriptNodeInitContext& p_context) {
    ERR_FAIL_COND_MSG(!p_context.method, "Failed to initialize CallScriptFunction without a MethodInfo");

    const MethodInfo& mi = p_context.method.value();
    _function = get_orchestration()->find_function(mi.name);
    if (_function.is_valid()) {
        _reference.guid = _function->get_guid();
        _reference.method = _function->get_method_info();
        _function_flags.set_flag(FF_IS_SELF);
    }

    super::initialize(p_context);
}

MethodInfo OScriptNodeCallScriptFunction::get_method_info() {
    if (_function.is_valid()) {
        return _function->get_method_info();
    }
    ERR_PRINT(vformat("Script function node has an invalid function %s", _reference.guid));
    return {};
}

bool OScriptNodeCallScriptFunction::is_override() const {
    if (get_orchestration()) {
        return get_orchestration()->is_function_override(_reference.method.name);
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OScriptNodeCallParentScriptFunction

String OScriptNodeCallParentScriptFunction::get_tooltip_text() const {
    if (!_reference.method.name.is_empty()) {
        return vformat("Calls the parent script function '%s'", _reference.method.name);
    }
    return "Calls the specified parent script function";
}

String OScriptNodeCallParentScriptFunction::get_node_title() const {
    return vformat("Parent: %s", super::get_node_title());
}

void OScriptNodeCallParentScriptFunction::initialize(const OScriptNodeInitContext& p_context) {
    super::initialize(p_context);

    // When callling super, we never want to imply self in this context
    if (get_function().is_valid() && _function_flags.has_flag(FF_IS_SELF)) {
        _function_flags.clear_flag(FF_IS_SELF);
    }
}

OScriptNodeCallParentScriptFunction::OScriptNodeCallParentScriptFunction() {
    _function_flags.set_flag(FF_SUPER);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OScriptNodeCallBuiltinFunction

bool OScriptNodeCallBuiltinFunction::_has_execution_pins(const MethodInfo& p_method) const {
    return !MethodUtils::has_return_value(p_method);
}

String OScriptNodeCallBuiltinFunction::get_tooltip_text() const {
    if (!_reference.method.name.is_empty()) {
        return vformat("Calls the built-in Godot function '%s'", _reference.method.name);
    }
    return "Calls the specified built-in Godot function";
}

String OScriptNodeCallBuiltinFunction::get_node_title() const {
    return vformat("%s", _reference.method.name.capitalize());
}

String OScriptNodeCallBuiltinFunction::get_help_topic() const {
    if (OScriptUtilityFunctions::function_exists(_reference.method.name)) {
        return vformat("class_method:@OScript:%s", _reference.method.name);
    }
    return vformat("class_method:@GlobalScope:%s", _reference.method.name);
}

void OScriptNodeCallBuiltinFunction::initialize(const OScriptNodeInitContext& p_context) {
    ERR_FAIL_COND_MSG(!p_context.user_data, "Failed to initialize BuiltInFunction without data");

    const Dictionary& data = p_context.user_data.value();
    ERR_FAIL_COND_MSG(!data.has("name"), "MethodInfo is incomplete.");

    const MethodInfo mi = DictionaryUtils::to_method(data);
    _reference.method = mi;

    _set_function_flags(_reference.method);

    super::initialize(p_context);
}

OScriptNodeCallBuiltinFunction::OScriptNodeCallBuiltinFunction() {
    _flags = CATALOGABLE;
    _function_flags = FF_PURE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OScriptNodeCallStaticFunction

void OScriptNodeCallStaticFunction::_upgrade(uint32_t p_version, uint32_t p_current_version) {
    if (MethodUtils::is_empty(_method)) {
        bool found = false;

        if (ExtensionDB::is_builtin_type(_class_name)) {
            const BuiltInType type = ExtensionDB::get_builtin_type(_class_name);
            for (const MethodInfo& method : type.get_method_list()) {
                if (method.name.match(_method_name)) {
                    found = true;
                    _method = method;
                    break;
                }
            }
        } else {
            TypedArray<Dictionary> methods;
            if (ScriptServer::is_global_class(_class_name)) {
                if (get_orchestration()->get_global_name() == _class_name) {
                    // Orchestrations can inherit from native classes with static methods, but does not
                    // currently support user-defined static methods, so this can safely look this up
                    // via the native class API.
                    const StringName native_class = ScriptServer::get_global_class_native_base(_class_name);
                    methods = ClassDB::class_get_method_list(native_class, true);
                } else {
                    ScriptServer::get_static_method_list(_class_name, &methods);
                }
            } else {
                methods = ClassDB::class_get_method_list(_class_name, true);
            }

            for (uint32_t i = 0; i < methods.size(); i++) {
                const Dictionary& dict = methods[i];
                if (_method_name.match(dict["name"])) {
                    found = true;
                    _method = DictionaryUtils::to_method(dict);
                    break;
                }
            }
        }

        if (!found) {
            ERR_FAIL_MSG(vformat("Failed to locate method for %s.%s", _class_name, _method_name));
        } else {
            reconstruct_node();
        }
    }

    super::_upgrade(p_version, p_current_version);
}

void OScriptNodeCallStaticFunction::_get_property_list(List<PropertyInfo>* r_list) const {
    r_list->push_back(PropertyInfo(Variant::STRING, "class_name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    if (MethodUtils::is_empty(_method)) {
        r_list->push_back(PropertyInfo(Variant::STRING, "function_name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    }
    r_list->push_back(PropertyInfo(Variant::DICTIONARY, "method", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
}

bool OScriptNodeCallStaticFunction::_get(const StringName& p_name, Variant& r_value) const {
    if (p_name.match("class_name")) {
        r_value = _class_name;
        return true;
    } else if (p_name.match("function_name")) {
        r_value = _method_name;
        return true;
    } else if (p_name.match("method")) {
        r_value = DictionaryUtils::from_method(_method);
        return true;
    }
    return false;
}

bool OScriptNodeCallStaticFunction::_set(const StringName& p_name, const Variant& p_value) {
    if (p_name.match("class_name")) {
        _class_name = p_value;
        return true;
    } else if (p_name.match("function_name")) {
        _method_name = p_value;
        return true;
    } else if (p_name.match("method")) {
        _method = DictionaryUtils::to_method(p_value);
        return true;
    }
    return false;
}

void OScriptNodeCallStaticFunction::post_initialize() {
    reconstruct_node();
    super::post_initialize();
}

void OScriptNodeCallStaticFunction::allocate_default_pins() {
    create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"));
    create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("ExecOut"));

    const size_t default_start_index = is_vector_empty(_method.arguments)
        ? 0
        : _method.arguments.size() - _method.default_arguments.size();

    size_t def_index = 0;
    for (size_t arg_index = 0; arg_index < _method.arguments.size(); arg_index++) {
        const PropertyInfo& pi = _method.arguments[arg_index];
        const Variant default_value = arg_index >= default_start_index ? _method.default_arguments[def_index++] : Variant();
        create_pin(PD_Input, PT_Data, pi, default_value);
    }

    if (MethodUtils::has_return_value(_method)) {
        Ref<OScriptNodePin> rvalue = create_pin(PD_Output, PT_Data, PropertyUtils::as("return_value", _method.return_val));
        if (_method.return_val.type == Variant::OBJECT) {
            rvalue->set_label(_method.return_val.class_name);
        } else {
            rvalue->hide_label();
        }
    }

    super::allocate_default_pins();
}

String OScriptNodeCallStaticFunction::get_tooltip_text() const {
    if (!_class_name.is_empty() && !_method.name.is_empty()) {
        return vformat("Calls the static function '%s.%s'", _class_name, _method.name);
    }
    return "Calls a static function";
}

String OScriptNodeCallStaticFunction::get_node_title() const {
    if (!_class_name.is_empty() && !_method.name.is_empty()) {
        return vformat("%s %s", _class_name, _method.name.capitalize());
    }
    return "Call Static Function";
}

String OScriptNodeCallStaticFunction::get_help_topic() const {
    const String class_name = MethodUtils::get_method_class(_class_name, _method.name);
    if (!class_name.is_empty()) {
        return vformat("class_method:%s:%s", class_name, _method.name);
    }
    return super::get_help_topic();
}

void OScriptNodeCallStaticFunction::initialize(const OScriptNodeInitContext& p_context) {
    ERR_FAIL_COND_MSG(!p_context.user_data, "Failed to initialize CallStaticFunction without user data");
    ERR_FAIL_COND_MSG(!p_context.method, "Method is missing");

    const Dictionary data = p_context.user_data.value();
    ERR_FAIL_COND_MSG(!data.has("class_name"), "Data is missing the class name.");

    _class_name = data["class_name"];
    _method = p_context.method.value();

    super::initialize(p_context);
}

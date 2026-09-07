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
#include "orchestration/function.h"

#include "common/dictionary_utils.h"
#include "common/method_utils.h"
#include "common/name_utils.h"
#include "common/property_utils.h"
#include "common/resource_utils.h"
#include "common/variant_utils.h"
#include "orchestration/annotation_registry.h"
#include "orchestration/nodes/call_function.h"
#include "orchestration/nodes/function_entry.h"
#include "orchestration/nodes/function_result.h"
#include "orchestration/nodes/local_variables.h"
#include "orchestration/orchestration.h"

TypedArray<Dictionary> OScriptFunction::_get_local_variables_internal() const {
    TypedArray<Dictionary> local_variables;
    for (const Ref<OScriptLocalVariable>& local_variable : _local_variables) {
        local_variables.push_back(ResourceUtils::get_storage_properties(local_variable));
    }
    return local_variables;
}

void OScriptFunction::_set_local_variables_internal(const TypedArray<Dictionary>& p_local_variables) {
    _local_variables.clear();

    const StringName class_name = OScriptLocalVariable::get_class_static();
    for (int i = 0; i < p_local_variables.size(); i++) {
        const Dictionary& data = p_local_variables[i];

        // Members are written directly rather than through the setters, so loading neither broadcasts
        // changes nor marks the orchestration as edited.
        Ref<OScriptLocalVariable> local_variable(memnew(OScriptLocalVariable));
        local_variable->_function = this;
        local_variable->_info = DictionaryUtils::to_property(ResourceUtils::get_storage_property(data, class_name, "info"));
        local_variable->_info.name = ResourceUtils::get_storage_property(data, class_name, "name");
        local_variable->_default_value = ResourceUtils::get_storage_property(data, class_name, "default_value");
        local_variable->_description = ResourceUtils::get_storage_property(data, class_name, "description");

        _local_variables.push_back(local_variable);
    }
}

int OScriptFunction::_find_local_variable_index(const StringName& p_name) const {
    for (int i = 0; i < _local_variables.size(); i++) {
        if (_local_variables[i]->get_variable_name() == p_name) {
            return i;
        }
    }
    return -1;
}

void OScriptFunction::_get_property_list(List<PropertyInfo> *r_list) const {
    r_list->push_back(PropertyInfo(Variant::STRING, "guid", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    r_list->push_back(PropertyInfo(Variant::DICTIONARY, "method", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    r_list->push_back(PropertyInfo(Variant::BOOL, "user_defined", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    r_list->push_back(PropertyInfo(Variant::INT, "id", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    r_list->push_back(PropertyInfo(Variant::ARRAY, "local_variables", PROPERTY_HINT_ARRAY_TYPE, Variant::get_type_name(Variant::DICTIONARY), PROPERTY_USAGE_STORAGE));

    r_list->push_back(PropertyInfo(Variant::STRING, "function_name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY | PROPERTY_USAGE_EDITOR));
    r_list->push_back(PropertyInfo(Variant::STRING, "built-in", PROPERTY_HINT_ENUM, "Yes,No", PROPERTY_USAGE_READ_ONLY | PROPERTY_USAGE_EDITOR));

    r_list->push_back(PropertyInfo(Variant::STRING, "description", PROPERTY_HINT_MULTILINE_TEXT, "", _user_defined ? PROPERTY_USAGE_DEFAULT : PROPERTY_USAGE_STORAGE));

    // Shown for every function: events accept warning suppression even though they cannot be RPCs.
    // Written only when non-empty so untouched functions serialize as before.
    r_list->push_back(PropertyInfo(Variant::ARRAY, "annotations", PROPERTY_HINT_NONE, "", _annotations.is_empty() ? PROPERTY_USAGE_EDITOR : PROPERTY_USAGE_DEFAULT));

    uint32_t usage = (_user_defined ? PROPERTY_USAGE_EDITOR : PROPERTY_USAGE_READ_ONLY | PROPERTY_USAGE_EDITOR);
    r_list->push_back(PropertyInfo(Variant::STRING, "Inputs/Outputs", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_CATEGORY));
    r_list->push_back(PropertyInfo(Variant::DICTIONARY, "inputs", PROPERTY_HINT_NONE, "", usage));
    r_list->push_back(PropertyInfo(Variant::DICTIONARY, "outputs", PROPERTY_HINT_NONE, "", usage));
}

bool OScriptFunction::_get(const StringName &p_name, Variant &r_value) {
    if (p_name.match("guid")) {
        r_value = _guid.to_string();
        return true;
    } else if (p_name.match("method")) {
        r_value = DictionaryUtils::from_method(_method, true);
        return true;
    } else if (p_name.match("id")) {
        r_value = _owning_node_id;
        return true;
    } else if (p_name.match("user_defined")) {
        r_value = _user_defined;
        return true;
    } else if (p_name.match("local_variables")) {
        r_value = _get_local_variables_internal();
        return true;
    } else if (p_name.match("description")) {
        r_value = _description;
        return true;
    } else if (p_name.match("annotations")) {
        r_value = _annotations.to_array();
        return true;
    } else if (p_name.match("built-in")) {
        r_value = _user_defined ? "No" : "Yes";
        return true;
    } else if (p_name.match("function_name")) {
        r_value = _method.name;
        return true;
    } else if (p_name.match("inputs")) {
        TypedArray<Dictionary> arguments;
        for (const PropertyInfo& property : _method.arguments) {
            arguments.push_back(DictionaryUtils::from_property(property));
        }
        r_value = arguments;
        return true;
    } else if (p_name.match("outputs")) {
        TypedArray<Dictionary> results;
        if (has_return_type()) {
            results.push_back(DictionaryUtils::from_property(_method.return_val));
        }
        r_value = results;
        return true;
    }
    return false;
}

bool OScriptFunction::_set(const StringName &p_name, const Variant &p_value)
{
    bool result = false;
    if (p_name.match("guid")) {
        _guid = Guid(p_value);
        result = true;
    } else if (p_name.match("method")) {
        _method = DictionaryUtils::to_method(p_value);
        _returns_value = MethodUtils::has_return_value(_method);

        // Cleanup the argument usage flags that were constructed incorrectly due to godot-cpp bug
        for (PropertyInfo& argument : _method.arguments) {
            if (argument.usage == 7) {
                argument.usage = PROPERTY_USAGE_DEFAULT;
            }
            // If "Any" (Variant::NIL) set the usage flag correctly
            if (PropertyUtils::is_nil_no_variant(argument)) {
                argument.usage |= PROPERTY_USAGE_NIL_IS_VARIANT;
            }
        }

        // Cleanup return value usage flags that were constructed incorrectly due to godot-cpp bug
        if (_method.return_val.usage == 7) {
            _method.return_val.usage = PROPERTY_USAGE_DEFAULT;
        }
        result = true;
    } else if (p_name.match("id")) {
        _owning_node_id = p_value;
        result = true;
    } else if (p_name.match("user_defined")) {
        _user_defined = p_value;
        result = true;
    } else if (p_name.match("local_variables")) {
        _set_local_variables_internal(p_value);
        result = true;
    } else if (p_name.match("description")) {
        _description = p_value;
        result = true;
    } else if (p_name.match("annotations")) {
        OScriptAnnotationList annotations;
        annotations.from_array(p_value);
        set_annotations(annotations.get_items());
        return true;
    } else if (p_name.match("inputs")) {
        // The inspector's argument editor adds and removes its own rows, and graph nodes that
        // reference this function reconstruct from "changed", so a property list refresh here
        // would only force a full inspector rebuild that discards in-progress edit state.
        set_arguments(p_value);
        return true;
    } else if (p_name.match("outputs")) {
        const TypedArray<Dictionary> results = p_value;
        if (results.is_empty()) {
            set_has_return_value(false);
        } else {
            set_return(DictionaryUtils::to_property(results[0]));
        }
        return true;
    }

    if (result) {
        emit_changed();
    }

    return result;
}

const StringName& OScriptFunction::get_function_name() const {
    return _method.name;
}

bool OScriptFunction::can_be_renamed() const {
    return _user_defined;
}

void OScriptFunction::rename(const StringName &p_new_name) {
    if (can_be_renamed() && (_method.name != p_new_name)) {
        _method.name = p_new_name;
        emit_changed();
    }
}

const Guid& OScriptFunction::get_guid() const {
    return _guid;
}

const MethodInfo& OScriptFunction::get_method_info() const {
    return _method;
}

bool OScriptFunction::is_user_defined() const {
    return _user_defined;
}

Orchestration* OScriptFunction::get_orchestration() const {
    return _orchestration;
}

int OScriptFunction::get_owning_node_id() const {
    return _owning_node_id;
}

Ref<OScriptNode> OScriptFunction::get_owning_node() const {
    return _orchestration->get_node(_owning_node_id);
}

Ref<OScriptNode> OScriptFunction::get_return_node() const {
    const Vector<Ref<OScriptNode>> nodes = get_return_nodes();
    return nodes.is_empty() ? Ref<OScriptNode>() : nodes[0];
}

Vector<Ref<OScriptNode>> OScriptFunction::get_return_nodes() const {
    Vector<Ref<OScriptNode>> results;

    const Ref<OScriptGraph> graph = get_function_graph();
    if (graph.is_valid()) {
        for (const Ref<OScriptNode>& node : graph->get_nodes()) {
            const Ref<OScriptNodeFunctionResult> result = node;
            if (result.is_valid()) {
                results.push_back(result);
            }
        }
    }
    return results;
}

Ref<OScriptGraph> OScriptFunction::get_function_graph() const {
    if (_orchestration->has_graph(get_function_name())) {
        return _orchestration->get_graph(get_function_name());
    }
    return {};
}

Ref<OScriptGraph> OScriptFunction::get_graph() const {
    // Check function graphs
    Ref<OScriptGraph> function_graph = get_function_graph();
    if (function_graph.is_valid()) {
        return function_graph;
    }

    // Check event graphs
    for (const Ref<OScriptGraph>& graph : _orchestration->get_graphs()) {
        if (graph->get_flags().has_flag(OScriptGraph::GF_EVENT) && graph->has_node(_owning_node_id)) {
            return graph;
        }
    }

    return {};
}

Dictionary OScriptFunction::to_dict() const {
    Dictionary result = DictionaryUtils::from_method(_method);
    result["_oscript_guid"] = _guid.to_string();
    result["_oscript_owning_node_id"] = _owning_node_id;
    return result;
}

size_t OScriptFunction::get_argument_count() const {
    return _method.arguments.size();
}

bool OScriptFunction::resize_argument_list(size_t p_new_size) {
    bool result = false;
    if (_user_defined) {
        const size_t current_size = get_argument_count();
        if (p_new_size > current_size) {
            _method.arguments.resize(p_new_size);
            for (size_t i = current_size; i < p_new_size; i++) {
                _method.arguments[i].name = "arg" + itos(i + 1);
                _method.arguments[i].type = Variant::NIL;
                _method.arguments[i].usage = PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT;
            }
            result = true;
        } else if (p_new_size < current_size) {
            _method.arguments.resize(p_new_size);
            result = true;
        }
    }

    if (result) {
        emit_changed();
        notify_property_list_changed();
    }
    return result;
}

void OScriptFunction::set_argument_type(size_t p_index, Variant::Type p_type) {
    if (_method.arguments.size() > p_index && _user_defined) {
        PropertyInfo& pi = _method.arguments[p_index];
        pi.type = p_type;

        // Function arguments set as "Any" type imply variant, using Variant::NIL
        if (PropertyUtils::is_nil(pi)) {
            pi.usage |= PROPERTY_USAGE_NIL_IS_VARIANT;
        } else {
            pi.usage &= ~PROPERTY_USAGE_NIL_IS_VARIANT;
        }

        emit_changed();
    }
}

void OScriptFunction::set_argument(size_t p_index, const PropertyInfo& p_property) {
    if (_method.arguments.size() > p_index && _user_defined) {
        _method.arguments[p_index] = p_property;
        emit_changed();
    }
}

void OScriptFunction::set_arguments(const TypedArray<Dictionary>& p_arguments) {
    if (_user_defined) {
        _method.arguments.clear();
        for (uint32_t i = 0; i < p_arguments.size(); i++) {
            const Variant& argument = p_arguments[i];
            _method.arguments.push_back(DictionaryUtils::to_property(argument));
        }
        emit_changed();
    }
}

void OScriptFunction::set_argument_name(size_t p_index, const StringName& p_name) {
    if (_method.arguments.size() > p_index && _user_defined) {
        _method.arguments[p_index].name = p_name;
        emit_changed();
    }
}

bool OScriptFunction::has_return_type() const {
    return _returns_value;
}

Variant::Type OScriptFunction::get_return_type() const {
    return _method.return_val.type;
}

void OScriptFunction::set_return_type(Variant::Type p_type) {
    if (_user_defined && _method.return_val.type != p_type) {
        if (_returns_value) {
            MethodUtils::set_return_value_type(_method, p_type);
        } else {
            MethodUtils::set_no_return_value(_method);
        }
        emit_changed();
    }
}

void OScriptFunction::set_return(const PropertyInfo& p_property) {
    if (_user_defined) {
        _method.return_val = p_property;
        _returns_value = MethodUtils::has_return_value(_method);

        if (_returns_value) {
            // Since function returns a value, if there is no result node, add one.
            // If the function entry exec pin is not yet wired, autowire it to the result node.
            Ref<OScriptNodeFunctionResult> result = get_return_node();
            if (!result.is_valid()) {
                const Ref<OScriptNodeFunctionEntry> entry = get_owning_node();
                const Vector2 position = entry->get_position() + Vector2(400, 0);

                OScriptNodeInitContext context;
                context.method = get_method_info();

                result = get_function_graph()->create_node<OScriptNodeFunctionResult>(context, position);

                const Ref<OScriptNodePin> entry_exec_out = entry->get_execution_pin();
                if (entry_exec_out.is_valid() && !entry_exec_out->has_any_connections() && result.is_valid()) {
                    get_function_graph()->link(entry->get_id(), 0, result->get_id(), 0);
                }
            }
        }

        emit_changed();
        notify_property_list_changed();
    }
}

void OScriptFunction::set_has_return_value(bool p_has_return_value) {
    if (_returns_value != p_has_return_value) {
        if (p_has_return_value) {
            MethodUtils::set_return_value(_method);
        } else {
            MethodUtils::set_no_return_value(_method);
        }
        _returns_value = p_has_return_value;
        emit_changed();
    }
}

void OScriptFunction::set_description(const String& p_description) {
    if (_description != p_description) {
        _description = p_description;
        emit_changed();
    }
}

void OScriptFunction::remove_argument(int p_index) {
    if (_orchestration && is_user_defined()) {
        // Unlink connections
        for (const Ref<OScriptNode>& node : _orchestration->get_nodes()) {
            const Ref<OScriptNodeCallScriptFunction> call_func = node;
            if (call_func.is_valid() && call_func->get_function() == this) {
                const Ref<OScriptNodePin> argument_pin = node->find_pin(p_index + 1, PD_Input);
                if (argument_pin.is_valid()) {
                    argument_pin->unlink_all();
                }
                _orchestration->adjust_connections(call_func.ptr(), p_index + 1, -1, PD_Input);
            }
        }

        const Ref<OScriptNodeFunctionEntry> entry = get_owning_node();
        if (entry.is_valid()) {
            const Ref<OScriptNodePin> argument_pin = entry->find_pin(p_index + 1, PD_Output);
            if (argument_pin.is_valid()) {
                argument_pin->unlink_all();
            }
            _orchestration->adjust_connections(entry.ptr(), p_index + 1, -1, PD_Output);
        }

        _method.arguments.remove_at(p_index);

        emit_changed();
    }
}

Error OScriptFunction::add_annotation(const OScriptAnnotation& p_annotation, String* r_reason) {
    const uint32_t target = _user_defined ? OScriptAnnotationRegistry::TARGET_FUNCTION : OScriptAnnotationRegistry::TARGET_EVENT;
    const Error result = _annotations.add(target, _method.return_val, p_annotation, r_reason);
    if (result == OK) {
        emit_changed();
    }
    return result;
}

void OScriptFunction::remove_annotation(int p_index) {
    if (_annotations.remove_at(p_index)) {
        emit_changed();
    }
}

void OScriptFunction::set_annotation_arguments(int p_index, const Array& p_arguments) {
    if (_annotations.set_arguments(p_index, p_arguments)) {
        emit_changed();
    }
}

void OScriptFunction::set_annotations(const Vector<OScriptAnnotation>& p_annotations) {
    // Whole-list replacement bypasses the cardinality check so a snapshot restores verbatim;
    // the parser reports any conflict at compile time.
    OScriptAnnotationList replacement;
    replacement.set_items(p_annotations);

    if (_annotations != replacement) {
        _annotations = replacement;
        emit_changed();
    }
}

bool OScriptFunction::has_local_variable(const StringName& p_name) const {
    return _find_local_variable_index(p_name) != -1;
}

Ref<OScriptLocalVariable> OScriptFunction::create_local_variable(const StringName& p_name, Variant::Type p_type) {
    ERR_FAIL_COND_V_MSG(!_orchestration, nullptr, "Cannot create local variable, function is not owned by an orchestration.");
    ERR_FAIL_COND_V_MSG(_orchestration->_has_instances(), nullptr, "Cannot create local variables, instances exist.");
    ERR_FAIL_COND_V_MSG(!String(p_name).is_valid_identifier(), nullptr, "Cannot create local variable, invalid name: " + p_name);
    ERR_FAIL_COND_V_MSG(has_local_variable(p_name), nullptr, "A local variable with that name already exists: " + p_name);
    ERR_FAIL_COND_V_MSG(!is_local_variable_name_available(p_name), nullptr, "A function argument with that name already exists: " + p_name);

    Ref<OScriptLocalVariable> local_variable(memnew(OScriptLocalVariable));
    local_variable->_function = this;
    local_variable->_info.name = p_name;
    local_variable->_info.type = p_type;
    local_variable->_info.hint = PROPERTY_HINT_NONE;
    local_variable->_info.hint_string = "";
    local_variable->_info.class_name = "";
    local_variable->_info.usage = PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_NIL_IS_VARIANT;
    local_variable->_default_value = VariantUtils::make_default(p_type);
    _local_variables.push_back(local_variable);

    emit_signal("local_variable_added", p_name);
    _orchestration->set_edited(true);

    return local_variable;
}

Ref<OScriptLocalVariable> OScriptFunction::duplicate_local_variable(const StringName& p_name) {
    ERR_FAIL_COND_V_MSG(!has_local_variable(p_name), nullptr, "Cannot duplicate local variable that does not exist: " + p_name);

    const Ref<OScriptLocalVariable> old_local_variable = find_local_variable(p_name);

    // The unique name must avoid both local variables and function arguments
    PackedStringArray names = get_local_variable_names();
    for (const PropertyInfo& argument : _method.arguments) {
        names.push_back(argument.name);
    }
    const String new_name = NameUtils::create_unique_name(p_name, names);

    Ref<OScriptLocalVariable> new_local_variable = create_local_variable(new_name, old_local_variable->get_info().type);
    ERR_FAIL_COND_V_MSG(!new_local_variable.is_valid(), nullptr, "Failed to create a new local variable with name: " + new_name);
    new_local_variable->copy_persistent_state(old_local_variable);

    return new_local_variable;
}

void OScriptFunction::remove_local_variable(const StringName& p_name) {
    const int index = _find_local_variable_index(p_name);
    ERR_FAIL_COND_MSG(index == -1, "Cannot remove a local variable that does not exist: " + p_name);

    // Nodes that reference the local variable go with it
    const Ref<OScriptGraph> graph = get_function_graph();
    if (graph.is_valid()) {
        Vector<int> node_ids;
        for (const Ref<OScriptNode>& node : graph->get_nodes()) {
            const Ref<OScriptNodeLocalVariable> local_variable_node = node;
            if (local_variable_node.is_valid() && local_variable_node->get_variable_name() == p_name) {
                node_ids.push_back(node->get_id());
            }
        }
        for (int node_id : node_ids) {
            _orchestration->remove_node(node_id);
        }
    }

    _local_variables.remove_at(index);

    emit_signal("local_variable_removed", p_name);
    if (_orchestration) {
        _orchestration->set_edited(true);
    }
}

Ref<OScriptLocalVariable> OScriptFunction::find_local_variable(const StringName& p_name) const {
    const int index = _find_local_variable_index(p_name);
    return index == -1 ? Ref<OScriptLocalVariable>() : _local_variables[index];
}

bool OScriptFunction::rename_local_variable(const StringName& p_old_name, const StringName& p_new_name) {
    if (p_old_name == p_new_name) {
        return false;
    }

    ERR_FAIL_COND_V_MSG(!_orchestration, false, "Cannot rename local variable, function is not owned by an orchestration.");
    ERR_FAIL_COND_V_MSG(_orchestration->_has_instances(), false, "Cannot rename local variable, instances exist.");
    ERR_FAIL_COND_V_MSG(!has_local_variable(p_old_name), false, "Cannot rename, no local variable exists with the old name: " + p_old_name);
    ERR_FAIL_COND_V_MSG(!String(p_new_name).is_valid_identifier(), false, "Cannot rename, local variable name is not valid: " + p_new_name);
    ERR_FAIL_COND_V_MSG(has_local_variable(p_new_name), false, "Cannot rename, a local variable already exists with the new name: " + p_new_name);
    ERR_FAIL_COND_V_MSG(!is_local_variable_name_available(p_new_name), false, "Cannot rename, a function argument already exists with the new name: " + p_new_name);

    find_local_variable(p_old_name)->set_variable_name(p_new_name);

    emit_signal("local_variable_renamed", p_old_name, p_new_name);
    _orchestration->set_edited(true);

    return true;
}

Vector<Ref<OScriptLocalVariable>> OScriptFunction::get_local_variables() const {
    return _local_variables;
}

PackedStringArray OScriptFunction::get_local_variable_names() const {
    PackedStringArray names;
    for (const Ref<OScriptLocalVariable>& local_variable : _local_variables) {
        names.push_back(local_variable->get_variable_name());
    }
    return names;
}

bool OScriptFunction::is_local_variable_name_available(const StringName& p_name) const {
    if (!String(p_name).is_valid_identifier() || has_local_variable(p_name)) {
        return false;
    }

    // GDScript rejects a local that shadows a function parameter
    for (const PropertyInfo& argument : _method.arguments) {
        if (argument.name == p_name) {
            return false;
        }
    }

    return true;
}

void OScriptFunction::_bind_methods() {
    ADD_SIGNAL(MethodInfo("local_variable_added", PropertyInfo(Variant::STRING_NAME, "name")));
    ADD_SIGNAL(MethodInfo("local_variable_removed", PropertyInfo(Variant::STRING_NAME, "name")));
    ADD_SIGNAL(MethodInfo("local_variable_renamed", PropertyInfo(Variant::STRING_NAME, "old_name"), PropertyInfo(Variant::STRING_NAME, "new_name")));
}
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
#include "script_instance.h"

#include "common/dictionary_utils.h"
#include "common/memory_utils.h"
#include "plugin/settings.h"
#include "script/nodes/variables/local_variable.h"
#include "script/script.h"

#include <iomanip>

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/mutex_lock.hpp>
#include <godot_cpp/templates/local_vector.hpp>

static int get_exec_pin_index_of_port(const Ref<OScriptNode>& p_node, int p_port, EPinDirection p_direction)
{
    int exec_index = 0;
    int port_index = 0;
    for (const Ref<OScriptNodePin>& pin : p_node->find_pins(p_direction))
    {
        if (port_index == p_port)
            break;

        port_index++;
        if (pin->is_execution())
            exec_index++;
    }
    return exec_index;
}

static int get_data_pin_index_of_port(const Ref<OScriptNode>& p_node, int p_port, EPinDirection p_direction)
{
    int data_index = 0;
    int port_index = 0;
    for (const Ref<OScriptNodePin>& pin : p_node->find_pins(p_direction))
    {
        if (port_index == p_port)
            break;

        port_index++;
        if (!pin->is_execution())
            data_index++;
    }
    return data_index;
}

static Ref<OScriptNodePin> get_data_pin_at_count_index(const Ref<OScriptNode>& p_node, int p_index,
                                                       EPinDirection p_direction)
{
    int node_index = 0;
    for (const Ref<OScriptNodePin>& pin : p_node->find_pins(p_direction))
    {
        if (pin->is_execution())
            continue;

        if (node_index == p_index)
            return pin;

        node_index++;
    }
    return {};
}

static GDExtensionScriptInstanceInfo2 init_script_instance_info()
{
    GDExtensionScriptInstanceInfo2 info;
    OScriptInstanceBase::init_instance(info);

    info.set_func = [](void* p_self, GDExtensionConstStringNamePtr p_name,
                       GDExtensionConstVariantPtr p_value) -> GDExtensionBool {
        OScriptInstanceBase::PropertyError r_error;
        return ((OScriptInstance*)p_self)->set(*((StringName*)p_name), *(const Variant*)p_value, &r_error);
    };

    info.get_func = [](void* p_self, GDExtensionConstStringNamePtr p_name,
                       GDExtensionVariantPtr p_value) -> GDExtensionBool {
        OScriptInstanceBase::PropertyError r_error;
        return ((OScriptInstance*)p_self)->get(*((StringName*)p_name), *(reinterpret_cast<Variant*>(p_value)), &r_error);
    };

    info.has_method_func = [](void* p_self, GDExtensionConstStringNamePtr p_name) -> GDExtensionBool {
        return ((OScriptInstance*)p_self)->has_method(*((StringName*)p_name));
    };

    info.property_can_revert_func = [](void* p_self, GDExtensionConstStringNamePtr p_name) -> GDExtensionBool {
        return ((OScriptInstance*)p_self)->property_can_revert(*((StringName*)p_name));
    };

    info.property_get_revert_func = [](void* p_self, GDExtensionConstStringNamePtr p_name,
                                       GDExtensionVariantPtr r_ret) -> GDExtensionBool {
        return ((OScriptInstance*)p_self)->property_get_revert(*((StringName*)p_name), (Variant*)r_ret);
    };

    info.call_func = [](void* p_self, GDExtensionConstStringNamePtr p_method, const GDExtensionConstVariantPtr* p_args,
                        GDExtensionInt p_argument_count, GDExtensionVariantPtr r_return, GDExtensionCallError* r_error) {
        return ((OScriptInstance*)p_self)
            ->call(*((StringName*)p_method), (const Variant**)p_args, p_argument_count, (Variant*)r_return, r_error);
    };

    info.notification_func = [](void* p_self, int32_t p_what, GDExtensionBool p_reversed) {
        ((OScriptInstance*)p_self)->notification(p_what, p_reversed);
    };

    info.free_func = [](void* p_self) {
        memdelete(((OScriptInstance*)p_self));
    };

    info.refcount_decremented_func = [](void*) -> GDExtensionBool {
        // If false (default), object cannot die
        return true;
    };

    return info;
}

const GDExtensionScriptInstanceInfo2 OScriptInstance::INSTANCE_INFO = init_script_instance_info();

OScriptInstance::OScriptInstance(const Ref<OScript>& p_script, OScriptLanguage* p_language, Object* p_owner)
    : _script(p_script)
    , _owner(p_owner)
    , _language(p_language)
{
    // Initialize variables
    _initialize_variables(p_script);

    // Initialize functions
    _initialize_functions(p_script);
}

OScriptInstance::~OScriptInstance()
{
    {
        MutexLock lock(*_language->lock.ptr());
        _script->_instances.erase(_owner);
    }

    // Cleanup node instances
    for (const KeyValue<int, OScriptNodeInstance*>& E : _nodes)
        memdelete(E.value);
}

bool OScriptInstance::set(const StringName& p_name, const Variant& p_value, PropertyError* r_err)
{
    const String variable_name = _get_variable_name_from_path(p_name);
    HashMap<StringName, Variant>::Iterator E = _variables.find(variable_name);
    if (!E || !_exported_vars.has(variable_name) || !_exported_vars[variable_name])
    {
        if (r_err)
            *r_err = PROP_NOT_FOUND;

        return false;
    }

    if (r_err)
        *r_err = PROP_OK;

    E->value = p_value;
    return true;
}

bool OScriptInstance::get(const StringName& p_name, Variant& p_value, PropertyError* r_err)
{
    const String variable_name = _get_variable_name_from_path(p_name);
    HashMap<StringName, Variant>::Iterator E = _variables.find(variable_name);
    if (!E || !_exported_vars.has(variable_name) || !_exported_vars[variable_name])
    {
        if (r_err)
            *r_err = PROP_NOT_FOUND;

        return false;
    }

    if (r_err)
        *r_err = PROP_OK;

    p_value = E->value;
    return true;
}

GDExtensionPropertyInfo* OScriptInstance::get_property_list(uint32_t* r_count)
{
    LocalVector<GDExtensionPropertyInfo> infos;
    for (const Ref<OScriptVariable>& variable : _script->get_variables())
    {
        // Only exported
        if (!variable->is_exported())
            continue;

        PropertyInfo info = variable->get_info();

        if (variable->is_grouped_by_category())
            info.name = vformat("%s/%s", variable->get_category(), info.name);

        const Dictionary property = DictionaryUtils::from_property(info);
        GDExtensionPropertyInfo pi = DictionaryUtils::to_extension_property(property);
        infos.push_back(pi);
    }

    *r_count = infos.size();
    GDExtensionPropertyInfo* list = MemoryUtils::memnew_with_size<GDExtensionPropertyInfo>(infos.size());
    memcpy(list, infos.ptr(), sizeof(GDExtensionPropertyInfo) * infos.size());
    return list;
}

Variant::Type OScriptInstance::get_property_type(const StringName& p_name, bool* r_is_valid) const
{
    if (!_variables.has(_get_variable_name_from_path(p_name)))
    {
        if (r_is_valid)
            *r_is_valid = false;
        ERR_FAIL_V(Variant::NIL);
    }

    if (r_is_valid)
        *r_is_valid = true;

    return _script->get_variable(p_name)->get_variable_type();
}

bool OScriptInstance::has_method(const StringName& p_name) const
{
    return _script->has_function(p_name);
}

Object* OScriptInstance::get_owner() const
{
    return _owner;
}

Ref<OScript> OScriptInstance::get_script() const
{
    return _script;
}

ScriptLanguage* OScriptInstance::get_language() const
{
    return _language;
}

bool OScriptInstance::is_placeholder() const
{
    return false;
}

String OScriptInstance::get_base_type() const
{
    return _script->get_base_type();
}

void OScriptInstance::set_base_type(const String& p_base_type)
{
    _script->set_base_type(p_base_type);
}

bool OScriptInstance::property_can_revert(const StringName& p_name)
{
    // Only applicable for Editor
    return false;
}

bool OScriptInstance::property_get_revert(const StringName& p_name, Variant* r_ret)
{
    // Only applicable for Editor
    return false;
}

void OScriptInstance::notification(int32_t p_what, bool p_reversed)
{
}

void OScriptInstance::to_string(GDExtensionBool* r_is_valid, String* r_out)
{
    *r_is_valid = true;

    if (r_out)
    {
        std::stringstream ss;
        ss << "OrchestratorScriptInstance[" << _script->get_path().utf8().get_data() << "]:" << std::hex << this;
        *r_out = ss.str().c_str();
    }
}

bool OScriptInstance::get_variable(const StringName& p_name, Variant& r_value) const
{
    if (_variables.has(_get_variable_name_from_path(p_name)))
    {
        r_value = _variables.get(p_name);
        return true;
    }
    return false;
}

bool OScriptInstance::set_variable(const StringName& p_name, const Variant& p_value)
{
    if (_variables.has(_get_variable_name_from_path(p_name)))
    {
        _variables[p_name] = p_value;
        return true;
    }
    return false;
}

OScriptInstance* OScriptInstance::from_object(GDExtensionObjectPtr p_object)
{
    return nullptr;
}

void OScriptInstance::_initialize_variables(const Ref<OScript>& p_script)
{
    for (const KeyValue<StringName, Ref<OScriptVariable>>& E : p_script->_variables)
    {
        _variables[E.key] = E.value->get_default_value();
        _exported_vars[E.key] = E.value->is_exported();
    }
}

void OScriptInstance::_initialize_functions(const Ref<OScript>& p_script)
{
    // Regardless of the node definitions in the OScript, the runtime is only concerned about function
    // definition nodes, as these define entry points into the script's logic. These nodes can be any
    // nodes such as overridden functions, event handlers, or even user-defined functions.
    for (const KeyValue<StringName, Ref<OScriptFunction>>& E : p_script->_functions)
    {
        const Ref<OScriptFunction>& script_function = E.value;
        if (!script_function.is_valid())
        {
            _language->debug_break_parse(p_script->get_path(), 0, "Function '" + String(E.key) + "' is invalid.");
            ERR_CONTINUE(script_function.is_valid());
        }

        // Construct a Function reference to the script function
        Function function;
        function.node = script_function->get_owning_node_id();
        function.argument_count = script_function->get_argument_count();
        function.max_stack = function.argument_count;
        function.flow_stack_size = 256;

        HashMap<String, int> lv_indices;

        if (function.node < 0)
        {
            _language->debug_break_parse(p_script->get_path(), 0, "No start node for function: " + String(E.key));
            ERR_CONTINUE(function.node < 0);
        }

        Ref<OScriptNode> node = script_function->get_owning_node();
        if (node.is_null())
        {
            _language->debug_break_parse(p_script->get_path(), 0, "Invalid script node with id: " + itos(function.node));
            ERR_CONTINUE(node.is_valid());
        }

        // Adjust the maximum input arguments based on function calls
        _max_input_args = Math::max(_max_input_args, function.argument_count);

        _initialize_function_node_graph(p_script, function, lv_indices);

        _functions[E.key] = function;
    }

}

bool OScriptInstance::_initialize_function_node_graph(const Ref<OScript>& p_script, Function& p_function,
                                                      HashMap<String, int>& p_lv_indices)
{
    RBSet<OScriptConnection> exec_pins;
    RBSet<OScriptConnection> data_pins;

    RBSet<int> node_ids;
    node_ids.insert(p_function.node);

    List<int> queue;
    queue.push_back(p_function.node);
    while (!queue.is_empty())
    {
        // Capture execution connections
        RBSet<OScriptConnection> exec_connections;
        for (const OScriptConnection& F : p_script->_connections)
        {
            const Ref<OScriptNodePin>& pin = p_script->get_node(F.from_node)->find_pin(F.from_port, PD_Output);
            if (pin.is_valid() && pin->is_execution())
                exec_connections.insert(F);
        }

        // Traverse execution paths
        for (const OScriptConnection& F : exec_connections)
        {
            if (queue.front()->get() == F.from_node && !node_ids.has(F.to_node))
            {
                queue.push_back(F.to_node);
                node_ids.insert(F.to_node);
            }
            if (queue.front()->get() == F.from_node && !exec_pins.has(F))
                exec_pins.insert(F);
        }
        queue.pop_front();
    }

    // Create data connection lookup table
    HashMap<int, HashMap<int, Pair<int, int>>> data_conn_lookups;
    for (const OScriptConnection& F : p_script->_connections)
    {
        Ref<OScriptNode> source_node = p_script->get_node(F.from_node);
        Ref<OScriptNode> target_node = p_script->get_node(F.to_node);
        if (source_node.is_valid() && target_node.is_valid())
        {
            const Ref<OScriptNodePin>& pin = source_node->find_pin(F.from_port, PD_Output);
            if (pin.is_valid())
            {
                if (pin->is_execution())
                    continue;

                int from_pin_index = get_data_pin_index_of_port(source_node, F.from_port, PD_Output);
                int to_pin_index = get_data_pin_index_of_port(target_node, F.to_port, PD_Input);
                data_conn_lookups[F.to_node][to_pin_index] = Pair<int, int>(F.from_node, from_pin_index);
            }
        }
    }

    // Push all nodes onto the queue
    // This creates the node execution order for the function call
    for (const int& F : node_ids)
        queue.push_back(F);

    // Iterate queue and create data pin connection lists
    while (!queue.is_empty())
    {
        int key = queue.front()->get();
        for (const KeyValue<int, Pair<int, int>>& F : data_conn_lookups[key])
        {
            OScriptConnection connection;
            connection.from_node = F.value.first;
            connection.from_port = F.value.second;
            connection.to_node = key;
            connection.to_port = F.key;

            data_pins.insert(connection);
            queue.push_back(connection.from_node);
            node_ids.insert(connection.from_node);
        }
        queue.pop_front();
    }

    // The function node graph is a complex step that requires multiple passes
    // In this section we execute the various passes to build the graph

    // Step 1
    for (const int& F : node_ids)
    {
        if (!_create_node_instance(p_script, F, p_function, p_lv_indices))
        {
            _language->debug_break_parse(p_script->get_path(), 0, "Failed to create node instance for node " + itos(F));
            ERR_CONTINUE(false);
        }
    }

    // Step 2
    // Create the data connections
    // We currently do this inline here because OScript::Connection cannot be visible in the OScriptInstance header.
    for (const OScriptConnection& F : data_pins)
    {
        OScriptConnection c = F;

        ERR_CONTINUE(!_nodes.has(c.from_node));
        OScriptNodeInstance* source = _nodes[c.from_node];

        ERR_CONTINUE(!_nodes.has(c.to_node));
        OScriptNodeInstance* target = _nodes[c.to_node];

        ERR_CONTINUE(c.from_port > source->output_pin_count);
        ERR_CONTINUE(c.to_port > target->input_pin_count);

        // Check whether the source output pin is -1, which means we can assign the value
        // If it doesn't match -1, then another node is already connected and we need to
        // ignore the additional connection as it won't work for data flow.
        if (source->output_pins[c.from_port] == -1)
        {
            int stack_pos = p_function.max_stack++;
            source->output_pins[c.from_port] = stack_pos;
        }

        // If the node we are reading from has no output execution pins, step() must be called
        // before we can read it. Additionally, if it's a dependency, it needs to be added as a
        // dependency of the target.
        if (source->execution_output_pin_count == 0 && target->dependencies.find(source) == -1)
        {
            if (source->pass_index == -1)
            {
                source->pass_index = p_function.pass_stack_size;
                p_function.pass_stack_size++;
            }
            target->dependencies.push_back(source);
        }

        // Read from where ever the stack is
        target->input_pins[c.to_port] = source->output_pins[c.from_port];
    }

    // This must be done at this point, order in the stack is critical
    p_function.trash_pos = p_function.max_stack++;

    // Pass 3
    // Create the execution connections
    // We currently do this inline here because OScript::Connection cannot be visible in the OScriptInstance header.
    for (const OScriptConnection& F : exec_pins)
    {
        OScriptConnection c = F;

        ERR_CONTINUE(!_nodes.has(c.from_node));
        OScriptNodeInstance* source = _nodes[c.from_node];

        ERR_CONTINUE(!_nodes.has(c.to_node));
        OScriptNodeInstance* target = _nodes[c.to_node];

        ERR_CONTINUE(c.from_port >= source->output_pin_count);

        int from_pin_index = get_exec_pin_index_of_port(source->get_base_node(), c.from_port, PD_Output);
        int to_pin_index = get_exec_pin_index_of_port(target->get_base_node(), c.to_port, PD_Input);

        source->execution_outputs[from_pin_index] = target;
        source->execution_output_pins[from_pin_index] = to_pin_index;
    }

    // Pass 4
    // Unassigned input pins are set as their default values
    // Connect unassigned output pins to the trash position
    for (const int& F : node_ids)
    {
        ERR_CONTINUE(!_nodes.has(F));

        const Ref<OScriptNode>& node = p_script->_nodes[F];
        const OScriptNodeInstance* instance = _nodes[F];

        _set_unassigned_inputs(node, instance);
        _set_unassigned_outputs(instance, p_function);
    }

    return true;
}

bool OScriptInstance::_create_node_instance(const Ref<OScript>& p_script, int p_node_id, Function& p_function,
                                            HashMap<String, int>& p_lv_indices)
{
    const Ref<OScriptNode>& node = p_script->_nodes[p_node_id];

    OScriptNodeInstance* instance = node->instantiate(this);
    ERR_FAIL_COND_V(!instance, false);

    instance->_base = node.ptr();
    instance->id = p_node_id;
    instance->execution_index = p_function.node_count++;
    instance->pass_index = -1;

    if (!_create_node_instance_pins(node, instance))
        return false;

    Ref<OScriptNodeLocalVariable> lv_node = node;
    Ref<OScriptNodeAssignLocalVariable> alv_node = node;
    if (lv_node.is_valid() || alv_node.is_valid())
    {
        // For local variables and assignments to local variables, we need to assign a pointer into the
        // stack to where these nodes can share data so that changes made by the assignment can then be
        // propagated when other nodes retrieve the value of the local variable. We do this by using a
        // map table to map both node types to the same stack position offset; however, these mappings
        // need to be controlled based on the variable reference.
        //
        // Local variables in Orchestrator are not assigned names, but instead the Local Variable node
        // auto-generates a unique GUID. We use this GUID as the basis for both the assignment and the
        // get nodes to cross-reference into the map table so that an assignment that refers to a
        // given local variable node share the same memory offset into the stack.
        const String lv_name = alv_node.is_valid() ? alv_node->get_variable_guid() : lv_node->get_variable_guid();
        if (!p_lv_indices.has(lv_name))
        {
            // First time local variable GUID has been detected, register it
            p_lv_indices[lv_name] = p_function.max_stack;
            p_function.max_stack++;
        }
        // Assign working memory reference
        instance->working_memory_index = p_lv_indices[lv_name];
    }
    else if (instance->get_working_memory_size())
    {
        instance->working_memory_index = p_function.max_stack;
        p_function.max_stack += instance->get_working_memory_size();
    }
    else
    {
        // The node does not require any working memory
        instance->working_memory_index = -1;
    }

    // Recalculate the max input/output pins
    _max_input_args = Math::max(_max_input_args, instance->data_input_pin_count);
    _max_output_args = Math::max(_max_output_args, instance->data_output_pin_count);

    _nodes[p_node_id] = instance;
    return true;
}

bool OScriptInstance::_create_node_instance_pins(const Ref<OScriptNode>& p_node, OScriptNodeInstance* p_instance)
{
    for (const Ref<OScriptNodePin>& pin : p_node->get_all_pins())
    {
        // todo:    typically hidden pins would carry information that is generated
        //          by the node at build time, so why do we ignore them here?
        if (pin->get_flags().has_flag(OScriptNodePin::Flags::HIDDEN))
            continue;

        switch (pin->get_direction())
        {
            case PD_Input:
            {
                p_instance->input_pin_count++;
                if (pin->is_execution())
                    p_instance->execution_input_pin_count++;
                break;
            }
            case PD_Output:
            {
                p_instance->output_pin_count++;
                if (pin->is_execution())
                    p_instance->execution_output_pin_count++;
                break;
            }
            default:
                ERR_PRINT("An unexpected pin direction found: " + itos(pin->get_direction()));
                return false;
        }
    }

    p_instance->data_input_pin_count = p_instance->input_pin_count - p_instance->execution_input_pin_count;
    p_instance->data_output_pin_count = p_instance->output_pin_count - p_instance->execution_output_pin_count;

    if (p_instance->data_input_pin_count)
    {
        // Create the input pin array
        // Each defaults to -1, and if left as -1 signals to replace with default values
        p_instance->input_pins = memnew_arr(int, p_instance->data_input_pin_count);
        for (int i = 0; i < p_instance->data_input_pin_count; i++)
            p_instance->input_pins[i] = -1;
    }

    if (p_instance->data_output_pin_count)
    {
        // Creates the output pin array
        // Each defaults to -1, and if left as -1 signals the output writes to the trash pointer
        // Trash basically means the output is unused, so discard it
        p_instance->output_pins = memnew_arr(int, p_instance->data_output_pin_count);
        for (int i = 0; i < p_instance->data_output_pin_count; i++)
            p_instance->output_pins[i] = -1;
    }

    if (p_instance->execution_output_pin_count)
    {
        // Creates the output execution instance references array
        // The values are initialized as null and if it remains as null, execution is meant to end
        p_instance->execution_outputs = memnew_arr(OScriptNodeInstance*, p_instance->execution_output_pin_count);
        p_instance->execution_output_pins = memnew_arr(int, p_instance->execution_output_pin_count);
        for (int i = 0; i < p_instance->execution_output_pin_count; i++)
        {
            p_instance->execution_outputs[i] = nullptr;
            p_instance->execution_output_pins[i] = -1;
        }
    }
    return true;
}

void OScriptInstance::_set_unassigned_inputs(const Ref<OScriptNode>& p_node, const OScriptNodeInstance* p_instance)
{
    for (int i = 0; i < p_instance->data_input_pin_count; i++)
    {
        if (p_instance->input_pins[i] == -1)
        {
            p_instance->input_pins[i] = _default_values.size() | OScriptNodeInstance::INPUT_DEFAULT_VALUE_BIT;

            const Ref<OScriptNodePin> pin = get_data_pin_at_count_index(p_node, i, PD_Input);
            if (pin.is_valid())
                _default_values.push_back(pin->get_effective_default_value());
        }
    }
}

void OScriptInstance::_set_unassigned_outputs(const OScriptNodeInstance* p_instance,
                                              const OScriptInstance::Function& p_function)
{
    for (int i = 0; i < p_instance->data_output_pin_count; i++)
    {
        if (p_instance->output_pins[i] == -1)
            p_instance->output_pins[i] = p_function.trash_pos;
    }
}

void OScriptInstance::call(const StringName& p_method, const Variant* const* p_args, GDExtensionInt p_arg_count,
                           Variant* r_return, GDExtensionCallError* r_err)
{
    // Reset the error as the engine doesn't reset this upon call
    r_err->error = GDEXTENSION_CALL_OK;

    // We first check whether this function is defined as part of the Orchestration
    // If it is, we invoke that variant of the method.
    HashMap<StringName, Function>::Iterator F = _functions.find(p_method);
    if (F)
    {
        Function* f = &F->value;

        // Lookup node that offers the function call
        HashMap<int, OScriptNodeInstance*>::Iterator E = _nodes.find(f->node);
        if (!E)
        {
            r_err->error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;
            ERR_FAIL_MSG("Unable to locate node for method '" + p_method + "' with id " + itos(f->node));
        }

        // settings/runtime/max_call_stack
        OrchestratorSettings* os = OrchestratorSettings::get_singleton();
        int max_call_stack = os->get_setting("settings/runtime/max_call_stack");

        // Setup the OScriptExecutionStackInfo object
        OScriptExecutionStackInfo si;
        si.max_stack_size = max_call_stack;  //! Max Call Stack
        si.node_count = f->node_count;       //! Number of nodes
        si.max_inputs = _max_input_args;     //! max input arguments
        si.max_outputs = _max_output_args;   //! max output arguments
        si.flow_size = f->flow_stack_size;   //! flow size
        si.pass_size = f->pass_stack_size;   //! pass stack size

        // Setup the stack
        Ref<OScriptExecutionStack> stack(memnew(OScriptExecutionStack(si)));
        stack->push_node_onto_flow_stack(f->node);
        stack->push_arguments(p_args, p_arg_count);

        // Dispatch the call to the internal handler
        *r_return = _call_internal(p_method, stack, 0, 0, false, E->value, *r_err);
    }
    else if (_script->has_method(p_method))
    {
        // Calling a native method
        Array args;
        for (int i = 0; i < p_arg_count; i++)
            args.push_back(*p_args[i]);

        r_err->error = GDEXTENSION_CALL_OK;
        *r_return = _script->callv(p_method, args);
    }
    else
    {
        // Method invalid
        r_err->error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;
        *r_return = Variant();
    }
}

Variant OScriptInstance::_call_internal(const StringName& p_method, const Ref<OScriptExecutionStack>& p_stack, int p_flow_pos,
                                        int p_passes, bool p_resume_yield, OScriptNodeInstance* p_node,
                                        GDExtensionCallError& r_err)
{
    // Check that the method is defined in the function map
    // If the function isn't, we fail and return immediately
    HashMap<StringName, Function>::Iterator F = _functions.find(p_method);
    ERR_FAIL_COND_V(!F, Variant());

    Variant return_value;

    OScriptNodeInstance* node = p_node;
    int node_port = 0; // always assumes 0
    Function* f = &F->value;

    OScriptExecutionContext context(p_stack, f->node, p_passes, p_flow_pos, &r_err);

    while (node)
    {
        // Track the current node
        context.set_current_node(node->get_id());
        context.set_current_node_port(node_port);

        // Keep track of the number of iterations in the flow
        context.increment_passes();

        _resolve_inputs(context, node, f);
        if (context.has_error())
        {
            const String message = vformat("Script call error #%d with Node %d: %s",
                                           context.get_error().error, context.get_current_node(),
                                           context.get_error_reason());
            ERR_PRINT(message);
            return return_value;
        }

        // Initialize working memory
        // This must be initialized after the input resolution since any dependency chain
        // sets this variable and it must be set explicitly for this call afterward.
        context.set_working_memory(node->working_memory_index);

        // Setup outputs
        _copy_stack_to_node_outputs(context, node);

        _resolve_step_mode(context, p_resume_yield);

        // Pre-initialize this
        context.clear_error();

        // Execute the node's step and check for errors
        int result = _execute_step(context, node);
        if (context.has_error())
            break;

        if (result & OScriptNodeInstance::STEP_FLAG_YIELD)
        {
            // Yielded
            if (node->get_working_memory_size() == 0)
            {
                context.set_error(GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT, "Yielded without working memory.");
                break;
            }

            // Ref<OrchestratorScriptState> state = *context.working_memory;
            Ref<OScriptState> state = context.get_working_memory();
            if (!state.is_valid())
            {
                context.set_error(GDEXTENSION_CALL_ERROR_INVALID_METHOD, "Node yielded without working memory state.");
                break;
            }

            state->instance_id = get_owner()->get_instance_id();
            state->script_id = get_script()->get_instance_id();
            state->instance = this;
            state->function = p_method;
            state->working_mem_index = node->working_memory_index;
            state->variant_stack_size = f->max_stack;
            state->node = node;
            state->flow_stack_pos = context.get_flow_stack_position();
            state->stack = p_stack;
            state->pass = context.get_passes();
            context.set_error(GDEXTENSION_CALL_OK);
            return state;
        }

        // Calculate the output node
        int output = result & OScriptNodeInstance::STEP_MASK;

        // Check whether the function exited/ended
        if (result & OScriptNodeInstance::STEP_FLAG_END)
        {
            if (node->get_working_memory_size() == 0)
            {
                context.set_error(GDEXTENSION_CALL_ERROR_INVALID_METHOD,
                                  "Return value must be assigned to first element of node's working memory");
            }
            else
            {
                // Assign first element from working memory
                return_value = context.get_working_memory();
            }
            break;
        }

        // Resolve the next node in the graph
        OScriptNodeInstance* next = _resolve_next_node(context, node, result, output);
        if (context.has_error())
            break;

        int next_port = _resolve_next_node_port(context, node, result, output, next);

        if (context.has_flow_stack())
        {
            // Update the flow stack, it may have changed
            context.set_flow_stack(context.get_current_node());

            _set_node_flow_execution_state(context, node, result);

            if (result & OScriptNodeInstance::STEP_FLAG_GO_BACK_BIT)
            {
                // If the flow position is less-than or equal to 0, cannot go back, exit gracefully
                if (context.get_flow_stack_position() <= 0)
                    break;

                context.decrement_flow_stack_position();
                node = _nodes[context.get_flow_stack_value() & OScriptNodeInstance::FLOW_STACK_MASK];
                node_port = 0; // ??
            }
            else if (next)
            {
                if (context.has_node_executed(next->execution_index))
                {
                    // Enter a node that is in the middle of doing a sequence (pushed stack)
                    // operation from the front because each node has working memory. We
                    // can't really do a sub-sequence as a result, the sequence will be
                    // restarted and the stack will roll back to find where this node began
                    // the sequence.
                    bool found = false;
                    for (int i = context.get_flow_stack_position(); i >= 0; i--)
                    {
                        if ((context.get_flow_stack_value(i) & OScriptNodeInstance::FLOW_STACK_MASK) == next->get_id())
                        {
                            // Roll back and remove bit
                            context.set_flow_stack_position(i);
                            context.set_flow_stack(next->get_id());
                            context.set_node_execution_state(next->execution_index, false);
                            found = true;
                        }
                    }

                    if (!found)
                    {
                        context.set_error(GDEXTENSION_CALL_ERROR_INVALID_METHOD,
                                          "Found execution bit but not the node in the stack, likely a bug?");
                        break;
                    }

                    node = next;
                    node_port = next_port;
                }
                else
                {
                    // Check for overflow
                    if (context.get_flow_stack_position() + 1 >= context.get_flow_stack_size())
                    {
                        context.set_error(GDEXTENSION_CALL_ERROR_INVALID_METHOD, "Stack overflow");
                        break;
                    }

                    node = next;
                    node_port = next_port;

                    context.increment_flow_stack_position();
                    context.set_flow_stack(node->get_id());
                }
            }
            else
            {
                // No next node, try to go back in stack and push bit.
                bool found = false;
                for (int i = context.get_flow_stack_position(); i >= 0; i--)
                {
                    int flow_stack_value = context.get_flow_stack_value(i);
                    if (flow_stack_value & OScriptNodeInstance::FLOW_STACK_PUSHED_BIT)
                    {
                        node = _nodes[flow_stack_value & OScriptNodeInstance::FLOW_STACK_MASK];
                        context.set_flow_stack_position(i);
                        found = true;
                        break;
                    }
                }
                // Could not find a push stack bit, exit
                if (!found)
                    break;
            }
        }
        else
        {
            // Stackless mode, simply assign next node
            node = next;
            node_port = next_port;
        }
    }

    if (context.has_error())
        _report_error(context, node, p_method);

    // Clean-up variant stack
    context.cleanup();
    return return_value;
}

void OScriptInstance::_resolve_inputs(OScriptExecutionContext& p_context, OScriptNodeInstance* p_node,
                                      Function* p_function)
{
    if (p_context.is_initial_node())
    {
        p_context.copy_stack_to_inputs(p_function->argument_count);
        return;
    }

    // If node has dependencies, resolve those first
    if (!p_node->dependencies.is_empty())
    {
        int count = p_node->dependencies.size();
        OScriptNodeInstance** deps = p_node->dependencies.ptrw();
        for (int i = 0; i < count; i++)
        {
            _dependency_step(p_context, deps[i], &p_node);
            if (p_context.has_error())
            {
                p_context.set_current_node(p_node->id);
                break;
            }
        }

        if (p_context.has_error())
            return;
    }

    // Setup input arguments
    for (int i = 0; i < p_node->data_input_pin_count; i++)
    {
        int index = p_node->input_pins[i] & OScriptNodeInstance::INPUT_MASK;
        if (p_node->input_pins[i] & OScriptNodeInstance::INPUT_DEFAULT_VALUE_BIT)
            p_context.set_input_from_default_value(i, _default_values[index]);
        else
            p_context.copy_stack_to_input(index, i);
    }
}

void OScriptInstance::_resolve_step_mode(OScriptExecutionContext& p_context, bool& p_resume_yield)
{
    if (p_resume_yield)
    {
        p_context.set_step_mode(OScriptNodeInstance::STEP_MODE_RESUME);
        p_resume_yield = false;
    }
    else if (p_context.has_flow_stack_bit(OScriptNodeInstance::FLOW_STACK_PUSHED_BIT))
    {
        // A node had a flow stack bit pushed, meaning we re-execute the node a subsequent time
        p_context.set_step_mode(OScriptNodeInstance::STEP_MODE_CONTINUE);
    }
    else
    {
        // Start from the beginning
        p_context.set_step_mode(OScriptNodeInstance::STEP_MODE_BEGIN);
    }
}

void OScriptInstance::_report_error(OScriptExecutionContext& p_context, OScriptNodeInstance* p_node,
                                    const StringName& p_method)
{
    String err_file = _script->get_path();
    String err_func = p_method;
    int err_line = p_context.get_current_node();
    String error_str = p_context.get_error_reason();

    if (p_node && (p_context.get_error().error != GDEXTENSION_CALL_ERROR_INVALID_METHOD || error_str.is_empty()))
    {
        if (!error_str.is_empty())
            error_str += " ";

        switch (p_context.get_error().error)
        {
            case GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT:
                error_str += "Cannot convert argument " + itos(p_context.get_error().argument) + " to "
                             + Variant::get_type_name(Variant::Type(p_context.get_error().expected)) + ".";
                break;

            case GDEXTENSION_CALL_ERROR_TOO_MANY_ARGUMENTS:
            case GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS:
                error_str += "Expected " + itos(p_context.get_error().argument) + " arguments.";
                break;

            case GDEXTENSION_CALL_ERROR_INVALID_METHOD:
                error_str += "Invalid call.";
                break;

            case GDEXTENSION_CALL_ERROR_METHOD_NOT_CONST:
                error_str += "Method not const in a const instance.";
                break;

            case GDEXTENSION_CALL_ERROR_INSTANCE_IS_NULL:
                error_str += "Instance is null";
                break;

            default:
                // no-op
                break;
        }
    }

    if (!OScriptLanguage::get_singleton()->debug_break(error_str, false))
        _err_print_error(err_func.utf8().get_data(), err_file.utf8().get_data(), err_line, error_str.utf8().get_data());
}

void OScriptInstance::_dependency_step(OScriptExecutionContext& p_context, OScriptNodeInstance* p_node,
                                       OScriptNodeInstance** r_error_node)
{
    ERR_FAIL_COND(p_node->pass_index == -1);

    if (p_context.get_pass_at(p_node->pass_index) == p_context.get_passes())
        return;

    p_context.add_current_pass(p_node->pass_index);

    if (!p_node->dependencies.is_empty())
    {
        OScriptNodeInstance** deps = p_node->dependencies.ptrw();
        int size = p_node->dependencies.size();
        for (int i = 0; i < size; i++)
        {
            _dependency_step(p_context, deps[i], r_error_node);
            if (p_context.has_error())
                return;
        }
    }

    // Set step details (this is needed because we're about to assign inputs)
    // todo: clean this up
    p_context.set_current_node_working_memory(p_node->get_working_memory_size());
    p_context.set_current_node_inputs(p_node->data_input_pin_count);
    p_context.set_current_node_outputs(p_node->data_output_pin_count);

    for (int i = 0; i < p_node->data_input_pin_count; i++)
    {
        int index = p_node->input_pins[i] & OScriptNodeInstance::INPUT_MASK;
        if (p_node->input_pins[i] & OScriptNodeInstance::INPUT_DEFAULT_VALUE_BIT)
            p_context.set_input(i, &_default_values[index]);
        else
            p_context.copy_stack_to_input(index, i);
    }

    _copy_stack_to_node_outputs(p_context, p_node);

    p_context.set_working_memory(p_node->working_memory_index);

    _execute_step(p_context, p_node);

    if (p_context.has_error())
        *r_error_node = p_node;
}

OScriptNodeInstance* OScriptInstance::_resolve_next_node(OScriptExecutionContext& p_context,
                                                         OScriptNodeInstance* p_node, int p_result, int p_output)
{
    if ((p_result == p_output || p_result & OScriptNodeInstance::STEP_FLAG_PUSH_STACK_BIT)
        && p_node->execution_output_pin_count)
    {
        if (p_output >= 0 && p_output < p_node->execution_output_pin_count)
            return p_node->execution_outputs[p_output];

        // No exit bit was set and node has an execution output
        p_context.set_error(GDEXTENSION_CALL_ERROR_INVALID_METHOD,
                            "Node " + p_node->get_base_node()->get_class() + ":" + itos(p_node->get_id())
                                + " returned an invalid execution pin output: " + itos(p_output));
    }
    return nullptr;
}

int OScriptInstance::_resolve_next_node_port(OScriptNodeExecutionContext& p_context, OScriptNodeInstance* p_node,
                                                              int p_result, int p_output, OScriptNodeInstance* p_next)
{
    if (p_node && p_next)
    {
        if (p_output >= 0 && p_output < p_node->execution_output_pin_count)
            return p_node->execution_output_pins[p_output];
    }

    return -1;
}

void OScriptInstance::_set_node_flow_execution_state(OScriptExecutionContext& p_context, OScriptNodeInstance* p_node,
                                                     int p_result)
{
    if (p_result & OScriptNodeInstance::STEP_FLAG_PUSH_STACK_BIT)
    {
        p_context.set_flow_stack_bit(OScriptNodeInstance::FLOW_STACK_PUSHED_BIT);
        p_context.set_node_execution_state(p_node->execution_index, true);
    }
    else
    {
        p_context.set_node_execution_state(p_node->execution_index, false);
    }
}

int OScriptInstance::_execute_step(OScriptExecutionContext& p_context, OScriptNodeInstance* p_node)
{
    // Set step details
    p_context.set_current_node_working_memory(p_node->get_working_memory_size());
    p_context.set_current_node_inputs(p_node->data_input_pin_count);
    p_context.set_current_node_outputs(p_node->data_output_pin_count);

    // Execute step
    return p_node->step(p_context);
}

void OScriptInstance::_copy_stack_to_node_outputs(OScriptExecutionContext& p_context, OScriptNodeInstance* p_node)
{
    for (int i = 0; i < p_node->data_output_pin_count; i++)
        p_context.copy_stack_to_output(p_node->output_pins[i], i);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OScriptState::~OScriptState()
{
    if (function != StringName())
        stack->cleanup_variant_stack();
}

void OScriptState::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("connect_to_signal", "object", "signals", "args"), &OScriptState::connect_to_signal);
    ClassDB::bind_method(D_METHOD("resume", "args"), &OScriptState::resume, DEFVAL(Array()));
    ClassDB::bind_method(D_METHOD("is_valid"), &OScriptState::is_valid);
    ClassDB::bind_method(D_METHOD("_signal_callback", "args"), &OScriptState::_signal_callback);
}

void OScriptState::_signal_callback(const Array& p_args)
{
    ERR_FAIL_COND(function == StringName());

    int p_argcount = p_args.size();

    GDExtensionCallError r_error;
    r_error.error = GDEXTENSION_CALL_OK;

    Array args;
    if (p_argcount == 0)
    {
        r_error.error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS;
        r_error.argument = 1;
        return;
    }
    else if (p_argcount > 1)
    {
        // State with arguments
        for (int i = 0; i < p_argcount - 1; i++)
            args.push_back(p_args[i]);
    }

    Ref<OScriptState> self = p_args[p_argcount - 1];
    if (self.is_null())
    {
        r_error.error = GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT;
        r_error.argument = p_argcount - 1;
        r_error.expected = Variant::OBJECT;
        return;
    }

    r_error.error = GDEXTENSION_CALL_OK;

    Variant result = instance->_call_internal(function, stack, flow_stack_pos, pass, true, node, r_error);
    function = StringName();
}

void OScriptState::connect_to_signal(Object* p_object, const String& p_signal, Array p_binds)
{
    ERR_FAIL_NULL(p_object);

    Array binds;
    for (int i = 0; i < p_binds.size(); i++)
        binds.push_back(p_binds[i]);

    binds.push_back(Ref<OScriptState>(this));

    p_object->connect(p_signal, Callable(this, "_signal_callback").bind(binds), CONNECT_ONE_SHOT);
}

bool OScriptState::is_valid() const
{
    return function != StringName();
}

Variant OScriptState::resume(const Array& p_args)
{
    ERR_FAIL_COND_V(function == StringName(), Variant());

    GDExtensionCallError r_error;
    r_error.error = GDEXTENSION_CALL_OK;

    Variant result = instance->_call_internal(function, stack, flow_stack_pos, pass, true, node, r_error);
    function = StringName();
    return result;
}
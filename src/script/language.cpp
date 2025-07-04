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
#include "script/language.h"

#include "common/dictionary_utils.h"
#include "common/settings.h"
#include "common/resource_utils.h"
#include "common/string_utils.h"
#include "script/script.h"
#include "script/vm/script_vm.h"
#include "utility_functions.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/engine_debugger.hpp>
#include <godot_cpp/classes/expression.hpp>
#if GODOT_VERSION >= 0x040300
  #include <godot_cpp/classes/os.hpp>
#endif
#ifdef TOOLS_ENABLED
  #include <godot_cpp/core/mutex_lock.hpp>
#endif

OScriptLanguage* OScriptLanguage::_singleton = nullptr;

void OScriptLanguage::_add_global(const StringName& p_name, const Variant& p_value)
{
    if (_globals.has(p_name))
    {
        _global_array.write[_globals[p_name]] = p_value;
        return;
    }

    if (_global_array_empty_indices.size())
    {
        int index = _global_array_empty_indices[_globals.size() - 1];
        _globals[p_name] = index;
        _global_array.write[index] = p_value;
        _global_array_empty_indices.resize(_global_array_empty_indices.size() - 1);
    }
    else
    {
        _globals[p_name] = static_cast<int>(_global_array.size());
        _global_array.push_back(p_value);
        _global_array_ptr = _global_array.ptrw();
    }
}

void OScriptLanguage::_remove_global(const StringName& p_name)
{
    if (!_globals.has(p_name))
        return;

    _global_array_empty_indices.push_back(_globals[p_name]);
    _global_array.write[_globals[p_name]] = Variant::NIL;
    _globals.erase(p_name);
}

String OScriptLanguage::_get_name() const
{
    return TYPE;
}

void OScriptLanguage::_init()
{
    OrchestratorSettings* settings = OrchestratorSettings::get_singleton();
    if (settings)
    {
        const String format = settings->get_setting("settings/storage_format", "Text");
        if (format.match("Binary"))
            _extension = ORCHESTRATOR_SCRIPT_EXTENSION;
    }

    #if GODOT_VERSION >= 0x040300
    if (EngineDebugger::get_singleton()->is_active())
    {
        OrchestratorSettings* os = OrchestratorSettings::get_singleton();
        int max_call_stack = os->get_setting("settings/runtime/max_call_stack", 1024);
        _debug_max_call_stack = max_call_stack;
        _call_stack = memnew_arr(CallStack, _debug_max_call_stack + 1);
    }
    else
    {
        _debug_max_call_stack = 0;
        _call_stack = nullptr;
    }
    #endif
}

String OScriptLanguage::_get_type() const
{
    return TYPE;
}

String OScriptLanguage::_get_extension() const
{
    return _extension;
}

PackedStringArray OScriptLanguage::_get_recognized_extensions() const
{
    return { Array::make(ORCHESTRATOR_SCRIPT_EXTENSION, ORCHESTRATOR_SCRIPT_TEXT_EXTENSION) };
}

TypedArray<Dictionary> OScriptLanguage::_get_built_in_templates(const StringName& p_object) const
{
    Dictionary data;
    data["inherit"] = p_object;
    data["name"] = "Orchestration";
    data["description"] = "Basic Orchestration";
    data["content"] = "";
    data["id"] = 0;
    data["origin"] = 0; //built-in

    return Array::make(data);
}

Ref<Script> OScriptLanguage::_make_template(const String& p_template, const String& p_class_name, const String& p_base_class_name) const
{
    // NOTE:
    // The p_template argument is the content of the template, set in _get_built_in_templates.
    // Even if the user deselects the template option in the script dialog, this method is called.
    //
    // The p_class_name is derived from the file name.
    // The p_base_class_name is the actor/class type the script inherits from.
    //
    Ref<OScript> script;
    script.instantiate();

    // Set the script's base actor/class type
    script->set_base_type(p_base_class_name);

    // All orchestrator scripts start with an "EventGraph" graph definition.
    script->create_graph("EventGraph", OScriptGraph::GF_EVENT);

    return script;
}

String OScriptLanguage::_validate_path(const String& p_path) const
{
    // This is primarily used by the CScriptScript module so that the base filename of a C#
    // file, aka the class name, does not clash with any reserved words as that is not a
    // valid combination. For GDScript and for us, returning "" means that things are okay.
    return "";
}

Dictionary OScriptLanguage::_validate(const String& p_script, const String& p_path, bool p_validate_functions, bool p_validate_errors, bool p_validate_warnings, bool p_validate_safe_lines) const
{
    // This hook assumes that the data provided by 'p_script' is the actual script text. This does not
    // currently apply to Orchestrator, but can eventually be used when we begin to decouple several
    // parts of the OScript<->Orchestration model.
    //
    // For now, there is the OScriptLanguage::validate method that takes the actual Ref<Script> as an
    // input and performs the same steps as a current substitute.
    //
    return DictionaryUtils::of({{"valid", true}});
}

Object* OScriptLanguage::_create_script() const
{
    const String base_type = ORCHESTRATOR_GET("settings/default_type", "Node");

    OScript* script = memnew(OScript);
    script->set_base_type(base_type);
    script->create_graph("EventGraph", OScriptGraph::GF_EVENT);

    return script;
}

bool OScriptLanguage::_is_control_flow_keyword(const String& p_keyword) const
{
    return false;
}

void OScriptLanguage::_add_global_constant(const StringName& p_name, const Variant& p_value)
{
    _add_global(p_name, p_value);
}

void OScriptLanguage::_add_named_global_constant(const StringName& p_name, const Variant& p_value)
{
    _named_global_constants[p_name] = p_value;
}

void OScriptLanguage::_remove_named_global_constant(const StringName& p_name)
{
    ERR_FAIL_COND(!_named_global_constants.has(p_name));
    _named_global_constants.erase(p_name);
}

int32_t OScriptLanguage::_find_function(const String& p_function_name, const String& p_code) const
{
    // Locates the function name in the specified code.
    // Used in the connections dialog and when linking a signal to a function in text-based languages.
    // We don't use this in Orchestrator.
    return -1;
}

String OScriptLanguage::_make_function(const String& p_class_name, const String& p_function_name, const PackedStringArray& p_function_args) const
{
    // Creates a function stub for the given name.
    // This is called by the ScriptTextEditor::add_callback
    // Since we don't use the ScriptTextEditor, this doesn't apply.
    return {};
}

TypedArray<Dictionary> OScriptLanguage::_get_public_functions() const
{
    TypedArray<Dictionary> results;
    for (const StringName& function : OScriptUtilityFunctions::get_function_list())
    {
        const MethodInfo method = OScriptUtilityFunctions::get_function_info(function);
        results.push_back(DictionaryUtils::from_method(method));
    }
    return results;
}

Dictionary OScriptLanguage::_get_public_constants() const
{
    // This includes things like PI, TAU, INF, and NAN.
    // Orchestrator does not have anything beyond standard Godot.
    return {};
}

TypedArray<Dictionary> OScriptLanguage::_get_public_annotations() const
{
    // Returns list of annotation MethodInfo values.
    // Orchestrator does not have any.
    return {};
}

void OScriptLanguage::_reload_all_scripts()
{
#ifdef TOOLS_ENABLED
    List<Ref<OScript>> scripts = get_scripts();
    for (Ref<OScript>& script : scripts)
        script->reload(false);
#endif
}

void OScriptLanguage::_reload_tool_script(const Ref<Script>& p_script, bool p_soft_reload)
{
#ifdef TOOLS_ENABLED
    ERR_PRINT("Tool script reloading is not yet implemented");
#endif
}

void OScriptLanguage::_finish()
{
    // Move call stack cleanup here
}

#if GODOT_VERSION >= 0x040300
String OScriptLanguage::_debug_get_stack_level_source(int32_t p_level) const
{
    if (_debug_parse_err_line >= 0)
        return _debug_parse_err_file;

    ERR_FAIL_INDEX_V(p_level, _debug_call_stack_pos, {});
    int l = _debug_call_stack_pos - p_level - 1;
    return _call_stack[l].instance->get_script()->get_path();
}

int32_t OScriptLanguage::_debug_get_stack_level_line(int32_t p_level) const
{
    if (_debug_parse_err_line >= 0)
        return _debug_parse_err_line;

    ERR_FAIL_INDEX_V(p_level, _debug_call_stack_pos, -1);
    int l = _debug_call_stack_pos - p_level - 1;
    return *_call_stack[l].id;
}

String OScriptLanguage::_debug_get_stack_level_function(int32_t p_level) const
{
    if (_debug_parse_err_line >= 0)
        return {};

    ERR_FAIL_INDEX_V(p_level, _debug_call_stack_pos, {});
    int l = _debug_call_stack_pos - p_level - 1;
    return *(_call_stack[l].current_function);
}

void* OScriptLanguage::_debug_get_stack_level_instance(int32_t p_level)
{
    if (_debug_parse_err_line >= 0)
         return nullptr;

    ERR_FAIL_INDEX_V(p_level, _debug_call_stack_pos, nullptr);
    int l = _debug_call_stack_pos - p_level - 1;
    return _call_stack[l].instance->_script_instance;
}

Dictionary OScriptLanguage::_debug_get_stack_level_members(int32_t p_level, int32_t p_max_subitems, int32_t p_max_depth)
{
    if (_debug_parse_err_line >= 0)
        return {};

    ERR_FAIL_INDEX_V(p_level, _debug_call_stack_pos, {});
    int l =_debug_call_stack_pos - p_level - 1;

    Ref<OScript> script = _call_stack[l].instance->get_script();
    if (!script.is_valid())
        return {};

    PackedStringArray member_names;
    Array member_values;

    for (const String& variable_name: script->get_variable_names())
    {
        Variant value;
        if (_call_stack[l].instance->get_variable(variable_name, value))
        {
            member_names.push_back("Variables/" + variable_name);
            member_values.push_back(value);
        }
    }

    Dictionary members;
    members["members"] = member_names;
    members["values"] = member_values;

    return members;
}

Dictionary OScriptLanguage::_debug_get_stack_level_locals(int32_t p_level, int32_t p_max_subitems, int32_t p_max_depth)
{
    if (_debug_parse_err_line >= 0)
        return {};

    ERR_FAIL_INDEX_V(p_level, _debug_call_stack_pos, {});

    int l =_debug_call_stack_pos - p_level - 1;
    const StringName* function_name = _call_stack[l].current_function;
    ERR_FAIL_COND_V(!_call_stack[l].instance->_vm._functions.has(*function_name), {});

    OScriptNodeInstance* node = _call_stack[l].instance->_vm._nodes[*_call_stack[l].id];
    ERR_FAIL_COND_V(!node, {});

    PackedStringArray local_names;
    Array local_values;

    local_names.push_back("Script Node Name");
    local_values.push_back(node->get_base_node()->get_node_title());
    local_names.push_back("Script Node ID");
    local_values.push_back(node->get_base_node()->get_id());
    local_names.push_back("Script Node Type");
    local_values.push_back(node->get_base_node()->get_class());

    int offset = 0;
    for (int i = 0; i < node->input_pin_count; i++)
    {
        Ref<OScriptNodePin> pin = node->get_base_node()->find_pin(i, PD_Input);
        if (pin->is_execution())
        {
            offset++;
            continue;
        }

        if (!pin->get_label().is_empty())
            local_names.push_back("Inputs/" + pin->get_label());
        else
            local_names.push_back("Inputs/" + pin->get_pin_name());

        int in_from = node->input_pins[i - offset];
        int in_value = in_from & OScriptNodeInstance::INPUT_MASK;
        if (in_from & OScriptNodeInstance::INPUT_DEFAULT_VALUE_BIT)
            local_values.push_back(_call_stack[l].instance->_vm._default_values[in_value]);
        else
            local_values.push_back(_call_stack[l].stack[in_value]);
    }

    offset = 0;
    for (int i = 0; i < node->output_pin_count; i++)
    {
        Ref<OScriptNodePin> pin = node->get_base_node()->find_pin(i, PD_Output);
        if (pin->is_execution())
        {
            offset++;
            continue;
        }

        if (!pin->get_label().is_empty())
            local_names.push_back("Outputs/" + pin->get_label());
        else
            local_names.push_back("Outputs/" + pin->get_pin_name());

        int out = node->output_pins[i - offset];
        local_values.push_back(_call_stack[l].stack[out]);
    }

    Dictionary locals;
    locals["locals"] = local_names;
    locals["values"] = local_values;
    return locals;
}

Dictionary OScriptLanguage::_debug_get_globals(int32_t p_max_subitems, int32_t p_max_depth)
{
    Dictionary results;
    results["globals"] = get_global_constant_names();

    Array values;
    for (const String& name : get_global_constant_names())
        values.push_back(get_any_global_constant(name));
    results["values"] = values;

    return results;
}

String OScriptLanguage::_debug_get_error() const
{
    return _debug_error;
}

int32_t OScriptLanguage::_debug_get_stack_level_count() const
{
    if (_debug_parse_err_line >= 0)
        return 1;

    return _debug_call_stack_pos;
}
#endif

TypedArray<Dictionary> OScriptLanguage::_debug_get_current_stack_info()
{
    TypedArray<Dictionary> array;
    #if GODOT_VERSION >= 0x040300
    for (int i = 0; i < _debug_call_stack_pos; i++)
    {
        Dictionary data;
        data["file"] = _call_stack[i].instance->get_script()->get_path();
        data["func"] = *_call_stack[i].current_function;
        data["line"] = *_call_stack[i].id;
        array.append(data);
    }
    #endif
    return array;
}

bool OScriptLanguage::_handles_global_class_type(const String& p_type) const
{
    return p_type == _get_type();
}

Dictionary OScriptLanguage::_get_global_class_name(const String& p_path) const
{
    // OrchestratorScripts do not have global class names
    return {};
}

#ifdef TOOLS_ENABLED
List<Ref<OScript>> OScriptLanguage::get_scripts() const
{
    List<Ref<OScript>> scripts;
    {
        const PackedStringArray extensions = _get_recognized_extensions();

        MutexLock mutex_lock(*this->lock.ptr());
        const SelfList<OScript>* iterator = _scripts.first();
        while (iterator)
        {
            String path = iterator->self()->get_path();
            if (extensions.has(path.get_extension().to_lower()))
                scripts.push_back(Ref<OScript>(iterator->self()));

            iterator = iterator->next();
        }
    }
    return scripts;
}
#endif

bool OScriptLanguage::has_any_global_constant(const StringName& p_name) const
{
    return _named_global_constants.has(p_name) || _globals.has(p_name);
}

Variant OScriptLanguage::get_any_global_constant(const StringName& p_name)
{
    if (_named_global_constants.has(p_name))
        return _named_global_constants[p_name];

    if (_globals.has(p_name))
        return _global_array[_globals[p_name]];

    return Variant();
}

PackedStringArray OScriptLanguage::get_global_constant_names() const
{
    PackedStringArray keys;
    for (const KeyValue<StringName, Variant>& E : _named_global_constants)
        if (!keys.has(E.key))
            keys.push_back(E.key);

    for (const KeyValue<StringName, int>& E : _globals)
        if (!keys.has(E.key))
            keys.push_back(E.key);

    return keys;
}

bool OScriptLanguage::debug_break(const String& p_error, bool p_allow_continue)
{
    #if GODOT_VERSION >= 0x040300
    if (EngineDebugger::get_singleton()->is_active())
    {
        if (OS::get_singleton()->get_thread_caller_id() == OS::get_singleton()->get_main_thread_id())
        {
            _debug_parse_err_line = -1;
            _debug_parse_err_file = "";
            _debug_error = p_error;

            EngineDebugger::get_singleton()->script_debug(this, p_allow_continue, true);
            return true;
        }
    }
    #endif
    return false;
}

bool OScriptLanguage::debug_break_parse(const String& p_file, int p_node, const String& p_error)
{
    #if GODOT_VERSION >= 0x040300
    if (EngineDebugger::get_singleton()->is_active())
    {
        if (OS::get_singleton()->get_thread_caller_id() == OS::get_singleton()->get_main_thread_id())
        {
            _debug_parse_err_line = p_node;
            _debug_parse_err_file = p_file;
            _debug_error = p_error;

            EngineDebugger::get_singleton()->script_debug(this, false, true);
            return true;
        }
    }
    #endif
    return false;
}

#if GODOT_VERSION >= 0x040300
void OScriptLanguage::function_entry(const StringName* p_method, const OScriptExecutionContext* p_context)
{
    // Debugging can only happen within main thread
    if (OS::get_singleton()->get_thread_caller_id() != OS::get_singleton()->get_main_thread_id())
        return;

    EngineDebugger* debugger = EngineDebugger::get_singleton();
    if (!debugger || !debugger->is_active())
        return;

    if (debugger->get_lines_left() > 0 && debugger->get_depth() >= 0)
        debugger->set_depth(debugger->get_depth() + 1);

    if (_debug_call_stack_pos >= _debug_max_call_stack)
    {
        // Stack overflow
        _debug_error = vformat("Stack overflow detected (stack size: %s)", _debug_max_call_stack);
        debugger->script_debug(this, false, false);
        return;
    }

    Variant* ptr = p_context->_working_memory;
    _call_stack[_debug_call_stack_pos].stack = reinterpret_cast<Variant*>(p_context->_stack);
    _call_stack[_debug_call_stack_pos].instance = p_context->_script_instance;
    _call_stack[_debug_call_stack_pos].current_function = p_method;
    _call_stack[_debug_call_stack_pos].working_memory = &ptr;
    _call_stack[_debug_call_stack_pos].id = const_cast<int*>(p_context->get_current_node_ref());
    _debug_call_stack_pos++;
}

void OScriptLanguage::function_exit(const StringName* p_method, const OScriptExecutionContext* p_context)
{
    // Debugging can only happen within main thread
    if (OS::get_singleton()->get_thread_caller_id() != OS::get_singleton()->get_main_thread_id())
        return;

    EngineDebugger* debugger = EngineDebugger::get_singleton();
    if (!debugger || !debugger->is_active())
        return;

    if (debugger->get_lines_left() > 0 && debugger->get_depth() >= 0)
        debugger->set_depth(debugger->get_depth() - 1);

    if (_debug_call_stack_pos == 0)
    {
        // Stack underflow
        _debug_error = "Stack underflow detected";
        debugger->script_debug(this, false, false);
        return;
    }

    if (_call_stack[_debug_call_stack_pos - 1].instance != p_context->_script_instance
        || *_call_stack[_debug_call_stack_pos - 1].current_function != *p_method)
    {
        // Function mismatch
        _debug_error = "Function mismatch detected";
        debugger->script_debug(this, false, false);
        return;
    }

    _debug_call_stack_pos--;
}
#endif

String OScriptLanguage::get_script_extension_filter() const
{
    PackedStringArray results;
    for (const String& extension : _get_recognized_extensions())
        results.push_back(vformat("*.%s", extension));

    return StringUtils::join(",", results);
}

bool OScriptLanguage::validate(const Ref<OScript>& p_script, const String& p_path, List<String>* r_functions,
                               List<Warning>* r_warnings, List<ScriptError>* r_errors)
{
    if (!p_script.is_valid())
        return false;


    BuildLog build_log;
    for (const Ref<OScriptNode>& node : p_script->get_orchestration()->get_nodes())
        node->validate_node_during_build(build_log);

    bool valid = true;
    for (const BuildLog::Failure& failure : build_log.get_failures())
    {
        valid = false;

        switch (failure.type)
        {
            case BuildLog::FT_Warning:
            {
                if (r_warnings)
                {
                    Warning warning;
                    warning.node = failure.node->get_id();
                    warning.name = failure.node->get_node_title() + " / " + failure.node->get_class().replace("OScriptNode", "").capitalize();
                    warning.message = failure.message;
                    r_warnings->push_back(warning);
                }
                break;
            }
            case BuildLog::FT_Error:
            {
                if (r_errors)
                {
                    ScriptError error;
                    error.node = failure.node->get_id();
                    error.name = failure.node->get_node_title() + " / " + failure.node->get_class().replace("OScriptNode", "").capitalize();
                    error.message = failure.message;
                    r_errors->push_back(error);
                }
                break;
            }
        }
    }

    return valid;
}

OScriptLanguage* OScriptLanguage::get_singleton()
{
    return _singleton;
}

OScriptLanguage::OScriptLanguage()
{
    _singleton = this;
    lock.instantiate();
}

OScriptLanguage::~OScriptLanguage()
{
    _singleton = nullptr;

    #if GODOT_VERSION >= 0x040300
    if (_call_stack)
    {
        memdelete_arr(_call_stack);
        _call_stack = nullptr;
    }
    #endif
}
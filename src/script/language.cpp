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
#include "language.h"

#include "common/logger.h"
#include "script.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/engine_debugger.hpp>
#ifdef TOOLS_ENABLED
  #include <godot_cpp/core/mutex_lock.hpp>
#endif
#include <godot_cpp/variant/utility_functions.hpp>

OScriptLanguage* OScriptLanguage::_singleton = nullptr;
HashMap<StringName, OScriptLanguage::ScriptNodeInfo> OScriptLanguage::_nodes;

OScriptLanguage::OScriptLanguage()
{
    _singleton = this;
    lock.instantiate();
}

OScriptLanguage::~OScriptLanguage()
{
    _singleton = nullptr;
}

OScriptLanguage* OScriptLanguage::get_singleton()
{
    return _singleton;
}

void OScriptLanguage::_add_node_class_internal(const StringName& p_class, const StringName& p_inherits)
{
    const StringName& name = p_class;
    ERR_FAIL_COND_MSG(_nodes.has(name), "Class '" + String(p_class) + "' already exists.");

    _nodes[name] = ScriptNodeInfo();
    ScriptNodeInfo& sni = _nodes[name];
    sni.name = name;
    sni.inherits = p_inherits;

    if (!sni.inherits.is_empty())
    {
        ERR_FAIL_COND_MSG(!_nodes.has(sni.inherits), "Node " + p_inherits + " is not defined as a node");
        sni.inherits_ptr = &_nodes[sni.inherits];
    }
    else
    {
        sni.inherits_ptr = nullptr;
    }
}

void OScriptLanguage::_init()
{
    Logger::info("Initializing OrchestratorScript");
}

String OScriptLanguage::_get_name() const
{
    return TYPE;
}

String OScriptLanguage::_get_type() const
{
    return TYPE;
}

String OScriptLanguage::_get_extension() const
{
    return EXTENSION;
}

PackedStringArray OScriptLanguage::_get_recognized_extensions() const
{
    return { Array::make(EXTENSION) };
}

bool OScriptLanguage::_can_inherit_from_file() const
{
    return true;
}

bool OScriptLanguage::_supports_builtin_mode() const
{
    return true;
}

bool OScriptLanguage::_supports_documentation() const
{
    return false;
}

bool OScriptLanguage::_is_using_templates()
{
    return false;
}

TypedArray<Dictionary> OScriptLanguage::_get_built_in_templates(const StringName& p_object) const
{
    return {};
}

Ref<Script> OScriptLanguage::_make_template(const String& p_template, const String& p_class_name,
                                            const String& p_base_class_name) const
{
    Ref<OScript> script;
    script.instantiate();
    script->set_base_type(p_base_class_name);
    return script;
}

bool OScriptLanguage::_overrides_external_editor()
{
    return true;
}

Error OScriptLanguage::_open_in_external_editor(const Ref<Script>& p_script, int32_t p_line, int32_t p_column)
{
    // We don't currently support this but return OK to avoid editor errors.
    return OK;
}

String OScriptLanguage::_validate_path(const String& p_path) const
{
    // This is primarily used by the CScriptScript module so that the base filename of a C#
    // file, aka the class name, does not clash with any reserved words as that is not a
    // valid combination. For GDScript and for us, returning "" means that things are okay.
    return "";
}

Dictionary OScriptLanguage::_validate(const String& p_script, const String& p_path, bool p_validate_functions,
                                      bool p_validate_errors, bool p_validate_warnings, bool p_validate_safe_lines) const
{
    // TODO:    GodotCPP
    //          Do not see how this method is being invoked by Godot.
    //          Need to discuss with the GDE team to understand the purpose for this call,
    //          particularly with the fact there is technically no source code in our use case.

    Dictionary result;
    result["valid"] = true;
    return result;
}

Object* OScriptLanguage::_create_script() const
{
    OScript* script = memnew(OScript);

    // All orchestrator scripts start with an "EventGraph" graph definition.
    script->create_graph("EventGraph");

    return script;
}

PackedStringArray OScriptLanguage::_get_comment_delimiters() const
{
    // We don't support any comments
    return {};
}

PackedStringArray OScriptLanguage::_get_string_delimiters() const
{
    // We don't support any string/line delimiters
    return {};
}

PackedStringArray OScriptLanguage::_get_reserved_words() const
{
    // We don't support reserved keywords
    return {};
}

bool OScriptLanguage::_has_named_classes() const
{
    return false;
}

bool OScriptLanguage::_is_control_flow_keyword(const String& p_keyword) const
{
    return false;
}

void OScriptLanguage::_add_global_constant(const StringName& p_name, const Variant& p_value)
{
    _global_constants[p_name] = p_value;
}

void OScriptLanguage::_add_named_global_constant(const StringName& p_name, const Variant& p_value)
{
    _named_global_constants[p_name] = p_value;
}

void OScriptLanguage::_remove_named_global_constant(const StringName& p_name)
{
    _named_global_constants.erase(p_name);
}

int32_t OScriptLanguage::_find_function(const String& p_class_name, const String& p_function_name) const
{
    return -1;
}

String OScriptLanguage::_make_function(const String& p_class_name, const String& p_function_name,
                                       const PackedStringArray& p_function_args) const
{
    return {};
}

TypedArray<Dictionary> OScriptLanguage::_get_public_functions() const
{
    return {};
}

Dictionary OScriptLanguage::_get_public_constants() const
{
    return {};
}

TypedArray<Dictionary> OScriptLanguage::_get_public_annotations() const
{
    return {};
}

String OScriptLanguage::_auto_indent_code(const String& p_code, int32_t p_from_line, int32_t p_to_line) const
{
    return {};
}

Dictionary OScriptLanguage::_lookup_code(const String& p_code, const String& p_symbol, const String& p_path,
                                         Object* p_owner) const
{
    return {};
}

Dictionary OScriptLanguage::_complete_code(const String& p_code, const String& p_path, Object* p_owner) const
{
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

void OScriptLanguage::_thread_enter()
{
}

void OScriptLanguage::_thread_exit()
{
}

void OScriptLanguage::_profiling_start()
{
}

void OScriptLanguage::_profiling_stop()
{
}

void OScriptLanguage::_frame()
{
}

void OScriptLanguage::_finish()
{
}

TypedArray<Dictionary> OScriptLanguage::_debug_get_current_stack_info()
{
    return {};
}

bool OScriptLanguage::_handles_global_class_type(const String& p_type) const
{
#ifdef TOOLS_ENABLED
    return p_type == _get_type();
#else
    return false;
#endif
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
        MutexLock mutex_lock(*this->lock.ptr());
        const SelfList<OScript>* iterator = _scripts.first();
        while (iterator)
        {
            String path = iterator->self()->get_path();
            if (path.get_extension().to_lower() == EXTENSION)
                scripts.push_back(Ref<OScript>(iterator->self()));

            iterator = iterator->next();
        }
    }
    return scripts;
}
#endif

bool OScriptLanguage::has_any_global_constant(const StringName& p_name) const
{
    return _named_global_constants.has(p_name) || _global_constants.has(p_name);
}

Variant OScriptLanguage::get_any_global_constant(const StringName& p_name)
{
    if (_named_global_constants.has(p_name))
        return _named_global_constants[p_name];

    if (_global_constants.has(p_name))
        return _global_constants[p_name];

    return Variant();
}

PackedStringArray OScriptLanguage::get_global_constant_names() const
{
    PackedStringArray keys;
    for (const KeyValue<StringName, Variant>& E : _named_global_constants)
        if (!keys.has(E.key))
            keys.push_back(E.key);

    for (const KeyValue<StringName, Variant>& E : _global_constants)
        if (!keys.has(E.key))
            keys.push_back(E.key);

    return keys;
}

bool OScriptLanguage::debug_break(const String& p_error, bool p_allow_continue)
{
    // TODO:    GodotCPP Feature Request
    //          Currently the EngineDebugger::debug method is not exposed to GDE and so it is
    //          not presently possible to actually execute any type of debugger operation.
    return false;
}

bool OScriptLanguage::debug_break_parse(const String& p_file, int p_node, const String& p_error)
{
    // TODO:    GodotCPP Feature Request
    //          Currently the EngineDebugger::debug method is not exposed to GDE and so it is
    //          not presently possible to actually execute any type of debugger operation.
    return false;
}

Ref<OScriptNode> OScriptLanguage::create_node_from_name(const String& p_class_name, const Ref<OScript>& p_owner,
                                                        bool p_allocate_id)
{
    ERR_FAIL_COND_V_MSG(!_nodes.has(p_class_name), Ref<OScriptNode>(), "No node found with name: " + p_class_name);

    Ref<OScriptNode> node(_nodes[p_class_name].creation_func());
    node->set_id(p_allocate_id ? p_owner->get_available_id() : -1);
    node->set_owning_script(p_owner.ptr());

    return node;
}

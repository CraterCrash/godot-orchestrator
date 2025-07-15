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
#ifndef ORCHESTRATOR_SCRIPT_LANGUAGE_H
#define ORCHESTRATOR_SCRIPT_LANGUAGE_H

#include "common/version.h"
#include "script/serialization/format_defs.h"

#include <godot_cpp/classes/mutex.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/classes/script_language_extension.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/self_list.hpp>
#include <godot_cpp/variant/string_name.hpp>

using namespace godot;

/// Forward declarations
class Orchestration;
class OScript;
class OScriptExecutionContext;
class OScriptInstance;
class OScriptNode;
class OScriptVirtualMachine;

/// Defines an extension for Godot where we define the language for Orchestrations.
class OScriptLanguage : public ScriptLanguageExtension
{
    GDCLASS(OScriptLanguage, ScriptLanguageExtension);

protected:
    static void _bind_methods() { }

private:

    #if GODOT_VERSION >= 0x040300
    struct CallStack
    {
        Variant* stack{ nullptr };
        Variant** working_memory{ nullptr };
        const StringName* current_function{ nullptr };
        OScriptInstance* instance{ nullptr };
        int* id{ nullptr };
    };

    int _debug_parse_err_line{ -1 };    //! The line number of the parse error
    String _debug_parse_err_file;       //! The script file name of the parse error
    String _debug_error;                //! The error message
    int _debug_call_stack_pos{ 0 };     //! The current call stack position
    int _debug_max_call_stack{ 0 };     //! The maximum call stack size
    CallStack* _call_stack{ nullptr };  //! The call stack
    #endif

    static OScriptLanguage* _singleton;
    SelfList<OScript>::List _scripts;
    String _extension{ ORCHESTRATOR_SCRIPT_TEXT_EXTENSION };

    Variant* _global_array_ptr = nullptr;
    Vector<Variant> _global_array;
    Vector<int> _global_array_empty_indices;
    HashMap<StringName, int> _globals;
    HashMap<StringName, Variant> _named_global_constants;

    void _add_global(const StringName& p_name, const Variant& p_value);
    void _remove_global(const StringName& p_name);

public:

    // Defines a script validation warning
    struct Warning
    {
        int node;
        String name;
        String message;
    };

    // Defines a script validation error
    struct ScriptError
    {
        String path;
        int node;
        String name;
        String message;
    };

    /// Public lock used for specific synchronizing use cases.
    Ref<Mutex> lock;

    static inline const char* TYPE = "Orchestrator";
    static inline const char* ICON = "res://addons/orchestrator/icons/Orchestrator_16x16.png";

    static OScriptLanguage* get_singleton();

    //~ Begin ScriptLanguageExtension Interface
    String _get_name() const override;
    void _init() override;
    String _get_type() const override;
    String _get_extension() const override;
    PackedStringArray _get_recognized_extensions() const override;
    bool _can_inherit_from_file() const override { return true; }
    bool _supports_builtin_mode() const override { return true; }
    bool _supports_documentation() const override { return false; }
    bool _is_using_templates() override { return true; }
    TypedArray<Dictionary> _get_built_in_templates(const StringName& p_object) const override;
    Ref<Script> _make_template(const String& p_template, const String& p_class_name, const String& p_base_class_name) const override;
    bool _overrides_external_editor() override { return true; }
    Error _open_in_external_editor(const Ref<Script>& p_script, int32_t p_line, int32_t p_column) override { return OK; }
    String _validate_path(const String& p_path) const override;
    Dictionary _validate(const String& p_script, const String& p_path, bool p_validate_functions, bool p_validate_errors, bool p_validate_warnings, bool p_validate_safe_lines) const override;
    Object* _create_script() const override;
    PackedStringArray _get_comment_delimiters() const override { return {}; }
    PackedStringArray _get_doc_comment_delimiters() const override { return {}; }
    PackedStringArray _get_string_delimiters() const override { return {}; }
    PackedStringArray _get_reserved_words() const override { return {}; }
    bool _has_named_classes() const override { return false; }
    bool _is_control_flow_keyword(const String& p_keyword) const override;
    void _add_global_constant(const StringName& p_name, const Variant& p_value) override;
    void _add_named_global_constant(const StringName& p_name, const Variant& p_value) override;
    void _remove_named_global_constant(const StringName& p_name) override;
    int32_t _find_function(const String& p_function_name, const String& p_code) const override;
    String _make_function(const String& p_class_name, const String& p_function_name, const PackedStringArray& p_function_args) const override;
    #if GODOT_VERSION >= 0x040300
    bool _can_make_function() const override { return true; }
    #endif
    TypedArray<Dictionary> _get_public_functions() const override;
    Dictionary _get_public_constants() const override;
    TypedArray<Dictionary> _get_public_annotations() const override;
    String _auto_indent_code(const String& p_code, int32_t p_from_line, int32_t p_to_line) const override { return {}; }
    Dictionary _lookup_code(const String& p_code, const String& p_symbol, const String& p_path, Object* p_owner) const override { return {}; }
    Dictionary _complete_code(const String& p_code, const String& p_path, Object* p_owner) const override { return {}; }
    void _reload_all_scripts() override;
    void _reload_tool_script(const Ref<Script>& p_script, bool p_soft_reload) override;
    void _thread_enter() override {}
    void _thread_exit() override {}
    void _profiling_start() override {}
    void _profiling_stop() override {}
    void _frame() override {}
    void _finish() override;
    #if GODOT_VERSION >= 0x040300
    String _debug_get_stack_level_source(int32_t p_level) const override;
    int32_t _debug_get_stack_level_line(int32_t p_level) const override;
    String _debug_get_stack_level_function(int32_t p_level) const override;
    void* _debug_get_stack_level_instance(int32_t p_level) override;
    Dictionary _debug_get_stack_level_members(int32_t p_level, int32_t p_max_subitems, int32_t p_max_depth) override;
    Dictionary _debug_get_stack_level_locals(int32_t p_level, int32_t p_max_subitems, int32_t p_max_depth) override;
    Dictionary _debug_get_globals(int32_t p_max_subitems, int32_t p_max_depth) override;
    String _debug_get_error() const override;
    int32_t _debug_get_stack_level_count() const override;
    #endif
    TypedArray<Dictionary> _debug_get_current_stack_info() override;
    bool _handles_global_class_type(const String& p_type) const override;
    Dictionary _get_global_class_name(const String& p_path) const override;
    //~ End ScriptLanguageExtension Interface

    bool has_any_global_constant(const StringName& p_name) const;
    Variant get_any_global_constant(const StringName& p_name);
    PackedStringArray get_global_constant_names() const;

    // Debugging
    bool debug_break(const String& p_error, bool p_allow_continue);
    bool debug_break_parse(const String& p_file, int p_node, const String& p_error);
    #if GODOT_VERSION >= 0x040300
    void function_entry(const StringName* p_method, const OScriptExecutionContext* p_context);
    void function_exit(const StringName* p_method, const OScriptExecutionContext* p_context);
    #endif

    String get_script_extension_filter() const;

    #ifdef TOOLS_ENABLED
    List<Ref<OScript>> get_scripts() const;
    #endif

    bool validate(const Ref<OScript>& p_script, const String& p_path, List<String>* r_functions = nullptr, List<Warning>* r_warnings = nullptr, List<ScriptError>* r_errors = nullptr);

    OScriptLanguage();
    ~OScriptLanguage() override;
};

#endif  // ORCHESTRATOR_SCRIPT_LANGUAGE_H
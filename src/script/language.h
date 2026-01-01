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
#include "script/compiler/compiled_function.h"
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
class OScriptCompiledFunction;
class OScriptCompiler;
class OScriptExecutionContext;
class OScriptFunctionState;
class OScriptInstance;
class OScriptNode;
class OScriptNodePrintStringOverlay;
class OScriptVirtualMachine;

/// Defines an extension for Godot where we define the language for Orchestrations.
class OScriptLanguage : public ScriptLanguageExtension {
    GDCLASS(OScriptLanguage, ScriptLanguageExtension);

    friend class OScript;
    friend class OScriptCompiledFunction;
    friend class OScriptCompiler;
    friend class OScriptFunctionState;
    friend class OScriptInstance;

    static OScriptLanguage* _singleton;
    bool finishing = false;

    Variant* _global_array = nullptr;
    Vector<Variant> global_array;
    HashMap<StringName, int> globals;
    HashMap<StringName, Variant> named_globals;
    Vector<int> global_array_empty_indexes;

    #if GODOT_VERSION >= 0x040300
    struct CallLevel {
        Variant* stack{ nullptr };
        OScriptCompiledFunction* function{ nullptr };
        OScriptInstance* instance{ nullptr };
        int* ip{ nullptr };
        int* node{ nullptr };
        CallLevel* prev{ nullptr };
    };
    #endif

    static thread_local int _debug_parse_err_line;
    static thread_local String _debug_parse_err_file;
    static thread_local String _debug_error;
    static thread_local CallLevel* _call_stack;
    static thread_local uint32_t _call_stack_size;
    uint32_t _debug_max_call_stack = 0;

    bool track_call_stack = false;
    bool track_locals = false;

    static CallLevel* _get_stack_level(uint32_t p_level);

    void _add_global(const StringName& p_name, const Variant& p_value);
    void _remove_global(const StringName& p_name);

    Ref<Mutex> lock;

    SelfList<OScript>::List _scripts;
    SelfList<OScriptCompiledFunction>::List function_list;
    #ifdef DEBUG_ENABLED
    bool profiling;
    bool profile_native_calls;
    uint64_t script_frame_time;
    #endif

    HashMap<String, ObjectID> orphan_subclasses;
    String _extension = ORCHESTRATOR_SCRIPT_TEXT_EXTENSION;

protected:
    static void _bind_methods();

public:
    //~ Begin ScriptLanguageExtension Interface
    String _get_name() const override;
    void _init() override;
    String _get_type() const override;
    String _get_extension() const override;
    void _finish() override;
    PackedStringArray _get_reserved_words() const override { return {}; }
    Ref<Script> _make_template(const String& p_template, const String& p_class_name, const String& p_base_class_name) const override;
    TypedArray<Dictionary> _get_built_in_templates(const StringName& p_object) const override;
    bool _is_using_templates() override { return true; }
    Dictionary _validate(const String& p_script, const String& p_path, bool p_validate_functions, bool p_validate_errors, bool p_validate_warnings, bool p_validate_safe_lines) const override;
    String _validate_path(const String &p_path) const override { return {}; }
    Object* _create_script() const override;
    bool _has_named_classes() const override { return false; }
    bool _supports_builtin_mode() const override { return true; }
    bool _supports_documentation() const override { return false; }
    bool _can_inherit_from_file() const override { return true; }
    int32_t _find_function(const String& p_function_name, const String& p_code) const override;
    String _make_function(const String& p_class_name, const String& p_function_name, const PackedStringArray& p_function_args) const override;
    #if GODOT_VERSION >= 0x040300
    bool _can_make_function() const override { return true; }
    #endif
    Error _open_in_external_editor(const Ref<Script>& p_script, int32_t p_line, int32_t p_column) override { return OK; }
    bool _overrides_external_editor() override { return true; }
    void _add_global_constant(const StringName& p_name, const Variant& p_value) override;
    void _add_named_global_constant(const StringName& p_name, const Variant& p_value) override;
    void _remove_named_global_constant(const StringName& p_name) override;
    void _thread_enter() override {}
    void _thread_exit() override {}
    String _debug_get_error() const override;
    int32_t _debug_get_stack_level_count() const override;
    int32_t _debug_get_stack_level_line(int32_t p_level) const override;
    String _debug_get_stack_level_function(int32_t p_level) const override;
    String _debug_get_stack_level_source(int32_t p_level) const override;
    Dictionary _debug_get_stack_level_locals(int32_t p_level, int32_t p_max_subitems, int32_t p_max_depth) override;
    Dictionary _debug_get_stack_level_members(int32_t p_level, int32_t p_max_subitems, int32_t p_max_depth) override;
    void* _debug_get_stack_level_instance(int32_t p_level) override;
    Dictionary _debug_get_globals(int32_t p_max_subitems, int32_t p_max_depth) override;
    String _debug_parse_stack_level_expression(int32_t p_level, const String& p_expression, int32_t p_max_subitems, int32_t p_max_depth) override;
    TypedArray<Dictionary> _debug_get_current_stack_info() override;
    void _reload_all_scripts() override;
    void _reload_scripts(const Array& p_scripts, bool p_soft_reload) override;
    void _reload_tool_script(const Ref<Script>& p_script, bool p_soft_reload) override;
    PackedStringArray _get_recognized_extensions() const override;
    TypedArray<Dictionary> _get_public_functions() const override;
    Dictionary _get_public_constants() const override;
    TypedArray<Dictionary> _get_public_annotations() const override { return {}; }
    void _profiling_start() override;
    void _profiling_stop() override;
    void _profiling_set_save_native_calls(bool p_enable) override;
    int32_t _profiling_get_accumulated_data(ScriptLanguageExtensionProfilingInfo* p_info_array, int32_t p_info_max) override;
    int32_t _profiling_get_frame_data(ScriptLanguageExtensionProfilingInfo* p_info_array, int32_t p_info_max) override;
    void _frame() override;
    bool _handles_global_class_type(const String& p_type) const override;
    Dictionary _get_global_class_name(const String& p_path) const override;
    //~ End ScriptLanguageExtension Interface

    struct {
        StringName _init;
        StringName _static_init;
        StringName _notification;
        StringName _set;
        StringName _get;
        StringName _get_property_list;
        StringName _validate_property;
        StringName _property_can_revert;
        StringName _property_get_revert;
        StringName _script_source;
    } strings;

    _FORCE_INLINE_ bool should_track_call_stack() const { return track_call_stack; }
    _FORCE_INLINE_ bool should_track_locals() const { return track_locals; }
    _FORCE_INLINE_ int get_global_array_size() const { return global_array.size(); }
    _FORCE_INLINE_ Variant* get_global_array() { return _global_array; }
    _FORCE_INLINE_ const HashMap<StringName, int>& get_global_map() const { return globals; }
    _FORCE_INLINE_ const HashMap<StringName, Variant>& get_named_globals_map() const { return named_globals; }

    void add_orphan_subclass(const String& p_qualified_name, const ObjectID& p_subclass);
    Ref<OScript> get_orphan_subclass(const String& p_qualified_name);

    bool has_any_global_constant(const StringName& p_name) const { return named_globals.has(p_name) || globals.has(p_name); }
    Variant get_any_global_constant(const StringName& p_name);
    PackedStringArray get_global_constant_names() const;
    PackedStringArray get_global_named_constant_names() const;

    // Debugging
    bool debug_break(const String& p_error, bool p_allow_continue = true);
    bool debug_break_parse(const String& p_file, int p_node, const String& p_error);
    void enter_function(CallLevel* p_level, OScriptInstance* p_instance, OScriptCompiledFunction* p_function, Variant* p_stack, int* p_ip, int* p_node);
    void exit_function();

    // Profiling
    void profiling_collate_native_call_data(bool p_accumulated);

    Ref<OScript> get_script_by_fully_qualified_name(const String& p_name);
    String get_script_extension_filter() const;

    static OScriptLanguage* get_singleton();

    OScriptNodePrintStringOverlay* get_or_create_overlay();

    OScriptLanguage();
    ~OScriptLanguage() override;
};

#endif  // ORCHESTRATOR_SCRIPT_LANGUAGE_H
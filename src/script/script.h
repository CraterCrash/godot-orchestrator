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
#ifndef ORCHESTRATOR_SCRIPT_H
#define ORCHESTRATOR_SCRIPT_H

#include "core/godot/doc_data.h"
#include "orchestration/orchestration.h"
#include "script/compiler/compiled_function.h"
#include "script/script_instance.h"
#include "script/script_native_class.h"
#include "script/script_source.h"

#include <gdextension_interface.h>
#include <godot_cpp/classes/script_extension.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>

using namespace godot;

/// Forward declarations
class OScriptAnalyzer;
class OScriptCompiledFunction;
class OScriptCompiler;
class OScriptInstance;
class OScriptLanguage;
class OScriptPlaceHolderInstance;

/// Defines the script extension for OScript, a visual scripting engine that uses <code>Orchestration</code>
/// resources to define the script's behavior.
///
class OScript : public ScriptExtension {
    GDCLASS(OScript, ScriptExtension);

    friend class OScriptAnalyzer;
    friend class OScriptCompiledFunction;
    friend class OScriptCompiler;
    friend class OScriptDocGen;
    friend class OScriptInstance;
    friend class OScriptLanguage;

    struct MemberInfo {
        int index = 0;
        StringName setter;
        StringName getter;
        OScriptDataType data_type;
        PropertyInfo property_info;
    };

    struct ClearData {
        RBSet<OScriptCompiledFunction*> functions;
        RBSet<Ref<Script>> scripts;
        void clear() {
            functions.clear();
            scripts.clear();
        }
    };

    struct OScriptMemberSort {
        int index = 0;
        StringName name;
        _FORCE_INLINE_ bool operator<(const OScriptMemberSort &p_member) const { return index < p_member.index; }
    };

    Ref<Orchestration> orchestration;

    bool _tool = false;
    bool _valid = false;
    bool reloading = false;
    bool is_abstract = false;
    bool _placeholder_fallback_enabled = false;
    Ref<OScriptNativeClass> native;
    Ref<OScript> base;
    OScript* subclass_owner = nullptr;
    OScriptLanguage* _language = nullptr;
    OScriptSource source;

    HashMap<StringName, MemberInfo> member_indices;
    HashSet<StringName> members;

    HashMap<StringName, MemberInfo> static_variables_indices;
    Vector<Variant> static_variables;

    HashMap<StringName, Variant> constants;
    HashMap<StringName, OScriptCompiledFunction*> member_functions;
    HashMap<StringName, Ref<OScript>> subclasses;
    HashMap<StringName, MethodInfo> signals;
    Dictionary rpc_config;

    struct LambdaInfo {
        int capture_count;
        bool use_self;
    };
    HashMap<OScriptCompiledFunction*, LambdaInfo> lambda_info;

    class UpdatableFuncPtr {
        friend class OScript;

        OScriptCompiledFunction* ptr = nullptr;
        OScript* script = nullptr;
        List<UpdatableFuncPtr*>::Element* list_element = nullptr;

    public:
        OScriptCompiledFunction* operator->() const { return ptr; }
        operator OScriptCompiledFunction*() const { return ptr; }

        explicit UpdatableFuncPtr(OScriptCompiledFunction* p_function);
        ~UpdatableFuncPtr();
    };

    List<UpdatableFuncPtr*> func_ptrs_to_update;
    Ref<Mutex> func_ptrs_to_update_mutex;

    void _recurse_replace_function_ptrs(const HashMap<OScriptCompiledFunction*, OScriptCompiledFunction*>& p_replacements) const;

    #ifdef TOOLS_ENABLED
    bool source_changed_cache = false;
    uint64_t source_last_modified_time = 0;
    HashMap<StringName, MemberInfo> old_static_variables_indices;
    Vector<Variant> old_static_variables;
    void _save_old_static_data();
    void _restore_old_static_data();

    HashMap<StringName, int> member_node_ids;
    HashMap<StringName, Variant> member_default_values;
    List<PropertyInfo> members_cache;
    HashMap<StringName, Variant> member_default_values_cache;
    Ref<OScript> base_cache;
    HashSet<ObjectID> inheritors_cache;
    StringName doc_class_name;
    DocData::ClassDoc doc;
    Vector<DocData::ClassDoc> docs;
    void _add_doc(const DocData::ClassDoc& p_doc);
    void _clear_doc();
    #endif

    OScriptCompiledFunction* initializer = nullptr; // Direct pointer to `new()` / `_init()`.
    OScriptCompiledFunction* implicit_initializer = nullptr; // `@implicit_new()` special function.
    OScriptCompiledFunction* implicit_ready = nullptr; // `@implicit_ready()` special function.
    OScriptCompiledFunction* static_initializer = nullptr; // `@static_initializer()` special function

    mutable RBSet<Object*> instances;
    mutable HashMap<Object*, OScriptInstanceBase*> instance_script_instances;
    #ifdef TOOLS_ENABLED
    mutable HashSet<OScriptPlaceHolderInstance*> placeholders;
    #endif

    bool destructing = false;
    bool clearing = false;
    bool path_valid = false;
    String path;
    StringName local_name;
    StringName global_name;
    String fully_qualified_name;
    String simplified_icon_path;
    SelfList<OScript> script_list;
    SelfList<OScriptFunctionState>::List pending_func_states;

    #ifdef DEBUG_ENABLED
    HashMap<ObjectID, List<Pair<StringName, Variant>>> pending_reload_state;
    #endif

    Error _static_init();
    void _static_default_init(); // Initialize static variables with default values based on types.

    Variant callp(const StringName& p_method, const Variant** p_args, int p_arg_count, GDExtensionCallError& r_error);

    OScriptCompiledFunction* _super_constructor(OScript* p_script);
    void _super_implicit_constructor(OScript* p_script, OScriptInstance* p_instance, GDExtensionCallError& r_error);

    OScriptInstance* _create_instance(const Variant** p_args, int p_arg_count, Object* p_owner, GDExtensionCallError& r_error) const;

    String _get_debug_path() const;

    void _update_export_values(HashMap<StringName, Variant>& r_values, List<PropertyInfo>& r_properties);
    bool _update_exports_placeholder(bool* r_err = nullptr, bool p_recursive = false, OScriptPlaceHolderInstance* p_instance_to_update = nullptr, bool p_base_exports_changed = false);
    void _update_exports_down(bool p_base_exports_changed);

    TypedArray<Dictionary> _get_script_properties(bool p_include_base) const;
    TypedArray<Dictionary> _get_script_methods(bool p_include_base) const;
    TypedArray<Dictionary> _get_script_signals(bool p_include_base) const;

    OScript* _get_from_variant(const Variant& p_value);
    void _collect_function_dependencies(OScriptCompiledFunction* p_function, RBSet<OScript*>& p_dependencies, const OScript* p_except);
    void _collect_dependencies(RBSet<OScript*>& p_dependencies, const OScript* p_except);

protected:
    static void _bind_methods();

public:
    //~ Begin ScriptExtension Interface
    bool _editor_can_reload_from_file() override;
    void _placeholder_erased(void* p_placeholder) override;
    bool _can_instantiate() const override;
    Ref<Script> _get_base_script() const override;
    StringName _get_global_name() const override;
    bool _inherits_script(const Ref<Script>& p_script) const override;
    StringName _get_instance_base_type() const override;
    void* _instance_create(Object* p_object) const override;
    void* _placeholder_instance_create(Object* p_object) const override;
    bool _instance_has(Object* p_object) const override;
    bool _has_source_code() const override;
    String _get_source_code() const override;
    void _set_source_code(const String& p_code) override;
    Error _reload(bool p_keep_state) override;
    #ifdef TOOLS_ENABLED
    StringName _get_doc_class_name() const override;
    TypedArray<Dictionary> _get_documentation() const override;
    String _get_class_icon_path() const override;
    #endif
    bool _has_method(const StringName& p_method) const override;
    bool _has_static_method(const StringName& p_method) const override;
    Variant _get_script_method_argument_count(const StringName& p_method) const override;
    Dictionary _get_method_info(const StringName& p_method) const override;
    bool _is_tool() const override;
    bool _is_valid() const override;
    bool _is_abstract() const override;
    ScriptLanguage* _get_language() const override;
    bool _has_script_signal(const StringName& p_signal) const override;
    TypedArray<Dictionary> _get_script_signal_list() const override;
    bool _has_property_default_value(const StringName& p_property) const override;
    Variant _get_property_default_value(const StringName& p_property) const override;
    void _update_exports() override;
    TypedArray<Dictionary> _get_script_method_list() const override;
    TypedArray<Dictionary> _get_script_property_list() const override;
    int32_t _get_member_line(const StringName& p_member) const override;
    Dictionary _get_constants() const override;
    TypedArray<StringName> _get_members() const override;
    bool _is_placeholder_fallback_enabled() const override;
    Variant _get_rpc_config() const override;
    //~ End ScriptExtension Interface

    #ifdef DEBUG_ENABLED
    static String debug_get_script_name(const Ref<Script> &p_script);
    #endif

    static String canonicalize_path(const String& p_path);
    _FORCE_INLINE_ static bool is_canonically_equal_paths(const String &p_path_a, const String &p_path_b) {
        return canonicalize_path(p_path_a) == canonicalize_path(p_path_b);
    }

    //~ Begin Script Interface
    //~ NOTE: These are not exposed to GDExtension, but are added here for compatibility
    ScriptLanguage* get_language() const { return _get_language(); }
    void reload_from_file();

    #ifdef TOOLS_ENABLED
    // This is provided by Script in the engine, but it isn't exposed to GDE
    PropertyInfo get_class_category() const;
    #endif
    //~ End Script Interface

    //~ Helper methods similar to GDScript.
    Variant _new(const Variant** p_args, GDExtensionInt p_arg_count, GDExtensionCallError& r_error);

    Ref<OScript> get_base() const { return base; }
    String get_script_path() const;

    _FORCE_INLINE_ StringName get_local_name() const { return local_name; }

    void clear(ClearData* p_clear_data = nullptr);

    // Cancels all functions of the script that are waiting to be resumed after using await.
    void cancel_pending_functions(bool p_warn);

    OScript *find_class(const String &p_qualified_name);
    bool has_class(const OScript *p_script);
    OScript* get_root_script();
    bool is_root_script() const { return subclass_owner == nullptr; }
    String get_fully_qualified_class_name() const { return fully_qualified_name; }

    const OScriptDataType& get_member_type(const StringName &p_member) const {
        CRASH_COND(!member_indices.has(p_member));
        return member_indices[p_member].data_type;
    }

    const Ref<OScriptNativeClass>& get_native() const { return native; }

    _FORCE_INLINE_ const HashMap<StringName, OScriptCompiledFunction*>& get_member_functions() const { return member_functions; }

    _FORCE_INLINE_ const OScriptCompiledFunction* get_implicit_initializer() const { return implicit_initializer; }
    _FORCE_INLINE_ const OScriptCompiledFunction* get_implicit_ready() const { return implicit_ready; }
    _FORCE_INLINE_ const OScriptCompiledFunction* get_static_initializer() const { return static_initializer; }

    RBSet<OScript*> get_dependencies();
    HashMap<OScript*, RBSet<OScript*>> get_all_dependencies();
    RBSet<OScript*> get_must_clear_dependencies();

    const HashMap<StringName, MemberInfo>& debug_get_member_indices() const { return member_indices; }
    const HashMap<StringName, OScriptCompiledFunction*>& debug_get_member_functions() const { return member_functions; } // debug only
    StringName debug_get_member_by_index(int p_index) const;
    StringName debug_get_static_var_by_index(int p_index) const;

    bool get_property_default_value(const StringName& p_property, Variant& r_value) const;
    void get_constants(HashMap<StringName, Variant>* r_constants);

    void unload_static() const;
    //~ End Helper methods

    Ref<Orchestration> get_orchestration();

    void set_edited(bool p_edited);

    void set_source(const OScriptSource& p_source);
    Error load_source_code(const String& p_path);

    #ifdef DEV_TOOLS
    String dump_compiled_state();
    #endif

    OScript();
    ~OScript() override;
};

#endif  // ORCHESTRATOR_SCRIPT_H
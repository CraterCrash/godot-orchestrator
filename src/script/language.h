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
#ifndef ORCHESTRATOR_SCRIPT_LANGUAGE_H
#define ORCHESTRATOR_SCRIPT_LANGUAGE_H

#include "common/logger.h"
#include "common/version.h"
#include "serialization/format_defs.h"

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

/// Defines the script node creation function callback
typedef Ref<OScriptNode> (*OScriptNodeRegisterFunc)(const String& p_type);

/// Defines an extension for Godot where we define the language for Orchestrations.
class OScriptLanguage : public ScriptLanguageExtension
{
    GDCLASS(OScriptLanguage, ScriptLanguageExtension);

protected:
    static void _bind_methods() { }

private:

    // Structure that describes a registered node that provides some unique
    // functionality to the visual script subsystem.
    struct ScriptNodeInfo
    {
        ScriptNodeInfo* inherits_ptr { nullptr };
        void* class_ptr { nullptr };
        StringName inherits;
        StringName name;

        Object* (*creation_func)() { nullptr };

        ScriptNodeInfo() = default;
        ~ScriptNodeInfo() = default;
    };

    #if GODOT_VERSION >= 0x040300
    struct CallStack
    {
        Variant* stack{ nullptr };
        Variant** working_memory{ nullptr };
        const StringName* current_function{ nullptr };
        OScriptInstance* instance{ nullptr };
        int* id{ nullptr };
    };
    #endif

    static OScriptLanguage* _singleton;                        //! The one and only instance
    static HashMap<StringName, ScriptNodeInfo> _nodes;         //! Script node registration data
    SelfList<OScript>::List _scripts;                          //! all loaded scripts
    HashMap<StringName, Variant> _global_constants;            //! Stores global constants
    HashMap<StringName, Variant> _named_global_constants;      //! Stores named global constants
    String _extension{ ORCHESTRATOR_SCRIPT_TEXT_EXTENSION };   //! The language's extension

    #if GODOT_VERSION >= 0x040300
    int _debug_parse_err_line{ -1 };    //! The line number of the parse error
    String _debug_parse_err_file;       //! The script file name of the parse error
    String _debug_error;                //! The error message
    int _debug_call_stack_pos{ 0 };     //! The current call stack position
    int _debug_max_call_stack{ 0 };     //! The maximum call stack size
    CallStack* _call_stack{ nullptr };  //! The call stack
    #endif

protected:

    /// Standard creator method for nodes
    /// @tparam T the node type
    /// @return the created node
    template <typename T>
    static Object* creator()
    {
        return memnew(T);
    }

    // Internal Registration
    static void _add_node_class_internal(const StringName& p_class, const StringName& p_inherits);

public:
    /// Public lock used for specific synchronizing use cases.
    Ref<Mutex> lock;

    /// The language's type
    static inline const char* TYPE = "Orchestrator";

    /// The language's default icon
    static inline const char* ICON = "res://addons/orchestrator/icons/Orchestrator_16x16.png";

    /// Get the singleton instance for the language.
    /// @return the language instance
    static OScriptLanguage* get_singleton();

    /// Constructs the OScriptLanguage instance, assigning the singleton.
    OScriptLanguage();

    /// Destroys the OScriptLanguage instance, clearing the singleton reference.
    ~OScriptLanguage() override;

    //~ Begin ScriptLanguageExtension Interface
    void _init() override;
    String _get_name() const override;
    String _get_type() const override;
    String _get_extension() const override;
    PackedStringArray _get_recognized_extensions() const override;
    bool _can_inherit_from_file() const override;
    bool _supports_builtin_mode() const override;
    bool _supports_documentation() const override;
    bool _is_using_templates() override;
    TypedArray<Dictionary> _get_built_in_templates(const StringName& p_object) const override;
    Ref<Script> _make_template(const String& p_template, const String& p_class_name,
                               const String& p_base_class_name) const override;
    bool _overrides_external_editor() override;
    Error _open_in_external_editor(const Ref<Script>& p_script, int32_t p_line, int32_t p_column) override;
    String _validate_path(const String& p_path) const override;
    Dictionary _validate(const String& p_script, const String& p_path, bool p_validate_functions,
                         bool p_validate_errors, bool p_validate_warnings, bool p_validate_safe_lines) const override;
    Object* _create_script() const override;
    PackedStringArray _get_comment_delimiters() const override;
    PackedStringArray _get_string_delimiters() const override;
    PackedStringArray _get_reserved_words() const override;
    bool _has_named_classes() const override;
    bool _is_control_flow_keyword(const String& p_keyword) const override;
    void _add_global_constant(const StringName& p_name, const Variant& p_value) override;
    void _add_named_global_constant(const StringName& p_name, const Variant& p_value) override;
    void _remove_named_global_constant(const StringName& p_name) override;
    int32_t _find_function(const String& p_function_name, const String& p_code) const override;
    String _make_function(const String& p_class_name, const String& p_function_name,
                          const PackedStringArray& p_function_args) const override;
    #if GODOT_VERSION >= 0x040300
    bool _can_make_function() const override;
    #endif
    TypedArray<Dictionary> _get_public_functions() const override;
    Dictionary _get_public_constants() const override;
    TypedArray<Dictionary> _get_public_annotations() const override;
    String _auto_indent_code(const String& p_code, int32_t p_from_line, int32_t p_to_line) const override;
    Dictionary _lookup_code(const String& p_code, const String& p_symbol, const String& p_path,
                            Object* p_owner) const override;
    Dictionary _complete_code(const String& p_code, const String& p_path, Object* p_owner) const override;
    void _reload_all_scripts() override;
    void _reload_tool_script(const Ref<Script>& p_script, bool p_soft_reload) override;
    void _thread_enter() override;
    void _thread_exit() override;
    void _profiling_start() override;
    void _profiling_stop() override;
    void _frame() override;
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

    /// Adds the node clas to the language (DO NOT USE DIRECTLY!!!!)
    /// @tparam T the class type
    template <class T> static void _add_node_class()
    {
        if (T::get_class_static().match("OScriptNode"))
            _add_node_class_internal(T::get_class_static(), StringName());
        else
            _add_node_class_internal(T::get_class_static(), T::get_parent_class_static());
    }

    /// Registers a node class with the language
    /// @tparam T the node class type
    template <typename T>
    static void register_node_class()
    {
        static_assert(TypesAreSame<typename T::self_node_type, T>::value,
                      "Node not declared properly, please use ORCHESTRATOR_CLASS.");
        T::initialize_orchestrator_class();

        ScriptNodeInfo* node = _nodes.getptr(T::get_class_static());
        ERR_FAIL_NULL(node);
        node->creation_func = &creator<T>;
        node->class_ptr = T::get_orchestrator_node_ptr_static();

        T::register_custom_orchestrator_data_to_otdb();
        Logger::debug("Registered node '", T::get_class_static(), "'.");
    }

    #ifdef TOOLS_ENABLED
    /// Get a list of all orchestration scripts
    /// @return list of references
    List<Ref<OScript>> get_scripts() const;
    #endif

    /// Create a script node based on the name of the node.
    /// @param p_class_name the node class name
    /// @param p_owner the orchestration
    /// @param p_allocate_id allocate node id from script, defaults to true.
    /// @return the script node reference
    Ref<OScriptNode> create_node_from_name(const String& p_class_name, Orchestration* p_owner, bool p_allocate_id = true);

    /// Templated function to create a node from a node type
    /// @tparam T the node type
    /// @param p_owner the orchestration
    /// @return the created node reference, may be an invalid reference on failure
    template <typename T>
    Ref<T> create_node_from_type(Orchestration* p_owner)
    {
        for (const KeyValue<StringName, ScriptNodeInfo>& E : _nodes)
            if (E.key == T::get_class_static())
                return create_node_from_name(E.key, p_owner);
        ERR_FAIL_V_MSG(Ref<T>(), "No node found with class type: " + T::get_class_static());
    }
};

#define ORCHESTRATOR_NODE_CLASS_COMMON(m_class, m_inherits) /*************************************/ \
    GDCLASS(m_class, m_inherits);                                                                   \
private:                                                                                            \
    friend class ::OScriptLanguage;                                                                 \
public:                                                                                             \
    typedef m_class self_node_type;                                                                 \
    typedef m_inherits super;                                                                       \
    static _FORCE_INLINE_ void* get_orchestrator_node_ptr_static()                                  \
    {                                                                                               \
        static int ptr;                                                                             \
        return &ptr;                                                                                \
    }

#define ORCHESTRATOR_NODE_CLASS_BASE(m_class, m_inherits) /***************************************/ \
    ORCHESTRATOR_NODE_CLASS_COMMON(m_class, m_inherits);                                            \
public:                                                                                             \
    static void initialize_orchestrator_class()                                                     \
    {                                                                                               \
        static bool orchestrator_initialized = false;                                               \
        if (orchestrator_initialized)                                                               \
        {                                                                                           \
            return;                                                                                 \
        }                                                                                           \
        OScriptLanguage::_add_node_class<m_class>();                                                \
        orchestrator_initialized = true;                                                            \
    }                                                                                               \
protected:                                                                                          \
    virtual void _initialize_orchestator_classv()                                                   \
    {                                                                                               \
        initialize_orchestrator_class();                                                            \
    }

#define ORCHESTRATOR_NODE_CLASS(m_class, m_inherits) /********************************************/ \
    ORCHESTRATOR_NODE_CLASS_COMMON(m_class, m_inherits);                                            \
public:                                                                                             \
    static void initialize_orchestrator_class()                                                     \
    {                                                                                               \
        static bool orchestrator_initialized = false;                                               \
        if (orchestrator_initialized)                                                               \
        {                                                                                           \
            return;                                                                                 \
        }                                                                                           \
        m_inherits::initialize_orchestrator_class();                                                \
        OScriptLanguage::_add_node_class<m_class>();                                                \
        orchestrator_initialized = true;                                                            \
    }                                                                                               \
protected:                                                                                          \
    virtual void _initialize_orchestator_classv() override                                          \
    {                                                                                               \
        initialize_orchestrator_class();                                                            \
    }                                                                                               \
private:

#define ORCHESTRATOR_REGISTER_NODE_CLASS(m_class) /***********************************************/ \
    GDREGISTER_CLASS(m_class);                                                                      \
    ::OScriptLanguage::register_node_class<m_class>();

#define ORCHESTRATOR_REGISTER_ABSTRACT_NODE_CLASS(m_class) /**************************************/ \
    GDREGISTER_ABSTRACT_CLASS(m_class);                                                             \
    ::OScriptLanguage::register_node_class<m_class>();

#define ORCHESTRATOR_REGISTER_CLASS(m_class) /****************************************************/ \
    GDREGISTER_CLASS(m_class)

#define ORCHESTRATOR_REGISTER_INTERNAL_CLASS(m_class) /*******************************************/ \
    GDREGISTER_INTERNAL_CLASS(m_class)

#endif  // ORCHESTRATOR_SCRIPT_LANGUAGE_H
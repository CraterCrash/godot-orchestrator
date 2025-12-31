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
#ifndef ORCHESTRATOR_SCRIPT_NODE_FACTORY_H
#define ORCHESTRATOR_SCRIPT_NODE_FACTORY_H

#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/templates/hash_map.hpp>

using namespace godot;

/// Forward declarations
class Orchestration;
class OScriptNode;

/// Factory that stores and provides a way to create OScriptNode instances
class OScriptNodeFactory {
    // Describes a registered script node that provides functionality
    struct ScriptNodeInfo {
        StringName name;
        StringName inherits;
        ScriptNodeInfo* inherits_ptr = nullptr;
        void* class_ptr = nullptr;
        Object* (*creation_func)() = nullptr;
    };

    static HashMap<StringName, ScriptNodeInfo> _nodes;

    /// Standard creator method for nodes
    /// @tparam T the node class type
    /// @return the node instance
    template <typename T>
    static Object* creator() { return memnew(T); }

    /// Registers the class.
    /// Classes should be registered in hierarchical order, parents before children.
    /// @param p_class the node class name to register
    /// @param p_inherits the parent node class name of the registered subject
    static void _add_node_class(const StringName& p_class, const StringName& p_inherits);

    /// Checks whether the class is the base script node type
    /// @param p_class the class name to check
    /// @return true if it's the base node type, false otherwise
    static bool _is_base_node_type(const StringName& p_class);

public:

    /// Adds a node class to the factory by type
    /// @tparam T the node class type
    template <class T>
    static void add_node_class() {
        const bool is_base_node = _is_base_node_type(T::get_class_static());
        _add_node_class(T::get_class_static(), is_base_node ? StringName() : T::get_parent_class_static());
    }

    /// Registers the node class with the factory
    /// @tparam T the node class type
    template <typename T>
    static void register_node_class() {
        static_assert(TypesAreSame<typename T::self_node_type, T>::value,
              "Node not declared properly, please use ORCHESTRATOR_CLASS.");

        T::initialize_orchestrator_class();

        // Node should already be registered in factory using `add_node_class`
        // This is done via the ORCHESTRATOR_REGISTER_NODE_CLASS and friend macros
        ScriptNodeInfo* node_info = _nodes.getptr(T::get_class_static());
        ERR_FAIL_NULL(node_info);

        node_info->creation_func = &creator<T>;
        node_info->class_ptr = T::get_orchestrator_node_ptr_static();

        T::register_custom_orchestrator_data_to_otdb();
    }

    /// Creates an Orchestration node instance by name
    /// @param p_class_name the node class type to create
    /// @param p_owner the orchestration that should own the node instance
    /// @return the script node reference
    static Ref<OScriptNode> create_node_from_name(const String& p_class_name, Orchestration* p_owner);

    /// Creates an Orchestration node instance by type
    /// @tparam T the node type to create
    /// @param p_owner the orchestration that should own the node instance
    /// @return the node reference
    template <typename T>
    static Ref<T> create_node_from_type(Orchestration* p_owner) {
        if (_nodes.has(T::get_class_static())) {
            return create_node_from_name(T::get_class_static(), p_owner);
        }
        ERR_FAIL_V_MSG(Ref<T>(), "No node definition found with class type: " + T::get_class_static());
    }
};

#define ORCHESTRATOR_NODE_CLASS_COMMON(m_class, m_inherits) /*************************************/ \
    GDCLASS(m_class, m_inherits);                                                                   \
private:                                                                                            \
    friend class ::OScriptNodeFactory;                                                              \
public:                                                                                             \
    typedef m_class self_node_type;                                                                 \
    typedef m_inherits super;                                                                       \
    static _FORCE_INLINE_ void* get_orchestrator_node_ptr_static() {                                \
        static int ptr;                                                                             \
        return &ptr;                                                                                \
    }

#define ORCHESTRATOR_NODE_CLASS_BASE(m_class, m_inherits) /***************************************/ \
    ORCHESTRATOR_NODE_CLASS_COMMON(m_class, m_inherits);                                            \
public:                                                                                             \
    static void initialize_orchestrator_class() {                                                   \
        static bool orchestrator_initialized = false;                                               \
        if (orchestrator_initialized) {                                                             \
            return;                                                                                 \
        }                                                                                           \
        OScriptNodeFactory::add_node_class<m_class>();                                              \
        orchestrator_initialized = true;                                                            \
    }                                                                                               \
protected:                                                                                          \
    virtual void _initialize_orchestator_classv() {                                                 \
        initialize_orchestrator_class();                                                            \
    }                                                                                               \
private:

#define ORCHESTRATOR_NODE_CLASS(m_class, m_inherits) /********************************************/ \
    ORCHESTRATOR_NODE_CLASS_COMMON(m_class, m_inherits);                                            \
public:                                                                                             \
    static void initialize_orchestrator_class() {                                                   \
        static bool orchestrator_initialized = false;                                               \
        if (orchestrator_initialized) {                                                             \
            return;                                                                                 \
        }                                                                                           \
        m_inherits::initialize_orchestrator_class();                                                \
        OScriptNodeFactory::add_node_class<m_class>();                                              \
        orchestrator_initialized = true;                                                            \
    }                                                                                               \
protected:                                                                                          \
    virtual void _initialize_orchestator_classv() override {                                        \
        initialize_orchestrator_class();                                                            \
    }                                                                                               \
private:

#define ORCHESTRATOR_REGISTER_NODE_CLASS(m_class) /***********************************************/ \
    GDREGISTER_CLASS(m_class);                                                                      \
    ::OScriptNodeFactory::register_node_class<m_class>();

#define ORCHESTRATOR_REGISTER_ABSTRACT_NODE_CLASS(m_class) /**************************************/ \
    GDREGISTER_ABSTRACT_CLASS(m_class);                                                             \
    ::OScriptNodeFactory::register_node_class<m_class>();

#endif // ORCHESTRATOR_SCRIPT_NODE_FACTORY_H

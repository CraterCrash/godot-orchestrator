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
#ifndef ORCHESTRATOR_GRAPH_DEFAULT_ACTION_REGISTRAR_H
#define ORCHESTRATOR_GRAPH_DEFAULT_ACTION_REGISTRAR_H

#include "action_registrar.h"

/// The default OrchestratorGraphActionRegistrar, which registers the standard OScript script nodes
/// and any nodes related to the filter or context details.
class OrchestratorDefaultGraphActionRegistrar : public OrchestratorGraphActionRegistrar
{
    GDCLASS(OrchestratorDefaultGraphActionRegistrar, OrchestratorGraphActionRegistrar);
    static void _bind_methods() { }

protected:
    const OrchestratorGraphActionRegistrarContext* _context{ nullptr };   //! The action registrar context
    Vector<String> _classes_new_instances;
    bool _friendly_method_names{ false };

    /// Get the method's icon
    /// @param p_method the method
    /// @return the icon name to be used
    static String _get_method_icon(const MethodInfo& p_method);

    /// Get the method's type icon
    /// @param p_method the method
    /// @return the type icon, such as event, function, etc.
    static String _get_method_type_icon(const MethodInfo& p_method);

    /// Get the specified native class hierarchy
    /// @param p_derived_class_name the class name
    /// @return array of classes in the class hierarchy
    static PackedStringArray _get_class_hierarchy(const String& p_derived_class_name);

    /// Get the built-in type icon name
    /// @param p_type the built-in type
    /// @return the icon name
    String _get_builtin_type_icon_name(Variant::Type p_type) const;

    /// Get the built-in type display name
    /// @param p_type the built-in type
    /// @return the type name
    String _get_builtin_type_display_name(Variant::Type p_type) const;

    /// Returns a method action specification
    /// @param p_method the method
    /// @param p_base_type the orchestration base type
    /// @param p_class_name the class name to register the method under, optional
    /// @return the action spec
    OrchestratorGraphActionSpec _get_method_spec(const MethodInfo& p_method, const String& p_base_type, const String& p_class_name = "");

    /// Returns a signal action item specification
    /// @param p_signal_name the signal name
    /// @param p_base_type the orchestration base type
    /// @param p_class_name the class name to register the signal under, optional
    /// @return the action spec
    OrchestratorGraphActionSpec _get_signal_spec(const StringName& p_signal_name, const String& p_base_type, const String& p_class_name = "");

    /// Register a script node by class
    /// @param p_class_name the script node class name
    /// @param p_category the category to register the node under
    /// @param p_data any additional data to pass to the constructed node
    void _register_node(const StringName& p_class_name, const StringName& p_category, const Dictionary& p_data = Dictionary());

    /// Registration that uses OrchestratorGraphNodeSpawnerScriptNode
    /// @param p_category the category to register the node under
    /// @param p_data the additional data to pass to the constructed node
    template <typename T> void _register_node(const StringName& p_category, const Dictionary& p_data = Dictionary())
    {
        _register_node(T::get_class_static(), p_category, p_data);
    }

    /// Register category
    /// @param p_category the category name
    /// @param p_display_name the category display name
    /// @param p_icon the icon to use for the category
    void _register_category(const String& p_category, const String& p_display_name, const String& p_icon = String());

    /// Registers a specific class
    void _register_class(const String& p_class_name);

    /// Registers an array of method definitions with the specified class name
    /// @param p_class_name the class name
    /// @param p_methods array of methods
    void _register_methods(const String& p_class_name, const TypedArray<Dictionary>& p_methods);

    /// Registers an array of property definitions with the specified class name
    /// @param p_class_name the class name
    /// @param p_properties array of properties
    void _register_properties(const String& p_class_name, const TypedArray<Dictionary>& p_properties);

    /// Register an array of signal definitions with the specified class name
    /// @param p_class_name the class name
    /// @param p_signals array of signals
    void _register_signals(const String& p_class_name, const TypedArray<Dictionary>& p_signals);

    /// Registers all orchestration script nodes
    void _register_orchestration_nodes();

    /// Registers all orchestration functions
    void _register_orchestration_functions();

    /// Registers all orchestration variables
    void _register_orchestration_variables();

    /// Registers all orchestration signals
    void _register_orchestration_signals();

public:
    //~ Begin OrchestratorGraphActionRegistrar Interface
    void register_actions(const OrchestratorGraphActionRegistrarContext& p_context) override;
    //~ End OrchestratorGraphActionRegistrar Interface
};

#endif  // ORCHESTRATOR_GRAPH_DEFAULT_ACTION_REGISTRAR_H
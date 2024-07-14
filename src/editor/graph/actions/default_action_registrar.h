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
#ifndef ORCHESTRATOR_GRAPH_DEFAULT_ACTION_REGISTRAR_H
#define ORCHESTRATOR_GRAPH_DEFAULT_ACTION_REGISTRAR_H

#include "action_registrar.h"

/// The default OrchestratorGraphActionRegistrar, which registers the standard OScript script nodes
/// and any nodes related to the filter or context details.
class OrchestratorDefaultGraphActionRegistrar : public OrchestratorGraphActionRegistrar
{
    GDCLASS(OrchestratorDefaultGraphActionRegistrar, OrchestratorGraphActionRegistrar)

    // Helper methods

    static String _get_method_signature(const MethodInfo& p_method);
    static String _get_method_icon(const MethodInfo& p_method);
    static String _get_method_type_icon(const MethodInfo& p_method);
    static PackedStringArray _get_class_hierarchy(const String& p_derived_class_name);
    static void _register_node(const OrchestratorGraphActionRegistrarContext& p_context, const StringName& p_class_name,
                               const StringName& p_category, const Dictionary& p_data = Dictionary());

    String _get_builtin_type_icon_name(Variant::Type p_type) const;
    String _get_builtin_type_display_name(Variant::Type p_type) const;

    /// Registration that uses OrchestratorGraphNodeSpawnerScriptNode
    template <typename T>
    void _register_node(const OrchestratorGraphActionRegistrarContext& p_context, const StringName& p_category,
                        const Dictionary& p_data = Dictionary())
    {
        _register_node(p_context, T::get_class_static(), p_category, p_data);
    }

    // Registration steps

    void _register_category(const OrchestratorGraphActionRegistrarContext& p_context, const String& p_category, const String& p_display_name, const String& p_icon = String());

    void _register_script_nodes(const OrchestratorGraphActionRegistrarContext& p_context);
    void _register_graph_items(const OrchestratorGraphActionRegistrarContext& p_context);
    void _register_class_properties(const OrchestratorGraphActionRegistrarContext& p_context, const StringName& p_class_name);
    void _register_class_methods(const OrchestratorGraphActionRegistrarContext& p_context, const StringName& p_class_name);
    void _register_class_signals(const OrchestratorGraphActionRegistrarContext& p_context, const StringName& p_class_name);
    void _register_script_functions(const OrchestratorGraphActionRegistrarContext& p_context);
    void _register_script_variables(const OrchestratorGraphActionRegistrarContext& p_context);
    void _register_script_signals(const OrchestratorGraphActionRegistrarContext& p_context);

protected:
    static void _bind_methods() { }

public:
    //~ Begin OrchestratorGraphActionRegistrar Interface
    void register_actions(const OrchestratorGraphActionRegistrarContext& p_context) override;
    //~ End OrchestratorGraphActionRegistrar Interface
};

#endif  // ORCHESTRATOR_GRAPH_DEFAULT_ACTION_REGISTRAR_H
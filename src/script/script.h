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
#ifndef ORCHESTRATOR_SCRIPT_H
#define ORCHESTRATOR_SCRIPT_H

#include "common/version.h"
#include "orchestration/orchestration.h"
#include "script/instances/instance_base.h"

#include <gdextension_interface.h>
#include <godot_cpp/classes/script_extension.hpp>
#include <godot_cpp/templates/hash_map.hpp>

using namespace godot;

/// Forward declarations
class OScriptInstance;
class OScriptPlaceHolderInstance;

/// Defines the script extension for Orchestrations.
///
/// An orchestration is a visual-script like graph of nodes that allows to build code visually.
/// These graphs are stored as a script that can be attached to any scene tree node and this is
/// the base class that offers that functionality.
///
class OScript : public ScriptExtension, public Orchestration
{
    friend class OScriptInstance;
    GDCLASS(OScript, ScriptExtension);

private:
    bool _tool{ false };                                   //! Is this script marked as a tool script
    bool _valid{ false };                                  //! Determines whether the script is currently valid
    bool _placeholder_fallback_enabled{ false };           //! Deals with placeholders
    OScriptLanguage* _language{ nullptr };                 //! The script language

    // these are mutable because they're modified within const function callbacks
    mutable HashMap<Object*, OScriptInstance*> _instances;
    mutable HashMap<uint64_t, OScriptPlaceHolderInstance*> _placeholders;

protected:
    // Godot bindings
    static void _bind_methods();

    //~ Begin Serialization API
    StringName _get_base_type() const { return _base_type; }
    void _set_base_type(const StringName& p_base_type) { set_base_type(p_base_type); }
    TypedArray<OScriptNode> _get_nodes() const { return _get_nodes_internal(); }
    void _set_nodes(const TypedArray<OScriptNode>& p_nodes) { _set_nodes_internal(p_nodes); }
    TypedArray<int> _get_connections() const { return _get_connections_internal(); }
    void _set_connections(const TypedArray<int>& p_connections) { _set_connections_internal(p_connections); }
    TypedArray<OScriptGraph> _get_graphs() const { return _get_graphs_internal(); }
    void _set_graphs(const TypedArray<OScriptGraph>& p_graphs) { _set_graphs_internal(p_graphs); }
    TypedArray<OScriptFunction> _get_functions() const { return _get_functions_internal(); }
    void _set_functions(const TypedArray<OScriptFunction>& p_functions) { _set_functions_internal(p_functions); }
    TypedArray<OScriptVariable> _get_variables() const { return _get_variables_internal(); }
    void _set_variables(const TypedArray<OScriptVariable>& p_variables) { _set_variables_internal(p_variables); }
    TypedArray<OScriptSignal> _get_signals() const { return _get_signals_internal(); }
    void _set_signals(const TypedArray<OScriptSignal>& p_signals) { _set_signals_internal(p_signals); }
    //~ End Serialization API

    /// Update export placeholders
    /// @param r_err the output error
    /// @param p_recursive whether called recursively
    /// @param p_instance the script instance, should never be null
    bool _update_exports_placeholder(bool* r_err, bool p_recursive, OScriptInstance* p_instance) const;

    /// Updates the placeholders
    void _update_placeholders() override;

public:
    OScript();

    //~ ScriptExtension overrides
    bool _editor_can_reload_from_file() override;
    void* _placeholder_instance_create(Object* p_object) const override;
    void _placeholder_erased(void* p_placeholder) override;
    bool _is_placeholder_fallback_enabled() const override;
    bool placeholder_has(Object* p_object) const;
    void* _instance_create(Object* p_object) const override;
    bool _instance_has(Object* p_object) const override;
    bool _can_instantiate() const override;
    Ref<Script> _get_base_script() const override;
    bool _inherits_script(const Ref<Script>& p_script) const override;
    StringName _get_global_name() const override;
    StringName _get_instance_base_type() const override;
    bool _has_source_code() const override;
    String _get_source_code() const override;
    void _set_source_code(const String& p_code) override;
    Error _reload(bool p_keep_state) override;
    TypedArray<Dictionary> _get_documentation() const override;
    bool _has_static_method(const StringName& p_method) const override;
    bool _has_method(const StringName& p_method) const override;
    Dictionary _get_method_info(const StringName& p_method) const override;
    TypedArray<Dictionary> _get_script_method_list() const override;
    TypedArray<Dictionary> _get_script_property_list() const override;
    bool _is_tool() const override;
    bool _is_valid() const override;
    ScriptLanguage* _get_language() const override;
    bool _has_script_signal(const StringName& p_signal) const override;
    TypedArray<Dictionary> _get_script_signal_list() const override;
    bool _has_property_default_value(const StringName& p_property) const override;
    Variant _get_property_default_value(const StringName& p_property) const override;
    void _update_exports() override;
    int32_t _get_member_line(const StringName& p_member) const override;
    Dictionary _get_constants() const override;
    TypedArray<StringName> _get_members() const override;
    Variant _get_rpc_config() const override;
    String _get_class_icon_path() const override;
    //~ End ScriptExtension overrides

    /// Get the underlying script's language
    /// @return the script language instance
    ScriptLanguage* get_language() const { return _get_language(); }

    /// Check whether the script is marked as a tool script
    /// @return true if the script is marked as tool-mode, false otherwise
    bool get_tool() const override { return _is_tool(); }

    /// Set whether the script operates in tool-mode
    /// @param p_tool true sets the script to tool mode, false does not
    void set_tool(bool p_tool) override { _tool = p_tool; }
};

#endif  // ORCHESTRATOR_SCRIPT_H
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
class OScript : public ScriptExtension
{
    friend class OScriptInstance;
    GDCLASS(OScript, ScriptExtension);

    Ref<Orchestration> _orchestration;                     //! Orchestration model
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
    StringName _get_base_type() const;
    void _set_base_type(const StringName& p_base_type);
    TypedArray<OScriptNode> _get_nodes() const;
    void _set_nodes(const TypedArray<OScriptNode>& p_nodes);
    TypedArray<int> _get_connections() const;
    void _set_connections(const TypedArray<int>& p_connections);
    TypedArray<OScriptGraph> _get_graphs() const;
    void _set_graphs(const TypedArray<OScriptGraph>& p_graphs);
    TypedArray<OScriptFunction> _get_functions() const;
    void _set_functions(const TypedArray<OScriptFunction>& p_functions);
    TypedArray<OScriptVariable> _get_variables() const;
    void _set_variables(const TypedArray<OScriptVariable>& p_variables);
    TypedArray<OScriptSignal> _get_signals() const;
    void _set_signals(const TypedArray<OScriptSignal>& p_signals);
    //~ End Serialization API

    /// Updates the exported values
    /// @param r_values the exported variable values
    /// @param r_properties the exported variable property details
    void _update_export_values(HashMap<StringName, Variant>& r_values, List<PropertyInfo>& r_properties) const;

    /// Update export placeholders
    /// @param r_err the output error
    /// @param p_recursive whether called recursively
    /// @param p_instance the script instance, should never be null
    /// @param p_base_exports_changed whether base exports changed
    bool _update_exports_placeholder(bool* r_err = nullptr, bool p_recursive = false, OScriptPlaceHolderInstance* p_instance = nullptr, bool p_base_exports_changed = false) const;

    /// Updates the exports
    /// @param p_base_exports_changed whether the base class exports changed
    void _update_exports_down(bool p_base_exports_changed);

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
    #if GODOT_VERSION >= 0x040400
    StringName _get_doc_class_name() const override;
    #endif
    //~ End ScriptExtension overrides

    /// Get the underlying script's language
    /// @return the script language instance
    ScriptLanguage* get_language() const { return _get_language(); }

    Orchestration* get_orchestration();

    bool get_tool() const { return _is_tool(); }
    void set_tool(bool p_tool);

    bool is_edited() const;
    void set_edited(bool p_edited);

    // Taken from script.h/.cpp
    void reload_from_file();
};

#endif  // ORCHESTRATOR_SCRIPT_H
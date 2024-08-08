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
#ifndef ORCHESTRATOR_SCRIPT_INSTANCE_H
#define ORCHESTRATOR_SCRIPT_INSTANCE_H

#include "script/instances/instance_base.h"
#include "script/instances/node_instance.h"
#include "script/vm/script_vm.h"

using namespace godot;

/// Forward declarations
class OScriptNode;
class OScriptState;

/// The runtime instance of an OScript object.
///
/// When an Orchestration (OrchestratorScript) is loaded and prepares to run, the script creates
/// an instance of an OScriptInstance, which maintains the runtime state of the executing script
/// object.
///
/// This instance type represents the game instance, the one that does not run within the editor
/// but instead runs when the scene is running outside the editor's scope.
///
class OScriptInstance : public OScriptInstanceBase
{
    friend class OScript;
    friend class OScriptLanguage;
    friend class OScriptState;

    Ref<OScript> _script;                       //! The script this instance represents
    Object* _owner{ nullptr };                  //! The owning object of the script
    OScriptLanguage* _language{ nullptr };      //! The language the script represents
    OScriptVirtualMachine _vm;                  //! The virtual machine instance

public:
    /// Defines details about the script instance to be passed to Godot
    static const OScriptInstanceInfo INSTANCE_INFO;

    /// Create an OScriptInstance object
    /// @param p_script the orchestrator script this instance represents
    /// @param p_language the language object
    /// @param p_owner the owner of the script instance
    OScriptInstance(const Ref<OScript>& p_script, OScriptLanguage* p_language, Object* p_owner);

    /// OScriptInstance destructor
    ~OScriptInstance() override;

    //~ Begin OScriptInstanceBase Interface
    bool set(const StringName& p_name, const Variant& p_value, PropertyError* r_err) override;
    bool get(const StringName& p_name, Variant& p_value, PropertyError* r_err) override;
    GDExtensionPropertyInfo* get_property_list(uint32_t* r_count) override;
    Variant::Type get_property_type(const StringName& p_name, bool* r_is_valid) const override;
    bool has_method(const StringName& p_name) const override;
    Object* get_owner() const override;
    Ref<OScript> get_script() const override;
    ScriptLanguage* get_language() const override;
    bool is_placeholder() const override;
    //~ End OScriptInstanceBase Interface

    //~ Begin ScriptInstanceInfo2 Interface
    bool property_can_revert(const StringName& p_name);
    bool property_get_revert(const StringName& p_name, Variant* r_ret);
    void call(const StringName& p_method, const Variant* const* p_args, GDExtensionInt p_arg_count, Variant* r_return, GDExtensionCallError* r_err);
    void notification(int32_t p_what, bool p_reversed);
    void to_string(GDExtensionBool* r_is_valid, String* r_out);
    //~ End ScriptInstanceInfo2 Interface

    /// Get the base node/object type the script is based on
    /// @return the base type of the script
    String get_base_type() const;

    /// Set the base node/object type
    /// @param p_base_type the base type the script instance is based on
    void set_base_type(const String& p_base_type);

    /// Get a script defined variable
    /// @param p_name the variable name
    /// @param r_value the variable's value, only applicable if the method returns true
    /// @return true if the variable is found; false otherwise
    bool get_variable(const StringName& p_name, Variant& r_value) const;

    /// Set a script defined variable's value.
    /// @param p_name the variable to be set
    /// @param p_value the value to set for the variable
    /// @return true if the variable is set, false otherwise
    bool set_variable(const StringName& p_name, const Variant& p_value);

    /// Helper to lookup an OScriptInstance from a Godot Object reference
    /// @param p_object the godot object to find a script instance about
    /// @return the orchestrator script instance if found; null otherwise
    static OScriptInstance* from_object(GDExtensionObjectPtr p_object);
};

#endif  // ORCHESTRATOR_SCRIPT_INSTANCE_H
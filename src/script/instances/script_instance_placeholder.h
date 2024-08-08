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
#ifndef ORCHESTRATOR_SCRIPT_PLACEHOLDER_INSTANCE_H
#define ORCHESTRATOR_SCRIPT_PLACEHOLDER_INSTANCE_H

#include "script/instances/instance_base.h"

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/templates/hash_map.hpp>

using namespace godot;

/// The editor instance of an OScript object.
///
/// When an Orchestration (OrchestratorScript) is loaded and prepares to run, the script creates
/// an instance of an OScriptInstancePlaceHolderInstance, which maintains the state of the script
/// when running within the editor.
///
/// This instance type represents the idle, editor instance, the one that runs within the editor
/// but does not run when the game is running the scene outside of the editor.
///
class OScriptPlaceHolderInstance : public OScriptInstanceBase
{
    Ref<OScript> _script;                       //! The script this instance represents
    Object* _owner;                             //! The owning object of the script
    HashMap<StringName, Variant> _values;
    List<PropertyInfo> _properties;

public:
    /// Defines details about the script instance to be passed to Godot
    static const OScriptInstanceInfo INSTANCE_INFO;

    /// Creates an OScriptPlaceHolderInstance
    /// @param p_script the orchestrator script this instance represents
    /// @param p_owner the owner of the script instance
    OScriptPlaceHolderInstance(Ref<OScript> p_script, Object* p_owner);

    /// OScriptPlaceHolderInstance destructor
    ~OScriptPlaceHolderInstance() override;

    //~ Begin OScriptInstanceBase Interface
    bool set(const StringName& p_name, const Variant& p_value, PropertyError* r_err) override;
    bool get(const StringName& p_name, Variant& r_value, PropertyError* r_err) override;
    GDExtensionPropertyInfo* get_property_list(uint32_t* r_count) override;
    Variant::Type get_property_type(const StringName& p_name, bool* r_is_valid) const override;
    bool has_method(const StringName& p_name) const override;
    Ref<OScript> get_script() const override;
    Object* get_owner() const override;
    ScriptLanguage* get_language() const override;
    bool is_placeholder() const override;
    //~ End OScriptInstanceBase Interface

    //~ Begin ScriptInstanceInfo2 Interface
    bool property_can_revert(const StringName& p_name);
    bool property_get_revert(const StringName& p_name, Variant* r_ret);
    void call(const StringName& p_method, const Variant* const* p_args, GDExtensionInt p_arg_count, Variant* r_return,
              GDExtensionCallError* r_err);
    void notification(int32_t p_what, bool p_reversed);
    void to_string(GDExtensionBool* r_is_valid, String* r_out);
    //~ End ScriptInstanceInfo2 Interface

    void update(const List<PropertyInfo>& p_properties, const HashMap<StringName, Variant>& p_values);
};

#endif  // ORCHESTRATOR_SCRIPT_PLACEHOLDER_INSTANCE_H
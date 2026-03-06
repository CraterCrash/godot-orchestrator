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
#ifndef ORCHESTRATOR_SCRIPT_INSTANCE_NEW_H
#define ORCHESTRATOR_SCRIPT_INSTANCE_NEW_H

#include "common/version.h"
#include "script/compiler/compiled_function.h"

#include <godot_cpp/classes/script_language.hpp>
#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/templates/pair.hpp>
#include <godot_cpp/templates/self_list.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

typedef GDExtensionScriptInstanceInfo3 OScriptInstanceInfo;
#define GDEXTENSION_SCRIPT_INSTANCE_CREATE GDE_INTERFACE(script_instance_create3)

/// Abstract base class for all OScript-based script instance objects.
class OScriptInstanceBase {
    GDExtensionScriptInstancePtr _script_instance = nullptr;

    friend class OScript;
    friend class OScriptCompiledFunction;
    friend class OScriptCompiler;
    friend class OScriptLanguage;

protected:
    Ref<OScript> _script;
    ObjectID _owner_id;
    Object* _owner = nullptr;

    OScriptInstanceBase(const Ref<OScript>& p_script, Object* p_owner);

    #if GODOT_VERSION >= 0x040500
    bool _is_same_script_instance() const;
    #endif

    virtual LocalVector<PropertyInfo> _get_property_list() = 0;
    virtual LocalVector<MethodInfo> _get_method_list() const = 0;

public:
    virtual bool set(const StringName& p_name, const Variant& p_value) = 0;
    virtual bool get(const StringName& p_name, Variant& r_value) = 0;
    virtual void validate_property(PropertyInfo& p_property) const {}
    virtual bool property_can_revert(const StringName& p_name) const { return false; }
    virtual bool property_get_revert(const StringName& p_name, Variant& r_value) const { return false; }
    virtual void property_set_fallback(const StringName& p_name, const Variant& p_value, bool* r_valid);
    virtual Variant property_get_fallback(const StringName& p_name, bool* r_valid);
    virtual void get_property_state(GDExtensionScriptInstancePropertyStateAdd p_add_func, void* p_user_data);
    virtual void get_property_state(List<Pair<StringName, Variant>>& p_list);
    virtual GDExtensionPropertyInfo* get_property_list(uint32_t* r_size);
    virtual void free_property_list(const GDExtensionPropertyInfo* p_list, uint32_t p_size);
    virtual void free_property_list(const GDExtensionPropertyInfo* p_list);
    virtual Variant::Type get_property_type(const StringName& p_name, bool* r_is_valid) = 0;
    virtual GDExtensionMethodInfo* get_method_list(uint32_t* r_size) const;
    virtual bool has_method(const StringName& p_name) const = 0;
    virtual int get_method_argument_count(const StringName& p_name, bool* r_is_valid) const { return 0; }
    virtual void free_method_list(const GDExtensionMethodInfo* p_list, uint32_t p_size) const;
    virtual void free_method_list(const GDExtensionMethodInfo* p_list) const;
    virtual Ref<OScript> get_script() const;
    virtual Object* get_owner() const { return _owner; }
    virtual ScriptLanguage* get_language() const;
    virtual Variant get_rpc_config() const;
    virtual bool is_placeholder() const { return false; }
    virtual void notification(int p_notification, bool p_reversed = false) {}
    virtual Variant callp(const StringName& p_method, const Variant** p_args, int p_argcount, GDExtensionCallError& r_error) = 0;
    virtual String to_string();

    GDExtensionScriptInstancePtr get_instance_info() const { return _script_instance; }
    void set_instance_info(const GDExtensionScriptInstancePtr& p_info);

    virtual ~OScriptInstanceBase() = default;
};

/// Runtime-based OScript instance container, responsible for managing the specific instance state of a given
/// OScript instance that is being executed by Godot. These are designed to be lightweight with references
/// to the singular underlying OScript script object and its compiled state.
class OScriptInstance : public OScriptInstanceBase {

    friend class OScript;
    friend class OScriptCompiler;
    friend class OScriptCompiledFunction;

    Vector<Variant> _members;
    SelfList<OScriptFunctionState>::List _pending_func_states;
    #ifdef DEBUG_ENABLED
    HashMap<StringName, int> _member_indices_cache;
    #endif

    void _call_implicit_ready_recursively(const OScript* p_script);

protected:
    //~ Begin OScriptInstanceBase Interface
    LocalVector<PropertyInfo> _get_property_list() override;
    LocalVector<MethodInfo> _get_method_list() const override;
    //~ End OScriptInstanceBase Interface

public:
    static const OScriptInstanceInfo INSTANCE_INFO;

    //~ Begin OScriptInstanceBase Interface
    bool set(const StringName& p_name, const Variant& p_value) override;
    bool get(const StringName& p_name, Variant& r_value) override;
    void validate_property(PropertyInfo& p_property) const override;
    bool property_can_revert(const StringName& p_name) const override;
    bool property_get_revert(const StringName& p_name, Variant& r_value) const override;
    Variant::Type get_property_type(const StringName& p_name, bool* r_valid) override;
    bool has_method(const StringName& p_name) const override;
    int get_method_argument_count(const StringName& p_name, bool* r_valid) const override;
    void notification(int p_notification, bool p_reversed) override;
    Variant callp(const StringName& p_method, const Variant** p_args, int p_arg_count, GDExtensionCallError& r_error) override;
    //~ End OScriptInstanceBase Interface

    Variant debug_get_member_by_index(int p_index) { return _members[p_index]; }

    void reload_members();

    OScriptInstance(const Ref<OScript>& p_script, Object* p_owner);
    ~OScriptInstance() override;
};

/// Editor-based OScript instance container, responsible for managing the specific instance state of a given
/// OScript instance that is being referenced by a Node in the scene tree or a tool-based script. Just like
/// the runtime counterparts, these are designed to be lightweight with references to the singular underlying
/// OScript script object and its compiled state.
///
class OScriptPlaceHolderInstance : public OScriptInstanceBase {
    List<PropertyInfo> _properties;
    HashMap<StringName, Variant> _values;
    HashMap<StringName, Variant> _constants;

protected:
    //~ Begin OScriptInstanceBase Interface
    LocalVector<PropertyInfo> _get_property_list() override;
    LocalVector<MethodInfo> _get_method_list() const override;
    //~ End OScriptInstanceBase Interface

public:
    static const OScriptInstanceInfo INSTANCE_INFO;

    //~ Begin OScriptInstanceBase Interface
    bool set(const StringName& p_name, const Variant& p_value) override;
    bool get(const StringName& p_name, Variant& r_value) override;
    void property_set_fallback(const StringName& p_name, const Variant& p_value, bool* r_valid) override;
    Variant property_get_fallback(const StringName& p_name, bool* r_valid) override;
    Variant::Type get_property_type(const StringName& p_name, bool* r_valid) override;
    bool has_method(const StringName& p_name) const override;
    bool is_placeholder() const override { return true; }
    Variant callp(const StringName& p_method, const Variant** p_args, int p_arg_count, GDExtensionCallError& r_error) override;
    //~ End OScriptInstanceBase Interface

    void update(const List<PropertyInfo>& p_properties, const HashMap<StringName, Variant>& p_values);

    OScriptPlaceHolderInstance(const Ref<OScript>& p_script, Object* p_owner);
    ~OScriptPlaceHolderInstance() override;
};

#endif // ORCHESTRATOR_SCRIPT_INSTANCE_NEW_H
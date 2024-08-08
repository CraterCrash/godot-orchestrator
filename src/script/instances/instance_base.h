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
#ifndef ORCHESTRATOR_SCRIPT_INSTANCE_BASE_H
#define ORCHESTRATOR_SCRIPT_INSTANCE_BASE_H

#include "common/version.h"

#include <godot_cpp/classes/script_extension.hpp>
#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/templates/pair.hpp>

using namespace godot;

/// Forward declarations
class OScript;
class OScriptLanguage;

#if GODOT_VERSION >= 0x040300
typedef GDExtensionScriptInstanceInfo3 OScriptInstanceInfo;
#define GDEXTENSION_SCRIPT_INSTANCE_CREATE internal::gdextension_interface_script_instance_create3
#else
typedef GDExtensionScriptInstanceInfo2 OScriptInstanceInfo;
#define GDEXTENSION_SCRIPT_INSTANCE_CREATE internal::gdextension_interface_script_instance_create2
#endif

/// A base class implementation for various OrchestratorScript instance types.
///
/// In Godot, they use a paradigm with leveraging different "instance" class types depending on whether
/// the object is being used in the editor's scene tree or if it is executing as part of tooling or the
/// game's runtime. This class provides the base functionality regardless of the instance type.
///
class OScriptInstanceBase
{
    friend class OScript;
    friend class OScriptLanguage;

    void* _script_instance{ nullptr };          //! Godot script instance

public:
    OScriptInstanceBase();
    virtual ~OScriptInstanceBase();

    /// A set of error codes for property accessor methods
    enum PropertyError
    {
        PROP_OK,
        PROP_NOT_FOUND,
        PROP_WRONG_TYPE,
        PROP_READ_ONLY,
        PROP_WRITE_ONLY,
        PROP_GET_FAILED,
        PROP_SET_FAILED
    };

    /// Initializes a script instance.
    /// @param p_info the instance information structure to be initialized
    static void init_instance(OScriptInstanceInfo& p_info);

    /// Sets a given property with the specified value.
    /// @param p_name property name
    /// @param p_value value to set
    /// @param r_err the error code
    /// @return true if successful; false otherwise
    virtual bool set(const StringName& p_name, const Variant& p_value, PropertyError* r_err = nullptr) = 0;

    /// Gets a given property's value.
    /// @param p_name property name
    /// @param p_value property value read
    /// @param r_err the error code
    /// @return true if successful; false otherwise
    virtual bool get(const StringName& p_name, Variant& p_value, PropertyError* r_err = nullptr) = 0;

    /// Get the property state.
    /// @param p_add_func function details
    /// @param p_userdata user data
    void get_property_state(GDExtensionScriptInstancePropertyStateAdd p_add_func, void* p_userdata);

    /// Get the property state for all properties
    /// @param p_list the returned property state list
    void get_property_state(List<Pair<StringName, Variant>>& p_list);

    /// Retrieve the property information.
    /// @param r_count the number of properties
    /// @return property details
    virtual GDExtensionPropertyInfo* get_property_list(uint32_t* r_count) = 0;

    #if GODOT_VERSION >= 0x040300
    /// Releases the memory used by the property list.
    /// @param p_list property list details to be deallocated
    /// @param p_count the number of property infos to be freed
    void free_property_list(const GDExtensionPropertyInfo* p_list, uint32_t p_count) const;
    #else
    /// Releases the memory used by the property list.
    /// @param p_list property list details to be deallocated
    void free_property_list(const GDExtensionPropertyInfo* p_list) const;
    #endif

    /// Returns the property type of a given property.
    /// @param p_name the property name
    /// @param r_is_valid whether the property is valid
    /// @return the property type
    virtual Variant::Type get_property_type(const StringName& p_name, bool* r_is_valid) const = 0;

    /// Get all methods associated with the script.
    /// @param r_count the number of methods
    /// @return method details
    virtual GDExtensionMethodInfo* get_method_list(uint32_t* r_count) const;

    #if GODOT_VERSION >= 0x040300
    /// Releases the memory used by the method list.
    /// @param p_list method list details to be deallocated
    /// @param p_count the number of method info to be freed
    void free_method_list(const GDExtensionMethodInfo* p_list, uint32_t p_count) const;
    #else
    /// Releases the memory used by the method list.
    /// @param p_list method list details to be deallocated
    void free_method_list(const GDExtensionMethodInfo* p_list) const;
    #endif

    /// Return whether the specified method is available in the script.
    /// @param p_name the method name
    /// @return true if the method exists; otherwise false
    virtual bool has_method(const StringName& p_name) const = 0;

    /// Get the script owner
    /// @return the owning object
    virtual Object* get_owner() const = 0;

    /// Get the associated script this instance represents
    /// @return the script
    virtual Ref<OScript> get_script() const = 0;

    /// Get the script language
    /// @return script language
    virtual ScriptLanguage* get_language() const = 0;

    /// Return whether the script instance is a placeholder or not
    /// @return true if the instance is a placeholder, false otherwise
    virtual bool is_placeholder() const = 0;

protected:
    /// Copies a property from a PropertyInfo to a GDExtensionPropertyInfo
    /// @param p_property the property info
    /// @param p_dst the target GDExtension object
    void copy_property(const PropertyInfo& p_property, GDExtensionPropertyInfo& p_dst) const;

    /// Get the variable name from a property path, which may include category/groups
    /// @param p_property_path the property path, i.e. "group/property_name"
    /// @return the variable name, i.e. "property_name"
    StringName _get_variable_name_from_path(const StringName& p_property_path) const;
};

#endif  // ORCHESTRATOR_SCRIPT_INSTANCE_BASE_H
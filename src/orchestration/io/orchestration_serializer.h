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
#ifndef ORCHESTRATOR_ORCHESTRATION_SERIALIZER_H
#define ORCHESTRATOR_ORCHESTRATION_SERIALIZER_H

#include "orchestration/orchestration.h"

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/rb_map.hpp>

using namespace godot;

class OrchestrationSerializer
{
protected:
    struct NonPersistentKey
    {
        Ref<Resource> base;
        StringName property;

        bool operator<(const NonPersistentKey& p_key) const
        {
            return base == p_key.base ? property < p_key.property : base < p_key.base;
        }
    };

    Error _error{ OK };
    String _error_text;
    String _path;
    String _local_path;

    bool _bundle_resources{ false };
    bool _skip_editor{ false };
    bool _take_over_paths{ false };
    bool _relative_paths{ false };

    HashSet<Ref<Resource>> _resource_set;
    HashMap<String, HashMap<String, Variant>> _default_value_cache;
    RBMap<NonPersistentKey, Variant> _non_persistent_map;
    List<Ref<Resource>> _saved_resources;

    bool _is_cached(const String& p_path) const;
    Ref<Resource> _get_cached_resource(const String& p_path);
    String _get_resource_class(const Ref<Resource>& p_resource) const;

    void _set_resource_edited(const Ref<Resource>& p_resource, bool p_edited);

    bool _is_creating_missing_resources_if_class_unavailable_enabled();
    bool _is_parse_error(const String& p_reason);

    Error _set_error(const String& p_reason = String());
    Error _set_error(Error p_error, const String& p_reason = String());

    bool _is_built_in_resource(const Ref<Resource>& p_resource);

    int64_t _get_resource_id_for_path(const String& p_path, bool p_generate = false);
    void _warn_invalid_external_resource_uid(uint32_t p_index, const String& p_path, uint64_t p_uid);

    Variant _get_class_property_default(const StringName& p_class, const StringName& p_property);

    virtual bool _is_resource_gatherable(const Ref<Resource>& p_resource, bool p_main) = 0;
    virtual void _gather_resources(const Variant& p_value, bool p_main);
    virtual void _gather_object_resources(const Ref<Resource>& p_value, bool p_main);
    virtual void _gather_array_resources(const Array& p_value, bool p_main);
    virtual void _gather_dictionary_resources(const Dictionary& p_value, bool p_main);
    virtual void _gather_node_path(const NodePath& p_path, bool p_main) { }

public:
    /// Serializes the orchestration
    /// @param p_orchestration the orchestration
    /// @param p_path the orchestration resource path
    /// @param p_flags the serialization flags
    /// @return the serialized output
    virtual Variant serialize(const Ref<Orchestration>& p_orchestration, const String& p_path, uint32_t p_flags) = 0;

    /// Get the parse error, if any exist
    /// @return the parse error code
    virtual Error get_error() const { return _error; }

    /// Get the parse error text/message, if any exist.
    /// @return the parse error text/message
    virtual String get_error_text() const { return _error_text; }

    virtual ~OrchestrationSerializer() = default;
};

#endif // ORCHESTRATOR_ORCHESTRATION_SERIALIZER_H
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

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/rb_map.hpp>

using namespace godot;

/// Defines the common contract for all Orchestration resource serializers.
class OrchestrationSerializer {

protected:

    struct NonPersistentKey {
        Ref<Resource> base;
        StringName property;
        bool operator<(const NonPersistentKey& p_right) const {
            return base == p_right.base ? property < p_right.property : base < p_right.base;
        }
    };

    RBMap<NonPersistentKey, Variant> _non_persistent_map;
    HashSet<Ref<Resource>> _resource_set;
    List<Ref<Resource>> _saved_resources;

    bool _relative_paths = false;
    bool _skip_editor = false;
    bool _bundle_resources = false;
    bool _take_over_paths = false;
    String _path;

    virtual void _decode_and_set_flags(const String& p_path, uint32_t p_flags);

    bool _is_resource_built_in(const Ref<Resource>& p_resource);
    static int64_t _get_resource_id_for_path(const String& p_path, bool p_generate = false);
    void _set_resource_edited(const Ref<Resource>& p_resource, bool p_edited = true);

    static String _resource_get_class(const Ref<Resource>& p_resource);
    static String _generate_scene_unique_id();
    static String _create_resource_uid(const Ref<Resource>& p_resource, const HashSet<String>& p_used_ids, bool& r_generated);

    virtual void _find_resources(const Variant& p_variant, bool p_main);
    virtual void _find_resources_array(const Array& p_array, bool p_main);
    virtual void _find_resources_dictionary(const Dictionary& p_dictionary, bool p_main);
    virtual void _find_resources_node_path(const NodePath& p_node_path, bool p_main) { }
    virtual void _find_resources_object(const Variant& p_variant, bool p_main) = 0;
    virtual void _find_resources_resource(const Ref<Resource>& p_resource, bool p_main) = 0;
    virtual void _find_resources_resource_properties(const Ref<Resource>& p_resource, bool p_main);

public:
    virtual PackedStringArray get_recognized_extensions(const Ref<Resource>& p_resource) = 0;
    virtual bool recognize(const Ref<Resource>& p_resource) = 0;
    virtual Error set_uid(const String& p_path, int64_t p_uid) = 0;
    virtual bool recognize_path(const Ref<Resource>& p_resource, const String& p_path) = 0;
    virtual Error save(const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags) = 0;

    virtual ~OrchestrationSerializer() = default;
};

#endif // ORCHESTRATOR_ORCHESTRATION_SERIALIZER_H
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
#ifndef ORCHESTRATOR_ORCHESTRATION_TEXT_SERIALIZER_H
#define ORCHESTRATOR_ORCHESTRATION_TEXT_SERIALIZER_H

#include "orchestration/io/orchestration_serializer.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/resource_format_loader.hpp>

using namespace godot;

class OrchestrationTextSerializer : public OrchestrationSerializer
{
    struct ExternalResource
    {
        String path;
        String type;
        Ref<Resource> resource;
    };

    struct ResourceSort
    {
        Ref<Resource> resource;
        String id;

        bool operator<(const ResourceSort& p_right) const { return id.naturalnocasecmp_to(p_right.id) < 0; }
    };

    HashMap<String, ExternalResource> _external_resources;
    HashMap<Ref<Resource>, String> _external_resource_ids;
    HashMap<Ref<Resource>, String> _internal_resource_ids;

    String _generate_scene_unique_id();

    Error _write_orchestration_tag(const Ref<Resource>& p_resource, String& r_value);
    Error _write_external_resource_tags(String& r_value);
    Error _write_objects(const Ref<Resource>& p_orchestration, const String& p_path, String& r_value);
    Error _write_resource(const Ref<Resource>& p_orchestration, String& r_value);
    Error _write_properties(const Ref<Resource>& p_orchestration, const Ref<Resource>& p_resource, String& r_value);
    Error _write_property(const Variant& p_value, String& r_value, int p_recursion_count = 0);

    String _write_encoded_resource(const Ref<Resource>& p_resource);

protected:
    //~ Begin OrchestrationSerializer Interface
    bool _is_resource_gatherable(const Ref<Resource>& p_resource, bool p_main) override;
    //~ End OrchestrationSerializer Interface

public:
    //~ Begin OrchestrationSerializer Interface
    Variant serialize(const Ref<Orchestration>& p_orchestration, const String& p_path, uint32_t p_flags) override;
    //~ End OrchestrationSerializer Interface

    /// Get the serialized start tag
    /// @param p_type the resource type
    /// @param p_script_class the script class
    /// @param p_resources the number of resources
    /// @param p_version the resource format
    /// @param p_uid the unique resource id
    /// @return the tag
    String get_start_tag(const String& p_type, const String& p_script_class, uint64_t p_resources, uint64_t p_version, int64_t p_uid);

    /// Get the serialized external resource tag
    /// @param p_type the resource type
    /// @param p_path the resource path
    /// @param p_uid the unique resource id
    /// @param p_with_newline whether to include newline
    /// @return the tag
    String get_ext_resource_tag(const String& p_type, const String& p_path, const String& p_uid, bool p_with_newline);
};

#endif // ORCHESTRATOR_ORCHESTRATION_TEXT_SERIALIZER_H
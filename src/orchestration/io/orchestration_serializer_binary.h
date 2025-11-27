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
#ifndef ORCHESTRATOR_ORCHESTRATION_BINARY_SERIALIZER_H
#define ORCHESTRATOR_ORCHESTRATION_BINARY_SERIALIZER_H

#include "orchestration/io/orchestration_serializer.h"
#include "orchestration/io/orchestration_stream.h"

#include <godot_cpp/classes/resource_format_loader.hpp>
#include <godot_cpp/classes/resource_uid.hpp>

class OrchestrationBinarySerializer : public OrchestrationSerializer
{
    struct Property
    {
        int name_index;
        Variant value;
        PropertyInfo info;
    };

    struct ResourceInfo
    {
        String type;
        List<Property> properties;
    };

    HashMap<Ref<Resource>, int> _ext_resources;             //! Stores gather traversal order for external resources.
    Vector<StringName> _string_map;

    int _get_string_index(const String& p_value);

    // p_resource_map appears to be internal
    // p_ext_resources appears to be external
    void _write_variant(OrchestrationByteStream& p_stream, const Variant& p_value, HashMap<Ref<Resource>, int>& p_resource_map, HashMap<Ref<Resource>, int>& p_ext_resources);

protected:
    //~ Begin OrchestrationSerializer Interface
    bool _is_resource_gatherable(const Ref<Resource>& p_resource, bool p_main) override;
    void _gather_node_path(const NodePath& p_path, bool p_main) override;
    //~ End OrchestrationSerializer Interface

public:
    //~ Begin OrchestrationSerializer Interface
    Variant serialize(const Ref<Orchestration>& p_orchestration, const String& p_path, uint32_t p_flags) override;
    //~ End OrchestrationSerializer Interface
};

#endif // ORCHESTRATOR_ORCHESTRATION_BINARY_SERIALIZER_H
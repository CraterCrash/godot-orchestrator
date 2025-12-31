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
#ifndef ORCHESTRATOR_ORCHESTRATION_SERIALIZER_BINARY_H
#define ORCHESTRATOR_ORCHESTRATION_SERIALIZER_BINARY_H

#include "orchestration/serialization/serializer.h"

#include <godot_cpp/classes/file_access.hpp>

// Serializer that saves binary-based <code>Orchestration/code> resources.
class OrchestrationBinarySerializer : public OrchestrationSerializer {

    struct Property {
        uint32_t index;
        PropertyInfo info;
        Variant value;
    };

    struct ResourceInfo {
        String type;
        List<Property> properties;
    };

    Ref<FileAccess> _file;
    HashMap<Ref<Resource>, uint32_t> _external_resources;
    HashMap<StringName, uint32_t> _string_map;
    Vector<StringName> _strings;

    bool _big_endian = false;

    uint32_t _get_string_index(const String& p_value);

    void _save_unicode_string(const String& p_value, bool p_bit_on_length = false);
    void _write_variant(const Variant& p_value, HashMap<Ref<Resource>, uint32_t>& p_resource_map, const PropertyInfo& p_hint = PropertyInfo());

protected:
    //~ Begin OrchestrationSerializer Interface
    void _decode_and_set_flags(const String& p_path, uint32_t p_flags) override;
    void _find_resources_node_path(const NodePath& p_node_path, bool p_main) override;
    void _find_resources_object(const Variant& p_variant, bool p_main) override;
    void _find_resources_resource(const Ref<Resource>& p_resource, bool p_main) override;
    //~ End OrchestrationSerializer Interface

public:
    //~ Begin OrchestrationSerializer Interface
    PackedStringArray get_recognized_extensions(const Ref<Resource>& p_resource) override;
    bool recognize(const Ref<Resource>& p_resource) override;
    Error set_uid(const String& p_path, int64_t p_uid) override;
    bool recognize_path(const Ref<Resource>& p_resource, const String& p_path) override;
    Error save(const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags) override;
    //~ End OrchestrationSerializer Interface
};

#endif // ORCHESTRATOR_ORCHESTRATION_SERIALIZER_BINARY_H
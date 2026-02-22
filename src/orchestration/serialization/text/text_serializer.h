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
#ifndef ORCHESTRATOR_ORCHESTRATION_SERIALIZER_TEXT_H
#define ORCHESTRATOR_ORCHESTRATION_SERIALIZER_TEXT_H

#include "orchestration/serialization/serializer.h"

// Forward declarations
class OrchestrationTextParser;

/// Serializer that saves text-based <code>Orchestration</code> resources.
class OrchestrationTextSerializer : public OrchestrationSerializer {

    friend class OrchestrationTextParser;

    struct ResourceSort {
        Ref<Resource> resource;
        String id;
        bool operator<(const ResourceSort& p_right) const {
            return id.naturalnocasecmp_to(p_right.id) < 0;
        }
    };

    HashMap<Ref<Resource>, String> _external_resources;
    HashMap<Ref<Resource>, String> _internal_resources;

    static String _write_resources(void* p_userdata, const Ref<Resource>& p_resource);
    String _write_resource(const Ref<Resource>& p_resource);
    String _write_resource_ref(const Ref<Resource>& p_resource);
    String _write_external_resource_ref(const Ref<Resource>& p_resource);
    String _write_internal_resource_ref(const Ref<Resource>& p_resource);

    String _create_start_tag(const String& p_class, const String& p_script_class, const String& p_icon_path, uint32_t p_steps, uint32_t p_version, int64_t p_uid);
    String _create_ext_resource_tag(const String& p_type, const String& p_path, const String& p_id, bool p_newline = true);
    String _create_obj_tag(const Ref<Resource>& p_resource, const String& p_uid);
    String _create_resource_tag();

protected:
    //~ Begin OrchestrationSerializer Interface
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

#endif // ORCHESTRATOR_ORCHESTRATION_SERIALIZER_TEXT_H
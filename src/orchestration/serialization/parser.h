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
#ifndef ORCHESTRATOR_ORCHESTRATION_PARSER_H
#define ORCHESTRATOR_ORCHESTRATION_PARSER_H

#include <godot_cpp/classes/missing_resource.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

/// Defines the common contract for all Orchestration resource parsers.
class OrchestrationParser {
    struct ParseError {
        int position;
        String message;
    };

protected:
    static void _set_resource_property(Ref<Resource>& r_resource, const MissingResource* p_missing_resource, const StringName& p_name, const Variant& p_value, Dictionary& r_missing_properties);

    bool _is_creating_missing_resources_if_class_unavailable_enabled() const;
    Variant _instantiate_resource(const String& p_resource_type);
    void _set_resource_edited(const Ref<Resource>& p_resource, bool p_edited);

public:
    virtual String get_resource_script_class(const String& p_path) = 0;
    virtual int64_t get_resource_uid(const String& p_path) = 0;
    virtual PackedStringArray get_dependencies(const String& p_path, bool p_add_types) = 0;
    virtual Error rename_dependencies(const String& p_path, const Dictionary& p_renames) = 0;
    virtual PackedStringArray get_classes_used(const String& p_path) = 0;
    virtual Variant load(const String& p_path) = 0;

    virtual ~OrchestrationParser() = default;
};

#endif // ORCHESTRATOR_ORCHESTRATION_PARSER_H
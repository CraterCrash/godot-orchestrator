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

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/resource_format_loader.hpp>

using namespace godot;

/// Forward declaration
class Orchestration;

/// Responsible for parsing an orchestration's source.
class OrchestrationParser
{
protected:
    Error _error{ OK };
    String _error_text;
    String _path;
    String _local_path;

    bool _is_cached(const String& p_path) const;
    Ref<Resource> _get_cached_resource(const String& p_path);

    void _set_resource_edited(const Ref<Resource>& p_resource, bool p_edited);

    bool _is_creating_missing_resources_if_class_unavailable_enabled();
    bool _is_parse_error(const String& p_reason);

    Error _set_error(const String& p_reason = String());
    Error _set_error(Error p_error, const String& p_reason = String());

    int64_t _get_resource_id_for_path(const String& p_path, bool p_generate = false);
    void _warn_invalid_external_resource_uid(uint32_t p_index, const String& p_path, uint64_t p_uid);

public:
    /// Parse an <code>Orchestration</code> source into an Orchestration resource.
    /// @param p_source the source, should match the intended parser type.
    /// @param p_path the orchestration resource file path
    /// @param p_cache_mode how caching should be handled while parsing
    /// @return the parsed Orchestration resource, or an invalid resource if parsing failed.
    virtual Ref<Orchestration> parse(const Variant& p_source, const String& p_path, ResourceFormatLoader::CacheMode p_cache_mode) = 0;

    /// Parse the resource unique ID
    /// @param p_path the resource path
    /// @return the resource unique id or <code>ResourceUID::INVALID_ID</code> if parse failed
    virtual int64_t get_uid(const String& p_path) = 0;

    /// Parse the resource script class, if any exists.
    /// @param p_path the resource path
    /// @return the resource script class or an empty string if none exist.
    virtual String get_script_class(const String& p_path) = 0;

    /// Parse all internal classes used by this resource
    /// @param p_path the resource path
    /// @return array of all classes used
    virtual PackedStringArray get_classes_used(const String& p_path) = 0;

    /// Get a list of all external resource dependencies used by this resource
    /// @param p_path the resource path
    /// @param p_add_types whether to append "::<type>" to the dependency resource path
    /// @return array of all dependencies used by this resource
    virtual PackedStringArray get_dependencies(const String& p_path, bool p_add_types) = 0;

    /// Rename all dependencies based on the provided renames dictionary
    /// @param p_path the resource path to modify
    /// @param p_renames the resources to be renamed
    /// @return error code on whether the rename succeeded or failed
    virtual Error rename_dependencies(const String& p_path, const Dictionary& p_renames) = 0;

    /// Get the parse error, if any exist
    /// @return the parse error code
    virtual Error get_error() const { return _error; }

    /// Get the parse error text/message, if any exist.
    /// @return the parse error text/message
    virtual String get_error_text() const { return _error_text; }

    virtual ~OrchestrationParser() = default;
};

#endif // ORCHESTRATOR_ORCHESTRATION_PARSER_H
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
#ifndef ORCHESTRATOR_ORCHESTRATION_PARSER_TEXT_H
#define ORCHESTRATOR_ORCHESTRATION_PARSER_TEXT_H

#include "orchestration_parser.h"
#include "orchestration_stream.h"

#include <godot_cpp/classes/resource_uid.hpp>
#include <godot_cpp/templates/hash_map.hpp>

using namespace godot;

/// Responsible for parsing an orchestration's text-based source.
class OrchestrationTextParser : public OrchestrationParser
{
    enum TokenType
    {
        TK_CURLY_BRACKET_OPEN,
        TK_CURLY_BRACKET_CLOSE,
        TK_BRACKET_OPEN,
        TK_BRACKET_CLOSE,
        TK_PARENTHESIS_OPEN,
        TK_PARENTHESIS_CLOSE,
        TK_IDENTIFIER,
        TK_STRING,
        TK_STRING_NAME,
        TK_NUMBER,
        TK_COLOR,
        TK_COLON,
        TK_COMMA,
        TK_PERIOD,
        TK_EQUAL,
        TK_EOF,
        TK_ERROR,
        TK_MAX
    };

    static const char* tk_name[TK_MAX];

    struct Token
    {
        TokenType type;
        Variant value;
    };

    struct Tag
    {
        String name;
        HashMap<String, Variant> fields;
    };

    struct ExternalResource
    {
        String path;
        String type;
        Ref<Resource> resource;
    };

    HashMap<String, ExternalResource> _external_resources;
    HashMap<String, Ref<Resource>> _internal_resources;
    HashMap<String, String> _remaps;
    String _res_path;
    String _res_type;
    String _script_class;
    int64_t _line{ 1 };
    int64_t _total_resources{ 0 };
    int64_t _parsed_external_resources{ 0 };
    int64_t _parsed_internal_resources{ 0 };
    uint32_t _version{ 0 };
    int64_t _res_uid = ResourceUID::INVALID_ID;
    ResourceFormatLoader::CacheMode _cache_mode = ResourceFormatLoader::CACHE_MODE_REUSE;
    bool _ignore_external_resources{ false };
    Tag _tag;

    template<typename T>
    Error _parse_construct(OrchestrationStringStream& p_stream, Vector<T>& r_construct);

    Error _get_token(OrchestrationStringStream& p_stream, Token& r_token);
    Error _get_color_token(OrchestrationStringStream& p_stream, Token& r_token);
    Error _get_string_name_token(OrchestrationStringStream& p_stream, Token& r_token);
    Error _get_string_token(OrchestrationStringStream& p_stream, Token& r_token);
    Error _get_number_token(OrchestrationStringStream& p_stream, Token& r_token);
    Error _get_identifier_token(OrchestrationStringStream& p_stream, Token& r_token);

    Error _parse_tag(OrchestrationStringStream& p_stream, bool p_simple = false);
    Error _parse_tag(OrchestrationStringStream& p_stream, Token& p_token, bool p_simple = false);
    Error _parse_tag_assign_eof(OrchestrationStringStream& p_stream, String& r_name, Variant& r_value, bool p_simple = false);

    Error _parse_value(OrchestrationStringStream& p_stream, Token& p_token, Variant& r_value);
    Error _parse_dictionary(OrchestrationStringStream& p_stream, Dictionary& r_value);
    Error _parse_array(OrchestrationStringStream& p_stream, Array& r_value);
    Error _parse_identifier(OrchestrationStringStream& p_stream, Token& p_token, Variant& r_value);
    Error _parse_resource(OrchestrationStringStream& p_stream, Ref<Resource>& r_value);
    Error _parse_extresource(OrchestrationStringStream& p_stream, Ref<Resource>& r_value);
    Error _parse_subresource(OrchestrationStringStream& p_stream, Ref<Resource>& r_value);

    Error _parse_header(OrchestrationStringStream& p_stream, bool p_skip_first_tag = false);
    Error _parse_ext_resources(OrchestrationStringStream& p_stream);
    Error _parse_objects(OrchestrationStringStream& p_stream);
    Error _parse_resource(OrchestrationStringStream& p_stream, Ref<Orchestration>& r_value);
    Error _parse(const String& p_path, ResourceFormatLoader::CacheMode p_cache_mode, bool p_parse_resources = true);

public:
    //~ Begin OrchestrationParser Interface
    Ref<Orchestration> parse(const Variant& p_source, const String& p_path, ResourceFormatLoader::CacheMode p_cache_mode) override;
    int64_t get_uid(const String& p_path) override;
    String get_script_class(const String& p_path) override;
    PackedStringArray get_classes_used(const String& p_path) override;
    PackedStringArray get_dependencies(const String& p_path, bool p_add_types) override;
    Error rename_dependencies(const String& p_path, const Dictionary& p_renames) override;
    //~ End OrchestrationParser Interface

    /// Get the line number where the parse failed, if it failed.
    /// @return the line number where parsing failed
    int64_t get_error_line() const { return _line; }

    ~OrchestrationTextParser() override = default;
};

#endif // ORCHESTRATOR_ORCHESTRATION_PARSER_TEXT_H
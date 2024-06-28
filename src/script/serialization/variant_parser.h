// This file is part of the Godot Orchestrator project.
//
// Copyright (c) 2023-present Vahera Studios LLC and its contributors.
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
#ifndef ORCHESTRATOR_SCRIPT_VARIANT_PARSER_H
#define ORCHESTRATOR_SCRIPT_VARIANT_PARSER_H

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

/// Based specifically on Godot's internal "core/variant/variant_parser.h/cpp" implementation.
class OScriptVariantParser
{
public:
    struct Stream
    {
    private:
        enum { READAHEAD_SIZE = 2048 };
        char32_t _readahead_buffer[READAHEAD_SIZE];
        uint32_t _readahead_pointer{ 0 };
        uint32_t _readahead_filled{ 0 };
        bool _eof{ false };

    protected:
        bool _readahead_enabled{ true };

        virtual uint32_t _read_buffer(char32_t* p_buffer, uint32_t p_num_chars) = 0;
        virtual bool _is_eof() const = 0;

    public:
        char32_t saved{ 0 };

        char32_t get_char();
        bool is_eof() const;
        virtual bool is_utf8() const = 0;

        Stream() = default;
        virtual ~Stream() = default;
    };

    struct StreamFile : public Stream
    {
    protected:
        //~ Begin Stream Interface
        uint32_t _read_buffer(char32_t* p_buffer, uint32_t p_num_chars) override;
        bool _is_eof() const override;
        //~ End Stream Interface

    public:
        Ref<FileAccess> data;

        //~ Begin Stream Interface
        bool is_utf8() const override;
        //~ End Stream Interface

        StreamFile(bool p_readahead_enabled = true) { _readahead_enabled = p_readahead_enabled; }
    };

    struct StreamString : public Stream
    {
        String _data;
        int _pos{ 0 };

    protected:
        //~ Begin Stream Interface
        uint32_t _read_buffer(char32_t* p_buffer, uint32_t p_num_chars) override;
        bool _is_eof() const override;
        //~ End Stream Interface

    public:
        //~ Begin Stream Interface
        bool is_utf8() const override;
        //~ End Stream Interface

        StreamString(bool p_readhead_enabled = true) { _readahead_enabled = p_readhead_enabled; }
    };

    typedef Error (*ParseResourceFunction)(void* p_self, Stream* p_stream, Ref<Resource>& r_res, int& p_line, String& r_err_str);

    struct ResourceParser
    {
        void* userdata{ nullptr };
        ParseResourceFunction func{ nullptr };
        ParseResourceFunction external_func{ nullptr };
        ParseResourceFunction subres_func{ nullptr };
    };

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

    enum Expecting
    {
        EXPECT_OBJECT,
        EXPECT_OBJECT_KEY,
        EXPECT_COLON,
        EXPECT_OBJECT_VALUE
    };

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

private:
    static const char* tk_name[TK_MAX];

    template<typename T>
    static Error _parse_construct(Stream* p_stream, Vector<T>& r_construct, int& p_line, String& r_err_string);
    static Error _parse_enginecfg(Stream* p_stream, Vector<String>& p_strings, int& p_line, String& r_err_string);
    static Error _parse_dictionary(Stream* p_stream, Dictionary& p_object, int& p_line, String& r_err_string, ResourceParser* p_res_parser = nullptr);
    static Error _parse_array(Stream* p_stream, Array& p_array, int& p_line, String& r_err_string, ResourceParser* p_res_parser = nullptr);
    static Error _parse_tag(Stream* p_stream, Token& p_token, int& p_line, String& r_err_string, Tag& r_tag, ResourceParser* p_res_parser = nullptr, bool p_simple_tag = false);

public:
    static Error parse_tag(Stream* p_stream, int& p_line, Tag& r_tag, String& r_err_string, ResourceParser* p_res_parser = nullptr, bool p_simple_tag = false);
    static Error parse_tag_assign_eof(Stream* p_stream, int& p_line, String& r_err_string, Tag& r_tag, String& r_assign, Variant& r_value, ResourceParser* p_res_parser = nullptr, bool p_simple_tag = false);
    static Error parse_value(Stream* p_stream, Token& p_token, int& p_line, Variant& r_value, String& r_err_string, ResourceParser* p_res_parser = nullptr);
    static Error parse(Stream* p_stream, Variant& r_ret, String& r_err_string, int& r_err_line, ResourceParser* p_res_parser = nullptr);
    static Error get_token(Stream* p_stream, int& r_line, Token& r_token, String& r_err_string);
};

/// Based specifically on Godot's internal "core/variant/variant_parser.h/cpp" implementation.
class OScriptVariantWriter
{
    static bool _is_resource_file(const String& p_path);

public:
    typedef Error (*StoreStringFunction)(void* p_userdata, const String& p_string);
    typedef String (*EncodeResourceFunction)(void* p_userdata, const Ref<Resource>& p_resource);

    static Error write(const Variant& p_variant, StoreStringFunction p_store_string, void* p_store_userdata, EncodeResourceFunction p_encode_resource, void* p_encode_userdata,int p_recursion_count = 0);
    static Error write_to_string(const Variant& p_variant, String& r_string, EncodeResourceFunction p_encode_resource = nullptr, void* p_encode_userdata = nullptr);
};

#endif // ORCHESTRATOR_SCRIPT_VARIANT_PARSER_H
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
#include "orchestration/io/orchestration_parser_text.h"

#include "common/string_utils.h"
#include "orchestration/orchestration.h"
#include "orchestration_format.h"
#include "orchestration_serializer_text.h"
#include "script/serialization/resource_cache.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/missing_resource.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

const char* OrchestrationTextParser::tk_name[TK_MAX] = {
    "'{'",
    "'}'",
    "'['",
    "']'",
    "'('",
    "')'",
    "identifier",
    "string",
    "string_name",
    "number",
    "color",
    "':'",
    "','",
    "'.'",
    "'='",
    "EOF",
    "ERROR"
};

#define READING_SIGN 0
#define READING_INT 1
#define READING_DEC 2
#define READING_EXP 3
#define READING_DONE 4

static double stor_fix(const String& p_str)
{
    if (p_str == "inf")
        return INFINITY;

    if (p_str == "inf_neg")
        return -INFINITY;

    if (p_str == "nan")
        return NAN;

    return -1;
}

template <typename T>
Error OrchestrationTextParser::_parse_construct(OrchestrationStringStream& p_stream, Vector<T>& r_construct)
{
    Token token;
    _get_token(p_stream, token);

    if (token.type != TK_PARENTHESIS_OPEN)
        return _set_error("Expected '(' in constructor");

    bool first = true;
    while (true)
    {
        if (!first)
        {
            _get_token(p_stream, token);
            if (token.type != TK_COMMA)
            {
                if (token.type == TK_PARENTHESIS_CLOSE)
                    break;

                return _set_error("Expected ',' or ')' in constructor");
            }
        }

        _get_token(p_stream, token);
        if (first && token.type == TK_PARENTHESIS_CLOSE)
            break;

        if (token.type != TK_NUMBER)
        {
            bool valid = false;
            if (token.type == TK_IDENTIFIER)
            {
                double real = stor_fix(token.value);
                if (real != -1)
                {
                    token.type = TK_NUMBER;
                    token.value = real;
                    valid = true;
                }
            }

            if (!valid)
                return _set_error("Expected float in constructor");

        }

        r_construct.push_back(token.value);
        first = false;
    }

    return OK;
}

Error OrchestrationTextParser::_get_token(OrchestrationStringStream& p_stream, Token& r_token)
{
    while (true)
    {
        char32_t ch = p_stream.read_char();
        if (p_stream.is_eof())
        {
            r_token.type = TK_EOF;
            return OK;
        }

        switch (ch)
        {
            case '\n':
            {
                _line++;
                break;
            }
            case 0:
            {
                r_token.type = TK_EOF;
                return OK;
            }
            case '{':
            {
                r_token.type = TK_CURLY_BRACKET_OPEN;
                return OK;
            }
            case '}':
            {
                r_token.type = TK_CURLY_BRACKET_CLOSE;
                return OK;
            }
            case '[':
            {
                r_token.type = TK_BRACKET_OPEN;
                return OK;
            }
            case ']':
            {
                r_token.type = TK_BRACKET_CLOSE;
                return OK;
            }
            case '(':
            {
                r_token.type = TK_PARENTHESIS_OPEN;
                return OK;
            }
            case ')':
            {
                r_token.type = TK_PARENTHESIS_CLOSE;
                return OK;
            }
            case ':':
            {
                r_token.type = TK_COLON;
                return OK;
            }
            case ';':
            {
                while (true)
                {
                    char32_t c = p_stream.read_char();
                    if (p_stream.is_eof())
                    {
                        r_token.type = TK_EOF;
                        return OK;
                    }

                    if (c == '\n')
                    {
                        _line++;
                        break;
                    }
                }
                break;
            }
            case ',':
            {
                r_token.type = TK_COMMA;
                return OK;
            }
            case '.':
            {
                r_token.type = TK_PERIOD;
                return OK;
            }
            case '=':
            {
                r_token.type = TK_EQUAL;
                return OK;
            }
            case '#':
            {
                return _get_color_token(p_stream, r_token);
            }
            case '&':
            {
                return _get_string_name_token(p_stream, r_token);
            }
            case '"':
            {
                return _get_string_token(p_stream, r_token);
            }
            default:
            {
                if (ch <= 32)
                    break;

                if (ch == '-' || (ch >= '0' && ch <= '9'))
                {
                    p_stream.rewind();
                    return _get_number_token(p_stream, r_token);
                }

                #if GODOT_VERSION >= 0x040500
                if (is_ascii_alphanumeric_char(ch) || is_underscore(ch))
                #else
                if (is_ascii_char(ch) || is_underscore(ch))
                #endif
                {
                    p_stream.rewind();
                    return _get_identifier_token(p_stream, r_token);
                }

                r_token.type = TK_ERROR;
                return _set_error(vformat("Unexpected character: %s", ch));
            }
        }
    }
}

Error OrchestrationTextParser::_get_color_token(OrchestrationStringStream& p_stream, Token& r_token) // NOLINT
{
    String color;
    color += '#';

    while (true)
    {
        char32_t ch = p_stream.read_char();
        if (p_stream.is_eof())
        {
            r_token.type = TK_EOF;
            return OK;
        }

        if (!is_hex_digit(ch))
        {
            p_stream.rewind();
            break;
        }

        color += ch;
    }

    r_token.value = Color::html(color);
    r_token.type = TK_COLOR;
    return OK;
}

Error OrchestrationTextParser::_get_string_name_token(OrchestrationStringStream& p_stream, Token& r_token)
{
    uint32_t ch =  p_stream.read_char();
    if (ch != '"')
    {
        r_token.type = TK_ERROR;
        return _set_error("Expected '\"' after '&'");
    }

    Token string_token;
    if (_get_string_token(p_stream, string_token) != OK)
    {
        r_token = string_token;
        if (r_token.type != TK_ERROR)
            r_token.type = TK_ERROR;

        return _set_error("Failed to parse string name");
    }

    r_token.type = TK_STRING_NAME;
    r_token.value = StringName(string_token.value);

    return OK;
}

Error OrchestrationTextParser::_get_string_token(OrchestrationStringStream& p_stream, Token& r_token)
{
    String value;
    char32_t prev = 0;

    while (true)
    {
        char32_t ch = p_stream.read_char();
        if (ch == 0)
        {
            r_token.type = TK_ERROR;
            return _set_error("Unterminated tag");
        }

        if (ch == '"')
            break;

        if (ch == '\\')
        {
            // Escaped characters
            char32_t next = p_stream.read_char();
            if (next == 0)
            {
                r_token.type = TK_ERROR;
                return _set_error("Unterminated String");
            }

            char32_t res = 0;
            switch (next)
            {
                case 'b':
                {
                    res = 8;
                    break;
                }
                case 't':
                {
                    res = 9;
                    break;
                }
                case 'n':
                {
                    res = 10;
                    break;
                }
                case 'f':
                {
                    res = 12;
                    break;
                }
                case 'r':
                {
                    res = 13;
                    break;
                }
                case 'U':
                case 'u':
                {
                    // Hexadecimal sequence.
                    int hex_len = (next == 'U') ? 6 : 4;
                    for (int j = 0; j < hex_len; j++)
                    {
                        char32_t c = p_stream.read_char();

                        if (c == 0)
                        {
                            r_token.type = TK_ERROR;
                            return _set_error("Unterminated String");
                        }

                        if (!is_hex_digit(c))
                        {
                            r_token.type = TK_ERROR;
                            return _set_error("Malformed hex constant in string");
                        }

                        char32_t v;
                        if (is_digit(c))
                        {
                            v = c - '0';
                        }
                        else if (c >= 'a' && c <= 'f')
                        {
                            v = c - 'a';
                            v += 10;
                        }
                        else if (c >= 'A' && c <= 'F')
                        {
                            v = c - 'A';
                            v += 10;
                        }
                        else
                        {
                            ERR_PRINT("Bug parsing hex constant.");
                            v = 0;
                        }

                        res <<= 4;
                        res |= v;
                    }
                    break;
                }
                default:
                {
                    res = next;
                    break;
                }
            }

            // Parse UTF-16 pair.
            if ((res & 0xfffffc00) == 0xd800)
            {
                if (prev == 0)
                {
                    prev = res;
                    continue;
                }

                r_token.type = TK_ERROR;
                return _set_error("Invalid UTF-16 sequence in string, unpaired lead surrogate");
            }

            if ((res & 0xfffffc00) == 0xdc00)
            {
                if (prev == 0)
                {
                    r_token.type = TK_ERROR;
                    return _set_error("Invalid UTF-16 sequence in string, unpaired trail surrogate");
                }

                res = (prev << 10UL) + res - ((0xd800 << 10UL) + 0xdc00 - 0x10000);
                prev = 0;
            }

            if (prev != 0)
            {
                r_token.type = TK_ERROR;
                return _set_error("Invalid UTF-16 sequence in string, unpaired lead surrogate");
            }

            value += res;
        }
        else
        {
            if (prev != 0)
            {
                r_token.type = TK_ERROR;
                return _set_error("Invalid UTF-16 sequence in string, unpaired lead surrogate");
            }

            if (ch == '\n')
                _line++;

            value += ch;
        }
    }

    if (prev != 0)
    {
        r_token.type = TK_ERROR;
        return _set_error("Invalid UTF-16 sequence in string, unpaired lead surrogate");
    }

    if (p_stream.is_utf8())
        value.parse_utf8(value.ascii().get_data());

    r_token.type = TK_STRING;
    r_token.value = value;

    return OK;
}

Error OrchestrationTextParser::_get_number_token(OrchestrationStringStream& p_stream, Token& r_token) // NOLINT
{
    String value;
    int reading = READING_INT;

    char32_t ch = p_stream.read_char();
    if (ch == '-')
    {
        value += '-';
        ch = p_stream.read_char();
    }

    bool exp_sign = false;
    bool exp_begin = false;
    bool is_float = false;

    while (true)
    {
        switch (reading) // NOLINT
        {
            case READING_INT:
            {
                if (!is_digit(ch))
                {
                    if (ch == '.')
                    {
                        reading = READING_DEC;
                        is_float = true;
                    }
                    else if (ch == 'e')
                    {
                        reading = READING_EXP;
                        is_float = true;
                    }
                    else
                        reading = READING_DONE;
                }
                break;
            }
            case READING_DEC:
            {
                if (!is_digit(ch))
                    reading = (ch == 'e') ? READING_EXP : READING_DONE;

                break;
            }
            case READING_EXP:
            {
                if (is_digit(ch))
                    exp_begin = true;
                else if ((ch == '-' || ch == '+') && !exp_sign && !exp_begin)
                    exp_sign = true;
                else
                    reading = READING_DONE;
                break;
            }
        }

        if (reading == READING_DONE)
            break;

        value += ch;
        ch = p_stream.read_char();
    }

    p_stream.rewind();

    r_token.type = TK_NUMBER;
    if (is_float)
        r_token.value = value.to_float();
    else
        r_token.value = value.to_int();

    return OK;
}

Error OrchestrationTextParser::_get_identifier_token(OrchestrationStringStream& p_stream, Token& r_token) // NOLINT
{
    String id;
    bool first = true;

    char32_t ch = p_stream.read_char();
    #if GODOT_VERSION >= 0x040500
    while (is_ascii_alphanumeric_char(ch) || is_underscore(ch) || (!first && is_digit(ch)))
    #else
    while (is_ascii_char(ch) || is_underscore(ch) || (!first && is_digit(ch)))
    #endif
    {
        id += ch;
        ch = p_stream.read_char();
        first = false;
    }

    p_stream.rewind();

    r_token.type = TK_IDENTIFIER;
    r_token.value = id;
    return OK;
}

Error OrchestrationTextParser::_parse_tag(OrchestrationStringStream& p_stream, bool p_simple)
{
    Token token;

    const Error error = _get_token(p_stream, token);
    if (error != OK)
    {
        if (token.type == TK_EOF)
            return _set_error(ERR_FILE_EOF, "End-of-file");

        return error;
    }

    if (token.type != TK_BRACKET_OPEN)
        return _set_error("Expected '['");

    return _parse_tag(p_stream, token, p_simple);
}

Error OrchestrationTextParser::_parse_tag(OrchestrationStringStream& p_stream, Token& p_token, bool p_simple)
{
    _tag.fields.clear();

    if (p_token.type != TK_BRACKET_OPEN)
        return _set_error("Expected '['");

    if (p_simple)
    {
        _tag.name = "";
        _tag.fields.clear();

        bool escaping = false;

        if (p_stream.is_utf8())
        {
            CharString cs;
            while (true)
            {
                // todo: this may be problematic
                char ch = p_stream.read_char();
                if (p_stream.is_eof())
                    return _set_error(ERR_FILE_CORRUPT, "Unexpected EOF while parsing simple tag");

                if (ch == ']')
                {
                    if (escaping)
                        escaping = false;
                    else
                        break;
                }
                else if (ch == '\\')
                    escaping = true;
                else
                    escaping = false;

                cs += ch;
            }
            _tag.name.parse_utf8(cs.get_data(), cs.length());
        }
        else
        {
            while (true)
            {
                char32_t ch = p_stream.read_char();
                if (p_stream.is_eof())
                    return _set_error(ERR_FILE_CORRUPT, "Unexpected EOF while parsing simple tag");

                if (ch == ']')
                {
                    if (escaping)
                        escaping = false;
                    else
                        break;
                }
                else if (ch == '\\')
                    escaping = true;
                else
                    escaping = false;

                _tag.name += String::chr(ch);
            }
        }

        _tag.name = _tag.name.strip_edges();
        return OK;
    }

    if (const Error token_result = _get_token(p_stream, p_token))
        return token_result;

    if (p_token.type != TK_IDENTIFIER)
        return _set_error("Expected identifier (tag name)");

    _tag.name = p_token.value;

    bool parsing_tag = true;
    while (true)
    {
        if (p_stream.is_eof())
            return _set_error(ERR_FILE_CORRUPT, "Unexpected EOF while parsing tag: " + _tag.name);

        _get_token(p_stream, p_token);
        if (p_token.type == TK_BRACKET_CLOSE)
            break;

        if (parsing_tag && p_token.type == TK_PERIOD)
        {
            _tag.name += "."; // supports tags like [someprop.Android] for platform
            _get_token(p_stream, p_token);
        }
        else if (parsing_tag && p_token.type == TK_COLON)
        {
            _tag.name += ":"; // supports tags like [someprop:Android] for platform
            _get_token(p_stream, p_token);
        }
        else
            parsing_tag = false;

        if (p_token.type != TK_IDENTIFIER)
            return _set_error("Expected identifier");

        const String identifier = p_token.value;
        if (parsing_tag)
        {
            _tag.name += identifier;
            continue;
        }

        _get_token(p_stream, p_token);
        if (p_token.type != TK_EQUAL)
            return _set_error("Expected '='");

        _get_token(p_stream, p_token);

        Variant value;
        if (const Error err = _parse_value(p_stream, p_token, value))
            return err;

        _tag.fields[identifier] = value;
    }

    return OK;
}

Error OrchestrationTextParser::_parse_tag_assign_eof(OrchestrationStringStream& p_stream, String& r_name, Variant& r_value, bool p_simple)
{
    r_name = "";

    String what;
    while (true)
    {
        char32_t ch = p_stream.read_char();
        if (p_stream.is_eof())
            return _set_error(ERR_FILE_EOF, "Unexpected end-of-file");

        if (ch == ';')
        {
            // Comments
            while (true)
            {
                ch = p_stream.read_char();
                if (p_stream.is_eof())
                    return _set_error(ERR_FILE_EOF, "Unexpected end-of-file");

                if (ch == '\n')
                {
                    _line++;
                    break;
                }
            }
            continue;
        }

        if (ch == '[' && what.length() == 0)
        {
            // Tag detected
            p_stream.rewind();
            return _parse_tag(p_stream, p_simple);
        }

        if (ch > 32)
        {
            if (ch == '"')
            {
                // Quoted
                p_stream.rewind();

                Token token;
                if (const Error err = _get_token(p_stream, token))
                    return err;

                if (token.type != TK_STRING)
                    return _set_error(ERR_INVALID_DATA, "Error reading quoted string");

                what = token.value;
            }
            else if (ch != '=')
            {
                what += String::chr(ch);
            }
            else
            {
                r_name = what;

                Token token;
                _get_token(p_stream, token);
                return _parse_value(p_stream, token, r_value);
            }
        }
        else if (ch == '\n')
            _line++;
    }

    return _set_error("Failed to parse assignment");
}

Error OrchestrationTextParser::_parse_value(OrchestrationStringStream& p_stream, Token& p_token, Variant& r_value)
{
    switch (p_token.type)
    {
        case TK_CURLY_BRACKET_OPEN:
        {
            Dictionary dict;
            if (const Error err = _parse_dictionary(p_stream, dict))
                return err;

            r_value = dict;
            return OK;
        }
        case TK_BRACKET_OPEN:
        {
            Array array;
            if (const Error err = _parse_array(p_stream, array))
                return err;

            r_value = array;
            return OK;
        }
        case TK_IDENTIFIER:
        {
            if (const Error err = _parse_identifier(p_stream, p_token, r_value))
                return err;

            return OK;
        }
        case TK_NUMBER:
        case TK_STRING:
        case TK_STRING_NAME:
        case TK_COLOR:
        {
            r_value = p_token.value;
            return OK;
        }
        default:
            return _set_error("Expected value, got " + String(tk_name[p_token.type]) + ".");
    }
}

Error OrchestrationTextParser::_parse_dictionary(OrchestrationStringStream& p_stream, Dictionary& r_value)
{
    Token token;
    Variant key;
    bool at_key = true;
    bool need_comma = false;

    while (true)
    {
        if (p_stream.is_eof())
            return _set_error(ERR_FILE_CORRUPT, "Unexpected EOF while parsing dictionary");

        if (at_key)
        {
            if (const Error err = _get_token(p_stream, token))
                return err;

            if (token.type == TK_CURLY_BRACKET_CLOSE)
                return OK;

            if (need_comma)
            {
                if (token.type != TK_COMMA)
                    return _set_error("Expected '}' or ','");

                need_comma = false;
                continue;
            }

            if (const Error err = _parse_value(p_stream, token, key))
                return err;

            if (const Error err = _get_token(p_stream, token))
                return err;

            if (token.type != TK_COLON)
                return _set_error("Expected ':'");

            at_key = false;
        }
        else
        {
            if (const Error err = _get_token(p_stream, token))
                return err;

            Variant value;
            const Error err = _parse_value(p_stream, token, value);
            if (err && err != ERR_FILE_MISSING_DEPENDENCIES)
                return err;

            r_value[key] = value;
            need_comma = true;
            at_key = true;
        }
    }

    return _set_error("Failed to parse dictionary");
}

Error OrchestrationTextParser::_parse_array(OrchestrationStringStream& p_stream, Array& r_value)
{
    Token token;
    bool need_comma = false;

    while (true)
    {
        if (p_stream.is_eof())
            return _set_error(ERR_FILE_CORRUPT, "Unexpected EOF while parsing array");

        if (const Error err = _get_token(p_stream, token))
            return err;

        if (token.type == TK_BRACKET_CLOSE)
            return OK;

        if (need_comma)
        {
            if (token.type != TK_COMMA)
                return _set_error("Expected ','");

            need_comma = false;
            continue;
        }

        Variant value;
        if (const Error err = _parse_value(p_stream, token, value))
            return err;

        r_value.push_back(value);
        need_comma = true;
    }

    return _set_error("Failed to parse array");
}

Error OrchestrationTextParser::_parse_identifier(OrchestrationStringStream& p_stream, Token& p_token, Variant& r_value)
{
    const String id = p_token.value;

    if (id == "true")
    {
        r_value = true;
    }
    else if (id == "false")
    {
        r_value = false;
    }
    else if (id == "null" || id == "nil")
    {
        r_value = Variant();
    }
    else if (id == "inf")
    {
        r_value = INFINITY;
    }
    else if (id == "inf_neg")
    {
        r_value = -INFINITY;
    }
    else if (id == "nan")
    {
        r_value = NAN;
    }
    else if (id == "Vector2")
    {
        Vector<real_t> args;
        if (const Error err = _parse_construct<real_t>(p_stream, args))
            return err;

        if (args.size() != 2)
            return _set_error("Expected 2 arguments for constructor");

        r_value = Vector2(args[0], args[1]);
    }
    else if (id == "Vector2i")
    {
        Vector<int32_t> args;
        if (const Error err = _parse_construct<int32_t>(p_stream, args))
            return err;

        if (args.size() != 2)
            _set_error("Expected 2 arguments for constructor");

        r_value = Vector2i(args[0], args[1]);
    }
    else if (id == "Rect2")
    {
        Vector<real_t> args;
        if (const Error err = _parse_construct<real_t>(p_stream, args))
            return err;

        if (args.size() != 4)
            return _set_error("Expected 4 arguments for constructor");

        r_value = Rect2(args[0], args[1], args[2], args[3]);
    }
    else if (id == "Rect2i")
    {
        Vector<int32_t> args;
        if (const Error err = _parse_construct<int32_t>(p_stream, args))
            return err;

        if (args.size() != 4)
            return _set_error("Expected 4 arguments for constructor");

        r_value = Rect2i(args[0], args[1], args[2], args[3]);
    }
    else if (id == "Vector3")
    {
        Vector<real_t> args;
        if (const Error err = _parse_construct<real_t>(p_stream, args))
            return err;

        if (args.size() != 3)
            return _set_error("Expected 3 arguments for constructor");

        r_value = Vector3(args[0], args[1], args[2]);
    }
    else if (id == "Vector3i")
    {
        Vector<int32_t> args;
        if (const Error err = _parse_construct<int32_t>(p_stream, args))
            return err;

        if (args.size() != 3)
            return _set_error("Expected 3 arguments for constructor");

        r_value = Vector3i(args[0], args[1], args[2]);
    }
    else if (id == "Vector4")
    {
        Vector<real_t> args;
        if (const Error err = _parse_construct<real_t>(p_stream, args))
            return err;

        if (args.size() != 4)
            return _set_error("Expected 4 arguments for constructor");

        r_value = Vector4(args[0], args[1], args[2], args[3]);
    }
    else if (id == "Vector4i")
    {
        Vector<int32_t> args;
        if (const Error err = _parse_construct<int32_t>(p_stream, args))
            return err;

        if (args.size() != 4)
            return _set_error("Expected 4 arguments for constructor");

        r_value = Vector4i(args[0], args[1], args[2], args[3]);
    }
    else if (id == "Transform2D" || id == "Matrix32")
    {
        Vector<real_t> args;
        if (const Error err = _parse_construct<real_t>(p_stream, args))
            return err;

        if (args.size() != 6)
            return _set_error("Expected 6 arguments for constructor");

        Transform2D m;
        m[0] = Vector2(args[0], args[1]);
        m[1] = Vector2(args[2], args[3]);
        m[2] = Vector2(args[4], args[5]);
        r_value = m;
    }
    else if (id == "Plane")
    {
        Vector<real_t> args;
        if (const Error err = _parse_construct<real_t>(p_stream, args))
            return err;

        if (args.size() != 4)
            return _set_error("Expected 4 arguments for constructor");

        r_value = Plane(args[0], args[1], args[2], args[3]);
    }
    else if (id == "Quaternion" || id == "Quat")
    {
        Vector<real_t> args;
        if (const Error err = _parse_construct<real_t>(p_stream, args))
            return err;

        if (args.size() != 4)
            return _set_error("Expected 4 arguments for constructor");

        r_value = Quaternion(args[0], args[1], args[2], args[3]);
    }
    else if (id == "AABB" || id == "Rect3")
    {
        Vector<real_t> args;
        if (const Error err = _parse_construct<real_t>(p_stream, args))
            return err;

        if (args.size() != 6)
            return _set_error("Expected 6 arguments for constructor");

        r_value = AABB(Vector3(args[0], args[1], args[2]), Vector3(args[3], args[4], args[5]));
    }
    else if (id == "Basis" || id == "Matrix3")
    {
        Vector<real_t> args;
        if (const Error err = _parse_construct<real_t>(p_stream, args))
            return err;

        if (args.size() != 9)
            return _set_error("Expected 9 arguments for constructor");

        r_value = Basis(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
    }
    else if (id == "Transform3D" || id == "Transform")
    {
        Vector<real_t> args;
        if (const Error err = _parse_construct<real_t>(p_stream, args))
            return err;

        if (args.size() != 12)
            return _set_error("Expected 12 arguments for constructor");

        r_value = Transform3D(
            Basis(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]),
            Vector3(args[9], args[10], args[11]));
    }
    else if (id == "Projection")
    {
        Vector<real_t> args;
        if (const Error err = _parse_construct<real_t>(p_stream, args))
            return err;

        if (args.size() != 16)
            return _set_error("Expected 16 arguments for constructor");

        r_value = Projection(
            Vector4(args[0], args[1], args[2], args[3]),
            Vector4(args[4], args[5], args[6], args[7]),
            Vector4(args[8], args[9], args[10], args[11]),
            Vector4(args[12], args[13], args[14], args[15]));;
    }
    else if (id == "Color")
    {
        Vector<float> args;
        if (const Error err = _parse_construct<float>(p_stream, args))
            return err;

        if (args.size() != 4)
            return _set_error("Expected 4 arguments for constructor");

        r_value = Color(args[0], args[1], args[2], args[3]);
    }
    else if (id == "NodePath")
    {
        _get_token(p_stream, p_token);
        if (p_token.type != TK_PARENTHESIS_OPEN)
            return _set_error("Expected '('");

        _get_token(p_stream, p_token);
        if (p_token.type != TK_STRING)
            return _set_error("Expected string as argument for NodePath()");

        r_value = NodePath(String(p_token.value));

        _get_token(p_stream, p_token);
        if (p_token.type != TK_PARENTHESIS_CLOSE)
            return _set_error("Expected ')'");
    }
    else if (id == "RID")
    {
        _get_token(p_stream, p_token);
        if (p_token.type != TK_PARENTHESIS_OPEN)
            return _set_error("Expected '('");

        _get_token(p_stream, p_token);

        // Permit empty RID
        if (p_token.type == TK_PARENTHESIS_CLOSE)
        {
            r_value = RID();
            return OK;
        }

        if (p_token.type != TK_STRING)
            return _set_error("Expected number as argument or ')'");

        // Cannot construct RID
        r_value = RID();

        _get_token(p_stream, p_token);
        if (p_token.type != TK_PARENTHESIS_CLOSE)
            return _set_error("Expected ')'");
    }
    else if (id == "Signal")
    {
        _get_token(p_stream, p_token);
        if (p_token.type != TK_PARENTHESIS_OPEN)
            return _set_error("Expected '('");

        // Load as empty
        r_value = Signal();

        _get_token(p_stream, p_token);
        if (p_token.type != TK_PARENTHESIS_CLOSE)
            return _set_error("Expected ')'");
    }
    else if (id == "Callable")
    {
        _get_token(p_stream, p_token);
        if (p_token.type != TK_PARENTHESIS_OPEN)
            return _set_error("Expected '('");

        // Load as empty
        r_value = Callable();

        _get_token(p_stream, p_token);
        if (p_token.type != TK_PARENTHESIS_CLOSE)
            return _set_error("Expected ')'");
    }
    else if (id == "Object")
    {
        _get_token(p_stream, p_token);
        if (p_token.type != TK_PARENTHESIS_OPEN)
            return _set_error("Expected '('");

        _get_token(p_stream, p_token);
        if (p_token.type != TK_IDENTIFIER)
            return _set_error("Expected identifier with type of object");

        String type = p_token.value;
        if (!ClassDB::can_instantiate(type))
            return _set_error("Expected a constructable type, cannot construct '" + type + "'.");

        Object* obj = ClassDB::instantiate(type);
        if (!obj)
            return _set_error("Cannot instantiate Object() of type: " + type);

        Ref<RefCounted> ref = Object::cast_to<RefCounted>(obj);

        _get_token(p_stream, p_token);
        if (p_token.type != TK_COMMA)
            return _set_error("Expected ',' after object type");

        bool at_key{ true };
        bool need_comma{ false };
        String key;

        while (true)
        {
            if (p_stream.is_eof())
                return _set_error(ERR_FILE_CORRUPT, "Unexpected EOF while parsing Object()");

            if (at_key)
            {
                if (const Error err = _get_token(p_stream, p_token))
                    return err;

                if (p_token.type == TK_PARENTHESIS_CLOSE)
                {
                    r_value = ref.is_valid() ? Variant(ref) : Variant(obj);
                    return OK;
                }

                if (need_comma)
                {
                    if (p_token.type != TK_COMMA)
                        return _set_error("Expected '}' or ','");

                    need_comma = false;
                    continue;
                }

                if (p_token.type != TK_STRING)
                    return _set_error("Expected property name as string");

                key = p_token.value;

                if (const Error err = _get_token(p_stream, p_token))
                    return err;

                if (p_token.type != TK_COLON)
                    return _set_error("Expected ':'");

                at_key = false;
            }
            else
            {
                if (const Error err = _get_token(p_stream, p_token))
                    return err;

                Variant value;
                if (const Error err = _parse_value(p_stream, p_token, value))
                    return err;

                obj->set(key, value);
                need_comma = true;
                at_key = true;
            }
        }
    }
    else if (id == "Resource" || id == "SubResource" || id == "ExtResource")
    {
        _get_token(p_stream, p_token);
        if (p_token.type != TK_PARENTHESIS_OPEN)
            return _set_error("Expected '('");

        if (id == "Resource")
        {
            Ref<Resource> res;
            if (const Error err = _parse_resource(p_stream, res))
                return err;

            r_value = res;
        }
        else if (id == "ExtResource")
        {
            Ref<Resource> res;
            if (const Error err = _parse_extresource(p_stream, res))
            {
                // If the file is missing, can safely be ignored
                if (err != ERR_FILE_NOT_FOUND && err != ERR_CANT_OPEN)
                    return err;
            }

            r_value = res;
        }
        else if (id == "SubResource")
        {
            Ref<Resource> res;
            if (const Error err = _parse_subresource(p_stream, res))
                return err;

            r_value = res;
        }
        else
        {
            _get_token(p_stream, p_token);
            if (p_token.type != TK_STRING)
                return _set_error("Expected string as argument for Resource().");

            String path = p_token.value;
            Ref<Resource> res = ResourceLoader::get_singleton()->load(path);
            if (res.is_null())
                return _set_error("Cannot load resource at path: '" + path + "'.");

            _get_token(p_stream, p_token);
            if (p_token.type != TK_PARENTHESIS_CLOSE)
                return _set_error("Expected ')'");

            r_value = res;
        }
    }
    else if (id == "Array")
    {
        _get_token(p_stream, p_token);
        if (p_token.type != TK_BRACKET_OPEN)
            return _set_error("Expected '['");

        _get_token(p_stream, p_token);
        if (p_token.type != TK_IDENTIFIER)
            return _set_error("Expected type identifier");

        static HashMap<String, Variant::Type> builtin_types;
        if (builtin_types.is_empty())
        {
            for (int i = 0; i < Variant::VARIANT_MAX; i++)
                builtin_types[Variant::get_type_name(Variant::Type(i))] = Variant::Type(i);
        }

        Array array = Array();
        bool got_bracket_token{ false };
        if (builtin_types.has(p_token.value))
        {
            array.set_typed(builtin_types.get(p_token.value), StringName(), Variant());
        }
        else if (p_token.value == Variant("Resource") || p_token.value == Variant("SubResource") || p_token.value == Variant("ExtResource"))
        {
            Variant resource;
            if (const Error err = _parse_value(p_stream, p_token, resource))
            {
                if (p_token.value == Variant("Resource") && _is_parse_error("Expected '('") && p_token.type == TK_BRACKET_CLOSE)
                {
                    _set_error(OK);
                    array.set_typed(Variant::OBJECT, p_token.value, Variant());
                    got_bracket_token = true;
                }
                else
                    return err;
            }
            else
            {
                Ref<Script> script = resource;
                // todo: the Script::is_valid is not exposed
                if (script.is_valid() /*&& script->is_valid()*/)
                    array.set_typed(Variant::OBJECT, script->get_instance_base_type(), script);
            }
        }
        else if (ClassDB::class_exists(p_token.value))
            array.set_typed(Variant::OBJECT, p_token.value, Variant());

        if (!got_bracket_token)
        {
            _get_token(p_stream, p_token);
            if (p_token.type != TK_BRACKET_CLOSE)
                return _set_error("Expected ']'");
        }

        _get_token(p_stream, p_token);
        if (p_token.type != TK_PARENTHESIS_OPEN)
            return _set_error("Expected '('");

        _get_token(p_stream, p_token);
        if (p_token.type != TK_BRACKET_OPEN)
            return _set_error("Expected '['");

        Array values;
        if (const Error err = _parse_array(p_stream, values))
            return err;

        _get_token(p_stream, p_token);
        if (p_token.type != TK_PARENTHESIS_CLOSE)
            return _set_error("Expected ')'");

        array.assign(values);
        r_value = array;
    }
    else if (id == "PackedByteArray")
    {
        Vector<uint8_t> args;
        if (const Error err = _parse_construct<uint8_t>(p_stream, args))
            return err;

        PackedByteArray array;
        {
            int size = args.size();
            array.resize(size);

            uint8_t *w = array.ptrw();
            for (int i = 0; i < size; i++)
                w[i] = args[i];
        }
        r_value = array;
    }
    else if (id == "PackedInt32Array")
    {
        Vector<int32_t> args;
        if (const Error err = _parse_construct<int32_t>(p_stream, args))
            return err;

        PackedInt32Array array;
        {
            int size = args.size();
            array.resize(size);

            int32_t *w = array.ptrw();
            for (int i = 0; i < size; i++)
                w[i] = args[i];
        }
        r_value = array;
    }
    else if (id == "PackedInt64Array")
    {
        Vector<int64_t> args;
        if (const Error err = _parse_construct<int64_t>(p_stream, args))
            return err;

        PackedInt64Array array;
        {
            int size = args.size();
            array.resize(size);

            int64_t *w = array.ptrw();
            for (int i = 0; i < size; i++)
                w[i] = args[i];
        }
        r_value = array;
    }
    else if (id == "PackedFloat32Array")
    {
        Vector<float> args;
        if (const Error err = _parse_construct<float>(p_stream, args))
            return err;

        PackedFloat32Array array;
        {
            int size = args.size();
            array.resize(size);

            float *w = array.ptrw();
            for (int i = 0; i < size; i++)
                w[i] = args[i];
        }
        r_value = array;
    }
    else if (id == "PackedFloat64Array")
    {
        Vector<double> args;
        if (const Error err = _parse_construct<double>(p_stream, args))
            return err;

        PackedFloat64Array array;
        {
            int size = args.size();
            array.resize(size);

            double *w = array.ptrw();
            for (int i = 0; i < size; i++)
                w[i] = args[i];
        }
        r_value = array;
    }
    else if (id == "PackedStringArray")
    {
        _get_token(p_stream, p_token);
        if (p_token.type != TK_PARENTHESIS_OPEN)
            return _set_error("Expected '('");

        bool first{ true };
        Vector<String> vs;
        while (true)
        {
            if (!first)
            {
                _get_token(p_stream, p_token);
                if (p_token.type != TK_COMMA)
                {
                    if (p_token.type == TK_PARENTHESIS_CLOSE)
                        break;

                    return _set_error("Expected ',' or ')'");
                }
            }

            _get_token(p_stream, p_token);
            if (p_token.type == TK_PARENTHESIS_CLOSE)
                break;

            if (p_token.type != TK_STRING)
                return _set_error("Expected string");

            first = false;
            vs.push_back(p_token.value);
        }

        PackedStringArray array;
        {
            int size = vs.size();
            array.resize(size);
            String *w = array.ptrw();
            for (int i = 0; i < size; i++)
                w[i] = vs[i];
        }
        r_value = array;
    }
    else if (id == "PackedVector2Array")
    {
        Vector<real_t> args;
        if (const Error err = _parse_construct<real_t>(p_stream, args))
            return err;

        PackedVector2Array array;
        {
            int size = args.size() / 2;
            array.resize(size);

            Vector2* w = array.ptrw();
            for (int i = 0; i < size; i++)
                w[i] = Vector2(args[i * 2 + 0], args[i * 2 + 1]);
        }
        r_value = array;
    }
    else if (id == "PackedVector3Array")
    {
        Vector<real_t> args;
        if (const Error err = _parse_construct<real_t>(p_stream, args))
            return err;

        PackedVector3Array array;
        {
            int size = args.size() / 3;
            array.resize(size);

            Vector3* w = array.ptrw();
            for (int i = 0; i < size; i++)
                w[i] = Vector3(args[i * 3 + 0], args[i * 3 + 1], args[i * 3 + 2]);
        }
        r_value = array;
    }
    else if (id == "PackedColorArray")
    {
        Vector<real_t> args;
        if (const Error err = _parse_construct<real_t>(p_stream, args))
            return err;

        PackedColorArray array;
        {
            int size = args.size() / 4;
            array.resize(size);

            Color* w = array.ptrw();
            for (int i = 0; i < size; i++)
                w[i] = Color(args[i * 4 + 0], args[i * 4 + 1], args[i * 4 + 2], args[i * 4 + 3]);
        }
        r_value = array;
    }
    else if (id == "PackedVector4Array")
    {
        Vector<real_t> args;
        if (const Error err = _parse_construct<real_t>(p_stream, args))
            return err;

        PackedVector4Array array;
        {
            int size = args.size() / 4;
            array.resize(size);

            Vector4* w = array.ptrw();
            for (int i = 0; i < size; i++)
                w[i] = Vector4(args[i * 3 + 0], args[i * 3 + 1], args[i * 3 + 2], args[i * 3 + 3]);
        }
        r_value = array;
    }
    else
    {
        return _set_error("Unknown identifier: '" + id + "'.");
    }

    // All above branches end up here unless they returned early
    return OK;
}

Error OrchestrationTextParser::_parse_resource(OrchestrationStringStream& p_stream, Ref<Resource>& r_value) // NOLINT
{
    ERR_FAIL_V_MSG(OK, "Orchestration text format does not use 'Resource' types.");
}

Error OrchestrationTextParser::_parse_extresource(OrchestrationStringStream& p_stream, Ref<Resource>& r_value)
{
    // todo: get_classes_used needs dummy

    Token token;
    _get_token(p_stream, token);
    if (token.type != TK_NUMBER && token.type != TK_STRING)
        return _set_error("Expected number (old style sub-resource index) or String (ext-resource ID)");

    const String id = token.value;

    if (!_ignore_external_resources)
    {
        if (!_external_resources.has(id))
            return _set_error("Can't load cached ext-resource id: " + id);

        String path = _external_resources[id].path;
        String type = _external_resources[id].type;

        // todo: support load tokens
        Ref<Resource> res = _external_resources[id].resource;
        if (res.is_valid())
        {
            if (res.is_null())
            {
                WARN_PRINT("External resource failed to load at: " + path);
            }
            else
            {
                #ifdef TOOLS_ENABLED
                #if GODOT_VERSION >= 0x040400
                res->set_id_for_path(_local_path, id);
                #else
                ResourceCache::get_singleton()->set_id_for_path(_local_path, res->get_path(), id);
                #endif
                #endif
                r_value = res;
            }
        }
        else
            r_value = Ref<Resource>();

        #ifdef TOOLS_ENABLED
        if (r_value.is_null())
        {
            // Hack to allow checking original path
            r_value.instantiate();
            r_value->set_meta("__load_path__", _external_resources[id].path);
        }
        #endif
    }

    _get_token(p_stream, token);
    if (token.type != TK_PARENTHESIS_CLOSE)
        return _set_error("Expected ')'");

    return OK;
}

Error OrchestrationTextParser::_parse_subresource(OrchestrationStringStream& p_stream, Ref<Resource>& r_value)
{
    // get_classes_used needs dummy

    Token token;
    _get_token(p_stream, token);
    if (token.type != TK_NUMBER && token.type != TK_STRING)
        return _set_error("Expected number (old style) or string (sub-resource index)");

    const String id = token.value;
    ERR_FAIL_COND_V(!_internal_resources.has(id), ERR_INVALID_PARAMETER);

    r_value = _internal_resources[id];

    _get_token(p_stream, token);
    if (token.type != TK_PARENTHESIS_CLOSE)
        return _set_error("Expected ')'");

    return OK;
}

Error OrchestrationTextParser::_parse_header(OrchestrationStringStream& p_stream, bool p_skip_first_tag)
{
    _ignore_external_resources = false;
    _total_resources = 0;

    if (const Error err = _parse_tag(p_stream))
        return err;

    if (_tag.fields.has("format"))
    {
        uint32_t format = _tag.fields["format"];
        if (format > OrchestrationFormat::FORMAT_VERSION)
            return _set_error(ERR_FILE_UNRECOGNIZED, "Saved with a newer version of the format");

        _version = format;
    }

    if (_tag.name != "orchestration")
        return _set_error("Unrecognized file type: " + _tag.name);

    if (!_tag.fields.has("type"))
        return _set_error("Missing 'type' field in 'orchestration' tag");

    if (_tag.fields.has("script_class"))
        _script_class = _tag.fields["script_class"];

    _res_type = _tag.fields["type"];
    if (_res_type == "OScript")
        _res_type = Orchestration::get_class_static();

    if (_tag.fields.has("uid"))
        _res_uid = ResourceUID::get_singleton()->text_to_id(_tag.fields["uid"]);
    else
        _res_uid = ResourceUID::INVALID_ID;

    if (_tag.fields.has("load_steps"))
        _total_resources = _tag.fields["load_steps"];
    else
        _total_resources = 0;

    if (!p_skip_first_tag)
    {
        if (const Error err = _parse_tag(p_stream))
            return _set_error(ERR_FILE_CORRUPT, "Unexpected end-of-file");
    }

    return OK;
}

Error OrchestrationTextParser::_parse_ext_resources(OrchestrationStringStream& p_stream)
{
    while (true)
    {
        if (_tag.name != "ext_resource")
            break;

        if (!_tag.fields.has("path"))
            return _set_error(ERR_FILE_CORRUPT, "Missing 'path' in external resource tag");

        if (!_tag.fields.has("type"))
            return _set_error(ERR_FILE_CORRUPT, "Missing 'type' in external resource tag");

        if (!_tag.fields.has("id"))
            return _set_error(ERR_FILE_CORRUPT, "Missing 'id' in external resource tag");

        String path = _tag.fields["path"];
        String type = _tag.fields["type"];
        String id = _tag.fields["id"];

        if (_tag.fields.has("uid"))
        {
            const String uid_text = _tag.fields["uid"];

            int64_t uid = ResourceUID::get_singleton()->text_to_id(uid_text);
            if (uid != ResourceUID::INVALID_ID && ResourceUID::get_singleton()->has_id(uid))
            {
                // If a UID is found and the path is valid, it will be used; otherwise fallback to path
                path = ResourceUID::get_singleton()->get_id_path(uid);
            }
            else
            {
                #ifdef TOOLS_ENABLED
                const bool show = ResourceLoader::get_singleton()->get_resource_uid(path) != uid;
                #else
                const bool show = true;
                #endif
                if (show)
                {
                    WARN_PRINT(String(_res_path + ":" + itos(_line) + " - ext_resource, invalid UID: " + uid_text +
                        " - using text path instead: " + path).utf8().get_data());
                }
            }
        }

        if (!path.contains("://") && path.is_relative_path())
        {
            // path is relative to file being loaded, so convert to a resource path
            path = ProjectSettings::get_singleton()->localize_path(_local_path.get_base_dir().path_join(path));
        }

        if (_remaps.has(path))
            path = _remaps[path];

        _external_resources[id].path = path;
        _external_resources[id].type = type;
        _external_resources[id].resource = ResourceLoader::get_singleton()->load(path, type, (ResourceLoader::CacheMode)_cache_mode);

        if (!_external_resources[id].resource.is_valid())
            return _set_error(ERR_FILE_CORRUPT, "[ext_resource] referenced non-existent resource at: " + path);

        if (const Error error = _parse_tag(p_stream))
            return error;

        _parsed_external_resources++;
    }

    return OK;
}

Error OrchestrationTextParser::_parse_objects(OrchestrationStringStream& p_stream)
{
    while (true)
    {
        if (_tag.name != "obj")
            break;

        if (!_tag.fields.has("type"))
            return _set_error(ERR_FILE_CORRUPT, "Missing 'type' in obj tag");

        if (!_tag.fields.has("id"))
            return _set_error(ERR_FILE_CORRUPT, "Missing 'id' in obj tag");

        String type = _tag.fields["type"];
        String id = _tag.fields["id"];
        String path = _local_path + "::" + id;

        bool assign = false;
        Ref<Resource> resource;

        if (_cache_mode == ResourceFormatLoader::CACHE_MODE_REPLACE && ResourceCache::has(path))
        {
            // Reuse existing
            Ref<Resource> cache = ResourceCache::get_singleton()->get_ref(path);
            if (cache.is_valid() && cache->get_class() == type)
            {
                resource = cache;
                #if GODOT_VERSION >= 0x040400
                resource->reset_state();
                #endif
                assign = true;
            }
        }

        MissingResource* missing_resource = nullptr;

        if (resource.is_null())
        {
            Ref<Resource> cache = ResourceCache::get_singleton()->get_ref(path);
            if (_cache_mode == ResourceFormatLoader::CACHE_MODE_IGNORE && cache.is_valid())
            {
                // cached, do not assign
                resource = cache;
            }
            else
            {
                // Create
                Variant obj = ClassDB::instantiate(type);
                if (!obj)
                {
                    if (!_is_creating_missing_resources_if_class_unavailable_enabled())
                        return _set_error(ERR_FILE_CORRUPT, "Cannot create sub resource of type: " + type);

                    missing_resource = memnew(MissingResource);
                    missing_resource->set_original_class(type);
                    missing_resource->set_recording_properties(true);
                    obj = missing_resource;
                }

                Resource* r = Object::cast_to<Resource>(obj);
                if (!r)
                    return _set_error(ERR_FILE_CORRUPT, "Cannot create sub resource of type, because not a resource: " + type);

                resource = Ref<Resource>(r);
                assign = true;
            }
        }

        _parsed_internal_resources++;

        _internal_resources[id] = resource;
        if (assign)
        {
            // todo: is this completely necessary for internal resources?
            //  this seems to add a path to the resource unnecessarily for internal resources

            // if (_cache_mode != ResourceFormatLoader::CACHE_MODE_IGNORE)
            // {
            //     if (_cache_mode == ResourceFormatLoader::CACHE_MODE_REPLACE)
            //         resource->take_over_path(path);
            //     else
            //         resource->set_path(path);
            // }
            // else
            // {
            //     #if GODOT_VERSION >= 0x040400
            //     resource->set_path_cache(path);
            //     #endif
            // }

            #if GODOT_VERSION >= 0x040300
            resource->set_scene_unique_id(id);
            #else
            ResourceCache::get_singleton()->set_scene_unique_id(_local_path, resource, id);
            #endif
        }

        Dictionary missing_properties;
        while (true)
        {
            String property_name;
            Variant value;

            if (const Error error = _parse_tag_assign_eof(p_stream, property_name, value))
                return error;

            if (!property_name.is_empty())
            {
                if (assign)
                {
                    bool set_valid{ true };
                    if (value.get_type() == Variant::OBJECT && missing_resource != nullptr)
                    {
                        // If the property being set is a missing resource and the parent isn't, setting it
                        // most likely will not work, so save it as metadata
                        Ref<MissingResource> mr = value;
                        if (mr.is_valid())
                        {
                            missing_properties[property_name] = mr;
                            set_valid = false;
                        }
                    }

                    if (value.get_type() == Variant::ARRAY)
                    {
                        Array set_array = value;
                        // todo: how to deal with is valid?
                        Variant get_value = resource->get(property_name);
                        if (get_value.get_type() == Variant::ARRAY)
                        {
                            Array get_array = get_value;
                            if (!set_array.is_same_typed(get_array))
                                value = Array(set_array, get_array.get_typed_builtin(), get_array.get_typed_class_name(), get_array.get_typed_script());
                        }
                    }

                    if (set_valid)
                        resource->set(property_name, value);
                }
            }
            else if (!_tag.name.is_empty())
            {
                break;
            }
            else
            {
                return _set_error(ERR_FILE_CORRUPT, "Premature EOF while parsing [obj]");
            }
        }

        if (missing_resource)
            missing_resource->set_recording_properties(false);

        if (!missing_properties.is_empty())
            resource->set_meta("metadata/_missing_resources", missing_properties);
    }

    return OK;
}

Error OrchestrationTextParser::_parse_resource(OrchestrationStringStream& p_stream, Ref<Orchestration>& r_value)
{
    while (true)
    {
        if (_tag.name != "resource")
            break;

        Ref<Resource> resource;

        Ref<Resource> cached = ResourceCache::get_singleton()->get_ref(_local_path);
        if (_cache_mode == ResourceFormatLoader::CACHE_MODE_REPLACE && cached.is_valid() && cached->get_class() == _res_type)
        {
            #if GODOT_VERSION >= 0x040400
            cached->reset_state();
            #endif
            resource = cached;
        }

        MissingResource *missing_resource = nullptr;
        if (!resource.is_valid())
        {
            Variant obj = ClassDB::instantiate(_res_type);
            if (!obj)
            {
                if (!_is_creating_missing_resources_if_class_unavailable_enabled())
                    return _set_error(ERR_FILE_CORRUPT, "Cannot create resource of type: " + _res_type);

                missing_resource = memnew(MissingResource);
                missing_resource->set_original_class(_res_type);
                missing_resource->set_recording_properties(true);
                obj = missing_resource;
            }

            Resource *r = Object::cast_to<Resource>(obj);
            if (!r)
                _set_error(ERR_FILE_CORRUPT, "Can't create sub resource of type, because not a resource: " + _res_type);

            resource = Ref<Resource>(r);
        }

        r_value = resource;
        r_value->_version = _version;

        Dictionary missing_resource_properties;
        while (true)
		{
			String property_name;
			Variant value;

            _parse_tag_assign_eof(p_stream, property_name, value);
            if (_error)
            {
                if (_error == ERR_FILE_EOF)
                {
                    _set_error(OK);

                    // todo: setting this on the orchestration causes an issue when setting the path
                    //  on the script. Godot does not permit two resources referring to the same path.
                    // if (_cache_mode != ResourceFormatLoader::CACHE_MODE_IGNORE)
                    // {
                    //     if (!ResourceCache::has(_res_path))
                    //         resource->set_path(_res_path);
                    //
                    //     // todo: requires Godot change to support this
                    //     // _resource->set_as_translation_remapped(_translation_remapped);
                    // }
                    // else
                    // {
                    //     #if GODOT_VERSION >= 0x040400
                    //     resource->set_path_cache(_res_path);
                    //     #endif
                    // }
                }
                return _error;
            }

			if (!property_name.is_empty())
			{
				bool set_valid = true;

				if (value.get_type() == Variant::OBJECT && missing_resource != nullptr)
				{
					// If the property being set is a missing resource (and the parent is not),
					// then setting it will most likely not work.
					// Instead, save it as metadata.

					Ref<MissingResource> mr = value;
					if (mr.is_valid()) {
						missing_resource_properties[property_name] = mr;
						set_valid = false;
					}
				}

				if (value.get_type() == Variant::ARRAY)
				{
					Array set_array = value;
				    // todo: Godot does not expose the Object::get(StringName, bool&) method, assume valid?
				    // bool is_get_valid = false;
				    Variant get_value = resource->get(property_name);
				    if (get_value.get_type() == Variant::ARRAY)
					{
						Array get_array = get_value;
						if (!set_array.is_same_typed(get_array)) {
							value = Array(set_array, get_array.get_typed_builtin(), get_array.get_typed_class_name(), get_array.get_typed_script());
						}
					}
				}

				if (set_valid)
				    resource->set(property_name, value);
			}
			else if (!_tag.name.is_empty())
		    {
		        return _set_error(ERR_FILE_CORRUPT, "Extra tag found when parsing main resource file");
			}
			else
				break;
		}

        _parsed_internal_resources++;

        if (missing_resource)
            missing_resource->set_recording_properties(false);

        if (!missing_resource_properties.is_empty())
            resource->set_meta("metadata/_missing_resources", missing_resource_properties);

        return OK;
    }

    return _set_error(ERR_FILE_CORRUPT, "Failed to read resource tag");
}

Error OrchestrationTextParser::_parse(const String& p_path, ResourceFormatLoader::CacheMode p_cache_mode, bool p_parse_resources)
{
    _path = p_path;
    _local_path = ProjectSettings::get_singleton()->localize_path(p_path);
    _res_path = _local_path;
    _cache_mode = p_cache_mode;

    const Ref<FileAccess> file = FileAccess::open(_local_path, FileAccess::READ);
    ERR_FAIL_COND_V_MSG(!file.is_valid(), ERR_FILE_CANT_OPEN, "Failed to open file '" + p_path + "'.");

    const String source = file->get_as_text();
    OrchestrationStringStream stream(source);

    if (_parse_header(stream) == OK)
    {
        if (!p_parse_resources)
            return OK;

        if (_parse_ext_resources(stream) == OK)
        {
            if (_parse_objects(stream) == OK)
                return OK;
        }
    }

    return _error;
}

Ref<Orchestration> OrchestrationTextParser::parse(const Variant& p_source, const String& p_path, ResourceFormatLoader::CacheMode p_cache_mode)
{
    _path = p_path;
    _local_path = ProjectSettings::get_singleton()->localize_path(p_path);
    _res_path = _local_path;
    _cache_mode = ResourceFormatLoader::CACHE_MODE_IGNORE; // p_cache_mode;

    OrchestrationStringStream stream(p_source);

    if (_parse_header(stream) == OK)
    {
        if (_parse_ext_resources(stream) == OK)
        {
            if (_parse_objects(stream) == OK)
            {
                Ref<Orchestration> orchestration;
                if (_parse_resource(stream, orchestration) == OK)
                {
                    if (orchestration.is_valid())
                    {
                        if (!orchestration->has_graph("EventGraph"))
                            orchestration->create_graph("EventGraph", OScriptGraph::GraphFlags::GF_EVENT);

                        orchestration->post_initialize();
                        return orchestration;
                    }
                }
            }
        }
    }

    return {};
}

int64_t OrchestrationTextParser::get_uid(const String& p_path)
{
    if (_parse(p_path, ResourceFormatLoader::CACHE_MODE_IGNORE_DEEP, false) == OK)
        return _res_uid;

    return ResourceUID::INVALID_ID;
}

String OrchestrationTextParser::get_script_class(const String& p_path)
{
    if (_parse(p_path, ResourceFormatLoader::CACHE_MODE_IGNORE_DEEP, false) == OK)
        return _script_class;

    return "";
}

PackedStringArray OrchestrationTextParser::get_classes_used(const String& p_path)
{
    PackedStringArray classes_used;
    if (_parse(p_path, ResourceFormatLoader::CACHE_MODE_IGNORE_DEEP) == OK)
    {
        for (const KeyValue<String, Ref<Resource>>& entry : _internal_resources)
        {
            if (!classes_used.has(entry.key))
                classes_used.push_back(entry.key);
        }
    }

    return classes_used;
}

PackedStringArray OrchestrationTextParser::get_dependencies(const String& p_path, bool p_add_types)
{
    PackedStringArray dependencies;
    if (_parse(p_path, ResourceFormatLoader::CACHE_MODE_IGNORE_DEEP) == OK)
    {
        for (const KeyValue<String, ExternalResource>& entry : _external_resources)
        {
            dependencies.push_back(p_add_types
                    ? vformat("%s::%s", entry.value.path, entry.value.type)
                    : entry.value.path);
        }
    }

    return dependencies;
}

Error OrchestrationTextParser::rename_dependencies(const String& p_path, const Dictionary& p_renames)
{
    _path = p_path;
    _local_path = ProjectSettings::get_singleton()->localize_path(p_path);
    _res_path = _local_path;
    _cache_mode = ResourceFormatLoader::CACHE_MODE_IGNORE_DEEP;

    const Ref<FileAccess> file = FileAccess::open(_local_path, FileAccess::READ);
    ERR_FAIL_COND_V_MSG(!file.is_valid(), ERR_FILE_CANT_OPEN, "Failed to open file '" + p_path + "'.");

    const String source = file->get_as_text();
    OrchestrationStringStream stream(source);

    if (_parse_header(stream) == OK)
    {
        if (_parse_ext_resources(stream) == OK)
        {
            const String base_path = _local_path.get_base_dir();

            OrchestrationTextSerializer serializer;

            const Ref<FileAccess> depren = FileAccess::open(vformat("%s.depren", p_path), FileAccess::WRITE);
            if (_res_uid == ResourceUID::INVALID_ID)
                _res_uid = _get_resource_id_for_path(p_path, false);

            depren->store_line(serializer.get_start_tag(_res_type, _script_class, _total_resources, _version, _res_uid));

            for (const KeyValue<String, ExternalResource>& entry : _external_resources)
            {
                String path = entry.value.path;
                bool relative_path = false;

                if (path.begins_with("res://"))
                {
                    path = base_path.path_join(path).simplify_path();
                    relative_path = true;
                }

                if (p_renames.has(path))
                    path = p_renames[path];

                if (relative_path)
                    path = StringUtils::path_to_file(base_path, path);

                depren->store_line(serializer.get_ext_resource_tag(entry.value.type, path, entry.key, false));
            }

            const String remainder = stream.get_as_text().substr(stream.tell());
            depren->store_string(remainder);

            if (depren->get_error() == OK)
                return OK;
        }
    }

    return ERR_CANT_CREATE;
}

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
#include "script/serialization/variant_parser.h"

#define READING_SIGN 0
#define READING_INT 1
#define READING_DEC 2
#define READING_EXP 3
#define READING_DONE 4

#define MAX_RECURSION 100

#include "common/dictionary_utils.h"
#include "common/string_utils.h"

#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/templates/vector.hpp>

char32_t OScriptVariantParser::Stream::get_char()
{
    if (_readahead_pointer < _readahead_filled)
        return _readahead_buffer[_readahead_pointer++];

    _readahead_filled = _read_buffer(_readahead_buffer, _readahead_enabled ? READAHEAD_SIZE : 1);
    if (_readahead_filled)
    {
        _readahead_pointer = 0;
        return get_char();
    }
    else
    {
        _readahead_pointer = 1;
        _eof = true;
        return 0;
    }
}

bool OScriptVariantParser::Stream::is_eof() const
{
    if (_readahead_enabled)
        return _eof;

    return _is_eof();
}

bool OScriptVariantParser::StreamFile::is_utf8() const
{
    return true;
}

bool OScriptVariantParser::StreamFile::_is_eof() const
{
    return data->eof_reached();
}

uint32_t OScriptVariantParser::StreamFile::_read_buffer(char32_t* p_buffer, uint32_t p_num_chars)
{
    // Buffer is assumed to have at least 1 character for null terminator
    ERR_FAIL_COND_V(!p_num_chars, 0);

    uint8_t *temp = reinterpret_cast<uint8_t*>(alloca(p_num_chars));
    uint64_t read = data->get_buffer(temp, p_num_chars);
    ERR_FAIL_COND_V(read == UINT64_MAX, 0);

    // Convert to wchar
    for (uint32_t i = 0; i < read; i++)
        p_buffer[i] = temp[i];

    // Could be less than p_num_chars, or zero
    return read;
}

bool OScriptVariantParser::StreamString::is_utf8() const
{
    return false;
}

bool OScriptVariantParser::StreamString::_is_eof() const
{
    return _pos > _data.length();
}

uint32_t OScriptVariantParser::StreamString::_read_buffer(char32_t* p_buffer, uint32_t p_num_chars)
{
    // Buffer is assumed to have at least 1 character for null terminator
    ERR_FAIL_COND_V(!p_num_chars, 0);

    uint32_t avail = MAX(_data.length() - _pos, 0);
    if (avail >= p_num_chars)
    {
        const char32_t* src = _data.ptr();
        src += _pos;
        memcpy(p_buffer, src, p_num_chars * sizeof(char32_t));
        _pos += p_num_chars;
        return p_num_chars;
    }

    // Going to reach EOF
    if (avail)
    {
        const char32_t* src = _data.ptr();
        src += _pos;
        memcpy(p_buffer, src, avail * sizeof(char32_t));
        _pos += avail;
    }

    // Add a zero
    p_buffer[avail] = 0;

    return avail;
}

const char* OScriptVariantParser::tk_name[TK_MAX] = {
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

static double stor_fix(const String& p_str)
{
    if (p_str == "inf")
        return INFINITY;
    else if (p_str == "inf_neg")
        return -INFINITY;
    else if (p_str == "nan")
        return NAN;
    else
        return -1;
}

Error OScriptVariantParser::get_token(Stream* p_stream, int& r_line, Token& r_token, String& r_err_string)
{
    bool is_string_name{ false };
    while (true)
    {
        char32_t cchar;
        if (p_stream->saved)
        {
            cchar = p_stream->saved;
            p_stream->saved = 0;
        }
        else
        {
            cchar = p_stream->get_char();
            if (p_stream->is_eof())
            {
                r_token.type = TK_EOF;
                return OK;
            }
        }

        switch (cchar)
        {
            case '\n':
            {
                r_line++;
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
                    char32_t ch = p_stream->get_char();
                    if (p_stream->is_eof())
                    {
                        r_token.type = TK_EOF;
                        return OK;
                    }
                    if (ch == '\n')
                    {
                        r_line++;
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
                String color;
                color += '#';
                while (true)
                {
                    char32_t ch = p_stream->get_char();
                    if (p_stream->is_eof())
                    {
                        r_token.type = TK_EOF;
                        return OK;
                    }
                    else if (is_hex_digit(ch))
                    {
                        color += ch;
                    }
                    else
                    {
                        p_stream->saved = ch;
                        break;
                    }
                }
                r_token.value = Color::html(color);
                r_token.type = TK_COLOR;
                return OK;
            }
            case '&': // StringName
            {
                cchar = p_stream->get_char();
                if (cchar != '"')
                {
                    r_err_string = "Expected '\"' after '&'";
                    r_token.type = TK_ERROR;
                    return ERR_PARSE_ERROR;
                }
                is_string_name = true;
                [[fallthrough]];
            }
            case '"':
            {
                String str;
                char32_t prev = 0;
                while (true)
                {
                    char32_t ch = p_stream->get_char();
                    if (ch == 0)
                    {
                        r_err_string = "Unterminated String";
                        r_token.type = TK_ERROR;
                        return ERR_PARSE_ERROR;
                    }
                    else if (ch == '"')
                    {
                        break;
                    }
                    else if (ch == '\\')
                    {
                        // Escaped characters
                        char32_t next = p_stream->get_char();
                        if (next == 0)
                        {
                            r_err_string = "Unterminated String";
                            r_token.type = TK_ERROR;
                            return ERR_PARSE_ERROR;
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
                                    char32_t c = p_stream->get_char();

                                    if (c == 0)
                                    {
                                        r_err_string = "Unterminated String";
                                        r_token.type = TK_ERROR;
                                        return ERR_PARSE_ERROR;
                                    }
                                    if (!is_hex_digit(c))
                                    {
                                        r_err_string = "Malformed hex constant in string";
                                        r_token.type = TK_ERROR;
                                        return ERR_PARSE_ERROR;
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
                            else
                            {
                                r_err_string = "Invalid UTF-16 sequence in string, unpaired lead surrogate";
                                r_token.type = TK_ERROR;
                                return ERR_PARSE_ERROR;
                            }
                        }
                        else if ((res & 0xfffffc00) == 0xdc00)
                        {
                            if (prev == 0)
                            {
                                r_err_string = "Invalid UTF-16 sequence in string, unpaired trail surrogate";
                                r_token.type = TK_ERROR;
                                return ERR_PARSE_ERROR;
                            }
                            else
                            {
                                res = (prev << 10UL) + res - ((0xd800 << 10UL) + 0xdc00 - 0x10000);
                                prev = 0;
                            }
                        }
                        if (prev != 0)
                        {
                            r_err_string = "Invalid UTF-16 sequence in string, unpaired lead surrogate";
                            r_token.type = TK_ERROR;
                            return ERR_PARSE_ERROR;
                        }
                        str += res;
                    }
                    else
                    {
                        if (prev != 0)
                        {
                            r_err_string = "Invalid UTF-16 sequence in string, unpaired lead surrogate";
                            r_token.type = TK_ERROR;
                            return ERR_PARSE_ERROR;
                        }
                        if (ch == '\n')
                        {
                            r_line++;
                        }
                        str += ch;
                    }
                }
                if (prev != 0)
                {
                    r_err_string = "Invalid UTF-16 sequence in string, unpaired lead surrogate";
                    r_token.type = TK_ERROR;
                    return ERR_PARSE_ERROR;
                }

                if (p_stream->is_utf8())
                    str.parse_utf8(str.ascii().get_data());

                if (is_string_name)
                {
                    r_token.type = TK_STRING_NAME;
                    r_token.value = StringName(str);
                }
                else
                {
                    r_token.type = TK_STRING;
                    r_token.value = str;
                }
                return OK;
            }
            default:
            {
                if (cchar <= 32)
                    break;

                if (cchar == '-' || (cchar >= '0' && cchar <= '9'))
                {
                    // Number
                    String num;
                    int reading = READING_INT;
                    if (cchar == '-')
                    {
                        num += '-';
                        cchar = p_stream->get_char();
                    }

                    char32_t c = cchar;
                    bool exp_sign{ false };
                    bool exp_beg{ false };
                    bool is_float{ false };

                    while (true)
                    {
                        switch (reading)
                        {
                            case READING_INT:
                            {
                                if (is_digit(c))
                                {
                                    // pass
                                }
                                else if (c == '.')
                                {
                                    reading = READING_DEC;
                                    is_float = true;
                                }
                                else if (c == 'e')
                                {
                                    reading = READING_EXP;
                                    is_float = true;
                                }
                                else
                                    reading = READING_DONE;

                                break;
                            }
                            case READING_DEC:
                            {
                                if (is_digit(c))
                                {
                                    // pass
                                }
                                else if (c == 'e')
                                    reading = READING_EXP;
                                else
                                    reading = READING_DONE;
                                break;
                            }
                            case READING_EXP:
                            {
                                if (is_digit(c))
                                    exp_beg = true;
                                else if ((c == '-' || c == '+') && !exp_sign && !exp_beg)
                                    exp_sign = true;
                                else
                                    reading = READING_DONE;
                                break;
                            }
                        }

                        if (reading == READING_DONE)
                            break;

                        num += c;
                        c = p_stream->get_char();
                    }

                    p_stream->saved = c;
                    r_token.type = TK_NUMBER;
                    if (is_float)
                        r_token.value = num.to_float();
                    else
                        r_token.value = num.to_int();
                    return OK;
                }
                else if (is_ascii_char(cchar) || is_underscore(cchar))
                {
                    String id;
                    bool first{ true };

                    while (is_ascii_char(cchar) || is_underscore(cchar) || (!first && is_digit(cchar)))
                    {
                        id += cchar;
                        cchar = p_stream->get_char();
                        first = false;
                    }

                    p_stream->saved = cchar;
                    r_token.type = TK_IDENTIFIER;
                    r_token.value = id;
                    return OK;
                }
                else
                {
                    r_err_string = "Unexpected character.";
                    r_token.type = TK_ERROR;
                    return ERR_PARSE_ERROR;
                }
            }
        }
    }

    r_err_string = "Failed to find token";
    r_token.type = TK_ERROR;
    return ERR_PARSE_ERROR;
}

Error OScriptVariantParser::_parse_enginecfg(Stream* p_stream, Vector<String>& p_strings, int& p_line, String& r_err_string)
{
    Token token;
    get_token(p_stream, p_line, token, r_err_string);

    if (token.type != TK_PARENTHESIS_OPEN)
    {
        r_err_string = "Expected '(' in old-style project.godot construct";
        return ERR_PARSE_ERROR;
    }

    String accum;
    while (true)
    {
        char32_t c = p_stream->get_char();

        if (p_stream->is_eof())
        {
            r_err_string = "Unexpected EOF while parsing old-style project.godot construct";
            return ERR_PARSE_ERROR;
        }

        if (c == ',')
        {
            p_strings.push_back(accum.strip_edges());
            accum = String();
        }
        else if (c == ')')
        {
            p_strings.push_back(accum.strip_edges());
            return OK;
        }
        else if (c == '\n')
            p_line++;
    }
}

template <typename T>
Error OScriptVariantParser::_parse_construct(Stream* p_stream, Vector<T>& r_construct, int& p_line, String& r_err_string)
{
    Token token;
    get_token(p_stream, p_line, token, r_err_string);

    if (token.type != TK_PARENTHESIS_OPEN)
    {
        r_err_string = "Expected '(' in constructor";
        return ERR_PARSE_ERROR;
    }

    bool first = true;
    while (true)
    {
        if (!first)
        {
            get_token(p_stream, p_line, token, r_err_string);
            if (token.type == TK_COMMA)
            {
                //do none
            }
            else if (token.type == TK_PARENTHESIS_CLOSE)
            {
                break;
            }
            else
            {
                r_err_string = "Expected ',' or ')' in constructor";
                return ERR_PARSE_ERROR;
            }
        }

        get_token(p_stream, p_line, token, r_err_string);

        if (first && token.type == TK_PARENTHESIS_CLOSE)
        {
            break;
        }
        else if (token.type != TK_NUMBER)
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
            {
                r_err_string = "Expected float in constructor";
                return ERR_PARSE_ERROR;
            }
        }

        r_construct.push_back(token.value);
        first = false;
    }

    return OK;
}

Error OScriptVariantParser::parse_value(Stream* p_stream, Token& p_token, int& p_line, Variant& r_value, String& r_err_string, ResourceParser* p_res_parser)
{
    if (p_token.type == TK_CURLY_BRACKET_OPEN)
    {
        Dictionary dict;
        if (const Error err = _parse_dictionary(p_stream, dict, p_line, r_err_string, p_res_parser))
            return err;

        r_value = dict;
        return OK;
    }
    else if (p_token.type == TK_BRACKET_OPEN)
    {
        Array array;
        if (const Error err = _parse_array(p_stream, array, p_line, r_err_string, p_res_parser))
            return err;

        r_value = array;
        return OK;
    }
    else if (p_token.type == TK_IDENTIFIER)
    {
        String id = p_token.value;
        if (id == "true")
            r_value = true;
        else if (id == "false")
            r_value = false;
        else if (id == "null" || id == "nil")
            r_value = Variant();
        else if (id == "inf")
            r_value = INFINITY;
        else if (id == "inf_neg")
            r_value = -INFINITY;
        else if (id == "nan")
            r_value = NAN;
        else if (id == "Vector2")
        {
            Vector<real_t> args;
            if (const Error err = _parse_construct<real_t>(p_stream, args, p_line, r_err_string))
                return err;

            if (args.size() != 2)
            {
                r_err_string = "Expected 2 arguments for constructor";
                return ERR_PARSE_ERROR;
            }
            r_value = Vector2(args[0], args[1]);
        }
        else if (id == "Vector2i")
        {
            Vector<int32_t> args;
            if (const Error err = _parse_construct<int32_t>(p_stream, args, p_line, r_err_string))
                return err;

            if (args.size() != 2)
            {
                r_err_string = "Expected 2 arguments for constructor";
                return ERR_PARSE_ERROR;
            }
            r_value = Vector2i(args[0], args[1]);
        }
        else if (id == "Rect2")
        {
            Vector<real_t> args;
            if (const Error err = _parse_construct<real_t>(p_stream, args, p_line, r_err_string))
                return err;

            if (args.size() != 4)
            {
                r_err_string = "Expected 4 arguments for constructor";
                return ERR_PARSE_ERROR;
            }
            r_value = Rect2(args[0], args[1], args[2], args[3]);
        }
        else if (id == "Rect2i")
        {
            Vector<int32_t> args;
            if (const Error err = _parse_construct<int32_t>(p_stream, args, p_line, r_err_string))
                return err;

            if (args.size() != 4)
            {
                r_err_string = "Expected 4 arguments for constructor";
                return ERR_PARSE_ERROR;
            }
            r_value = Rect2i(args[0], args[1], args[2], args[3]);
        }
        else if (id == "Vector3")
        {
            Vector<real_t> args;
            if (const Error err = _parse_construct<real_t>(p_stream, args, p_line, r_err_string))
                return err;

            if (args.size() != 3)
            {
                r_err_string = "Expected 3 arguments for constructor";
                return ERR_PARSE_ERROR;
            }
            r_value = Vector3(args[0], args[1], args[2]);
        }
        else if (id == "Vector3i")
        {
            Vector<int32_t> args;
            if (const Error err = _parse_construct<int32_t>(p_stream, args, p_line, r_err_string))
                return err;

            if (args.size() != 3)
            {
                r_err_string = "Expected 3 arguments for constructor";
                return ERR_PARSE_ERROR;
            }
            r_value = Vector3i(args[0], args[1], args[2]);
        }
        else if (id == "Vector4")
        {
            Vector<real_t> args;
            if (const Error err = _parse_construct<real_t>(p_stream, args, p_line, r_err_string))
                return err;

            if (args.size() != 4)
            {
                r_err_string = "Expected 4 arguments for constructor";
                return ERR_PARSE_ERROR;
            }
            r_value = Vector4(args[0], args[1], args[2], args[3]);
        }
        else if (id == "Vector4i")
        {
            Vector<int32_t> args;
            if (const Error err = _parse_construct<int32_t>(p_stream, args, p_line, r_err_string))
                return err;

            if (args.size() != 4)
            {
                r_err_string = "Expected 4 arguments for constructor";
                return ERR_PARSE_ERROR;
            }
            r_value = Vector4i(args[0], args[1], args[2], args[3]);
        }
        else if (id == "Transform2D" || id == "Matrix32")
        {
            Vector<real_t> args;
            if (const Error err = _parse_construct<real_t>(p_stream, args, p_line, r_err_string))
                return err;

            if (args.size() != 6)
            {
                r_err_string = "Expected 6 arguments for constructor";
                return ERR_PARSE_ERROR;
            }

            Transform2D m;
            m[0] = Vector2(args[0], args[1]);
            m[1] = Vector2(args[2], args[3]);
            m[2] = Vector2(args[4], args[5]);
            r_value = m;
        }
        else if (id == "Plane")
        {
            Vector<real_t> args;
            if (const Error err = _parse_construct<real_t>(p_stream, args, p_line, r_err_string))
                return err;

            if (args.size() != 4)
            {
                r_err_string = "Expected 4 arguments for constructor";
                return ERR_PARSE_ERROR;
            }
            r_value = Plane(args[0], args[1], args[2], args[3]);
        }
        else if (id == "Quaternion" || id == "Quat")
        {
            Vector<real_t> args;
            if (const Error err = _parse_construct<real_t>(p_stream, args, p_line, r_err_string))
                return err;

            if (args.size() != 4)
            {
                r_err_string = "Expected 4 arguments for constructor";
                return ERR_PARSE_ERROR;
            }
            r_value = Quaternion(args[0], args[1], args[2], args[3]);
        }
        else if (id == "AABB" || id == "Rect3")
        {
            Vector<real_t> args;
            if (const Error err = _parse_construct<real_t>(p_stream, args, p_line, r_err_string))
                return err;

            if (args.size() != 6)
            {
                r_err_string = "Expected 6 arguments for constructor";
                return ERR_PARSE_ERROR;
            }
            r_value = AABB(Vector3(args[0], args[1], args[2]), Vector3(args[3], args[4], args[5]));
        }
        else if (id == "Basis" || id == "Matrix3")
        {
            Vector<real_t> args;
            if (const Error err = _parse_construct<real_t>(p_stream, args, p_line, r_err_string))
                return err;

            if (args.size() != 9)
            {
                r_err_string = "Expected 9 arguments for constructor";
                return ERR_PARSE_ERROR;
            }
            r_value = Basis(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
        }
        else if (id == "Transform3D" || id == "Transform")
        {
            Vector<real_t> args;
            if (const Error err = _parse_construct<real_t>(p_stream, args, p_line, r_err_string))
                return err;

            if (args.size() != 12)
            {
                r_err_string = "Expected 12 arguments for constructor";
                return ERR_PARSE_ERROR;
            }
            r_value = Transform3D(
                Basis(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]),
                Vector3(args[9], args[10], args[11]));
        }
        else if (id == "Projection")
        {
            Vector<real_t> args;
            if (const Error err = _parse_construct<real_t>(p_stream, args, p_line, r_err_string))
                return err;

            if (args.size() != 16)
            {
                r_err_string = "Expected 16 arguments for constructor";
                return ERR_PARSE_ERROR;
            }
            r_value = Projection(
                Vector4(args[0], args[1], args[2], args[3]),
                Vector4(args[4], args[5], args[6], args[7]),
                Vector4(args[8], args[9], args[10], args[11]),
                Vector4(args[12], args[13], args[14], args[15]));;
        }
        else if (id == "Color")
        {
            Vector<float> args;
            if (const Error err = _parse_construct<float>(p_stream, args, p_line, r_err_string))
                return err;

            if (args.size() != 4)
            {
                r_err_string = "Expected 4 arguments for constructor";
                return ERR_PARSE_ERROR;
            }
            r_value = Color(args[0], args[1], args[2], args[3]);
        }
        else if (id == "NodePath")
        {
            get_token(p_stream, p_line, p_token, r_err_string);
            if (p_token.type != TK_PARENTHESIS_OPEN)
            {
                r_err_string = "Expected '('";
                return ERR_PARSE_ERROR;
            }
            get_token(p_stream, p_line, p_token, r_err_string);
            if (p_token.type != TK_STRING)
            {
                r_err_string = "Expected string as argument for NodePath()";
                return ERR_PARSE_ERROR;
            }
            r_value = NodePath(String(p_token.value));
            get_token(p_stream, p_line, p_token, r_err_string);
            if (p_token.type != TK_PARENTHESIS_CLOSE)
            {
                r_err_string = "Expected ')'";
                return ERR_PARSE_ERROR;
            }
        }
        else if (id == "RID")
        {
            get_token(p_stream, p_line, p_token, r_err_string);
            if (p_token.type != TK_PARENTHESIS_OPEN)
            {
                r_err_string = "Expected '('";
                return ERR_PARSE_ERROR;
            }
            get_token(p_stream, p_line, p_token, r_err_string);
            // Permit empty RID
            if (p_token.type == TK_PARENTHESIS_CLOSE)
            {
                r_value = RID();
                return OK;
            }
            else if (p_token.type != TK_STRING)
            {
                r_err_string = "Expected number as argument or ')'";
                return ERR_PARSE_ERROR;
            }
            // Cannot construct RID
            r_value = RID();
            get_token(p_stream, p_line, p_token, r_err_string);
            if (p_token.type != TK_PARENTHESIS_CLOSE)
            {
                r_err_string = "Expected ')'";
                return ERR_PARSE_ERROR;
            }
        }
        else if (id == "Signal")
        {
            get_token(p_stream, p_line, p_token, r_err_string);
            if (p_token.type != TK_PARENTHESIS_OPEN)
            {
                r_err_string = "Expected '('";
                return ERR_PARSE_ERROR;
            }
            // Load as empty
            r_value = Signal();
            get_token(p_stream, p_line, p_token, r_err_string);
            if (p_token.type != TK_PARENTHESIS_CLOSE)
            {
                r_err_string = "Expected ')'";
                return ERR_PARSE_ERROR;
            }
        }
        else if (id == "Callable")
        {
            get_token(p_stream, p_line, p_token, r_err_string);
            if (p_token.type != TK_PARENTHESIS_OPEN)
            {
                r_err_string = "Expected '('";
                return ERR_PARSE_ERROR;
            }
            // Load as empty
            r_value = Callable();
            get_token(p_stream, p_line, p_token, r_err_string);
            if (p_token.type != TK_PARENTHESIS_CLOSE)
            {
                r_err_string = "Expected ')'";
                return ERR_PARSE_ERROR;
            }
        }
        else if (id == "Object")
        {
            get_token(p_stream, p_line, p_token, r_err_string);
            if (p_token.type != TK_PARENTHESIS_OPEN)
            {
                r_err_string = "Expected '('";
                return ERR_PARSE_ERROR;
            }

            get_token(p_stream, p_line, p_token, r_err_string);
            if (p_token.type != TK_IDENTIFIER)
            {
                r_err_string = "Expected identifier with type of object";
                return ERR_PARSE_ERROR;
            }

            String type = p_token.value;
            if (!ClassDB::can_instantiate(type))
            {
                r_err_string = "Expected a constructable type, cannot construct '" + type + "'.";
                return ERR_PARSE_ERROR;
            }

            Object* obj = ClassDB::instantiate(type);
            if (!obj)
            {
                r_err_string = "Cannot instantiate Object() of type: " + type;
                return ERR_PARSE_ERROR;
            }

            Ref<RefCounted> ref = Object::cast_to<RefCounted>(obj);

            get_token(p_stream, p_line, p_token, r_err_string);
            if (p_token.type != TK_COMMA)
            {
                r_err_string = "Expected ',' after object type";
                return ERR_PARSE_ERROR;
            }

            bool at_key{ true };
            bool need_comma{ false };
            String key;

            while (true)
            {
                if (p_stream->is_eof())
                {
                    r_err_string = "Unexpected EOF while parsing Object()";
                    return ERR_FILE_CORRUPT;
                }
                if (at_key)
                {
                    if (const Error err = get_token(p_stream, p_line, p_token, r_err_string))
                        return err;

                    if (p_token.type == TK_PARENTHESIS_CLOSE)
                    {
                        r_value = ref.is_valid() ? Variant(ref) : Variant(obj);
                        return OK;
                    }

                    if (need_comma)
                    {
                        if (p_token.type != TK_COMMA)
                        {
                            r_err_string = "Expected '}' or ','";
                            return ERR_PARSE_ERROR;
                        }
                        else
                        {
                            need_comma = false;
                            continue;
                        }
                    }

                    if (p_token.type != TK_STRING)
                    {
                        r_err_string = "Expected property name as string";
                        return ERR_PARSE_ERROR;
                    }

                    key = p_token.value;

                    if (const Error err = get_token(p_stream, p_line, p_token, r_err_string))
                        return err;

                    if (p_token.type != TK_COLON)
                    {
                        r_err_string = "Expected ':'";
                        return ERR_PARSE_ERROR;
                    }
                    at_key = false;
                }
                else
                {
                    if (const Error err = get_token(p_stream, p_line, p_token, r_err_string))
                        return err;

                    Variant v;
                    if (const Error err = parse_value(p_stream, p_token, p_line, v, r_err_string))
                        return err;

                    obj->set(key, v);
                    need_comma = true;
                    at_key = true;
                }
            }
        }
        else if (id == "Resource" || id == "SubResource" || id == "ExtResource")
        {
            get_token(p_stream, p_line, p_token, r_err_string);
            if (p_token.type != TK_PARENTHESIS_OPEN)
            {
                r_err_string = "Expected '('";
                return ERR_PARSE_ERROR;
            }

            if (p_res_parser && id == "Resource" && p_res_parser->func)
            {
                Ref<Resource> res;
                if (const Error err = p_res_parser->func(p_res_parser->userdata, p_stream, res, p_line, r_err_string))
                    return err;
                r_value = res;
            }
            else if (p_res_parser && id == "ExtResource" && p_res_parser->external_func)
            {
                Ref<Resource> res;
                if (const Error err = p_res_parser->external_func(p_res_parser->userdata, p_stream, res, p_line, r_err_string))
                {
                    // If the file is missing, can safely be ignored
                    if (err != ERR_FILE_NOT_FOUND && err != ERR_CANT_OPEN)
                        return err;
                }
                r_value = res;
            }
            else if (p_res_parser && id == "SubResource" && p_res_parser->subres_func)
            {
                Ref<Resource> res;
                if (const Error err = p_res_parser->subres_func(p_res_parser->userdata, p_stream, res, p_line, r_err_string))
                    return err;
                r_value = res;
            }
            else
            {
                get_token(p_stream, p_line, p_token, r_err_string);
                if (p_token.type == TK_STRING)
                {
                    String path = p_token.value;
                    Ref<Resource> res = ResourceLoader::get_singleton()->load(path);
                    if (res.is_null())
                    {
                        r_err_string = "Cannot load resource at path: '" + path + "'.";
                        return ERR_PARSE_ERROR;
                    }

                    get_token(p_stream, p_line, p_token, r_err_string);
                    if (p_token.type != TK_PARENTHESIS_CLOSE)
                    {
                        r_err_string = "Expected ')'";
                        return ERR_PARSE_ERROR;
                    }
                    r_value = res;
                }
                else
                {
                    r_err_string = "Expected string as argument for Resource().";
                    return ERR_PARSE_ERROR;
                }
            }
        }
        else if (id == "Array")
        {
            get_token(p_stream, p_line, p_token, r_err_string);
            if (p_token.type != TK_BRACKET_OPEN)
            {
                r_err_string = "Expected '['";
                return ERR_PARSE_ERROR;
            }

            get_token(p_stream, p_line, p_token, r_err_string);
            if (p_token.type != TK_IDENTIFIER)
            {
                r_err_string = "Expected type identifier";
                return ERR_PARSE_ERROR;
            }

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
                if (const Error err = parse_value(p_stream, p_token, p_line, resource, r_err_string, p_res_parser))
                {
                    if (p_token.value == Variant("Resource") && err == ERR_PARSE_ERROR && r_err_string == "Expected '('" && p_token.type == TK_BRACKET_CLOSE)
                    {
                        r_err_string = String();
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
                get_token(p_stream, p_line, p_token, r_err_string);
                if (p_token.type != TK_BRACKET_CLOSE)
                {
                    r_err_string = "Expected ']'";
                    return ERR_PARSE_ERROR;
                }
            }

            get_token(p_stream, p_line, p_token, r_err_string);
            if (p_token.type != TK_PARENTHESIS_OPEN)
            {
                r_err_string = "Expected '('";
                return ERR_PARSE_ERROR;
            }

            get_token(p_stream, p_line, p_token, r_err_string);
            if (p_token.type != TK_BRACKET_OPEN)
            {
                r_err_string = "Expected '['";
                return ERR_PARSE_ERROR;
            }

            Array values;
            if (const Error err = _parse_array(p_stream, values, p_line, r_err_string, p_res_parser))
                return err;

            get_token(p_stream, p_line, p_token, r_err_string);
            if (p_token.type != TK_PARENTHESIS_CLOSE)
            {
                r_err_string = "Expected ')'";
                return ERR_PARSE_ERROR;
            }

            array.assign(values);
            r_value = array;
        }
        else if (id == "PackedByteArray")
        {
            Vector<uint8_t> args;
            if (const Error err = _parse_construct<uint8_t>(p_stream, args, p_line, r_err_string))
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
            if (const Error err = _parse_construct<int32_t>(p_stream, args, p_line, r_err_string))
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
            if (const Error err = _parse_construct<int64_t>(p_stream, args, p_line, r_err_string))
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
            if (const Error err = _parse_construct<float>(p_stream, args, p_line, r_err_string))
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
            if (const Error err = _parse_construct<double>(p_stream, args, p_line, r_err_string))
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
            get_token(p_stream, p_line, p_token, r_err_string);
            if (p_token.type != TK_PARENTHESIS_OPEN)
            {
                r_err_string = "Expected '('";
                return ERR_PARSE_ERROR;
            }

            bool first{ true };
            Vector<String> vs;
            while (true)
            {
                if (!first)
                {
                    get_token(p_stream, p_line, p_token, r_err_string);
                    if (p_token.type == TK_COMMA)
                    {
                        // do nothing
                    }
                    else if (p_token.type == TK_PARENTHESIS_CLOSE)
                    {
                        break;
                    }
                    else
                    {
                        r_err_string = "Expected ',' or ')'";
                        return ERR_PARSE_ERROR;
                    }
                }

                get_token(p_stream, p_line, p_token, r_err_string);
                if (p_token.type == TK_PARENTHESIS_CLOSE)
                {
                    break;
                }
                else if (p_token.type != TK_STRING)
                {
                    r_err_string = "Expected string";
                    return ERR_PARSE_ERROR;
                }

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
            if (const Error err = _parse_construct<real_t>(p_stream, args, p_line, r_err_string))
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
            if (const Error err = _parse_construct<real_t>(p_stream, args, p_line, r_err_string))
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
            if (const Error err = _parse_construct<real_t>(p_stream, args, p_line, r_err_string))
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
        else
        {
            r_err_string = "Unknown identifier: '" + id + "'.";
            return ERR_PARSE_ERROR;
        }

        // All above branches end up here unless they returned early
        return OK;
    }
    else if (p_token.type == TK_NUMBER)
    {
        r_value = p_token.value;
        return OK;
    }
    else if (p_token.type == TK_STRING)
    {
        r_value = p_token.value;
        return OK;
    }
    else if (p_token.type == TK_STRING_NAME)
    {
        r_value = p_token.value;
        return OK;
    }
    else if (p_token.type == TK_COLOR)
    {
        r_value = p_token.value;
        return OK;
    }
    else
    {
        r_err_string = "Expected value, got " + String(tk_name[p_token.type]) + ".";
        return ERR_PARSE_ERROR;
    }
}

Error OScriptVariantParser::_parse_array(Stream* p_stream, Array& p_array, int& p_line, String& r_err_string, ResourceParser* p_res_parser)
{
    Token token;
    bool need_comma{ false };
    while (true)
    {
        if (p_stream->is_eof())
        {
            r_err_string = "Unexpected EOF while parsing array";
            return ERR_FILE_CORRUPT;
        }

        if (const Error err = get_token(p_stream, p_line, token, r_err_string))
            return err;

        if (token.type == TK_BRACKET_CLOSE)
            return OK;

        if (need_comma)
        {
            if (token.type != TK_COMMA)
            {
                r_err_string = "Expected ','";
                return ERR_PARSE_ERROR;
            }
            else
            {
                need_comma = false;
                continue;
            }
        }

        Variant v;
        if (const Error err = parse_value(p_stream, token, p_line, v, r_err_string, p_res_parser))
            return err;

        p_array.push_back(v);
        need_comma = true;
    }
}

Error OScriptVariantParser::_parse_dictionary(Stream* p_stream, Dictionary& p_object, int& p_line, String& r_err_string, ResourceParser* p_res_parser)
{
    Token token;
    Variant key;
    bool at_key{ true };
    bool need_comma{ false };
    while (true)
    {
        if (p_stream->is_eof())
        {
            r_err_string = "Unexpected EOF while parsing dictionary";
            return ERR_FILE_CORRUPT;
        }

        if (at_key)
        {
            Error err = get_token(p_stream, p_line, token, r_err_string);
            if (err)
                return err;

            if (token.type == TK_CURLY_BRACKET_CLOSE)
                return OK;

            if (need_comma)
            {
                if (token.type != TK_COMMA)
                {
                    r_err_string = "Expected '}' or ','";
                    return ERR_PARSE_ERROR;
                }
                else
                {
                    need_comma = false;
                    continue;
                }
            }

            err = parse_value(p_stream, token, p_line, key, r_err_string, p_res_parser);
            if (err)
                return err;

            err = get_token(p_stream, p_line, token, r_err_string);
            if (err)
                return err;

            if (token.type != TK_COLON)
            {
                r_err_string = "Expected ':'";
                return ERR_PARSE_ERROR;
            }
            at_key = false;
        }
        else
        {
            Error err = get_token(p_stream, p_line, token, r_err_string);
            if (err)
                return err;

            Variant v;
            err = parse_value(p_stream, token, p_line, v, r_err_string, p_res_parser);
            if (err && err != ERR_FILE_MISSING_DEPENDENCIES)
                return err;

            p_object[key] = v;
            need_comma = true;
            at_key = true;
        }
    }
}

Error OScriptVariantParser::_parse_tag(Stream* p_stream, Token& p_token, int& p_line, String& r_err_string, Tag& r_tag, ResourceParser* p_res_parser, bool p_simple_tag)
{
    r_tag.fields.clear();

    if (p_token.type != TK_BRACKET_OPEN)
    {
        r_err_string = "Expected '['";
        return ERR_PARSE_ERROR;
    }

    if (p_simple_tag)
    {
        r_tag.name = "";
        r_tag.fields.clear();
        bool escaping{ false };

        if (p_stream->is_utf8())
        {
            CharString cs;
            while (true)
            {
                char c = p_stream->get_char();
                if (p_stream->is_eof())
                {
                    r_err_string = "Unexpected EOF while parsing simple tag";
                    return ERR_FILE_CORRUPT;
                }
                if (c == ']')
                {
                    if (escaping)
                        escaping = false;
                    else
                        break;
                }
                else if (c == '\\')
                    escaping = true;
                else
                    escaping = false;

                cs += c;
            }
            r_tag.name.parse_utf8(cs.get_data(), cs.length());
        }
        else
        {
            while (true)
            {
                char32_t c = p_stream->get_char();
                if (p_stream->is_eof())
                {
                    r_err_string = "Unexpected EOF while parsing simple tag";
                    return ERR_FILE_CORRUPT;
                }
                if (c == ']')
                {
                    if (escaping)
                        escaping = false;
                    else
                        break;
                }
                else if (c == '\\')
                    escaping = true;
                else
                    escaping = false;

                r_tag.name += String::chr(c);
            }
        }
        r_tag.name = r_tag.name.strip_edges();
        return OK;
    }

    get_token(p_stream, p_line, p_token, r_err_string);
    if (p_token.type != TK_IDENTIFIER)
    {
        r_err_string = "Expected identifier (tag name)";
        return ERR_PARSE_ERROR;
    }

    r_tag.name = p_token.value;
    bool parsing_tag{ true };
    while (true)
    {
        if (p_stream->is_eof())
        {
            r_err_string = "Unexpected EOF while parsing tag: " + r_tag.name;
            return ERR_FILE_CORRUPT;
        }

        get_token(p_stream, p_line, p_token, r_err_string);
        if (p_token.type == TK_BRACKET_CLOSE)
            break;

        if (parsing_tag && p_token.type == TK_PERIOD)
        {
            r_tag.name += "."; // supports tags like [someprop.Android] for platforms
            get_token(p_stream, p_line, p_token, r_err_string);
        }
        else if (parsing_tag && p_token.type == TK_COLON)
        {
            r_tag.name += ":"; // supports tags like [someprops:Anroid] for platforms
            get_token(p_stream, p_line, p_token, r_err_string);
        }
        else
            parsing_tag = false;

        if (p_token.type != TK_IDENTIFIER)
        {
            r_err_string = "Expected identifier";
            return ERR_PARSE_ERROR;
        }

        String id = p_token.value;
        if (parsing_tag)
        {
            r_tag.name += id;
            continue;
        }

        get_token(p_stream, p_line, p_token, r_err_string);
        if (p_token.type != TK_EQUAL)
        {
            r_err_string = "Expected '='";
            return ERR_PARSE_ERROR;
        }

        get_token(p_stream, p_line, p_token, r_err_string);

        Variant value;
        if (const Error err = parse_value(p_stream, p_token, p_line, value, r_err_string, p_res_parser))
            return err;

        r_tag.fields[id] = value;
    }

    return OK;
}

Error OScriptVariantParser::parse_tag(Stream* p_stream, int& p_line, Tag& r_tag, String& r_err_string, ResourceParser* p_res_parser, bool p_simple_tag)
{
    Token token;

    get_token(p_stream, p_line, token, r_err_string);
    if (token.type == TK_EOF)
        return ERR_FILE_EOF;

    if (token.type != TK_BRACKET_OPEN)
    {
        r_err_string = "Expected '['";
        return ERR_PARSE_ERROR;
    }

    return _parse_tag(p_stream, token, p_line, r_err_string, r_tag, p_res_parser, p_simple_tag);
}

Error OScriptVariantParser::parse_tag_assign_eof(Stream* p_stream, int& p_line, String& r_err_string, Tag& r_tag, String& r_assign, Variant& r_value, ResourceParser* p_res_parser, bool p_simple_tag)
{
    r_assign = "";
    String what;

    while (true)
    {
        char32_t c;
        if (p_stream->saved)
        {
            c = p_stream->saved;
            p_stream->saved = 0;
        }
        else
            c = p_stream->get_char();

        if (p_stream->is_eof())
            return ERR_FILE_EOF;

        if (c == ';') // Comments
        {
            while (true)
            {
                char32_t ch = p_stream->get_char();
                if (p_stream->is_eof())
                    return ERR_FILE_EOF;

                if (ch == '\n')
                {
                    p_line++;
                    break;
                }
            }
            continue;
        }

        if (c == '[' && what.length() == 0)
        {
            // Tag detected
            p_stream->saved = '['; // push it back
            return parse_tag(p_stream, p_line, r_tag, r_err_string, p_res_parser, p_simple_tag);
        }

        if (c > 32)
        {
            if (c == '"') // Quoted
            {
                p_stream->saved = '"';

                Token tk;
                if (const Error err = get_token(p_stream, p_line, tk, r_err_string))
                    return err;

                if (tk.type != TK_STRING)
                {
                    r_err_string = "Error reading quoted string";
                    return ERR_INVALID_DATA;
                }
                what = tk.value;
            }
            else if (c != '=')
            {
                what += String::chr(c);
            }
            else
            {
                r_assign = what;

                Token token;
                get_token(p_stream, p_line, token, r_err_string);
                return parse_value(p_stream, token, p_line, r_value, r_err_string, p_res_parser);
            }
        }
        else if (c == '\n')
            p_line++;
    }
}

Error OScriptVariantParser::parse(Stream* p_stream, Variant& r_ret, String& r_err_string, int& r_err_line, ResourceParser* p_res_parser)
{
    Token token;
    if (const Error err = get_token(p_stream, r_err_line, token, r_err_string))
        return err;

    if (token.type == TK_EOF)
        return ERR_FILE_EOF;

    return parse_value(p_stream, token, r_err_line, r_ret, r_err_string, p_res_parser);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static String rtos_fix(double p_value)
{
    if (p_value == 0.0)
        return "0"; // Avoid negative (-0) written
    else if (std::isnan(p_value))
        return "nan";
    else if (std::isinf(p_value))
    {
        if (p_value > 0)
            return "inf";
        else
            return "inf_neg";
    }

    return rtoss(p_value);
}

bool OScriptVariantWriter::_is_resource_file(const String& p_path)
{
    return p_path.begins_with("res://") && p_path.find("::") == -1;
}

Error OScriptVariantWriter::write(const Variant& p_variant, StoreStringFunction p_store_string, void* p_store_userdata, EncodeResourceFunction p_encode_resource, void* p_encode_userdata, int p_recursion_count)
{
    switch (p_variant.get_type())
    {
        case Variant::NIL:
        {
            p_store_string(p_store_userdata, "null");
            break;
        }
        case Variant::BOOL:
        {
            p_store_string(p_store_userdata, p_variant.operator bool() ? "true" : "false");
            break;
        }
        case Variant::INT:
        {
            p_store_string(p_store_userdata, itos(p_variant.operator int64_t()));
            break;
        }
        case Variant::FLOAT:
        {
            String s = rtos_fix(p_variant.operator double());
            if (s != "inf" && s != "inf_neg" && s != "nan")
                if (!s.contains(".") && !s.contains("e"))
                    s += ".0";
            p_store_string(p_store_userdata, s);
            break;
        }
        case Variant::STRING:
        {
            String str = p_variant;
            str = "\"" + StringUtils::c_escape_multiline(str) + "\"";
            p_store_string(p_store_userdata, str);
            break;
        }
        case Variant::VECTOR2:
        {
            const Vector2 v = p_variant;
            p_store_string(p_store_userdata, "Vector2(" + rtos_fix(v.x) + ", " + rtos_fix(v.y) + ")");
            break;
        }
        case Variant::VECTOR2I:
        {
            const Vector2i v = p_variant;
            p_store_string(p_store_userdata, "Vector2i(" + itos(v.x) + ", " + itos(v.y) + ")");
            break;
        }
        case Variant::RECT2:
        {
            const Rect2 r = p_variant;
            p_store_string(p_store_userdata, "Rect2(" + rtos_fix(r.position.x) + ", " + rtos_fix(r.position.y) + ", " + rtos_fix(r.size.x) + ", " + rtos_fix(r.size.y) + ")");
            break;
        }
        case Variant::RECT2I:
        {
            const Rect2i r = p_variant;
            p_store_string(p_store_userdata, "Rect2i(" + itos(r.position.x) + ", " + itos(r.position.y) + ", " + itos(r.size.x) + ", " + itos(r.size.y) + ")");
            break;
        }
        case Variant::VECTOR3:
        {
            const Vector3 v = p_variant;
            p_store_string(p_store_userdata, "Vector3(" + rtos_fix(v.x) + ", " + rtos_fix(v.y) + ", " + rtos_fix(v.z) + ")");
            break;
        }
        case Variant::VECTOR3I:
        {
            const Vector3i v = p_variant;
            p_store_string(p_store_userdata, "Vector3i(" + itos(v.x) + ", " + itos(v.y) + ", " + itos(v.z) + ")");
            break;
        }
        case Variant::VECTOR4:
        {
            const Vector4 v = p_variant;
            p_store_string(p_store_userdata, "Vector4(" + rtos_fix(v.x) + ", " + rtos_fix(v.y) + ", " + rtos_fix(v.z) + ", " + rtos_fix(v.w) + ")");
            break;
        }
        case Variant::VECTOR4I:
        {
            const Vector4i v = p_variant;
            p_store_string(p_store_userdata, "Vector4i(" + itos(v.x) + ", " + itos(v.y) + ", " + itos(v.z) + ", " + itos(v.w) + ")");
            break;
        }
        case Variant::PLANE:
        {
            const Plane p = p_variant;
            p_store_string(p_store_userdata, "Plane(" + rtos_fix(p.normal.x) + ", " + rtos_fix(p.normal.y) + ", " + rtos_fix(p.normal.z) + ", " + rtos_fix(p.d) + ")");
            break;
        }
        case Variant::AABB:
        {
            const AABB aabb = p_variant;
            p_store_string(p_store_userdata, "AABB(" + rtos_fix(aabb.position.x) + ", " + rtos_fix(aabb.position.y) + ", " + rtos_fix(aabb.position.z) + ", " + rtos_fix(aabb.size.x) + ", " + rtos_fix(aabb.size.y) + ", " + rtos_fix(aabb.size.z) + ")");
            break;
        }
        case Variant::QUATERNION:
        {
            const Quaternion q = p_variant;
            p_store_string(p_store_userdata, "Quaternion(" + rtos_fix(q.x) + ", " + rtos_fix(q.y) + ", " + rtos_fix(q.z) + ", " + rtos_fix(q.w) + ")");
            break;
        }
        case Variant::TRANSFORM2D:
        {
            String s = "Transform2D(";
            const Transform2D t = p_variant;
            for (int i = 0; i < 3; i++)
            {
                for (int j = 0; j < 2; j++)
                {
                    if (i != 0 || j != 0)
                        s += ", ";
                    s += rtos_fix(t.columns[i][j]);
                }
            }
            p_store_string(p_store_userdata, s + ")");
            break;
        }
        case Variant::BASIS:
        {
            String s = "Basis(";
            const Basis b = p_variant;
            for (int i = 0; i < 3; i++)
            {
                for (int j = 0; j < 3; j++)
                {
                    if (i != 0 || j != 0)
                        s += ", ";
                    s += rtos_fix(b.rows[i][j]);
                }
            }
            p_store_string(p_store_userdata, s + ")");
            break;
        }
        case Variant::TRANSFORM3D:
        {
            String s = "Transform3D(";
            const Transform3D t = p_variant;
            const Basis& m3 = t.basis;
            for (int i = 0; i < 3; i++)
            {
                for (int j = 0; j < 3; j++)
                {
                    if (i != 0 || j != 0)
                        s += ", ";
                    s += rtos_fix(m3.rows[i][j]);
                }
            }
            s = s + ", " + rtos_fix(t.origin.x) + ", " + rtos_fix(t.origin.y) + ", " + rtos_fix(t.origin.z);
            p_store_string(p_store_userdata, s + ")");
            break;
        }
        case Variant::PROJECTION:
        {
            String s = "Projection(";
            const Projection p = p_variant;
            for (int i = 0; i < 4; i++)
            {
                for (int j = 0; j < 4; j++)
                {
                    if (i != 0 || j != 0)
                        s += ", ";
                    s += rtos_fix(p.columns[i][j]);
                }
            }
            p_store_string(p_store_userdata, s + ")");
            break;
        }
        case Variant::COLOR:
        {
            const Color c = p_variant;
            p_store_string(p_store_userdata, "Color(" + rtos_fix(c.r) + ", " + rtos_fix(c.g) + ", " + rtos_fix(c.b) + ", " + rtos_fix(c.a) + ")");
            break;
        }
        case Variant::STRING_NAME:
        {
            String str = p_variant;
            str = "&\"" + str.c_escape() + "\"";
            p_store_string(p_store_userdata, str);
            break;
        }
        case Variant::NODE_PATH:
        {
            String str = p_variant;
            str = "NodePath(\"" + str.c_escape() + "\")";
            p_store_string(p_store_userdata, str);
            break;
        }
        case Variant::RID:
        {
            // RIDs are not stored
            p_store_string(p_store_userdata, "RID()");
            break;
        }
        case Variant::SIGNAL:
        {
            // Signals are not stored
            p_store_string(p_store_userdata, "Signal()");
            break;
        }
        case Variant::CALLABLE:
        {
            // Callables are not stored
            p_store_string(p_store_userdata, "Callable()");
            break;
        }
        case Variant::OBJECT:
        {
            if (unlikely(p_recursion_count > MAX_RECURSION))
            {
                ERR_PRINT("Max recursion reached");
                p_store_string(p_store_userdata, "null");
                return OK;
            }
            p_recursion_count++;

            Object* obj = p_variant;
            if (!obj)
            {
                p_store_string(p_store_userdata, "null");
                break; // don't save it
            }

            Ref<Resource> res = p_variant;
            if (res.is_valid())
            {
                String res_text;
                if (p_encode_resource)
                    res_text = p_encode_resource(p_encode_userdata, res);

                if (res_text.is_empty() && _is_resource_file(res->get_path()))
                {
                    // External Resource
                    String path = res->get_path();
                    res_text = "Resource(\"" + path + "\")";
                }

                // Could come up with some sort of text
                if (!res_text.is_empty())
                {
                    p_store_string(p_store_userdata, res_text);
                    break;
                }
            }

            // Generic Object
            p_store_string(p_store_userdata, "Object(" + obj->get_class() + ",");

            bool first{ true };
            List<PropertyInfo> properties = DictionaryUtils::to_properties(obj->get_property_list());
            for (const PropertyInfo& property : properties)
            {
                if (property.usage & PROPERTY_USAGE_STORAGE || property.usage & PROPERTY_USAGE_SCRIPT_VARIABLE)
                {
                    if (first)
                        first = false;
                    else
                        p_store_string(p_store_userdata, ",");

                    p_store_string(p_store_userdata, "\"" + property.name + "\":");
                    write(obj->get(property.name), p_store_string, p_store_userdata, p_encode_resource, p_encode_userdata, p_recursion_count);
                }
            }
            p_store_string(p_store_userdata, ")\n");
            break;
        }
        case Variant::DICTIONARY:
        {
            Dictionary dict = p_variant;
            if (unlikely(p_recursion_count > MAX_RECURSION))
            {
                ERR_PRINT("Max recursion reached");
                p_store_string(p_store_userdata, "{}");
            }
            else
            {
                p_recursion_count++;

                Array keys = dict.keys();
                if (keys.is_empty())
                {
                    p_store_string(p_store_userdata, "{}");
                    break;
                }

                int size = keys.size();
                p_store_string(p_store_userdata, "{\n");

                for (int i = 0; i < size; i++)
                {
                    const Variant& key = keys[i];
                    write(key, p_store_string, p_store_userdata, p_encode_resource, p_encode_userdata, p_recursion_count);
                    p_store_string(p_store_userdata, ": ");
                    write(dict[key], p_store_string, p_store_userdata, p_encode_resource, p_encode_userdata, p_recursion_count);
                    if ((i + 1) < size)
                        p_store_string(p_store_userdata, ",\n");
                    else
                        p_store_string(p_store_userdata, "\n");
                }
                p_store_string(p_store_userdata, "}");
            }
            break;
        }
        case Variant::ARRAY:
        {
            Array array = p_variant;
            if (array.get_typed_builtin() != Variant::NIL)
            {
                p_store_string(p_store_userdata, "Array[");

                Variant::Type builtin_type = (Variant::Type) array.get_typed_builtin();
                StringName class_name = array.get_typed_class_name();
                Ref<Script> script = array.get_typed_script();
                if (script.is_valid())
                {
                    String res_text = String();
                    if (p_encode_resource)
                        res_text = p_encode_resource(p_encode_userdata, script);

                    if (res_text.is_empty() && _is_resource_file(script->get_path()))
                        res_text = "Resource(\"" + script->get_path() + "\")";

                    if (res_text.is_empty())
                    {
                        ERR_PRINT("Failed to encode a path to a custom script for an array type.");
                        p_store_string(p_store_userdata, class_name);
                    }
                    else
                        p_store_string(p_store_userdata, res_text);
                }
                else if (class_name != StringName())
                    p_store_string(p_store_userdata, class_name);
                else
                    p_store_string(p_store_userdata, Variant::get_type_name(builtin_type));

                p_store_string(p_store_userdata, "](");
            }

            if (unlikely(p_recursion_count > MAX_RECURSION))
            {
                ERR_PRINT("Max recursion reached");
                p_store_string(p_store_userdata, "[]");
            }
            else
            {
                p_recursion_count++;

                p_store_string(p_store_userdata, "[");
                int size = array.size();
                for (int i = 0; i < size; i++)
                {
                    if (i > 0)
                        p_store_string(p_store_userdata, ", ");
                    write(array[i], p_store_string, p_store_userdata, p_encode_resource, p_encode_userdata, p_recursion_count);
                }
                p_store_string(p_store_userdata, "]");
            }

            if (array.get_typed_builtin() != Variant::NIL)
                p_store_string(p_store_userdata, ")");

            break;
        }
        case Variant::PACKED_BYTE_ARRAY:
        {
            p_store_string(p_store_userdata, "PackedByteArray(");
            const PackedByteArray data = p_variant;
            int size = data.size();
            for (int i = 0; i < size; i++)
            {
                if (i > 0)
                    p_store_string(p_store_userdata, ", ");
                p_store_string(p_store_userdata, itos(data[i]));
            }
            p_store_string(p_store_userdata, ")");
            break;
        }
        case Variant::PACKED_INT32_ARRAY:
        {
            p_store_string(p_store_userdata, "PackedInt32Array(");
            const PackedInt32Array data = p_variant;
            int size = data.size();
            for (int i = 0; i < size; i++)
            {
                if (i > 0)
                    p_store_string(p_store_userdata, ", ");
                p_store_string(p_store_userdata, itos(data[i]));
            }
            p_store_string(p_store_userdata, ")");
            break;
        }
        case Variant::PACKED_INT64_ARRAY:
        {
            p_store_string(p_store_userdata, "PackedInt64Array(");
            const PackedInt64Array data = p_variant;
            int size = data.size();
            for (int i = 0; i < size; i++)
            {
                if (i > 0)
                    p_store_string(p_store_userdata, ", ");
                p_store_string(p_store_userdata, itos(data[i]));
            }
            p_store_string(p_store_userdata, ")");
            break;
        }
        case Variant::PACKED_FLOAT32_ARRAY:
        {
            p_store_string(p_store_userdata, "PackedFloat32Array(");
            const PackedFloat32Array data = p_variant;
            int size = data.size();
            for (int i = 0; i < size; i++)
            {
                if (i > 0)
                    p_store_string(p_store_userdata, ", ");
                p_store_string(p_store_userdata, rtos_fix(data[i]));
            }
            p_store_string(p_store_userdata, ")");
            break;
        }
        case Variant::PACKED_FLOAT64_ARRAY:
        {
            p_store_string(p_store_userdata, "PackedFloat64Array(");
            const PackedFloat64Array data = p_variant;
            int size = data.size();
            for (int i = 0; i < size; i++)
            {
                if (i > 0)
                    p_store_string(p_store_userdata, ", ");
                p_store_string(p_store_userdata, rtos_fix(data[i]));
            }
            p_store_string(p_store_userdata, ")");
            break;
        }
        case Variant::PACKED_STRING_ARRAY:
        {
            p_store_string(p_store_userdata, "PackedStringArray(");
            const PackedStringArray data = p_variant;
            int size = data.size();
            for (int i = 0; i < size; i++)
            {
                if (i > 0)
                    p_store_string(p_store_userdata, ", ");
                p_store_string(p_store_userdata, "\"" + data[i].c_escape() + "\"");
            }
            p_store_string(p_store_userdata, ")");
            break;
        }
        case Variant::PACKED_VECTOR2_ARRAY:
        {
            p_store_string(p_store_userdata, "PackedVector2Array(");
            const PackedVector2Array data = p_variant;
            int size = data.size();
            for (int i = 0; i < size; i++)
            {
                if (i > 0)
                    p_store_string(p_store_userdata, ", ");
                p_store_string(p_store_userdata, rtos_fix(data[i].x) + ", " + rtos_fix(data[i].y));
            }
            p_store_string(p_store_userdata, ")");
            break;
        }
        case Variant::PACKED_VECTOR3_ARRAY:
        {
            p_store_string(p_store_userdata, "PackedVector3Array(");
            const PackedVector3Array data = p_variant;
            int size = data.size();
            for (int i = 0; i < size; i++)
            {
                if (i > 0)
                    p_store_string(p_store_userdata, ", ");
                p_store_string(p_store_userdata, rtos_fix(data[i].x) + ", " + rtos_fix(data[i].y) + ", " + rtos_fix(data[i].z));
            }
            p_store_string(p_store_userdata, ")");
            break;
        }
        case Variant::PACKED_COLOR_ARRAY:
        {
            p_store_string(p_store_userdata, "PackedColorArray(");
            const PackedColorArray data = p_variant;
            int size = data.size();
            for (int i = 0; i < size; i++)
            {
                if (i > 0)
                    p_store_string(p_store_userdata, ", ");
                p_store_string(p_store_userdata, rtos_fix(data[i].r) + ", " + rtos_fix(data[i].g) + ", " + rtos_fix(data[i].b) + ", " + rtos(data[i].a));
            }
            p_store_string(p_store_userdata, ")");
            break;
        }
        default:
        {
            ERR_PRINT("Unknown variant type");
            return ERR_BUG;
        }
    }
    return OK;
}

static Error _write_to_str(void* p_userdata, const String& p_string)
{
    String* str = (String*) p_userdata;
    (*str) += p_string;
    return OK;
}

Error OScriptVariantWriter::write_to_string(const Variant& p_variant, String& r_string, EncodeResourceFunction p_encode_resource, void* p_encode_userdata)
{
    r_string = String();
    return write(p_variant, _write_to_str, &r_string, p_encode_resource, p_encode_userdata);
}
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
#ifndef ORCHESTRATOR_ORCHESTRATION_PARSER_STREAM_H
#define ORCHESTRATOR_ORCHESTRATION_PARSER_STREAM_H

#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

class OrchestrationStream
{
protected:
    int64_t _position{ 0 };

public:
    virtual void seek(int64_t p_position) = 0;
    virtual int64_t tell() const { return _position; }
    virtual int64_t size() const = 0;
    virtual bool is_eof() const = 0;
    virtual void reset() = 0;

    virtual ~OrchestrationStream() = default;
};

class OrchestrationStringStream : public OrchestrationStream
{
    String _data;
    bool _utf8{ false };

public:
    //! Begin OrchestrationStream Interface
    void seek(int64_t p_position) override;
    bool is_eof() const override;
    int64_t size() const override;
    void reset() override;
    //~ End OrchestrationStream Interface

    char32_t read_char();
    void rewind() { seek(tell() - 1); }
    bool is_utf8() const { return _utf8; }

    void write_char(char32_t p_char);
    void write_line(const String& p_line);

    String get_as_text() const { return _data; }

    explicit OrchestrationStringStream(const String& p_data, bool p_utf8 = true) : _data(p_data), _utf8(p_utf8) {}
};

class OrchestrationByteStream : public OrchestrationStream
{
    PackedByteArray _data;
    bool _big_endian{ false };

public:
    //! Begin OrchestrationStream Interface
    void seek(int64_t p_position) override;
    bool is_eof() const override;
    int64_t size() const override;
    void reset() override;
    //~ End OrchestrationStream Interface

    uint8_t read_u8();
    uint16_t read_u16();
    uint32_t read_u32();
    uint64_t read_u64();
    uint64_t read_buffer(uint8_t* p_buffer, uint64_t p_size);
    real_t read_real();
    float read_float();
    double read_double();
    String read_unicode_string();

    bool write_u8(uint8_t p_value);
    bool write_u16(uint16_t p_value);
    bool write_u32(uint32_t p_value);
    bool write_u64(uint64_t p_value);
    bool write_buffer(const uint8_t* p_buffer, uint64_t p_size);
    bool write_real(real_t p_value);
    bool write_float(float p_value);
    bool write_double(double p_value);
    void write_unicode_string(const String& p_data, bool p_bit_on_length = false);

    bool is_big_endian() const { return _big_endian; }
    void set_big_endian(bool p_big_endian) { _big_endian = p_big_endian; }

    PackedByteArray get_as_bytes() const { return _data; }

    OrchestrationByteStream() {}
    OrchestrationByteStream(const PackedByteArray& p_bytes) : _data(p_bytes) {}
};

#endif // ORCHESTRATOR_ORCHESTRATION_PARSER_STREAM_H
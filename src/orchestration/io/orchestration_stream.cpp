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
#include "orchestration/io/orchestration_stream.h"

#include <godot_cpp/templates/vector.hpp>

#ifdef REAL_T_IS_DOUBLE
typedef uint64_t uintr_t;
#else
typedef uint32_t uintr_t;
#endif

union MarshallFloat {
    uint32_t i; ///< int
    float f; ///< float
};

union MarshallDouble {
    uint64_t l; ///< long long
    double d; ///< double
};

union MarshallReal {
    uintr_t i;
    real_t r;
};

void OrchestrationStringStream::seek(int64_t p_position)
{
    ERR_FAIL_COND_MSG(p_position > _data.length(), "Cannot seek beyond length of data");
    _position = p_position;
}

bool OrchestrationStringStream::is_eof() const
{
    return _position >= _data.length();
}

int64_t OrchestrationStringStream::size() const
{
    return _data.length();
}

void OrchestrationStringStream::reset()
{
    _data.resize(0);
    _position = 0;
}

char32_t OrchestrationStringStream::read_char()
{
    ERR_FAIL_COND_V_MSG(_position >= _data.length(), 0, "No more data available");
    return _data[_position++];
}

void OrchestrationStringStream::write_char(char32_t p_char)
{
    _data += p_char;
    _position++;
}


void OrchestrationStringStream::write_line(const String& p_line)
{
    _data += p_line + "\n";
    _position += p_line.length() + 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OrchestrationByteStream::seek(int64_t p_position)
{
    ERR_FAIL_COND_MSG(p_position > _data.size(), "Cannot seek beyond length of data");
    _position = p_position;
}

bool OrchestrationByteStream::is_eof() const
{
    return _position >= _data.size();
}

int64_t OrchestrationByteStream::size() const
{
    return _data.size();
}

void OrchestrationByteStream::reset()
{
    _data.clear();
    _position = 0;
}

uint8_t OrchestrationByteStream::read_u8()
{
    uint8_t value = 0;
    read_buffer(&value, sizeof(uint8_t));

    return value;
}

uint16_t OrchestrationByteStream::read_u16()
{
    uint16_t value = 0;
    read_buffer(reinterpret_cast<uint8_t*>(&value), sizeof(uint16_t));

    if (_big_endian)
        value = BSWAP16(value);

    return value;
}

uint32_t OrchestrationByteStream::read_u32()
{
    uint32_t value = 0;
    read_buffer(reinterpret_cast<uint8_t*>(&value), sizeof(uint32_t));

    if (_big_endian)
        value = BSWAP32(value);

    return value;
}

uint64_t OrchestrationByteStream::read_u64()
{
    uint64_t value = 0;
    read_buffer(reinterpret_cast<uint8_t*>(&value), sizeof(uint64_t));

    if (_big_endian)
        value = BSWAP64(value);

    return value;
}

uint64_t OrchestrationByteStream::read_buffer(uint8_t* p_buffer, uint64_t p_size)
{
    ERR_FAIL_COND_V_MSG(_position + static_cast<int64_t>(p_size) > _data.size(), 0, "No more data available");

    memcpy(p_buffer, _data.ptr() + _position, p_size);
    _position += p_size;

    return p_size;
}

real_t OrchestrationByteStream::read_real()
{
    #ifdef REAL_T_IS_DOUBLE
    return read_double();
    #else
    return read_float();
    #endif
}

float OrchestrationByteStream::read_float()
{
    MarshallFloat m;
    m.i = read_u32();
    return m.f;
}

double OrchestrationByteStream::read_double()
{
    MarshallDouble m;
    m.l = read_u64();
    return m.d;
}

String OrchestrationByteStream::read_unicode_string()
{
    const uint32_t size = read_u32();

    Vector<char> buffer;
    buffer.resize(size);
    read_buffer(reinterpret_cast<uint8_t*>(buffer.ptrw()), size);

    return String::utf8(&buffer[0]);
}

bool OrchestrationByteStream::write_u8(uint8_t p_value)
{
    return write_buffer(&p_value, sizeof(uint8_t));
}

bool OrchestrationByteStream::write_u16(uint16_t p_value)
{
    if (_big_endian)
        p_value = BSWAP16(p_value);

    return write_buffer(reinterpret_cast<const uint8_t*>(&p_value), sizeof(uint16_t));
}

bool OrchestrationByteStream::write_u32(uint32_t p_value)
{
    if (_big_endian)
        p_value = BSWAP32(p_value);

    return write_buffer(reinterpret_cast<const uint8_t*>(&p_value), sizeof(uint32_t));
}

bool OrchestrationByteStream::write_u64(uint64_t p_value)
{
    if (_big_endian)
        p_value = BSWAP64(p_value);

    return write_buffer(reinterpret_cast<const uint8_t*>(&p_value), sizeof(uint64_t));
}

bool OrchestrationByteStream::write_buffer(const uint8_t* p_buffer, uint64_t p_size)
{
    const int64_t end_position = _position + static_cast<int64_t>(p_size);
    if (end_position > _data.size())
        _data.resize(end_position);

    memcpy(_data.ptrw() + _position, p_buffer, p_size);
    _position += p_size;

    return true;
}

bool OrchestrationByteStream::write_real(real_t p_value)
{
    #ifdef REAL_T_IS_DOUBLE
    return write_double(p_value);
    #else
    return write_float(p_value);
    #endif
}

bool OrchestrationByteStream::write_float(float p_value)
{
    MarshallFloat m;
    m.f = p_value;
    return write_u32(m.i);
}

bool OrchestrationByteStream::write_double(double p_value)
{
    MarshallDouble m;
    m.d = p_value;
    return write_u64(m.l);
}

void OrchestrationByteStream::write_unicode_string(const String& p_data, bool p_bit_on_length)
{
    CharString utf8 = p_data.utf8();

    const size_t length = p_bit_on_length ? (utf8.length() + 1) | 0x8000000 : utf8.length() + 1;
    write_u32(length);
    write_buffer(reinterpret_cast<const uint8_t*>(utf8.get_data()), utf8.length() + 1);
}

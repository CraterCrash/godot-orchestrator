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
#include "guid.h"

bool Guid::_parse(const String &p_guid_str, uint32_t &r_a, uint32_t &r_b, uint32_t &r_c, uint32_t &r_d)
{
    PackedStringArray bits = p_guid_str.split("-");
    if (bits.size() != 5)
    {
        ERR_FAIL_V_MSG(false, "The GUID '" + p_guid_str + "' is an invalid format.");
    }

    r_a = bits[0].hex_to_int();
    r_b = (bits[1].hex_to_int() << 16) + bits[2].hex_to_int();
    r_c = (bits[3].hex_to_int() << 16) + bits[4].substr(0, 4).hex_to_int();
    r_d = bits[4].substr(4).hex_to_int();
    return true;
}

Guid::Guid()
{
    invalidate();
}

Guid::Guid(const String& p_guid) { Guid::_parse(p_guid, _a, _b, _c, _d); }

Guid::Guid(uint32_t p_a, uint32_t p_b, uint32_t p_c, uint32_t p_d)
    : _a(p_a)
    , _b(p_b)
    , _c(p_c)
    , _d(p_d)
{
}

void Guid::invalidate()
{
    _a = 0;
    _b = 0;
    _c = 0;
    _d = 0;
}

bool Guid::is_valid() const
{
    return _a != 0 && _b != 0 && _c != 0 && _d != 0;
}

String Guid::to_string() const
{
    return vformat("%08X-%04X-%04X-%04X-%04X%08X", _a, _b >> 16, _b & 0xFFFF, _c >> 16, _c & 0xFFFF, _d);
}

Guid Guid::create_guid()
{
    Ref<RandomNumberGenerator> rng(memnew(RandomNumberGenerator));

    uint32_t a = rng->randi();
    uint32_t b = rng->randi();
    uint32_t c = rng->randi();
    uint32_t d = rng->randi();

    // The 4 bits of digit M indicate the GUID version, and the 1-3 most significant bits
    // of digit N indicate the UUID variant.
    // xxxxxxxx-xxxx-Mxxx-Nxxx-xxxxxxxxxxxx
    b = (b & 0xffff0fff) | 0x00004000; // version 4
    c = (c & 0x3fffffff) | 0x80000000; // variant 1

    return { a, b, c, d };
}

bool Guid::operator==(const Guid& p_o) const
{
    return _a == p_o._a && _b == p_o._b && _c == p_o._c && _d == p_o._d;
}

bool Guid::operator!=(const Guid& p_o) const
{
    return _a != p_o._a || _b != p_o._b || _c != p_o._c || _d != p_o._d;
}
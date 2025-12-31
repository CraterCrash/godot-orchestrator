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
#include "orchestration/serialization/binary/binary_format.h"

void OrchestrationBinaryFormat::save_unicode_string(const Ref<FileAccess>& p_file, const String& p_value, bool p_bit_on_length) {
    ERR_FAIL_COND_MSG(p_file.is_null(), "Cannot save unicode string when file reference is not valid.");

    CharString utf8 = p_value.utf8();

    size_t length = utf8.length() + 1;
    if (p_bit_on_length) {
        length |= 0x8000000;
    }

    p_file->store_32(length);
    p_file->store_buffer(reinterpret_cast<const uint8_t*>(utf8.get_data()), utf8.length() + 1);
}

String OrchestrationBinaryFormat::read_unicode_string(const Ref<FileAccess>& p_file) {
    ERR_FAIL_COND_V_MSG(p_file.is_null(), {}, "Cannot read unicode string when file reference is not valid.");

    uint32_t size = p_file->get_32();

    Vector<char> buffer;
    buffer.resize(size);
    p_file->get_buffer((uint8_t*) &buffer[0], size);

    return String::utf8(&buffer[0]);
}
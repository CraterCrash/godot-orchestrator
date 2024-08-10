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
#include "script/serialization/instance.h"

#include "common/version.h"

#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_uid.hpp>
#include <godot_cpp/templates/vector.hpp>

// The file format version
// 2: Introduced with text-based resources
// 3: Introduced external resources with binary-format; serializing script class to binary format; PackedVector4Array
uint32_t OScriptResourceFormatInstance::FORMAT_VERSION = 3;

// Remaining reserved fields in the file format (binary only)
uint32_t OScriptResourceFormatInstance::RESERVED_FIELDS = 10;

int64_t OScriptResourceFormatInstance::_get_resource_id_for_path(const String& p_path, bool p_generate)
{
    int64_t fallback = ResourceLoader::get_singleton()->get_resource_uid(p_path);
    if (fallback != ResourceUID::INVALID_ID)
        return fallback;

    if (p_generate)
        return ResourceUID::get_singleton()->create_id();

    return ResourceUID::INVALID_ID;
}

bool OScriptResourceFormatInstance::_is_resource_built_in(const Ref<Resource>& p_resource) const
{
    // Taken from resource.h
    String path_cache = p_resource->get_path();
    return path_cache.is_empty() || path_cache.contains("::") || path_cache.begins_with("local://");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

String OScriptResourceBinaryFormatInstance::_read_unicode_string(const Ref<FileAccess>& p_file)
{
    int length = p_file->get_32();

    Vector<char> buffer;
    buffer.resize(length);
    p_file->get_buffer((uint8_t*) &buffer[0], length);

    return String::utf8(&buffer[0]);
}

void OScriptResourceBinaryFormatInstance::_save_unicode_string(const Ref<FileAccess>& p_file, const String& p_value, bool p_bit_on_length)
{
    CharString utf8 = p_value.utf8();

    size_t length;
    if (p_bit_on_length)
        length = (utf8.length() + 1) | 0x8000000;
    else
        length = (utf8.length() + 1);

    p_file->store_32(length);
    p_file->store_buffer((const uint8_t*)utf8.get_data(), utf8.length() + 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

String OScriptResourceTextFormatInstance::_create_start_tag(const String& p_resource_class, const String& p_script_class, uint32_t p_load_steps, uint32_t p_version, int64_t p_uid)
{
    // todo: replace the implementation in "save"

    String tag = "[orchestration type=\"" + p_resource_class + "\" ";

    #if GODOT_VERSION >= 0x040300
    if (!p_script_class.is_empty())
        tag += "script_class=\"" + p_script_class + "\" ";
    #endif

    if (p_load_steps > 1)
        tag += "load_steps=" + itos(p_load_steps) + " ";

    tag += "format=" + itos(p_version) + "";

    if (p_uid != ResourceUID::INVALID_ID)
        tag += " uid=\"" + ResourceUID::get_singleton()->id_to_text(p_uid) + "\"";

    tag += "]\n";

    return tag;
}

String OScriptResourceTextFormatInstance::_create_ext_resource_tag(const String& p_type, const String& p_path, const String& p_id, bool p_newline)
{
    String tag = "[ext_resource type=\"" + p_type + "\"";

    #if GODOT_VERSION >= 0x040300
    int64_t uid = _get_resource_id_for_path(p_path, false);
    #else
    int64_t uid = ResourceUID::INVALID_ID;
    #endif

    if (uid != ResourceUID::INVALID_ID)
        tag += " uid=\"" + ResourceUID::get_singleton()->id_to_text(uid) + "\"";

    tag += " path=\"" + p_path + "\" id=\"" + p_id + "\"]";
    if (p_newline)
        tag += "\n";

    return tag;
}

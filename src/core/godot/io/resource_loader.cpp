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
#include "core/godot/io/resource_loader.h"

#include "orchestration/serialization/text/variant_parser.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_uid.hpp>

String GDE::ResourceLoader::get_resource_type(const String& p_path) {
    const Ref<Resource> res = godot::ResourceLoader::get_singleton()->load(p_path, "", godot::ResourceLoader::CACHE_MODE_IGNORE);
    if (res.is_valid()) {
        return res->get_class();
    }
    return {};
}

String GDE::ResourceLoader::path_remap(const String& p_path) {
    String new_path = ResourceUID::ensure_path(p_path);
    if (FileAccess::file_exists(new_path + ".remap")) {
        const Ref<FileAccess> file = FileAccess::open(new_path + ".remap", FileAccess::READ);
        if (file.is_valid()) {
            OScriptVariantParser::StreamFile stream;
            stream.data = file;

            String property;
            Variant value;
            OScriptVariantParser::Tag next_tag;

            int lines = 0;
            String error_text;
            while (true) {
                property = Variant();
                next_tag.fields.clear();
                next_tag.name = String();

                Error err = OScriptVariantParser::parse_tag_assign_eof(&stream, lines, error_text, next_tag, property, value, nullptr, true);
                if (err == ERR_FILE_EOF) {
                    break;
                } else if (err != OK) {
                    ERR_PRINT(vformat("Parse error: %s.remap:%d error: %s.", p_path, lines, error_text));
                    break;
                }

                if (property == "path") {
                    new_path = value;
                    break;
                } else if (next_tag.name != "remap") {
                    break;
                }
            }
        }
    }

    return new_path;
}
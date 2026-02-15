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
#include "orchestration/serialization/text/text_format.h"

#include "common/version.h"

#include <godot_cpp/classes/resource_uid.hpp>

int64_t OrchestrationTextFormat::get_resource_id_for_path(const String& p_path, bool p_generate) {
    #if GODOT_VERSION >= 0x040300
    int64_t fallback = ResourceLoader::get_singleton()->get_resource_uid(p_path);
    if (fallback != ResourceUID::INVALID_ID) {
        return fallback;
    }
    if (p_generate) {
        return ResourceUID::get_singleton()->create_id();
    }
    #endif

    return ResourceUID::INVALID_ID;
}

String OrchestrationTextFormat::create_start_tag(const String& p_class, const String& p_script_class, const String& p_icon_path, uint32_t p_steps, uint32_t p_version, int64_t p_uid) {
    String tag = vformat(R"([orchestration type="%s" )", p_class);

    if (!p_script_class.is_empty()) {
        tag += vformat(R"(script_class="%s" )", p_script_class);
    }

    if (!p_icon_path.is_empty()) {
        tag += vformat(R"(icon="%s" )", p_icon_path);
    }

    if (p_steps > 1) {
        tag += vformat("load_steps=%d ", p_steps);
    }

    tag += vformat("format=%d", p_version);

    if (p_uid != ResourceUID::INVALID_ID) {
        tag += vformat(R"( uid="%s")", ResourceUID::get_singleton()->id_to_text(p_uid));
    }

    return vformat("%s]\n", tag);
}

String OrchestrationTextFormat::create_ext_resource_tag(const String& p_type, const String& p_path, const String& p_id, bool p_newline) {
    String tag = vformat(R"([ext_resource type="%s")", p_type);

    int64_t uid = get_resource_id_for_path(p_path, false);
    if (uid != ResourceUID::INVALID_ID) {
        tag += vformat(R"( uid="%s")", ResourceUID::get_singleton()->id_to_text(uid));
    }

    return vformat(R"(%s path="%s" id="%s"%s])", tag, p_path, p_id, p_newline ? "\n" : "");
}
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
#include "script/script_template_registry.h"

#include "templates.gen.h"

#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/marshalls.hpp>

void OScriptTemplateRegistry::_load_template_data() {
    PackedByteArray compressed;
    compressed.resize(_template_data_compressed_size);
    memcpy(compressed.ptrw(), _template_data_compressed, _template_data_compressed_size);

    const PackedByteArray decompressed = compressed.decompress(_template_data_uncompressed_size, 1);

    const Dictionary json = JSON::parse_string(decompressed.get_string_from_utf8());
    ERR_FAIL_COND_MSG(json.is_empty(), "Failed to load Orchestrator OScript template data.");

    const Array templates = json.get("templates", Array());
    for (uint32_t i = 0; i < templates.size(); i++) {
        const Dictionary data = templates[i];

        int tmpl_decompress_size = data.get("data_size", 0);
        String tmpl_base64 = data.get("data", "");

        const PackedByteArray tmpl_compressed = Marshalls::get_singleton()->base64_to_raw(tmpl_base64);
        const PackedByteArray tmpl_decompress = tmpl_compressed.decompress(tmpl_decompress_size, 1);

        Template tmpl;
        tmpl.name = String(data.get("name", "")).get_basename().capitalize();
        tmpl.inherits = String(data.get("inherits", ""));
        tmpl.description = data.get("description", "");
        tmpl.script_template = tmpl_decompress.get_string_from_utf8();

        ERR_CONTINUE(tmpl.name.is_empty() || tmpl.inherits.is_empty() || tmpl.script_template.is_empty());
        _templates.push_back(tmpl);
    }
}

List<OScriptTemplateRegistry::Template> OScriptTemplateRegistry::get_templates(const StringName& p_base_type) {
    List<Template> templates;
    for (const Template& E : _templates) {
        if (E.inherits == String(p_base_type)) {
            templates.push_back(E);
        }
    }
    return templates;
}

OScriptTemplateRegistry::OScriptTemplateRegistry() {
    #ifdef TOOLS_ENABLED
    _load_template_data();
    #endif
}

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
#include "script/serialization/text/resource_saver_text.h"

#include "orchestration/serialization/text/text_serializer.h"
#include "script/script.h"
#include "script/script_server.h"

PackedStringArray OScriptTextResourceFormatSaver::_get_recognized_extensions(const Ref<Resource>& p_resource) const {
    return _recognize(p_resource) ? Array::make(ORCHESTRATOR_SCRIPT_TEXT_EXTENSION) : Array();
}

bool OScriptTextResourceFormatSaver::_recognize_path(const Ref<Resource>& p_resource, const String& p_path) const {
    return p_path.get_extension() == ORCHESTRATOR_SCRIPT_TEXT_EXTENSION;
}

bool OScriptTextResourceFormatSaver::_recognize(const Ref<Resource>& p_resource) const {
    return p_resource.is_valid() && p_resource->get_class() == OScript::get_class_static();
}

Error OScriptTextResourceFormatSaver::_set_uid(const String& p_path, int64_t p_uid) {
    OrchestrationTextSerializer serializer;
    return serializer.set_uid(p_path, p_uid);
}

Error OScriptTextResourceFormatSaver::_save(const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags) {
    Ref<OScript> script = p_resource;
    ERR_FAIL_COND_V_MSG(script.is_null(), ERR_INVALID_PARAMETER, "Cannot save a non OScript resource.");

    Ref<Orchestration> orchestration = script->get_orchestration();
    ERR_FAIL_COND_V_MSG(orchestration.is_null(), ERR_INVALID_PARAMETER, "Cannot save, Orchestration is empty");

    OrchestrationTextSerializer serializer;
    Error err = serializer.save(orchestration, p_path, p_flags);
    if (err != OK) {
        return err;
    }

    if (ScriptServer::is_reload_scripts_on_save_enabled()) {
        OScriptLanguage::get_singleton()->_reload_tool_script(p_resource, true);
    }

    return OK;
}

void OScriptTextResourceFormatSaver::_bind_methods() {
}
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
#include "core/godot/object/script_language.h"

#include "common/dictionary_utils.h"
#include "script/script.h"

#include <godot_cpp/classes/script_extension.hpp>

MethodInfo GDE::Script::get_method_info(const Ref<godot::Script>& p_script, const StringName& p_function) {
    ERR_FAIL_COND_V(p_script.is_null(), {});

    const TypedArray<Dictionary> methods = p_script->get_script_method_list();
    for (uint32_t i = 0; i < methods.size(); i++) {
        const MethodInfo method = DictionaryUtils::to_method(methods[i]);
        if (method.name == p_function) {
            return method;
        }
    }
    return {};
}

bool GDE::Script::inherits_script(const Ref<godot::Script>& p_script, const Ref<godot::Script>& p_parent_script) {
    if (p_script->get_class() != p_parent_script->get_class()) {
        // Godot does not allow cross script language inheritance
        return false;
    }

    const Ref<OScript> script = p_script;
    if (script.is_valid()) {
        return script->_inherits_script(p_parent_script);
    }

    return false;
}

bool GDE::Script::is_valid(const Ref<godot::Script>& p_script) {
    if (p_script.is_valid()) {
        const Ref<OScript> oscript = p_script;
        if (oscript.is_null()) {
            // Assume other scripts are valid
            return true;
        }
        if (oscript.is_valid() && oscript->_is_valid()) {
            return true;
        }
    }
    return false;
}
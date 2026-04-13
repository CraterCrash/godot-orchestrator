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
#include "script/register_script_types.h"

#include "script/language.h"
#include "script/script.h"
#include "script/script_cache.h"
#include "script/serialization/binary/resource_loader_binary.h"
#include "script/serialization/binary/resource_saver_binary.h"
#include "script/serialization/resource_cache.h"
#include "script/serialization/resource_format.h"
#include "script/serialization/text/resource_loader_text.h"
#include "script/serialization/text/resource_saver_text.h"
#include "script/utility_functions.h"

#include <godot_cpp/classes/engine.hpp>

void register_script_types() {

    // Register loader/savers
    GDREGISTER_INTERNAL_CLASS(OScriptTextResourceFormatLoader)
    GDREGISTER_INTERNAL_CLASS(OScriptTextResourceFormatSaver)
    GDREGISTER_INTERNAL_CLASS(OScriptBinaryResourceFormatLoader)
    GDREGISTER_INTERNAL_CLASS(OScriptBinaryResourceFormatSaver)

    // Nodes - Abstract first
    GDREGISTER_INTERNAL_CLASS(OScriptLanguage)

    // Purposely public
    GDREGISTER_CLASS(OScript)
    GDREGISTER_INTERNAL_CLASS(OScriptNativeClass)
    GDREGISTER_INTERNAL_CLASS(OScriptFunctionState)
    GDREGISTER_INTERNAL_CLASS(OScriptParserRef)

    OScriptLanguage::create();
    OScriptCache::create();
    OScriptUtilityFunctions::register_functions();
}

void unregister_script_types() {
    OScriptCache::destroy();
    OScriptLanguage::destroy();
    OScriptUtilityFunctions::unregister_functions();
}

void register_script_extension() {
    OScriptLanguage* language = OScriptLanguage::get_singleton();
    if (language) {
        Engine::get_singleton()->register_script_language(language);
    }
}

void unregister_script_extension() {
    OScriptLanguage* language = OScriptLanguage::get_singleton();
    if (language) {
        Engine::get_singleton()->unregister_script_language(language);
    }
}

void register_script_resource_formats() {
    ResourceCache::create();
    OScriptResourceFormat::create();
}

void unregister_script_resource_formats() {
    OScriptResourceFormat::destroy();
    ResourceCache::destroy();
}

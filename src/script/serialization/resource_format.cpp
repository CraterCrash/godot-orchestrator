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
#include "script/serialization/resource_format.h"

#include "script/serialization/binary/resource_loader_binary.h"
#include "script/serialization/binary/resource_saver_binary.h"
#include "script/serialization/text/resource_loader_text.h"
#include "script/serialization/text/resource_saver_text.h"

#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>

LocalVector<Ref<ResourceFormatLoader>> OScriptResourceFormat::_loaders;
LocalVector<Ref<ResourceFormatSaver>> OScriptResourceFormat::_savers;

void OScriptResourceFormat::create() {
    _loaders.push_back(memnew(OScriptBinaryResourceFormatLoader));
    _loaders.push_back(memnew(OScriptTextResourceFormatLoader));
    for (const Ref<ResourceFormatLoader>& loader : _loaders) {
        ResourceLoader::get_singleton()->add_resource_format_loader(loader);
    }

    _savers.push_back(memnew(OScriptBinaryResourceFormatSaver));
    _savers.push_back(memnew(OScriptTextResourceFormatSaver));
    for (const Ref<ResourceFormatSaver>& saver : _savers) {
        ResourceSaver::get_singleton()->add_resource_format_saver(saver);
    }
}

void OScriptResourceFormat::destroy() {
    for (Ref<ResourceFormatSaver>& saver : _savers) {
        ResourceSaver::get_singleton()->remove_resource_format_saver(saver);
        saver.unref();
    }
    _savers.clear();

    for (Ref<ResourceFormatLoader>& loader : _loaders) {
        ResourceLoader::get_singleton()->remove_resource_format_loader(loader);
        loader.unref();
    }
    _loaders.clear();
}
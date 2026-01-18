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
#ifndef ORCHESTRATOR_CORE_GODOT_IO_RESOURCE_LOADER_H
#define ORCHESTRATOR_CORE_GODOT_IO_RESOURCE_LOADER_H

#include <godot_cpp/variant/string.hpp>

using namespace godot;

namespace GDE {
    struct ResourceLoader {
        ResourceLoader() = delete;

        // While this works, it's highly inefficient because it requires loading the resource. If the resource
        // isn't used, this incurs a potentially higherIO cost that could otherwise be lower, depending on the
        // resource's loader implementation.
        static String get_resource_type(const String& p_path);

        // Reads the <code>p_path.remap</code> file if exists, providing the remapped filename if path is remapped
        static String path_remap(const String& p_path);
    };
}

#endif // ORCHESTRATOR_CORE_GODOT_IO_RESOURCE_LOADER_H
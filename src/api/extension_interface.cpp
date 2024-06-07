// This file is part of the Godot Orchestrator project.
//
// Copyright (c) 2023-present Vahera Studios LLC and its contributors.
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
#include "extension_interface.h"

#include "common/logger.h"
#include "common/version.h"
#include "editor/register_editor_types.h"
#include "script/register_script_types.h"

#include <gdextension_interface.h>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

namespace orchestrator
{
    Logger* logger = nullptr;

    void initialize_extension_module(ModuleInitializationLevel p_level)
    {
        if (p_level == MODULE_INITIALIZATION_LEVEL_CORE)
        {
            // Initialize the logger
            logger = LoggerFactory::create("user://orchestrator.log");
            Logger::info("Starting " VERSION_FULL_NAME);

            GDExtensionGodotVersion godot_version;
            internal::gdextension_interface_get_godot_version(&godot_version);
            Logger::info("Using ", godot_version.string);

            register_extension_db();
        }

        if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS)
        {
            register_script_types();
        }
        if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE)
        {
            register_script_extension();
            register_script_resource_formats();
            register_script_node_types();
        }
        if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR)
        {
            register_editor_types();
        }
    }

    void uninitialize_extension_module(ModuleInitializationLevel p_level)
    {
        if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR)
        {
            unregister_editor_types();
        }
        if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE)
        {
            unregister_script_node_types();
            unregister_script_extension();
        }
        if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS)
        {
            unregister_script_resource_formats();
            unregister_script_types();
        }
        if (p_level == MODULE_INITIALIZATION_LEVEL_CORE)
        {
            unregister_extension_db();

            Logger::info("Shutting down " VERSION_FULL_NAME);
            delete(logger);
        }
    }

    extern "C"
    {
        GDExtensionBool GDE_EXPORT extension_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address,
                                                          GDExtensionClassLibraryPtr p_library,
                                                          GDExtensionInitialization* r_initialization)
        {
            GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
            init_obj.register_initializer(initialize_extension_module);
            init_obj.register_terminator(uninitialize_extension_module);
            init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_CORE);
            return init_obj.init();
        }
    }
}

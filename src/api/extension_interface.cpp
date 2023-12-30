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

#include <gdextension_interface.h>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

// Godot helpers
#include "common/logger.h"
#include "common/version.h"

// Plugin bits
#include "plugin/plugin.h"
#include "plugin/settings.h"

// Script bits
#include "script/nodes/script_nodes.h"
#include "script/resource/format_loader.h"
#include "script/resource/format_saver.h"
#include "script/script.h"

// Editor bits
#include "editor/editor.h"

#include "extension_db.h"

using namespace godot;

namespace orchestrator
{
    OScriptLanguage* script_language_extension = nullptr;
    OrchestratorSettings* settings = nullptr;
    Ref<OScriptResourceLoader> loader;
    Ref<OScriptResourceSaver> saver;
    Logger* logger = nullptr;
    ExtensionDB* extension_db = nullptr;

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

            extension_db = new ExtensionDB();

            internal::ExtensionDBLoader db_loader;
            db_loader.prime();
        }

        if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS)
        {
            ORCHESTRATOR_REGISTER_INTERNAL_CLASS(OrchestratorSettings)

            register_script_classes();
            script_language_extension = memnew(OScriptLanguage);
        }
        if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE)
        {
            settings = memnew(OrchestratorSettings);

            // Adjust logger level based on project settings
            const String level = settings->get_setting("settings/log_level");
            Logger::set_level(Logger::get_level_from_name(level));

            Engine::get_singleton()->register_script_language(script_language_extension);

            loader.instantiate();
            ResourceLoader::get_singleton()->add_resource_format_loader(loader);

            saver.instantiate();
            ResourceSaver::get_singleton()->add_resource_format_saver(saver);

            register_script_node_classes();
        }
        if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR)
        {
            register_editor_classes();
            register_plugin_classes();

            EditorPlugins::add_by_type<OrchestratorPlugin>();
        }
    }

    void uninitialize_extension_module(ModuleInitializationLevel p_level)
    {
        if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS)
        {
            if (script_language_extension)
                memdelete(script_language_extension);

            if (loader.is_valid())
            {
                ResourceLoader::get_singleton()->remove_resource_format_loader(loader);
                loader.unref();
            }

            if (saver.is_valid())
            {
                ResourceSaver::get_singleton()->remove_resource_format_saver(saver);
                saver.unref();
            }
        }

        if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE)
        {
            if (settings)
                memdelete(settings);

            if (script_language_extension)
                Engine::get_singleton()->unregister_script_language(script_language_extension);
        }

        if (p_level == MODULE_INITIALIZATION_LEVEL_CORE)
        {
            delete(extension_db);

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

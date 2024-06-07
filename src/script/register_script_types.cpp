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
#include "script/register_script_types.h"

#include "common/settings.h"
#include "script/nodes/script_nodes.h"
#include "script/script.h"
#include "script/serialization/serialization.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/templates/vector.hpp>

namespace orchestrator
{
    namespace internal
    {
        Vector<Ref<ResourceFormatLoader>> loaders;
        Vector<Ref<ResourceFormatSaver>> savers;

        OScriptLanguage* language{ nullptr };
        OrchestratorSettings* settings{ nullptr };
        ExtensionDB* extension_db{ nullptr };
    }
}

void register_script_types()
{
    using namespace orchestrator::internal;

    // Register loader/savers
    ORCHESTRATOR_REGISTER_INTERNAL_CLASS(OScriptBinaryResourceLoader)
    ORCHESTRATOR_REGISTER_INTERNAL_CLASS(OScriptBinaryResourceSaver)

    // Settings
    ORCHESTRATOR_REGISTER_INTERNAL_CLASS(OrchestratorSettings)

    // Nodes - Abstract first
    ORCHESTRATOR_REGISTER_ABSTRACT_NODE_CLASS(OScriptNode)
    ORCHESTRATOR_REGISTER_INTERNAL_CLASS(OScriptTargetObject)
    ORCHESTRATOR_REGISTER_INTERNAL_CLASS(OScriptNodePin)
    ORCHESTRATOR_REGISTER_INTERNAL_CLASS(OScriptLanguage)
    ORCHESTRATOR_REGISTER_INTERNAL_CLASS(OScriptGraph)
    ORCHESTRATOR_REGISTER_INTERNAL_CLASS(OScriptFunction)
    ORCHESTRATOR_REGISTER_INTERNAL_CLASS(OScriptVariable)
    ORCHESTRATOR_REGISTER_INTERNAL_CLASS(OScriptSignal)
    ORCHESTRATOR_REGISTER_INTERNAL_CLASS(OScriptState)
    ORCHESTRATOR_REGISTER_INTERNAL_CLASS(OScriptAction)

    // Purposely public
    ORCHESTRATOR_REGISTER_CLASS(OScript)

    // Create the ScriptExtension
    language = memnew(OScriptLanguage);
}

void unregister_script_types()
{
    using namespace orchestrator::internal;

    if (language)
    {
        memdelete(language);
        language = nullptr;
    }
}

void register_script_extension()
{
    using namespace orchestrator::internal;

    // Create the settings implementation
    // This must be done before we create the OScriptLanguage
    settings = memnew(OrchestratorSettings);
    if (settings)
    {
        // Adjust logger level based on project settings
        const String level = settings->get_setting("settings/log_level");
        Logger::set_level(Logger::get_level_from_name(level));
    }

    Engine::get_singleton()->register_script_language(language);
}

void unregister_script_extension()
{
    using namespace orchestrator::internal;

    if (language)
        Engine::get_singleton()->unregister_script_language(language);

    if (settings)
    {
        memdelete(settings);
        settings = nullptr;
    }
}

void register_script_node_types()
{
    // Script Nodes (Abstracts first)
    ORCHESTRATOR_REGISTER_ABSTRACT_NODE_CLASS(OScriptEditablePinNode)
    ORCHESTRATOR_REGISTER_ABSTRACT_NODE_CLASS(OScriptNodeProperty)
    ORCHESTRATOR_REGISTER_ABSTRACT_NODE_CLASS(OScriptNodeVariable)
    ORCHESTRATOR_REGISTER_ABSTRACT_NODE_CLASS(OScriptNodeConstant)
    ORCHESTRATOR_REGISTER_ABSTRACT_NODE_CLASS(OScriptNodeSwitchEditablePin)
    ORCHESTRATOR_REGISTER_ABSTRACT_NODE_CLASS(OScriptNodeClassConstantBase)

    //~ Constants
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeGlobalConstant)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeMathConstant)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeTypeConstant)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeClassConstant)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeSingletonConstant)

    //~ Data
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeMakeArray)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeMakeDictionary)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeArrayGet)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeArraySet)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeArrayFind)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeArrayClear)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeArrayAppend)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeArrayAddElement)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeArrayRemoveElement)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeArrayRemoveIndex)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeCoercion)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeCompose)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeComposeFrom)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeDecompose)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeDictionarySet)

    //~ Dialogue
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeDialogueChoice)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeDialogueMessage)

    //~ Flow
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeBranch)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeChance)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeDelay)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeForLoop)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeForEach)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeRandom)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeSelect)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeSequence)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeSwitch)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeSwitchEnum)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeTypeCast)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeWhile)

    //~ Functions
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeCallFunction)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeCallBuiltinFunction)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeCallMemberFunction)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeCallScriptFunction)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeCallStaticFunction)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeFunctionTerminator)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeFunctionEntry)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeFunctionResult)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeEvent)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeSwitchString)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeSwitchInteger)

    //~ Input
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeInputAction)

    //~ Math
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeOperator)

    //~ Memory
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeNew);
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeFree);

    //~ Properties
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodePropertyGet)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodePropertySet)

    //~ Resources
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodePreload)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeResourcePath)

    //~ Scene
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeInstantiateScene)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeSceneNode)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeSceneTree)

    //~ Signals
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeAwaitSignal)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeEmitMemberSignal)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeEmitSignal)

    //~ Utility
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeAutoload)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeComment)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeEngineSingleton)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodePrintString)

    // Variables
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeSelf)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeVariableGet)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeVariableSet)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeLocalVariable)
    ORCHESTRATOR_REGISTER_NODE_CLASS(OScriptNodeAssignLocalVariable)
}

void unregister_script_node_types()
{

}

void register_script_resource_formats()
{
    using namespace orchestrator::internal;

    // Create loaders & register
    loaders.push_back(memnew(OScriptBinaryResourceLoader));
    for (const Ref<ResourceFormatLoader>& loader : loaders)
        ResourceLoader::get_singleton()->add_resource_format_loader(loader);

    // Create savers & register
    savers.push_back(memnew(OScriptBinaryResourceSaver));
    for (const Ref<ResourceFormatSaver>& saver : savers)
        ResourceSaver::get_singleton()->add_resource_format_saver(saver);
}

void unregister_script_resource_formats()
{
    using namespace orchestrator::internal;

    for (Ref<ResourceFormatSaver>& saver : savers)
    {
        ResourceSaver::get_singleton()->remove_resource_format_saver(saver);
        saver.unref();
    }
    savers.clear();

    for (Ref<ResourceFormatLoader>& loader : loaders)
    {
        ResourceLoader::get_singleton()->remove_resource_format_loader(loader);
        loader.unref();
    }
    loaders.clear();
}

void register_extension_db()
{
    using namespace orchestrator::internal;

    extension_db = new ExtensionDB();

    internal::ExtensionDBLoader loader;
    loader.prime();
}

void unregister_extension_db()
{
    using namespace orchestrator::internal;

    delete extension_db;
    extension_db = nullptr;
}

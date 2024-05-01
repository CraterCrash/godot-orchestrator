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
#include "script_nodes.h"

void register_script_node_classes()
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
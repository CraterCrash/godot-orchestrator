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
#include "editor/register_editor_types.h"

#include "about_dialog.h"
#include "editor/component_panels/component_panel.h"
#include "editor/component_panels/functions_panel.h"
#include "editor/component_panels/graphs_panel.h"
#include "editor/component_panels/macros_panel.h"
#include "editor/component_panels/signals_panel.h"
#include "editor/component_panels/variables_panel.h"
#include "editor/editor_panel.h"
#include "editor/file_dialog.h"
#include "editor/getting_started.h"
#include "editor/goto_node_dialog.h"
#include "editor/graph/actions/action_menu.h"
#include "editor/graph/actions/default_action_registrar.h"
#include "editor/graph/autowire_selections.h"
#include "editor/graph/graph_edit.h"
#include "editor/graph/graph_knot.h"
#include "editor/graph/graph_node.h"
#include "editor/graph/graph_node_pin.h"
#include "editor/graph/graph_node_spawner.h"
#include "editor/graph/nodes/graph_node_comment.h"
#include "editor/graph/nodes/graph_node_default.h"
#include "editor/graph/pins/graph_node_pins.h"
#include "editor/inspector/editor_property_class_name.h"
#include "editor/inspector/property_info_container_property.h"
#include "editor/inspector/property_type_button_property.h"
#include "editor/plugins/inspector_plugins.h"
#include "editor/plugins/orchestration_editor_export_plugin.h"
#include "editor/plugins/orchestrator_editor_plugin.h"
#include "editor/property_selector.h"
#include "editor/scene_node_selector.h"
#include "editor/script_connections.h"
#include "editor/script_editor_viewport.h"
#include "editor/search/search_dialog.h"
#include "editor/select_class_dialog.h"
#include "editor/select_type_dialog.h"
#include "editor/theme/theme_cache.h"
#include "editor/updater.h"
#include "editor/window_wrapper.h"
#include "script/serialization/text_loader_instance.h"


void register_editor_types()
{
    // Plugin bits
    GDREGISTER_INTERNAL_CLASS(OrchestratorPlugin)
    #if GODOT_VERSION >= 0x040300
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorDebuggerPlugin)
    #endif
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorExportPlugin)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorInspectorPluginFunction)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorInspectorPluginSignal)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorInspectorPluginVariable)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorInspectorPluginTypeCast)
    GDREGISTER_INTERNAL_CLASS(OrchestratorThemeCache)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorCache)
    GDREGISTER_INTERNAL_CLASS(OrchestratorBuildOutputPanel)

    // Editor bits
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorPropertyClassName)
    GDREGISTER_INTERNAL_CLASS(OrchestratorPropertyInfoContainerEditorProperty)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorPropertyVariableClassification)
    GDREGISTER_INTERNAL_CLASS(OrchestratorFileDialog)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorSearchDialogItem)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorSearchDialog)
    GDREGISTER_INTERNAL_CLASS(OrchestratorSelectClassSearchDialog)
    GDREGISTER_INTERNAL_CLASS(OrchestratorSelectTypeSearchDialog)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorSearchHelpBit)
    GDREGISTER_INTERNAL_CLASS(OrchestratorScriptAutowireSelections)
    GDREGISTER_INTERNAL_CLASS(OrchestratorPropertySelector)
    GDREGISTER_INTERNAL_CLASS(OrchestratorSceneNodeSelector)

    // Action components
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphActionMenu)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphActionMenuItem)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphActionHandler)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphNodeSpawner)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphActionRegistrar)
    GDREGISTER_INTERNAL_CLASS(OrchestratorDefaultGraphActionRegistrar)

    // Node spawners
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphNodeSpawnerProperty)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphNodeSpawnerPropertyGet)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphNodeSpawnerPropertySet)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphNodeSpawnerCallMemberFunction)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphNodeSpawnerCallScriptFunction)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphNodeSpawnerEvent)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphNodeSpawnerEmitMemberSignal)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphNodeSpawnerEmitSignal)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphNodeSpawnerVariable)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphNodeSpawnerVariableGet)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphNodeSpawnerVariableSet)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphNodeSpawnerScriptNode)

    // View components
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorPanel)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorViewport)
    GDREGISTER_INTERNAL_CLASS(OrchestratorScriptEditorViewport)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGotoNodeDialog)
    GDREGISTER_INTERNAL_CLASS(OrchestratorUpdaterButton)
    GDREGISTER_INTERNAL_CLASS(OrchestratorUpdaterVersionPicker)
    GDREGISTER_INTERNAL_CLASS(OrchestratorUpdaterReleaseNotesDialog)
    GDREGISTER_INTERNAL_CLASS(OrchestratorAboutDialog)
    GDREGISTER_INTERNAL_CLASS(OrchestratorScreenSelect)
    GDREGISTER_INTERNAL_CLASS(OrchestratorWindowWrapper)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGettingStarted)
    GDREGISTER_INTERNAL_CLASS(OrchestratorScriptConnectionsDialog)
    GDREGISTER_INTERNAL_CLASS(OrchestratorScriptComponentPanel)
    GDREGISTER_INTERNAL_CLASS(OrchestratorScriptFunctionsComponentPanel)
    GDREGISTER_INTERNAL_CLASS(OrchestratorScriptGraphsComponentPanel)
    GDREGISTER_INTERNAL_CLASS(OrchestratorScriptMacrosComponentPanel)
    GDREGISTER_INTERNAL_CLASS(OrchestratorScriptSignalsComponentPanel)
    GDREGISTER_INTERNAL_CLASS(OrchestratorScriptVariablesComponentPanel)

    // Graph Classes
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphEdit)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphNode)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphNodePin)
    GDREGISTER_INTERNAL_CLASS(OrchestratorKnotPoint)

    // Graph Node Type
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphNodeDefault)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphNodeComment)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphKnot)

    // Graph Pin Types
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphNodePinBitField)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphNodePinBool)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphNodePinColor)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphNodePinEnum)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphNodePinExec)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphNodePinFile)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphNodePinInputAction)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphNodePinNodePath)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphNodePinNumeric)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphNodePinObject)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphNodePinString)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphNodePinStruct)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGraphNodePinText)

    // Add plugin to the editor
    EditorPlugins::add_by_type<OrchestratorPlugin>();
}

void unregister_editor_types()
{
    // Remove plugin from the editor
    EditorPlugins::remove_by_type<OrchestratorPlugin>();
}

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

#include "editor/actions/definition.h"
#include "editor/actions/filter_engine.h"
#include "editor/actions/menu.h"
#include "editor/actions/registry.h"
#include "editor/actions/rules/override_function_rule.h"
#include "editor/autowire_connection_dialog.h"
#include "editor/editor.h"
#include "editor/export/orchestration_export_plugin.h"
#include "editor/getting_started.h"
#include "editor/goto_node_dialog.h"
#include "editor/graph/graph_node.h"
#include "editor/graph/graph_panel.h"
#include "editor/graph/knot_editor.h"
#include "editor/graph/nodes/comment_graph_node.h"
#include "editor/graph/nodes/knot_node.h"
#include "editor/graph/pins/pins.h"
#include "editor/gui/about_dialog.h"
#include "editor/gui/context_menu.h"
#include "editor/gui/editor_log_event_router.h"
#include "editor/gui/file_dialog.h"
#include "editor/gui/search_dialog.h"
#include "editor/gui/select_class_dialog.h"
#include "editor/gui/select_type_dialog.h"
#include "editor/gui/window_wrapper.h"
#include "editor/inspector/function_inspector_plugin.h"
#include "editor/inspector/orchestration_inspector_plugin.h"
#include "editor/inspector/properties/editor_property_class_name.h"
#include "editor/inspector/properties/editor_property_extends.h"
#include "editor/inspector/properties/editor_property_pin_properties.h"
#include "editor/inspector/properties/editor_property_variable_classification.h"
#include "editor/inspector/signal_inspector_plugin.h"
#include "editor/inspector/type_cast_inspector_plugin.h"
#include "editor/inspector/variable_inspector_plugin.h"
#include "editor/plugins/orchestrator_editor_plugin.h"
#include "editor/property_selector.h"
#include "editor/scene/connections_dock.h"
#include "editor/scene/script_connections.h"
#include "editor/scene_node_selector.h"
#include "editor/script_components_container.h"
#include "editor/script_editor_view.h"
#include "editor/updater/updater.h"

void register_editor_types() {
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
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorInspectorPluginOrchestration)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorGraphNodeThemeCache)

    // Editor bits
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorLogEventRouter)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorPropertyClassName)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorPropertyPinProperties)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorPropertyVariableClassification)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorPropertyExtends)
    GDREGISTER_INTERNAL_CLASS(OrchestratorFileDialog)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorSearchDialogItem)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorSearchDialog)
    GDREGISTER_INTERNAL_CLASS(OrchestratorSelectClassSearchDialog)
    GDREGISTER_INTERNAL_CLASS(OrchestratorSelectTypeSearchDialog)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorSearchHelpBit)
    GDREGISTER_INTERNAL_CLASS(OrchestratorAutowireConnectionDialog)
    GDREGISTER_INTERNAL_CLASS(OrchestratorPropertySelector)
    GDREGISTER_INTERNAL_CLASS(OrchestratorSceneNodeSelector)

    // Action components
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorActionMenu)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorActionHelp)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorActionDefinition)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorActionFilterEngine)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorActionRegistry)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorActionFilterRule)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorActionAnyFilterRule)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorActionTypeRule)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorActionPortRule)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorActionSearchTextRule)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorActionClassHierarchyScopeRule)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorActionVirtualFunctionRule)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorActionGraphTypeRule)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorActionOverrideFunctionRule)

    // View components
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditor)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorConnectionsDock)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorContextMenu)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorView) // todo: should be abstract internal
    GDREGISTER_INTERNAL_CLASS(OrchestratorScriptGraphEditorView)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorComponentView)
    GDREGISTER_INTERNAL_CLASS(OrchestratorScriptComponentsContainer)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGotoNodeDialog)
    GDREGISTER_INTERNAL_CLASS(OrchestratorUpdaterButton)
    GDREGISTER_INTERNAL_CLASS(OrchestratorUpdaterVersionPicker)
    GDREGISTER_INTERNAL_CLASS(OrchestratorUpdaterReleaseNotesDialog)
    GDREGISTER_INTERNAL_CLASS(OrchestratorAboutDialog)
    GDREGISTER_INTERNAL_CLASS(OrchestratorScreenSelect)
    GDREGISTER_INTERNAL_CLASS(OrchestratorWindowWrapper)
    GDREGISTER_INTERNAL_CLASS(OrchestratorGettingStarted)
    GDREGISTER_INTERNAL_CLASS(OrchestratorScriptConnectionsDialog)

    // Graph Classes
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorGraphPanel)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorGraphPanelStyler)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorGraphPanelKnotEditor)

    // Graph Node Type
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorGraphNode)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorGraphNodeComment)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorGraphNodeKnot)

    // Graph Pin Types
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorGraphPin)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorGraphPinButtonBase)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorGraphPinBitfield)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorGraphPinOptionPicker)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorGraphPinCheckbox)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorGraphPinColorPicker)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorGraphPinEnum)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorGraphPinExec)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorGraphPinFilePicker)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorGraphPinInputActionPicker)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorGraphPinLineEdit)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorGraphPinNodePath)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorGraphPinNumber)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorGraphPinObject)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorGraphPinTextEdit)
    GDREGISTER_INTERNAL_CLASS(OrchestratorEditorGraphPinStruct)

    // Add plugin to the editor
    EditorPlugins::add_by_type<OrchestratorPlugin>();
}

void unregister_editor_types() {
    // Remove plugin from the editor
    EditorPlugins::remove_by_type<OrchestratorPlugin>();
}

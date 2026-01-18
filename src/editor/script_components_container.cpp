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
#include "editor/script_components_container.h"

#include "common/callable_lambda.h"
#include "common/dictionary_utils.h"
#include "common/macros.h"
#include "common/name_utils.h"
#include "common/scene_utils.h"
#include "common/settings.h"
#include "core/godot/core_string_names.h"
#include "core/godot/scene_string_names.h"
#include "editor/editor.h"
#include "editor/editor_component_view.h"
#include "editor/graph/graph_panel.h"
#include "editor/gui/context_menu.h"
#include "editor/gui/dialogs_helper.h"
#include "editor/inspector/variable_inspector_plugin.h"
#include "editor/plugins/orchestrator_editor_plugin.h"
#include "editor/scene/connections_dock.h"
#include "editor/scene/script_connections.h"
#include "script/nodes/functions/function_entry.h"
#include "script/nodes/functions/function_result.h"
#include "script/script.h"

#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/scene_tree_timer.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

Ref<Orchestration> OrchestratorScriptComponentsContainer::_get_orchestration() {
    return _orchestration;
}

void OrchestratorScriptComponentsContainer::_open_graph(const String& p_graph_name) {
    emit_signal("open_graph_requested", p_graph_name);
}

void OrchestratorScriptComponentsContainer::_open_graph_with_focus(const String& p_graph_name, int p_node_id) {
    _open_graph(p_graph_name);
    call_deferred("emit_signal", "focus_node", p_node_id);
}

void OrchestratorScriptComponentsContainer::_close_graph(const String& p_graph_name) {
    emit_signal("close_graph_requested", p_graph_name);
}

void OrchestratorScriptComponentsContainer::_show_invalid_identifier(const String& p_name, bool p_friendly_names) {
    String message = vformat("The %s name is not valid. Names must follow these requirements:\n\n", p_name);
    message += "* Must start with a letter (A-Z, a-z) or an underscore ('_')\n";
    message += "* Can include letters (A-Z, a-z), numbers (0-9), and underscores ('_')\n";
    message += "* Should not start with a number (0-9)\n";
    message += "* Cannot contain spaces or special characters\n";

    if (p_friendly_names) {
        message += vformat("\nIf you want a space to appear in the %s name, please use camel-case (MyName).\n", p_name);
        message += "With friendly names enabled, the name will be rendered as 'My Name' automatically.";
    }

    ORCHESTRATOR_ACCEPT(message);
}

bool OrchestratorScriptComponentsContainer::_is_identifier_used(const String& p_name) {
    if (_get_orchestration()->has_variable(p_name)) {
        ORCHESTRATOR_ACCEPT_V(vformat("A %s already exists with the name \"%s\".", "variable", p_name), true);
    }

    if (_get_orchestration()->has_custom_signal(p_name)) {
        ORCHESTRATOR_ACCEPT_V(vformat("A %s already exists with the name \"%s\".", "signal", p_name), true);
    }

    if (_get_orchestration()->has_function(p_name)) {
        String item_name = _use_function_friendly_names ? p_name.capitalize() : p_name;
        ORCHESTRATOR_ACCEPT_V(vformat("A %s already exists with the name \"%s\".", "function", item_name), true);
    }

    if (_get_orchestration()->has_graph(p_name)) {
        String item_name = _use_graph_friendly_names ? p_name.capitalize() : p_name;
        ORCHESTRATOR_ACCEPT_V(vformat("A %s already exists with the name \"%s\".", "graph", item_name), true);
    }

    return false;
}

void OrchestratorScriptComponentsContainer::_component_show_context_menu(Node* p_node, TreeItem* p_item, const Vector2& p_position) {
    #define RENAME_ITEM(x, i) callable_mp(x, &OrchestratorEditorComponentView::rename_tree_item).bind(i, callable_mp_this(_component_rename_item))

    ERR_FAIL_NULL(p_item);
    ERR_FAIL_COND(!_orchestration.is_valid());

    OrchestratorEditorContextMenu* menu = memnew(OrchestratorEditorContextMenu);
    menu->set_auto_destroy(true);
    add_child(menu);

    const uint32_t type = p_item->get_meta("__component_type", NONE);
    switch (type) {
        case EVENT_GRAPH: {
            const Ref<OScriptGraph> graph = _get_orchestration()->get_graph(p_item->get_meta("__name", ""));
            const bool can_be_renamed = graph->get_flags().has_flag(OScriptGraph::GF_RENAMABLE);
            const bool can_be_removed = graph->get_flags().has_flag(OScriptGraph::GF_DELETABLE);

            menu->add_item("Open Graph", callable_mp_this(_open_graph).bind(graph->get_graph_name()), false, KEY_ENTER);
            menu->add_icon_item("Rename", "Rename", RENAME_ITEM(_graphs, p_item), !can_be_renamed, KEY_F2);
            menu->add_icon_item("Remove", "Remove", callable_mp_this(_component_remove_item).bind(p_item, true), !can_be_removed, KEY_DELETE);

            break;
        }
        case EVENT_GRAPH_FUNCTION: {
            const String function_name = p_item->get_meta("__name", "");

            menu->add_item("Focus", callable_mp_this(_component_focus_item).bind(p_item), false, KEY_ENTER);
            menu->add_icon_item("Remove", "Remove", callable_mp_this(_component_remove_item).bind(p_item, true), false, KEY_DELETE);
            if (p_item->get_meta("__slot", false)) {
                int32_t id = menu->add_icon_item("Unlinked", "Disconnect", callable_mp_this(_disconnect_slot_item).bind(p_item));
                menu->set_item_tooltip(id, "Disconnect the slot function from the signal.");
            }

            break;
        }
        case SCRIPT_FUNCTION: {
            const Ref<OScriptFunction> func = _get_orchestration()->find_function(p_item->get_meta("__name", ""));

            menu->add_item("Open In Graph", callable_mp_this(_open_graph).bind(func->get_function_name()), false, KEY_ENTER);
            menu->add_icon_item("Duplicate", "Duplicate", callable_mp_this(_component_duplicate_item).bind(p_item, DictionaryUtils::of({{ "include_code", "true" }})));
            menu->add_icon_item("Duplicate", "Duplicate (no_code)", callable_mp_this(_component_duplicate_item).bind(p_item, Dictionary()));
            menu->add_icon_item("Rename", "Rename", RENAME_ITEM(_functions, p_item), false, KEY_F2);
            menu->add_icon_item("Remove", "Remove", callable_mp_this(_component_remove_item).bind(p_item, true), false, KEY_DELETE);

            if (p_item->get_meta("__slot", false)) {
                int32_t id = menu->add_icon_item("Unlinked", "Disconnect", callable_mp_this(_disconnect_slot_item).bind(p_item));
                menu->set_item_tooltip(id, "Disconnect the slot function from the signal.");
            }

            break;
        }
        case SCRIPT_VARIABLE: {
            menu->add_icon_item("Duplicate", "Duplicate", callable_mp_this(_component_duplicate_item).bind(p_item, Dictionary()));
            menu->add_icon_item("Rename", "Rename", RENAME_ITEM(_variables, p_item), false, KEY_F2);
            menu->add_icon_item("Remove", "Remove", callable_mp_this(_component_remove_item).bind(p_item, true), false, KEY_DELETE);
            break;
        }
        case SCRIPT_SIGNAL: {
            menu->add_icon_item("Rename", "Rename", RENAME_ITEM(_signals, p_item), false, KEY_F2);
            menu->add_icon_item("Remove", "Remove", callable_mp_this(_component_remove_item).bind(p_item, true), false, KEY_DELETE);
            break;
        }
        default: {
            memdelete(menu);
            return;
        }
    }

    menu->set_position(p_position);
    menu->popup();

    #undef RENAME_ITEM
}

void OrchestratorScriptComponentsContainer::_component_item_gui_input(TreeItem* p_item, const Ref<InputEvent>& p_event) {
    ERR_FAIL_NULL(p_item);

    const Ref<InputEventKey> key = p_event;
    if (key.is_valid() && key->is_pressed() && !key->is_echo()) {
        switch (key->get_keycode()) {
            case KEY_F2: {
                bool can_be_renamed = p_item->get_meta("__can_be_renamed", true);
                if (!can_be_renamed) {
                    return;
                }

                // As we do not know the view, we need to fetch it
                Node* node = p_item->get_tree()->get_parent();
                OrchestratorEditorComponentView* view = cast_to<OrchestratorEditorComponentView>(node);
                if (view) {
                    view->rename_tree_item(p_item, callable_mp_this(_component_rename_item));
                    accept_event();
                }
                break;
            }
            case KEY_DELETE: {
                const bool can_be_removed = p_item->get_meta("__can_be_removed", true);
                if (!can_be_removed) {
                    return;
                }

                _component_remove_item(p_item);
                accept_event();
                break;
            }
            case KEY_ENTER: {
                _component_item_activated(nullptr, p_item);
                accept_event();
                break;
            }
            default: {
                break;
            }
        }
    }
}

Variant OrchestratorScriptComponentsContainer::_component_item_dragged(TreeItem* p_item, const Vector2& p_position) {
    ERR_FAIL_NULL_V(p_item, Variant());
    ERR_FAIL_COND_V(!_orchestration.is_valid(), Variant());

    const uint32_t component_type = p_item->get_meta("__component_type", NONE);

    Dictionary data;
    switch (component_type) {
        case SCRIPT_FUNCTION: {
            const StringName function_name = p_item->get_meta("__name", "");
            const Ref<OScriptFunction> func = _get_orchestration()->find_function(function_name);
            if (func.is_valid()) {
                data["type"] = "function";
                data["functions"] = DictionaryUtils::from_method(func->get_method_info());
            }
            break;
        }
        case SCRIPT_VARIABLE: {
            const StringName variable_name = p_item->get_meta("__name", "");
            const Ref<OScriptVariable> variable = _get_orchestration()->get_variable(variable_name);
            if (variable.is_valid()) {
                data["type"] = "variable";
                data["variables"] = Array::make(variable_name);
            }
            break;
        }
        case SCRIPT_SIGNAL: {
            const StringName signal_name = p_item->get_meta("__name", "");
            const Ref<OScriptSignal> signal = _get_orchestration()->find_custom_signal(signal_name);
            if (signal.is_valid()) {
                data["type"] = "signal";
                data["signals"] = DictionaryUtils::from_method(signal->get_method_info());
            }
            break;
        }
        default: {
            break;
        }
    }

    if (data.is_empty()) {
        return {};
    }

    // todo: improve the looks of this
    PanelContainer* container = memnew(PanelContainer);
    container->set_anchors_preset(PRESET_TOP_LEFT);
    container->set_v_size_flags(SIZE_SHRINK_BEGIN);

    HBoxContainer* hbc = memnew(HBoxContainer);
    hbc->set_v_size_flags(SIZE_SHRINK_CENTER);
    container->add_child(hbc);

    TextureRect* rect = memnew(TextureRect);
    rect->set_texture(p_item->get_icon(0));
    rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
    rect->set_h_size_flags(SIZE_SHRINK_CENTER);
    rect->set_v_size_flags(SIZE_SHRINK_CENTER);
    hbc->add_child(rect);

    Label* label = memnew(Label);
    label->set_text(p_item->get_meta("__name", ""));
    hbc->add_child(label);

    set_drag_preview(container);

    return data;
}

void OrchestratorScriptComponentsContainer::_component_item_button_clicked(Node* p_node, TreeItem* p_item, int p_column, int p_id, int p_button) {
    ERR_FAIL_NULL(p_item);
    ERR_FAIL_COND(!_orchestration.is_valid());

    const Ref<Script> script = _orchestration->as_script();
    ERR_FAIL_COND(!script.is_valid());

    const uint32_t type = p_item->get_meta("__component_type", NONE);
    const StringName item_name = p_item->get_meta("__name", "");

    switch (type) {
        case EVENT_GRAPH_FUNCTION:
        case SCRIPT_FUNCTION: {
            const Vector<Node*> scene_nodes = SceneUtils::find_all_nodes_for_script_in_edited_scene(script);

            OrchestratorScriptConnectionsDialog* dialog = memnew(OrchestratorScriptConnectionsDialog);
            add_child(dialog);

            dialog->popup_connections(p_item->get_meta("__name", ""), scene_nodes);
            break;
        }
        case SCRIPT_VARIABLE: {
            const Ref<OScriptVariable> variable = _get_orchestration()->get_variable(item_name);
            if (!variable.is_valid()) {
                return;
            }

            // p_id == 1 -> warning
            if (p_column == 0 && p_id == 2) {
                const Ref<OrchestratorEditorInspectorPluginVariable> plugin =
                    OrchestratorPlugin::get_singleton()->get_editor_inspector_plugin<OrchestratorEditorInspectorPluginVariable>();

                if (plugin.is_valid()) {
                    plugin->edit_classification(variable.ptr());
                }
            } else if (p_column == 0 && p_id == 3) {
                variable->set_exported(!variable->is_exported());
                _set_edited(true);
                callable_mp_this(_update_components).bind(SCRIPT_VARIABLE).call_deferred();
            }
            break;
        }
        default: {
            break;
        }
    }
}

void OrchestratorScriptComponentsContainer::_component_item_selected(Node* p_node, TreeItem* p_item) {
    ERR_FAIL_NULL(p_item);
    ERR_FAIL_COND(!_orchestration.is_valid());

    const StringName item_name = p_item->get_meta("__name", "");

    const uint32_t type = p_item->get_meta("__component_type", NONE);
    switch (type) {
        case EVENT_GRAPH_FUNCTION: {
            const Ref<OScriptFunction> function  = _get_orchestration()->find_function(item_name);
            if (function.is_valid()) {
                EI->edit_resource(function);
            }
            break;
        }
        case SCRIPT_FUNCTION: {
            const Ref<OScriptFunction> function = _get_orchestration()->find_function(item_name);
            if (function.is_valid()) {
                EI->edit_resource(function);
            }
            break;
        }
        case SCRIPT_VARIABLE: {
            const Ref<OScriptVariable> variable = _get_orchestration()->get_variable(item_name);
            if (variable.is_valid()) {
                EI->edit_resource(variable);
            }
            break;
        }
        case SCRIPT_SIGNAL: {
            const Ref<OScriptSignal> signal = _get_orchestration()->find_custom_signal(item_name);
            if (signal.is_valid()) {
                EI->edit_resource(signal);
            }
            break;
        }
        default: {
            break;
        }
    }
}

void OrchestratorScriptComponentsContainer::_component_item_activated(Node* p_node, TreeItem* p_item) {
    ERR_FAIL_NULL(p_item);

    const uint32_t type = p_item->get_meta("__component_type", NONE);
    switch (type) {
        case EVENT_GRAPH: {
            const String name = p_item->get_meta("__name", "");
            _open_graph(name);
            break;
        }
        case EVENT_GRAPH_FUNCTION:
        case SCRIPT_FUNCTION: {
            _component_focus_item(p_item);
            break;
        }
        default: {
            break;
        }
    }
}

void OrchestratorScriptComponentsContainer::_component_add_item(int p_component_type) {
    ERR_FAIL_COND_MSG(!_orchestration.is_valid(), "Cannot add component, orchestration is invalid");

    switch (p_component_type) {
        case EVENT_GRAPH: {
            const PackedStringArray existing_names = _get_orchestration()->get_graph_names();
            const String label = NameUtils::create_unique_name("NewEventGraph", existing_names);

            TreeItem* item = _graphs->add_tree_item(label, SceneUtils::get_editor_icon("ClassList"));
            item->set_meta("__component_type", EVENT_GRAPH);

            _graphs->edit_tree_item(item, callable_mp_this(_component_add_item_commit), callable_mp_this(_component_add_item_canceled));

            break;
        }
        case SCRIPT_FUNCTION: {
            const PackedStringArray existing_names = _get_orchestration()->get_function_names();
            const String label = NameUtils::create_unique_name("NewFunction", existing_names);

            bool any_functions = false;
            for (const Ref<OScriptFunction>& function : _get_orchestration()->get_functions()) {
                if (function.is_valid()) {
                    // Functions defined in event graphs will not have a function graph relationship
                    // And in such cases we need to exclude those as their names are returned in the names array
                    const Ref<OScriptGraph> graph = function->get_function_graph();
                    if (graph.is_valid() && graph->get_flags().has_flag(OScriptGraph::GF_FUNCTION)) {
                        any_functions = true;
                        break;
                    }
                }
            }

            if (!any_functions) {
                _functions->clear_tree();
            }

            TreeItem* item = _functions->add_tree_item(label, SceneUtils::get_editor_icon("MemberMethod"));
            item->set_meta("__component_type", SCRIPT_FUNCTION);

            _functions->edit_tree_item(item, callable_mp_this(_component_add_item_commit), callable_mp_this(_component_add_item_canceled));

            break;
        }
        case SCRIPT_VARIABLE: {
            const PackedStringArray existing_names = _get_orchestration()->get_variable_names();
            const String label = NameUtils::create_unique_name("NewVar", existing_names);

            if (existing_names.is_empty()) {
                _variables->clear_tree();
            }

            TreeItem* item = _variables->add_tree_item(label, SceneUtils::get_editor_icon("MemberProperty"));
            item->set_meta("__component_type", SCRIPT_VARIABLE);

            _variables->edit_tree_item(item, callable_mp_this(_component_add_item_commit), callable_mp_this(_component_add_item_canceled));
            break;
        }
        case SCRIPT_SIGNAL: {
            const PackedStringArray existing_names = _get_orchestration()->get_custom_signal_names();
            const String label = NameUtils::create_unique_name("NewSignal", existing_names);

            if (existing_names.is_empty()) {
                _signals->clear_tree();
            }

            TreeItem* item = _signals->add_tree_item(label, SceneUtils::get_editor_icon("MemberSignal"));
            item->set_meta("__component_type", SCRIPT_SIGNAL);

            _signals->edit_tree_item(item, callable_mp_this(_component_add_item_commit), callable_mp_this(_component_add_item_canceled));
            break;
        }
        default: {
            break;
        }
    }
}

void OrchestratorScriptComponentsContainer::_component_add_item_commit(TreeItem* p_item) {
    ScopedDeferredCallable sdc(callable_mp_this(_update_components).bind(COMPONENT_MAX));

    ERR_FAIL_NULL_MSG(p_item, "Cannot add component item with no tree item");
    ERR_FAIL_COND_MSG(!_orchestration.is_valid(), "Cannot add component item, orchestration is invalid");

    const String item_name = p_item->get_text(0);

    if (!item_name.is_valid_identifier()) {
        _show_invalid_identifier(item_name, _use_graph_friendly_names);
        return;
    }

    if (_is_identifier_used(item_name)) {
        return;
    }

    const uint32_t type = p_item->get_meta("__component_type", NONE);
    switch (type) {
        case EVENT_GRAPH: {
            if (_get_orchestration()->has_graph(item_name)) {
                ORCHESTRATOR_ACCEPT("A graph already exists with the name " + item_name);
            }

            // creation logic should be moved to a helper
            constexpr int32_t flags = OScriptGraph::GF_DEFAULT | OScriptGraph::GF_EVENT;
            if (!_get_orchestration()->create_graph(item_name, flags).is_valid()) {
                ORCHESTRATOR_ACCEPT("Failed to create scene event graph "+ item_name);
            }

            _set_edited(true);

            _open_graph(item_name);
            break;
        }
        case SCRIPT_FUNCTION: {
            if (_get_orchestration()->has_function(item_name) || _get_orchestration()->has_graph(item_name))
                ORCHESTRATOR_ACCEPT("A function already exists with the name " + item_name);

            // creation logic should be moved to a helper
            const uint32_t flags = OScriptGraph::GF_FUNCTION | OScriptGraph::GF_DEFAULT;
            const Ref<OScriptGraph> graph = _get_orchestration()->create_graph(item_name, flags);
            ERR_FAIL_COND_MSG(!graph.is_valid(), "Failed to create function graph named " + item_name);

            MethodInfo method;
            method.name = item_name;
            method.flags = METHOD_FLAG_NORMAL;
            method.return_val.type = Variant::NIL;
            method.return_val.hint = PROPERTY_HINT_NONE;
            method.return_val.usage = PROPERTY_USAGE_DEFAULT;

            OScriptNodeInitContext context;
            context.method = method;

            const Ref<OScriptNodeFunctionEntry> entry = graph->create_node<OScriptNodeFunctionEntry>(context);
            if (!entry.is_valid()) {
                _get_orchestration()->remove_graph(item_name);
                ORCHESTRATOR_ERROR("Failed to create function entry node in graph");
            }

            const Vector2 position = entry->get_position() + Vector2(300, 0);
            const Ref<OScriptNodeFunctionResult> result = graph->create_node<OScriptNodeFunctionResult>(context, position);
            if (!result.is_valid()) {
                _get_orchestration()->remove_graph(item_name);
                ORCHESTRATOR_ERROR("Failed to create function result node in graph");
            }

            _set_edited(true);

            // Connect the two execution pins
            entry->find_pin(0, PD_Output)->link(result->find_pin(0, PD_Input));

            _open_graph(item_name);

            // Because the editor is being opened, the focus needs to be deferred
            // or else the graph will open but the node won't be focused
            call_deferred("emit_signal", "scroll_to_center");

            break;
        }
        case SCRIPT_VARIABLE: {
            if (_get_orchestration()->has_variable(item_name)) {
                ORCHESTRATOR_ACCEPT("A variable already exists with the name " + item_name);
            }
            _set_edited(true);
            _get_orchestration()->create_variable(item_name);
            break;
        }
        case SCRIPT_SIGNAL: {
            if (_get_orchestration()->has_custom_signal(item_name)) {
                ORCHESTRATOR_ACCEPT("A signal already exists with the name " + item_name);
            }
            if (!_get_orchestration()->create_custom_signal(item_name).is_valid()) {
                ORCHESTRATOR_ACCEPT("Failed to create the signal with name " + item_name);
            }
            _set_edited(true);
            break;
        }
        default: {
            break;
        }
    }
}

void OrchestratorScriptComponentsContainer::_component_add_item_canceled(TreeItem* p_item) {
    ERR_FAIL_NULL(p_item);

    memdelete(p_item);

    _update_components();
}

void OrchestratorScriptComponentsContainer::_component_duplicate_item(TreeItem* p_item, const Dictionary& p_data) {
    ERR_FAIL_NULL_MSG(p_item, "Cannot duplicate component item with no tree item");

    const String name = p_item->get_meta("__name", "");
    const uint32_t type = p_item->get_meta("__component_type", NONE);

    switch (type) {
        case SCRIPT_FUNCTION: {
            const bool include_code = p_data.get("include_code", false);
            const Ref<OScriptFunction> duplicate = _get_orchestration()->duplicate_function(name, include_code);
            if (duplicate.is_valid()) {
                _open_graph_with_focus(duplicate->get_function_name(), duplicate->get_owning_node_id());
                _update_components();
                _find_and_edit_function(duplicate->get_function_name());
            }
            break;
        }
        case SCRIPT_VARIABLE: {
            const Ref<OScriptVariable> duplicate = _get_orchestration()->duplicate_variable(name);
            if (duplicate.is_valid()) {
                _update_components();
                _find_and_edit_variable(duplicate->get_variable_name());
            }
            break;
        }
        default:
            break;
    }
}

void OrchestratorScriptComponentsContainer::_component_rename_item(TreeItem* p_item) {
    ScopedDeferredCallable sdc(callable_mp_this(_update_components).bind(COMPONENT_MAX));

    ERR_FAIL_NULL_MSG(p_item, "Cannot rename component item with no tree item");
    ERR_FAIL_COND_MSG(!_orchestration.is_valid(), "Cannot rename component item, orchestration is invalid");

    const String old_name = p_item->get_meta("__original_name", "");
    const String new_name = p_item->get_text(0);

    if (old_name == new_name) {
        return;
    }

    if (!new_name.is_valid_identifier()) {
        _show_invalid_identifier(new_name, _use_graph_friendly_names);
        return;
    }

    if (_is_identifier_used(new_name)) {
        return;
    }

    const uint32_t type = p_item->get_meta("__component_type", NONE);
    switch (type) {
        case EVENT_GRAPH: {
            if (!_get_orchestration()->has_graph(old_name)) {
                ORCHESTRATOR_ACCEPT("No graph found with the name " + old_name);
            }
            if (_get_orchestration()->has_graph(new_name)) {
                ORCHESTRATOR_ACCEPT("A graph already exists with the name " + new_name);
            }
            if (!_get_orchestration()->rename_graph(old_name, new_name)) {
                ORCHESTRATOR_ACCEPT("Failed to rename event graph " + old_name);
            }
            _set_edited(true);
            break;
        }
        case SCRIPT_FUNCTION: {
            if (!_get_orchestration()->has_graph(old_name)) {
                ORCHESTRATOR_ACCEPT("No function graph found with the name " + old_name);
            }
            if (_get_orchestration()->has_graph(new_name) || _get_orchestration()->has_function(new_name)) {
                ORCHESTRATOR_ACCEPT("A function already exists with the name " + new_name);
            }
            if (!_get_orchestration()->rename_function(old_name, new_name)) {
                ORCHESTRATOR_ACCEPT("Failed to rename function graph " + old_name);
            }
            _set_edited(true);
            break;
        }
        case SCRIPT_VARIABLE: {
            if (!_get_orchestration()->has_variable(old_name)) {
                ORCHESTRATOR_ACCEPT("No variable found with the name " + old_name);
            }
            if (_get_orchestration()->has_variable(new_name)) {
                ORCHESTRATOR_ACCEPT("A variable already exists with the name " + new_name);
            }
            if (!_get_orchestration()->rename_variable(old_name, new_name)) {
                ORCHESTRATOR_ACCEPT("Failed to rename variable " + old_name);
            }
            _set_edited(true);
            break;
        }
        case SCRIPT_SIGNAL: {
            if (!_get_orchestration()->has_custom_signal(old_name)) {
                ORCHESTRATOR_ACCEPT("No signal found with the name " + old_name);
            }
            if (_get_orchestration()->has_custom_signal(new_name)) {
                ORCHESTRATOR_ACCEPT("A signal already exists with the name " + new_name);
            }
            if (!_get_orchestration()->rename_custom_user_signal(old_name, new_name)) {
                ORCHESTRATOR_ACCEPT("Failed to rename signal " + old_name);
            }
            _set_edited(true);
            break;
        }
        default: {
            break;
        }
    }
}

void OrchestratorScriptComponentsContainer::_component_remove_item(TreeItem* p_item, bool p_confirm) {
    ERR_FAIL_NULL_MSG(p_item, "Cannot remove component item with no tree item");
    ERR_FAIL_COND_MSG(!_orchestration.is_valid(), "Cannot component item, orchestration is invalid");

    const int32_t component_type = p_item->get_meta("__component_type", NONE);
    const StringName item_name = p_item->get_meta("__name", "");

    if (p_confirm) {
        String text;
        switch (component_type) {
            case EVENT_GRAPH: {
                text = "Removing a graph removes all nodes within the graph.";
                break;
            }
            case SCRIPT_FUNCTION: {
                text = "Removing a function removes all nodes that participate in the function and any nodes\n"
                       "that call that function from the event graphs.";
                break;
            }
            case SCRIPT_VARIABLE: {
                text = "Removing a variable will remove all nodes that get or set the variable.";
                break;
            }
            case SCRIPT_SIGNAL: {
                text = "Removing a signal will remove all nodes that emit the signal.";
                break;
            }
            default: {
                break;
            }
        }

        if (!text.is_empty()) {
            ORCHESTRATOR_CONFIRM(vformat("%s\n\nDo you want to continue?", text),
                callable_mp_this(_component_remove_item).bind(p_item, false));
        }
    }

    ScopedDeferredCallable sdc(callable_mp_this(_update_components).bind(COMPONENT_MAX));

    switch (component_type) {
        case EVENT_GRAPH: {
            const Ref<OScriptGraph> graph = _get_orchestration()->get_graph(item_name);
            if (!graph.is_valid()) {
                ORCHESTRATOR_ACCEPT("No graph found with the name " + item_name);
            }

            _set_edited(true);

            _close_graph(item_name);
            _get_orchestration()->remove_graph(item_name);

            break;
        }
        case EVENT_GRAPH_FUNCTION: {
            if (_get_orchestration()->has_function(item_name)) {
                _set_edited(true);
                _get_orchestration()->remove_function(item_name);
            }
            break;
        }
        case SCRIPT_FUNCTION: {
            const Ref<OScriptFunction> function = _get_orchestration()->find_function(item_name);
            if (!function.is_valid()) {
                ORCHESTRATOR_ACCEPT("No function found with the name " + item_name);
            }

            _set_edited(true);
            _close_graph(item_name);
            _get_orchestration()->remove_function(item_name);

            break;
        }
        case SCRIPT_VARIABLE: {
            const Ref<OScriptVariable> variable = _get_orchestration()->get_variable(item_name);
            if (!variable.is_valid()) {
                ORCHESTRATOR_ACCEPT("No variable found with the name " + item_name);
            }

            _set_edited(true);
            _get_orchestration()->remove_variable(variable->get_variable_name());
            break;
        }
        case SCRIPT_SIGNAL: {
            const Ref<OScriptSignal> signal = _get_orchestration()->get_custom_signal(item_name);
            if (!signal.is_valid()) {
                ORCHESTRATOR_ACCEPT("No signal found with the name " + item_name);
            }

            _set_edited(true);
            _get_orchestration()->remove_custom_signal(signal->get_signal_name());
            break;
        }
        default: {
            break;
        }
    }

    // Clear the inspected object after removal
    switch (component_type) {
        case EVENT_GRAPH_FUNCTION:
        case SCRIPT_FUNCTION:
        case SCRIPT_VARIABLE:
        case SCRIPT_SIGNAL: {
            EI->inspect_object(nullptr);
            break;
        }
        default: {
            break;
        }
    }
}

void OrchestratorScriptComponentsContainer::_component_focus_item(TreeItem* p_item) {
    ERR_FAIL_NULL_MSG(p_item, "Cannot focus component item with no tree item");
    ERR_FAIL_COND_MSG(!_orchestration.is_valid(), "Cannot focus component item, orchestration is invalid");

    const uint32_t type = p_item->get_meta("__component_type", NONE);
    switch (type) {
        case EVENT_GRAPH_FUNCTION: {
            const String graph_name = p_item->get_meta("__graph_name", "EventGraph");
            const int node_id = static_cast<int>(String(p_item->get_meta("__node_id", -1)).to_int());
            _open_graph_with_focus(graph_name, node_id);
            break;
        }
        case SCRIPT_FUNCTION: {
            const StringName function_name = p_item->get_meta("__name", "");
            const int node_id = static_cast<int>(String(p_item->get_meta("__node_id", -1)).to_int());
            _open_graph_with_focus(function_name, node_id);
            break;
        }
        default:
            break;
    }
}

void OrchestratorScriptComponentsContainer::_update_components(int p_component_type) {
    if (!_orchestration.is_valid()) {
        return;
    }

    switch (p_component_type) {
        case EVENT_GRAPH:
        case EVENT_GRAPH_FUNCTION:
        case SCRIPT_FUNCTION: {
            _update_graphs_and_functions();
            break;
        }
        case SCRIPT_MACRO: {
            _update_macros();
            break;
        }
        case SCRIPT_VARIABLE: {
            _update_variables();
            break;
        }
        case SCRIPT_SIGNAL: {
            _update_signals();
            break;
        }
        default: {
            _update_graphs_and_functions();
            _update_macros();
            _update_variables();
            _update_signals();
            break;
        }
    }
}

void OrchestratorScriptComponentsContainer::_find_and_edit_function(const String& p_function_name) {
    TreeItem* item = _functions->find_item(p_function_name);
    if (item) {
        _functions->rename_tree_item(item, callable_mp_this(_component_rename_item));
    }
}

void OrchestratorScriptComponentsContainer::_find_and_edit_variable(const String& p_variable_name) {
    TreeItem* item = _variables->find_item(p_variable_name);
    if (item) {
        _variables->rename_tree_item(item, callable_mp_this(_component_rename_item));
    }
}

void OrchestratorScriptComponentsContainer::_update_graphs_and_functions() {
    ERR_FAIL_COND_MSG(!_orchestration.is_valid(), "Orchestration is invalid");

    _graphs->clear_tree();
    _functions->clear_tree();

    PackedStringArray graph_names = _get_orchestration()->get_graph_names();
    graph_names.sort();

    // Always guarantee that "EventGraph" is at the top
    if (graph_names.has("EventGraph")) {
        #if GODOT_VERSION >= 0x040500
        graph_names.erase("EventGraph");
        #else
        graph_names.remove_at(graph_names.find("EventGraph"));
        #endif
        graph_names.insert(0, "EventGraph");
    }

    PackedStringArray function_names = _get_orchestration()->get_function_names();
    function_names.sort();

    //~ Populate graphs and functions panels
    const Ref<Texture2D> graph_icon = SceneUtils::get_editor_icon("ClassList");
    const Ref<Texture2D> event_icon = SceneUtils::get_editor_icon("PlayStart");
    const Ref<Texture2D> function_icon = SceneUtils::get_editor_icon("MemberMethod");
    for (const String& graph_name : graph_names) {
        const Ref<OScriptGraph>& script_graph = _get_orchestration()->get_graph(graph_name);
        if (script_graph->get_flags().has_flag(OScriptGraph::GF_EVENT)) {
            String name = script_graph->get_graph_name();
            if (_use_graph_friendly_names) {
                name = name.capitalize();
            }

            TreeItem* graph = _graphs->add_tree_fancy_item(name, script_graph->get_graph_name(), graph_icon);
            graph->set_meta("__component_type", EVENT_GRAPH);

            if (!script_graph->get_flags().has_flag(OScriptGraph::GF_DELETABLE)) {
                graph->set_meta("__can_be_removed", false);
            }

            if (!script_graph->get_flags().has_flag(OScriptGraph::GF_RENAMABLE)) {
                graph->set_meta("__can_be_renamed", false);
            }

            for (const String& function_name : function_names) {
                int function_id = _get_orchestration()->get_function_node_id(function_name);
                if (script_graph->has_node(function_id)) {
                    name = function_name;
                    if (_use_graph_friendly_names) {
                        name = vformat("%s Event", name.capitalize());
                    }

                    TreeItem* item = _graphs->add_tree_fancy_item(name, function_name, event_icon, graph);
                    item->set_meta("__component_type", EVENT_GRAPH_FUNCTION);
                    item->set_meta("__graph_name", script_graph->get_graph_name());
                    item->set_meta("__node_id", function_id);

                    const Ref<OScriptFunction> function = _get_orchestration()->find_function(StringName(function_name));
                    if (function.is_valid() && !function->is_user_defined()) {
                        item->set_meta("__can_be_renamed", false);
                    }
                }
            }
        } else if (script_graph->get_flags().has_flag(OScriptGraph::GF_FUNCTION)) {
            int function_id = _get_orchestration()->get_function_node_id(script_graph->get_graph_name());

            String name = script_graph->get_graph_name();
            if (_use_function_friendly_names) {
                name = name.capitalize();
            }

            TreeItem* item = _functions->add_tree_fancy_item(name, script_graph->get_graph_name(), function_icon);
            item->set_meta("__component_type", SCRIPT_FUNCTION);
            item->set_meta("__node_id", function_id);
        }
    }

    _graphs->add_tree_empty_item("No graphs defined");
    _functions->add_tree_empty_item("No functions defined");

    callable_mp_this(_update_slots).call_deferred();
}

void OrchestratorScriptComponentsContainer::_update_macros() {
    _macros->clear_tree();
    _macros->add_tree_empty_item("No macros defined");
}

void OrchestratorScriptComponentsContainer::_update_variables() {
    ERR_FAIL_COND_MSG(!_orchestration.is_valid(), "Orchestration is invalid");

    _variables->clear_tree();

    //~ Populate variables component panel
    const Vector<Ref<OScriptVariable>> variables = _get_orchestration()->get_variables();
    if (variables.is_empty()) {
        _variables->add_tree_empty_item("No variables defined");
        return;
    }

    // Pass 1: Construct lists of category / variable names
    PackedStringArray category_names;
    PackedStringArray variable_names;
    HashMap<String, Ref<OScriptVariable>> variable_map;
    for (const Ref<OScriptVariable>& variable : variables) {
        if (variable->is_grouped_by_category() && !category_names.has(variable->get_category())) {
            category_names.push_back(variable->get_category());
        }
        variable_names.push_back(variable->get_variable_name());
        variable_map[variable->get_variable_name()] = variable;
    }
    category_names.sort();
    variable_names.sort();

    // Pass 2: Create categories
    HashMap<String, TreeItem*> categories;
    for (const String& category_name : category_names) {
        TreeItem* item = _variables->add_tree_item(category_name);
        if (item) {
            categories[category_name] = item;
        }
    }

    // Pass 3: Create variables
    const Ref<Texture2D> variable_icon = SceneUtils::get_editor_icon("MemberProperty");
    for (const String& variable_name : variable_names) {
        const Ref<OScriptVariable>& variable = variable_map[variable_name];

        // Any existing variables should be connected to this function, to refresh the
        // view whenever any variable data changes.
        if (!variable->is_connected(CoreStringName(changed), callable_mp_this(_update_variables))) {
            variable->connect(CoreStringName(changed), callable_mp_this(_update_variables));
        }

        TreeItem* parent = nullptr;
        if (variable->is_grouped_by_category()) {
            parent = categories[variable->get_category()];
        }

        TreeItem* item = _variables->add_tree_item(variable_name, variable_icon, parent);
        item->set_meta("__component_type", SCRIPT_VARIABLE);

        if (variable->is_exported() && variable->get_variable_name().begins_with("_")) {
            int32_t index = item->get_button_count(0);
            item->add_button(0, SceneUtils::get_editor_icon("NodeWarning"), 1);
            item->set_button_tooltip_text(0, index, "Variable is exported but defined as private using underscore prefix.");
            item->set_button_disabled(0, index, true);
        }

        {
            // There is no way to set the size of the image on the button, so we must rescale
            Ref<Texture2D> class_icon = SceneUtils::get_class_icon(variable->get_variable_type_name());
            const Ref<Image> image = class_icon->get_image();
            image->resize(SceneUtils::get_editor_class_icon_size(), SceneUtils::get_editor_class_icon_size());
            class_icon = ImageTexture::create_from_image(image);

            int32_t index = item->get_button_count(0);
            item->add_button(0, class_icon, 2);
            item->set_button_tooltip_text(0, index, "Change variable type");
        }

        if (!variable->get_description().is_empty()) {
            const String tooltip = variable->get_variable_name() + "\n\n" + variable->get_description();
            item->set_tooltip_text(0, SceneUtils::create_wrapped_tooltip_text(tooltip));
        }

        if (variable->is_exported()) {
            int32_t index = item->get_button_count(0);
            item->add_button(0, SceneUtils::get_editor_icon("GuiVisibilityVisible"), 3);
            item->set_button_tooltip_text(0, index, "Variable is exported and can be modified in the inspector.");
            item->set_button_disabled(0, index, false);
        } else if (variable->is_constant()) {
            int32_t index = item->get_button_count(0);
            item->add_button(0, SceneUtils::get_editor_icon("MemberConstant"), 4);
            item->set_button_tooltip_text(0, index, "Variable is a constant.");
            item->set_button_disabled(0, index, false);
        } else {
            String tooltip_text = "Variable is not exported and only visible to scripts.";
            if (!variable->is_exportable()) {
                tooltip_text += "\nType cannot be exported.";
            }
            int32_t index = item->get_button_count(0);
            item->add_button(0, SceneUtils::get_editor_icon("GuiVisibilityHidden"), 3);
            item->set_button_tooltip_text(0, index, tooltip_text);
            item->set_button_disabled(0, index, !variable->is_exportable());
        }
    }
}

void OrchestratorScriptComponentsContainer::_update_signals() {
    ERR_FAIL_COND_MSG(!_orchestration.is_valid(), "Orchestration is invalid");

    _signals->clear_tree();

    PackedStringArray signal_names = _get_orchestration()->get_custom_signal_names();
    if (signal_names.is_empty()) {
        _signals->add_tree_empty_item("No signals defined");
        return;
    }

    signal_names.sort();

    const Ref<Texture2D> signal_icon = SceneUtils::get_editor_icon("MemberSignal");
    for (const String& signal_name: signal_names) {
        const Ref<OScriptSignal> signal = _get_orchestration()->get_custom_signal(signal_name);
        TreeItem* item = _signals->add_tree_item(signal->get_signal_name(), signal_icon);
        item->set_meta("__component_type", SCRIPT_SIGNAL);
    }
}

void OrchestratorScriptComponentsContainer::_update_slots() {
    ERR_FAIL_COND(!_orchestration.is_valid());

    _graphs->for_each_item(callable_mp_this(_update_slot_item));
    _functions->for_each_item(callable_mp_this(_update_slot_item));
}

void OrchestratorScriptComponentsContainer::_update_slot_item(TreeItem* p_item) {
    ERR_FAIL_COND(!p_item);
    ERR_FAIL_COND(!_orchestration.is_valid());

    const Ref<OScript> script = _orchestration->as_script();
    ERR_FAIL_COND(!script.is_valid());

    const String base_type = script->get_instance_base_type();
    const Vector<Node*> nodes = SceneUtils::find_all_nodes_for_script_in_edited_scene(script);

    const uint32_t component_type = p_item->get_meta("__component_type", NONE);
    switch (component_type) {
        case EVENT_GRAPH_FUNCTION:
        case SCRIPT_FUNCTION: {
            const String function_name = p_item->get_meta("__name", "");
            if (SceneUtils::has_any_signals_connected_to_function(function_name, base_type, nodes)) {
                if (p_item->get_button_count(0) == 0) {
                    p_item->add_button(0, SceneUtils::get_editor_icon("Slot"));
                    p_item->set_button_tooltip_text(0, 0, "A signal is connected.");
                    p_item->set_meta("__slot", true);
                }
            } else if (p_item->get_button_count(0) > 0) {
                p_item->erase_button(0, 0);
                p_item->remove_meta("__slot");
            }
            break;
        }
        default: {
            break;
        }
    }
}

void OrchestratorScriptComponentsContainer::_disconnect_slot_item(TreeItem* p_item) {
    ERR_FAIL_COND(!p_item);

    const Ref<OScript> script = _orchestration->as_script();
    ERR_FAIL_COND(!script.is_valid());

    const String method_name = p_item->get_meta("__name", "");
    if (OrchestratorEditorConnectionsDock::get_singleton()->disconnect_slot(script, method_name)) {
        _update_slot_item(p_item);
    }
}

void OrchestratorScriptComponentsContainer::_scene_changed(Node* p_node) {
    _update_slots();
}

void OrchestratorScriptComponentsContainer::_project_settings_changed() {
    bool use_friendly_graph_names = ORCHESTRATOR_GET("ui/components_panel/show_graph_friendly_names", true);
    bool use_friendly_function_names = ORCHESTRATOR_GET("ui/components_panel/show_function_friendly_names", true);

    bool components_require_update =
        (use_friendly_function_names != _use_function_friendly_names)
        || (use_friendly_graph_names != _use_graph_friendly_names);

    _use_function_friendly_names = use_friendly_function_names;
    _use_graph_friendly_names = use_friendly_graph_names;

    if (components_require_update) {
        _update_components();
    }

    const bool components_visible = PROJECT_GET("Orchestrator", "component_panel_visibility", true);
    set_visible(components_visible);
}

void OrchestratorScriptComponentsContainer::_set_edited(bool p_edited) {
    if (_orchestration.is_valid()) {
        _orchestration->set_edited(p_edited);
        emit_signal("validate_script");
    }
}

void OrchestratorScriptComponentsContainer::set_edited_resource(const Ref<Resource>& p_resource) {
    const Ref<OScript> script = p_resource;
    ERR_FAIL_COND_MSG(!script.is_valid(), "Could not set the orchestration");

    if (script.is_valid()) {
        _orchestration = script->get_orchestration();
    }
}

Dictionary OrchestratorScriptComponentsContainer::get_edit_state() {
    Dictionary panel_states;
    panel_states["graphs"] = _graphs->is_collapsed();
    panel_states["functions"] = _functions->is_collapsed();
    panel_states["macros"] = _macros->is_collapsed();
    panel_states["variables"] = _variables->is_collapsed();
    panel_states["signals"] = _signals->is_collapsed();
    return panel_states;
}

void OrchestratorScriptComponentsContainer::set_edit_state(const Variant& p_state) {
    Dictionary state = p_state;
    if (!state.is_empty()) {
        Dictionary panel_states = state.get("panels", Dictionary());
        _graphs->set_collapsed(panel_states.get("graphs", false));
        _functions->set_collapsed(panel_states.get("functions", false));
        _macros->set_collapsed(panel_states.get("macros", false));
        _variables->set_collapsed(panel_states.get("variables", false));
        _signals->set_collapsed(panel_states.get("signals", false));
    }
}

void OrchestratorScriptComponentsContainer::update() {
    _update_components(COMPONENT_MAX);
}

void OrchestratorScriptComponentsContainer::notify_graph_opened(OrchestratorEditorGraphPanel* p_graph) {
    ERR_FAIL_NULL(p_graph);

    p_graph->connect("nodes_changed", callable_mp_this(update));
    p_graph->connect("edit_function_requested", callable_mp_this(_find_and_edit_function));
}

void OrchestratorScriptComponentsContainer::_bind_methods() {
    ADD_SIGNAL(MethodInfo("open_graph_requested", PropertyInfo(Variant::STRING, "graph_name")));
    ADD_SIGNAL(MethodInfo("close_graph_requested", PropertyInfo(Variant::STRING, "graph_name")));
    ADD_SIGNAL(MethodInfo("scroll_to_center"));
    ADD_SIGNAL(MethodInfo("focus_node", PropertyInfo(Variant::INT, "node")));
    ADD_SIGNAL(MethodInfo("add_function_override_requested"));
    ADD_SIGNAL(MethodInfo("validate_script"));
}

OrchestratorScriptComponentsContainer::OrchestratorScriptComponentsContainer() {
    set_horizontal_scroll_mode(SCROLL_MODE_DISABLED);
    set_vertical_scroll_mode(SCROLL_MODE_AUTO);

    VBoxContainer* components = memnew(godot::VBoxContainer);
    components->set_h_size_flags(SIZE_EXPAND_FILL);
    add_child(components);

    _graphs = memnew(OrchestratorEditorComponentView);
    _graphs->set_title("Graphs");
    _graphs->set_tree_drag_forward(callable_mp_this(_component_item_dragged));
    _graphs->set_tree_gui_handler(callable_mp_this(_component_item_gui_input));
    _graphs->connect("add_requested", callable_mp_this(_component_add_item).bind(EVENT_GRAPH));
    _graphs->connect("context_menu_requested", callable_mp_this(_component_show_context_menu));
    _graphs->connect(SceneStringName(item_selected), callable_mp_this(_component_item_selected));
    _graphs->connect(SceneStringName(item_activated), callable_mp_this(_component_item_activated));
    _graphs->connect("item_button_clicked", callable_mp_this(_component_item_button_clicked));
    _graphs->set_panel_tooltip(SceneUtils::create_wrapped_tooltip_text(
        "A graph allows you to place many types of nodes to create various behaviors. "
        "Event graphs are flexible and can control multiple event nodes that start execution, "
        "nodes that may take time, react to signals, or call functions and macro nodes.\n\n"
        "While there is always one event graph called \"EventGraph\", you can create new "
        "event graphs to better help organize event logic."));
    components->add_child(_graphs);

    Button* add_function_override = memnew(Button);
    add_function_override->set_focus_mode(FOCUS_NONE);
    add_function_override->set_button_icon(SceneUtils::get_editor_icon("Override"));
    add_function_override->set_tooltip_text("Override a Godot virtual function");
    add_function_override->connect(SceneStringName(pressed), callable_mp_signal_lambda("add_function_override_requested"));

    _functions = memnew(OrchestratorEditorComponentView);
    _functions->set_title("Functions");
    _functions->set_tree_drag_forward(callable_mp_this(_component_item_dragged));
    _functions->set_tree_gui_handler(callable_mp_this(_component_item_gui_input));
    _functions->add_button(add_function_override);
    _functions->connect("add_requested", callable_mp_this(_component_add_item).bind(SCRIPT_FUNCTION));
    _functions->connect("context_menu_requested", callable_mp_this(_component_show_context_menu));
    _functions->connect(SceneStringName(item_selected), callable_mp_this(_component_item_selected));
    _functions->connect(SceneStringName(item_activated), callable_mp_this(_component_item_activated));
    _functions->connect("item_button_clicked", callable_mp_this(_component_item_button_clicked));
    _functions->set_panel_tooltip(SceneUtils::create_wrapped_tooltip_text(
        "A function graph allows the encapsulation of functionality for re-use. Function graphs have "
        "a single input with an optional output node. Function graphs have a single execution pin "
        "with multiple input data pins and the result node may return a maximum of one data value to "
        "the caller.\n\n"
        "Functions can be called by selecting the action in the action menu or by dragging the "
        "function from this component view onto the graph area."));
    components->add_child(_functions);

    _macros = memnew(OrchestratorEditorComponentView);
    _macros->set_title("Macros");
    _macros->set_tree_drag_forward(callable_mp_this(_component_item_dragged));
    _macros->set_tree_gui_handler(callable_mp_this(_component_item_gui_input));
    _macros->set_add_button_disabled(true);
    _macros->set_panel_tooltip(SceneUtils::create_wrapped_tooltip_text(
        "A macro graph allows for the encapsulation of functionality for re-use. Macros have both a "
        "singular input and output node, but these nodes can have as many input or output data "
        "values needed for logic. Macros can contain nodes that take time, such as delays, but are "
        "not permitted to contain event nodes, such as a node that reacts to '_ready'.\n\n"
        "This feature is currently disabled and will be available in a future release."));
    components->add_child(_macros);

    _variables = memnew(OrchestratorEditorComponentView);
    _variables->set_title("Variables");
    _variables->set_tree_drag_forward(callable_mp_this(_component_item_dragged));
    _variables->set_tree_gui_handler(callable_mp_this(_component_item_gui_input));
    _variables->connect("add_requested", callable_mp_this(_component_add_item).bind(SCRIPT_VARIABLE));
    _variables->connect("context_menu_requested", callable_mp_this(_component_show_context_menu));
    _variables->connect(SceneStringName(item_selected), callable_mp_this(_component_item_selected));
    _variables->connect(SceneStringName(item_activated), callable_mp_this(_component_item_activated));
    _variables->connect("item_button_clicked", callable_mp_this(_component_item_button_clicked));
    _variables->set_panel_tooltip(SceneUtils::create_wrapped_tooltip_text(
        "A variable represents some data that will be stored and managed by the orchestration.\n\n"
        "Drag a variable from the component view onto the graph area to select whether to create "
        "a get/set node or use the action menu to find the get/set option for the variable.\n\n"
        "Selecting a variable in the component view displays the variable details in the inspector."));
    components->add_child(_variables);

    _signals = memnew(OrchestratorEditorComponentView);
    _signals->set_title("Signals");
    _signals->set_tree_drag_forward(callable_mp_this(_component_item_dragged));
    _signals->set_tree_gui_handler(callable_mp_this(_component_item_gui_input));
    _signals->connect("add_requested", callable_mp_this(_component_add_item).bind(SCRIPT_SIGNAL));
    _signals->connect("context_menu_requested", callable_mp_this(_component_show_context_menu));
    _signals->connect(SceneStringName(item_selected), callable_mp_this(_component_item_selected));
    _signals->connect(SceneStringName(item_activated), callable_mp_this(_component_item_activated));
    _signals->connect("item_button_clicked", callable_mp_this(_component_item_button_clicked));
    _signals->set_panel_tooltip(SceneUtils::create_wrapped_tooltip_text(
        "A signal is used to send a notification synchronously to any number of observers that have "
        "connected to the defined signal on the orchestration. Signals allow for a variable number "
        "of arguments to be passed to the observer.\n\n"
        "Selecting a signal in the component view displays the signal details in the inspector."));
    components->add_child(_signals);

    OrchestratorEditor::get_singleton()->connect("scene_changed", callable_mp_this(_scene_changed));
    ProjectSettings::get_singleton()->connect("settings_changed", callable_mp_this(_project_settings_changed));
    OrchestratorEditorConnectionsDock::get_singleton()->connect(CoreStringName(changed), callable_mp_this(_update_slots));

    _project_settings_changed();
}

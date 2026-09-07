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
#include "common/string_utils.h"
#include "core/godot/config/project_settings_cache.h"
#include "core/godot/core_string_names.h"
#include "core/godot/scene_string_names.h"
#include "editor/editor.h"
#include "editor/editor_component_view.h"
#include "editor/graph/graph_panel.h"
#include "editor/gui/clipboard_conflict_dialog.h"
#include "editor/gui/context_menu.h"
#include "editor/gui/dialogs_helper.h"
#include "editor/inspector/properties/type_selector.h"
#include "editor/plugins/orchestrator_editor_plugin.h"
#include "editor/scene/connections_dock.h"
#include "editor/scene/script_connections.h"
#include "editor/settings/editor_settings.h"
#include "orchestration/annotation_registry.h"
#include "orchestration/nodes/function_entry.h"
#include "orchestration/nodes/function_result.h"
#include "script/script.h"

#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

Ref<Orchestration> OrchestratorScriptComponentsContainer::_get_orchestration() {
    return _orchestration;
}

// todo: remove when https://github.com/godotengine/godot/issues/117458 is fixed
bool OrchestratorScriptComponentsContainer::_make_inspector_dock_visible() const {
    const TypedArray<Node> docks = EditorNode->find_children("Inspector", "InspectorDock", true, false);
    ERR_FAIL_COND_V(docks.is_empty(), false);

    Control* inspector = cast_to<Control>(docks[0]);
    ERR_FAIL_NULL_V(inspector, false);

    Control* parent = inspector->get_parent_control();
    ERR_FAIL_NULL_V(parent, false);

    if (!parent->get_name().begins_with("DockSlot")) {
        // Floating or invisible
        if (!inspector->is_visible()) {
            ERR_FAIL_V_MSG("Cannot make inspector visible, it isn't toggled", false);
            return false;
        }
    }

    // Inspector is docked
    const TypedArray<Node> tabs = parent->find_children("*", "TabBar", true, false);
    ERR_FAIL_COND_V(tabs.is_empty(), false);

    TabBar* tab_bar = cast_to<TabBar>(tabs[0]);
    ERR_FAIL_NULL_V(tab_bar, false);

    tab_bar->set_current_tab(inspector->get_index());
    return true;
}

void OrchestratorScriptComponentsContainer::_functions_changed() {
    _set_edited(true);
}

void OrchestratorScriptComponentsContainer::_variables_changed() {
    _set_edited(true);
    _update_variables();
}

void OrchestratorScriptComponentsContainer::_local_variable_changed() {
    _set_edited(true);
    _update_local_variables();
}

void OrchestratorScriptComponentsContainer::_local_variable_added(const StringName& p_name) {
    _update_local_variables();
}

void OrchestratorScriptComponentsContainer::_local_variable_removed(const StringName& p_name) {
    _update_local_variables();
}

void OrchestratorScriptComponentsContainer::_local_variable_renamed(const StringName& p_old_name, const StringName& p_new_name) {
    _update_local_variables();
}

void OrchestratorScriptComponentsContainer::_set_active_function(const Ref<OScriptFunction>& p_function) {
    if (_active_function == p_function) {
        return;
    }

    if (_active_function.is_valid()) {
        _active_function->disconnect("local_variable_added", callable_mp_this(_local_variable_added));
        _active_function->disconnect("local_variable_removed", callable_mp_this(_local_variable_removed));
        _active_function->disconnect("local_variable_renamed", callable_mp_this(_local_variable_renamed));
    }

    _active_function = p_function;

    if (_active_function.is_valid()) {
        _active_function->connect("local_variable_added", callable_mp_this(_local_variable_added));
        _active_function->connect("local_variable_removed", callable_mp_this(_local_variable_removed));
        _active_function->connect("local_variable_renamed", callable_mp_this(_local_variable_renamed));
    }

    // The view is scoped to the active function, so it must repopulate even though no model changed
    _update_local_variables();
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

    OrchestratorEditorContextMenu* menu = OrchestratorEditorContextMenu::create(this);

    const uint32_t type = p_item->get_meta("__component_type", NONE);
    switch (type) {
        case EVENT_GRAPH: {
            const Ref<OScriptGraph> graph = _get_orchestration()->get_graph(p_item->get_meta("__name", ""));
            const bool can_be_renamed = graph->get_flags().has_flag(OScriptGraph::GF_RENAMABLE);
            const bool can_be_removed = graph->get_flags().has_flag(OScriptGraph::GF_DELETABLE);

            menu->add_shortcut(ED_GET_SHORTCUT("graph_components_panel/open"), callable_mp_this(_open_graph).bind(graph->get_graph_name()));
            menu->add_icon_shortcut("Rename", ED_GET_SHORTCUT("graph_components_panel/rename"), RENAME_ITEM(_graphs, p_item), { .disabled = !can_be_renamed });
            menu->add_icon_shortcut("Remove", ED_GET_SHORTCUT("graph_components_panel/remove"), callable_mp_this(_component_remove_item).bind(p_item, true), { .disabled = !can_be_removed });

            break;
        }
        case EVENT_GRAPH_FUNCTION: {
            const String function_name = p_item->get_meta("__name", "");

            menu->add_shortcut(ED_GET_SHORTCUT("graph_components_panel/focus"), callable_mp_this(_component_focus_item).bind(p_item));
            menu->add_icon_shortcut("Remove", ED_GET_SHORTCUT("graph_components_panel/remove"), callable_mp_this(_component_remove_item).bind(p_item, true));
            menu->add_icon_shortcut(
                "Unlinked",
                ED_GET_SHORTCUT("graph_components_panel/disconnect_signal"),
                callable_mp_this(_disconnect_slot_item).bind(p_item),
                {
                    .visible = p_item->get_meta("__slot", false),
                    .tooltip = "Disconnect the slot function from the signal."
                });

            break;
        }
        case SCRIPT_FUNCTION: {
            const Ref<OScriptFunction> func = _get_orchestration()->find_function(p_item->get_meta("__name", ""));

            menu->add_shortcut(ED_GET_SHORTCUT("graph_components_panel/open"), callable_mp_this(_open_graph).bind(func->get_function_name()));
            menu->add_icon_shortcut("Duplicate", ED_GET_SHORTCUT("graph_components_panel/duplicate"), callable_mp_this(_component_duplicate_item).bind(p_item, DictionaryUtils::of({{ "include_code", "true" }})));
            menu->add_icon_shortcut("Duplicate", ED_GET_SHORTCUT("graph_components_panel/duplicate_without_code"), callable_mp_this(_component_duplicate_item).bind(p_item, Dictionary()));
            menu->add_icon_shortcut("ActionCopy", ED_ACTION_SHORTCUT("ui_copy", "Copy"), callable_mp_this(_component_copy_item).bind(p_item));
            menu->add_icon_shortcut("ActionPaste", ED_ACTION_SHORTCUT("ui_paste", "Paste"), callable_mp_this(_component_paste));
            menu->add_icon_shortcut("Rename", ED_GET_SHORTCUT("graph_components_panel/rename"), RENAME_ITEM(_functions, p_item));
            menu->add_icon_shortcut("Remove", ED_GET_SHORTCUT("graph_components_panel/remove"), callable_mp_this(_component_remove_item).bind(p_item, true));
            menu->add_icon_shortcut(
                "Unlinked",
                ED_GET_SHORTCUT("graph_components_panel/disconnect_signal"), callable_mp_this(_disconnect_slot_item).bind(p_item),
                {
                    .visible = p_item->get_meta("__slot", false),
                    .tooltip = "Disconnect the slot function from the signal."
                });

            break;
        }
        case SCRIPT_VARIABLE: {
            menu->add_icon_shortcut("Duplicate", ED_GET_SHORTCUT("graph_components_panel/duplicate"), callable_mp_this(_component_duplicate_item).bind(p_item, Dictionary()));
            menu->add_icon_shortcut("ActionCopy", ED_ACTION_SHORTCUT("ui_copy", "Copy"), callable_mp_this(_component_copy_item).bind(p_item));
            menu->add_icon_shortcut("ActionPaste", ED_ACTION_SHORTCUT("ui_paste", "Paste"), callable_mp_this(_component_paste));
            menu->add_icon_item("LocalVariable", "Copy as Local Variable", callable_mp_this(_component_copy_variable_as_local).bind(p_item),
                {
                    .disabled = !_active_function.is_valid(),
                    .tooltip = "Declares a local variable of the same type in the open function graph."
                });
            menu->add_icon_shortcut("Rename", ED_GET_SHORTCUT("graph_components_panel/rename"), RENAME_ITEM(_variables, p_item));
            menu->add_icon_shortcut("Remove", ED_GET_SHORTCUT("graph_components_panel/remove"), callable_mp_this(_component_remove_item).bind(p_item, true));
            break;
        }
        case SCRIPT_SIGNAL: {
            menu->add_icon_shortcut("ActionCopy", ED_ACTION_SHORTCUT("ui_copy", "Copy"), callable_mp_this(_component_copy_item).bind(p_item));
            menu->add_icon_shortcut("ActionPaste", ED_ACTION_SHORTCUT("ui_paste", "Paste"), callable_mp_this(_component_paste));
            menu->add_icon_shortcut("Rename", ED_GET_SHORTCUT("graph_components_panel/rename"), RENAME_ITEM(_signals, p_item));
            menu->add_icon_shortcut("Remove", ED_GET_SHORTCUT("graph_components_panel/remove"), callable_mp_this(_component_remove_item).bind(p_item, true));
            break;
        }
        case FUNCTION_LOCAL_VARIABLE: {
            menu->add_icon_shortcut("Duplicate", ED_GET_SHORTCUT("graph_components_panel/duplicate"), callable_mp_this(_component_duplicate_item).bind(p_item, Dictionary()));
            menu->add_icon_shortcut("ActionCopy", ED_ACTION_SHORTCUT("ui_copy", "Copy"), callable_mp_this(_component_copy_item).bind(p_item));
            menu->add_icon_shortcut("ActionPaste", ED_ACTION_SHORTCUT("ui_paste", "Paste"), callable_mp_this(_component_paste));
            menu->add_icon_item("MemberProperty", "Promote to Variable", callable_mp_this(_component_promote_local_variable).bind(p_item),
                { .tooltip = "Turns the local variable into a script variable and converts every node that uses it." });
            menu->add_icon_shortcut("Rename", ED_GET_SHORTCUT("graph_components_panel/rename"), RENAME_ITEM(_local_variables, p_item));
            menu->add_icon_shortcut("Remove", ED_GET_SHORTCUT("graph_components_panel/remove"), callable_mp_this(_component_remove_item).bind(p_item, true));
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
    if (key.is_null() || !key->is_pressed() || key->is_echo()) {
        return;
    }

    if (ED_IS_SHORTCUT("graph_components_panel/rename", p_event)) {
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
        return;
    }

    if (ED_IS_SHORTCUT("graph_components_panel/remove", p_event)) {
        const bool can_be_removed = p_item->get_meta("__can_be_removed", true);
        if (!can_be_removed) {
            return;
        }

        _component_remove_item(p_item);
        accept_event();
        return;
    }

    if (ED_IS_ACTION_SHORTCUT("ui_copy", p_event)) {
        _component_copy_item(p_item);
        accept_event();
        return;
    }

    if (ED_IS_ACTION_SHORTCUT("ui_paste", p_event)) {
        _component_paste();
        accept_event();
        return;
    }

    if (ED_IS_SHORTCUT("graph_components_panel/open", p_event)
            || ED_IS_SHORTCUT("graph_components_panel/focus", p_event)) {
        _component_item_activated(nullptr, p_item);
        accept_event();
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
        case FUNCTION_LOCAL_VARIABLE: {
            const StringName variable_name = p_item->get_meta("__name", "");
            if (_active_function.is_valid() && _active_function->has_local_variable(variable_name)) {
                data["type"] = "local_variable";
                data["function"] = _active_function->get_function_name();
                data["variables"] = Array::make(variable_name);
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
                Tree* tree = p_item->get_tree();
                const Rect2 cell_rect = tree->get_item_area_rect(p_item, p_column);
                const Rect2 xformed = tree->get_global_transform().xform(cell_rect);
                const Rect2i screen_rect = Rect2i(Vector2i(xformed.position.x, xformed.position.y + xformed.size.y), Vector2i(cell_rect.size));

                AcceptDialog* dialog = memnew(AcceptDialog);
                dialog->set_title("Change Type: " + variable->get_variable_name());
                dialog->connect(SceneStringName(confirmed), callable_mp_cast(dialog, Node, queue_free));
                dialog->connect(SceneStringName(canceled), callable_mp_cast(dialog, Node, queue_free));
                dialog->set_flag(Window::FLAG_RESIZE_DISABLED, true);
                dialog->set_flag(Window::FLAG_MAXIMIZE_DISABLED, true);
                dialog->set_flag(Window::FLAG_MINIMIZE_DISABLED, true);
                add_child(dialog);

                OrchestratorEditorTypeSelector* selector = memnew(OrchestratorEditorTypeSelector);
                selector->set_property(variable->get_info());
                selector->setup("variable_type", true);
                selector->connect(CoreStringName(changed), callable_mp_lambda(this, [variable, selector](const Dictionary& value) {
                    variable->set_info(DictionaryUtils::to_property(value));
                    selector->set_property(DictionaryUtils::to_property(value));
                }));
                dialog->add_child(selector);
                dialog->popup_on_parent(screen_rect);

            } else if (p_column == 0 && p_id == 3) {
                variable->set_exported(!variable->is_exported());
                _set_edited(true);
                callable_mp_this(_update_components).bind(SCRIPT_VARIABLE).call_deferred();
            }
            break;
        }
        case FUNCTION_LOCAL_VARIABLE: {
            if (!_active_function.is_valid()) {
                return;
            }

            const Ref<OScriptLocalVariable> variable = _active_function->find_local_variable(item_name);
            if (!variable.is_valid()) {
                return;
            }

            if (p_column == 0 && p_id == 2) {
                Tree* tree = p_item->get_tree();
                const Rect2 cell_rect = tree->get_item_area_rect(p_item, p_column);
                const Rect2 xformed = tree->get_global_transform().xform(cell_rect);
                const Rect2i screen_rect = Rect2i(Vector2i(xformed.position.x, xformed.position.y + xformed.size.y), Vector2i(cell_rect.size));

                AcceptDialog* dialog = memnew(AcceptDialog);
                dialog->set_title("Change Type: " + variable->get_variable_name());
                dialog->connect(SceneStringName(confirmed), callable_mp_cast(dialog, Node, queue_free));
                dialog->connect(SceneStringName(canceled), callable_mp_cast(dialog, Node, queue_free));
                dialog->set_flag(Window::FLAG_RESIZE_DISABLED, true);
                dialog->set_flag(Window::FLAG_MAXIMIZE_DISABLED, true);
                dialog->set_flag(Window::FLAG_MINIMIZE_DISABLED, true);
                add_child(dialog);

                OrchestratorEditorTypeSelector* selector = memnew(OrchestratorEditorTypeSelector);
                selector->set_property(variable->get_info());
                selector->setup("variable_type", true);
                selector->connect(CoreStringName(changed), callable_mp_lambda(this, [variable, selector](const Dictionary& value) {
                    variable->set_info(DictionaryUtils::to_property(value));
                    selector->set_property(DictionaryUtils::to_property(value));
                }));
                dialog->add_child(selector);
                dialog->popup_on_parent(screen_rect);
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
        case FUNCTION_LOCAL_VARIABLE: {
            if (_active_function.is_valid()) {
                const Ref<OScriptLocalVariable> variable = _active_function->find_local_variable(item_name);
                if (variable.is_valid()) {
                    EI->edit_resource(variable);
                }
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
        case FUNCTION_LOCAL_VARIABLE: {
            if (!_active_function.is_valid()) {
                ORCHESTRATOR_ACCEPT("Local variables can only be added while a function graph is open.");
            }

            PackedStringArray existing_names = _active_function->get_local_variable_names();
            for (const PropertyInfo& argument : _active_function->get_method_info().arguments) {
                existing_names.push_back(argument.name);
            }
            const String label = NameUtils::create_unique_name("NewLocal", existing_names);

            if (_active_function->get_local_variables().is_empty()) {
                _local_variables->clear_tree();
            }

            TreeItem* item = _local_variables->add_tree_item(label, SceneUtils::get_editor_icon("LocalVariable"));
            item->set_meta("__component_type", FUNCTION_LOCAL_VARIABLE);

            _local_variables->edit_tree_item(item, callable_mp_this(_component_add_item_commit), callable_mp_this(_component_add_item_canceled));
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

    const uint32_t type = p_item->get_meta("__component_type", NONE);

    // Local variables share a namespace with the function's arguments and other locals only;
    // shadowing a script member is legal and reported by the analyzer as a warning.
    if (type == FUNCTION_LOCAL_VARIABLE) {
        if (!_active_function.is_valid()) {
            ORCHESTRATOR_ACCEPT("Local variables can only be added while a function graph is open.");
        }
        if (!_active_function->is_local_variable_name_available(item_name)) {
            ORCHESTRATOR_ACCEPT(vformat("A local variable or argument already exists with the name \"%s\".", item_name));
        }
        if (!_active_function->create_local_variable(item_name).is_valid()) {
            ORCHESTRATOR_ACCEPT("Failed to create the local variable with name " + item_name);
        }
        _set_edited(true);
        return;
    }

    if (_is_identifier_used(item_name)) {
        return;
    }

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
        case FUNCTION_LOCAL_VARIABLE: {
            if (!_active_function.is_valid()) {
                break;
            }
            const Ref<OScriptLocalVariable> duplicate = _active_function->duplicate_local_variable(name);
            if (duplicate.is_valid()) {
                _update_local_variables();
                _find_and_edit_local_variable(duplicate->get_variable_name());
            }
            break;
        }
        default:
            break;
    }
}

void OrchestratorScriptComponentsContainer::_component_promote_local_variable(TreeItem* p_item) {
    ERR_FAIL_NULL_MSG(p_item, "Cannot promote component item with no tree item");
    ERR_FAIL_COND_MSG(!_orchestration.is_valid(), "Cannot promote component item, orchestration is invalid");
    ERR_FAIL_COND_MSG(!_active_function.is_valid(), "Cannot promote local variable, no function graph is active");

    const StringName name = p_item->get_meta("__name", "");

    // Another function's local or argument with the same name would shadow the new script variable there.
    // That is legal, so the user decides whether to keep the name or take a unique one.
    const PackedStringArray shadowing = _get_orchestration()->get_functions_shadowing(name, _active_function);
    if (shadowing.is_empty()) {
        _promote_local_variable(name, false);
        return;
    }

    const String message = vformat(
        "The %s %s also declare%s a local variable or argument named \"%s\".\n\n"
        "If the script variable is also named \"%s\", it will be shadowed there and the analyzer will warn about it.",
        shadowing.size() == 1 ? "function" : "functions",
        StringUtils::join(", ", shadowing),
        shadowing.size() == 1 ? "s" : "",
        name, name);

    OrchestratorEditorDialogs::confirm_with_alternative(
        message,
        vformat("Promote as \"%s\"", name),
        callable_mp_this(_promote_local_variable).bind(name, false),
        "Use a unique name",
        callable_mp_this(_promote_local_variable).bind(name, true));
}

void OrchestratorScriptComponentsContainer::_promote_local_variable(const StringName& p_name, bool p_avoid_shadowing) {
    ERR_FAIL_COND_MSG(!_active_function.is_valid(), "Cannot promote local variable, no function graph is active");

    const Ref<OScriptVariable> variable = _get_orchestration()->promote_local_variable(_active_function, p_name, p_avoid_shadowing);
    if (!variable.is_valid()) {
        ORCHESTRATOR_ACCEPT("Failed to promote local variable " + p_name);
    }

    EI->inspect_object(nullptr);
    _update_components();
    _set_edited(true);

    // A rename the user asked for needs no notice; one forced by a script-wide name collision does
    if (!p_avoid_shadowing && variable->get_variable_name() != p_name) {
        ORCHESTRATOR_ACCEPT(vformat("The name \"%s\" is already in use, so the local variable was promoted as variable \"%s\".", p_name, variable->get_variable_name()));
    }
}

void OrchestratorScriptComponentsContainer::_component_copy_variable_as_local(TreeItem* p_item) {
    ERR_FAIL_NULL_MSG(p_item, "Cannot copy component item with no tree item");
    ERR_FAIL_COND_MSG(!_orchestration.is_valid(), "Cannot copy component item, orchestration is invalid");
    ERR_FAIL_COND_MSG(!_active_function.is_valid(), "Cannot copy variable as local, no function graph is active");

    const StringName name = p_item->get_meta("__name", "");
    const Ref<OScriptVariable> variable = _get_orchestration()->get_variable(name);
    if (!variable.is_valid()) {
        ORCHESTRATOR_ACCEPT("No variable found with the name " + name);
    }

    // The copy is a declaration only; the variable's nodes stay as they are
    PackedStringArray used = _active_function->get_local_variable_names();
    for (const PropertyInfo& argument : _active_function->get_method_info().arguments) {
        used.push_back(argument.name);
    }
    const String local_name = NameUtils::create_unique_name("local_" + name, used);

    const Ref<OScriptLocalVariable> local_variable = _active_function->create_local_variable(local_name, variable->get_info().type);
    if (!local_variable.is_valid()) {
        ORCHESTRATOR_ACCEPT("Failed to create the local variable with name " + local_name);
    }

    local_variable->set_info(variable->get_info());
    local_variable->set_default_value(variable->get_default_value());
    local_variable->set_description(variable->get_description());

    _update_local_variables();
    _set_edited(true);
}

void OrchestratorScriptComponentsContainer::_component_copy_item(TreeItem* p_item) {
    ERR_FAIL_NULL_MSG(p_item, "Cannot copy component item with no tree item");
    ERR_FAIL_COND_MSG(!_orchestration.is_valid(), "Cannot copy component item, orchestration is invalid");

    const StringName name = p_item->get_meta("__name", "");
    const uint32_t type = p_item->get_meta("__component_type", NONE);

    switch (type) {
        case SCRIPT_FUNCTION: {
            _clipboard.copy_function(_get_orchestration().ptr(), name);
            break;
        }
        case SCRIPT_VARIABLE: {
            _clipboard.copy_variable(_get_orchestration().ptr(), name);
            break;
        }
        case SCRIPT_SIGNAL: {
            _clipboard.copy_signal(_get_orchestration().ptr(), name);
            break;
        }
        case FUNCTION_LOCAL_VARIABLE: {
            if (_active_function.is_valid()) {
                _clipboard.copy_local_variable(_get_orchestration().ptr(), _active_function->get_function_name(), name);
            }
            break;
        }
        default:
            break;
    }
}

void OrchestratorScriptComponentsContainer::_component_paste() {
    ERR_FAIL_COND_MSG(!_orchestration.is_valid(), "Cannot paste, orchestration is invalid");

    // Declarations that exist here with a different definition need the user's decision first.
    // Local variables paste into the function whose graph is active, so conflicts are checked there.
    const StringName function_name = _active_function.is_valid() ? _active_function->get_function_name() : StringName();
    const Vector<OrchestratorEditorGraphClipboard::Conflict> conflicts = _clipboard.plan(_get_orchestration().ptr(), function_name);
    if (conflicts.is_empty()) {
        _component_paste_declarations(Vector<OrchestratorEditorGraphClipboard::Resolution>());
        return;
    }

    OrchestratorEditorClipboardConflictDialog* dialog = memnew(OrchestratorEditorClipboardConflictDialog);
    dialog->popup_conflicts(conflicts, callable_mp_this(_component_paste_conflicts_confirmed).bind(dialog));
}

void OrchestratorScriptComponentsContainer::_component_paste_conflicts_confirmed(Object* p_dialog) {
    OrchestratorEditorClipboardConflictDialog* dialog = cast_to<OrchestratorEditorClipboardConflictDialog>(p_dialog);
    ERR_FAIL_NULL(dialog);

    _component_paste_declarations(dialog->get_resolutions());
}

void OrchestratorScriptComponentsContainer::_component_paste_declarations(const Vector<OrchestratorEditorGraphClipboard::Resolution>& p_resolutions) {
    // Only declarations are pasted here; functions bring their bodies, graph nodes in the payload are ignored.
    // Local variables go to the active function, or are skipped when no function graph is open.
    const StringName function_name = _active_function.is_valid() ? _active_function->get_function_name() : StringName();
    const OrchestratorEditorGraphClipboard::ClipboardResult result = _clipboard.paste_declarations(_get_orchestration().ptr(), p_resolutions, function_name);
    if (result.is_empty()) {
        OrchestratorEditorDialogs::error("Nothing was pasted");
        return;
    }

    _update_components();
    _set_edited(true);

    const String summary = result.get_summary();
    if (!summary.is_empty()) {
        OrchestratorEditorDialogs::accept(summary);
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

    const uint32_t type = p_item->get_meta("__component_type", NONE);

    if (type == FUNCTION_LOCAL_VARIABLE) {
        if (!_active_function.is_valid() || !_active_function->has_local_variable(old_name)) {
            ORCHESTRATOR_ACCEPT("No local variable found with the name " + old_name);
        }
        if (!_active_function->is_local_variable_name_available(new_name)) {
            ORCHESTRATOR_ACCEPT(vformat("A local variable or argument already exists with the name \"%s\".", new_name));
        }
        if (!_active_function->rename_local_variable(old_name, new_name)) {
            ORCHESTRATOR_ACCEPT("Failed to rename local variable " + old_name);
        }
        _set_edited(true);
        return;
    }

    if (_is_identifier_used(new_name)) {
        return;
    }

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
            case FUNCTION_LOCAL_VARIABLE: {
                text = "Removing a local variable will remove all nodes that get or set the local variable.";
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
        case FUNCTION_LOCAL_VARIABLE: {
            if (!_active_function.is_valid() || !_active_function->has_local_variable(item_name)) {
                ORCHESTRATOR_ACCEPT("No local variable found with the name " + item_name);
            }

            _set_edited(true);
            _active_function->remove_local_variable(item_name);
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
        case SCRIPT_SIGNAL:
        case FUNCTION_LOCAL_VARIABLE: {
            EI->inspect_object(nullptr);
            break;
        }
        default: {
            break;
        }
    }

    emit_signal("validate_script");
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

void OrchestratorScriptComponentsContainer::_component_item_edit_started() {
    _editing = true;
}

void OrchestratorScriptComponentsContainer::_component_item_edit_finished() {
    _editing = false;
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
        case FUNCTION_LOCAL_VARIABLE: {
            _update_local_variables();
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
            _update_local_variables();
            _update_signals();
            break;
        }
    }
}

void OrchestratorScriptComponentsContainer::_find_and_edit_function(const String& p_function_name) {
    TreeItem* item = _functions->find_item(p_function_name);
    ERR_FAIL_NULL_MSG(item, "Failed to find function with name " + p_function_name);

    _functions->rename_tree_item(item, callable_mp_this(_component_rename_item));
}

void OrchestratorScriptComponentsContainer::_find_and_edit_variable(const String& p_variable_name) {
    TreeItem* item = _variables->find_item(p_variable_name);
    ERR_FAIL_NULL_MSG(item, "Failed to find variable with name " + p_variable_name);

    _variables->rename_tree_item(item, callable_mp_this(_component_rename_item));
}

void OrchestratorScriptComponentsContainer::_find_and_edit_local_variable(const String& p_variable_name) {
    TreeItem* item = _local_variables->find_item(p_variable_name);
    ERR_FAIL_NULL_MSG(item, "Failed to find local variable with name " + p_variable_name);

    _local_variables->rename_tree_item(item, callable_mp_this(_component_rename_item));
}

void OrchestratorScriptComponentsContainer::_update_graphs_and_functions() {
    ERR_FAIL_COND_MSG(!_orchestration.is_valid(), "Orchestration is invalid");

    _graphs->clear_tree();
    _functions->clear_tree();

    PackedStringArray graph_names = _get_orchestration()->get_graph_names();
    graph_names.sort();

    // Always guarantee that "EventGraph" is at the top
    if (graph_names.has("EventGraph")) {
        graph_names.erase("EventGraph");
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
            if (_get_orchestration()->has_function(script_graph->get_graph_name())) {
                int function_id = _get_orchestration()->get_function_node_id(script_graph->get_graph_name());

                const Ref<OScriptFunction> function = _get_orchestration()->find_function(script_graph->get_graph_name());
                if (function.is_valid() && !function->is_connected(CoreStringName(changed), callable_mp_this(_functions_changed))) {
                    function->connect(CoreStringName(changed), callable_mp_this(_functions_changed));
                }

                String name = script_graph->get_graph_name();
                if (_use_function_friendly_names) {
                    name = name.capitalize();
                }

                TreeItem* item = _functions->add_tree_fancy_item(name, script_graph->get_graph_name(), function_icon);
                item->set_meta("__component_type", SCRIPT_FUNCTION);
                item->set_meta("__node_id", function_id);
                item->set_meta("__override", function.is_valid() ? !function->is_user_defined() : false);

                if (function.is_valid() && !function->get_description().strip_edges().is_empty()) {
                    const String tooltip = vformat("%s\n\n%s", function->get_function_name(), function->get_description().strip_edges());
                    item->set_tooltip_text(0, SceneUtils::create_wrapped_tooltip_text(tooltip));
                }
            }
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

void OrchestratorScriptComponentsContainer::_save_category_state(TreeItem* p_item) {
    // One need to save state for items that have children, currently only variables
    if (p_item->has_meta("__name") && p_item->get_child_count() > 0) {
        const String category_name = p_item->get_meta("__name");
        _category_states[category_name] = p_item->is_collapsed();
    }
}

void OrchestratorScriptComponentsContainer::_update_variables() {
    ERR_FAIL_COND_MSG(!_orchestration.is_valid(), "Orchestration is invalid");

    // The panel is rebuilt on any orchestration change, so the collapsed state of the
    // categories must be remembered and reapplied below.
    _variables->for_each_item(callable_mp_this(_save_category_state));

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
            if (const bool* collapsed = _category_states.getptr(category_name)) {
                item->set_collapsed(*collapsed);
            }
            categories[category_name] = item;
        }
    }

    // Pass 3: Create variables
    const Ref<Texture2D> variable_icon = SceneUtils::get_editor_icon("MemberProperty");
    for (const String& variable_name : variable_names) {
        const Ref<OScriptVariable>& variable = variable_map[variable_name];

        // Any existing variables should be connected to this function, to refresh the
        // view whenever any variable data changes.
        if (!variable->is_connected(CoreStringName(changed), callable_mp_this(_variables_changed))) {
            variable->connect(CoreStringName(changed), callable_mp_this(_variables_changed));
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
            if (class_icon.is_valid()) {
                class_icon = SceneUtils::get_sized_icon(class_icon, SceneUtils::get_editor_class_icon_size());
            } else {
                class_icon = SceneUtils::get_editor_icon("FileBroken");
            }

            item->add_button(0, class_icon, 2, false, "Change variable type");
        }

        if (!variable->get_description().strip_edges().is_empty()) {
            const String tooltip = vformat("%s\n\n%s", variable->get_variable_name(), variable->get_description().strip_edges());
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
                const StringName conflict = OScriptAnnotationRegistry::find_conflict("@export", variable->get_annotations());
                if (conflict.is_empty()) {
                    tooltip_text += "\nType cannot be exported.";
                } else {
                    tooltip_text += vformat("\nCannot be exported while %s is applied.", conflict);
                }
            }
            int32_t index = item->get_button_count(0);
            item->add_button(0, SceneUtils::get_editor_icon("GuiVisibilityHidden"), 3);
            item->set_button_tooltip_text(0, index, tooltip_text);
            item->set_button_disabled(0, index, !variable->is_exportable());
        }
    }
}

void OrchestratorScriptComponentsContainer::_update_local_variables() {
    // Local variables belong to the function whose graph tab is active; event graphs have none
    _local_variables->set_visible(_active_function.is_valid());
    _local_variables->clear_tree();

    if (!_active_function.is_valid()) {
        return;
    }

    const Vector<Ref<OScriptLocalVariable>> local_variables = _active_function->get_local_variables();
    if (local_variables.is_empty()) {
        _local_variables->add_tree_empty_item("No local variables defined");
        return;
    }

    PackedStringArray names;
    HashMap<String, Ref<OScriptLocalVariable>> variable_map;
    for (const Ref<OScriptLocalVariable>& local_variable : local_variables) {
        names.push_back(local_variable->get_variable_name());
        variable_map[local_variable->get_variable_name()] = local_variable;
    }
    names.sort();

    const Ref<Texture2D> variable_icon = SceneUtils::get_editor_icon("LocalVariable");
    for (const String& name : names) {
        const Ref<OScriptLocalVariable>& local_variable = variable_map[name];

        if (!local_variable->is_connected(CoreStringName(changed), callable_mp_this(_local_variable_changed))) {
            local_variable->connect(CoreStringName(changed), callable_mp_this(_local_variable_changed));
        }

        TreeItem* item = _local_variables->add_tree_item(name, variable_icon);
        item->set_meta("__component_type", FUNCTION_LOCAL_VARIABLE);

        {
            // There is no way to set the size of the image on the button, so we must rescale
            Ref<Texture2D> class_icon = SceneUtils::get_class_icon(local_variable->get_variable_type_name());
            if (class_icon.is_valid()) {
                class_icon = SceneUtils::get_sized_icon(class_icon, SceneUtils::get_editor_class_icon_size());
            } else {
                class_icon = SceneUtils::get_editor_icon("FileBroken");
            }

            item->add_button(0, class_icon, 2, false, "Change local variable type");
        }

        if (!local_variable->get_description().strip_edges().is_empty()) {
            const String tooltip = vformat("%s\n\n%s", name, local_variable->get_description().strip_edges());
            item->set_tooltip_text(0, SceneUtils::create_wrapped_tooltip_text(tooltip));
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
        if (signal.is_valid()) {
            TreeItem* item = _signals->add_tree_item(signal->get_signal_name(), signal_icon);
            item->set_meta("__component_type", SCRIPT_SIGNAL);

            if (!signal->get_description().strip_edges().is_empty()) {
                String tooltip = vformat("%s\n\n%s", signal->get_signal_name(), signal->get_description().strip_edges());
                item->set_tooltip_text(0, SceneUtils::create_wrapped_tooltip_text(tooltip));
            }
        }
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
            bool virtual_override = p_item->get_meta("__override", false);
            if (virtual_override) {
                p_item->add_button(0, SceneUtils::get_editor_icon("Override"), -1, true);
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
    bool use_friendly_graph_names = ORCHESTRATOR_GET("interface/editor/components_panel/show_graph_friendly_names", true);
    bool use_friendly_function_names = ORCHESTRATOR_GET("interface/editor/components_panel/show_function_friendly_names", true);

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
        _set_active_function(nullptr);
        _orchestration = script->get_orchestration();
    }
}

Dictionary OrchestratorScriptComponentsContainer::get_edit_state() {
    Dictionary panel_states;
    panel_states["graphs"] = _graphs->is_collapsed();
    panel_states["functions"] = _functions->is_collapsed();
    panel_states["macros"] = _macros->is_collapsed();
    panel_states["variables"] = _variables->is_collapsed();
    panel_states["local_variables"] = _local_variables->is_collapsed();
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
        _local_variables->set_collapsed(panel_states.get("local_variables", false));
        _signals->set_collapsed(panel_states.get("signals", false));
    }
}

void OrchestratorScriptComponentsContainer::update() {
    // Avoids a queued refresh signal to break edits
    if (_editing) {
        return;
    }

    _update_components(COMPONENT_MAX);
}

void OrchestratorScriptComponentsContainer::notify_graph_opened(OrchestratorEditorGraphPanel* p_graph) {
    ERR_FAIL_NULL(p_graph);

    p_graph->connect("nodes_changed", callable_mp_this(update));
    p_graph->connect("edit_function_requested", callable_mp_this(_find_and_edit_function));
}

void OrchestratorScriptComponentsContainer::notify_active_graph_changed(const Ref<OScriptGraph>& p_graph) {
    Ref<OScriptFunction> function;
    if (_orchestration.is_valid() && p_graph.is_valid() && p_graph->get_flags().has_flag(OScriptGraph::GF_FUNCTION)) {
        function = _get_orchestration()->find_function(p_graph->get_graph_name());
    }

    _set_active_function(function);
}

void OrchestratorScriptComponentsContainer::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_THEME_CHANGED: {
            _add_function_override->set_button_icon(SceneUtils::get_editor_icon("Override"));
            _update_components();
            break;
        }
    }
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
    _graphs->connect("item_edit_started", callable_mp_this(_component_item_edit_started));
    _graphs->connect("item_edit_finished", callable_mp_this(_component_item_edit_finished));
    _graphs->set_panel_tooltip(SceneUtils::create_wrapped_tooltip_text(
        "A graph allows you to place many types of nodes to create various behaviors. "
        "Event graphs are flexible and can control multiple event nodes that start execution, "
        "nodes that may take time, react to signals, or call functions and macro nodes.\n\n"
        "While there is always one event graph called \"EventGraph\", you can create new "
        "event graphs to better help organize event logic."));
    components->add_child(_graphs);

    _add_function_override = memnew(Button);
    _add_function_override->set_focus_mode(FOCUS_NONE);
    _add_function_override->set_button_icon(SceneUtils::get_editor_icon("Override"));
    _add_function_override->set_tooltip_text("Override a Godot virtual function");
    _add_function_override->connect(SceneStringName(pressed), callable_mp_signal_lambda("add_function_override_requested"));

    _functions = memnew(OrchestratorEditorComponentView);
    _functions->set_title("Functions");
    _functions->set_tree_drag_forward(callable_mp_this(_component_item_dragged));
    _functions->set_tree_gui_handler(callable_mp_this(_component_item_gui_input));
    _functions->add_button(_add_function_override);
    _functions->connect("add_requested", callable_mp_this(_component_add_item).bind(SCRIPT_FUNCTION));
    _functions->connect("context_menu_requested", callable_mp_this(_component_show_context_menu));
    _functions->connect(SceneStringName(item_selected), callable_mp_this(_component_item_selected));
    _functions->connect(SceneStringName(item_activated), callable_mp_this(_component_item_activated));
    _functions->connect("item_button_clicked", callable_mp_this(_component_item_button_clicked));
    _functions->connect("item_edit_started", callable_mp_this(_component_item_edit_started));
    _functions->connect("item_edit_finished", callable_mp_this(_component_item_edit_finished));
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
    _macros->connect("item_edit_started", callable_mp_this(_component_item_edit_started));
    _macros->connect("item_edit_finished", callable_mp_this(_component_item_edit_finished));
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
    _variables->connect("item_edit_started", callable_mp_this(_component_item_edit_started));
    _variables->connect("item_edit_finished", callable_mp_this(_component_item_edit_finished));
    _variables->set_panel_tooltip(SceneUtils::create_wrapped_tooltip_text(
        "A variable represents some data that will be stored and managed by the orchestration.\n\n"
        "Drag a variable from the component view onto the graph area to select whether to create "
        "a get/set node or use the action menu to find the get/set option for the variable.\n\n"
        "Selecting a variable in the component view displays the variable details in the inspector."));
    components->add_child(_variables);

    _local_variables = memnew(OrchestratorEditorComponentView);
    _local_variables->set_title("Local Variables");
    _local_variables->set_tree_drag_forward(callable_mp_this(_component_item_dragged));
    _local_variables->set_tree_gui_handler(callable_mp_this(_component_item_gui_input));
    _local_variables->connect("add_requested", callable_mp_this(_component_add_item).bind(FUNCTION_LOCAL_VARIABLE));
    _local_variables->connect("context_menu_requested", callable_mp_this(_component_show_context_menu));
    _local_variables->connect(SceneStringName(item_selected), callable_mp_this(_component_item_selected));
    _local_variables->connect(SceneStringName(item_activated), callable_mp_this(_component_item_activated));
    _local_variables->connect("item_button_clicked", callable_mp_this(_component_item_button_clicked));
    _local_variables->connect("item_edit_started", callable_mp_this(_component_item_edit_started));
    _local_variables->connect("item_edit_finished", callable_mp_this(_component_item_edit_finished));
    _local_variables->set_panel_tooltip(SceneUtils::create_wrapped_tooltip_text(
        "A local variable holds data for the duration of a single function call and is only visible "
        "within the function graph that declares it. This view lists the local variables of the "
        "function whose graph is currently open.\n\n"
        "Drag a local variable from the component view onto the function graph to select whether to "
        "create a get/set node or use the action menu to find the get/set option for the local variable.\n\n"
        "Selecting a local variable in the component view displays its details in the inspector."));
    _local_variables->set_visible(false);
    components->add_child(_local_variables);

    _signals = memnew(OrchestratorEditorComponentView);
    _signals->set_title("Signals");
    _signals->set_tree_drag_forward(callable_mp_this(_component_item_dragged));
    _signals->set_tree_gui_handler(callable_mp_this(_component_item_gui_input));
    _signals->connect("add_requested", callable_mp_this(_component_add_item).bind(SCRIPT_SIGNAL));
    _signals->connect("context_menu_requested", callable_mp_this(_component_show_context_menu));
    _signals->connect(SceneStringName(item_selected), callable_mp_this(_component_item_selected));
    _signals->connect(SceneStringName(item_activated), callable_mp_this(_component_item_activated));
    _signals->connect("item_button_clicked", callable_mp_this(_component_item_button_clicked));
    _signals->connect("item_edit_started", callable_mp_this(_component_item_edit_started));
    _signals->connect("item_edit_finished", callable_mp_this(_component_item_edit_finished));
    _signals->set_panel_tooltip(SceneUtils::create_wrapped_tooltip_text(
        "A signal is used to send a notification synchronously to any number of observers that have "
        "connected to the defined signal on the orchestration. Signals allow for a variable number "
        "of arguments to be passed to the observer.\n\n"
        "Selecting a signal in the component view displays the signal details in the inspector."));
    components->add_child(_signals);

    OrchestratorEditor::get_singleton()->connect("scene_changed", callable_mp_this(_scene_changed));
    OrchestratorProjectSettingsCache::get_singleton()->connect("settings_changed", callable_mp_this(_project_settings_changed));
    OrchestratorEditorConnectionsDock::get_singleton()->connect(CoreStringName(changed), callable_mp_this(_update_slots));

    _project_settings_changed();
}

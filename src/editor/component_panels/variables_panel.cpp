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
#include "editor/component_panels/variables_panel.h"

#include "common/callable_lambda.h"
#include "common/macros.h"
#include "common/scene_utils.h"
#include "editor/plugins/inspector_plugins.h"
#include "editor/plugins/orchestrator_editor_plugin.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/tree.hpp>

void OrchestratorScriptVariablesComponentPanel::_create_variable_item(TreeItem* p_parent, const Ref<OScriptVariable>& p_variable)
{
    TreeItem* category = nullptr;
    for (TreeItem* child = p_parent->get_first_child(); child; child = child->get_next())
    {
        if (_get_tree_item_name(child).match(p_variable->get_category()))
        {
            category = child;
            break;
        }
    }

    if (p_variable->is_grouped_by_category())
    {
        if (!category)
        {
            category = _create_item(p_parent, p_variable->get_category(), p_variable->get_category(), "");
            category->set_selectable(0, false);
        }
    }
    else
        category = p_parent;

    TreeItem* item = _create_item(category, p_variable->get_variable_name(), p_variable->get_variable_name(), "MemberProperty");

    if (p_variable->is_exported() && p_variable->get_variable_name().begins_with("_"))
    {
        int32_t index = item->get_button_count(0);
        item->add_button(0, SceneUtils::get_editor_icon("NodeWarning"), 1);
        item->set_button_tooltip_text(0, index, "Variable is exported but defined as private using underscore prefix.");
        item->set_button_disabled(0, index, true);
    }

    item->add_button(0, SceneUtils::get_class_icon(p_variable->get_variable_type_name()), 2);
    item->set_button_tooltip_text(0, 0, "Change variable type");

    if (!p_variable->get_description().is_empty())
    {
        const String tooltip = p_variable->get_variable_name() + "\n\n" + p_variable->get_description();
        item->set_tooltip_text(0, SceneUtils::create_wrapped_tooltip_text(tooltip));
    }

    if (p_variable->is_exported())
    {
        int32_t index = item->get_button_count(0);
        item->add_button(0, SceneUtils::get_editor_icon("GuiVisibilityVisible"), 3);
        item->set_button_tooltip_text(0, index, "Variable is exported and visible outside the orchestration.");
        item->set_button_disabled(0, index, false);
    }
    else if (p_variable->is_constant())
    {
        String tooltip = "Variable is a constant.";

        int32_t index = item->get_button_count(0);
        item->add_button(0, SceneUtils::get_editor_icon("MemberConstant"), 4);
        item->set_button_tooltip_text(0, index, tooltip);
        item->set_button_disabled(0, index, false);
    }
    else
    {
        String tooltip = "Variable is private and not exported.";
        if (!p_variable->is_exportable())
            tooltip += "\nType cannot be exported.";

        int32_t index = item->get_button_count(0);
        item->add_button(0, SceneUtils::get_editor_icon("GuiVisibilityHidden"), 3);
        item->set_button_tooltip_text(0, index, tooltip);
        item->set_button_disabled(0, index, !p_variable->is_exportable());
    }
}

PackedStringArray OrchestratorScriptVariablesComponentPanel::_get_existing_names() const
{
    return _orchestration->get_variable_names();
}

String OrchestratorScriptVariablesComponentPanel::_get_tooltip_text() const
{
    return "A variable represents some data that will be stored and managed by the orchestration.\n\n"
           "Drag a variable from the component view onto the graph area to select whether to create "
           "a get/set node or use the action menu to find the get/set option for the variable.\n\n"
           "Selecting a variable in the component view displays the variable details in the inspector.";
}

String OrchestratorScriptVariablesComponentPanel::_get_remove_confirm_text(TreeItem* p_item) const
{
    return "Removing a variable will remove all nodes that get or set the variable.";
}

bool OrchestratorScriptVariablesComponentPanel::_populate_context_menu(TreeItem* p_item)
{
    _context_menu->add_icon_item(SceneUtils::get_editor_icon("Rename"), "Rename", CM_RENAME_VARIABLE, KEY_F2);
    _context_menu->add_icon_item(SceneUtils::get_editor_icon("Remove"), "Remove", CM_REMOVE_VARIABLE, KEY_DELETE);
    return true;
}

void OrchestratorScriptVariablesComponentPanel::_handle_context_menu(int p_id)
{
    switch (p_id)
    {
        case CM_RENAME_VARIABLE:
            _edit_selected_tree_item();
            break;
        case CM_REMOVE_VARIABLE:
            _confirm_removal(_tree->get_selected());
            break;
    }
}

bool OrchestratorScriptVariablesComponentPanel::_handle_add_new_item(const String& p_name)
{
    // Add the new variable and update the components display
    return _orchestration->create_variable(p_name).is_valid();
}

void OrchestratorScriptVariablesComponentPanel::_handle_item_selected()
{
    TreeItem* item = _tree->get_selected();

    Ref<OScriptVariable> variable = _orchestration->get_variable(_get_tree_item_name(item));
    EditorInterface::get_singleton()->edit_resource(variable);
}

void OrchestratorScriptVariablesComponentPanel::_handle_item_activated(TreeItem* p_item)
{
    Ref<OScriptVariable> variable = _orchestration->get_variable(_get_tree_item_name(p_item));
    EditorInterface::get_singleton()->edit_resource(variable);
}

bool OrchestratorScriptVariablesComponentPanel::_handle_item_renamed(const String& p_old_name, const String& p_new_name)
{
    if (_get_existing_names().has(p_new_name))
    {
        _show_notification("A variable with the name '" + p_new_name + "' already exists.");
        return false;
    }

    if (!p_new_name.is_valid_identifier())
    {
        _show_invalid_name("variable", false);
        return false;
    }

    return _orchestration->rename_variable(p_old_name, p_new_name);
}

void OrchestratorScriptVariablesComponentPanel::_handle_remove(TreeItem* p_item)
{
    _orchestration->remove_variable(_get_tree_item_name(p_item));
}

void OrchestratorScriptVariablesComponentPanel::_handle_button_clicked(TreeItem* p_item, int p_column, int p_id, int p_mouse_button)
{
    Ref<OScriptVariable> variable = _orchestration->get_variable(_get_tree_item_name(p_item));
    if (!variable.is_valid())
        return;

    _tree->set_selected(p_item, 0);

    // id 1 => warning

    if (p_column == 0 && p_id == 2)
    {
        // Type clicked
        Ref<OrchestratorEditorInspectorPluginVariable> plugin = OrchestratorPlugin::get_singleton()
            ->get_editor_inspector_plugin<OrchestratorEditorInspectorPluginVariable>();

        if (plugin.is_valid())
            plugin->edit_classification(variable.ptr());
    }
    else if (p_column == 0 && p_id == 3)
    {
        // Visibility changed on variable
        variable->set_exported(!variable->is_exported());
        update();
    }
}

Dictionary OrchestratorScriptVariablesComponentPanel::_handle_drag_data(const Vector2& p_position)
{
    Dictionary data;

    TreeItem* selected = _tree->get_selected();
    if (selected)
    {
        data["type"] = "variable";
        data["variables"] = Array::make(_get_tree_item_name(selected));
    }
    return data;
}

void OrchestratorScriptVariablesComponentPanel::_handle_tree_gui_input(const Ref<InputEvent>& p_event, TreeItem* p_item)
{
    const Ref<InputEventKey> key = p_event;
    if (key.is_valid() && key->is_pressed() && !key->is_echo())
    {
        if (key->get_keycode() == KEY_F2)
        {
            _handle_context_menu(CM_RENAME_VARIABLE);
            accept_event();
        }
        else if (key->get_keycode() == KEY_DELETE)
        {
            _handle_context_menu(CM_REMOVE_VARIABLE);
            accept_event();
        }
    }
}

void OrchestratorScriptVariablesComponentPanel::update()
{
    Callable callback = callable_mp(this, &OrchestratorScriptVariablesComponentPanel::_update_variables);

    // Make sure all variables are disconnected
    for (const Ref<OScriptVariable>& variable : _orchestration->get_variables())
        ODISCONNECT(variable, "changed", callback);

    _clear_tree();

    PackedStringArray variable_names = _orchestration->get_variable_names();
    if (!variable_names.is_empty())
    {
        HashMap<String, Ref<OScriptVariable>> categorized;
        HashMap<String, Ref<OScriptVariable>> uncategorized;
        HashMap<String, String> categorized_names;
        for (const String& variable_name : variable_names)
        {
            Ref<OScriptVariable> variable = _orchestration->get_variable(variable_name);
            if (variable->is_grouped_by_category())
            {
                const String category = variable->get_category().to_lower();
                const String sort_name = vformat("%s/%s", category, variable_name.to_lower());

                categorized[variable_name] = variable;
                categorized_names[sort_name] = variable_name;
            }
            else
                uncategorized[variable_name] = variable;
        }

        // Sort categorized
        PackedStringArray sorted_categorized_names;
        for (const KeyValue<String, String>& E : categorized_names)
            sorted_categorized_names.push_back(E.key);
        sorted_categorized_names.sort();

        // Sort uncategorized
        PackedStringArray sorted_uncategorized_names;
        for (const KeyValue<String, Ref<OScriptVariable>>& E : uncategorized)
            sorted_uncategorized_names.push_back(E.key);
        sorted_uncategorized_names.sort();

        TreeItem* root = _tree->get_root();
        for (const String& sort_categorized_name : sorted_categorized_names)
        {
            const String variable_name = categorized_names[sort_categorized_name];
            const Ref<OScriptVariable>& variable = categorized[variable_name];

            if (variable.is_valid())
                OCONNECT(variable, "changed", callback);

            _create_variable_item(root, variable);
        }

        for (const String& sort_uncategorized_name : sorted_uncategorized_names)
        {
            const Ref<OScriptVariable>& variable = uncategorized[sort_uncategorized_name];
            if (variable.is_valid())
                OCONNECT(variable, "changed", callback);

            _create_variable_item(root, variable);
        }
    }

    if (_tree->get_root()->get_child_count() == 0)
    {
        TreeItem* item = _tree->get_root()->create_child();
        item->set_text(0, "No variables defined");
        item->set_selectable(0, false);
        return;
    }

    OrchestratorScriptComponentPanel::update();
}

void OrchestratorScriptVariablesComponentPanel::_bind_methods()
{
}

OrchestratorScriptVariablesComponentPanel::OrchestratorScriptVariablesComponentPanel(Orchestration* p_orchestration)
    : OrchestratorScriptComponentPanel("Variables", p_orchestration)
{
}

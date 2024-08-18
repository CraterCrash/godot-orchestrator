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
#include "editor/component_panels/local_variables_panel.h"

#include "common/callable_lambda.h"
#include "common/macros.h"
#include "common/scene_utils.h"
#include "editor/plugins/inspector_plugins.h"
#include "editor/plugins/orchestrator_editor_plugin.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/tree.hpp>

void OrchestratorScriptLocalVariablesComponentPanel::_create_variable_item(TreeItem* p_parent, const Ref<OScriptLocalVariable>& p_variable)
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

    item->add_button(0, SceneUtils::get_class_icon(p_variable->get_variable_type_name()), 0);
    item->set_button_tooltip_text(0, 0, "Change variable type");

    if (!p_variable->get_description().is_empty())
    {
        const String tooltip = p_variable->get_variable_name() + "\n\n" + p_variable->get_description();
        item->set_tooltip_text(0, SceneUtils::create_wrapped_tooltip_text(tooltip));
    }
}

PackedStringArray OrchestratorScriptLocalVariablesComponentPanel::_get_existing_names() const
{
    PackedStringArray names;
    if (_function.is_valid())
    {
        for (const Ref<OScriptLocalVariable>& variable : _function->get_local_variables())
            names.push_back(variable->get_variable_name());
    }
    return names;
}

String OrchestratorScriptLocalVariablesComponentPanel::_get_tooltip_text() const
{
    return "A local variable represents some temporary data that will exist only within the function.\n\n"
           "Drag a local variable from the component view onto the function graph area to select whether "
           "to create a get/set node or use the action menu to find the get/set option for the variable.\n\n"
           "Selecting a local variable in the component view displays the variable details in the inspector.";
}

String OrchestratorScriptLocalVariablesComponentPanel::_get_remove_confirm_text(TreeItem* p_item) const
{
    return "Removing a local variable will remove all nodes that get or set the variable.";
}

bool OrchestratorScriptLocalVariablesComponentPanel::_populate_context_menu(TreeItem* p_item)
{
    _context_menu->add_icon_item(SceneUtils::get_editor_icon("Rename"), "Rename", CM_RENAME_VARIABLE);
    _context_menu->add_icon_item(SceneUtils::get_editor_icon("Remove"), "Remove", CM_REMOVE_VARIABLE);
    return true;
}

void OrchestratorScriptLocalVariablesComponentPanel::_handle_context_menu(int p_id)
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

bool OrchestratorScriptLocalVariablesComponentPanel::_handle_add_new_item(const String& p_name)
{
    if (!_function.is_valid())
        return false;

    // Add the new variable and update the components display
    return _function->create_local_variable(p_name).is_valid();
}

void OrchestratorScriptLocalVariablesComponentPanel::_handle_item_selected()
{
    if (!_function.is_valid())
        return;

    TreeItem* item = _tree->get_selected();

    Ref<OScriptLocalVariable> variable = _function->get_local_variable(_get_tree_item_name(item));
    EditorInterface::get_singleton()->edit_resource(variable);
}

void OrchestratorScriptLocalVariablesComponentPanel::_handle_item_activated(TreeItem* p_item)
{
    if (!_function.is_valid())
        return;

    Ref<OScriptLocalVariable> variable = _function->get_local_variable(_get_tree_item_name(p_item));
    EditorInterface::get_singleton()->edit_resource(variable);
}

bool OrchestratorScriptLocalVariablesComponentPanel::_handle_item_renamed(const String& p_old_name, const String& p_new_name)
{
    if (_get_existing_names().has(p_new_name))
    {
        _show_notification("A local variable with the name '" + p_new_name + "' already exists.");
        return false;
    }

    if (!p_new_name.is_valid_identifier())
    {
        _show_invalid_name("local variable", false);
        return false;
    }

    if (!_function.is_valid())
        return false;

    return _function->rename_local_variable(p_old_name, p_new_name);
}

void OrchestratorScriptLocalVariablesComponentPanel::_handle_remove(TreeItem* p_item)
{
    if (!_function.is_valid())
        return;

    const String variable_name = _get_tree_item_name(p_item);
    _function->remove_local_variable(variable_name);
}

void OrchestratorScriptLocalVariablesComponentPanel::_handle_button_clicked(TreeItem* p_item, int p_column, int p_id, int p_mouse_button)
{
    if (!_function.is_valid())
        return;

    const String variable_name = _get_tree_item_name(p_item);
    const Ref<OScriptLocalVariable> variable = _function->get_local_variable(variable_name);
    if (!variable.is_valid())
        return;

    _tree->set_selected(p_item, 0);

    if (p_column == 0 && p_id == 0)
    {
        // Type clicked
        Ref<OrchestratorEditorInspectorPluginVariable> plugin = OrchestratorPlugin::get_singleton()
            ->get_editor_inspector_plugin<OrchestratorEditorInspectorPluginVariable>();

        if (plugin.is_valid())
            plugin->edit_classification(variable.ptr());
    }
}

Dictionary OrchestratorScriptLocalVariablesComponentPanel::_handle_drag_data(const Vector2& p_position)
{
    Dictionary data;

    TreeItem* selected = _tree->get_selected();
    if (selected)
    {
        data["type"] = "local_variable";
        data["local_variables"] = Array::make(_get_tree_item_name(selected));
    }
    return data;
}

void OrchestratorScriptLocalVariablesComponentPanel::update()
{
    _clear_tree();

    Callable callback = callable_mp(this, &OrchestratorScriptLocalVariablesComponentPanel::_update_variables);

    // Make sure all variables are disconnected
    if (_function.is_valid())
    {
        for (const Ref<OScriptLocalVariable>& variable : _function->get_local_variables())
            ODISCONNECT(variable, "changed", callback);
    }

    if (_function.is_valid())
    {
        Vector<Ref<OScriptLocalVariable>> variables = _function->get_local_variables();
        if (!variables.is_empty())
        {
            PackedStringArray sorted_categorized_names;
            PackedStringArray sorted_uncategorized_names;

            HashMap<String, Ref<OScriptLocalVariable>> categorized;
            HashMap<String, Ref<OScriptLocalVariable>> uncategorized;
            HashMap<String, String> categorized_names;

            for (const Ref<OScriptLocalVariable>& variable : variables)
            {
                const String variable_name = variable->get_variable_name();

                if (variable->is_grouped_by_category())
                {
                    const String category = variable->get_category().to_lower();
                    const String sort_name = vformat("%s/%s", category, variable_name.to_lower());

                    categorized[variable_name] = variable;
                    categorized_names[sort_name] = variable_name;

                    sorted_categorized_names.push_back(sort_name);
                }
                else
                {
                    uncategorized[variable_name] = variable;
                    sorted_uncategorized_names.push_back(variable_name);
                }
            }

            // Sort names
            sorted_categorized_names.sort();
            sorted_uncategorized_names.sort();

            TreeItem* root = _tree->get_root();
            for (const String& name : sorted_categorized_names)
            {
                const String& variable_name = categorized_names[name];

                const Ref<OScriptLocalVariable>& variable = categorized[variable_name];
                if (variable.is_valid())
                    OCONNECT(variable, "changed", callback);

                _create_variable_item(root, variable);
            }

            for (const String& name : sorted_uncategorized_names)
            {
                const Ref<OScriptLocalVariable>& variable = uncategorized[name];
                if (variable.is_valid())
                    OCONNECT(variable, "changed", callback);

                _create_variable_item(root, variable);
            }
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

void OrchestratorScriptLocalVariablesComponentPanel::set_function(const Ref<OScriptFunction>& p_function)
{
    _function = p_function;
    update();
}

void OrchestratorScriptLocalVariablesComponentPanel::_bind_methods()
{
}

OrchestratorScriptLocalVariablesComponentPanel::OrchestratorScriptLocalVariablesComponentPanel(Orchestration* p_orchestration)
    : OrchestratorScriptComponentPanel("Local Variables", p_orchestration)
{
}

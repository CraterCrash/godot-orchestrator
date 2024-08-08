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
#include "action_db.h"

#include "editor/graph/graph_edit.h"
#include "editor/graph/actions/default_action_registrar.h"

#include "common/string_utils.h"

void OrchestratorGraphActionDB::_generate_action_items(const OrchestratorGraphActionFilter& p_filter, const StringName& p_name)
{
    if (!_object_items.has(p_name))
        return;

    List<Ref<OrchestratorGraphActionMenuItem>>& items = _object_items[p_name];
    if (!items.is_empty())
        items.clear();

    OrchestratorGraphActionRegistrarContext context = OrchestratorGraphActionRegistrarContext::from_filter(p_filter);
    context.list = &items;

    // Iterates each OrchestratorGraphActionRegistrar and registers actions
    for (const String& name : ClassDB::get_inheriters_from_class(OrchestratorGraphActionRegistrar::get_class_static()))
    {
        if (!ClassDB::can_instantiate(name))
            continue;

        Variant object = ClassDB::instantiate(name);
        if (OrchestratorGraphActionRegistrar* registrar = Object::cast_to<OrchestratorGraphActionRegistrar>(object))
            registrar->register_actions(context);
    }

    items.sort_custom<OrchestratorGraphActionMenuItemComparator>();
}

void OrchestratorGraphActionDB::_generate_filtered_items(const OrchestratorGraphActionFilter& p_filter, const StringName& p_name)
{
    _filtered_items.clear();

    if (!_object_items.has(p_name))
        return;

    const List<Ref<OrchestratorGraphActionMenuItem>>& items = _object_items.get(p_name);
    for (const Ref<OrchestratorGraphActionMenuItem>& E : items)
    {
        if (E->get_handler().is_valid())
        {
            if (E->get_handler()->is_filtered(p_filter, E->get_spec()))
                continue;
        }

        _filtered_items.push_back(E);
    }
}

void OrchestratorGraphActionDB::clear()
{
    _object_items.clear();
}

void OrchestratorGraphActionDB::use_temp(bool p_use_temp)
{
    _use_temp = p_use_temp;
    _object_items["$Temp$"] = { };
}

void OrchestratorGraphActionDB::load(const OrchestratorGraphActionFilter& p_filter)
{
    // When base type changes, refresh entire database
    const StringName base_type = p_filter.context.graph->get_orchestration()->get_base_type();
    if (_graph_base_type != base_type)
    {
        clear();
        _graph_base_type = base_type;
    }

    // Calculate the object name to store the items within
    StringName name = "$Default$";
    if (p_filter.has_target_object())
    {
        name = p_filter.get_target_class();
    }
    else if (p_filter.target_classes.size() == 1)
    {
        name = p_filter.target_classes.get(0);
    }
    else if (p_filter.target_classes.size() > 1)
    {
        ERR_PRINT("Action menu does not expect target classes to contain more than one class.");
        clear();
        return;
    }

    if (_use_temp)
        name = "$Temp$";

    if (_object_items.is_empty())
        _object_items[name] = { };

    if (_object_items[name].is_empty())
        _generate_action_items(p_filter, name);

    // Apply filter on items
    // This is always applied to handle per-request context-sensitive lists
    _generate_filtered_items(p_filter, name);
}
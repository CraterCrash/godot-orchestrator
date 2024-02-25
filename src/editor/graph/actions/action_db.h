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
#ifndef ORCHESTRATOR_GRAPH_ACTION_DB_H
#define ORCHESTRATOR_GRAPH_ACTION_DB_H

#include "action_menu_filter.h"
#include "action_menu_item.h"

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/templates/hash_map.hpp>

using namespace godot;

/// A simple database that maintains a collection of actions.
class OrchestratorGraphActionDB : public RefCounted
{
    GDCLASS(OrchestratorGraphActionDB, RefCounted);

    StringName _graph_base_type;
    HashMap<StringName, List<Ref<OrchestratorGraphActionMenuItem>>> _object_items;
    List<Ref<OrchestratorGraphActionMenuItem>> _filtered_items;

protected:
    /// Godot bind methods handler
    static void _bind_methods() {}

    /// Generates the action items.
    /// @param p_filter the filter
    void _generate_action_items(const OrchestratorGraphActionFilter& p_filter, const StringName& p_name);

    /// Generates the filtered actions items.
    /// @param p_filter the filter
    void _generate_filtered_items(const OrchestratorGraphActionFilter& p_filter, const StringName& p_name);

public:

    /// Clear all persisted state
    void clear();

    /// Load the database actions based on the supplied context.
    /// @param p_filter the filter
    void load(const OrchestratorGraphActionFilter& p_filter);

    /// Retrieve an immutable list of context-specific graph action items.
    const List<Ref<OrchestratorGraphActionMenuItem>>& get_items() const { return _filtered_items; }

};

#endif // ORCHESTRATOR_GRAPH_ACTION_DB_H
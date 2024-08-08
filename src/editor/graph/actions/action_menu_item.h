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
#ifndef ORCHESTRATOR_GRAPH_ACTION_MENU_ITEM_H
#define ORCHESTRATOR_GRAPH_ACTION_MENU_ITEM_H

#include <godot_cpp/classes/ref_counted.hpp>

using namespace godot;

/// Forward declarations
class OrchestratorGraphEdit;
struct OrchestratorGraphActionFilter;

/// A simple specification on how to render a specific OrchestratorGraphAction
struct OrchestratorGraphActionSpec
{
    String category;
    String tooltip;
    String keywords;
    String text;
    String qualifiers;
    String icon;
    String type_icon;
    bool graph_compatible{ true };

    OrchestratorGraphActionSpec() { }

    explicit OrchestratorGraphActionSpec(const String& p_text, const String& p_icon, const String& p_type_icon)
        : category(p_text)
        , tooltip(p_text)
        , keywords(p_text)
        , text(p_text.capitalize())
        , icon(p_icon)
        , type_icon(p_type_icon)
    {
    }
};

/// Base class for editor graph actions
class OrchestratorGraphActionHandler : public RefCounted
{
    GDCLASS(OrchestratorGraphActionHandler, RefCounted);
    static void _bind_methods() {}

public:

    /// Executes the desired action handler logic.
    /// @param p_graph the editor graph instance, should not be null
    /// @param p_position the position in the graph where the action should occur.
    virtual void execute(OrchestratorGraphEdit* p_graph, const Vector2& p_position) {}

    /// Check whether the action is filtered.
    /// @param p_filter the filter
    /// @param p_spec the action specification
    /// @return true if the action is to be filtered and not shown; false otherwise
    virtual bool is_filtered(const OrchestratorGraphActionFilter& p_filter, const OrchestratorGraphActionSpec& p_spec) { return false; }
};

/// Base class for editor menu items
class OrchestratorGraphActionMenuItem : public RefCounted
{
    GDCLASS(OrchestratorGraphActionMenuItem, RefCounted);

    static void _bind_methods() {}
    OrchestratorGraphActionMenuItem() = default;

    OrchestratorGraphActionSpec _spec;            //! The graph action specification
    Ref<OrchestratorGraphActionHandler> _handler; //! The graph action handler reference

public:
    OrchestratorGraphActionMenuItem(const OrchestratorGraphActionSpec& p_spec)
        : _spec(p_spec)
    {
    }
    OrchestratorGraphActionMenuItem(const OrchestratorGraphActionSpec& p_spec, Ref<OrchestratorGraphActionHandler>& p_handler)
        : _spec(p_spec)
        , _handler(p_handler)
    {
    }

    /// Get the graph action specification.
    /// @return the specification
    _FORCE_INLINE_ const OrchestratorGraphActionSpec& get_spec() const { return _spec; }

    /// Get the graph action handler
    /// @return the action handler reference, may be invalid
    _FORCE_INLINE_ const Ref<OrchestratorGraphActionHandler>& get_handler() const { return _handler; }
};

/// A simple OrchestratorGraphActionMenuItem comparator implementation that compares the category of
/// two different categories and sorts them in ascending, alphabetical order.
struct OrchestratorGraphActionMenuItemComparator
{
    _FORCE_INLINE_ bool operator()(const Ref<OrchestratorGraphActionMenuItem>& a, const Ref<OrchestratorGraphActionMenuItem>& b) const
    {
        const PackedStringArray categories_a = a->get_spec().category.to_lower().split("/");
        const PackedStringArray categories_b = b->get_spec().category.to_lower().split("/");

        const int max_checks = (int) Math::min(categories_a.size(), categories_b.size());
        for (int i = 0; i < max_checks; i++)
        {
            // If the bit is equal, go to the next one
            if (categories_a[i] == categories_b[i])
                continue;

            // Places "Project" top-level before others
            if (i == 0 && categories_a[i].match("project"))
                return true;
            else if (i == 0 && categories_b[i].match("project"))
                return false;

            return categories_a[i] < categories_b[i];
        }

        // If we got here both are identical, default to a < b
        return a->get_spec().category < b->get_spec().category;
    }
};

#endif  // ORCHESTRATOR_GRAPH_ACTION_MENU_ITEM_H

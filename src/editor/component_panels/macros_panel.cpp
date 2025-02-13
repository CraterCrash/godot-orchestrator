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
#include "editor/component_panels/macros_panel.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/tree.hpp>

String OrchestratorScriptMacrosComponentPanel::_get_tooltip_text() const
{
    return "A macro graph allows for the encapsulation of functionality for re-use. Macros have both a "
           "singular input and output node, but these nodes can have as many input or output data "
           "values needed for logic. Macros can contain nodes that take time, such as delays, but are "
           "not permitted to contain event nodes, such as a node that reacts to '_ready'.\n\n"
           "This feature is currently disabled and will be available in a future release.";
}

void OrchestratorScriptMacrosComponentPanel::update()
{
    if (_update_blocked)
        return;

    if (_tree->get_root()->get_child_count() == 0)
    {
        TreeItem* item = _tree->get_root()->create_child();
        item->set_text(0, "No macros defined");
        item->set_selectable(0, false);
        return;
    }

    OrchestratorScriptComponentPanel::update();
}

void OrchestratorScriptMacrosComponentPanel::_notification(int p_what)
{
    #if GODOT_VERSION < 0x040300
    // Godot does not dispatch to parent (shrugs)
    OrchestratorScriptComponentPanel::_notification(p_what);
    #endif

    if (p_what == NOTIFICATION_READY)
    {
        HBoxContainer* container = _get_panel_hbox();

        Button* button = Object::cast_to<Button>(container->get_child(-1));
        if (button)
            button->set_disabled(true);
    }
}

void OrchestratorScriptMacrosComponentPanel::_bind_methods()
{
}

OrchestratorScriptMacrosComponentPanel::OrchestratorScriptMacrosComponentPanel(Orchestration* p_orchestration)
    : OrchestratorScriptComponentPanel("Macros", p_orchestration)
{
}
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
#ifndef ORCHESTRATOR_BUILD_OUTPUT_PANEL_H
#define ORCHESTRATOR_BUILD_OUTPUT_PANEL_H

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>

using namespace godot;

/// An editor bottom panel that outputs the build and validation status details.
class OrchestratorBuildOutputPanel : public HBoxContainer
{
    GDCLASS(OrchestratorBuildOutputPanel, HBoxContainer);
    static void _bind_methods();

protected:
    RichTextLabel* _rtl{ nullptr };
    Button* _button{ nullptr };
    Button* _clear_button{ nullptr };

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

    void _append_text(const String& p_text);

public:
    /// Resets the output panel
    void reset();

    /// Adds an error to the output log
    /// @param p_text the error text to be appended
    void add_error(const String& p_text);

    /// Adds a warning to the output log
    /// @param p_text the warning text to be appended
    void add_warning(const String& p_text);

    /// Adds a basic message text
    /// @param p_text the message text
    void add_message(const String& p_text);

    /// The activating button
    void set_tool_button(Button* p_button);

    OrchestratorBuildOutputPanel();
};

#endif // ORCHESTRATOR_BUILD_OUTPUT_PANEL_H
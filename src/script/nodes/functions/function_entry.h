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
#ifndef ORCHESTRATOR_SCRIPT_NODE_FUNCTION_ENTRY_H
#define ORCHESTRATOR_SCRIPT_NODE_FUNCTION_ENTRY_H

#include "function_terminator.h"

/// Represents the entrypoint for a function.
///
/// All function graphs contain a function entry node, it's mandatory. It represents
/// the entrypoint into the function graph and the function itself.
///
class OScriptNodeFunctionEntry : public OScriptNodeFunctionTerminator
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeFunctionEntry, OScriptNodeFunctionTerminator);
    static void _bind_methods() { }

protected:
    virtual bool _is_user_defined() const { return true; }

public:
    OScriptNodeFunctionEntry();

    //~ Begin OScriptNode Interface
    void allocate_default_pins() override;
    bool can_user_delete_node() const override { return false; }
    String get_node_title() const override;
    String get_tooltip_text() const override;
    void post_paste_node() override;
    bool draw_node_as_entry() const override { return true; }
    bool can_duplicate() const override { return false; }
    bool can_create_user_defined_pin(EPinDirection p_direction, String& r_message) override;
    OScriptNodeInstance* instantiate() override;
    void initialize(const OScriptNodeInitContext& p_context) override;
    //~ End OScriptNode Interface

    /// Gets the execution pin for this function entry node
    /// @return the execution pin
    Ref<OScriptNodePin> get_execution_pin() const;
};

#endif  // ORCHESTRATOR_FUNCTION_ENTRY_H

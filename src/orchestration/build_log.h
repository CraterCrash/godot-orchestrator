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
#ifndef ORCHESTRATOR_ORCHESTRATION_BUILD_LOG_H
#define ORCHESTRATOR_ORCHESTRATION_BUILD_LOG_H

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

class OScriptNode;

/// Simple orchestration build log
class BuildLog
{
protected:
    Vector<String> _messages;
    int _errors{ 0 };
    int _warnings{ 0 };
    Ref<OScriptNode> _current_node;

public:
    /// Checks if log has any errors
    /// @return true if there are errors, false otherwise
    bool has_errors() const { return _errors > 0; }

    /// Checks if log has any warnings
    /// @return true if there are warnings, false otherwise
    bool has_warnings() const { return _warnings > 0; }

    /// Adds an error message to the log
    /// @param p_message the message
    void error(const String& p_message);

    /// Adds a warning message to the log
    /// @param p_message the message
    void warn(const String& p_message);

    /// Return the build messages
    /// @return the messages
    const Vector<String>& get_messages() const { return _messages; }

    /// Set the current node being analyzed
    /// @param p_node the node being analyzed
    void set_current_node(const Ref<OScriptNode>& p_node) { _current_node = p_node; }
};

#endif // ORCHESTRATOR_ORCHESTATION_BUILD_LOG_H
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
#ifndef ORCHESTRATOR_ORCHESTRATION_BUILD_LOG_H
#define ORCHESTRATOR_ORCHESTRATION_BUILD_LOG_H

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/templates/vector.hpp>

using namespace godot;

/// Forward declarations
class OScriptNode;
class OScriptNodePin;

/// Simple orchestration build log
class BuildLog
{
public:
    // Defines the different failure types
    enum FailureType
    {
        FT_Error,
        FT_Warning
    };

    // Defines a specific build failure observed
    struct Failure
    {
        FailureType type;           //! The failure type
        String message;             //! The failure message
        Ref<OScriptNode> node;      //! The node that triggered the failure
        Ref<OScriptNodePin> pin;    //! The pin that triggered the failure
    };

protected:
    Vector<Failure> _failures;

    /// Adds a failure to the build log
    /// @param p_type the failure type
    /// @param p_node the node that triggered the failure
    /// @param p_pin the optional pin that triggered the failure
    /// @param p_message the message
    void _add_failure(FailureType p_type, const OScriptNode* p_node, const Ref<OScriptNodePin>& p_pin, const String& p_message);

public:
    /// Register a specific node build error unrelated to pins.
    /// @param p_node the node
    /// @param p_message the error message
    void error(const OScriptNode* p_node, const String& p_message);

    /// Register a build error
    /// @param p_node the node
    /// @param p_pin the pin that the error is related to
    /// @param p_message the error message
    void error(const OScriptNode* p_node, const Ref<OScriptNodePin>& p_pin, const String& p_message);

    /// Register a specific node build warning unrelated to pins.
    /// @param p_node the node
    /// @param p_message the error message
    void warn(const OScriptNode* p_node, const String& p_message);

    /// Register a build warning
    /// @param p_node the node
    /// @param p_pin the pin that the error is related to
    /// @param p_message the error message
    void warn(const OScriptNode* p_node, const Ref<OScriptNodePin>& p_pin, const String& p_message);

    /// Get all failures
    /// @return a collection of all failures
    const Vector<Failure>& get_failures() const { return _failures; }
};

#endif // ORCHESTRATOR_ORCHESTATION_BUILD_LOG_H
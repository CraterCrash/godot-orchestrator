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
#ifndef ORCHESTRATOR_EDITOR_ACTIONS_PAYLOAD_H
#define ORCHESTRATOR_EDITOR_ACTIONS_PAYLOAD_H

#include <godot_cpp/classes/ref_counted.hpp>

using namespace godot;

/// Base class for all action payloads.
///
/// An action payload is a dynamic value such as a <code>MethodInfo</code> or <code>PropertyInfo</code>
/// object that can be safely passed in signals so that the handler for a selected action has all the
/// details for executing the action, rather than relying on a generic container like a Dictionary.
///
class OrchestratorEditorActionPayload : public RefCounted
{
    GDCLASS(OrchestratorEditorActionPayload, RefCounted);

protected:
    static void _bind_methods() { }
};

class OrchestratorEditorSpawnScriptNodePayload : public OrchestratorEditorActionPayload
{
    GDCLASS(OrchestratorEditorSpawnScriptNodePayload, OrchestratorEditorActionPayload);

protected:
    static void _bind_methods() { }

public:
    String node_type;
    Dictionary data;
};

class OrchestratorEditorPropertyPayload : public OrchestratorEditorActionPayload
{
protected:
    static void _bind_methods() { }

public:
    PropertyInfo property;
    NodePath node_path;
    PackedStringArray target_classes;
};

class OrchestratorEditorCallMemberFunctionPayload : public OrchestratorEditorActionPayload
{
protected:
    static void _bind_methods() { }

public:
    MethodInfo method;
    StringName class_name;
};

class OrchestratorEditorSpawnEmitMemberSignalPayload : public OrchestratorEditorActionPayload
{
protected:
    static void _bind_methods() { }

public:
    MethodInfo method;
    Dictionary data;
};

#endif // ORCHESTRATOR_EDITOR_ACTIONS_PAYLOAD_H
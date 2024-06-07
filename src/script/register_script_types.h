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
#ifndef ORCHESTRATOR_REGISTER_SCRIPT_TYPES_H
#define ORCHESTRATOR_REGISTER_SCRIPT_TYPES_H

void register_script_types();
void unregister_script_types();

void register_script_extension();
void unregister_script_extension();

void register_script_node_types();
void unregister_script_node_types();

void register_script_resource_formats();
void unregister_script_resource_formats();

void register_extension_db();
void unregister_extension_db();

#endif // ORCHESTRATOR_REGISTER_SCRIPT_TYPES_H
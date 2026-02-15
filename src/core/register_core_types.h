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
#ifndef ORCHESTRATOR_CORE_REGISTER_CORE_TYPES_H
#define ORCHESTRATOR_CORE_REGISTER_CORE_TYPES_H

void register_core_singletons();
void unregister_core_singletons();

void create_core_singletons();
void destroy_core_singletons();

#endif // ORCHESTRATOR_CORE_REGISTER_CORE_TYPES_H
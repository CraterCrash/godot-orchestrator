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
#ifndef ORCHESTRATOR_MACROS_H
#define ORCHESTRATOR_MACROS_H

#define OACCEL_KEY(mask,key) (Key(static_cast<int>(mask) | static_cast<int>(key)))

#define OCONNECT(obj, signal, method)               \
        if (!obj->is_connected(signal, method)) {   \
            obj->connect(signal, method);           \
        }

#define ODISCONNECT(obj, signal, method)            \
        if (obj->is_connected(signal, method)) {    \
            obj->disconnect(signal, method);        \
        }

#endif // ORCHESTRATOR_MACROS_H
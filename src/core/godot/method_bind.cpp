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
#include "core/godot/method_bind.h"

#include "core/godot/object/class_db.h"

#include <godot_cpp/classes/object.hpp>

using namespace godot;

PropertyInfo GDE::MethodBind::get_return_info(godot::MethodBind* p_method_bind) {
    MethodInfo method_info;
    if (ClassDB::get_method_info(p_method_bind->get_instance_class(), p_method_bind->get_name(), method_info)) {
        return method_info.return_val;
    }
    return {};
}
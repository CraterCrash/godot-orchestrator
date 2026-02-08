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
#ifndef ORCHESTRATOR_SCRIPT_TEMPLATE_REGISTRY_H
#define ORCHESTRATOR_SCRIPT_TEMPLATE_REGISTRY_H

#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/variant/string_name.hpp>

using namespace godot;

// Simple registry that stores decoded orchestration script templates.
//
// The templates are read and serialized into a ZLIB buffer that is loaded at editor startup. This class
// will read the serialized ZLIB buffer and generate a set of template entries that will be used by the
// scripting language when a template is requested by a user.
//
class OScriptTemplateRegistry {
public:
    struct Template {
        String name;
        String inherits;
        String description;
        String script_template;
    };

private:
    LocalVector<Template> _templates;

    void _load_template_data();

public:
    /// Get all templates for a given base type
    /// @param p_base_type the base type
    /// @return a list of available templates
    List<Template> get_templates(const StringName& p_base_type);

    OScriptTemplateRegistry();
};

#endif // ORCHESTRATOR_SCRIPT_TEMPLATE_REGISTRY_H
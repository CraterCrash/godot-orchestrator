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
#pragma once

#include <memory>

#include <godot_cpp/classes/editor_property.hpp>

using namespace godot;

/// Forward declarations
class OrchestratorEditorTypeSelector;

class OrchestratorEditorTypeConstraintProvider {
public:
    virtual bool is_key_locked() const = 0;
    virtual bool is_value_locked() const = 0;
    virtual PackedStringArray get_exclusions() const = 0;
    virtual ~OrchestratorEditorTypeConstraintProvider() = default;
};

class OrchestratorEditorPropertyType : public EditorProperty {
    GDCLASS(OrchestratorEditorPropertyType, EditorProperty);

    OrchestratorEditorTypeSelector* _selector = nullptr;
    std::unique_ptr<OrchestratorEditorTypeConstraintProvider> _provider;

    void _selector_type_changed(const Dictionary& p_property);

protected:
    static void _bind_methods() { }

public:
    //~ Begin EditorProperty Interface
    void _update_property() override;
    void _set_read_only(bool p_read_only) override;
    //~ End EditorProperty Interface

    void setup(const String& p_cache_suffix, bool p_allow_abstract_types = false, const PackedStringArray& p_exclusions = PackedStringArray());
    void set_constraint_provider(std::unique_ptr<OrchestratorEditorTypeConstraintProvider> p_provider);

    OrchestratorEditorPropertyType();
};
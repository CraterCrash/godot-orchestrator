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
#include "editor/actions/rules/port_rule.h"

#include "common/dictionary_utils.h"
#include "common/method_utils.h"
#include "common/property_utils.h"
#include "common/string_utils.h"
#include "editor/actions/filter_engine.h"
#include "editor/graph/graph_pin.h"
#include "script/script_server.h"

bool OrchestratorEditorActionPortRule::matches(const Ref<OrchestratorEditorActionDefinition>& p_action, const FilterContext& p_context) {
    ERR_FAIL_COND_V(!p_action.is_valid(), false);

    if (_execution) {
        return p_action->executions;
    }

    // Match against class types
    // These are typically methods that can be called within the scope of the action class, such as calling
    // the "quit" method on a "SceneTree" object.
    if (!_target_classes.is_empty() && _target_classes.has(p_action->class_name.value_or(""))) {
        return true;
    }

    // Match against methods that are associated with variant types.
    // For example, dragging from a Callable pin provides access to methods like bind.
    if (_type != Variant::VARIANT_MAX && _target_classes.has(p_action->target_class)) {
        return true;
    }

    // Match against method
    // For output pins, we check whether the method can accept the pin's class/variant type as an input
    // For input pins, we check whether the method return matches the input pin's class/variant type
    if (p_action->method.has_value()) {
        const MethodInfo& method = p_action->method.value();
        if (_output) {
            // Match against method arguments
            for (const PropertyInfo& argument : method.arguments) {
                if (argument.type == _type
                        || _target_classes.has(argument.class_name)
                        || PropertyUtils::is_variant(argument)) {
                    return true;
                }
            }
        }
        else {
            // Match against method return type
            if (MethodUtils::has_return_value(method)) {
                return method.return_val.type == _type
                    || _target_classes.has(method.return_val.class_name)
                    || PropertyUtils::is_variant(method.return_val);
            }
        }
        return false;
    }

    // Match against property
    // For property getters, verify that the input pin's type or class matches.
    // For property setters, verify that the output pin's type or class matches.
    if (p_action->property.has_value()) {
        const PropertyInfo& property = p_action->property.value();

        switch (p_action->type) {
            case OrchestratorEditorActionDefinition::ACTION_SET_PROPERTY:
            case OrchestratorEditorActionDefinition::ACTION_VARIABLE_SET: {
                if (_output) {
                    return property.type == _type
                        || _target_classes.has(property.class_name)
                        || PropertyUtils::is_variant(property);
                }
                break;
            }
            case OrchestratorEditorActionDefinition::ACTION_GET_PROPERTY:
            case OrchestratorEditorActionDefinition::ACTION_VARIABLE_GET: {
                if (!_output) {
                    return property.type == _type
                        || _target_classes.has(property.class_name)
                        || PropertyUtils::is_variant(property);
                }
                break;
            }
            default: {
                break;
            }
        }
    }

    // Match operator input/outputs
    // Operators do not have
    if (p_action->inputs.has_value() && !_output) {
        return p_action->inputs.value().has(_type);
    }

    if (p_action->outputs.has_value() && _output) {
        return p_action->outputs.value().has(_type);
    }

    return false;
}

void OrchestratorEditorActionPortRule::configure(const OrchestratorEditorGraphPin* p_pin, const Object* p_target) {
    _output = p_pin->get_direction() == PD_Output;
    _execution = p_pin->is_execution();

    const PropertyInfo& property = p_pin->get_property_info();
    if (property.type != Variant::NIL && property.type != Variant::OBJECT) {
        // Only match against property type
        _type = property.type;
        _target_classes.push_back(Variant::get_type_name(_type));
    }
    else {
        _type = Variant::VARIANT_MAX;
        if (!property.class_name.is_empty()) {
            if (ScriptServer::is_global_class(property.class_name)) {
                _target_classes = ScriptServer::get_class_hierarchy(property.class_name, true);
            }
            else if (p_target) {
                String class_name = p_target->get_class();
                while (!class_name.is_empty()) {
                    _target_classes.push_back(class_name);
                    class_name = ClassDB::get_parent_class(class_name);
                }
            }
            else {
                String class_name = property.class_name;
                while (!class_name.is_empty()) {
                    _target_classes.push_back(class_name);
                    class_name = ClassDB::get_parent_class(class_name);
                }
            }
        }
    }
}

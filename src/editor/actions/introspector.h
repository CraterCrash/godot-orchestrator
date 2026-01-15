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
#ifndef ORCHESTRATOR_EDITOR_ACTIONS_INTROSPECTOR_H
#define ORCHESTRATOR_EDITOR_ACTIONS_INTROSPECTOR_H

#include "api/extension_db.h"
#include "editor/actions/definition.h"
#include "script/node.h"

#include <godot_cpp/templates/hash_map.hpp>

/// A standalone component that is responsible for being able to read and generate a set of actions based on
/// provided class, object, or script metadata. It also can provide actions based on the visual scripting
/// language, built-in Godot types, and project configured autoloads.
class OrchestratorEditorIntrospector {

    using ActionBuilder = OrchestratorEditorActionBuilder;
    using Action = OrchestratorEditorActionDefinition;
    using ActionType = Action::ActionType;
    using GraphType = Action::GraphType;

    static HashMap<String, Ref<OScriptNode>> _script_node_cache;

    template <typename T>
    static ActionBuilder _script_node_builder(const String& p_category, const String& p_name, const Dictionary& p_data = Dictionary())
    { return _script_node_builder(T::get_class_static(), p_category, p_name, p_data); }

    template <typename T> static Ref<OScriptNode> _get_or_create_node_template(bool p_ignore_not_catalogable = false)
    { return _get_or_create_node_template(T::get_class_static(), p_ignore_not_catalogable); }

    static ActionBuilder _script_node_builder(const String& p_node_type, const String& p_name, const String& p_category, const Dictionary& p_data = Dictionary());
    static Ref<OScriptNode> _get_or_create_node_template(const String& p_node_type, bool p_ignore_not_catalogable = false);
    static Vector<Ref<Action>> _create_categories_from_path(const String& p_category_path, const String& p_icon = String());
    static PackedStringArray _get_native_class_hierarchy(const String& p_class_name);

    static String _get_type_icon(Variant::Type p_type);
    static String _get_type_name(Variant::Type p_type);
    static String _get_method_icon_name(const MethodInfo& p_method);
    static String _get_method_type_icon_name(const MethodInfo& p_method);
    static String _get_builtin_function_category_from_godot_category(const FunctionInfo& p_function_info);
    static Vector<Ref<Action>> _get_actions_for_class(const String& p_class_name, const String& p_category_name, const TypedArray<Dictionary>& p_methods, const TypedArray<Dictionary>& p_properties, const TypedArray<Dictionary>& p_signals);

public:

    // registrar.filter->target_object->get_target()
    static Vector<Ref<Action>> generate_actions_from_object(Object* p_object);
    // registrar.filter->target_classes()
    static Vector<Ref<Action>> generate_actions_from_classes(const PackedStringArray& p_class_names);
    // No specific registrar filter
    static Vector<Ref<Action>> generate_actions_from_class(const StringName& p_class_name);
    static Vector<Ref<Action>> generate_actions_from_script(const Ref<Script>& p_script);

    static Vector<Ref<Action>> generate_actions_from_script_nodes();
    static Vector<Ref<Action>> generate_actions_from_variant_types();
    static Vector<Ref<Action>> generate_actions_from_builtin_functions();
    static Vector<Ref<Action>> generate_actions_from_autoloads();
    static Vector<Ref<Action>> generate_actions_from_native_classes();
    static Vector<Ref<Action>> generate_actions_from_static_script_methods();
    static Vector<Ref<Action>> generate_actions_from_script_global_classes();

    static Vector<Ref<Action>> generate_actions_from_category(const String& p_category, const String& p_icon = String());

    static void free_resources();
};

#endif // ORCHESTRATOR_EDITOR_ACTIONS_INTROSPECTOR_H
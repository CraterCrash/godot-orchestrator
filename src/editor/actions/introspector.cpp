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
#include "editor/actions/introspector.h"

#include "api/extension_db.h"
#include "common/dictionary_utils.h"
#include "common/method_utils.h"
#include "common/property_utils.h"
#include "common/scene_utils.h"
#include "common/settings.h"
#include "common/string_utils.h"
#include "common/variant_utils.h"
#include "core/godot/config/project_settings_cache.h"
#include "core/godot/core_string_names.h"
#include "core/godot/object/class_db.h"
#include "orchestration/nodes.h"
#include "script/language.h"
#include "script/script.h"
#include "script/script_server.h"

#include <godot_cpp/classes/engine.hpp>

HashMap<String, Ref<OScriptNode>> OrchestratorEditorIntrospector::_script_node_cache;

void OrchestratorEditorIntrospector::_apply_method_overrides(const String& p_class_name, MethodInfo& r_method) {
    // For Object.connect, override the flags attribute to disable a list of connect flag enum values
    if (p_class_name == Object::get_class_static() && r_method.name == CoreStringName(_connect)) {
        for (uint32_t j = 0; j < r_method.arguments.size(); j++) {
            if (r_method.arguments[j].name == StringName("flags")) {
                r_method.arguments[j].hint_string = "Default:0,Deferred:1,Persist:2,One Shot:4,Reference Counted:8,Append Source Object:16";
                r_method.arguments[j].hint = PROPERTY_HINT_ENUM;
                break;
            }
        }
    }
}

void OrchestratorEditorIntrospector::_register_static_methods(const String& p_lookup_class, const String& p_register_class, const String& p_category, ActionSet& r_actions) {
    for (const String& function_name : ExtensionDB::get_class_static_function_names(p_lookup_class)) {
        MethodInfo mi;
        ExtensionDB::get_class_method_info(p_lookup_class, function_name, mi);
        if (MethodUtils::is_empty(mi)) {
            ERR_PRINT(vformat("Failed to locate method info for %s.%s", p_lookup_class, function_name));
        }

        PackedStringArray extra;
        if (p_lookup_class != p_register_class) {
            extra.push_back(p_lookup_class);
        }

        r_actions.insert(_create_static_function_action(p_category, function_name, p_register_class, mi, extra));
    }
}

void OrchestratorEditorIntrospector::_register_global_class_static_methods(const String& p_class_name, const String& p_category, ActionSet& r_actions) {
    const TypedArray<Dictionary> methods = ScriptServer::get_global_class(p_class_name).get_static_method_list();
    for (int i = 0; i < methods.size(); i++) {
        const MethodInfo method = DictionaryUtils::to_method(methods[i]);
        r_actions.insert(_create_static_function_action(p_category, method.name, p_class_name, method));
    }
}

OrchestratorEditorIntrospector::ActionBuilder OrchestratorEditorIntrospector::_script_node_builder(
    const String& p_node_type, const String& p_category, const String& p_name, const Dictionary& p_data) {

    const Ref<OScriptNode> node_template = _get_or_create_node_template(p_node_type);
    const bool experimental = node_template->get_flags().has_flag(OScriptNode::EXPERIMENTAL);

    // todo:
    //  for nodes that have static pin configurations, flow control, and many others, it would
    //  be useful to introduce static pin metadata on the nodes that is registered in the call
    //  of _bind_methods, and then we can use that metadata here to set inputs/outputs on the
    //  action, likely relying on a PropertyInfo struct for now.
    return ActionBuilder(p_category, p_name)
        .type(ActionType::ACTION_SPAWN_NODE)
        .icon(node_template->get_icon())
        .tooltip(node_template->get_tooltip_text())
        .keywords(node_template->get_keywords())
        .selectable(true)
        .node_class(p_node_type)
        .flags(experimental ? Action::FLAG_EXPERIMENTAL : Action::FLAG_NONE)
        .data(p_data);
}

Ref<OScriptNode> OrchestratorEditorIntrospector::_get_or_create_node_template(const String& p_node_type, bool p_ignore_not_catalogable) {
    if (!_script_node_cache.has(p_node_type)) {
        const Ref<OScriptNode> node = OScriptNodeFactory::create_node_from_name(p_node_type, nullptr);
        if (!node.is_valid()) {
            WARN_PRINT("Failed to create template node with name " + p_node_type);
            return Ref<OScriptNode>();
        }

        _script_node_cache[p_node_type] = node;
    }

    const Ref<OScriptNode> node = _script_node_cache[p_node_type];
    if (!node->get_flags().has_flag(OScriptNode::CATALOGABLE) && !p_ignore_not_catalogable) {
        WARN_PRINT("Node " + p_node_type + " is not catalogable");
        return Ref<OScriptNode>();
    }

    return node;
}

void OrchestratorEditorIntrospector::_create_categories_from_path(ActionSet& r_actions, const String& p_category_path, const String& p_icon) {
    const PackedStringArray parts = p_category_path.split("/");
    const int64_t last = parts.size() - 1;
    String cumulative;
    for (int64_t i = 0; i < parts.size(); i++) {
        if (i > 0) {
            cumulative += "/";
        }
        cumulative += parts[i];
        r_actions.insert(ActionBuilder(cumulative).icon(i == last ? p_icon : "").build());
    }
}

PackedStringArray OrchestratorEditorIntrospector::_get_native_class_hierarchy(const String& p_class_name) {
    PackedStringArray hierarchy;

    StringName class_name = p_class_name;
    while (!class_name.is_empty() && ClassDB::class_exists(class_name)) {
        hierarchy.push_back(class_name);
        class_name = ClassDB::get_parent_class(class_name);
    }

    // Order is always most descendant to eldest grandparent, i.e. Node3D -> Node -> Object
    return hierarchy;
}

String OrchestratorEditorIntrospector::_get_type_icon(Variant::Type p_type) {
    if (p_type == Variant::NIL) {
        return "Variant";
    }
    return Variant::get_type_name(p_type);
}

String OrchestratorEditorIntrospector::_get_type_name(Variant::Type p_type) {
    switch (p_type) {
        case Variant::NIL: {
            return "Any";
        }
        case Variant::BOOL: {
            return "Boolean";
        }
        case Variant::INT: {
            return "Integer";
        }
        case Variant::FLOAT: {
            return "Float";
        }
        default: {
            return Variant::get_type_name(p_type).replace(" ", "");
        }
    }
}

String OrchestratorEditorIntrospector::_get_method_icon_name(const MethodInfo& p_method) {
    if (!OScriptNodeEvent::is_event_method(p_method)) {
        if (MethodUtils::has_return_value(p_method)) {
            const String return_type = PropertyUtils::get_property_type_name(p_method.return_val);
            if (!return_type.is_empty()) {
                return return_type;
            }
        } else if (p_method.name.capitalize().begins_with("Set ") && p_method.arguments.size() == 1) {
            // Treat it was a setter
            String argument_type = PropertyUtils::get_property_type_name(p_method.arguments[0]);
            if (!argument_type.is_empty()) {
                return argument_type;
            }
        }
    }
    return "MemberMethod";
}

String OrchestratorEditorIntrospector::_get_builtin_function_category_from_godot_category(const FunctionInfo& p_function_info) {
    if (p_function_info.category.match("general")) {
        return "Utilities";
    }

    if (p_function_info.category.match("random")) {
        return "Random Numbers";
    }

    return p_function_info.category.capitalize();
}

PackedStringArray OrchestratorEditorIntrospector::_build_member_keywords(const String& p_name, const String& p_class_name) {
    PackedStringArray keywords = p_name.capitalize().to_lower().split(" ", false);
    keywords.push_back(p_name);
    keywords.push_back(p_class_name);
    return keywords;
}

Ref<OrchestratorEditorActionDefinition> OrchestratorEditorIntrospector::_create_static_function_action(
        const String& p_category, const String& p_name, const String& p_class_name,
        const MethodInfo& p_method, const PackedStringArray& p_extra_keywords) {
    PackedStringArray keywords = _build_member_keywords(p_name, p_class_name);
    keywords.append_array(p_extra_keywords);

    return _script_node_builder<OScriptNodeCallStaticFunction>(
            p_category,
            p_name,
            DictionaryUtils::of({ { "class_name", p_class_name } }))
        .keywords(keywords)
        .class_name(p_class_name)
        .method(p_method)
        .executions(true)
        .build();
}

void OrchestratorEditorIntrospector::_get_actions_for_class(const String& p_class_name, const String& p_category_name,
    const TypedArray<Dictionary>& p_methods, const TypedArray<Dictionary>& p_properties, const TypedArray<Dictionary>& p_signals, ActionSet& r_actions) {

    // Exclude classes that are prefixed with Editor, Orchestrator, and OScript.
    if (p_class_name.begins_with("Editor") || p_class_name.begins_with("Orchestrator") || p_class_name.begins_with("OScript")) {
        return;
    }

    const String properties_category = vformat("Properties/%s", p_category_name);
    _create_categories_from_path(r_actions, properties_category, p_class_name);

    const String methods_category = vformat("Methods/%s", p_category_name);
    _create_categories_from_path(r_actions, methods_category, p_class_name);

    const String static_methods_category = vformat("Methods (Static)/%s", p_category_name);
    _create_categories_from_path(r_actions, static_methods_category, p_class_name);

    const String signals_category = vformat("Signals/%s", p_category_name);
    _create_categories_from_path(r_actions, signals_category, p_class_name);

    PackedStringArray property_methods;
    PackedStringArray internal_method_names;

    ScriptServer::GlobalClass global_class;
    if (ScriptServer::is_global_class(p_class_name)) {
        global_class = ScriptServer::get_global_class(p_class_name);
    }

    if (!p_properties.is_empty()) {
        for (int i = 0; i < p_properties.size(); i++) {
            const PropertyInfo property = DictionaryUtils::to_property(p_properties[i]);

            String getter_name = vformat("get_%s", property.name);
            String setter_name = vformat("set_%s", property.name);

            if (!global_class.name.is_empty()) {
                if (ScriptServer::get_global_class(global_class.base_type).has_property(property.name)) {
                    continue;
                }
            }

            if (property.usage & PROPERTY_USAGE_INTERNAL) {
                String getter = StringUtils::default_if_empty(ClassDB::class_get_property_getter(p_class_name, property.name), getter_name);
                String setter = StringUtils::default_if_empty(ClassDB::class_get_property_setter(p_class_name, property.name), setter_name);
                if (!getter.is_empty()) {
                    internal_method_names.push_back(getter);
                }
                if (!setter.is_empty()) {
                    internal_method_names.push_back(setter);
                }
                continue;
            }

            if (property.usage & PROPERTY_USAGE_CATEGORY || property.usage & PROPERTY_USAGE_GROUP) {
                continue;
            }

            property_methods.push_back(getter_name);
            r_actions.insert(
                ActionBuilder(properties_category, getter_name)
                .type(ActionType::ACTION_GET_PROPERTY)
                .icon(Variant::get_type_name(property.type))
                .tooltip(vformat("Returns the value of property '%s'", property.name))
                .keywords(Array::make("get", p_class_name, property.name))
                .target_class(p_class_name)
                .selectable(true)
                .property(property)
                .class_name(p_class_name)
                .target_classes(Array::make(p_class_name))
                .build());

            property_methods.push_back(setter_name);
            r_actions.insert(
                ActionBuilder(properties_category, setter_name)
                .type(ActionType::ACTION_SET_PROPERTY)
                .icon(Variant::get_type_name(property.type))
                .tooltip(vformat("Set the value of property '%s'", property.name))
                .keywords(Array::make("set", p_class_name, property.name))
                .target_class(p_class_name)
                .selectable(true)
                .property(property)
                .class_name(p_class_name)
                .target_classes(Array::make(p_class_name))
                .executions(true)
                .build());
        }
    }

    if (ClassDB::can_instantiate(p_class_name) || ScriptServer::is_global_class(p_class_name)) {
        r_actions.insert(
            _script_node_builder<OScriptNodeNew>(
                methods_category,
                "Create New Instance",
                DictionaryUtils::of({ { "class_name", p_class_name } }))
            .target_class(p_class_name)
            .tooltip(vformat("Creates a new instance of '%s'.", p_class_name))
            .build());

        r_actions.insert(
            _script_node_builder<OScriptNodeFree>(
                methods_category,
                "Free Instance",
                DictionaryUtils::of({ { "class_name", p_class_name } }))
            .target_class(p_class_name)
            .tooltip(vformat("Free the memory used by the '%s' instance.", p_class_name))
            .build());
    }

    if (!p_methods.is_empty()) {
        const Ref<OScriptNodeEvent> event_node = _get_or_create_node_template<OScriptNodeEvent>(true);
        const Ref<OScriptNodeCallMemberFunction> func_node = _get_or_create_node_template<OScriptNodeCallMemberFunction>();

        const bool prefer_properties_over_methods = ORCHESTRATOR_GET("ui/actions_menu/prefer_properties_over_methods", false);

        for (int i = 0; i < p_methods.size(); i++) {
            MethodInfo method = DictionaryUtils::to_method(p_methods[i]);

            if (internal_method_names.has(method.name) || (method.flags & METHOD_FLAG_STATIC)) {
                continue;
            }

            if (prefer_properties_over_methods && property_methods.has(method.name)) {
                continue;
            }

            PackedStringArray keywords = _build_member_keywords(method.name, p_class_name);

            const bool event_method = OScriptNodeEvent::is_event_method(method);
            if (event_method) {
                r_actions.insert(
                    ActionBuilder(methods_category, method.name)
                    .type(ActionType::ACTION_EVENT)
                    .icon(_get_method_icon_name(method))
                    .keywords(keywords)
                    .target_class(p_class_name)
                    .selectable(true)
                    .method(method)
                    .class_name(p_class_name)
                    .tooltip(vformat("Creates an event callback '%s', called automatically by Godot's object lifecycle.", method.name))
                    .build());
            } else {

                // Hook to override method details as needed
                _apply_method_overrides(p_class_name, method);

                r_actions.insert(
                    ActionBuilder(methods_category, method.name)
                    .type(ActionType::ACTION_CALL_MEMBER_FUNCTION)
                    .icon(_get_method_icon_name(method))
                    .tooltip(func_node->get_tooltip_text())
                    .keywords(keywords)
                    .target_class(p_class_name)
                    .selectable(true)
                    .method(method)
                    .class_name(p_class_name)
                    .executions(true)
                    .build());
            }
        }
    }

    if (!p_signals.is_empty()) {
        const Ref<OScriptNodeEmitSignal> node = _get_or_create_node_template<OScriptNodeEmitSignal>();
        for (int i = 0; i < p_signals.size(); i++) {
            const MethodInfo signal = DictionaryUtils::to_method(p_signals[i]);

            PackedStringArray keywords = node->get_keywords();
            keywords.append_array(signal.name.capitalize().to_lower().split(" ", false));
            keywords.append(signal.name);
            keywords.append(p_class_name);
            keywords.append("emit");
            keywords.append("signal");

            r_actions.insert(
                ActionBuilder(signals_category, vformat("Emit %s", signal.name))
                .type(ActionType::ACTION_EMIT_MEMBER_SIGNAL)
                .icon("Signal")
                .tooltip(node->get_tooltip_text())
                .keywords(keywords)
                .target_class(p_class_name)
                .selectable(true)
                .method(signal)
                .class_name(p_class_name)
                .data(DictionaryUtils::of({ { "target_class", p_class_name } }))
                .executions(true)
                .build());
        }
    }

    _register_static_methods(p_class_name, p_class_name, static_methods_category, r_actions);
}

void OrchestratorEditorIntrospector::_get_actions_for_named_class(const String& p_class_name, ActionSet& r_actions) {
    if (ScriptServer::is_global_class(p_class_name)) {
        const ScriptServer::GlobalClass global_class = ScriptServer::get_global_class(p_class_name);
        _get_actions_for_class(
            global_class.name,
            global_class.name,
            global_class.get_method_list(),
            global_class.get_property_list(),
            global_class.get_signal_list(),
            r_actions);
    } else {
        _get_actions_for_class(
            p_class_name,
            p_class_name,
            ClassDB::class_get_method_list(p_class_name, true),
            ClassDB::class_get_property_list(p_class_name, true),
            ClassDB::class_get_signal_list(p_class_name, true),
            r_actions);
    }
}

void OrchestratorEditorIntrospector::generate_actions_from_object(Object* p_object, ActionSet& r_actions) {
    ERR_FAIL_NULL_MSG(p_object, "Cannot generate actions for a null object");

    // This method generates a set of actions that are specific to a single object.
    // It traverses the object's hierarchy, so there is no need to combine this with generate_actions_from_class

    const Ref<Script> script = p_object->get_script();

    String global_name;
    if (script.is_valid()) {
        global_name = ScriptServer::get_global_name(script);
    }

    String autoload_name = "";
    for (const String& constant_name : OScriptLanguage::get_singleton()->get_global_named_constant_names()) {
        Variant value = OScriptLanguage::get_singleton()->get_any_global_constant(constant_name);
        if (value.get_type() == Variant::OBJECT) {
            Object* other = Object::cast_to<Object>(value);
            if (other == p_object) {
                autoload_name = constant_name;
                break;
            }
        }
    }

    if (!global_name.is_empty()) {
        // The object has a named script attached
        // The script methods, properties, and signals must be registered using the script's class_name
        // rather than adding these as part of the base script type.
        const PackedStringArray class_hierarchy = ScriptServer::get_class_hierarchy(global_name);
        for (const String& class_name : class_hierarchy) {
            _get_actions_for_named_class(class_name, r_actions);
        }
    } else if (script.is_valid()) {
        _get_actions_for_class(
            p_object->get_class(),
            autoload_name.is_empty() ? p_object->get_class() : autoload_name,
            script->get_script_method_list(),
            script->get_script_property_list(),
            script->get_script_signal_list(),
            r_actions);
    }

    const PackedStringArray native_hierarchy = _get_native_class_hierarchy(p_object->get_class());
    for (const String& native_class : native_hierarchy) {
        _get_actions_for_named_class(native_class, r_actions);
    }
}

void OrchestratorEditorIntrospector::generate_actions_from_classes(const PackedStringArray& p_class_names, ActionSet& r_actions) {
    PackedStringArray classes_added;
    for (const String& provided_class_name : p_class_names) {
        PackedStringArray class_names;
        if (ScriptServer::is_global_class(provided_class_name)) {
            class_names = ScriptServer::get_class_hierarchy(provided_class_name, true);
        } else {
            class_names = _get_native_class_hierarchy(provided_class_name);
        }

        for (const String& class_name : class_names) {
            if (!classes_added.has(class_name)) {
                classes_added.append(class_name);
                _get_actions_for_named_class(class_name, r_actions);
            }
        }
    }
}

void OrchestratorEditorIntrospector::generate_actions_from_class(const StringName& p_class_name, ActionSet& r_actions) {
    _get_actions_for_named_class(p_class_name, r_actions);
}

void OrchestratorEditorIntrospector::generate_actions_from_script(const Ref<Script>& p_script, ActionSet& r_actions) {
    const Ref<OScript> oscript = p_script;
    if (oscript.is_valid()) {

        const String base_type = oscript->get_orchestration()->get_base_type();

        const Ref<OScript> base_script = oscript->get_base();
        if (base_script.is_valid() && base_type.begins_with("res://")) {
            // Inherits from another non-named script
            generate_actions_from_script(oscript->get_base(), r_actions);
        }

        for (const Ref<OScriptFunction>& function : oscript->get_orchestration()->get_functions()) {

            if (!function.is_valid() || !function->is_user_defined()) {
                continue;
            }

            const MethodInfo& method = function->get_method_info();

            PackedStringArray keywords = _build_member_keywords(method.name, base_type);

            String description = function->get_description();
            if (description.is_empty()) {
                description = vformat("Calls the script function '%s'.", function->get_function_name());
            }

            r_actions.insert(
                ActionBuilder("Call Function", vformat("Call %s", method.name))
                .type(ActionType::ACTION_CALL_SCRIPT_FUNCTION)
                .icon(_get_method_icon_name(method))
                .tooltip(description)
                .keywords(keywords)
                .target_class(base_type)
                .selectable(true)
                .method(method)
                .class_name(base_type)
                .build());
        }

        for (const Ref<OScriptSignal>& signal : oscript->get_orchestration()->get_custom_signals()) {
            if (!signal.is_valid()) {
                continue;
            }

            const MethodInfo& method = signal->get_method_info();

            PackedStringArray keywords = _build_member_keywords(method.name, base_type);

            r_actions.insert(
                ActionBuilder("Signals", vformat("Emit %s", method.name))
                .type(ActionType::ACTION_EMIT_SIGNAL)
                .icon("MemberSignal")
                .tooltip(signal->get_description())
                .keywords(keywords)
                .target_class(base_type)
                .selectable(true)
                .method(method)
                .class_name(base_type)
                .build());
        }

        for (const Ref<OScriptVariable>& variable : oscript->get_orchestration()->get_variables()) {
            if (!variable.is_valid()) {
                continue;
            }

            const PropertyInfo& property = variable->get_info();

            String get_desc = vformat("Get the value of the variable '%s' in the orchestration.", variable->get_variable_name());
            if (!variable->get_description().is_empty()) {
                get_desc += "\n\n" + variable->get_description();
            }

            String set_desc = vformat("Sets the value of the variable '%s' in the orchestration.", variable->get_variable_name());
            if (!variable->get_description().is_empty()) {
                set_desc += "\n\n" + variable->get_description();
            }

            PackedStringArray keywords = _build_member_keywords(property.name, base_type);

            r_actions.insert(
                ActionBuilder("Variables", vformat("Get %s", property.name))
                .type(ActionType::ACTION_VARIABLE_GET)
                .icon(_get_type_icon(property.type))
                .tooltip(get_desc)
                .keywords(keywords)
                .target_class(base_type)
                .selectable(true)
                .property(property)
                .class_name(base_type)
                .build());

            if (!variable->is_constant()) {
                r_actions.insert(
                    ActionBuilder("Variables", vformat("Set %s", property.name))
                    .type(ActionType::ACTION_VARIABLE_SET)
                    .icon(_get_type_icon(property.type))
                    .tooltip(set_desc)
                    .keywords(keywords)
                    .target_class(base_type)
                    .selectable(true)
                    .property(property)
                    .class_name(base_type)
                    .build());
            }
        }
    }
}

void OrchestratorEditorIntrospector::generate_actions_from_script_nodes(ActionSet& r_actions) {
    // todo:
    //  we need a way to describe the pin types on nodes
    //  this is so that dragging from a port can match a script node

    const Dictionary with_break = DictionaryUtils::of({ { "with_break", true } });
    const Dictionary without_break = DictionaryUtils::of({ { "with_break", false } });
    const Dictionary array_data = DictionaryUtils::of({ { "collection_type", Variant::ARRAY },{ "index_type", Variant::NIL } });

    // Constants
    r_actions.insert(_script_node_builder<OScriptNodeGlobalConstant>("Constants", "Global Constant").build());
    r_actions.insert(_script_node_builder<OScriptNodeGlobalConstant>("Constants", "Global Constant").build());
    r_actions.insert(_script_node_builder<OScriptNodeMathConstant>("Constants", "Math Constant").build());
    r_actions.insert(_script_node_builder<OScriptNodeTypeConstant>("Constants", "Type Constant").build());
    r_actions.insert(_script_node_builder<OScriptNodeClassConstant>("Constants", "Class Constant").build());
    r_actions.insert(_script_node_builder<OScriptNodeSingletonConstant>("Constants", "Singleton Constant").build());

    // Data
    r_actions.insert(_script_node_builder<OScriptNodeArrayGet>("Types/Array/Operators", "Get at Index", array_data).build());
    r_actions.insert(_script_node_builder<OScriptNodeArraySet>("Types/Array/Operators", "Set at Index", array_data).build());
    r_actions.insert(_script_node_builder<OScriptNodeArrayFind>("Types/Array", "Find Array Element").build());
    r_actions.insert(_script_node_builder<OScriptNodeArrayClear>("Types/Array", "Clear Array").build());
    r_actions.insert(_script_node_builder<OScriptNodeArrayAppend>("Types/Array", "Append Arrays").build());
    r_actions.insert(_script_node_builder<OScriptNodeArrayAddElement>("Types/Array", "Add Element").build());
    r_actions.insert(_script_node_builder<OScriptNodeArrayRemoveElement>("Types/Array", "Remove Element").build());
    r_actions.insert(_script_node_builder<OScriptNodeArrayRemoveIndex>("Types/Array", "Remove Element by Index").build());
    r_actions.insert(_script_node_builder<OScriptNodeMakeArray>("Types/Array", "Make Array").build());
    r_actions.insert(_script_node_builder<OScriptNodeMakeDictionary>("Types/Dictionary", "Make Dictionary").build());
    r_actions.insert(_script_node_builder<OScriptNodeDictionarySet>("Types/Dictionary", "Set").build());

    // Dialogue
    r_actions.insert(_script_node_builder<OScriptNodeDialogueMessage>("Dialogue", "Show Message").executions(true).build());
    r_actions.insert(_script_node_builder<OScriptNodeDialogueChoice>("Dialogue", "Show Message Choice").build());

    // Flow Control
    r_actions.insert(_script_node_builder<OScriptNodeBranch>("Flow Control", "Branch").executions(true).build());
    r_actions.insert(_script_node_builder<OScriptNodeChance>("Flow Control", "Chance").executions(true).build());
    r_actions.insert(_script_node_builder<OScriptNodeDelay>("Flow Control", "Delay").executions(true).build());
    r_actions.insert(_script_node_builder<OScriptNodeForEach>("Flow Control", "For Each", without_break).executions(true).build());
    r_actions.insert(_script_node_builder<OScriptNodeForEach>("Flow Control", "For Each With Break", with_break).executions(true).build());
    r_actions.insert(_script_node_builder<OScriptNodeForLoop>("Flow Control", "For Loop", without_break).executions(true).build());
    r_actions.insert(_script_node_builder<OScriptNodeForLoop>("Flow Control", "For Loop With Break", with_break).executions(true).build());
    r_actions.insert(_script_node_builder<OScriptNodeRandom>("Flow Control", "Random").executions(true).build());
    r_actions.insert(_script_node_builder<OScriptNodeSelect>("Flow Control", "Select").executions(true).build());
    r_actions.insert(_script_node_builder<OScriptNodeSequence>("Flow Control", "Sequence").executions(true).build());
    r_actions.insert(_script_node_builder<OScriptNodeSwitch>("Flow Control", "Switch").executions(true).build());
    r_actions.insert(_script_node_builder<OScriptNodeSwitchInteger>("Flow Control", "Switch on Integer").executions(true).build());
    r_actions.insert(_script_node_builder<OScriptNodeSwitchString>("Flow Control", "Switch on String").executions(true).build());
    r_actions.insert(_script_node_builder<OScriptNodeTypeCast>("Flow Control", "Type Cast").executions(true).build());
    r_actions.insert(_script_node_builder<OScriptNodeWhile>("Flow Control", "While").executions(true).build());

    // Switch on Enums
    // todo: this isn't sorting properly
    for (const String& enum_name : ExtensionDB::get_global_enum_names()) {
        const EnumInfo& info = ExtensionDB::get_global_enum(enum_name);
        r_actions.insert(
            _script_node_builder<OScriptNodeSwitchEnum>(
                "Flow Control/Switch On",
                vformat("Switch on %s", info.name),
                DictionaryUtils::of({ { "enum" , info.name } }))
            .executions(true)
            .tooltip(vformat("Performs a switch/match based on the input value for a '%s' enum.", info.name))
            .build());
    }

    // Function Helpers
    r_actions.insert(
        _script_node_builder<OScriptNodeFunctionResult>("", "Add Return Node")
        .graph_type(GraphType::GRAPH_FUNCTION)
        .executions(true)
        .build());

    // Input
    r_actions.insert(_script_node_builder<OScriptNodeInputAction>("Input", "Input Action").build());

    // Memory
    r_actions.insert(_script_node_builder<OScriptNodeNew>("Memory", "New Object").build());
    r_actions.insert(_script_node_builder<OScriptNodeFree>("Memory", "Free Object").build());

    // Resources
    r_actions.insert(_script_node_builder<OScriptNodePreload>("Resource", "Preload Resource").build());
    r_actions.insert(_script_node_builder<OScriptNodeResourcePath>("Resource", "Get Resource Path").build());

    // Scene
    r_actions.insert(_script_node_builder<OScriptNodeInstantiateScene>("Scene", "Instantiate Scene").executions(true).build());
    r_actions.insert(_script_node_builder<OScriptNodeSceneNode>("Scene", "Get Scene Node").build());
    r_actions.insert(_script_node_builder<OScriptNodeSceneTree>("Scene", "Get Scene Tree").build());
    r_actions.insert(_script_node_builder<OScriptNodeSelf>("Scene", "Get Self").build());

    // Signals
    r_actions.insert(_script_node_builder<OScriptNodeAwaitSignal>("Signals", "Await Signal").executions(true).build());

    // Utilities
    r_actions.insert(_script_node_builder<OScriptNodeComment>("Utilities", "Add Comment").build());
    r_actions.insert(_script_node_builder<OScriptNodeAutoload>("Utilities", "Get an Autoload").build());
    r_actions.insert(_script_node_builder<OScriptNodeEngineSingleton>("Utilities", "Get an Engine Singleton").build());
    r_actions.insert(_script_node_builder<OScriptNodePrintString>("Utilities", "Print String").executions(true).build());

    // Variable Assignment
    const Dictionary local_object = DictionaryUtils::of({ { "type", Variant::OBJECT } });
    r_actions.insert(_script_node_builder<OScriptNodeAssignLocalVariable>("Variables", "Assign Local").graph_type(GraphType::GRAPH_FUNCTION).build());
    r_actions.insert(_script_node_builder<OScriptNodeAssignLocalVariable>("Utilities/Macros", "Assign Local").graph_type(GraphType::GRAPH_MACRO).build());
    r_actions.insert(_script_node_builder<OScriptNodeLocalVariable>("Variables", "Local Object", local_object).graph_type(GraphType::GRAPH_FUNCTION).build());
    r_actions.insert(_script_node_builder<OScriptNodeLocalVariable>("Utilities/Macros", "Local Object", local_object).graph_type(GraphType::GRAPH_MACRO).build());

    // List each engine singleton directly
    for (const String& name : Engine::get_singleton()->get_singleton_list()) {
        const Dictionary data = DictionaryUtils::of({ { "singleton_name", name } });
        r_actions.insert(_script_node_builder<OScriptNodeEngineSingleton>("Singleton", name, data)
            .no_capitalize(true)
            .tooltip("Obtain a reference to the " + name + " singleton.")
            .build());
    }

    // Orchestrator Script Language Functions
    const TypedArray<Dictionary> language_functions = OScriptLanguage::get_singleton()->_get_public_functions();
    for (int i = 0; i < language_functions.size(); i++) {
        const MethodInfo& mi = DictionaryUtils::to_method(language_functions[i]);
        // Exclude any internal methods that are prefixed with `_`.
        if (mi.name.begins_with("_")) {
            continue;
        }

        r_actions.insert(_script_node_builder<OScriptNodeCallBuiltinFunction>("@OScript", mi.name, language_functions[i]).build());
    }
}

void OrchestratorEditorIntrospector::generate_actions_from_variant_types(ActionSet& r_actions) {
    for (const BuiltInType& type : ExtensionDB::get_builtin_types()) {
        // Nothing to show for NIL/Any
        if (type.type == Variant::NIL) {
            continue;
        }

        const String type_icon = _get_type_icon(type.type);
        const String type_name = _get_type_name(type.type);

        const String category = vformat("Types/%s", type_name);

        // Register top level category with icon for type
        _create_categories_from_path(r_actions, category, type_icon);

        const Dictionary type_dict = DictionaryUtils::of({ { "type", type.type } });

        // Local variables for macros
        r_actions.insert(
            _script_node_builder<OScriptNodeLocalVariable>(
                category,
                vformat("Local %s Variable", type_name), type_dict)
            //.graph_type(GraphType::GRAPH_MACRO)
            .build());

        if (!type.properties.is_empty()) {
            if (OScriptNodeCompose::is_supported(type.type)) {
                r_actions.insert(
                    _script_node_builder<OScriptNodeCompose>(
                        category,
                        vformat("Make %s", type_name),
                        type_dict)
                    .build());
            }

            if (type.type == Variant::COLOR) {
                r_actions.insert(
                _script_node_builder<OScriptNodeDecompose>(
                    category,
                    vformat("Break %s into RGBA", type_name),
                    DictionaryUtils::of({ { "type", type.type }, { "sub_type", OScriptNodeDecompose::ST_COLOR_RGBA } }))
                .no_capitalize(true)
                .build());

                r_actions.insert(
                    _script_node_builder<OScriptNodeDecompose>(
                        category,
                        vformat("Break %s into RGBA8", type_name),
                        DictionaryUtils::of({ { "type", type.type }, { "sub_type", OScriptNodeDecompose::ST_COLOR_RGBA8 } }))
                    .no_capitalize(true)
                    .build());

                r_actions.insert(
                    _script_node_builder<OScriptNodeDecompose>(
                        category,
                        vformat("Break %s into HSV", type_name),
                        DictionaryUtils::of({ { "type", type.type }, { "sub_type", OScriptNodeDecompose::ST_COLOR_HSV } }))
                    .no_capitalize(true)
                    .build());

                r_actions.insert(
                    _script_node_builder<OScriptNodeDecompose>(
                        category,
                        vformat("Break %s into OK HSL", type_name),
                        DictionaryUtils::of({ { "type", type.type }, { "sub_type", OScriptNodeDecompose::ST_COLOR_OK_HSL } }))
                    .no_capitalize(true)
                    .build());
            } else {
                r_actions.insert(
                _script_node_builder<OScriptNodeDecompose>(
                    category,
                    vformat("Break %s", type_name),
                    type_dict)
                .build());
            }
        }

        if (!type.constructors.is_empty()) {
            for (const ConstructorInfo& info : type.constructors) {
                if (!info.arguments.is_empty()) {
                    if (!OScriptNodeComposeFrom::is_supported(type.type, info.arguments)) {
                        continue;
                    }

                    Vector<String> argument_types;
                    Array arguments;
                    for (const PropertyInfo& argument : info.arguments) {
                        String argument_name;
                        if (argument.name.to_lower().match("from")) {
                            argument_name = VariantUtils::get_friendly_type_name(argument.type);
                        } else {
                            argument_name = argument.name.capitalize();
                        }
                        argument_types.push_back(argument_name);
                        arguments.push_back(DictionaryUtils::from_property(argument));
                    }

                    const String args = StringUtils::join(" and ", argument_types);
                    const Dictionary ctor_dict = DictionaryUtils::of(
                        { { "type", type.type }, { "constructor_args", arguments } });

                    r_actions.insert(
                        _script_node_builder<OScriptNodeComposeFrom>(
                            category,
                            vformat("Make %s From %s", type_name, args),
                            ctor_dict)
                        .build());
                }
            }
        }

        for (const MethodInfo& method : type.get_method_list()) {
            const Dictionary method_dict = DictionaryUtils::from_method(method);

            if (method.flags & METHOD_FLAG_STATIC) {
                r_actions.insert(
                    _script_node_builder<OScriptNodeCallStaticFunction>(
                        category,
                        method.name,
                        DictionaryUtils::of({ { "class_name", type.name } }))
                    .executions(true)
                    .method(method)
                    .build());
            } else {
                r_actions.insert(
                    _script_node_builder<OScriptNodeCallMemberFunction>(
                        category,
                        method.name,
                        DictionaryUtils::of({ { "target_type", type.type }, { "method", method_dict } }))
                    .method(method)
                    .target_class(Variant::get_type_name(type.type))
                    .executions(true)
                    .build());
            }
        }

        if (OScriptNodeOperator::is_supported(type.type)) {
            const String operator_category = vformat("%s/Operators", category);
            _create_categories_from_path(r_actions, operator_category);

            for (const OperatorInfo& info : type.operators) {
                if (!OScriptNodeOperator::is_operator_supported(info)) {
                    continue;
                }

                String operator_name;
                if (!info.name.match("Not")) {
                    operator_name = vformat("%s %s", type_name, info.name);
                } else {
                    operator_name = info.name;
                }

                if (!info.right_type_name.is_empty()) {
                    operator_name += vformat(" %s", _get_type_name(info.right_type));
                }

                const Dictionary data = DictionaryUtils::of({ { "op", info.op },
                                                              { "code", info.code },
                                                              { "name", info.name },
                                                              { "type", type.type },
                                                              { "left_type", info.left_type },
                                                              { "left_type_name", info.left_type_name },
                                                              { "right_type", info.right_type },
                                                              { "right_type_name", info.right_type_name },
                                                              { "return_type", info.return_type } });

                PackedStringArray keywords = Array::make(info.name, info.code, info.left_type_name, info.right_type_name);
                if (info.op == VariantOperators::OP_MODULE) {
                    keywords.push_back("mod");
                    keywords.push_back("modulus");
                }

                r_actions.insert(_script_node_builder<OScriptNodeOperator>(operator_category, operator_name, data)
                    .inputs(Vector({ info.left_type, info.right_type }))
                    .outputs(Vector({ info.return_type }))
                    .keywords(keywords)
                    .no_capitalize(true)
                    .build());
            }
        }

        if (type.index_returning_type != Variant::NIL && type.type >= Variant::ARRAY) {
            const String operator_category = vformat("%s/Operators", category);
            const Dictionary data = DictionaryUtils::of({ { "collection_type", type.type }, { "index_type", type.index_returning_type } });

            r_actions.insert(_script_node_builder<OScriptNodeArrayGet>(operator_category, "Get At Index", data).build());
            r_actions.insert(_script_node_builder<OScriptNodeArraySet>(operator_category, "Set At Index", data).build());
        }
    }
}

void OrchestratorEditorIntrospector::generate_actions_from_builtin_functions(ActionSet& r_actions) {
    for (const FunctionInfo& info : ExtensionDB::get_utility_functions()) {
        const MethodInfo& method = info.method;

        // Godot exports utility functions under "math", "random", and "general"
        // We remap "general" to "utilities" and "random" to "random numbers"
        const String category = _get_builtin_function_category_from_godot_category(info);
        _create_categories_from_path(r_actions, category);

        r_actions.insert(
            _script_node_builder<OScriptNodeCallBuiltinFunction>(
                category,
                method.name,
                DictionaryUtils::from_method(method))
            .method(method)
            .tooltip(vformat("Calls the specified built-in Godot function '%s'.", method.name))
            .build());
    }
}

void OrchestratorEditorIntrospector::generate_actions_from_autoloads(ActionSet& r_actions) {
    OrchestratorProjectSettingsCache* cache = OrchestratorProjectSettingsCache::get_singleton();
    for (const String& name : cache->get_autoload_names()) {
        r_actions.insert(
            _script_node_builder<OScriptNodeAutoload>(
                vformat("Project/Autoloads"),
                vformat("Get %s", name),
                DictionaryUtils::of({ { "class_name", name } }))
            .tooltip(vformat("Get a reference to the project autoload %s.", name))
            .no_capitalize(true)
            .build());
    }
}

void OrchestratorEditorIntrospector::generate_actions_from_native_classes(ActionSet& r_actions) {
    for (const String& class_name : ClassDB::get_class_list()) {
        generate_actions_from_class(class_name, r_actions);
    }
}

void OrchestratorEditorIntrospector::generate_actions_from_static_script_methods(ActionSet& r_actions) {
    for (const String& global_class : ScriptServer::get_global_class_list()) {
        _register_global_class_static_methods(global_class, vformat("Methods (Static)/%s", global_class), r_actions);
    }
}

void OrchestratorEditorIntrospector::generate_actions_from_script_global_classes(ActionSet& r_actions) {
    for (const String& global_name : ScriptServer::get_global_class_list()) {
        // The object has a named script attached
        // The script methods, properties, and signals must be registered using the script's class_name
        // rather than adding these as part of the base script type.
        const PackedStringArray class_hierarchy = ScriptServer::get_class_hierarchy(global_name, false);
        for (const String& class_name : class_hierarchy) {
            _get_actions_for_named_class(class_name, r_actions);
        }

        const String static_category_name = vformat("Methods (Static)/%s", global_name);
        _register_global_class_static_methods(global_name, static_category_name, r_actions);

        // Also register static methods from parent type as accessible via the script type.
        const String base_type = ScriptServer::get_global_class(global_name).base_type;
        _register_static_methods(base_type, global_name, static_category_name, r_actions);
    }
}

Vector<Ref<OrchestratorEditorIntrospector::Action>> OrchestratorEditorIntrospector::generate_actions_from_category(
    const String& p_category, const String& p_icon) {

    ActionSet actions;
    _create_categories_from_path(actions, p_category, p_icon);

    Vector<Ref<Action>> result;
    for (const Ref<Action>& action : actions) {
        result.push_back(action);
    }

    return result;
}

void OrchestratorEditorIntrospector::free_resources() {
    _script_node_cache.clear();
}

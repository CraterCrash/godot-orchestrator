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
#include "common/version.h"
#include "core/godot/config/project_settings.h"
#include "script/nodes/script_nodes.h"
#include "script/script_server.h"

#include <godot_cpp/classes/engine.hpp>

HashMap<String, Ref<OScriptNode>> OrchestratorEditorIntrospector::_script_node_cache;

OrchestratorEditorIntrospector::ActionBuilder OrchestratorEditorIntrospector::_script_node_builder(
    const String& p_node_type, const String& p_category, const String& p_name, const Dictionary& p_data)
{
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

Ref<OScriptNode> OrchestratorEditorIntrospector::_get_or_create_node_template(const String& p_node_type, bool p_ignore_not_catalogable)
{
    if (!_script_node_cache.has(p_node_type))
    {
        const Ref<OScriptNode> node = OScriptNodeFactory::create_node_from_name(p_node_type, nullptr);
        if (!node.is_valid())
        {
            WARN_PRINT("Failed to create template node with name " + p_node_type);
            return Ref<OScriptNode>();
        }

        _script_node_cache[p_node_type] = node;
    }

    const Ref<OScriptNode> node = _script_node_cache[p_node_type];
    if (!node->get_flags().has_flag(OScriptNode::CATALOGABLE) && !p_ignore_not_catalogable)
    {
        WARN_PRINT("Node " + p_node_type + " is not catalogable");
        return Ref<OScriptNode>();
    }

    return node;
}

Vector<Ref<OrchestratorEditorIntrospector::Action>> OrchestratorEditorIntrospector::_create_categories_from_path(
    const String& p_category_path, const String& p_icon)
{
    const PackedStringArray category_parts = p_category_path.split("/");

    Vector<String> categories;
    String cumulative;
    for (int i = 0; i < category_parts.size(); i++)
    {
        if (i > 0)
            cumulative += "/";

        cumulative += category_parts[i];
        categories.push_back(cumulative);
    }

    Vector<Ref<Action>> category_actions;
    for (const String& category : categories)
        category_actions.push_back(ActionBuilder(category).build());

    // Set icon on the leaf descendant
    category_actions[category_actions.size() - 1]->icon = p_icon;

    return category_actions;
}

PackedStringArray OrchestratorEditorIntrospector::_get_native_class_hierarchy(const String& p_class_name)
{
    PackedStringArray hierarchy;

    StringName class_name = p_class_name;
    while (!class_name.is_empty() && ClassDB::class_exists(class_name))
    {
        hierarchy.push_back(class_name);
        class_name = ClassDB::get_parent_class(class_name);
    }

    // Order is always most descendant to eldest grandparent, i.e. Node3D -> Node -> Object
    return hierarchy;
}

String OrchestratorEditorIntrospector::_get_type_icon(Variant::Type p_type)
{
    if (p_type == Variant::NIL)
        return "Variant";

    return Variant::get_type_name(p_type);
}

String OrchestratorEditorIntrospector::_get_type_name(Variant::Type p_type)
{
    switch (p_type)
    {
        case Variant::NIL:
            return "Any";
        case Variant::BOOL:
            return "Boolean";
            break;
        case Variant::INT:
            return "Integer";
        case Variant::FLOAT:
            return "Float";
        default:
            return Variant::get_type_name(p_type).replace(" ", "");
    }
}

String OrchestratorEditorIntrospector::_get_method_icon_name(const MethodInfo& p_method)
{
    if (!OScriptNodeEvent::is_event_method(p_method))
    {
        if (MethodUtils::has_return_value(p_method))
        {
            const String return_type = PropertyUtils::get_property_type_name(p_method.return_val);
            if (!return_type.is_empty())
                return return_type;
        }
        else if (p_method.name.capitalize().begins_with("Set ") && p_method.arguments.size() == 1)
        {
            // Treat it was a setter
            String argument_type = PropertyUtils::get_property_type_name(p_method.arguments[0]);
            if (!argument_type.is_empty())
                return argument_type;
        }
    }
    return "MemberMethod";
}

String OrchestratorEditorIntrospector::_get_method_type_icon_name(const MethodInfo& p_method)
{
    const bool event_method = OScriptNodeEvent::is_event_method(p_method);
    if (!event_method && p_method.flags & METHOD_FLAG_VIRTUAL)
        return "MethodOverride";

    if (event_method)
        return "MemberSignal";

    return "MemberMethod";
}

String OrchestratorEditorIntrospector::_get_builtin_function_category_from_godot_category(const FunctionInfo& p_function_info)
{
    if (p_function_info.category.match("general"))
        return "Utilities";

    if (p_function_info.category.match("random"))
        return "Random Numbers";

    return p_function_info.category.capitalize();
}

Vector<Ref<OrchestratorEditorIntrospector::Action>> OrchestratorEditorIntrospector::_get_actions_for_class(
    const String& p_class_name, const String& p_category_name,
    const TypedArray<Dictionary>& p_methods, const TypedArray<Dictionary>& p_properties, const TypedArray<Dictionary>& p_signals)
{
    Vector<Ref<Action>> actions;

    // Exclude classes that are prefixed with Editor, Orchestrator, and OScript.
    if (p_class_name.begins_with("Editor") || p_class_name.begins_with("Orchestrator") || p_class_name.begins_with("OScript"))
        return actions;

    const String properties_category = vformat("Properties/%s", p_category_name);
    actions.append_array(_create_categories_from_path(properties_category, p_class_name));

    const String methods_category = vformat("Methods/%s", p_category_name);
    actions.append_array(_create_categories_from_path(methods_category, p_class_name));

    const String static_methods_category = vformat("Methods (Static)/%s", p_category_name);
    actions.append_array(_create_categories_from_path(static_methods_category, p_class_name));

    const String signals_category = vformat("Signals/%s", p_category_name);
    actions.append_array(_create_categories_from_path(signals_category, p_class_name));

    PackedStringArray property_methods;

    ScriptServer::GlobalClass global_class;
    if (ScriptServer::is_global_class(p_class_name))
        global_class = ScriptServer::get_global_class(p_class_name);

    if (!p_properties.is_empty())
    {
        for (int i = 0; i < p_properties.size(); i++)
        {
            const PropertyInfo property = DictionaryUtils::to_property(p_properties[i]);

            if (property.usage & PROPERTY_USAGE_INTERNAL)
                continue;

            if (property.usage & PROPERTY_USAGE_CATEGORY || property.usage & PROPERTY_USAGE_GROUP)
                continue;

            // if (property.name.begins_with("_"))
            //     continue;

            // For script variables, check whether it's defined in the parent or child type. When it
            // is defined in the parent script types, it should be skipped.
            if (property.usage & PROPERTY_USAGE_SCRIPT_VARIABLE && !global_class.name.is_empty())
            {
                if (ScriptServer::get_global_class(global_class.base_type).has_property(property.name))
                    continue;
            }

            #if GODOT_VERSION >= 0x040400
            String getter_name = ClassDB::class_get_property_getter(p_class_name, property.name);
            if (getter_name.is_empty() && property.usage & PROPERTY_USAGE_SCRIPT_VARIABLE)
                getter_name = "get_" + property.name;

            String setter_name = ClassDB::class_get_property_setter(p_class_name, property.name);
            if (setter_name.is_empty() && property.usage & PROPERTY_USAGE_SCRIPT_VARIABLE)
                setter_name = "set_" + property.name;
            #else
            String getter_name = vformat("get_%s", property.name);
            String setter_name = vformat("set_%s", property.name);

            bool has_getter = false;
            if ((global_class.name.is_empty() && ClassDB::class_has_method(p_class_name, getter_name))
                || (!global_class.name.is_empty() && global_class.has_method(getter_name)))
                has_getter = true;

            if (!has_getter)
                getter_name = "";

            bool has_setter = false;
            if ((global_class.name.is_empty() && ClassDB::class_has_method(p_class_name, setter_name))
                || (!global_class.name.is_empty() && global_class.has_method(setter_name)))
                has_setter = true;

            if (!has_setter)
                setter_name = "";

            #endif

            if (!getter_name.is_empty())
            {
                property_methods.push_back(getter_name);

                actions.append(
                    ActionBuilder(properties_category, getter_name)
                    .type(ActionType::ACTION_GET_PROPERTY)
                    .icon(Variant::get_type_name(property.type))
                    .type_icon("MemberProperty")
                    .tooltip(vformat("Returns the value of property '%s'", property.name))
                    .keywords(Array::make("get", p_class_name, property.name))
                    .target_class(p_class_name)
                    .selectable(true)
                    .property(property)
                    .class_name(p_class_name)
                    .target_classes(Array::make(p_class_name))
                    .build());
            }

            if (!setter_name.is_empty())
            {
                property_methods.push_back(setter_name);

                actions.append(
                    ActionBuilder(properties_category, setter_name)
                    .type(ActionType::ACTION_SET_PROPERTY)
                    .icon(Variant::get_type_name(property.type))
                    .type_icon("MemberProperty")
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
    }

    if (ClassDB::can_instantiate(p_class_name) || ScriptServer::is_global_class(p_class_name))
    {
        actions.push_back(
            _script_node_builder<OScriptNodeNew>(
                methods_category,
                "Create New Instance",
                DictionaryUtils::of({ { "class_name", p_class_name } }))
            .target_class(p_class_name)
            .build());

        actions.push_back(
            _script_node_builder<OScriptNodeFree>(
                methods_category,
                "Free Instance",
                DictionaryUtils::of({ { "class_name", p_class_name } }))
            .target_class(p_class_name)
            .build());
    }

    if (!p_methods.is_empty())
    {
        const Ref<OScriptNodeEvent> event_node = _get_or_create_node_template<OScriptNodeEvent>(true);
        const Ref<OScriptNodeCallMemberFunction> func_node = _get_or_create_node_template<OScriptNodeCallMemberFunction>();

        const bool prefer_properties_over_methods = ORCHESTRATOR_GET("ui/actions_menu/prefer_properties_over_methods", false);

        for (int i = 0; i < p_methods.size(); i++)
        {
            const MethodInfo method = DictionaryUtils::to_method(p_methods[i]);

            // Skip private methods
            // if (method.name.begins_with("_") && !(method.flags & METHOD_FLAG_VIRTUAL))
            //     continue;

            // // Skip internal methods
            // if (method.name.begins_with("@"))
            //     continue;

            if (prefer_properties_over_methods && property_methods.has(method.name))
                continue;

            PackedStringArray keywords = method.name.capitalize().to_lower().split(" ", false);
            keywords.push_back(method.name);
            keywords.push_back(p_class_name);

            const bool event_method = OScriptNodeEvent::is_event_method(method);
            if (event_method)
            {
                actions.push_back(
                    ActionBuilder(methods_category, method.name)
                    .type(ActionType::ACTION_EVENT)
                    .icon(_get_method_icon_name(method))
                    .type_icon(_get_method_type_icon_name(method))
                    .tooltip(event_node->get_tooltip_text())
                    .keywords(keywords)
                    .target_class(p_class_name)
                    .selectable(true)
                    .method(method)
                    .class_name(p_class_name)
                    .build());
            }
            else
            {
                actions.push_back(
                    ActionBuilder(methods_category, method.name)
                    .type(ActionType::ACTION_CALL_MEMBER_FUNCTION)
                    .icon(_get_method_icon_name(method))
                    .type_icon(_get_method_type_icon_name(method))
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

    if (!p_signals.is_empty())
    {
        const Ref<OScriptNodeEmitSignal> node = _get_or_create_node_template<OScriptNodeEmitSignal>();
        for (int i = 0; i < p_signals.size(); i++)
        {
            const MethodInfo signal = DictionaryUtils::to_method(p_signals[i]);

            PackedStringArray keywords = node->get_keywords();
            keywords.append_array(signal.name.capitalize().to_lower().split(" ", false));
            keywords.append(signal.name);
            keywords.append(p_class_name);
            keywords.append("emit");
            keywords.append("signal");

            actions.append(
                ActionBuilder(signals_category, vformat("Emit %s", signal.name))
                .type(ActionType::ACTION_EMIT_MEMBER_SIGNAL)
                .icon("Signal")
                .type_icon("MemberSignal")
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

    const PackedStringArray static_functions = ExtensionDB::get_class_static_function_names(p_class_name);
    if (!static_functions.is_empty())
    {
        const Ref<OScriptNodeCallStaticFunction> node = _get_or_create_node_template<OScriptNodeCallStaticFunction>();

        for (const String& function_name : static_functions)
        {
            PackedStringArray keywords = node->get_keywords();
            keywords.append_array(function_name.capitalize().to_lower().split(" ", false));
            keywords.append(p_class_name);

            actions.append(
                ActionBuilder(static_methods_category, vformat("%s", function_name))
                .type(ActionType::ACTION_SPAWN_NODE)
                .icon("AudioBusSolo")
                .type_icon("AudioBusSolo")
                .tooltip(node->get_tooltip_text())
                .keywords(keywords)
                .selectable(true)
                .node_class(node->get_class())
                .data(DictionaryUtils::of({ { "class_name", p_class_name }, { "method_name", function_name } }))
                .executions(true)
                .build());
        }
    }

    return actions;
}

Vector<Ref<OrchestratorEditorIntrospector::Action>> OrchestratorEditorIntrospector::generate_actions_from_object(
    Object* p_object)
{
    Vector<Ref<Action>> actions;
    ERR_FAIL_NULL_V_MSG(p_object, actions, "Cannot generate actions for a null object");

    // This method generates a set of actions that are specific to a single object.
    // It traverses the object's hierarchy, so there is no need to combine this with generate_actions_from_class

    const Ref<Script> script = p_object->get_script();

    String global_name;
    if (script.is_valid())
        global_name = ScriptServer::get_global_name(script);

    String autoload_name = "";
    for (const String& constant_name : OScriptLanguage::get_singleton()->get_global_named_constant_names())
    {
        Variant value = OScriptLanguage::get_singleton()->get_any_global_constant(constant_name);
        if (value.get_type() == Variant::OBJECT)
        {
            Object* other = Object::cast_to<Object>(value);
            if (other == p_object)
            {
                autoload_name = constant_name;
                break;
            }
        }
    }

    if (!global_name.is_empty())
    {
        // The object has a named script attached
        // The script methods, properties, and signals must be registered using the script's class_name
        // rather than adding these as part of the base script type.
        const PackedStringArray class_hierarchy = ScriptServer::get_class_hierarchy(global_name);
        for (const String& class_name : class_hierarchy)
        {
            const ScriptServer::GlobalClass global_class = ScriptServer::get_global_class(class_name);
            actions.append_array(
                _get_actions_for_class(
                    global_class.name,
                    global_class.name,
                    global_class.get_method_list(),
                    global_class.get_property_list(),
                    global_class.get_signal_list()));
        }
    }
    else if (script.is_valid())
    {
        actions.append_array(
            _get_actions_for_class(
                p_object->get_class(),
                autoload_name.is_empty() ? p_object->get_class() : autoload_name,
                script->get_script_method_list(),
                script->get_script_property_list(),
                script->get_script_signal_list()));
    }

    const PackedStringArray native_hierarchy = _get_native_class_hierarchy(p_object->get_class());
    for (const String& native_class : native_hierarchy)
    {
        actions.append_array(
            _get_actions_for_class(
                native_class,
                native_class,
                ClassDB::class_get_method_list(native_class, true),
                ClassDB::class_get_property_list(native_class, true),
                ClassDB::class_get_signal_list(native_class, true)));
    }

    return actions;
}

Vector<Ref<OrchestratorEditorIntrospector::Action>> OrchestratorEditorIntrospector::generate_actions_from_classes(
    const PackedStringArray& p_class_names)
{
    Vector<Ref<Action>> actions;

    PackedStringArray classes_added;
    for (const String& provided_class_name : p_class_names)
    {
        PackedStringArray class_names;
        if (ScriptServer::is_global_class(provided_class_name))
            class_names = ScriptServer::get_class_hierarchy(provided_class_name, true);
        else
            class_names = _get_native_class_hierarchy(provided_class_name);

        for (const String& class_name : class_names)
        {
            if (!classes_added.has(class_name))
            {
                classes_added.append(class_name);
                if (ScriptServer::is_global_class(class_name))
                {
                    const ScriptServer::GlobalClass global_class = ScriptServer::get_global_class(class_name);
                    actions.append_array(
                        _get_actions_for_class(
                            class_name,
                            class_name,
                            global_class.get_method_list(),
                            global_class.get_property_list(),
                            global_class.get_signal_list()));
                }
                else
                {
                    actions.append_array(
                        _get_actions_for_class(
                            class_name,
                            class_name,
                            ClassDB::class_get_method_list(class_name, true),
                            ClassDB::class_get_property_list(class_name, true),
                            ClassDB::class_get_signal_list(class_name, true)));
                }
            }
        }
    }

    return actions;
}

Vector<Ref<OrchestratorEditorIntrospector::Action>> OrchestratorEditorIntrospector::generate_actions_from_class(
    const StringName& p_class_name)
{
    Vector<Ref<Action>> actions;

    if (ScriptServer::is_global_class(p_class_name))
    {
        const ScriptServer::GlobalClass global_class = ScriptServer::get_global_class(p_class_name);
        actions.append_array(
            _get_actions_for_class(
                global_class.name,
                global_class.name,
                global_class.get_method_list(),
                global_class.get_property_list(),
                global_class.get_signal_list()));
    }
    else
    {
        actions.append_array(
            _get_actions_for_class(
                p_class_name,
                p_class_name,
                ClassDB::class_get_method_list(p_class_name, true),
                ClassDB::class_get_property_list(p_class_name, true),
                ClassDB::class_get_signal_list(p_class_name, true)));
    }

    return actions;
}

Vector<Ref<OrchestratorEditorIntrospector::Action>> OrchestratorEditorIntrospector::generate_actions_from_script(
    const Ref<Script>& p_script)
{
    Vector<Ref<Action>> actions;

    const Ref<OScript> oscript = p_script;
    if (oscript.is_valid())
    {
        const String base_type = oscript->get_orchestration()->get_base_type(); // oscript->get_instance_base_type();
        for (const Ref<OScriptFunction>& function : oscript->get_orchestration()->get_functions())
        {
            if (!function.is_valid() || !function->is_user_defined())
                continue;

            const MethodInfo& method = function->get_method_info();

            // // Skip private methods
            // //
            // // We previously omitted method names that began with an `_` but this does not align with
            // if (method.name.begins_with("_") && !(method.flags & METHOD_FLAG_VIRTUAL))
            //     continue;
            //
            // // Skip internal methods
            // if (method.name.begins_with("@"))
            //     continue;

            PackedStringArray keywords = method.name.capitalize().to_lower().split(" ", false);
            keywords.append(method.name);
            keywords.append(base_type);

            actions.push_back(
                ActionBuilder("Call Function", vformat("Call %s", method.name))
                .type(ActionType::ACTION_CALL_SCRIPT_FUNCTION)
                .icon(_get_method_icon_name(method))
                .type_icon(_get_method_type_icon_name(method))
                .tooltip(function->get_description())
                .keywords(keywords)
                .target_class(base_type)
                .selectable(true)
                .method(method)
                .class_name(base_type)
                .build());
        }

        for (const Ref<OScriptSignal>& signal : oscript->get_orchestration()->get_custom_signals())
        {
            if (!signal.is_valid())
                continue;

            const MethodInfo& method = signal->get_method_info();

            PackedStringArray keywords = method.name.capitalize().to_lower().split(" ", false);
            keywords.append(method.name);
            keywords.append(base_type);

            actions.push_back(
                ActionBuilder("Signals", vformat("Emit %s", method.name))
                .type(ActionType::ACTION_EMIT_SIGNAL)
                .icon("MemberSignal")
                .type_icon("MemberSignal")
                .tooltip(signal->get_description())
                .keywords(keywords)
                .target_class(base_type)
                .selectable(true)
                .method(method)
                .class_name(base_type)
                .build());
        }

        for (const Ref<OScriptVariable>& variable : oscript->get_orchestration()->get_variables())
        {
            if (!variable.is_valid())
                continue;

            const PropertyInfo& property = variable->get_info();

            String get_desc = vformat("Get the value of the variable '%s' in the orchestration.", variable->get_variable_name());
            if (!variable->get_description().is_empty())
                get_desc += "\n\n" + variable->get_description();

            String set_desc = vformat("Sets the value of the variable '%s' in the orchestration.", variable->get_variable_name());
            if (!variable->get_description().is_empty())
                set_desc += "\n\n" + variable->get_description();

            PackedStringArray keywords = property.name.capitalize().to_lower().split(" ", false);
            keywords.append(property.name);
            keywords.append(base_type);

            actions.push_back(
                ActionBuilder("Variables", vformat("Get %s", property.name))
                .type(ActionType::ACTION_VARIABLE_GET)
                .icon(_get_type_icon(property.type))
                .type_icon("MemberProperty")
                .tooltip(get_desc)
                .keywords(keywords)
                .target_class(base_type)
                .selectable(true)
                .property(property)
                .class_name(base_type)
                .build());

            if (!variable->is_constant())
            {
                actions.push_back(
                    ActionBuilder("Variables", vformat("Set %s", property.name))
                    .type(ActionType::ACTION_VARIABLE_SET)
                    .icon(_get_type_icon(property.type))
                    .type_icon("MemberProperty")
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

    return actions;
}

Vector<Ref<OrchestratorEditorIntrospector::Action>> OrchestratorEditorIntrospector::generate_actions_from_script_nodes()
{
    // todo:
    //  we need a way to describe the pin types on nodes
    //  this is so that dragging from a port can match a script node

    const Dictionary with_break = DictionaryUtils::of({ { "with_break", true } });
    const Dictionary without_break = DictionaryUtils::of({ { "with_break", false } });
    const Dictionary array_data = DictionaryUtils::of({ { "collection_type", Variant::ARRAY },{ "index_type", Variant::NIL } });

    Vector<Ref<Action>> actions;

    // Constants
    actions.push_back(_script_node_builder<OScriptNodeGlobalConstant>("Constants", "Global Constant").build());
    actions.push_back(_script_node_builder<OScriptNodeGlobalConstant>("Constants", "Global Constant").build());
    actions.push_back(_script_node_builder<OScriptNodeMathConstant>("Constants", "Math Constant").build());
    actions.push_back(_script_node_builder<OScriptNodeTypeConstant>("Constants", "Type Constant").build());
    actions.push_back(_script_node_builder<OScriptNodeClassConstant>("Constants", "Class Constant").build());
    actions.push_back(_script_node_builder<OScriptNodeSingletonConstant>("Constants", "Singleton Constant").build());

    // Data
    actions.push_back(_script_node_builder<OScriptNodeArrayGet>("Types/Array/Operators", "Get at Index", array_data).build());
    actions.push_back(_script_node_builder<OScriptNodeArraySet>("Types/Array/Operators", "Set at Index", array_data).build());
    actions.push_back(_script_node_builder<OScriptNodeArrayFind>("Types/Array", "Find Array Element").build());
    actions.push_back(_script_node_builder<OScriptNodeArrayClear>("Types/Array", "Clear Array").build());
    actions.push_back(_script_node_builder<OScriptNodeArrayAppend>("Types/Array", "Append Arrays").build());
    actions.push_back(_script_node_builder<OScriptNodeArrayAddElement>("Types/Array", "Add Element").build());
    actions.push_back(_script_node_builder<OScriptNodeArrayRemoveElement>("Types/Array", "Remove Element").build());
    actions.push_back(_script_node_builder<OScriptNodeArrayRemoveIndex>("Types/Array", "Remove Element by Index").build());
    actions.push_back(_script_node_builder<OScriptNodeMakeArray>("Types/Array", "Make Array").build());
    actions.push_back(_script_node_builder<OScriptNodeMakeDictionary>("Types/Dictionary", "Make Dictionary").build());
    actions.push_back(_script_node_builder<OScriptNodeDictionarySet>("Types/Dictionary", "Set").build());

    // Dialogue
    actions.push_back(_script_node_builder<OScriptNodeDialogueMessage>("Dialogue", "Show Message").executions(true).build());
    actions.push_back(_script_node_builder<OScriptNodeDialogueChoice>("Dialogue", "Show Message Choice").build());

    // Flow Control
    actions.push_back(_script_node_builder<OScriptNodeBranch>("Flow Control", "Branch").executions(true).build());
    actions.push_back(_script_node_builder<OScriptNodeChance>("Flow Control", "Chance").executions(true).build());
    actions.push_back(_script_node_builder<OScriptNodeDelay>("Flow Control", "Delay").executions(true).build());
    actions.push_back(_script_node_builder<OScriptNodeForEach>("Flow Control", "For Each", without_break).executions(true).build());
    actions.push_back(_script_node_builder<OScriptNodeForEach>("Flow Control", "For Each With Break", with_break).executions(true).build());
    actions.push_back(_script_node_builder<OScriptNodeForLoop>("Flow Control", "For Loop", without_break).executions(true).build());
    actions.push_back(_script_node_builder<OScriptNodeForLoop>("Flow Control", "For Loop With Break", with_break).executions(true).build());
    actions.push_back(_script_node_builder<OScriptNodeRandom>("Flow Control", "Random").executions(true).build());
    actions.push_back(_script_node_builder<OScriptNodeSelect>("Flow Control", "Select").executions(true).build());
    actions.push_back(_script_node_builder<OScriptNodeSequence>("Flow Control", "Sequence").executions(true).build());
    actions.push_back(_script_node_builder<OScriptNodeSwitch>("Flow Control", "Switch").executions(true).build());
    actions.push_back(_script_node_builder<OScriptNodeSwitchInteger>("Flow Control", "Switch on Integer").executions(true).build());
    actions.push_back(_script_node_builder<OScriptNodeSwitchString>("Flow Control", "Switch on String").executions(true).build());
    actions.push_back(_script_node_builder<OScriptNodeTypeCast>("Flow Control", "Type Cast").executions(true).build());
    actions.push_back(_script_node_builder<OScriptNodeWhile>("Flow Control", "While").executions(true).build());

    // Switch on Enums
    // todo: this isn't sorting properly
    for (const String& enum_name : ExtensionDB::get_global_enum_names())
    {
        const EnumInfo& info = ExtensionDB::get_global_enum(enum_name);
        actions.push_back(
            _script_node_builder<OScriptNodeSwitchEnum>(
                "Flow Control/Switch On",
                vformat("Switch on %s", info.name),
                DictionaryUtils::of({ { "enum" , info.name } }))
            .executions(true)
            .build());
    }

    // Function Helpers
    actions.push_back(
        _script_node_builder<OScriptNodeFunctionResult>("", "Add Return Node")
        .graph_type(GraphType::GRAPH_FUNCTION)
        .executions(true)
        .build());

    // Input
    actions.push_back(_script_node_builder<OScriptNodeInputAction>("Input", "Input Action").build());

    // Memory
    actions.push_back(_script_node_builder<OScriptNodeNew>("Memory", "New Object").build());
    actions.push_back(_script_node_builder<OScriptNodeFree>("Memory", "Free Object").build());

    // Resources
    actions.push_back(_script_node_builder<OScriptNodePreload>("Resource", "Preload Resource").build());
    actions.push_back(_script_node_builder<OScriptNodeResourcePath>("Resource", "Get Resource Path").build());

    // Scene
    actions.push_back(_script_node_builder<OScriptNodeInstantiateScene>("Scene", "Instantiate Scene").executions(true).build());
    actions.push_back(_script_node_builder<OScriptNodeSceneNode>("Scene", "Get Scene Node").build());
    actions.push_back(_script_node_builder<OScriptNodeSceneTree>("Scene", "Get Scene Tree").build());
    actions.push_back(_script_node_builder<OScriptNodeSelf>("Scene", "Get Self").build());

    // Signals
    actions.push_back(_script_node_builder<OScriptNodeAwaitSignal>("Signals", "Await Signal").executions(true).build());

    // Utilities
    actions.push_back(_script_node_builder<OScriptNodeComment>("Utilities", "Add Comment").build());
    actions.push_back(_script_node_builder<OScriptNodeAutoload>("Utilities", "Get an Autoload").build());
    actions.push_back(_script_node_builder<OScriptNodeEngineSingleton>("Utilities", "Get an Engine Singleton").build());
    actions.push_back(_script_node_builder<OScriptNodePrintString>("Utilities", "Print String").executions(true).build());

    // Variable Assignment
    const Dictionary local_object = DictionaryUtils::of({ { "type", Variant::OBJECT } });
    actions.push_back(_script_node_builder<OScriptNodeAssignLocalVariable>("Variables", "Assign Local").graph_type(GraphType::GRAPH_FUNCTION).build());
    actions.push_back(_script_node_builder<OScriptNodeAssignLocalVariable>("Utilities/Macros", "Assign Local").graph_type(GraphType::GRAPH_MACRO).build());
    actions.push_back(_script_node_builder<OScriptNodeLocalVariable>("Variables", "Local Object", local_object).graph_type(GraphType::GRAPH_FUNCTION).build());
    actions.push_back(_script_node_builder<OScriptNodeLocalVariable>("Utilities/Macros", "Local Object", local_object).graph_type(GraphType::GRAPH_MACRO).build());

    // List each engine singleton directly
    for (const String& name : Engine::get_singleton()->get_singleton_list())
    {
        const Dictionary data = DictionaryUtils::of({ { "singleton_name", name } });
        actions.push_back(_script_node_builder<OScriptNodeEngineSingleton>("Singleton", name, data).build());
    }

    // Orchestrator Script Language Functions
    const TypedArray<Dictionary> language_functions = OScriptLanguage::get_singleton()->_get_public_functions();
    for (int i = 0; i < language_functions.size(); i++)
    {
        const MethodInfo& mi = DictionaryUtils::to_method(language_functions[i]);
        // Exclude any internal methods that are prefixed with `_`.
        if (mi.name.begins_with("_")) {
            continue;
        }

        actions.push_back(_script_node_builder<OScriptNodeCallBuiltinFunction>("@OScript", mi.name, language_functions[i]).build());
    }

    return actions;
}

Vector<Ref<OrchestratorEditorIntrospector::Action>> OrchestratorEditorIntrospector::generate_actions_from_variant_types()
{
    Vector<Ref<Action>> actions;

    for (const BuiltInType& type : ExtensionDB::get_builtin_types())
    {
        // Nothing to show for NIL/Any
        if (type.type == Variant::NIL)
            continue;

        const String type_icon = _get_type_icon(type.type);
        const String type_name = _get_type_name(type.type);

        const String category = vformat("Types/%s", type_name);

        // Register top level category with icon for type
        actions.append_array(_create_categories_from_path(category, type_icon));

        const Dictionary type_dict = DictionaryUtils::of({ { "type", type.type } });

        // Local variables for macros
        actions.push_back(
            _script_node_builder<OScriptNodeLocalVariable>(
                category,
                vformat("Local %s Variable", type_name), type_dict)
            //.graph_type(GraphType::GRAPH_MACRO)
            .build());

        if (!type.properties.is_empty())
        {
            if (OScriptNodeCompose::is_supported(type.type))
            {
                actions.push_back(
                    _script_node_builder<OScriptNodeCompose>(
                        category,
                        vformat("Make %s", type_name),
                        type_dict)
                    .build());
            }

            actions.push_back(
                _script_node_builder<OScriptNodeDecompose>(
                    category,
                    vformat("Break %s", type_name),
                    type_dict)
                .build());
        }

        if (!type.constructors.is_empty())
        {
            for (const ConstructorInfo& info : type.constructors)
            {
                if (!info.arguments.is_empty())
                {
                    if (!OScriptNodeComposeFrom::is_supported(type.type, info.arguments))
                        continue;

                    Vector<String> argument_types;
                    Array arguments;
                    for (const PropertyInfo& argument : info.arguments)
                    {
                        String argument_name;
                        if (argument.name.to_lower().match("from"))
                            argument_name = VariantUtils::get_friendly_type_name(argument.type);
                        else
                            argument_name = argument.name.capitalize();

                        argument_types.push_back(argument_name);
                        arguments.push_back(DictionaryUtils::from_property(argument));
                    }

                    const String args = StringUtils::join(" and ", argument_types);
                    const Dictionary ctor_dict = DictionaryUtils::of(
                        { { "type", type.type }, { "constructor_args", arguments } });

                    actions.push_back(
                        _script_node_builder<OScriptNodeComposeFrom>(
                            category,
                            vformat("Make %s From %s", type_name, args),
                            ctor_dict)
                        .build());
                }
            }
        }

        for (const MethodInfo& method : type.get_method_list())
        {
            const Dictionary method_dict = DictionaryUtils::from_method(method);

            actions.push_back(
                _script_node_builder<OScriptNodeCallMemberFunction>(
                    category,
                    method.name,
                    DictionaryUtils::of({ { "target_type", type.type }, { "method", method_dict } }))
                .method(method)
                .target_class(Variant::get_type_name(type.type))
                .executions(true)
                .build());
        }

        if (OScriptNodeOperator::is_supported(type.type))
        {
            const String operator_category = vformat("%s/Operators", category);
            actions.append_array(_create_categories_from_path(operator_category));

            for (const OperatorInfo& info : type.operators)
            {
                if (!OScriptNodeOperator::is_operator_supported(info))
                    continue;

                String operator_name;
                if (!info.name.match("Not"))
                    operator_name = vformat("%s %s", type_name, info.name);
                else
                    operator_name = info.name;

                if (!info.right_type_name.is_empty())
                    operator_name += vformat(" %s", _get_type_name(info.right_type));

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
                if (info.op == VariantOperators::OP_MODULE)
                {
                    keywords.push_back("mod");
                    keywords.push_back("modulus");
                }

                actions.push_back(_script_node_builder<OScriptNodeOperator>(operator_category, operator_name, data)
                    .inputs(Vector({ info.left_type, info.right_type }))
                    .outputs(Vector({ info.return_type }))
                    .keywords(keywords)
                    .no_capitalize(true)
                    .build());
            }
        }

        if (type.index_returning_type != Variant::NIL && type.type >= Variant::ARRAY)
        {
            const String operator_category = vformat("%s/Operators", category);
            const Dictionary data = DictionaryUtils::of({ { "collection_type", type.type }, { "index_type", type.index_returning_type } });

            actions.push_back(_script_node_builder<OScriptNodeArrayGet>(operator_category, "Get At Index", data).build());
            actions.push_back(_script_node_builder<OScriptNodeArraySet>(operator_category, "Set At Index", data).build());
        }
    }

    return actions;
}

Vector<Ref<OrchestratorEditorIntrospector::Action>> OrchestratorEditorIntrospector::generate_actions_from_builtin_functions()
{
    Vector<Ref<Action>> actions;

    for (const FunctionInfo& info : ExtensionDB::get_utility_functions())
    {
        const MethodInfo& method = info.method;

        // Godot exports utility functions under "math", "random", and "general"
        // We remap "general" to "utilities" and "random" to "random numbers"
        const String category = _get_builtin_function_category_from_godot_category(info);
        actions.append_array(_create_categories_from_path(category));

        actions.push_back(
            _script_node_builder<OScriptNodeCallBuiltinFunction>(
                category,
                method.name,
                DictionaryUtils::from_method(method))
            .method(method)
            .tooltip(vformat("Calls the specified built-in Godot function '%s'.", method.name))
            .build());
    }

    return actions;
}

Vector<Ref<OrchestratorEditorIntrospector::Action>> OrchestratorEditorIntrospector::generate_actions_from_autoloads()
{
    Vector<Ref<Action>> actions;

    for (const KeyValue<StringName, GDE::ProjectSettings::AutoloadInfo>& E : GDE::ProjectSettings::get_autoload_list())
    {
        actions.push_back(
            _script_node_builder<OScriptNodeAutoload>(
                vformat("Project/Autoloads"),
                vformat("Get %s", E.key),
                DictionaryUtils::of({ { "class_name", E.key } }))
            .tooltip(vformat("Get a reference to the project autoload %s.", E.key))
            .no_capitalize(true)
            .build());
    }

    return actions;
}

Vector<Ref<OrchestratorEditorIntrospector::Action>> OrchestratorEditorIntrospector::generate_actions_from_native_classes()
{
    Vector<Ref<Action>> actions;

    for (const String& class_name : ClassDB::get_class_list())
        actions.append_array(generate_actions_from_class(class_name));

    return actions;
}

Vector<Ref<OrchestratorEditorIntrospector::Action>> OrchestratorEditorIntrospector::generate_actions_from_static_script_methods()
{
    Vector<Ref<Action>> actions;

    for (const String& global_class : ScriptServer::get_global_class_list())
    {
        const String category = vformat("Static/%s", global_class);

        const TypedArray<Dictionary> static_methods = ScriptServer::get_global_class(global_class).get_static_method_list();
        for (int i = 0; i < static_methods.size(); i++)
        {
            const MethodInfo static_method = DictionaryUtils::to_method(static_methods[i]);
            actions.push_back(
                _script_node_builder<OScriptNodeCallStaticFunction>(
                    category,
                    static_method.name,
                    DictionaryUtils::of({ { "class_name", global_class }, { "method_name", static_method.name } }))
                .executions(true)
                .build());
        }
    }

    return actions;
}

Vector<Ref<OrchestratorEditorIntrospector::Action>> OrchestratorEditorIntrospector::generate_actions_from_script_global_classes()
{
    Vector<Ref<Action>> actions;

    for (const String& global_name : ScriptServer::get_global_class_list())
    {
        // The object has a named script attached
        // The script methods, properties, and signals must be registered using the script's class_name
        // rather than adding these as part of the base script type.
        const PackedStringArray class_hierarchy = ScriptServer::get_class_hierarchy(global_name, false);
        for (const String& class_name : class_hierarchy)
        {
            const ScriptServer::GlobalClass global_class = ScriptServer::get_global_class(class_name);
            actions.append_array(
                _get_actions_for_class(
                    global_class.name,
                    global_class.name,
                    global_class.get_method_list(),
                    global_class.get_property_list(),
                    global_class.get_signal_list()));
        }

        const String static_category_name = vformat("Static/%s", global_name);

        const TypedArray<Dictionary> static_methods = ScriptServer::get_global_class(global_name).get_static_method_list();
        for (int i = 0; i < static_methods.size(); i++)
        {
            const MethodInfo static_method = DictionaryUtils::to_method(static_methods[i]);
            actions.push_back(
                _script_node_builder<OScriptNodeCallStaticFunction>(
                    static_category_name,
                    static_method.name,
                    DictionaryUtils::of({ { "class_name", global_name }, { "method_name", static_method.name } }))
                .executions(true)
                .build());
        }
    }

    return actions;
}

Vector<Ref<OrchestratorEditorIntrospector::Action>> OrchestratorEditorIntrospector::generate_actions_from_category(
    const String& p_category, const String& p_icon)
{
    return _create_categories_from_path(p_category, p_icon);
}

void OrchestratorEditorIntrospector::free_resources()
{
    _script_node_cache.clear();
}

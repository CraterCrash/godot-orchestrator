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
#include "default_action_registrar.h"

#include "api/extension_db.h"
#include "common/dictionary_utils.h"
#include "common/variant_utils.h"
#include "common/string_utils.h"
#include "editor/graph/graph_edit.h"
#include "editor/graph/graph_node_spawner.h"
#include "script/nodes/script_nodes.h"

void OrchestratorDefaultGraphActionRegistrar::_register_node(const OrchestratorGraphActionRegistrarContext& p_context,
                                                       const StringName& p_class_name, const StringName& p_category,
                                                       const Dictionary& p_data)
{
    OScriptLanguage* language = OScriptLanguage::get_singleton();
    Ref<OScriptNode> node = language->create_node_from_name(p_class_name, p_context.script, false);
    if (!node->get_flags().has_flag(OScriptNode::ScriptNodeFlags::CATALOGABLE))
        return;

    PackedStringArray name_parts = p_category.split("/");

    OrchestratorGraphActionSpec spec;
    spec.category = "Script/Nodes/" + p_category;
    spec.tooltip = node->get_tooltip_text();
    spec.text = name_parts[int(name_parts.size()) - 1].capitalize();
    spec.keywords = StringUtils::join(",", name_parts);
    spec.icon = node->get_icon();
    spec.type_icon = "PluginScript";
    spec.graph_compatible = node->is_compatible_with_graph(p_context.graph->get_owning_graph());

    // Initialize the node based on the basic data so that filtration can resolve pin types
    OScriptNodeInitContext context;
    context.user_data = p_data;
    node->initialize(context);

    Ref<OrchestratorGraphActionHandler> handler(memnew(OrchestratorGraphNodeSpawnerScriptNode(p_class_name, p_data, node)));
    p_context.list->push_back(memnew(OrchestratorGraphActionMenuItem(spec, handler)));
}

PackedStringArray OrchestratorDefaultGraphActionRegistrar::_get_class_hierarchy(const String& p_derived_class_name)
{
    PackedStringArray classes;
    StringName class_name = p_derived_class_name;
    while (!class_name.is_empty() && ClassDB::class_exists(class_name))
    {
        classes.push_back(class_name);
        class_name = ClassDB::get_parent_class(class_name);
    }
    return classes;
}

String OrchestratorDefaultGraphActionRegistrar::_get_method_signature(const MethodInfo& p_method)
{
    String signature = p_method.name.replace("_", " ").capitalize();
    if (!p_method.arguments.empty())
    {
        signature += " (";
        bool first = true;
        int index = 0;
        for (const PropertyInfo& property : p_method.arguments)
        {
            if (!first)
                signature += ", ";

            if (property.name.is_empty())
                signature += "p" + itos(index++);
            else
                signature += property.name;

            if (property.type == Variant::NIL)
                signature += ":Any";
            else
                signature += ":" + Variant::get_type_name(property.type);

            first = false;
        }
        signature += ")";
    }
    return signature;
}

String OrchestratorDefaultGraphActionRegistrar::_get_method_icon(const MethodInfo& p_method)
{
    if (!OScriptNodeEvent::is_event_method(p_method))
    {
        if (p_method.return_val.type != Variant::NIL)
        {
            // Method has a return type
            String return_type = Variant::get_type_name(p_method.return_val.type);
            if (!return_type.is_empty())
                return return_type;
        }
        else if (p_method.name.capitalize().begins_with("Set ") && p_method.arguments.size() == 1)
        {
            // Method is a setter, base icon on the argument type
            String arg_type = Variant::get_type_name(p_method.arguments[0].type);
            if (!arg_type.is_empty())
                return arg_type;
        }
    }
    return "MemberMethod";
}

String OrchestratorDefaultGraphActionRegistrar::_get_method_type_icon(const MethodInfo& p_method)
{
    bool event_method = OScriptNodeEvent::is_event_method(p_method);
    if (!event_method && p_method.flags & METHOD_FLAG_VIRTUAL)
        return "MethodOverride";
    else if (event_method)
        return "MemberSignal";
    else
        return "MemberMethod";
}

void OrchestratorDefaultGraphActionRegistrar::_register_script_nodes(const OrchestratorGraphActionRegistrarContext& p_context)
{
    bool graph_function = false;
    if (p_context.graph)
        graph_function = p_context.graph->is_function();

    // Groups
    const String func_or_macro_group = graph_function ? "Variables" : "Utilities/Macro";

    // Data toggles
    const Dictionary with_break = DictionaryUtils::of({ { "with_break", true } });
    const Dictionary without_break = DictionaryUtils::of({ { "with_break", false } });
    const Dictionary array_data = DictionaryUtils::of({ { "collection_type", Variant::ARRAY },{ "index_type", Variant::NIL } });

    // Comments
    _register_node<OScriptNodeComment>(p_context, "add_comment");

    // Constants
    _register_node<OScriptNodeGlobalConstant>(p_context, "Constant/global_constant");
    _register_node<OScriptNodeMathConstant>(p_context, "Constant/math_constant");
    _register_node<OScriptNodeTypeConstant>(p_context, "Constant/type_constant");
    _register_node<OScriptNodeClassConstant>(p_context, "Constant/class_constant");
    _register_node<OScriptNodeSingletonConstant>(p_context, "Constant/singleton_constant");

    // Data
    _register_node<OScriptNodeArrayGet>(p_context, "Operators/Array/get_at_index", array_data);
    _register_node<OScriptNodeArraySet>(p_context, "Operators/Array/set_at_index", array_data);
    _register_node<OScriptNodeArrayFind>(p_context, "Array/find_array_element");
    _register_node<OScriptNodeArrayClear>(p_context, "Array/clear_array");
    _register_node<OScriptNodeArrayAppend>(p_context, "Array/append_arrays");
    _register_node<OScriptNodeArrayAddElement>(p_context, "Array/add_element");
    _register_node<OScriptNodeArrayRemoveElement>(p_context, "Array/remove_element");
    _register_node<OScriptNodeArrayRemoveIndex>(p_context, "Array/remove_element_by_index");
    _register_node<OScriptNodeMakeArray>(p_context, "Array/make_array");
    _register_node<OScriptNodeMakeDictionary>(p_context, "Dictionary/make_dictionary");

    // Dialogue
    _register_node<OScriptNodeDialogueChoice>(p_context, "Dialogue/choice");
    _register_node<OScriptNodeDialogueMessage>(p_context, "Dialogue/show_message");

    // Flow Control
    _register_node<OScriptNodeBranch>(p_context, "Flow Control/branch");
    _register_node<OScriptNodeChance>(p_context, "Flow Control/chance");
    _register_node<OScriptNodeDelay>(p_context, "Flow Control/delay");
    _register_node<OScriptNodeForEach>(p_context, "Flow Control/for_each", without_break);
    _register_node<OScriptNodeForEach>(p_context, "Flow Control/for_each_with_break", with_break);
    _register_node<OScriptNodeForLoop>(p_context, "Flow Control/for", without_break);
    _register_node<OScriptNodeForLoop>(p_context, "Flow Control/for_with_break", with_break);
    _register_node<OScriptNodeRandom>(p_context, "Flow Control/random");
    _register_node<OScriptNodeSelect>(p_context, "Flow Control/select");
    _register_node<OScriptNodeSequence>(p_context, "Flow Control/sequence");
    _register_node<OScriptNodeSwitch>(p_context, "Flow Control/switch");
    _register_node<OScriptNodeSwitchInteger>(p_context, "Flow Control/switch_on_integer");
    _register_node<OScriptNodeSwitchString>(p_context, "Flow Control/switch_on_string");
    _register_node<OScriptNodeTypeCast>(p_context, "Flow Control/type_cast");
    _register_node<OScriptNodeWhile>(p_context, "Flow Control/while");

    // Switch on Enums
    for (const String& enum_name : ExtensionDB::get_global_enum_names())
    {
        const EnumInfo ei = ExtensionDB::get_global_enum(enum_name);
        const String category = vformat("Flow Control/Switch On/switch_on_%s", ei.name);
        _register_node<OScriptNodeSwitchEnum>(p_context, category, DictionaryUtils::of({ { "enum" , ei.name } }));
    }

    // Functions
    _register_node<OScriptNodeFunctionResult>(p_context, "Function/add_return_node");

    // Input
    _register_node<OScriptNodeInputAction>(p_context, "Input/input_action");

    // Resource
    _register_node<OScriptNodePreload>(p_context, "Resource/preload_resource");
    _register_node<OScriptNodeResourcePath>(p_context, "Resource/get_resource_path");

    // Scene
    _register_node<OScriptNodeInstantiateScene>(p_context, "Scene/instantiate_scene");
    _register_node<OScriptNodeSceneNode>(p_context, "Scene/get_scene_node");
    _register_node<OScriptNodeSceneTree>(p_context, "Scene/get_scene_tree");

    // Signals
    _register_node<OScriptNodeAwaitSignal>(p_context, "Signals/Await Signal");

    // Utilities
    _register_node<OScriptNodeAutoload>(p_context, "Utilities/get_autoload");
    _register_node<OScriptNodeEngineSingleton>(p_context, "Utilities/engine_singleton");
    _register_node<OScriptNodePrintString>(p_context, "Utilities/print_string");

    // Variables
    _register_node<OScriptNodeSelf>(p_context, "Variables/get_self");

    // Register variable assignment differently for macros
    const String local_var_category = vformat("%s/Assign", func_or_macro_group);
    _register_node<OScriptNodeAssignLocalVariable>(p_context, local_var_category);

    // Register Local Object variables
    const String lv_object_name = vformat("%s/local_object", func_or_macro_group);
    const Dictionary object_type_dict = DictionaryUtils::of({ { "type", Variant::OBJECT } });
    _register_node<OScriptNodeLocalVariable>(p_context, lv_object_name, object_type_dict);

    // Builtin Types
    for (const String& type_name : ExtensionDB::get_builtin_type_names())
    {
        const BuiltInType type = ExtensionDB::get_builtin_type(type_name);
        const String friendly_name = VariantUtils::get_friendly_type_name(type.type, true);

        const Dictionary type_dict = DictionaryUtils::of({ { "type", type.type } });

        // Register local variables differently for macros
        const String lv_name = vformat("%s/local_%s", func_or_macro_group, friendly_name);
        _register_node<OScriptNodeLocalVariable>(p_context, lv_name, type_dict);

        if (!type.properties.is_empty())
        {
            if (OScriptNodeCompose::is_supported(type.type))
            {
                const String make_category = vformat("%s/make_%s", friendly_name, friendly_name.to_lower());
                _register_node<OScriptNodeCompose>(p_context, make_category, type_dict);
            }

            const String break_category = vformat("%s/break_%s", friendly_name, friendly_name.to_lower());
            _register_node<OScriptNodeDecompose>(p_context, break_category, type_dict);
        }

        if (!type.constructors.is_empty())
        {
            for (const ConstructorInfo& ci : type.constructors)
            {
                if (!ci.arguments.is_empty())
                {
                    if (!OScriptNodeComposeFrom::is_supported(type.type, ci.arguments))
                        continue;

                    Vector<String> type_names;
                    Array properties;
                    for (const PropertyInfo& pi : ci.arguments)
                    {
                        String name;
                        if (pi.name.to_lower().match("from"))
                            name = VariantUtils::get_friendly_type_name(pi.type);
                        else
                            name = pi.name.capitalize();

                        type_names.push_back(name);
                        properties.push_back(DictionaryUtils::from_property(pi));
                    }

                    const Dictionary ctor_dict = DictionaryUtils::of(
                        { { "type", type.type }, { "constructor_args", properties } });

                    const String args = StringUtils::join(" and ", type_names);
                    const String category = vformat("%s/make_%s_from_%s", type.name, friendly_name.to_lower(), args);

                    _register_node<OScriptNodeComposeFrom>(p_context, category, ctor_dict);
                }
            }
        }

        for (const MethodInfo& mi : type.methods)
        {
            const String category = vformat("%s/%s", type.name, mi.name);
            const Dictionary method_dict = DictionaryUtils::from_method(mi);
            const Dictionary data = DictionaryUtils::of({ { "target_type", type.type }, { "method", method_dict } });
            _register_node<OScriptNodeCallMemberFunction>(p_context, category, data);
        }

        if (OScriptNodeOperator::is_supported(type.type))
        {
            for (const OperatorInfo& op : type.operators)
            {
                if (!OScriptNodeOperator::is_operator_supported(op))
                    continue;

                // todo: move these to new package once tested
                String category = vformat("Operators/%s/%s", friendly_name, op.name);
                if (!op.right_type_name.is_empty())
                    category += vformat("_(_%s_)", op.right_type_name);

                const Dictionary data = DictionaryUtils::of({ { "op", op.op },
                                                              { "code", op.code },
                                                              { "name", op.name },
                                                              { "type", type.type },
                                                              { "left_type", op.left_type },
                                                              { "left_type_name", op.left_type_name },
                                                              { "right_type", op.right_type },
                                                              { "right_type_name", op.right_type_name },
                                                              { "return_type", op.return_type } });

                _register_node<OScriptNodeOperator>(p_context, category, data);
            }
        }

        if (type.index_returning_type != Variant::NIL && type.type >= Variant::ARRAY)
        {
            const String get_category = vformat("Operators/%s/%s", type.name, "Get At Index");
            const String set_category = vformat("Operators/%s/%s", type.name, "Set At Index");

            const Dictionary data = DictionaryUtils::of({
                { "collection_type", type.type },
                { "index_type", type.index_returning_type } });

            _register_node<OScriptNodeArrayGet>(p_context, get_category, data);
            _register_node<OScriptNodeArraySet>(p_context, set_category, data);
        }
    }

    // Builtin Functions
    for (const String& function_name : ExtensionDB::get_function_names())
    {
        const FunctionInfo& fi = ExtensionDB::get_function(function_name);

        MethodInfo mi;
        mi.name = fi.name;
        mi.return_val = fi.return_val;

        for (const PropertyInfo& argument : fi.arguments)
            mi.arguments.push_back(argument);

        // Godot exports utility functions under "math", "random", and "general"
        // Remap "general" to "utilities"
        String top_category = fi.category;
        if (top_category.match("general"))
            top_category = "utilities";

        const String category = vformat("%s/%s", top_category.capitalize(), fi.name);
        _register_node<OScriptNodeCallBuiltinFunction>(p_context, category, DictionaryUtils::from_method(mi));
    }
}

void OrchestratorDefaultGraphActionRegistrar::_register_graph_items(const OrchestratorGraphActionRegistrarContext& p_context)
{
    if (!p_context.graph)
        return;

    PackedStringArray classes;
    if (p_context.filter->target_object)
    {
        classes.push_back(p_context.filter->target_object->get_class());
    }
    else if (!p_context.filter->target_classes.is_empty())
    {
        for (String target_class : p_context.filter->target_classes)
        {
            PackedStringArray target_hierarchy = _get_class_hierarchy(target_class);
            for (const String& target : target_hierarchy)
                classes.push_back(target);
        }
    }
    else
    {
        classes = _get_class_hierarchy(p_context.graph->get_owning_script()->get_base_type());
    }

    for (const String& class_name : classes)
    {
        const OrchestratorGraphActionSpec spec(class_name, class_name, class_name);
        p_context.list->push_back(memnew(OrchestratorGraphActionMenuItem(spec)));

        // todo: how to deal with "get property" and "call get property" between properties and methods?
        _register_class_properties(p_context, class_name);
        _register_class_methods(p_context, class_name);
        _register_class_signals(p_context, class_name);
    }
}

void OrchestratorDefaultGraphActionRegistrar::_register_class_properties(const OrchestratorGraphActionRegistrarContext& p_context,
                                                                   const StringName& p_class_name)
{
    TypedArray<Dictionary> properties;
    if (p_context.filter->target_object)
        properties = p_context.filter->target_object->get_property_list();
    else
        properties = ClassDB::class_get_property_list(p_class_name, true);

    for (int i = 0; i < properties.size(); i++)
    {
        const PropertyInfo pi = DictionaryUtils::to_property(properties[i]);

        OrchestratorGraphActionSpec getter_spec;
        // todo: remove "class/properties/"
        getter_spec.category = vformat("Class/Properties/%s/get_%s", p_class_name, pi.name);
        getter_spec.tooltip = vformat("Return the value from the property '%s'", pi.name);
        getter_spec.text = vformat("get_%s", pi.name).capitalize();
        getter_spec.keywords = vformat("get,%s,%s", p_class_name, pi.name);
        getter_spec.icon = Variant::get_type_name(pi.type);
        getter_spec.type_icon = "MemberProperty";

        Ref<OrchestratorGraphActionHandler> getter_handler(memnew(OrchestratorGraphNodeSpawnerPropertyGet(pi, p_context.filter->target_classes)));
        p_context.list->push_back(memnew(OrchestratorGraphActionMenuItem(getter_spec, getter_handler)));

        OrchestratorGraphActionSpec setter_spec;
        // todo: remove "class/properties/"
        setter_spec.category = vformat("Class/Properties/%s/set_%s", p_class_name, pi.name);
        setter_spec.tooltip = vformat("Set the value of property '%s'", pi.name);
        setter_spec.text = vformat("set_%s", pi.name).capitalize();
        setter_spec.keywords = vformat("set,%s,%s", p_class_name, pi.name);
        setter_spec.icon = Variant::get_type_name(pi.type);
        setter_spec.type_icon = "MemberProperty";

        Ref<OrchestratorGraphActionHandler> setter_handler(memnew(OrchestratorGraphNodeSpawnerPropertySet(pi, p_context.filter->target_classes)));
        p_context.list->push_back(memnew(OrchestratorGraphActionMenuItem(setter_spec, setter_handler)));
    }
}

void OrchestratorDefaultGraphActionRegistrar::_register_class_methods(const OrchestratorGraphActionRegistrarContext& p_context,
                                                                const StringName& p_class_name)
{
    TypedArray<Dictionary> methods;
    if (p_context.filter->target_object)
        methods = p_context.filter->target_object->get_method_list();
    else
        methods = ClassDB::class_get_method_list(p_class_name, true);

    for (int i = 0; i < methods.size(); i++)
    {
        const MethodInfo mi = DictionaryUtils::to_method(methods[i]);

        OrchestratorGraphActionSpec spec;
        // todo: remove "class/methods/"
        spec.category = vformat("Class/Methods/%s/%s", p_class_name, mi.name);
        spec.tooltip = _get_method_signature(mi);
        spec.text = vformat("call_%s", mi.name).capitalize();
        spec.keywords = vformat("call,%s,%s", p_class_name, mi.name);
        spec.icon = _get_method_icon(mi);
        spec.type_icon = _get_method_type_icon(mi);

        OrchestratorGraphActionHandler* handler_ptr;
        if (OScriptNodeEvent::is_event_method(mi))
            handler_ptr = memnew(OrchestratorGraphNodeSpawnerEvent(mi));
        else
            handler_ptr = memnew(OrchestratorGraphNodeSpawnerCallMemberFunction(mi));

        Ref<OrchestratorGraphActionHandler> handler(handler_ptr);
        p_context.list->push_back(memnew(OrchestratorGraphActionMenuItem(spec, handler)));
    }
}

void OrchestratorDefaultGraphActionRegistrar::_register_class_signals(const OrchestratorGraphActionRegistrarContext& p_context,
                                                                const StringName& p_class_name)
{
    TypedArray<Dictionary> signals;
    if (p_context.filter->target_object)
        signals = p_context.filter->target_object->get_signal_list();
    else
        signals = ClassDB::class_get_signal_list(p_class_name, true);

    for (int i = 0; i < signals.size(); i++)
    {
        const MethodInfo si = DictionaryUtils::to_method(signals[i]);
        OrchestratorGraphActionSpec spec;
        // todo: remove "class/signals/"
        spec.category = vformat("Class/Signals/%s/%s", p_class_name, si.name);
        spec.tooltip = "Emit the signal " + si.name;
        spec.text = vformat("emit_%s", si.name).capitalize();
        spec.keywords = vformat("emit,signal,%s,%s", p_class_name, si.name);
        spec.icon = "Signal";
        spec.type_icon = "MemberSignal";

        MethodInfo mi;
        mi.name = si.name;
        mi.arguments = si.arguments;
        mi.return_val = PropertyInfo(Variant::NIL, "");

        Ref<OrchestratorGraphActionHandler> handler(memnew(OrchestratorGraphNodeSpawnerEmitSignal(mi)));
        p_context.list->push_back(memnew(OrchestratorGraphActionMenuItem(spec, handler)));
    }
}

void OrchestratorDefaultGraphActionRegistrar::_register_script_functions(const OrchestratorGraphActionRegistrarContext& p_context)
{
    OrchestratorGraphActionSpec call_function_spec;
    call_function_spec.category = "call_function";
    call_function_spec.tooltip = "Call Functions defined within the orchestration.";
    call_function_spec.text = "Call Function";
    call_function_spec.keywords = "call,function";
    call_function_spec.icon = "MemberMethod";
    call_function_spec.type_icon = "MemberMethod";
    p_context.list->push_back(memnew(OrchestratorGraphActionMenuItem(call_function_spec)));

    if (OrchestratorGraphEdit* graph = p_context.graph)
    {
        PackedStringArray function_names = graph->get_owning_script()->get_function_names();
        for (const String& function_name : function_names)
        {
            Ref<OScriptFunction> function = graph->get_owning_script()->find_function(StringName(function_name));
            if (!function.is_valid() || !function->is_user_defined())
                continue;

            const MethodInfo& mi = function->get_method_info();

            OrchestratorGraphActionSpec spec;
            // todo: remove "script/"
            spec.category = vformat("Script/Call Function/%s", mi.name);
            spec.tooltip = _get_method_signature(mi);
            spec.text = vformat("Call %s", mi.name);
            spec.keywords = vformat("call,%s,%s", graph->get_owning_script()->get_base_type(), mi.name);
            spec.icon = _get_method_icon(mi);
            spec.type_icon = _get_method_type_icon(mi);

            Ref<OrchestratorGraphActionHandler> handler(memnew(OrchestratorGraphNodeSpawnerCallScriptFunction(mi)));
            p_context.list->push_back(memnew(OrchestratorGraphActionMenuItem(spec, handler)));
        }
    }
}

void OrchestratorDefaultGraphActionRegistrar::_register_script_variables(const OrchestratorGraphActionRegistrarContext& p_context)
{
    OrchestratorGraphActionSpec variables_spec;
    variables_spec.category = "variables";
    variables_spec.tooltip = "Variables defined within the orchestration.";
    variables_spec.text = "Variables";
    variables_spec.keywords = "variable,variables";
    variables_spec.icon = "MemberProperty";
    variables_spec.type_icon = "MemberProperty";
    p_context.list->push_back(memnew(OrchestratorGraphActionMenuItem(variables_spec)));

    if (OrchestratorGraphEdit* graph = p_context.graph)
    {
        for (const Ref<OScriptVariable>& variable : graph->get_owning_script()->get_variables())
        {
            if (!variable.is_valid())
                continue;

            const String& variable_name = variable->get_variable_name();

            OrchestratorGraphActionSpec getter_spec;
            // todo: remove "script/"
            getter_spec.category = vformat("Script/Variables/get_%s", variable_name);
            getter_spec.tooltip = vformat("Get the value of the variable '%s'", variable_name);
            getter_spec.text = vformat("Get %s", variable_name);
            getter_spec.keywords = vformat("get,variable,%s", variable_name);
            getter_spec.icon = Variant::get_type_name(variable->get_variable_type());
            getter_spec.type_icon = "MemberProperty";

            Ref<OrchestratorGraphActionHandler> getter_handler(memnew(OrchestratorGraphNodeSpawnerVariableGet(variable_name)));
            p_context.list->push_back(memnew(OrchestratorGraphActionMenuItem(getter_spec, getter_handler)));

            OrchestratorGraphActionSpec setter_spec;
            // todo: remove "script/"
            setter_spec.category = vformat("Script/Variables/set_%s", variable_name);
            setter_spec.tooltip = vformat("Set the value of of variable '%s'", variable_name);
            setter_spec.text = vformat("Set %s", variable_name);
            setter_spec.keywords = vformat("set,variable,%s", variable_name);
            setter_spec.icon = Variant::get_type_name(variable->get_variable_type());
            setter_spec.type_icon = "MemberProperty";

            Ref<OrchestratorGraphActionHandler> setter_handler(memnew(OrchestratorGraphNodeSpawnerVariableSet(variable_name)));
            p_context.list->push_back(memnew(OrchestratorGraphActionMenuItem(setter_spec, setter_handler)));
        }
    }
}

void OrchestratorDefaultGraphActionRegistrar::_register_script_signals(const OrchestratorGraphActionRegistrarContext& p_context)
{
    OrchestratorGraphActionSpec signals_spec;
    signals_spec.category = "emit_signals";
    signals_spec.tooltip = "Signals defined within the orchestration.";
    signals_spec.text = "emit_signals";
    signals_spec.keywords = "signal,signals";
    signals_spec.icon = "MemberSignal";
    signals_spec.type_icon = "MemberSignal";
    p_context.list->push_back(memnew(OrchestratorGraphActionMenuItem(signals_spec)));

    if (OrchestratorGraphEdit* graph = p_context.graph)
    {
        PackedStringArray signal_names = graph->get_owning_script()->get_custom_signal_names();
        for (const String& signal_name : signal_names)
        {
            Ref<OScriptSignal> signal = graph->get_owning_script()->get_custom_signal(signal_name);
            if (!signal.is_valid())
                continue;

            OrchestratorGraphActionSpec spec;
            // todo: remove "script/"
            spec.category = vformat("Script/Emit Signals/emit_%s", signal_name);
            spec.tooltip = vformat("Emit signal '%s'", signal_name);
            spec.text = vformat("Emit %s", signal_name);
            spec.keywords = vformat("emit,signal,%s,%s", graph->get_owning_script()->get_base_type(), signal_name);
            spec.icon = "MemberSignal";
            spec.type_icon = "MemberSignal";

            Ref<OrchestratorGraphActionHandler> handler(memnew(OrchestratorGraphNodeSpawnerEmitSignal(signal->get_method_info())));
            p_context.list->push_back(memnew(OrchestratorGraphActionMenuItem(spec, handler)));
        }
    }
}

void OrchestratorDefaultGraphActionRegistrar::register_actions(const OrchestratorGraphActionRegistrarContext& p_context)
{
    _register_graph_items(p_context);
    _register_script_nodes(p_context);
    _register_script_functions(p_context);
    _register_script_variables(p_context);
    _register_script_signals(p_context);
}
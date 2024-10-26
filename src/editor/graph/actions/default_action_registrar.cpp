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
#include "default_action_registrar.h"

#include "api/extension_db.h"
#include "common/dictionary_utils.h"
#include "common/method_utils.h"
#include "common/property_utils.h"
#include "common/settings.h"
#include "common/string_utils.h"
#include "common/variant_utils.h"
#include "editor/graph/graph_edit.h"
#include "editor/graph/graph_node_spawner.h"
#include "godot_cpp/templates/hash_set.hpp"
#include "script/nodes/script_nodes.h"
#include "script/script_server.h"

#include <godot_cpp/classes/engine.hpp>

void OrchestratorDefaultGraphActionRegistrar::_register_node(const StringName& p_class_name, const StringName& p_category, const Dictionary& p_data)
{
    Orchestration* orchestration = _context->graph->get_orchestration();

    const Ref<OScriptNode> node = OScriptNodeFactory::create_node_from_name(p_class_name, orchestration);
    if (!node.is_valid() || !node->get_flags().has_flag(OScriptNode::ScriptNodeFlags::CATALOGABLE))
        return;

    PackedStringArray name_parts = p_category.split("/");

    PackedStringArray keywords = node->get_keywords();
    keywords.append_array(name_parts);

    OrchestratorGraphActionSpec spec;
    spec.category = p_category;
    spec.tooltip = node->get_tooltip_text();
    spec.text = name_parts[int(name_parts.size()) - 1].capitalize();
    spec.keywords = StringUtils::join(",", keywords);
    spec.icon = node->get_icon();
    spec.type_icon = "PluginScript";
    spec.graph_compatible = node->is_compatible_with_graph(_context->graph->get_owning_graph());

    const Ref<OScriptNodeCallStaticFunction> call_static_function = node;
    if (call_static_function.is_valid())
        spec.qualifiers += "static";

    // Initialize the node based on the basic data so that filtration can resolve pin types
    OScriptNodeInitContext context;
    context.user_data = p_data;
    node->initialize(context);

    Ref<OrchestratorGraphActionHandler> handler(memnew(OrchestratorGraphNodeSpawnerScriptNode(p_class_name, p_data, node)));
    _context->list->push_back(memnew(OrchestratorGraphActionMenuItem(spec, handler)));
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

String OrchestratorDefaultGraphActionRegistrar::_get_method_icon(const MethodInfo& p_method)
{
    if (!OScriptNodeEvent::is_event_method(p_method))
    {
        if (MethodUtils::has_return_value(p_method))
        {
            // Method has a return type
            String return_type = PropertyUtils::get_property_type_name(p_method.return_val);
            if (!return_type.is_empty())
                return return_type;
        }
        else if (p_method.name.capitalize().begins_with("Set ") && p_method.arguments.size() == 1)
        {
            // Method is a setter, base icon on the argument type
            String arg_type = PropertyUtils::get_property_type_name(p_method.arguments[0]);
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

String OrchestratorDefaultGraphActionRegistrar::_get_builtin_type_icon_name(Variant::Type p_type) const
{
    if (p_type == Variant::NIL)
        return "Variant";

    return Variant::get_type_name(p_type);
}

String OrchestratorDefaultGraphActionRegistrar::_get_builtin_type_display_name(Variant::Type p_type) const
{
    switch (p_type)
    {
        case Variant::NIL:
            return "Any";
        case Variant::BOOL:
            return "Boolean";
        case Variant::INT:
            return "Integer";
        case Variant::FLOAT:
            return "Float";
        default:
            return Variant::get_type_name(p_type).replace(" ", "");
    }
}

OrchestratorGraphActionSpec OrchestratorDefaultGraphActionRegistrar::_get_method_spec(const MethodInfo& p_method, const String& p_base_type, const String& p_class_name)
{
    OrchestratorGraphActionSpec spec;
    if (p_class_name.is_empty())
    {
        spec.category = vformat("Call Function/%s", p_method.name);
        spec.text = vformat("Call %s", _friendly_method_names ? p_method.name.capitalize() : String(p_method.name));
    }
    else
    {
        spec.category = vformat("Methods/%s/%s", p_class_name, p_method.name);
        spec.text = vformat("%s", p_method.name).capitalize();
    }

    spec.tooltip = MethodUtils::get_signature(p_method);
    spec.keywords = vformat("%s,%s", StringUtils::default_if_empty(p_class_name, p_base_type), p_method.name);
    spec.icon = _get_method_icon(p_method);
    spec.type_icon = _get_method_type_icon(p_method);

    return spec;
}

OrchestratorGraphActionSpec OrchestratorDefaultGraphActionRegistrar::_get_signal_spec(const StringName& p_signal_name, const String& p_base_type, const String& p_class_name)
{
    OrchestratorGraphActionSpec spec;
    if (p_class_name.is_empty())
        spec.category = vformat("Signals/emit_%s", p_signal_name);
    else
        spec.category = vformat("Signals/%s/%s", p_class_name, p_signal_name);

    spec.tooltip = "Emit the signal " + p_signal_name;
    spec.text = vformat("emit_%s", p_signal_name).capitalize();
    if (p_class_name.is_empty())
        spec.keywords = vformat("emit,signal,%s,%s", p_base_type, p_signal_name);
    else
        spec.keywords = vformat("emit,signal,%s,%s", p_signal_name, p_class_name);

    spec.icon = p_class_name.is_empty() ? "MemberSignal" : "Signal";
    spec.type_icon = "MemberSignal";

    return spec;
}

void OrchestratorDefaultGraphActionRegistrar::_register_category(const String& p_category, const String& p_display_name, const String& p_icon)
{
    OrchestratorGraphActionSpec spec;
    spec.category = p_category;
    spec.text = p_display_name;
    spec.icon = p_icon;
    _context->list->push_back(memnew(OrchestratorGraphActionMenuItem(spec)));
}

void OrchestratorDefaultGraphActionRegistrar::_register_orchestration_nodes()
{
    bool graph_function = false;
    if (_context->graph)
        graph_function = _context->graph->is_function();

    // Groups
    const String func_or_macro_group = graph_function ? "Variables" : "Utilities/Macro";

    // Data toggles
    const Dictionary with_break = DictionaryUtils::of({ { "with_break", true } });
    const Dictionary without_break = DictionaryUtils::of({ { "with_break", false } });
    const Dictionary array_data = DictionaryUtils::of({ { "collection_type", Variant::ARRAY },{ "index_type", Variant::NIL } });

    // Register several top-level categories
    _register_category("Project", "Project", "Godot");
    _register_category("Call Function", "Call Function", "ScriptExtend");
    _register_category("Constants", "Constants", "MemberConstant");
    _register_category("Dialogue", "Dialogue", "Window");
    _register_category("Flow Control", "Flow Control", "FileTree");
    _register_category("Input", "Input", "InputEventKey");
    _register_category("Math", "Math", "X509Certificate");
    _register_category("Memory", "Memory", "MiniObject");
    _register_category("Methods", "Methods", "MemberMethod");
    _register_category("Properties", "Properties", "MemberProperty");
    _register_category("Random Numbers", "Random Numbers", "RandomNumberGenerator");
    _register_category("Resource", "Resource", "File");
    _register_category("Scene", "Scene", "PackedScene");
    _register_category("Singletons", "Singletons", "MiniObject");
    _register_category("Static", "Static", "AudioBusSolo");
    _register_category("Utilities", "Utilities", "Tools");
    _register_category("Variables", "Variables", "Range");

    // Comments
    _register_node<OScriptNodeComment>("Utilities/add_comment");

    // Constants
    _register_node<OScriptNodeGlobalConstant>("Constants/global_constant");
    _register_node<OScriptNodeMathConstant>("Constants/math_constant");
    _register_node<OScriptNodeTypeConstant>("Constants/type_constant");
    _register_node<OScriptNodeClassConstant>("Constants/class_constant");
    _register_node<OScriptNodeSingletonConstant>("Constants/singleton_constant");

    // Data
    _register_node<OScriptNodeArrayGet>("Types/Array/Operators/get_at_index", array_data);
    _register_node<OScriptNodeArraySet>("Types/Array/Operators/set_at_index", array_data);
    _register_node<OScriptNodeArrayFind>("Types/Array/find_array_element");
    _register_node<OScriptNodeArrayClear>("Types/Array/clear_array");
    _register_node<OScriptNodeArrayAppend>("Types/Array/append_arrays");
    _register_node<OScriptNodeArrayAddElement>("Types/Array/add_element");
    _register_node<OScriptNodeArrayRemoveElement>("Types/Array/remove_element");
    _register_node<OScriptNodeArrayRemoveIndex>("Types/Array/remove_element_by_index");
    _register_node<OScriptNodeMakeArray>("Types/Array/make_array");
    _register_node<OScriptNodeMakeDictionary>("Types/Dictionary/make_dictionary");
    _register_node<OScriptNodeDictionarySet>("Types/Dictionary/set");

    // Dialogue
    _register_node<OScriptNodeDialogueChoice>("Dialogue/choice");
    _register_node<OScriptNodeDialogueMessage>("Dialogue/show_message");

    // Flow Control
    _register_node<OScriptNodeBranch>("Flow Control/branch");
    _register_node<OScriptNodeChance>("Flow Control/chance");
    _register_node<OScriptNodeDelay>("Flow Control/delay");
    _register_node<OScriptNodeForEach>("Flow Control/for_each", without_break);
    _register_node<OScriptNodeForEach>("Flow Control/for_each_with_break", with_break);
    _register_node<OScriptNodeForLoop>("Flow Control/for", without_break);
    _register_node<OScriptNodeForLoop>("Flow Control/for_with_break", with_break);
    _register_node<OScriptNodeRandom>("Flow Control/random");
    _register_node<OScriptNodeSelect>("Flow Control/select");
    _register_node<OScriptNodeSequence>("Flow Control/sequence");
    _register_node<OScriptNodeSwitch>("Flow Control/switch");
    _register_node<OScriptNodeSwitchInteger>("Flow Control/switch_on_integer");
    _register_node<OScriptNodeSwitchString>("Flow Control/switch_on_string");
    _register_node<OScriptNodeTypeCast>("Flow Control/type_cast");
    _register_node<OScriptNodeWhile>("Flow Control/while");

    // Switch on Enums
    for (const String& enum_name : ExtensionDB::get_global_enum_names())
    {
        const EnumInfo ei = ExtensionDB::get_global_enum(enum_name);
        const String category = vformat("Flow Control/Switch On/switch_on_%s", ei.name);
        _register_node<OScriptNodeSwitchEnum>(category, DictionaryUtils::of({ { "enum" , ei.name } }));
    }

    // Functions
    _register_node<OScriptNodeFunctionResult>("add_return_node");

    // Input
    _register_node<OScriptNodeInputAction>("Input/input_action");

    // Memory
    {
        Dictionary new_object;
        if (!_context->filter->target_classes.is_empty())
            new_object["class_name"] = _context->filter->target_classes[0];
        _register_node<OScriptNodeNew>("Memory/new_object", new_object);

        _register_node<OScriptNodeFree>("Memory/free_object");
    }

    // Resource
    _register_node<OScriptNodePreload>("Resource/preload_resource");
    _register_node<OScriptNodeResourcePath>("Resource/get_resource_path");

    // Scene
    _register_node<OScriptNodeInstantiateScene>("Scene/instantiate_scene");
    _register_node<OScriptNodeSceneNode>("Scene/get_scene_node");
    _register_node<OScriptNodeSceneTree>("Scene/get_scene_tree");

    // Signals
    _register_node<OScriptNodeAwaitSignal>("Signals/Await Signal");

    // Utilities
    _register_node<OScriptNodeAutoload>("Utilities/get_autoload");
    _register_node<OScriptNodeEngineSingleton>("Utilities/engine_singleton");
    _register_node<OScriptNodePrintString>("Utilities/print_string");

    // Register each Engine singleton type
    for (const String& name : Engine::get_singleton()->get_singleton_list())
    {
        const String category = vformat("Singletons/%s", name);
        const Dictionary data = DictionaryUtils::of({ { "singleton_name", name } });
        _register_node<OScriptNodeEngineSingleton>(category, data);
    }

    // Variables
    _register_node<OScriptNodeSelf>("Scene/get_self");

    // Register variable assignment differently for macros
    const String local_var_category = vformat("%s/assign_local", func_or_macro_group);
    _register_node<OScriptNodeAssignLocalVariable>(local_var_category);

    // Register Local Object variables
    const String lv_object_name = vformat("%s/local_object", func_or_macro_group);
    const Dictionary object_type_dict = DictionaryUtils::of({ { "type", Variant::OBJECT } });
    _register_node<OScriptNodeLocalVariable>(lv_object_name, object_type_dict);

    // Static Function Calls
    for (const String& class_name : ClassDB::get_class_list())
    {
        for (const String& function_name : ExtensionDB::get_static_function_names(class_name))
        {
            const String category = vformat("Static/%s/%s", class_name, function_name);
            _register_node<OScriptNodeCallStaticFunction>(category,
                DictionaryUtils::of({ { "class_name", class_name }, { "method_name", function_name } }));;
        }
    }

    // Builtin Types
    for (const String& builtin_type_name : ExtensionDB::get_builtin_type_names())
    {
        const BuiltInType type = ExtensionDB::get_builtin_type(builtin_type_name);
        const String type_icon = _get_builtin_type_icon_name(type.type);
        const String type_name = _get_builtin_type_display_name(type.type);

        _register_category(vformat("Types/%s", type_name), type_name, type_icon);

        const Dictionary type_dict = DictionaryUtils::of({ { "type", type.type } });

        // Register local variables differently for macros
        const String lv_name = vformat("Types/%s/local_%s_variable", type_name, type_name);
        _register_node<OScriptNodeLocalVariable>(lv_name, type_dict);

        if (!type.properties.is_empty())
        {
            if (OScriptNodeCompose::is_supported(type.type))
            {
                const String make_category = vformat("Types/%s/make_%s", type_name, type_name.to_lower());
                _register_node<OScriptNodeCompose>(make_category, type_dict);
            }

            const String break_category = vformat("Types/%s/break_%s", type_name, type_name.to_lower());
            _register_node<OScriptNodeDecompose>(break_category, type_dict);
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
                    const String category = vformat("Types/%s/make_%s_from_%s", type_name, type_name.to_lower(), args);

                    _register_node<OScriptNodeComposeFrom>(category, ctor_dict);
                }
            }
        }

        for (const MethodInfo& mi : type.methods)
        {
            const String category = vformat("Types/%s/%s", type_name, mi.name);
            const Dictionary method_dict = DictionaryUtils::from_method(mi);
            const Dictionary data = DictionaryUtils::of({ { "target_type", type.type }, { "method", method_dict } });
            _register_node<OScriptNodeCallMemberFunction>(category, data);
        }

        if (OScriptNodeOperator::is_supported(type.type))
        {
            for (const OperatorInfo& op : type.operators)
            {
                if (!OScriptNodeOperator::is_operator_supported(op))
                    continue;

                String category;
                if (!op.name.match("Not"))
                    category = vformat("Types/%s/Operators/%s_%s", type_name, type_name, op.name);
                else
                    category = vformat("Types/%s/Operators/%s", type_name, op.name);

                if (!op.right_type_name.is_empty())
                {
                    String right_type_name = _get_builtin_type_display_name(op.right_type);
                    category += vformat("_%s", right_type_name);
                }

                const Dictionary data = DictionaryUtils::of({ { "op", op.op },
                                                              { "code", op.code },
                                                              { "name", op.name },
                                                              { "type", type.type },
                                                              { "left_type", op.left_type },
                                                              { "left_type_name", op.left_type_name },
                                                              { "right_type", op.right_type },
                                                              { "right_type_name", op.right_type_name },
                                                              { "return_type", op.return_type } });

                _register_node<OScriptNodeOperator>(category, data);
            }
        }

        if (type.index_returning_type != Variant::NIL && type.type >= Variant::ARRAY)
        {
            const String get_category = vformat("Types/%s/Operators/%s", type_name, "Get At Index");
            const String set_category = vformat("Types/%s/Operators/%s", type_name, "Set At Index");

            const Dictionary data = DictionaryUtils::of({
                { "collection_type", type.type },
                { "index_type", type.index_returning_type } });

            _register_node<OScriptNodeArrayGet>(get_category, data);
            _register_node<OScriptNodeArraySet>(set_category, data);
        }
    }

    // Builtin Functions
    for (const String& function_name : ExtensionDB::get_function_names())
    {
        const FunctionInfo& fi = ExtensionDB::get_function(function_name);

        MethodInfo mi;
        mi.name = fi.name;
        mi.return_val = fi.return_val;
        mi.flags = METHOD_FLAGS_DEFAULT;

        if (fi.is_vararg)
            mi.flags |= METHOD_FLAG_VARARG;

        for (const PropertyInfo& argument : fi.arguments)
            mi.arguments.push_back(argument);

        // Godot exports utility functions under "math", "random", and "general"
        // Remap "general" to "utilities"
        String top_category = fi.category;
        if (top_category.match("general"))
            top_category = "utilities";
        else if (top_category.match("random"))
            top_category = "random_numbers";

        const String category = vformat("%s/%s", top_category.capitalize(), fi.name);
        _register_node<OScriptNodeCallBuiltinFunction>(category, DictionaryUtils::from_method(mi));
    }

    // Autoloads
    for (const String& class_name : OScriptLanguage::get_singleton()->get_global_constant_names())
    {
        String category = vformat("project/Autoloads/%s", class_name);
        _register_node<OScriptNodeAutoload>(category, DictionaryUtils::of({{ "class_name" , class_name }}));
    }
}

void OrchestratorDefaultGraphActionRegistrar::_register_class(const String& p_class_name)
{
    const OrchestratorGraphActionSpec spec(p_class_name, p_class_name, p_class_name);
    _context->list->push_back(memnew(OrchestratorGraphActionMenuItem(spec)));

    _register_methods(p_class_name, ClassDB::class_get_method_list(p_class_name, true));
    _register_properties(p_class_name, ClassDB::class_get_property_list(p_class_name, true));
    _register_signals(p_class_name, ClassDB::class_get_signal_list(p_class_name, true));
}

void OrchestratorDefaultGraphActionRegistrar::_register_methods(const String& p_class_name, const TypedArray<Dictionary>& p_methods)
{
    if (ClassDB::can_instantiate(p_class_name) && !_classes_new_instances.has(p_class_name))
    {
        _classes_new_instances.push_back(p_class_name);

        const String category = vformat("Methods/%s/New Instance", p_class_name);
        _register_node<OScriptNodeNew>(category, DictionaryUtils::of({ { "class_name", p_class_name } }));
    }

    Orchestration* orchestration = _context->filter->get_orchestration();
    for (int i = 0; i < p_methods.size(); i++)
    {
        const MethodInfo mi = DictionaryUtils::to_method(p_methods[i]);

        // Skip private methods
        if (mi.name.begins_with("_") && !(mi.flags & METHOD_FLAG_VIRTUAL))
            continue;

        // Skip internal methods (found from scripts like GDScript)
        if (mi.name.begins_with("@"))
            continue;

        const OrchestratorGraphActionSpec spec = _get_method_spec(mi, orchestration->get_base_type(), p_class_name);

        OrchestratorGraphActionHandler* handler_ptr;
        if (OScriptNodeEvent::is_event_method(mi))
            handler_ptr = memnew(OrchestratorGraphNodeSpawnerEvent(mi));
        else
            handler_ptr = memnew(OrchestratorGraphNodeSpawnerCallMemberFunction(mi, p_class_name));

        Ref<OrchestratorGraphActionHandler> handler(handler_ptr);
        _context->list->push_back(memnew(OrchestratorGraphActionMenuItem(spec, handler)));
    }
}

void OrchestratorDefaultGraphActionRegistrar::_register_properties(const String& p_class_name, const TypedArray<Dictionary>& p_properties)
{
    ScriptServer::GlobalClass global_class;
    if (ScriptServer::is_global_class(p_class_name))
        global_class = ScriptServer::get_global_class(p_class_name);

    for (int i = 0; i < p_properties.size(); i++)
    {
        const PropertyInfo pi = DictionaryUtils::to_property(p_properties[i]);

        // Exclude properties that are not included in the class reference
        if (pi.usage & PROPERTY_USAGE_INTERNAL)
            continue;

        // Exclude category and group properties
        if (pi.usage & PROPERTY_USAGE_CATEGORY || pi.usage & PROPERTY_USAGE_GROUP)
            continue;

        // Skip private properties
        if (pi.name.begins_with("_"))
            continue;

        // For script variables, checks whether its define in the parent or child type
        // If it's defined in the parent type, skip it
        if (pi.usage & PROPERTY_USAGE_SCRIPT_VARIABLE && !global_class.name.is_empty())
        {
            if (ScriptServer::is_global_class(global_class.base_type))
            {
                if (ScriptServer::get_global_class(global_class.base_type).has_property(pi.name))
                    continue;
            }
        }

        #if GODOT_VERSION >= 0x040400
        const String getter_name = ClassDB::class_get_property_getter(p_class_name, pi.name);
        const String setter_name = ClassDB::class_get_property_setter(p_class_name, pi.name);
        #else
        const String getter_name = vformat("get_%s", pi.name);
        const String setter_name = vformat("set_%s", pi.name);
        #endif

        bool has_getter = false;
        if ((global_class.name.is_empty() && ClassDB::class_has_method(p_class_name, getter_name))
            || (!global_class.name.is_empty() && global_class.has_method(getter_name)))
            has_getter = true;

        bool has_setter = false;
        if ((global_class.name.is_empty() && ClassDB::class_has_method(p_class_name, setter_name))
            || (!global_class.name.is_empty() && global_class.has_method(setter_name)))
            has_setter = true;

        if (!has_getter)
        {
            OrchestratorGraphActionSpec getter_spec;
            getter_spec.category = vformat("Properties/%s/get_%s", p_class_name, pi.name);
            getter_spec.tooltip = vformat("Return the value from the property '%s'", pi.name);
            getter_spec.text = vformat("get_%s", pi.name).capitalize();
            getter_spec.keywords = vformat("get,%s,%s", p_class_name, pi.name);
            getter_spec.icon = Variant::get_type_name(pi.type);
            getter_spec.type_icon = "MemberProperty";

            Ref<OrchestratorGraphActionHandler> getter_handler(memnew(OrchestratorGraphNodeSpawnerPropertyGet(pi, Array::make(p_class_name))));
            _context->list->push_back(memnew(OrchestratorGraphActionMenuItem(getter_spec, getter_handler)));
        }

        if (!has_setter)
        {
            OrchestratorGraphActionSpec setter_spec;
            setter_spec.category = vformat("Properties/%s/set_%s", p_class_name, pi.name);
            setter_spec.tooltip = vformat("Set the value of property '%s'", pi.name);
            setter_spec.text = vformat("set_%s", pi.name).capitalize();
            setter_spec.keywords = vformat("set,%s,%s", p_class_name, pi.name);
            setter_spec.icon = Variant::get_type_name(pi.type);
            setter_spec.type_icon = "MemberProperty";

            Ref<OrchestratorGraphActionHandler> setter_handler(memnew(OrchestratorGraphNodeSpawnerPropertySet(pi, Array::make(p_class_name))));
            _context->list->push_back(memnew(OrchestratorGraphActionMenuItem(setter_spec, setter_handler)));
        }
    }
}

void OrchestratorDefaultGraphActionRegistrar::_register_signals(const String& p_class_name, const TypedArray<Dictionary>& p_signals)
{
    ScriptServer::GlobalClass global_class;
    if (ScriptServer::is_global_class(p_class_name))
        global_class = ScriptServer::get_global_class(p_class_name);

    Orchestration* orchestration = _context->filter->get_orchestration();
    for (int i = 0; i < p_signals.size(); i++)
    {
        const MethodInfo si = DictionaryUtils::to_method(p_signals[i]);

        // Skip signals that are defined in parent global class types
        if (!global_class.name.is_empty() && ScriptServer::is_global_class(global_class.base_type))
            if (ScriptServer::get_global_class(global_class.base_type).has_signal(si.name))
                continue;

        const OrchestratorGraphActionSpec spec = _get_signal_spec(si.name, orchestration->get_base_type(), p_class_name);

        MethodInfo mi;
        mi.name = si.name;
        mi.arguments = si.arguments;
        mi.return_val = PropertyInfo(Variant::NIL, "");

        Ref<OrchestratorGraphActionHandler> handler(memnew(OrchestratorGraphNodeSpawnerEmitMemberSignal(mi, p_class_name)));
        _context->list->push_back(memnew(OrchestratorGraphActionMenuItem(spec, handler)));
    }
}

void OrchestratorDefaultGraphActionRegistrar::_register_orchestration_functions()
{
    OrchestratorGraphActionSpec call_function_spec;
    call_function_spec.category = "call_function";
    call_function_spec.tooltip = "Call Functions defined within the orchestration.";
    call_function_spec.text = "Call Function";
    call_function_spec.keywords = "call,function";
    call_function_spec.icon = "MemberMethod";
    call_function_spec.type_icon = "MemberMethod";
    _context->list->push_back(memnew(OrchestratorGraphActionMenuItem(call_function_spec)));

    if (OrchestratorGraphEdit* graph = _context->graph)
    {
        Orchestration* orchestration = graph->get_orchestration();
        for (const Ref<OScriptFunction>& function : orchestration->get_functions())
        {
            if (!function.is_valid() || !function->is_user_defined())
                continue;

            const MethodInfo& mi = function->get_method_info();
            const OrchestratorGraphActionSpec spec = _get_method_spec(mi, orchestration->get_base_type());

            Ref<OrchestratorGraphActionHandler> handler(memnew(OrchestratorGraphNodeSpawnerCallScriptFunction(mi)));
            _context->list->push_back(memnew(OrchestratorGraphActionMenuItem(spec, handler)));
        }
    }
}

void OrchestratorDefaultGraphActionRegistrar::_register_orchestration_variables()
{
    OrchestratorGraphActionSpec variables_spec;
    variables_spec.category = "variables";
    variables_spec.tooltip = "Variables defined within the orchestration.";
    variables_spec.text = "Variables";
    variables_spec.keywords = "variable,variables";
    variables_spec.icon = "MemberProperty";
    variables_spec.type_icon = "MemberProperty";
    _context->list->push_back(memnew(OrchestratorGraphActionMenuItem(variables_spec)));

    if (OrchestratorGraphEdit* graph = _context->graph)
    {
        for (const Ref<OScriptVariable>& variable : graph->get_orchestration()->get_variables())
        {
            if (!variable.is_valid())
                continue;

            const String& variable_name = variable->get_variable_name();

            OrchestratorGraphActionSpec getter_spec;
            getter_spec.category = vformat("Variables/get_%s", variable_name);
            getter_spec.tooltip = vformat("Get the value of the variable '%s'", variable_name);
            getter_spec.text = vformat("Get %s", variable_name);
            getter_spec.keywords = vformat("get,variable,%s", variable_name);
            getter_spec.icon = variable->get_variable_type_name();
            getter_spec.type_icon = "MemberProperty";

            Ref<OrchestratorGraphActionHandler> getter_handler(memnew(OrchestratorGraphNodeSpawnerVariableGet(variable_name)));
            _context->list->push_back(memnew(OrchestratorGraphActionMenuItem(getter_spec, getter_handler)));

            // Constants don't allow spawning using setters
            if (variable->is_constant())
                continue;

            OrchestratorGraphActionSpec setter_spec;
            setter_spec.category = vformat("Variables/set_%s", variable_name);
            setter_spec.tooltip = vformat("Set the value of of variable '%s'", variable_name);
            setter_spec.text = vformat("Set %s", variable_name);
            setter_spec.keywords = vformat("set,variable,%s", variable_name);
            setter_spec.icon = variable->get_variable_type_name();
            setter_spec.type_icon = "MemberProperty";

            Ref<OrchestratorGraphActionHandler> setter_handler(memnew(OrchestratorGraphNodeSpawnerVariableSet(variable_name)));
            _context->list->push_back(memnew(OrchestratorGraphActionMenuItem(setter_spec, setter_handler)));
        }
    }
}

void OrchestratorDefaultGraphActionRegistrar::_register_orchestration_signals()
{
    OrchestratorGraphActionSpec signals_spec;
    signals_spec.category = "emit_signals";
    signals_spec.tooltip = "Signals defined within the orchestration.";
    signals_spec.text = "emit_signals";
    signals_spec.keywords = "signal,signals";
    signals_spec.icon = "MemberSignal";
    signals_spec.type_icon = "MemberSignal";
    _context->list->push_back(memnew(OrchestratorGraphActionMenuItem(signals_spec)));

    if (OrchestratorGraphEdit* graph = _context->graph)
    {
        Orchestration* orchestration = graph->get_owning_graph()->get_orchestration();
        for (const Ref<OScriptSignal>& signal : orchestration->get_custom_signals())
        {
            if (!signal.is_valid())
                continue;

            const OrchestratorGraphActionSpec spec = _get_signal_spec(signal->get_signal_name(), orchestration->get_base_type());

            Ref<OrchestratorGraphActionHandler> handler(memnew(OrchestratorGraphNodeSpawnerEmitSignal(signal->get_method_info())));
            _context->list->push_back(memnew(OrchestratorGraphActionMenuItem(spec, handler)));
        }
    }
}

void OrchestratorDefaultGraphActionRegistrar::register_actions(const OrchestratorGraphActionRegistrarContext& p_context)
{
    OrchestratorSettings* settings = OrchestratorSettings::get_singleton();
    _friendly_method_names = settings->get_setting("ui/components_panel/show_function_friendly_names", true);

    _context = &p_context;

    if (p_context.graph)
    {
        if (p_context.filter->has_target_object())
        {
            const Object* object = p_context.filter->target_object->get_target();
            const Ref<Script> script = object->get_script();

            String global_name;
            if (script.is_valid())
                global_name = ScriptServer::get_global_name(script);

            if (!global_name.is_empty())
            {
                // The target object has a named script attached
                // In this context,register script methods, properties, and signals using the script's
                // class_name rather than adding these as part of the base script type.
                const PackedStringArray script_class_names = ScriptServer::get_class_hierarchy(global_name);
                for (const String& class_name : script_class_names)
                {
                    const ScriptServer::GlobalClass global_class = ScriptServer::get_global_class(class_name);
                    _register_methods(class_name, global_class.get_method_list());
                    _register_properties(class_name, global_class.get_property_list());
                    _register_signals(class_name, global_class.get_signal_list());
                }
            }
            else if (script.is_valid())
            {
                const String script_class_name = p_context.filter->get_target_class();
                _register_methods(script_class_name, script->get_script_method_list());
                _register_properties(script_class_name, script->get_script_property_list());
                _register_signals(script_class_name, script->get_script_signal_list());
            }

            const PackedStringArray class_names = _get_class_hierarchy(p_context.filter->get_target_class());
            for (const String& class_name : class_names)
                _register_class(class_name);
        }
        else if (!p_context.filter->target_classes.is_empty())
        {
            PackedStringArray classes_added;
            for (const StringName& target_class_name : p_context.filter->target_classes)
            {
                PackedStringArray class_names;
                if (ScriptServer::is_global_class(target_class_name))
                    class_names = ScriptServer::get_class_hierarchy(target_class_name, true);
                else
                    class_names = _get_class_hierarchy(target_class_name);

                for (const String& class_name : class_names)
                {
                    if (!classes_added.has(class_name))
                    {
                        classes_added.push_back(class_name);
                        if (ScriptServer::is_global_class(class_name))
                        {
                            const ScriptServer::GlobalClass global_class = ScriptServer::get_global_class(class_name);
                            _register_methods(class_name, global_class.get_method_list());
                            _register_properties(class_name, global_class.get_property_list());
                            _register_signals(class_name, global_class.get_signal_list());
                        }
                        else
                            _register_class(class_name);
                    }
                }
            }
        }
        else
        {
            const String base_type = p_context.graph->get_orchestration()->get_base_type();
            const PackedStringArray class_names = _get_class_hierarchy(base_type);
            for (const String& class_name : class_names)
                _register_class(class_name);
        }
    }

    _register_orchestration_nodes();
    _register_orchestration_functions();
    _register_orchestration_variables();
    _register_orchestration_signals();
}
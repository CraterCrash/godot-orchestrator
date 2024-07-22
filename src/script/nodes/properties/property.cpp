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
#include "property.h"

#include "common/dictionary_utils.h"
#include "script/script.h"
#include "script/script_server.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/scene_tree.hpp>

OScriptNodeProperty::OScriptNodeProperty()
{
    _flags = ScriptNodeFlags::NONE;
}

void OScriptNodeProperty::_bind_methods()
{
    BIND_ENUM_CONSTANT(CallMode::CALL_SELF)
    BIND_ENUM_CONSTANT(CallMode::CALL_INSTANCE)
    BIND_ENUM_CONSTANT(CallMode::CALL_NODE_PATH)
}

void OScriptNodeProperty::_get_property_list(List<PropertyInfo>* r_list) const
{
    uint32_t usage = PROPERTY_USAGE_STORAGE;
    if (!_node_path.is_empty())
    {
        usage |= PROPERTY_USAGE_EDITOR;
        usage |= PROPERTY_USAGE_READ_ONLY;
    }

    const String modes = "Self,Instance,Node Path";
    r_list->push_back(PropertyInfo(Variant::INT, "mode", PROPERTY_HINT_ENUM, modes, usage));

    // todo: deprecated, remove in a future release
    r_list->push_back(PropertyInfo(Variant::STRING, "target_class", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    r_list->push_back(PropertyInfo(Variant::STRING, "property_name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    r_list->push_back(PropertyInfo(Variant::STRING, "property_hint", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));

    r_list->push_back(PropertyInfo(Variant::NODE_PATH, "node_path", PROPERTY_HINT_NONE, "", usage));

    // todo:
    // For now we encode property details at the node and pin level, which is wasteful.
    // Given that property nodes are used infrequently, its not a high priority, but
    // this should be fixed to avoid the duplicity in the pin.
    r_list->push_back(PropertyInfo(Variant::DICTIONARY, "property", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
}

bool OScriptNodeProperty::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("mode"))
    {
        r_value = _call_mode;
        return true;
    }
    else if (p_name.match("target_class"))
    {
        r_value = _base_type;
        return true;
    }
    else if (p_name.match("property_name"))
    {
        r_value = _property.name;
        return true;
    }
    else if (p_name.match("property_hint"))
    {
        r_value = _property.hint_string;
        return true;
    }
    else if (p_name.match("property"))
    {
        r_value = DictionaryUtils::from_property(_property);
        return true;
    }
    else if (p_name.match("node_path"))
    {
        r_value = _node_path;
        return true;
    }
    return false;
}

bool OScriptNodeProperty::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("mode"))
    {
        _call_mode = CallMode(int(p_value));
        return true;
    }
    else if (p_name.match("target_class"))
    {
        _base_type = p_value;
        return true;
    }
    else if (p_name.match("property_name"))
    {
        _property.name = p_value;
        return true;
    }
    else if (p_name.match("property_hint"))
    {
        _property.hint_string = p_value;
        return true;
    }
    else if (p_name.match("property"))
    {

        _property = DictionaryUtils::to_property(p_value);
        _has_property = true;
        return true;
    }
    else if (p_name.match("node_path"))
    {
        _node_path = p_value;
        return true;
    }
    return false;
}

bool OScriptNodeProperty::_property_exists(const TypedArray<Dictionary>& p_properties) const
{
    for (int index = 0; index < p_properties.size(); ++index)
    {
        const Dictionary& property = p_properties[index];
        if (property.has("name") && _property.name.match(property["name"]))
            return true;
    }
    return false;
}

TypedArray<Dictionary> OScriptNodeProperty::_get_class_property_list(const String& p_class_name) const
{
    if (ScriptServer::is_global_class(p_class_name))
        return ScriptServer::get_global_class(p_class_name).get_property_list();

    return ClassDB::class_get_property_list(p_class_name);
}

bool OScriptNodeProperty::_get_class_property(const String& p_class_name, const String& p_name, PropertyInfo& r_property)
{
    const TypedArray<Dictionary> properties = _get_class_property_list(p_class_name);
    for (int index = 0; index < properties.size(); index++)
    {
        const Dictionary& dict = properties[index];
        if (dict.has("name") && p_name.match(dict["name"]))
        {
            r_property = DictionaryUtils::to_property(dict);
            return true;
        }
    }
    return false;
}

void OScriptNodeProperty::_upgrade(uint32_t p_version, uint32_t p_current_version)
{
    if (p_version == 1 && p_current_version >= 2)
    {
        if (!_has_property)
        {
            if (_call_mode == CALL_INSTANCE)
            {
                String class_name = _base_type;
                if (class_name.is_empty())
                {
                    // Base type was never encoded into the node.
                    // Attempt to resolve the base type based on the connection, if possible.
                    const Ref<OScriptNodePin> target = find_pin("target", PD_Input);
                    if (target.is_valid() && target->has_any_connections())
                    {
                        const Ref<OScriptNodePin> source = target->get_connections()[0];
                        if (source.is_valid())
                        {
                            const PropertyInfo& pi = source->get_property_info();
                            if (pi.type == Variant::OBJECT && !pi.class_name.is_empty())
                                class_name = pi.class_name;
                        }
                    }
                }

                // todo: what if the target should be a super type?

                PropertyInfo property;
                if (_get_class_property(class_name, _property.name, property))
                {
                    _base_type = class_name;
                    _property = property;
                    _has_property = true;
                }
            }
            else if (_call_mode == CALL_SELF)
            {
                PropertyInfo property;
                if (_get_class_property(get_orchestration()->get_base_type(), _property.name, property))
                {
                    _property = property;
                    _has_property = true;
                }
            }
            else if (_call_mode == CALL_NODE_PATH)
            {
                // todo: how to fix this?
            }

            if (_has_property)
                reconstruct_node();
        }
    }

    super::_upgrade(p_version, p_current_version);
}

void OScriptNodeProperty::post_initialize()
{
    // Cache property details in node
    for (const Ref<OScriptNodePin>& pin : find_pins())
    {
        if (pin->get_pin_name().match(_property.name))
        {
            _property = pin->get_property_info();
            break;
        }
    }

    super::post_initialize();
}

String OScriptNodeProperty::get_icon() const
{
    return "MemberProperty";
}

String OScriptNodeProperty::get_help_topic() const
{
    #if GODOT_VERSION >= 0x040300
    switch (_call_mode)
    {
        case CALL_INSTANCE:
            return vformat("class_property:%s:%s", _base_type, _property.name);
        case CALL_SELF:
            return vformat("class_property:%s:%s", get_orchestration()->get_base_type(), _property.name);
        case CALL_NODE_PATH:
        {
            if (SceneTree* st = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop()))
            {
                Node* node = st->get_edited_scene_root()->get_node_or_null(_node_path);
                if (node)
                    return vformat("class_property:%s:%s", node->get_class(), _property.name);
            }
            break;
        }
        default:
            break;
    }
    #endif
    return super::get_help_topic();
}


void OScriptNodeProperty::initialize(const OScriptNodeInitContext& p_context)
{
    ERR_FAIL_COND_MSG(!p_context.property, "A property node requires a PropertyInfo");

    const PropertyInfo& pi = p_context.property.value();
    _property = pi;
    _has_property = true;

    if (p_context.node_path)
    {
        _call_mode = CALL_NODE_PATH;
        _node_path = p_context.node_path.value();
    }
    else if (p_context.class_name)
    {
        _call_mode = CALL_INSTANCE;
        _base_type = p_context.class_name.value();
    }
    else
    {
        _call_mode = CALL_SELF;
    }

    super::initialize(p_context);
}

void OScriptNodeProperty::validate_node_during_build(BuildLog& p_log) const
{
    if (_call_mode == CALL_INSTANCE)
    {
        const Ref<OScriptNodePin> target = find_pin("target", PD_Input);
        if (!target->has_any_connections())
        {
            p_log.error(this, "Requires a connection.");
        }
        else
        {
            const Ref<OScriptNodePin> source = target->get_connections()[0];
            const Ref<OScriptTargetObject> target_object = source->resolve_target();
            if (!target_object.is_valid() || !target_object->has_target())
            {
                if (!_property_exists(ClassDB::class_get_property_list(target->get_property_info().class_name)))
                    p_log.error(this, vformat("No property name '%s' found in class '%s'", _property.name, _property.class_name));
            }
            else
            {
                const Ref<Script> script = target_object->get_target()->get_script();
                const bool script_property = script.is_valid() && _property_exists(script->get_script_property_list());
                const bool object_property = _property_exists(target_object->get_target()->get_property_list());

                if (!script_property && !object_property)
                    p_log.error(this, vformat("No property name '%s' found", _property.name));
            }
        }
    }
    else if (_call_mode == CALL_SELF)
    {
        const String base_type = get_orchestration()->get_base_type();
        if (!_property_exists(_get_class_property_list(base_type)))
            p_log.error(this, vformat("No property named '%s' on class '%s'", _property.name, base_type));
    }
    else if (_call_mode == CALL_NODE_PATH)
    {
        if (SceneTree* st = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop()))
        {
            // Only if the node is found in the current edited scene, validate existance of property
            if (Node* node = st->get_edited_scene_root()->get_node_or_null(_node_path))
            {
                const Ref<Script> script = node->get_script();
                const bool script_property = script.is_valid() && _property_exists(script->get_script_property_list());
                const bool object_property = _property_exists(node->get_property_list());

                if (!script_property && !object_property)
                    p_log.error(this, vformat("No property name '%s' found for node path '%s'.", _property.name, _node_path));
            }
        }
    }
    super::validate_node_during_build(p_log);
}


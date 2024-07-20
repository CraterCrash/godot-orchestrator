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
#include "self.h"

#include "common/property_utils.h"
#include "common/scene_utils.h"
#include "common/version.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>

class OScriptNodeSelfInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeSelf);
public:
    int step(OScriptExecutionContext& p_context) override
    {
        Variant owner = p_context.get_owner();
        p_context.set_output(0, &owner);
        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeSelf::_upgrade(uint32_t p_version, uint32_t p_current_version)
{
    if (p_version == 1 and p_current_version >= 2)
    {
        // Fixup - makes sure that base type matches pin
        const Ref<OScriptNodePin> self = find_pin("self", PD_Output);
        if (self.is_valid() && self->get_property_info().class_name != get_orchestration()->get_base_type())
            reconstruct_node();
    }

    super::_upgrade(p_version, p_current_version);
}

void OScriptNodeSelf::post_initialize()
{
    if (_is_in_editor() && get_orchestration())
        get_orchestration()->get_self()->connect("changed", callable_mp(this, &OScriptNodeSelf::_on_script_changed));

    super::post_initialize();
}

void OScriptNodeSelf::post_placed_new_node()
{
    if (_is_in_editor() && get_orchestration())
        get_orchestration()->get_self()->connect("changed", callable_mp(this, &OScriptNodeSelf::_on_script_changed));

    super::post_placed_new_node();
}

void OScriptNodeSelf::allocate_default_pins()
{
    create_pin(PD_Output, PT_Data, PropertyUtils::make_object("self", get_orchestration()->get_base_type()));
}

String OScriptNodeSelf::get_tooltip_text() const
{
    return "Get a reference to this instance of an Orchestration";
}

String OScriptNodeSelf::get_node_title() const
{
    return "Get self";
}

String OScriptNodeSelf::get_help_topic() const
{
    #if GODOT_VERSION >= 0x040300
    return vformat("class:%s", _orchestration->get_base_type());
    #else
    return super::get_help_topic();
    #endif
}

String OScriptNodeSelf::get_icon() const
{
    if (get_orchestration())
        return get_orchestration()->get_base_type();

    return super::get_icon();
}

Ref<OScriptTargetObject> OScriptNodeSelf::resolve_target(const Ref<OScriptNodePin>& p_pin) const
{
    if (_is_in_editor())
    {
        Ref<OScript> script = get_orchestration()->get_self();
        if (script.is_valid())
        {
            // For now look at the current edited scene, and if one exists, try and find the node
            // that has the attached script to refer to as "self". This is just an approximation,
            // as multiple nodes could have the script attached.
            Node* root = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop())->get_edited_scene_root();
            if (root)
            {
                Node* node = SceneUtils::get_node_with_script(script, root, root);
                return memnew(OScriptTargetObject(node, false));
            }
        }
    }

    return super::resolve_target(p_pin);
}

OScriptNodeInstance* OScriptNodeSelf::instantiate()
{
    OScriptNodeSelfInstance* i = memnew(OScriptNodeSelfInstance);
    i->_node = this;
    return i;
}

void OScriptNodeSelf::validate_node_during_build(BuildLog& p_log) const
{
    const String base_type = get_orchestration()->get_base_type();

    const Ref<OScriptNodePin> self = find_pin("self", PD_Output);
    if (!self.is_valid())
        p_log.error(this, "No output pin found.");
    else if (self->get_property_info().class_name != base_type)
        p_log.error(this, "Node requires reconstruction, right-click node and select 'Refresh Nodes'.");

    super::validate_node_during_build(p_log);
}

void OScriptNodeSelf::_on_script_changed()
{
    reconstruct_node();
    _notify_pins_changed();
}

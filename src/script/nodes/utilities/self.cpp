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

#include "common/version.h"

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
    create_pin(PD_Output, "self", Variant::OBJECT)->set_flags(OScriptNodePin::Flags::DATA);
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

OScriptNodeInstance* OScriptNodeSelf::instantiate()
{
    OScriptNodeSelfInstance* i = memnew(OScriptNodeSelfInstance);
    i->_node = this;
    return i;
}

void OScriptNodeSelf::_on_script_changed()
{
    _notify_pins_changed();
}

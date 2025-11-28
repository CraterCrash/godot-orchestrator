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
#include "print_string.h"

#include "common/property_utils.h"
#include "common/settings.h"
#include "godot_cpp/classes/editor_settings.hpp"
#include "script/script.h"
#include "script/vm/script_state.h"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/scene_tree_timer.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/mutex_lock.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

class OScriptNodePrintStringInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodePrintString);

private:
    static HashMap<String, Node*> _scene_containers;

public:
    float _scale{ 1.f };

    int get_working_memory_size() const override { return 1; }
    int step(OScriptExecutionContext& p_context) override;

private:
    Node* _get_or_create_ui_container(Node* p_root_node);
    SceneTree* _get_tree(OScriptExecutionContext& p_context);

    void _ui_container_node_added(Node* p_node);
    void _ui_container_node_removed(Node* p_node);
};

HashMap<String, Node*> OScriptNodePrintStringInstance::_scene_containers;

int OScriptNodePrintStringInstance::step(OScriptExecutionContext& p_context)
{
    // When this node is executed in export builds, it does nothing.
    if (!OS::get_singleton()->has_feature("editor"))
        return 0;

    if (p_context.get_step_mode() != STEP_MODE_RESUME)
    {
        if (Node* owner = Object::cast_to<Node>(p_context.get_owner()))
        {
            if (!owner->is_inside_tree())
            {
                Ref<OScriptState> state;
                state.instantiate();
                state->connect_to_signal(owner, "tree_entered", Array());
                p_context.set_working_memory(0, state);
                return STEP_FLAG_YIELD;
            }
        }
    }

    // When this node is called from within a "_ready" function, it is unable to add the UI bits
    // to the scene at the scene root because the scene's root is not yet marked ready. This UI
    // needs to be added at the root so that we have a way to resolve whether the UI bits exist
    // or not in case multiple PrintString nodes are invoked. The parent node isn't used for
    // the same reason.
    //
    // When this node exists in the "_ready" function, the text container object is created, but
    // it isn't added to the scene until the end of the frame, meaning that the text will appear
    // in the console just slightly before the UI, which is expected.
    //
    // It also means no assumption should be made about the returned object's existence in the
    // current scene, but rather that it will exist at some point in the future.
    if (p_context.get_input(1))
    {
        SceneTree* tree = _get_tree(p_context);

        Node* root = tree->get_current_scene();
        if (!root)
            root = tree->get_root()->get_child(0);

        if (root)
        {
            if (Node* container = _get_or_create_ui_container(root))
            {
                Variant text = p_context.get_input(0);

                RichTextLabel* label = memnew(RichTextLabel);
                label->set_fit_content(true);
                label->set_use_bbcode(true);
                label->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
                label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
                label->set_autowrap_mode(TextServer::AUTOWRAP_OFF);
                container->add_child(label);

                label->push_color(p_context.get_input(3));
                label->append_text(text);
                label->pop();

                Ref<SceneTreeTimer> timer = tree->create_timer(p_context.get_input(4));
                timer->connect("timeout", Callable(label, "queue_free"));
            }
        }
    }

    if (p_context.get_input(2))
        UtilityFunctions::print(p_context.get_input(0));

    return 0;
}

Node* OScriptNodePrintStringInstance::_get_or_create_ui_container(Node* p_root_node)
{
    String scene_name = p_root_node->get_scene_file_path();

    // Guarantees that only one element will be added to the scene.
    MutexLock lock(*OScriptLanguage::get_singleton()->lock.ptr());

    Node* node;
    if (!_scene_containers.has(scene_name))
    {
        // There currently doesn't exist an entry here for this scene, create it.
        VBoxContainer* container = memnew(VBoxContainer);
        container->set_anchors_preset(Control::PRESET_TOP_LEFT);
        container->set_position(Vector2(10, 10));
        container->set_custom_minimum_size(Vector2(300, 100));
        container->set_name("PrintStringUI");
        container->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
        container->set_scale(Vector2(_scale, _scale));

        // We maintain a cache of the container by scene to guarantee that if multiple PrintString
        // nodes attempt to render text, the UI will only have a single container.
        _scene_containers[scene_name] = container;
        node = container;

        // We specifically connect to these signals, especially tree_exiting so that we can do
        // cache cleanup on _scene_containers should the node be removed from the scene.
        container->connect(
            "ready", callable_mp(this, &OScriptNodePrintStringInstance::_ui_container_node_added).bind(container));
        container->connect("tree_exiting",
                           callable_mp(this, &OScriptNodePrintStringInstance::_ui_container_node_removed).bind(container));

        // There are situations where the root node has not yet finished setting things up
        // or the game may be in the middle of a signal dispatch, in which case we always
        // defer adding the container to avoid any scene errors.
        p_root_node->call_deferred("add_child", container);
    }
    else
    {
        // Scene has the object or its deferred and will exist soon.
        node = _scene_containers.get(scene_name);
    }

    return node;
}

SceneTree* OScriptNodePrintStringInstance::_get_tree(OScriptExecutionContext& p_context)
{
    return Object::cast_to<Node>(p_context.get_owner())->get_tree();
}

void OScriptNodePrintStringInstance::_ui_container_node_added([[maybe_unused]] Node* p_node)
{

}

void OScriptNodePrintStringInstance::_ui_container_node_removed(Node* p_node)
{
    // When the node is removed, we should remove it from the cache
    //
    // The only time the node should be removed is during scene reloads, and this would be a great
    // reason for us to remove it from the cache and let a new script instance re-create it.
    MutexLock lock(*OScriptLanguage::get_singleton()->lock.ptr());
    for (const KeyValue<String, Node*>& E : _scene_containers)
    {
        if (E.value == p_node)
        {
            _scene_containers.erase(E.key);
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodePrintString::allocate_default_pins()
{
    create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"));
    create_pin(PD_Input, PT_Data, PropertyUtils::make_typed("Text", Variant::STRING), "Hello");
    create_pin(PD_Input, PT_Data, PropertyUtils::make_typed("PrintToScreen", Variant::BOOL), true);
    create_pin(PD_Input, PT_Data, PropertyUtils::make_typed("PrintToLog", Variant::BOOL), true);
    create_pin(PD_Input, PT_Data, PropertyUtils::make_typed("TextColor", Variant::COLOR), Color(1, 1, 1));
    create_pin(PD_Input, PT_Data, PropertyUtils::make_typed("Duration", Variant::FLOAT), 2);
    create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("ExecOut"));

    super::allocate_default_pins();
}

String OScriptNodePrintString::get_tooltip_text() const
{
    return vformat("%s\n%s", "Prints a string to the log, and optionally to the screen.",
                   "If Print To Log is true, it will be shown in the output window.");
}

void OScriptNodePrintString::reallocate_pins_during_reconstruction(const Vector<Ref<OScriptNodePin>>& p_old_pins)
{
    super::reallocate_pins_during_reconstruction(p_old_pins);

    for (const Ref<OScriptNodePin>& pin : p_old_pins)
    {
        if (pin->is_input() && !pin->is_execution())
        {
            Ref<OScriptNodePin> new_pin = find_pin(pin->get_pin_name(), PD_Input);
            if (new_pin.is_valid())
                new_pin->set_default_value(pin->get_effective_default_value());
        }
    }
}

OScriptNodeInstance* OScriptNodePrintString::instantiate()
{
    OScriptNodePrintStringInstance* i = memnew(OScriptNodePrintStringInstance);
    i->_node = this;

    const String scale_str = ORCHESTRATOR_GET("settings/runtime/print_string_scale", "100%");
    i->_scale = scale_str.replace("%", "").to_float() / 100.0f;
    return i;
}

OScriptNodePrintString::OScriptNodePrintString()
{
    _flags.set_flag(DEVELOPMENT_ONLY);
}
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
#include "script/nodes/utilities/print_string.h"

#include "common/macros.h"
#include "common/property_utils.h"
#include "common/settings.h"
#include "script/script.h"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/scene_tree_timer.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/window.hpp>

void OScriptNodePrintString::allocate_default_pins() {
    create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"));
    create_pin(PD_Input, PT_Data, PropertyUtils::make_typed("Text", Variant::STRING), "Hello");
    create_pin(PD_Input, PT_Data, PropertyUtils::make_typed("PrintToScreen", Variant::BOOL), true);
    create_pin(PD_Input, PT_Data, PropertyUtils::make_typed("PrintToLog", Variant::BOOL), true);
    create_pin(PD_Input, PT_Data, PropertyUtils::make_typed("TextColor", Variant::COLOR), Color(1, 1, 1));
    create_pin(PD_Input, PT_Data, PropertyUtils::make_typed("Duration", Variant::FLOAT), 2);
    create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("ExecOut"));

    super::allocate_default_pins();
}

String OScriptNodePrintString::get_tooltip_text() const {
    return vformat("%s\n%s\n%s", "Prints a string to the log, and optionally to the screen.",
                   "If Print To Log is true, it will be shown in the output window.",
                   "For tool-based scripts, this node only writes to the log if enabled.");
}

void OScriptNodePrintString::reallocate_pins_during_reconstruction(const Vector<Ref<OScriptNodePin>>& p_old_pins) {
    super::reallocate_pins_during_reconstruction(p_old_pins);

    for (const Ref<OScriptNodePin>& pin : p_old_pins) {
        if (pin->is_input() && !pin->is_execution()) {
            Ref<OScriptNodePin> new_pin = find_pin(pin->get_pin_name(), PD_Input);
            if (new_pin.is_valid()) {
                new_pin->set_default_value(pin->get_effective_default_value());
            }
        }
    }
}

OScriptNodePrintString::OScriptNodePrintString() {
    _flags.set_flag(DEVELOPMENT_ONLY);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OScriptNodePrintStringOverlay

HashMap<Node*, OScriptNodePrintStringOverlay*> OScriptNodePrintStringOverlay::_overlays;

void OScriptNodePrintStringOverlay::_tree_entered() {
    // This must be tracked because the overlay is queued deferred, and if the overlay is queued while
    // the scene tree is actively being torn down, we need to know whether this object will be destroyed
    // by the scene or if we should clean it up.
    _is_in_tree = true;
}

void OScriptNodePrintStringOverlay::_tree_exiting() {
    for (const KeyValue<Node*, OScriptNodePrintStringOverlay*>& E : _overlays) {
        if (E.value == this) {
            _overlays.erase(E.key);
            break;
        }
    }
}

void OScriptNodePrintStringOverlay::_root_tree_exiting(Node* p_root) {
    if (p_root != nullptr) {
        _overlays.erase(p_root);
    }

    if (!_is_in_tree) {
        // In the event this object never made its way into the tree, cleanup
        queue_free();
    }
}

void OScriptNodePrintStringOverlay::add_text(const String& p_text, const String& p_key, float p_duration_sec, const Color& p_color) {
    RichTextLabel* label = nullptr;
    if (p_key.length() > 0 && p_key.to_lower() != "none") {
        Node* child = get_child(0)->find_child(p_key, false, false);
        if (child) {
            label = cast_to<RichTextLabel>(child);
            ERR_FAIL_NULL(label);
        }
    }

    if (label == nullptr) {
        label = memnew(RichTextLabel);
        label->set_fit_content(true);
        label->set_use_bbcode(true);
        label->set_mouse_filter(MOUSE_FILTER_IGNORE);
        label->set_h_size_flags(SIZE_EXPAND_FILL);
        label->set_autowrap_mode(TextServer::AUTOWRAP_OFF);
        get_child(0)->add_child(label);
    }

    label->push_color(p_color);
    label->append_text(p_text);
    label->pop();

    SceneTree* tree = cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
    Ref<SceneTreeTimer> timer = tree->create_timer(p_duration_sec);
    timer->connect("timeout", callable_mp_cast(label, Node, queue_free));
}

OScriptNodePrintStringOverlay* OScriptNodePrintStringOverlay::get_or_create_overlay() {
    SceneTree* tree = cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
    ERR_FAIL_NULL_V_MSG(tree, nullptr, "Cannot get or create print string overlay, no scene tree was found.");

    Node* root = tree->get_root();
    ERR_FAIL_NULL_V_MSG(root, nullptr, "Cannot get or create print string overlay, no scene root node found.");

    if (_overlays.has(root)) {
        return _overlays[root];
    }

    OScriptNodePrintStringOverlay* overlay = memnew(OScriptNodePrintStringOverlay);

    // These signals are extremely important to guard against resource leaks.
    // The 'tree_exiting' handles cleanups of objects that are queued but not yet entered the tree.
    // The 'tree_entered' handles setting a flag the node is in the tree.
    // The 'tree_exiting' handles cleaning up after itself from the container map.
    root->connect("tree_exiting", callable_mp(overlay, &OScriptNodePrintStringOverlay::_root_tree_exiting).bind(root));
    overlay->connect("tree_entered", callable_mp(overlay, &OScriptNodePrintStringOverlay::_tree_entered));
    overlay->connect("tree_exiting", callable_mp(overlay, &OScriptNodePrintStringOverlay::_tree_exiting));

    _overlays[root] = overlay;
    root->call_deferred("add_child", overlay);

    return overlay;
}

void OScriptNodePrintStringOverlay::_bind_methods() {
}

OScriptNodePrintStringOverlay::OScriptNodePrintStringOverlay() {
    add_theme_constant_override("margin_left", 10);
    add_theme_constant_override("margin_right", 10);
    add_theme_constant_override("margin_top", 10);
    add_theme_constant_override("margin_bottom", 10);
    set_anchors_preset(PRESET_FULL_RECT);
    set_name("OrchestratorPrintStringOverlay");
    set_mouse_filter(MOUSE_FILTER_IGNORE);

    VBoxContainer* container = memnew(VBoxContainer);
    add_child(container);

    const String scale_percent = ORCHESTRATOR_GET("settings/runtime/print_string_scale", "100%");
    const float scale = static_cast<float>(scale_percent.replace("%", "").to_float() / 100.0f);
    set_scale(Vector2(scale, scale));
}

OScriptNodePrintStringOverlay::~OScriptNodePrintStringOverlay() {
}
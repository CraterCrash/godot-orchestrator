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
#include "script/nodes/dialogue/dialogue_message.h"

#include "common/property_utils.h"
#include "common/settings.h"
#include "script/nodes/dialogue/dialogue_choice.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/node.hpp>

void OScriptNodeDialogueMessage::_upgrade(uint32_t p_version, uint32_t p_current_version) {
    if (p_version == 1 && p_current_version >= 2) {
        // Fixup - make sure the scene pin has a hint string encoded
        Ref<OScriptNodePin> scene = find_pin("scene", PD_Input);
        if (scene.is_valid() && scene->get_property_info().hint_string.is_empty()) {
            reconstruct_node();
        }
    }
    super::_upgrade(p_version, p_current_version);
}

void OScriptNodeDialogueMessage::post_initialize() {
    _choices = 0;
    for (const Ref<OScriptNodePin>& pin : find_pins(PD_Input)) {
        if (pin->get_pin_name().begins_with("choice_")) {
            _choices++;
        }
    }
    super::post_initialize();
}

void OScriptNodeDialogueMessage::allocate_default_pins() {
    create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"));
    create_pin(PD_Input, PT_Data, PropertyUtils::make_multiline("name"))->set_label("Speaker");
    create_pin(PD_Input, PT_Data, PropertyUtils::make_multiline("text"))->set_label("Message");
    create_pin(PD_Input, PT_Data, PropertyUtils::make_file("scene", "*.scn,*.tscn; Scene Files"), "");

    if (_choices > 0) {
        for (int i = 0; i < 4; i++) {
            create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("temp_" + itos(i)))->set_flag(OScriptNodePin::Flags::HIDDEN);
        }

        for (int i = 0; i < _choices; i++) {
            const String pin_name = _get_pin_name_given_index(i);
            const PropertyInfo pi = PropertyUtils::make_object(pin_name, OScriptNodeDialogueChoice::get_class_static());

            Ref<OScriptNodePin> input = create_pin(PD_Input, PT_Data, pi);
            input->set_flag(OScriptNodePin::Flags::IGNORE_DEFAULT);

            create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec(vformat("%s_out", pin_name)));
        }
    } else {
        create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("ExecOut"));
    }
    super::allocate_default_pins();
}

String OScriptNodeDialogueMessage::get_tooltip_text() const {
    return "Displays a dialogue selection to the player, optionally using a custom scene.";
}

String OScriptNodeDialogueMessage::get_node_title() const {
    return "Show Dialogue Message";
}

void OScriptNodeDialogueMessage::add_dynamic_pin() {
    _choices++;
    reconstruct_node();
}

bool OScriptNodeDialogueMessage::can_add_dynamic_pin() const {
    return _choices < 10;
}

bool OScriptNodeDialogueMessage::can_remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin) const {
    if (p_pin.is_valid() && p_pin->get_pin_name().begins_with(get_pin_prefix())) {
        return true;
    }
    return super::can_remove_dynamic_pin(p_pin);
}

void OScriptNodeDialogueMessage::remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin) {
    if (!p_pin.is_valid()) {
        return;
    }

    String other_pin_name;
    if (p_pin->is_input()) {
        other_pin_name = String(p_pin->get_pin_name()) + "_out";
    }
    else {
        other_pin_name = p_pin->get_pin_name().substr(0, p_pin->get_pin_name().length() - 4); // 4 length of "_out"
    }

    Ref<OScriptNodePin> other = find_pin(other_pin_name, p_pin->get_complimentary_direction());
    if (!other.is_valid()) {
        return;
    }

    // Calculate the pin offset based on the input pin, not the output
    // This is needed to adjust the connections later.
    int pin_offset = p_pin->is_input() ? p_pin->get_pin_index() : other->get_pin_index();

    p_pin->unlink_all(true);
    other->unlink_all(true);
    remove_pin(p_pin);
    remove_pin(other);

    // Adjust both input and output
    // We explicitly need to adjust connections separately due to hidden pins
    _adjust_connections(pin_offset, -1, PD_Input);
    _adjust_connections(pin_offset - 4, -1, PD_Output);

    _choices--;
    reconstruct_node();
}
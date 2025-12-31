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
#include "script/nodes/signals/emit_member_signal.h"

#include "common/dictionary_utils.h"
#include "common/property_utils.h"

void OScriptNodeEmitMemberSignal::_script_changed() {
    // Update the pin's target class details when script changes, but only if no connections exist
    const Ref<OScriptNodePin> target = find_pin("target", PD_Input);
    if (target.is_valid() && _target_class != get_orchestration()->get_base_type() && !target->has_any_connections()) {
        _target_class = get_orchestration()->get_base_type();
        reconstruct_node();
    }
}

void OScriptNodeEmitMemberSignal::_get_property_list(List<PropertyInfo>* r_list) const {
    r_list->push_back(PropertyInfo(Variant::STRING, "target_class", PROPERTY_HINT_TYPE_STRING, "", PROPERTY_USAGE_STORAGE));
    r_list->push_back(PropertyInfo(Variant::DICTIONARY, "method", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
}

bool OScriptNodeEmitMemberSignal::_get(const StringName& p_name, Variant& r_value) const {
    if (p_name.match("target_class")) {
        r_value = _target_class;
        return true;
    } else if (p_name.match("method")) {
        r_value = DictionaryUtils::from_method(_method);
        return true;
    }
    return false;
}

bool OScriptNodeEmitMemberSignal::_set(const StringName& p_name, const Variant& p_value) {
    if (p_name.match("target_class")) {
        if (_target_class != p_value) {
            _target_class = p_value;
            _notify_pins_changed();
            return true;
        }
    } else if (p_name.match("method")) {
        _method = DictionaryUtils::to_method(p_value);
        _notify_pins_changed();
        return true;
    }
    return false;
}

void OScriptNodeEmitMemberSignal::post_initialize() {
    // Fixup - always reconstructs; matches function calls
    reconstruct_node();

    super::post_initialize();
}

void OScriptNodeEmitMemberSignal::allocate_default_pins() {
    create_pin(PD_Input, PT_Execution, PropertyUtils::make_exec("ExecIn"));

    Ref<OScriptNodePin> target = create_pin(PD_Input, PT_Data, PropertyUtils::make_object("target", _target_class));
    target->set_label(_target_class + " (Emitter)");
    target->no_pretty_format();

    // Godot signals do not support default values or varargs, no need to be concerned with those
    // They also do not support return values.
    for (const PropertyInfo& pi : _method.arguments) {
        create_pin(PD_Input, PT_Data, pi);
    }

    create_pin(PD_Output, PT_Execution, PropertyUtils::make_exec("ExecOut"));

    super::allocate_default_pins();
}

String OScriptNodeEmitMemberSignal::get_tooltip_text() const {
    return vformat("Emit the %s signal '%s'", _target_class, _method.name);
}

String OScriptNodeEmitMemberSignal::get_node_title() const {
    return vformat("Emit %s", _method.name);
}

String OScriptNodeEmitMemberSignal::get_help_topic() const {
    #if GODOT_VERSION >= 0x040300
    return vformat("class_signal:%s:%s", _target_class, _method.name);
    #else
    return vformat("%s:%s", _target_class, _method.name);
    #endif
}

void OScriptNodeEmitMemberSignal::initialize(const OScriptNodeInitContext& p_context) {
    ERR_FAIL_COND_MSG(!p_context.user_data, "Failed to initialize an EmitMemberSignal");
    ERR_FAIL_COND_MSG(!p_context.method, "Failed to iniitialize an EmitMemberSignal, method info required.");

    const Dictionary& data = p_context.user_data.value();
    ERR_FAIL_COND_MSG(!data.has("target_class"), "Failed to initialize an EmitMemberSignal without target class.");

    _target_class = data["target_class"];
    _method = p_context.method.value();

    super::initialize(p_context);
}

void OScriptNodeEmitMemberSignal::validate_node_during_build(BuildLog& p_log) const {
    if (_target_class.is_empty()) {
        p_log.error(this, "No target class defined.");
        return;
    }

    if (_method.name.is_empty()) {
        p_log.error(this, "No method defined");
        return;
    }

    const Ref<OScriptNodePin> target_pin = find_pin("target", PD_Input);
    if (!target_pin.is_valid()) {
        p_log.error(this, "Failed to find target pin");
        return;
    }

    const Vector<Ref<OScriptNodePin>> connections = target_pin->get_connections();
    if (connections.is_empty()) {
        // If the node isn't connected on its execution input pin; safe to ignore this.
        Ref<OScriptNodePin> exec_in = find_pin("ExecIn", PD_Input);
        if (!(exec_in.is_valid() && exec_in->get_connections().is_empty())) {
            if (!ClassDB::class_has_signal(get_orchestration()->get_base_type(), _method.name)) {
                p_log.error(this, vformat("No signal found in %s with name: %s", get_orchestration()->get_orchestration_path(), _method.name));
            }
        }
    } else {
        const Ref<OScriptTargetObject> target = connections[0]->resolve_target();
        if (target.is_null()) {
            p_log.error(this, "No target object resolved");
        } else if (!target->get_target()->has_signal(_method.name)) {
            p_log.error(this, vformat("No signal found on target with method name: %s", _method.name));
        }
    }
}
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
#include "orchestration/pin_layout.h"

void OScriptPinLayout::add_row(const Ref<OScriptNodePin>& p_left, const Ref<OScriptNodePin>& p_right) {
    Row row;
    row.left = p_left;
    row.right = p_right;
    _rows.push_back(row);
}

void OScriptPinLayout::add_input_row(const Ref<OScriptNodePin>& p_left) {
    add_row(p_left, {});
}

void OScriptPinLayout::add_output_row(const Ref<OScriptNodePin>& p_right) {
    add_row({}, p_right);
}

void OScriptPinLayout::add_empty_row() {
    add_row({}, {});
}

void OScriptPinLayout::add_default_rows(const Vector<Ref<OScriptNodePin>>& p_inputs, const Vector<Ref<OScriptNodePin>>& p_outputs) {
    const int64_t count = Math::max(p_inputs.size(), p_outputs.size());
    for (int64_t i = 0; i < count; i++) {
        add_row(i < p_inputs.size() ? p_inputs[i] : Ref<OScriptNodePin>(), i < p_outputs.size() ? p_outputs[i] : Ref<OScriptNodePin>());
    }
}

void OScriptPinLayout::clear() {
    _rows.clear();
}
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
#pragma once

#include "orchestration/node_pin.h"

#include <godot_cpp/templates/vector.hpp>

using namespace godot;

/// Describes how a node's pins are arranged into editor rows.
///
/// A layout is a transient description a node produces on demand; it is never serialized. Each row
/// holds at most one input on the left and one output on the right, and either side may be empty.
/// Rows are authored against a node's logical pins. The editor is responsible for placing whatever
/// slot pins a logical pin stands for, so a node never needs to know whether a pin is split.
///
/// Every input and output of the node must appear exactly once, in the same order the node declares
/// them. Rows only space pins apart; they never reorder a side, which keeps editor ports aligned with
/// the pin indices that connections store.
///
class OScriptPinLayout {
public:
    struct Row {
        Ref<OScriptNodePin> left;   //! The input on this row, may be invalid
        Ref<OScriptNodePin> right;  //! The output on this row, may be invalid
    };

private:
    Vector<Row> _rows;  //! Rows in top-to-bottom order

public:
    /// Adds a row holding an input and an output.
    /// @param p_left the input pin
    /// @param p_right the output pin
    void add_row(const Ref<OScriptNodePin>& p_left, const Ref<OScriptNodePin>& p_right);

    /// Adds a row holding only an input.
    /// @param p_left the input pin
    void add_input_row(const Ref<OScriptNodePin>& p_left);

    /// Adds a row holding only an output.
    /// @param p_right the output pin
    void add_output_row(const Ref<OScriptNodePin>& p_right);

    /// Adds a row with no pins, acting as a visual separator.
    void add_empty_row();

    /// Adds the default rows: input i beside output i, with the longer side finishing alone.
    /// @param p_inputs the node's inputs in declaration order
    /// @param p_outputs the node's outputs in declaration order
    void add_default_rows(const Vector<Ref<OScriptNodePin>>& p_inputs, const Vector<Ref<OScriptNodePin>>& p_outputs);

    /// Get the rows in top-to-bottom order.
    /// @return the rows
    const Vector<Row>& get_rows() const { return _rows; }

    /// Removes all rows.
    void clear();
};
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
#include "graph_node_default.h"

#include "common/scene_utils.h"
#include "editor/graph/pins/graph_node_pin_factory.h"

#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

OrchestratorGraphNodeDefault::OrchestratorGraphNodeDefault(OrchestratorGraphEdit* p_graph, const Ref<OScriptNode>& p_node)
    : OrchestratorGraphNode(p_graph, p_node)
{
    set_mouse_filter(MOUSE_FILTER_STOP);
}

void OrchestratorGraphNodeDefault::_bind_methods()
{
}

void OrchestratorGraphNodeDefault::_update_pins()
{
    // Get previous row count
    int prev_rows = _get_previous_row_count();

    // Get current inputs/outputs
    const Vector<Ref<OScriptNodePin>> inputs = get_script_node()->find_pins(PD_Input);
    const Vector<Ref<OScriptNodePin>> outputs = get_script_node()->find_pins(PD_Output);

    // Calculate the max rows used by the node
    int max_rows = Math::max(inputs.size(), outputs.size());

    // Remove all pins
    if (is_inside_tree() && get_child_count() > 0)
    {
        if (prev_rows > max_rows)
        {
            // So currently, pin indices are calculated dynamically and this creates problems here
            // because if we were to use OrchestratorGraphNodePin::unlink_all, it will recalculate the
            // pin index and find it's -1 as the pin resource has already been removed from the
            // node resource at this point. Instead, we need to rely on the UI index
            // We cannot rely on OrchestratorGraphNodePin->unlink_all() in this code block because
            // the pin resource has been removed already and all that remains is the editor
            for (int index = max_rows; index < prev_rows; index++)
            {
                OrchestratorGraphNodePin* input = _pin_rows[index].left;
                if (input)
                    input->unlink_all();

                OrchestratorGraphNodePin* output = _pin_rows[index].right;
                if (output)
                    output->unlink_all();
            }
        }

        _pin_rows.clear();

        TypedArray<Node> nodes = get_children();
        for (int i = 0; i < nodes.size(); i++)
        {
            Node* child = Object::cast_to<Node>(nodes[i]);
            if (!child)
                continue;

            remove_child(child);
            child->queue_free();
        }

        clear_all_slots();
    }

    // If there are no pins defined, nothing to do
    if (inputs.is_empty() && outputs.is_empty())
        return;

    // Create each row
    for (int row_index = 0; row_index < max_rows; row_index++)
    {
        // Create the new row
        Row row;
        row.index = row_index;

        if (row_index < inputs.size())
        {
            const Ref<OScriptNodePin>& pin = inputs[row_index];
            row.left = OrchestratorGraphNodePinFactory::create_pin(this, pin);
        }

        if (row_index < outputs.size())
        {
            const Ref<OScriptNodePin>& pin = outputs[row_index];
            row.right = OrchestratorGraphNodePinFactory::create_pin(this, pin);
        }

        _pin_rows[row_index] = row;
        _create_row_widget(row);
    }

    real_t max_left_width{ 0 };
    real_t max_right_width{ 0 };
    for (int row_index = 0; row_index < max_rows; row_index++)
    {
        if (_pin_rows[row_index].left)
            max_left_width = Math::max(_pin_rows[row_index].left->get_size().x, max_left_width);
        if (_pin_rows[row_index].right)
            max_right_width = Math::max(_pin_rows[row_index].right->get_size().x, max_right_width);
    }
    for (int row_index = 0; row_index < max_rows; row_index++)
    {
        if (_pin_rows[row_index].left)
        {
            _pin_rows[row_index].left->set_custom_minimum_size(Vector2(max_left_width, 0));
            _pin_rows[row_index].left->set_alignment(BoxContainer::ALIGNMENT_BEGIN);
        }
        if (_pin_rows[row_index].right)
        {
            _pin_rows[row_index].right->set_custom_minimum_size(Vector2(max_right_width, 0));
            _pin_rows[row_index].right->set_alignment(BoxContainer::ALIGNMENT_END);
        }
    }

    OrchestratorGraphNode::_update_pins();
}

void OrchestratorGraphNodeDefault::_create_row_widget(Row& p_row)
{
    HBoxContainer* container = memnew(HBoxContainer);
    container->set_h_size_flags(Control::SIZE_FILL);
    p_row.widget = container;

    if (p_row.left)
        container->add_child(p_row.left);

    VBoxContainer* middle = memnew(VBoxContainer);
    middle->set_custom_minimum_size(Vector2(15, 0));
    middle->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    container->add_child(middle);

    if (p_row.right)
        container->add_child(p_row.right);

    container->connect("ready", callable_mp(this, &OrchestratorGraphNodeDefault::_on_row_ready).bind(p_row.index),
                       CONNECT_ONE_SHOT);

    add_child(container);
}

void OrchestratorGraphNodeDefault::_on_row_ready(int p_row_index)
{
    if (_pin_rows.has(p_row_index))
    {
        const Row& row = _pin_rows[p_row_index];

        const Ref<Texture2D> invalid = SceneUtils::get_editor_icon("GuiGraphNodePort");

        set_slot(row.index,
                 row.left && row.left->is_connectable() && !row.left->is_hidden(),
                 row.left ? row.left->get_slot_type() : 0,
                 row.left ? row.left->get_color() : Color(0, 0, 0, 1),
                 row.right && row.right->is_connectable() && !row.right->is_hidden(),
                 row.right ? row.right->get_slot_type() : 0,
                 row.right ? row.right->get_color() :Color(0, 0, 0, 1),
                 row.left ? SceneUtils::get_editor_icon(row.left->get_slot_icon_name()) : invalid,
                 row.right ? SceneUtils::get_editor_icon(row.right->get_slot_icon_name()) : invalid);
    }
}

int OrchestratorGraphNodeDefault::_get_previous_row_count()
{
    int prev_row_count = 0;
    for (const KeyValue<int, Row>& E : _pin_rows)
        prev_row_count = Math::max(prev_row_count, E.key + 1);
    return prev_row_count;
}

OrchestratorGraphNodePin* OrchestratorGraphNodeDefault::get_input_pin(int p_port)
{
    ERR_FAIL_COND_V_MSG(p_port < 0, nullptr, "Port must be greater-than or equal to 0.");
    ERR_FAIL_COND_V_MSG(p_port >= int(_pin_rows.size()), nullptr, "Failed to find row for slot " + itos(p_port));
    return _pin_rows[get_input_port_slot(p_port)].left;
}

OrchestratorGraphNodePin* OrchestratorGraphNodeDefault::get_output_pin(int p_port)
{
    ERR_FAIL_COND_V_MSG(p_port < 0, nullptr, "Port must be greater-than or equal to 0.");
    ERR_FAIL_COND_V_MSG(p_port >= int(_pin_rows.size()), nullptr, "Failed to find row for slot " + itos(p_port));
    return _pin_rows[get_output_port_slot(p_port)].right;
}

void OrchestratorGraphNodeDefault::show_icons(bool p_visible)
{
    for (const KeyValue<int, Row>& E : _pin_rows)
    {
        if (E.value.left)
            E.value.left->show_icon(p_visible);

        if (E.value.right)
            E.value.right->show_icon(p_visible);
    }
}
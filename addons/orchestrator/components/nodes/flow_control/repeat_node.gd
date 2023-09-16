@tool
extends OrchestrationNode

var _iterations : int = 0

func _init():
	type = 7
	name = "Repeat"
	category = "Flow Control"
	description = "Executes the repetitions output the specified number of times."


func execute(context: OrchestrationExecutionContext) -> Variant:
	# If the node has no attributes, there is no way to repeat.
	# In this case, we advance beyond the node to the next step
	if not context.has_attribute("iterations"):
		return context.get_output_target_node_id(1)

	var iterations = context.get_attribute("iterations", 0)
	if not iterations or iterations <= 0:
		return 0

	var remaining_iterations = context.get_state("iterations", iterations)
	if remaining_iterations == 0:
		context.remove_state()
		return context.get_output_target_node_id(1)

	context.set_state("iterations", remaining_iterations - 1)
	# context.editor_print("Iteration: %s" % (iterations - remaining_iterations + 1))
	return context.get_output_target_node_id(0)


func get_attributes() -> Dictionary:
	return { "iterations": _iterations }


func create_ui(attributes: OrchestratorDictionary, scene_node: Node) -> void:
	add_input_slot(0, 0)
	add_output_slot(0, 1)
	add_output_slot(1, 2)

	_iterations = attributes.get_default("iterations", 0)

	var margin1 = MarginContainer.new()
	margin1.add_theme_constant_override("margin_top", 8)

	var spinBox = SpinBox.new()
	spinBox.min_value = 1
	spinBox.max_value = 10
	spinBox.allow_lesser = false
	spinBox.allow_greater = true
	spinBox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	spinBox.custom_minimum_size = Vector2(175, 0)
	spinBox.value_changed.connect(func(new_value:float): _iterations = new_value)
	spinBox.value = _iterations
	margin1.add_child(spinBox)
	scene_node.add_child(margin1)

	var label = Label.new()
	label.text = "repetitions"
	label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT

	var row = HBoxContainer.new()
	row.alignment = BoxContainer.ALIGNMENT_END
	row.add_child(label)
	scene_node.add_child(row)

	label = Label.new()
	label.text = "done"
	label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT

	row = HBoxContainer.new()
	row.alignment = BoxContainer.ALIGNMENT_END
	row.add_child(label)

	scene_node.add_child(row)

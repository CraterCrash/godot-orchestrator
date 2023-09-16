@tool
extends OrchestrationNode

var _expression : String = ""

func _init():
	type = 5
	name = "Condition"
	category = "Flow Control"
	description = "Evaluates the specified condition as a boolean and takes the appropriate path."


func execute(context: OrchestrationExecutionContext) -> Variant:
	context.require_attribute("condition")

	var result = Orchestrator.evaluate(context, context.get_attribute("condition", false))
	if result:
		# context.editor_print("Output -> true")
		return context.get_output_target_node_id(0)

	# context.editor_print("Output -> false")
	return context.get_output_target_node_id(1)


func get_attributes() -> Dictionary:
	return { "condition": _expression }


func create_ui(attributes: OrchestratorDictionary, scene_node: Node) -> void:
	add_input_slot(0, 0)
	add_output_slot(0, 1)
	add_output_slot(1, 2)

	_expression = attributes.get_default("condition", "")

	var margin = MarginContainer.new()
	margin.add_theme_constant_override("margin_top", 8)

	var condition = LineEdit.new()
	condition.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	condition.custom_minimum_size = Vector2(256, 0)
	condition.text_changed.connect(func(new_value: String): _expression = new_value)
	condition.text = _expression

	var label = Label.new()
	label.text = "if"

	var row = HBoxContainer.new()
	row.add_child(label)
	row.add_child(condition)
	margin.add_child(row)
	scene_node.add_child(margin)

	var true_label = Label.new()
	true_label.text = "true"
	true_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT

	row = HBoxContainer.new()
	row.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	row.alignment = BoxContainer.ALIGNMENT_END
	row.add_child(true_label)
	scene_node.add_child(row)

	var false_label = Label.new()
	false_label.text = "false"
	false_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT

	row = HBoxContainer.new()
	row.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	row.alignment = BoxContainer.ALIGNMENT_END
	row.add_child(false_label)

	scene_node.add_child(row)

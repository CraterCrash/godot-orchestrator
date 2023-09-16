@tool
extends OrchestrationNode

var _chance := 0

func _init():
	type = 4
	name = "Chance"
	category = "Flow Control"
	description = "Calculates a chance between 0% and 100% and takes appropriate path."


func execute(context: OrchestrationExecutionContext) -> Variant:
	context.require_attribute("chance")

	var chance = context.get_attribute("chance", 0)
	var result = randi_range(1, 100)
	# context.editor_print("Calculated chance: %s" % result)

	if result <= chance:
		return context.get_output_target_node_id(0)
	return context.get_output_target_node_id(1)


func get_attributes() -> Dictionary:
	return { "chance": _chance }


func create_ui(attributes: OrchestratorDictionary, scene_node: Node) -> void:
	add_input_slot(0, 0)
	add_output_slot(0, 1)
	add_output_slot(1, 2)

	_chance = attributes.get_default("chance", 0)

	var margin = MarginContainer.new()
	margin.add_theme_constant_override("margin_top", 8)

	var slider = HSlider.new()
	slider.min_value = 0
	slider.max_value = 100
	slider.step = 1
	slider.scrollable = true
	slider.rounded = true
	slider.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	slider.custom_minimum_size = Vector2(175, 0)

	margin.add_child(slider)
	scene_node.add_child(margin)

	var chance = Label.new()
	chance.text = "0%"
	chance.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT

	var row = HBoxContainer.new()
	row.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	row.alignment = BoxContainer.ALIGNMENT_END
	row.add_child(chance)
	scene_node.add_child(row)

	var delta = Label.new()
	delta.text = "100%"
	delta.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT

	row = HBoxContainer.new()
	row.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	row.alignment = BoxContainer.ALIGNMENT_END
	row.add_child(delta)
	scene_node.add_child(row)

	slider.value_changed.connect(func(new_value: float):
		chance.text = str(new_value) + "%"
		delta.text = str(100 - new_value) + "%"
		_chance = new_value
	)

	slider.value = _chance

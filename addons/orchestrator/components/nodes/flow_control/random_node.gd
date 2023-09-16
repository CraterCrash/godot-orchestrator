@tool
extends OrchestrationNode

var _possibilities : int = 1

func _init():
	type = 6
	name = "Random"
	category = "Flow Control"
	description = "Picks a random output path."


func execute(context: OrchestrationExecutionContext) -> Variant:
	context.require_attribute("possibilities")

	var possibilities = context.get_attribute("possibilities", 0)
	if not possibilities:
		return context.get_output_target_node_id(0)

	var choice = randi_range(1, possibilities)
	# context.editor_print("Random Choice -> %s" % choice)
	return context.get_output_target_node_id(choice - 1)


func get_attributes() -> Dictionary:
	return { "possibilities" : _possibilities }


func create_ui(attributes: OrchestratorDictionary, scene_node: Node) -> void:
	add_input_slot(0, 0)
	add_output_slot(0, 2)

	_possibilities = attributes.get_default("possibilities", 0)

	var margin1 = MarginContainer.new()
	margin1.add_theme_constant_override("margin_top", 8)
	scene_node.add_child(margin1)

	var slider = HSlider.new()
	slider.min_value = 1
	slider.max_value = 10
	slider.step = 1
	slider.scrollable = true
	slider.rounded = true
	slider.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	slider.custom_minimum_size = Vector2(175, 0)
	margin1.add_child(slider)

	var label = Label.new()
	label.text = "Possibilities: %s" % _possibilities
	scene_node.add_child(label)

	var row = HBoxContainer.new()
	row.add_child(Label.new())
	scene_node.add_child(row)

	# Event Listeners
	slider.value_changed.connect(func(new_value: float):
		_possibilities = int(new_value)
		label.text = "Possibilities: %s" % _possibilities
		_create_slots_from_slider(scene_node)
	)

	slider.value = _possibilities


func _create_slots_from_slider(scene_node: Node) -> void:
	var children = scene_node.get_child_count()
	var delta = _possibilities - (children - 2)
	if delta > 0:
		var slot_index = children
		while delta > 0:
			_add_slot_row(scene_node)
			add_output_slot(slot_index - 2, slot_index)
			slot_index += 1
			delta -= 1
	elif delta < 0:
		var slot_index = children - 1
		while delta < 0:
			_remove_slot_row(scene_node)
			remove_output_slot(slot_index)
			slot_index -= 1
			delta += 1

	scene_node.size.y = 0


func _add_slot_row(scene_node: Node) -> void:
	var row = HBoxContainer.new()
	row.add_child(Label.new())
	scene_node.add_child(row)
	await scene_node.get_tree().process_frame


func _remove_slot_row(scene_node: Node) -> void:
	var child = scene_node.get_child(-1)
	scene_node.remove_child(child)
	child.queue_free()
	await scene_node.get_tree().process_frame

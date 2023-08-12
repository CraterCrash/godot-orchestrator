## An orchestration node resource that represents wait node.
@tool
extends OrchestrationNode

var _wait_time : int = 0

func _init():
	type = 13
	name = "Wait"
	category = "Logic"
	description = "Waits for a given number of seconds."


func execute(context: OrchestrationExecutionContext) -> Variant:
	var wait_time = context.get_attribute("wait_time", 0)
	if wait_time > 0:
		await Orchestrator.get_tree().create_timer(wait_time).timeout
	return context.get_output_target_node_id(0)


func get_attributes() -> Dictionary:
	return { "wait_time" : _wait_time }


func create_ui(attributes: OrchestratorDictionary, scene_node: Node) -> void:
	add_input_slot(0, 0)
	add_output_slot(0, 0)

	_wait_time = attributes.get_default("wait_time", 0)

	var margin = MarginContainer.new()
	margin.add_theme_constant_override("margin_top", 8)
	margin.add_theme_constant_override("margin_bottom", 8)

	var spinbox = SpinBox.new()
	spinbox.min_value = 1
	spinbox.max_value = 60
	spinbox.value = 1
	spinbox.step = 1
	spinbox.allow_greater = false
	spinbox.allow_lesser = false
	spinbox.rounded = true
	spinbox.size_flags_horizontal = Control.SIZE_FILL
	spinbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	spinbox.value_changed.connect(func(new_value:float): _wait_time = new_value)
	spinbox.value = _wait_time

	var label = Label.new()
	label.text = "seconds"

	var container = HBoxContainer.new()
	container.add_child(spinbox)
	container.add_child(label)
	margin.add_child(container)

	scene_node.add_child(margin)


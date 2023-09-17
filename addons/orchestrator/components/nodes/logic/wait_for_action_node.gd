## An orchestration node resource that represents the wait for action node.
@tool
extends OrchestrationNode

var _action_name : String

func _init():
	type = 14
	name = "Wait for Action"
	category = "Logic"
	description = "Waits for a given InputEventAction."


func execute(context: OrchestrationExecutionContext) -> Variant:
	var action_name = context.get_attribute("action_name")
	while not Input.is_action_just_pressed(action_name):
		await Orchestrator.get_tree().process_frame
	await Orchestrator.get_tree().process_frame
	return context.get_output_target_node_id(0)


func get_attributes() -> Dictionary:
	return {"action_name" : _action_name }


func create_ui(attributes: OrchestratorDictionary, scene_node: Node) -> void:
	add_input_slot(0, 0)
	add_output_slot(0, 0)
	_action_name = attributes.get_default("_action_name", "")

	var margin = MarginContainer.new()
	margin.add_theme_constant_override("margin_top", 8)
	margin.add_theme_constant_override("margin_bottom", 8)

	var input_event_action_name = LineEdit.new()
	input_event_action_name.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	input_event_action_name.custom_minimum_size = Vector2(128, 0)
	input_event_action_name.text_changed.connect(func(new_value: String): _action_name = new_value)
	input_event_action_name.text = _action_name

	var label = Label.new()
	label.text = "if InputEventAction.justpressed"

	var row = HBoxContainer.new()
	row.add_child(label)
	row.add_child(input_event_action_name)
	margin.add_child(row)
	scene_node.add_child(margin)


@tool
extends OrchestrationNode

func _init():
	type = 2
	name = "End"
	category = "Terminal"
	description = "Ends the orchestation."
	allow_multiple = true


func execute(context: OrchestrationExecutionContext) -> Variant:
	return END_EXECUTION


func create_ui(attributes: OrchestratorDictionary, scene_node: Node) -> void:
	add_input_slot(0, 0)

	var container = HBoxContainer.new()
	container.alignment = BoxContainer.ALIGNMENT_BEGIN
	container.mouse_filter = Control.MOUSE_FILTER_PASS


	var label = Label.new()
	label.text = "End"
	label.horizontal_alignment = HORIZONTAL_ALIGNMENT_LEFT
	container.add_child(label)

	scene_node.add_child(container)

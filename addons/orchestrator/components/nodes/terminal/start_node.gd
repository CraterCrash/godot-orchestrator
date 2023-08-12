## An orchestration node resource that represents a terminal start node.
@tool
extends OrchestrationNode

func _init():
	type = 1
	name = "Start"
	category = "Terminal"
	description = "Starts an orchestration."
	allow_multiple = false


func execute(context: OrchestrationExecutionContext) -> Variant:
	return context.get_output_target_node_id(0)


func create_ui(attributes: OrchestratorDictionary, scene_node: Node) -> void:
	add_output_slot(0, 0)

	var container = HBoxContainer.new()
	container.alignment = BoxContainer.ALIGNMENT_END
	container.mouse_filter = Control.MOUSE_FILTER_PASS

	var label = Label.new()
	label.text = "Start"
	label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	container.add_child(label)

	scene_node.add_child(container)

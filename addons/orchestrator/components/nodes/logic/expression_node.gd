@tool
extends OrchestrationNode

var _expression : String = ""

func _init():
	type = 10
	name = "Expression"
	category = "Logic"
	description = "Evaluates and executes the expression."


func execute(context: OrchestrationExecutionContext) -> Variant:
	context.require_attribute("expression")
	var result = Orchestrator.evaluate(context, context.get_attribute("expression"))
	return context.get_output_target_node_id(0)


func get_attributes() -> Dictionary:
	return { "expression" : _expression }


func create_ui(attributes: OrchestratorDictionary, scene_node: Node) -> void:
	add_input_slot(0, 0)
	add_output_slot(0, 0)

	_expression = attributes.get_default("expression", "")

	var expression = LineEdit.new()
	expression.custom_minimum_size = Vector2(256, 0)
	expression.text_changed.connect(func(new_expression:String): _expression = new_expression)
	expression.text = _expression

	var margin = MarginContainer.new()
	margin.add_theme_constant_override("margin_top", 8)
	margin.add_theme_constant_override("margin_bottom", 8)
	margin.add_child(expression)

	scene_node.add_child(margin)


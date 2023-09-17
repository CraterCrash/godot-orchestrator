## Singleton that provides runtime execution of Orchestrations
##
## [Orchestration] resources provide scripted behavior that can be used by the
## [Orchestrator] autoload singleton during game-play.
##
extends Node

## Emitted when an orchestration has started.
signal orchestration_started
## Emitted when an orchestration has finished its execution.
signal orchestration_finished

@onready var _node_factory = get_node("/root/OrchestratorNodeFactory")


func _ready():
	Engine.register_singleton("Orchestrator", self)
	_node_factory.rescan_for_resources()


## Executes the provided orchestration
func execute(resource: Orchestration) -> void:
	var state = {}

	var data = _get_start_node_data(resource)
	if data.is_empty():
		printerr("Failed to find start node in orchestration, terminated.")
		return

	# Because OrchestratorNodeFactory awaits on the process frame, we have to
	# await on the same spot here to avoid there being a loading issue and
	# the factory not yet having scanned the nodes.
	await get_tree().process_frame

	var node = _node_factory.get_node_resource(data["type"])

	var context = OrchestrationExecutionContext.new()
	context._data = data
	context._state = {}
	context._orchestration = resource
	context._locals = _get_locals()

	orchestration_started.emit()

	while is_instance_valid(node):
		# context.editor_print("Processing %s with node %s [%s]" % [data["name"], node.name, node])
		var next_node_id = await node.execute(context)
		# context.editor_print("Target node id: %s" % next_node_id)
		free_node(node)
		node = null

		if ((next_node_id == null)
			or (typeof(next_node_id) == TYPE_INT and next_node_id <= 0)
			or (typeof(next_node_id) == TYPE_STRING_NAME and next_node_id.is_empty())):
			break

		data = _get_node_data(next_node_id, resource)
		context._data = data

		node = _node_factory.get_node_resource(data["type"])

	context.editor_print("Orchestration execution completed.")
	orchestration_finished.emit()


## Evaluates the provided GDScript code block, which can be an expression or a block
## of code, returning the outcome of the expression, if a result is available.
func evaluate(context: OrchestrationExecutionContext, user_expression: String) -> Variant:
	var locals = (context.get_locals() if context else _get_locals())
	var expression = Expression.new()
	var err = expression.parse(user_expression, PackedStringArray(locals.keys()))
	if err != OK:
		push_warning(expression.get_error_text())
		return false

	var result = expression.execute(locals.values())
	if expression.has_execute_failed():
		push_error("Expression '%s' failed: %s" % [user_expression, expression.get_error_text()])
		return false

	return result


func _get_locals() -> Dictionary:
	var locals = {}
	var singletons = Engine.get_singleton_list()
	for singleton in singletons:
		locals[singleton] = Engine.get_singleton(singleton)

	var config = ConfigFile.new()
	config.load("res://project.godot")
	if config.has_section("autoload"):
		for key in config.get_section_keys("autoload"):
			locals[key] = get_tree().root.find_child(key, false, false)

	return locals


func _get_start_node_data(orchestration: Orchestration) -> Dictionary:
	for node in orchestration.nodes:
		if node["type_name"] == "Start":
			return node
	return {}


func _get_node_data(node_id: Variant, orchestration: Orchestration) -> Dictionary:
	for node in orchestration.nodes:
		if node["id"] == node_id:
			return node
	return {}


static func free_node(node) -> void:
	if node is Node:
		node.queue_free()

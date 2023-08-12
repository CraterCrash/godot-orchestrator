@tool
class_name OrchestrationGraphNode
extends GraphNode
## Uses an OrchestrationNode resource and creates a GraphNode UI widget.

## The orchestration node resource linked to this graph node.
var orchestration_node : OrchestrationNode : set = set_orchestration_node

var _attributes : OrchestratorDictionary


func _ready():
	# All nodes are resizable
	resizable = true
	show_close = true

	# Setup callbacks
	position_offset_changed.connect(_on_position_offset_changed)
	resize_request.connect(_on_resize_request)
	close_request.connect(_on_close_request)


func set_attributes(attributes: OrchestratorDictionary) -> void:
	_attributes = attributes


## Get the associated [code]OrchestrationNode[/code] resource.
func get_orchestration_node() -> OrchestrationNode:
	orchestration_node.id = name
	return orchestration_node


## set the associated [code]OrchestrationNode[/code] resource for this graph node.
func set_orchestration_node(node: OrchestrationNode) -> void:
	# Set the orchestration node reference
	orchestration_node = node

	# Before doing anything else, we need to wire up to the various signals
	# emitted by the orchestration node so that the graph node can be kept
	# sychronized.
	orchestration_node.input_slot_added.connect(_on_input_slot_added)
	orchestration_node.input_slot_removed.connect(_on_input_slot_removed)
	orchestration_node.output_slot_added.connect(_on_output_slot_added)
	orchestration_node.output_slot_removed.connect(_on_output_slot_removed)

	title = node.name
	tooltip_text = node.description

	# Bind the grpah node to the orchestration node
	# This should be done prior to creating any user interface
	orchestration_node._graph_node = self
	orchestration_node._graph_edit = get_parent()


func initialize(attributes: OrchestratorDictionary, node: OrchestrationNode) -> void:
	set_attributes(attributes)
	set_orchestration_node(node)

	# Populate the UI node's widgets
	node.create_ui(attributes, self)

	# Populate slots
	_create_connection_slots()
	_update_styles()


# Get the output connections for a given slot.
func _get_output_connection(slot: int) -> Dictionary:
	var editor = get_parent()
	if editor is GraphEdit:
		for entry in editor.get_connection_list():
			if entry["from"] == name and entry["from_port"] == slot:
				return entry
	return {}


## Disconnects a given connection.
func _disconnect(connection: Dictionary) -> void:
	print("Attempting to disconnect %s" % connection)
	if connection.is_empty():
		return

	var editor = get_parent()
	if editor is GraphEdit:
		editor.disconnect_node(connection["from"], \
			connection["from_port"], \
			connection["to"], \
			connection["to_port"])


# Resets all connection slots
func _reset_connection_slots() -> void:
	for index in get_child_count():
		set_slot(index, false, 0, Color.BLACK, false, 0, Color.BLACK)


# Creates connection slots based on input/output definition metadata
func _create_connection_slots() -> void:
	_reset_connection_slots()

	for index in orchestration_node.inputs:
		var input = orchestration_node.inputs[index]
		set_slot_enabled_left(index, true)
		set_slot_color_left(index, input.color)
		set_slot_type_right(index, input.type)

	for index in orchestration_node.outputs:
		var output = orchestration_node.outputs[index]
		set_slot_enabled_right(index, true)
		set_slot_color_right(index, output.color)
		set_slot_type_right(index, output.type)


## Return whether the specified slot is visible
func is_slot_visible(type: String, index: int) -> bool:
	if not orchestration_node:
		return false
	return true


## Called when a connection request is received
func on_connected(from_port: int, target_node: String, target_port: int) -> void:
	pass


## Called when a connection disconnect request is received
func on_disconnected(from_port: int, target_node: String, target_port: int) -> void:
	pass


# Updates the graph node's styles
func _update_styles() -> void:
	if not orchestration_node:
		return

	var category = orchestration_node.category.to_lower().replacen(" ", "_")

	var frame_name = "res://addons/orchestrator/assets/themes/%s_node.tres" % category
	if FileAccess.file_exists(frame_name):
		var frame = load(frame_name)
		if frame: add_theme_stylebox_override("frame", frame)

	var frame_selected_name = "res://addons/orchestrator/assets/themes/%s_node_selected.tres" % category
	if FileAccess.file_exists(frame_selected_name):
		var frame_selected = load(frame_selected_name)
		if frame_selected: add_theme_stylebox_override("selected_frame", frame_selected)


# Disconnects the output close from this node if its conencted.
func _disconnect_output_port(port: int) -> void:
	for entry in get_parent().get_connection_list():
		if entry["from"] == name and entry["from_port"] == port:
			get_parent().disconnect_node(entry["from"], entry["from_port"], entry["to"], entry["to_port"])


func _on_input_slot_added(slot_index: int, slot: OrchestrationNodeSlot) -> void:
	pass


func _on_input_slot_removed(slot_index: int, slot: OrchestrationNodeSlot) -> void:
	pass


func _on_output_slot_added(slot_index: int, slot: OrchestrationNodeSlot) -> void:
	if not is_slot_enabled_right(slot_index):
		set_slot_enabled_right(slot_index, true)
		set_slot_type_right(slot_index, slot.type)
		set_slot_color_right(slot_index, slot.color)


func _on_output_slot_removed(slot_index: int, slot: OrchestrationNodeSlot) -> void:
	if is_slot_enabled_right(slot_index):
		_disconnect_output_port(slot.port)
		set_slot_enabled_right(slot_index, false)


# Callback when the position offset of the graph node is changed.
func _on_position_offset_changed() -> void:
	pass


# Callback when the size of the grpah node is changed
func _on_resize_request(new_size: Vector2) -> void:
	size = new_size


# Callback when the graph node's close button is clicked
func _on_close_request() -> void:
	queue_free()


@tool
class_name OrchestrationGraphNode
extends GraphNode
## Uses an OrchestrationNode resource and creates a GraphNode UI widget.

const OrchestratorSettings = preload("res://addons/orchestrator/orchestrator_settings.gd")

## The orchestration node resource linked to this graph node.
var orchestration_node : OrchestrationNode : set = set_orchestration_node

var _attributes : OrchestratorDictionary


func _ready():
	# All nodes are resizable
	resizable = true

	if Engine.get_version_info().hex < 0x040200:
		call("set_show_close_button", true)
	else:
		var button = Button.new()
		button.icon = get_theme_icon("Close", "EditorIcons")
		button.flat = true
		get_titlebar_hbox().add_child(button)
		button.position.x += 10
		button.anchors_preset = Control.PRESET_CENTER_RIGHT
		button.pressed.connect(_on_close_request)

	# Setup callbacks
	position_offset_changed.connect(_on_position_offset_changed)
	resize_request.connect(_on_resize_request)


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


func update_project_settings() -> void:
	_update_styles()


# Get the output connections for a given slot.
func _get_output_connection(slot: int) -> Dictionary:
	return get_parent().get_output_connection(name, slot)


## Disconnects a given connection.
func _disconnect(connection: Dictionary) -> void:
	get_parent().disconnect_connection(connection)


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
	var is_42_plus = Engine.get_version_info().hex >= 0x040200

	var category_color = OrchestratorSettings.get_setting("nodes/colors/%s" % category, Color.DEEP_SKY_BLUE)
	var background_color = OrchestratorSettings.get_setting("nodes/colors/background", Color(0.12, 0.15, 0.19))

	# Creates the non-selected panel style
	var style = StyleBoxFlat.new()
	style.bg_color = background_color
	style.set_corner_radius_all(1)
	style.set_border_width_all(2)
	style.border_width_top = 24
	style.border_color = category_color
	style.shadow_color = Color.html("#0000004d")
	style.shadow_size = 2

	if is_42_plus:
		style.border_width_top = 0
		style.corner_radius_top_left = 0
		style.corner_radius_top_right = 0

	# Creates the selected panel style
	var selected_style = style.duplicate()
	selected_style.border_color = Color(category_color.r, category_color.g, category_color.b, 0.45)

	# Handle border/frame styles
	if is_42_plus:
		add_theme_stylebox_override("panel", style)
		add_theme_stylebox_override("panel_selected", selected_style)
	else:
		add_theme_stylebox_override("frame", style)
		add_theme_stylebox_override("selected_frame", selected_style)

	# Handle titlebar styles text
	if is_42_plus:
		# Create the titlebar style
		var titlebar_style = style.duplicate()
		titlebar_style.set_corner_radius_all(1)
		titlebar_style.corner_radius_bottom_left = 0
		titlebar_style.corner_radius_bottom_right = 0
		titlebar_style.bg_color = titlebar_style.border_color
		titlebar_style.shadow_size = 0
		add_theme_stylebox_override("titlebar", titlebar_style)

		# Create the selected titlebar style
		var selected_titlebar_style = titlebar_style.duplicate()
		selected_titlebar_style.border_color.a = 0.45
		selected_titlebar_style.bg_color.a = 0.45
		add_theme_stylebox_override("titlebar_selected", selected_titlebar_style)

		# Setup titlebar font
		var titlebar_label = get_titlebar_hbox().get_children()[0] as Label
		titlebar_label.add_theme_color_override("font_color", Color.WHITE)

		# Hack to adjust the title label to be offset like close icon
		titlebar_label.text = " " + titlebar_label.text.trim_prefix(" ")


# Disconnects the output close from this node if its conencted.
func _disconnect_output_port(port: int) -> void:
	get_parent().disconnect_output_port(name, port)


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

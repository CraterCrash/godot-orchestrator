## An orchestration node resource that represents script execution node.
@tool
extends OrchestrationNode

var _method_name : String
var _script : String

func _init():
	type = 11
	name = "Script"
	category = "Logic"
	description = "Executes a specific script function"


func execute(context: OrchestrationExecutionContext) -> Variant:
	context.require_attribute("method_name")
	context.require_attribute("file_name")
	_eval_script(context)
	return context.get_output_target_node_id(0)


func get_attributes() -> Dictionary:
	return { "method_name" : _method_name, "file_name": _script }


func create_ui(attributes: OrchestratorDictionary, scene_node: Node) -> void:
	add_input_slot(0, 0)
	add_output_slot(0, 0)

	_script = attributes.get_default("file_name", "")
	_method_name = attributes.get_default("method_name", "")

	# Holds reference to the file name
	var file_name = LineEdit.new()
	file_name.editable = false
	file_name.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	file_name.custom_minimum_size = Vector2(256, 0)

	# Button to open dialog
	var open_button = Button.new()
	open_button.icon = load("res://addons/orchestrator/assets/icons/FindScript.svg")

	var row = HBoxContainer.new()
	row.add_child(file_name)
	row.add_child(open_button)

	var margin1 = MarginContainer.new()
	margin1.add_theme_constant_override("margin_top", 8)
	margin1.add_child(row)
	scene_node.add_child(margin1)

	var margin2 = MarginContainer.new()
	margin2.add_theme_constant_override("margin_top", 10)
	scene_node.add_child(margin2)

	var label = Label.new()
	label.text = "Function To Execute:"
	row = HBoxContainer.new()
	row.add_child(label)
	margin2.add_child(row)

	var margin3 = MarginContainer.new()
	margin3.add_theme_constant_override("margin_bottom", 8)
	scene_node.add_child(margin3)

	var func_list = OptionButton.new()
	func_list.disabled = true
	margin3.add_child(func_list)

	# Create the file selection UI
	var file_dialog = FileDialog.new()
	file_dialog.file_mode = FileDialog.FILE_MODE_OPEN_FILE
	file_dialog.title = "Select script"
	file_dialog.add_filter("*.gd", "GDScript files")
	file_dialog.size = Vector2(700, 500)
	file_dialog.initial_position = Window.WINDOW_INITIAL_POSITION_CENTER_MAIN_WINDOW_SCREEN
	scene_node.add_child(file_dialog)

	# Setup event handlers
	open_button.pressed.connect(func():
		file_dialog.show()
	)
	file_dialog.canceled.connect(func():
		file_name.text = ""
		_script = ""
		_method_name = ""
		func_list.clear()
		func_list.disabled = true
	)
	file_dialog.file_selected.connect(func(new_file_name: String):
		file_name.text = new_file_name
		_script = new_file_name
		_populate_function_list(func_list, new_file_name)
	)
	func_list.item_selected.connect(func(index:int):
		_method_name = func_list.get_item_text(index)
	)

	if not _script.is_empty():
		file_name.text = _script
		_populate_function_list(func_list, _script, _method_name)


func _eval_script(context: OrchestrationExecutionContext) -> void:
	var method_name = context.get_attribute("method_name")
	var script_file = context.get_attribute("file_name")
	if not method_name or not script_file:
		return

	var file : FileAccess = FileAccess.open(script_file, FileAccess.READ)
	if not file.is_open():
		return

	var script = GDScript.new()
	script.set_source_code(file.get_as_text())
	script.reload()

	if not script.can_instantiate():
		if script is Node:
			script.queue_free()
		return

	var instance = script.new()
	if script is Node:
		Orchestrator.get_tree().current_scene.add_child(script)

	# context.editor_print("Calling method %s in script %s" % [method_name, script_file])

	instance.call(method_name)

	if script is Node:
		script.queue_free()


func _populate_function_list(dropdown: OptionButton, file_name: String, selected_method_name: String = "") -> void:
	var method_names := []
	var script : GDScript = load(file_name)

	var index = 0
	for method in script.get_script_method_list():
		var method_name = method["name"]
		if not method_names.has(method_name):
			method_names.push_back(method_name)
			dropdown.add_item(method_name)
			if not selected_method_name.is_empty() and method_name == selected_method_name:
				dropdown.select(index)
			index += 1

	dropdown.disabled = method_names.size() == 0

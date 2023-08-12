@tool
extends OrchestrationNode

const ShowMessageScene = preload("res://addons/orchestrator/components/scenes/show_message.tscn")

const LABEL_SIZE = 75

#var _speakers : OptionButton
var _speaker_name : String = ""
var _speaker_type : String = ""
var _recipient_name : String = ""
var _message : String = ""
var _scene_file_name : String = ""
var _choices: Array[ShowMessageChoice]


func _init():
	type = 12
	name = "Show Message"
	category = "Logic"
	description = "Presents a dialog with optional choices to the player."


func execute(context: OrchestrationExecutionContext) -> Variant:
	context.require_attributes(["speaker", "recipient", "message"])

	var speaker = context.get_attribute("speaker")
	var speaker_name = context.get_attribute("speaker_name", "")
	var recipient = context.get_attribute("recipient")
	var message = context.get_attribute("message")
	var choices = context.get_attribute("choices", [])
	var scene_file = context.get_attribute("scene_file_name")

	if speaker == null or recipient == null or message == null:
		return END_EXECUTION

	var node
	if scene_file != null and scene_file != "":
		node = load(scene_file).instantiate()
	else:
		node = ShowMessageScene.instantiate()

	if not node:
		return END_EXECUTION

	Orchestrator.get_tree().current_scene.add_child(node)
	var options = _get_options(context, choices)
	node.show_message(speaker, message, options)

	var selected_index = await node.show_message_finished
	if selected_index >= 0:
		return context.get_output_target_node_id(selected_index)
	else:
		return context.get_output_target_node_id(0)


func get_attributes() -> Dictionary:
	var attributes = {}
	attributes["speaker"] = _speaker_type
	attributes["speaker_name"] = _speaker_name
	attributes["recipient"] = _recipient_name
	attributes["message"] = _message
	attributes["scene_file_name"] = _scene_file_name

	var choices = []
	for choice in _choices:
		var item = {}
		item["condition"] = choice.condition
		item["message"] = choice.message
		choices.append(item)
	attributes["choices"] = choices

	return attributes


func create_ui(attributes: OrchestratorDictionary, scene_node: Node) -> void:

	add_input_slot(0, 0)
	add_output_slot(0, 0)

	_speaker_type = attributes.get_default("speaker", "")
	_speaker_name = attributes.get_default("speaker_name", "")
	_recipient_name = attributes.get_default("recipient", "")
	_message = attributes.get_default("message", "")
	_scene_file_name = attributes.get_default("scene_file_name", "")

	_choices.clear()

	var margin = MarginContainer.new()
	margin.add_theme_constant_override("margin_top", 8)
	margin.add_theme_constant_override("margin_bottom", 8)
	margin.custom_minimum_size = Vector2(300, 0)
	scene_node.add_child(margin)

	var container = VBoxContainer.new()
	container.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	margin.add_child(container)

	var speakers = OptionButton.new()
	speakers.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	speakers.add_item("Narrator")
	speakers.add_item("Character")
	container.add_child(_create_ui_row("Speaker:", speakers))

	var character_name := LineEdit.new()
	character_name.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	character_name.caret_blink = true
	character_name.text_changed.connect(func(new_text:String): _speaker_name = new_text)
	character_name.text = _speaker_name

	var character_name_row = _create_ui_row("Name:", character_name)
	character_name_row.visible = false
	container.add_child(character_name_row)

#	var recipient = OptionButton.new()
#	recipient.size_flags_horizontal = Control.SIZE_EXPAND_FILL
#	recipient.add_item("Character")
#	recipient.add_item("Player")
#	recipient.add_item("Narrator")
#	recipient.item_selected.connect(func(index:int): _recipient_name = recipient.get_item_text(index))
#	container.add_child(_create_ui_row("Recipient:", recipient))

	var scene_container = HBoxContainer.new()
	scene_container.size_flags_horizontal = Control.SIZE_EXPAND_FILL

	var select_scene_dialog = FileDialog.new()
	select_scene_dialog.file_mode = FileDialog.FILE_MODE_OPEN_FILE
	select_scene_dialog.initial_position = Window.WINDOW_INITIAL_POSITION_CENTER_MAIN_WINDOW_SCREEN
	select_scene_dialog.size = Vector2(700, 500)
	select_scene_dialog.title = "Select scene"
	select_scene_dialog.add_filter("*.tscn,*.scn", "Scene files")
	scene_container.add_child(select_scene_dialog)

	var scene_file_name = LineEdit.new()
	scene_file_name.editable = false
	scene_file_name.placeholder_text = "Default scene"
	scene_file_name.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	scene_file_name.focus_mode = Control.FOCUS_NONE
	scene_file_name.text = _scene_file_name
	scene_container.add_child(scene_file_name)

	var scene_dialog_button = Button.new()
	scene_dialog_button.focus_mode = Control.FOCUS_NONE
	scene_dialog_button.icon = load("res://addons/orchestrator/assets/icons/FindScript.svg")
	scene_container.add_child(scene_dialog_button)
	scene_dialog_button.pressed.connect(func(): select_scene_dialog.show())

	var scene_default_button = Button.new()
	scene_default_button.focus_mode = Control.FOCUS_NONE
	scene_default_button.icon = preload("res://addons/orchestrator/assets/icons/Remove.svg")
	scene_container.add_child(scene_default_button)
	scene_default_button.pressed.connect(func():
		_scene_file_name = ""
		scene_file_name.text = _scene_file_name
	)

	container.add_child(_create_ui_row("Scene:", scene_container))

	speakers.item_selected.connect(func(index:int):
		_speaker_type = speakers.get_item_text(index)
		character_name_row.visible = _speaker_type == "Character"
		scene_node.size.y = 0
		if not character_name_row.visible:
			character_name.text = ""
		_speaker_name = character_name.text
	)

	select_scene_dialog.file_selected.connect(func(new_file: String):
		_scene_file_name = new_file
		scene_file_name.text = _scene_file_name
	)
	select_scene_dialog.close_requested.connect(func():
		_scene_file_name = ""
		scene_file_name.text = _scene_file_name
	)

	var label = Label.new()
	label.text = "Message:"
	container.add_child(label)

	var message = TextEdit.new()
	message.placeholder_text = "Your message"
	message.autowrap_mode = TextServer.AUTOWRAP_WORD
	message.wrap_mode = TextEdit.LINE_WRAPPING_BOUNDARY
	message.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	message.custom_minimum_size = Vector2(0, 64)
	message.text_changed.connect(func(): _message = message.text)
	message.text = _message
	container.add_child(message)

	container.add_child(HSeparator.new())

	var choice_button = Button.new()
	choice_button.focus_mode = Control.FOCUS_NONE
	choice_button.icon = load("res://addons/orchestrator/assets/icons/Add.svg")
	choice_button.text = "Add Choice"
	choice_button.size_flags_horizontal = Control.SIZE_SHRINK_BEGIN
	container.add_child(choice_button)
	choice_button.pressed.connect(Callable(_create_choice_row).bind(scene_node))

	_set_option_selection(speakers, _speaker_type, 1)
#	_set_option_selection(recipient, _recipient_name, 1)

	var choices = attributes.get_default("choices", [])
	for choice in choices:
		_create_choice_row(scene_node, choice["condition"], choice["message"])

	_update_output_slots(scene_node)


func _create_ui_row(label_text: String, control: Node) -> Node:
	var row = HBoxContainer.new()
	row.size_flags_horizontal = Control.SIZE_EXPAND_FILL

	var label = Label.new()
	label.text = label_text
	label.custom_minimum_size = Vector2(LABEL_SIZE, 0)
	row.add_child(label)
	row.add_child(control)
	return row


func _create_choice_row(scene_node: Node, cond: String = "", message: String = "") -> void:
	var entry := ShowMessageChoice.new()
	entry.scene_node = scene_node
	_choices.push_back(entry)

	var margin = MarginContainer.new()
	margin.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	scene_node.add_child(margin)
	entry.ui = margin

	var container = HBoxContainer.new()
	container.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	margin.add_child(container)

	var button_up = Button.new()
	button_up.focus_mode = Control.FOCUS_NONE
	button_up.icon = load("res://addons/orchestrator/assets/icons/ArrowUp.svg")
	button_up.disabled = true
	button_up.pressed.connect(Callable(_move_choice_up).bind(entry))
	container.add_child(button_up)

	var button_down = Button.new()
	button_down.focus_mode = Control.FOCUS_NONE
	button_down.icon = load("res://addons/orchestrator/assets/icons/ArrowDown.svg")
	button_down.disabled = true
	button_down.pressed.connect(Callable(_move_choice_down).bind(entry))
	container.add_child(button_down)

	var button_remove = Button.new()
	button_remove.focus_mode = Control.FOCUS_NONE
	button_remove.icon = load("res://addons/orchestrator/assets/icons/Remove.svg")
	button_remove.pressed.connect(Callable(_remove_choice).bind(entry))
	container.add_child(button_remove)

	var condition_enabled = CheckBox.new()
	condition_enabled.focus_mode = Control.FOCUS_NONE
	container.add_child(condition_enabled)

	var label_if = Label.new()
	label_if.text = "if"
	container.add_child(label_if)

	var condition = LineEdit.new()
	condition.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	condition.custom_minimum_size = Vector2(76, 36)
	condition.expand_to_text_length = true
	condition.editable = false
	condition.caret_blink = true
	container.add_child(condition)

	condition_enabled.toggled.connect(func(pressed:bool):
		condition.editable = pressed
		if !pressed:
			condition.text = ""
			entry.condition = ""
	)
	condition.text_changed.connect(func(new_text: String): entry.condition = new_text)

	if cond != null && cond != "":
		condition_enabled.button_pressed = true
		condition.text = cond
		entry.condition = cond

	var label_colon = Label.new()
	label_colon.text = ":"
	container.add_child(label_colon)

	var choice = TextEdit.new()
	choice.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	choice.custom_minimum_size = Vector2(200, 36)
	choice.scroll_fit_content_height = true
	choice.wrap_mode = TextEdit.LINE_WRAPPING_BOUNDARY
	choice.caret_blink = true
	container.add_child(choice)
	choice.text_changed.connect(func(): entry.message = choice.text)

	if message != null && message != "":
		choice.text = message
		entry.message = message

	await scene_node.get_tree().process_frame

	_update_buttons()
	_update_output_slots(scene_node)


func _set_option_selection(option: OptionButton, value: String, default_index) -> void:
	for item in option.item_count:
		if option.get_item_text(item) == value:
			option.select(item)
			option.item_selected.emit(item)
			return
	option.select(default_index)


func _move_choice_up(entry: ShowMessageChoice) -> void:
	var index = _choices.find(entry)
	if entry.scene_node and entry.ui:
		var entry_index = entry.ui.get_index()
		entry.scene_node.move_child(entry.ui, entry_index - 1)
		_move_choice(entry, -1)
		_update_buttons()


func _move_choice_down(entry: ShowMessageChoice) -> void:
	var index = _choices.find(entry)
	if entry.scene_node and entry.ui:
		var entry_index = entry.ui.get_index()
		entry.scene_node.move_child(entry.ui, entry_index + 1)
		_move_choice(entry, 1)
		_update_buttons()


func _remove_choice(entry: ShowMessageChoice) -> void:
	var index = _choices.find(entry)
	_disconnect_slot(index)

	if entry.ui:
		if entry.scene_node:
			entry.scene_node.remove_child(entry.ui)
			entry.scene_node.size.y = 0
		entry.ui.queue_free()

	_choices.erase(entry)

	for i in range(index, _choices.size()):
		_relink_slot(index, index + 1)

	_update_buttons()
	_update_output_slots(entry.scene_node)


func _move_choice(entry: ShowMessageChoice, step: int) -> void:
	var index = _choices.find(entry)
	var other = _choices[index + step]
	_choices[index] = other
	_choices[index + step] = entry
	_swap_slot(index, index + step)


func _update_buttons() -> void:
	for index in range(_choices.size()):
		var choice = _choices[index]
		var buttons = choice.ui.find_children("", "Button", true, false)
		if buttons.size() >= 2:
			buttons[0].disabled = index == 0
			buttons[1].disabled = index == _choices.size() - 1

		choice.ui.add_theme_constant_override("margin_bottom", 0)
		if index == _choices.size() - 1:
			choice.ui.add_theme_constant_override("margin_bottom", 8)


func _update_output_slots(scene_node: GraphNode) -> void:
	if _choices.size() == 0:
		add_output_slot(0, 0)
		return

	remove_output_slot(0)
	for index in range(0, _choices.size()):
		add_output_slot(index, index + 1)


func _is_choice_visible(context: OrchestrationExecutionContext, choice: Dictionary) -> bool:
	if not choice.has("condition"):
		return true

	if not choice["condition"]:
		return true

	return Orchestrator.evaluate(context, choice["condition"])


func _get_options(context: OrchestrationExecutionContext, choices: Array) -> Dictionary:
	var options = {}
	if choices.size() > 0:
		for index in range(0, choices.size()):
			var choice = choices[index]
			if _is_choice_visible(context, choice):
				options[index] = choice["message"]

	return options


func _disconnect_slot(slot: int) -> void:
	_graph_edit.disconnect_output_connection(id, slot)


func _relink_slot(connect_slot: int, disconnect_slot: int) -> void:
	_graph_edit.move_output_connections(id, connect_slot, disconnect_slot)


func _swap_slot(index: int, other_index: int) -> void:
	_graph_edit.swap_output_connections(id, index, other_index)


class ShowMessageChoice:
	var ui: Node
	var scene_node: Node
	var condition: String
	var message: String


@tool
extends Panel

const AUTHORS = "res://addons/orchestrator/AUTHORS.txt"
const LICENSE = "res://addons/orchestrator/LICENSE.txt"

@onready var ok_button = $AboutLayout/ButtonContainer/OkButton
@onready var developers = $AboutLayout/MarginContainer2/TabContainer/Authors/ScrollContainer/MarginContainer/VBoxContainer/PanelContainer3/VBoxContainer
@onready var license = $AboutLayout/MarginContainer2/TabContainer/License/TextEdit
@onready var version = $AboutLayout/MarginContainer/HBoxContainer/VBoxContainer/Label

func _ready():
	var ok_button_handler := func():
		get_parent().close_requested.emit()
	ok_button.pressed.connect(ok_button_handler)
	ok_button.grab_focus()
	visibility_changed.connect(_on_visibility_changed)

	var config = ConfigFile.new()
	config.load("res://addons/orchestrator/plugin.cfg")
	version.text = "Godot Orchestrator v" + config.get_value("plugin", "version")


func _on_visibility_changed() -> void:
	if not visible:
		return

	for node in developers.get_children():
		developers.remove_child(node)

	var authors = FileAccess.open(AUTHORS, FileAccess.READ)
	if authors.is_open():
		while not authors.eof_reached():
			var line = authors.get_line()
			if line.is_empty():
				continue

			var label = Label.new()
			label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
			label.text = line
			developers.add_child(label)
			if not authors.eof_reached():
				developers.add_child(HSeparator.new())

		authors.close()


	var license_text = FileAccess.open(LICENSE, FileAccess.READ)
	if license_text.is_open():
		license.text = license_text.get_as_text()
		license_text.close()


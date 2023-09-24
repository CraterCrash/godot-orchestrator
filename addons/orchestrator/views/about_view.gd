@tool
extends Panel
## The about dialog panel for the Orchestrator plug-in.

const AUTHORS = "res://addons/orchestrator/AUTHORS.txt"
const LICENSE = "res://addons/orchestrator/LICENSE.txt"

const OrchestratorVersion = preload("res://addons/orchestrator/orchestrator_version.gd")


## Called when visibility_changed signal is emitted for the AboutView panel
func _on_visibility_changed() -> void:
	if not visible:
		return

	_populate_developers()
	_populate_license()

	%Version.text = OrchestratorVersion.get_full_version()
	%OkButton.grab_focus()


## Called when the OK button is pressed
func _on_ok_button_pressed():
	get_parent().close_requested.emit()


## Populates the license details in the license tab
func _populate_license() -> void:
	var license_text = FileAccess.open(LICENSE, FileAccess.READ)
	if license_text.is_open():
		%License.text = license_text.get_as_text()
		license_text.close()


## Populates the developer/contributor list in the authors tab
func _populate_developers() -> void:
	%Developers.get_children().map(func(node): %Developers.remove_child(node))

	var authors = FileAccess.open(AUTHORS, FileAccess.READ)
	if not authors.is_open():
		return

	while not authors.eof_reached():
		var line = authors.get_line()
		if not line.is_empty():
			%Developers.add_child(_create_developer_entry(line))
			if not authors.eof_reached():
				%Developers.add_child(HSeparator.new())

	authors.close()


## Creates a developer entry in the authors tab
func _create_developer_entry(developer_name: String) -> Label:
	var label = Label.new()
	label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	label.text = developer_name
	return label

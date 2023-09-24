@tool
extends Control
## A panel that handles the download and installation of the
## plugin updates from the GitHub releases page.

## Emitted when the update fails to be applied.
signal update_failed()

## Emitted when the update succeeds and is applied.
## The new version that was applied is passed as the argument.
signal update_succeeded(version: String)

const TEMP_FILE_NAME = "user://temp.zip"

@onready var label = $VBoxContainer/Label
@onready var http_request = $HTTPRequest
@onready var download_update = $VBoxContainer/CenterContainer/DownloadUpdate


var download_version : Dictionary :
	set(value):
		download_version = value
		label.text = "%s is available for download." % value.tag_name
	get:
		return download_version


func _ready():
	if _is_orchestrator_repository():
		download_update.disabled = true
		download_update.text = "Cannot update local repository"
	else:
		download_update.disabled = false
		download_update.text = "Download update"


func _on_download_update_pressed():
	if _is_orchestrator_repository():
		# Sanity check
		prints("You cannot update the addon from its source repository.")
		update_failed.emit()
		return

	http_request.request(download_version.zipball_url)
	download_update.disabled = true
	download_update.text = "Downloading..."


func _is_orchestrator_repository() -> bool:
	return FileAccess.file_exists("res://docs/modules/ROOT/pages/custom-nodes.adoc")


func _on_http_request_request_completed(result: int, \
										response_code: int, \
										headers: PackedStringArray, \
										body: PackedByteArray) -> void:
	if result != HTTPRequest.RESULT_SUCCESS:
		update_failed.emit()
		return

	# Save zip
	var zip : FileAccess = FileAccess.open(TEMP_FILE_NAME, FileAccess.WRITE)
	zip.store_buffer(body)
	zip.close()

	# Move orchestrator to the recycling bin
	OS.move_to_trash(ProjectSettings.globalize_path("res://addons/orchestrator"))

	var reader : ZIPReader = ZIPReader.new()
	reader.open(TEMP_FILE_NAME)

	var files : PackedStringArray = reader.get_files()

	var base_path
	var index = 0
	for path in files:
		index += 1
		if path.ends_with("/addons/"):
			base_path = path
			break

	for i in range(0, index):
		files.remove_at(0)


	for path in files:
		if path.begins_with(base_path):
			var new_file : String = path.replace(base_path, "")
			if path.ends_with("/"):
				DirAccess.make_dir_recursive_absolute("res://addons/%s" % new_file)
			else:
				var file: FileAccess = FileAccess.open("res://addons/%s" % new_file, FileAccess.WRITE)
				file.store_buffer(reader.read_file(path))

	reader.close()

	DirAccess.remove_absolute(TEMP_FILE_NAME)
	update_succeeded.emit(download_version.tag_name.substr(1))



func _on_release_notes_button_pressed():
	OS.shell_open(download_version.html_url)


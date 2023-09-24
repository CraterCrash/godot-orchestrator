@tool
extends Button
## A simple button that will be shown when a plugin update exists.

## Emitted when the update process requires an editor restart.
signal editor_restart_requested()

const CONFIG_FILE = "res://addons/orchestrator/plugin.cfg"

const OrchestratorVersion = preload("res://addons/orchestrator/orchestrator_version.gd")

@onready var http_request = $HTTPRequest
@onready var timer = $Timer
@onready var plugin_download_dialog = $PluginDownloadDialog
@onready var plugin_download_dialog_panel = $PluginDownloadDialog/PluginDownloadDialogPanel
@onready var plugin_download_failed_dialog = $PluginDownloadFailedDialog
@onready var needs_reload_dialog = $NeedsReloadDialog


func _ready() -> void:
	# Hidden by default, only shown if updates found.
	hide()

	_apply_theme()
	_check_for_updates()

	# Check periodically for updates (every 12 hours)
	timer.start(60 * 60 * 12)


func _apply_theme() -> void:
	var color = get_theme_color("success_color", "Editor") as Color
	add_theme_color_override("font_color", color)
	add_theme_color_override("font_focus_color", color)
	add_theme_color_override("font_hover_color", color)


func _check_for_updates() -> void:
	var config = ConfigFile.new()
	config.load(CONFIG_FILE)
	http_request.request(config.get_value("plugin", "github_releases_url"))


func _version_to_number(version: String) -> int:
	var bits = version.split(".")
	return bits[0].to_int() * 1000000 + bits[1].to_int() * 1000 + bits[2].to_int()


func _on_http_request_request_completed(result: int, \
										response_code: int, \
										headers: PackedStringArray, \
										body: PackedByteArray) -> void:
	if result != HTTPRequest.RESULT_SUCCESS:
		return

	var current_version = OrchestratorVersion.get_version()

	var response = JSON.parse_string(body.get_string_from_utf8())
	if typeof(response) != TYPE_ARRAY:
		return

	var versions = (response as Array).filter(func(release):
		var version = release.tag_name.substr(1) as String
		return _version_to_number(version) > _version_to_number(current_version)
	)

	if not versions.is_empty():
		text = versions[0]["tag_name"]
		plugin_download_dialog_panel.download_version = versions[0]
		show()
	else:
		hide()


func _on_pressed():
	plugin_download_dialog.popup_centered()


func _on_plugin_download_dialog_close_requested():
	plugin_download_dialog.hide()


func _on_timer_timeout():
	_check_for_updates()


func _on_plugin_download_dialog_panel_update_failed():
	plugin_download_dialog.hide()
	plugin_download_failed_dialog.dialog_text = "Update failed, please try again."
	plugin_download_failed_dialog.popup_centered()


func _on_plugin_download_dialog_panel_update_succeeded(version):
	plugin_download_dialog.hide()
	needs_reload_dialog.dialog_text = "The project needs to be reloaded."
	needs_reload_dialog.ok_button_text = "Reload"
	needs_reload_dialog.cancel_button_text = "Not now"
	needs_reload_dialog.popup_centered()
	_apply_theme()


func _on_needs_reload_dialog_confirmed():
	editor_restart_requested.emit()


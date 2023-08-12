## Handles settings for the Orchestrator addon.
@tool
extends Node

const BASE_PATH = "orchestrator/general/"
const USER_CONFIG_PATH = "user://orchestrator_user_config.json"

const DEFAULT_SETTINGS = {
	"custom_test_scene_path" = "res://addons/orchestrator/test/test.tscn"
}

static func prepare() -> void:
	for setting in DEFAULT_SETTINGS:
		if ProjectSettings.has_setting("%s/%s" % [BASE_PATH, setting]):
			ProjectSettings.set_initial_value("%s/%s" % [BASE_PATH, setting], DEFAULT_SETTINGS[setting])
	ProjectSettings.save()


static func set_setting(key: String, value: Variant) -> void:
	ProjectSettings.set_setting("%s/%s" % [BASE_PATH, key], value)
	ProjectSettings.set_initial_value("%s/%s" % [BASE_PATH, key], DEFAULT_SETTINGS[key])
	ProjectSettings.save()


static func get_setting(key: String, default_value: Variant = null) -> Variant:
	if ProjectSettings.has_setting("%s/%s" % [BASE_PATH, key]):
		return ProjectSettings.get_setting("%s/%s" % [BASE_PATH, key])
	else:
		return default_value


static func get_user_config() -> Dictionary:
	var user_config: Dictionary = {
		run_resource_path = "",
	}

	if FileAccess.file_exists(USER_CONFIG_PATH):
		var file : FileAccess = FileAccess.open(USER_CONFIG_PATH, FileAccess.READ)
		user_config.merge(JSON.parse_string(file.get_as_text()), true)

	return user_config


static func save_user_config(user_config: Dictionary) -> void:
	var file : FileAccess = FileAccess.open(USER_CONFIG_PATH, FileAccess.WRITE)
	file.store_string(JSON.stringify(user_config))


static func set_user_value(key: String, value) -> void:
	var user_config : Dictionary = get_user_config()
	user_config[key] = value
	save_user_config(user_config)


static func get_user_value(key: String, default_value : Variant = null) -> Variant:
	return get_user_config().get(key, default_value)



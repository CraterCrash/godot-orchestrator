@tool
extends Node
## Handles settings for the Orchestrator addon.

const BASE_PATH = "orchestrator"
const USER_CONFIG_PATH = "user://orchestrator_user_config.json"

const DEFAULT_PROJECT_SETTINGS = {
	"run/test_scene" = {
		"type": TYPE_STRING,
		"hint": PROPERTY_HINT_FILE,
		"hint_string": "*.tscn,*.scn",
		"value": "res://addons/orchestrator/test/test.tscn"
	},
	"nodes/colors/background" = {
		"type": TYPE_COLOR,
		"hint": PROPERTY_HINT_COLOR_NO_ALPHA,
		"hint_string": "Color used for graph node background",
		"value": Color(0.12, 0.15, 0.19)
	},
	"nodes/colors/data" = {
		"type": TYPE_COLOR,
		"hint": PROPERTY_HINT_COLOR_NO_ALPHA,
		"hint_string": "Color to be used for data nodes",
		"value": Color(0.1686, 0.2824, 0.7882)
	},
	"nodes/colors/flow_control" = {
		"type": TYPE_COLOR,
		"hint": PROPERTY_HINT_COLOR_NO_ALPHA,
		"hint_string": "Color to be used for flow control nodes",
		"value": Color(0.251, 0.4549, 0.2078)
	},
	"nodes/colors/logic" = {
		"type": TYPE_COLOR,
		"hint": PROPERTY_HINT_COLOR_NO_ALPHA,
		"hint_string": "Color to be used for logic nodes",
		"value": Color(0.6784, 0.2, 0.2)
	},
	"nodes/colors/terminal" = {
		"type": TYPE_COLOR,
		"hint": PROPERTY_HINT_COLOR_NO_ALPHA,
		"hint_string": "Color to be used for terminal nodes",
		"value": Color(0.2706, 0.3686, 0.4314)
	},
	"nodes/colors/utility" = {
		"type": TYPE_COLOR,
		"hint": PROPERTY_HINT_COLOR_NO_ALPHA,
		"hint_string": "Color to be used for utility nodes",
		"value": Color(0.5765, 0.1686, 0.4275)
	},
	"nodes/colors/custom" = {
		"type": TYPE_COLOR,
		"hint": PROPERTY_HINT_COLOR_NO_ALPHA,
		"hint_string": "Color to be used for utility nodes",
		"value": Color(0.47, 0.27, .2)
	}
}


static func prepare() -> void:
	# Iterate all default project settings and be sure they're added
	for setting in DEFAULT_PROJECT_SETTINGS:
		var setting_key = "%s/%s" % [BASE_PATH, setting]
		if not ProjectSettings.has_setting(setting_key):
			var setting_details = DEFAULT_PROJECT_SETTINGS[setting]
			var default_value = setting_details["value"]
			var info = {
				"name": setting_key,
				"type": setting_details["type"],
				"hint": setting_details["hint"],
				"hint_string": setting_details["hint_string"]
			}
			set_setting(setting_key, default_value, info, default_value)


static func set_setting(key: String, \
						value: Variant = null, \
						property_info: Dictionary = {}, \
						default_value: Variant = null) -> void:
	if key != null and value == null:
		# Remove the setting
		ProjectSettings.set_setting(key, null)
		return

	ProjectSettings.set_setting(key, value)
	ProjectSettings.set_initial_value(key, default_value)
	ProjectSettings.set_as_basic(key, true) 				# Does not require Advanced Settings
	ProjectSettings.add_property_info(property_info) 		# Required for UI to show element
	ProjectSettings.save()


static func get_setting(key: String, default_value: Variant = null) -> Variant:
	var setting_key = key
	if not setting_key.begins_with(BASE_PATH + "/"):
		setting_key = "%s/%s" % [BASE_PATH, key]

	if ProjectSettings.has_setting(setting_key):
		return ProjectSettings.get_setting(setting_key)
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



extends RefCounted
## A utility class for accessing version details about the plugin


## Get the full version string formatted as "<name> v<version>"
static func get_full_version() -> String:
	return "Godot Orchestrator v" + get_version()


## Get the full version tag from the plugin.cfg file
static func get_version() -> String:
	var config = ConfigFile.new()
	config.load("res://addons/orchestrator/plugin.cfg")
	return config.get_value("plugin", "version")


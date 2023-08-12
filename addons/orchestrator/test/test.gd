## Test scene for executing an orchestration.
extends Node

const OrchestratorSettings = preload("res://addons/orchestrator/orchestrator_settings.gd")

@onready var resource: Orchestration = load(OrchestratorSettings.get_user_value("run_resource_path"))

func _ready():

	var screen_index: int = DisplayServer.get_primary_screen()
	DisplayServer.window_set_position(Vector2(DisplayServer.screen_get_position(screen_index)) + (DisplayServer.screen_get_size(screen_index) - DisplayServer.window_get_size()) * 0.5)
	DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_WINDOWED)

	var finished := func():
		await get_tree().create_timer(1).timeout
		get_tree().quit()
	Orchestrator.orchestration_finished.connect(finished)
	Orchestrator.execute(resource)


func _enter_tree() -> void:
	OrchestratorSettings.set_user_value("is_running_test_scene", false)


@icon("res://addons/orchestrator/assets/icons/OrchestratorLogo.svg")
class_name OrchestrationPlayer
extends Node
## Allows playing an [code]Orchestration[/code] from a scene tree node.
##
## An orchestration player is used for general-purpose execution of orchestrations.[br]
## [br]
## The orchestration resource can be statically assigned as part of the scene or can
## be assigned as part of a script that dynamically adds the player to a scene.[br]
## [br]
## Additionally, the player can be toggled to start the orchestration automatically
## when the node's [code]_ready()[/code] function is called. When using this player
## dynamically in a script, be sure to set the player's attributes prior to adding
## the node to the scene should you require auto-play functionality.
##

## Emitted when the orchestration starts.
signal orchestration_started()
## Emitted when the orchestration has finished.
signal orchestration_finished()

## The orchestration resource to be used.
@export_file("*.tres", "*.res") var orchestration: String:
	## Set the orchestration to be played
	set(value):
		orchestration = value
	## Get the name of the orchestration to be played
	get:
		return orchestration

## Controls whether the orchestration starts automatically when the scene loads.
@export var auto_play : bool:
	set(value):
		auto_play = value
	get:
		return auto_play


# Reference to the autoload
var _orchestrator

func _ready() -> void:
	_orchestrator = get_tree().root.find_child("Orchestrator", true, false)
	if auto_play:
		start()


func _exit_tree() -> void:
	# Be sure to disconnect from the Orchestrator singleton if connected and
	# the node is being removed from the tree.
	if _orchestrator and _orchestrator.orchestration_started.is_connected(_on_orchestration_started):
		_orchestrator.orchestration_started.disconnect(_on_orchestration_started)
		_orchestrator.orchestration_finished.disconnect(_on_orchestration_finished)


## Starts the associated orchestration.
func start() -> void:
	if not orchestration:
		return

	# Verify resource exists
	if not FileAccess.file_exists(orchestration):
		printerr("Orchestration resource '%s' not found." % orchestration)

	# Load resource
	var resource = load(orchestration)
	if not resource:
		printerr("Failed to load orchestration resource '%s'" % orchestration)
		return

	# Connect to the orchestrator callbacks
	# We specifically use signal bubbling here as a way to allow different use cases
	# to subscribe either to the player or the Orchestrator singleton.
	if _orchestrator and not _orchestrator.orchestration_started.is_connected(_on_orchestration_started):
		_orchestrator.orchestration_started.connect(_on_orchestration_started)
		_orchestrator.orchestration_finished.connect(_on_orchestration_finished)

	if _orchestrator:
		_orchestrator.execute(resource)
	else:
		printerr("Orchestrator autoload is not in the scene, the orchestration won't be ran.")


func _on_orchestration_started() -> void:
	orchestration_started.emit()


func _on_orchestration_finished() -> void:
	orchestration_finished.emit()

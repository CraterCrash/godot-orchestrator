## An orchestration workflow resource.
##
## An orchestration is a visual scripting like language.
## The workflow provides access to stock behavior nodes and allows the user to
## define custom nodes. This resource is primarily used when using the Orchestrator
## plugin from within the edtor or running an Orchestration from within a game.
@tool
@icon("./assets/Orchestrator.svg")
class_name Orchestration
extends Resource

## The version of the workflow
@export var version : int = 1

## The next id to be assigned to a node within this orchestration.
## This is an always increasing number useful to keep nodes unique.
@export var next_node_id : int = 1

## The orchestration graph view's zoom.
@export var zoom : float = 1.0

## The orchestration graph view's scroll offset.
@export var scroll_offset : Vector2 = Vector2(0,0)

## The nodes within the workflow
@export var nodes: Array

## The connections between the nodes in the workflow
@export var connections: Array[Dictionary]

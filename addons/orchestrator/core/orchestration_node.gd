## A resource that represents a node within an orchestration.
##
## An [OrchestrationNode] represents the node details within an [Orchestration].
## This resource is shared by both the editor's UI interface and graph editor
## and the Orchestrator addon's runtime pipeline.
##
@tool
class_name OrchestrationNode
extends Resource

const END_EXECUTION := -1

## A signal emitted when an input slot is added to the node.
signal input_slot_added(index: int, slot: OrchestrationNodeSlot)

## A signal emitted when an input slot is removed from the node.
signal input_slot_removed(index: int, slot: OrchestrationNodeSlot)

## A signal emitted when an output slot is added to the node.
signal output_slot_added(index: int, slot: OrchestrationNodeSlot)

## A signal emitted when an output slot is removed from the node.
signal output_slot_removed(index: int, slot: OrchestrationNodeSlot)

## Whether multiple of the same node can exist in an orchestration.
var allow_multiple := true

## The node's unique type id.
## Every node type that is registered with the plugin should have a unique node type.
var type: int

## The node's name.
## This is the the name shown in both the node list and the node UI's caption bar.
var name: String

## The node's category.
## This is the node's category, used in the node list to group similar nodes by type.
var category: String = "Custom"

## The node's description.
## This is shown as a tooltip when hovering over a node in the editor's UI.
var description: String

## The unique id assigned to this node resource.
## All nodes within an orchestration have a unique [code]id[/code].
var id: String

## The defined input slot mappings.
## The key of the dictionary is the [code]slot[/code] while the value is an [code]OrchestrationNodeSlot[/code].
var inputs: Dictionary

## The defined output slot mappings.
## The key of the dictionary is the [code]slot[/code] while the value is an [code]OrchestrationNodeSlot[/code].
var outputs: Dictionary

# Editor-only attributes
# These are set only when the OrchestrationGraphNode is created based on this
# resource; but at runtime these values are not initialized and are null.
var _graph_node : OrchestrationGraphNode
var _graph_edit : GraphEdit


## Execute the node's behavior at runtime.
## This is used by the [Orchestrator] plugin at runtime to process the node's behavior.
func execute(context: OrchestrationExecutionContext) -> Variant:
	return 0


## Method that should be overwritten by subclass implementations to return attribute
## state that should be persisted with the node in an orchestration resource.
func get_attributes() -> Dictionary:
	return {}


## Allows a resource to provide a custom UI
func create_ui(attributes: OrchestratorDictionary, scene_node: Node) -> void:
	pass


## Called when another node is added to the orchestration
func node_added(scene_node: Node, resource: OrchestrationNode) -> void:
	pass


## Called when another node is removed from the orchestration
func node_removed(scene_node: Node, resource: OrchestrationNode) -> void:
	pass


## Creates an input slot mapping
func add_input_slot(port: int, slot: int, type: int = 0) -> void:
	var input = OrchestrationNodeSlot.new()
	input.port = port
	input.type = type
	input.color = Color.WHITE
	inputs[slot] = input
	input_slot_added.emit(slot, input)


## Add an output slot mapping.
func add_output_slot(port: int, slot: int, type: int = 0) -> void:
	var output = OrchestrationNodeSlot.new()
	output.port = port
	output.type = type
	output.color = Color.WHITE
	outputs[slot] = output
	output_slot_added.emit(slot, output)


## Removes an input slot mapping
func remove_input_slot(slot: int) -> bool:
	if not _graph_edit:
		return false
	if not inputs.has(slot):
		return false

	var input = inputs[slot]
	inputs.erase(slot)
	input_slot_removed.emit(slot, input)
	return true


## Removes an output slot mapping
func remove_output_slot(slot: int) -> bool:
	if not _graph_edit:
		return false
	if not outputs.has(slot):
		return false

	var output = outputs[slot]
	outputs.erase(slot)
	output_slot_removed.emit(slot, output)
	return true

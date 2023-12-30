## This file is part of the Godot Orchestrator project.
##
## Copyright (c) 2023-present Vahera Studios LLC and its contributors.
##
## Licensed under the Apache License, Version 2.0 (the "License");
## you may not use this file except in compliance with the License.
## You may obtain a copy of the License at
##
##		http://www.apache.org/licenses/LICENSE-2.0
##
## Unless required by applicable law or agreed to in writing, software
## distributed under the License is distributed on an "AS IS" BASIS,
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
## See the License for the specific language governing permissions and
## limitations under the License.
##
extends CanvasLayer

## A required signal that should be emitted when the Dialogue UI completes
## all its user interactions. This informs Orchestrator that it is then
## safe to proceed to the next node/step in the visual script workflow.
signal show_message_finished()

## The speed that the text will be rendered, i.e. typewriter
const TEXT_SPEED = 0.03

## Holds data passed by Orchestrator the Dialogue UI
##
## By default attributes are passed into this dictionary as:
##		"character_name" -> the name of the character speaking
##		"message" -> the text to be shown
##		"options" -> an array of text to be shown as choices
##
var dialogue_data : Dictionary

## Requirede by Orchestrator, passes the selected choice from
## the Dialogue UI back to the OrchestratorScript.
var selection : int

var _current_tween : Tween
var _current_choices : Dictionary

@onready var speaker 	  = $MarginContainer/PanelContainer/MarginContainer/VBoxContainer/Speaker
@onready var speaker_text = $MarginContainer/PanelContainer/MarginContainer/VBoxContainer/SpeakerText
@onready var response_tpl = $MarginContainer/PanelContainer/MarginContainer/VBoxContainer/ResponseTemplate
@onready var next_button  = $MarginContainer/PanelContainer/MarginContainer/VBoxContainer/NextButton

func _ready() -> void:
	response_tpl.visible = false
	next_button.visible = false
	visible = false
	
	## Setup what to do if there are no options and the next button
	## is shown, which is "Continue". In this case the selection is
	## set as -1 and the signal gets emitted.
	var button_handler := func():
		selection = -1
		show_message_finished.emit()
		queue_free()
	next_button.pressed.connect(button_handler)
	
	## Grab data from Orchestrator dictionary to present the UI
	var character_name = dialogue_data["character_name"]
	var message = dialogue_data["message"]	
	var options = dialogue_data["options"]	
	show_message(character_name, message, options)
	
	
func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey:
		if event.is_pressed() and _should_end_typing(event.keycode):	
			if _current_tween.is_running():
				_current_tween.stop()
				speaker_text.visible_characters = speaker_text.text.length()
				_on_tween_finished(_current_choices)
		
			get_viewport().set_input_as_handled()
			 				
		# ShowMessage is only allowed to process input when visible
		_disable_player_movement()
			

func show_message(speaker_name: String, message: String, choices: Dictionary) -> void:
	speaker.text = speaker_name
	speaker_text.text = message
	_current_choices = choices
	await get_tree().process_frame
	
	var duration = speaker_text.text.length() * TEXT_SPEED
	_current_tween = get_tree().create_tween()
	_current_tween.tween_property(speaker_text, "visible_characters", speaker_text.text.length(), duration)
	_current_tween.finished.connect(Callable(_on_tween_finished).bind(choices))
	show()
	
## Callback when the tween typing has finished.
## This allows presenting the user choice options or Continue button.
func _on_tween_finished(choices: Dictionary) -> void:
	if choices.size() > 0:		
		for key in choices.keys():
			var button = response_tpl.duplicate()
			button.text = choices[key]
			button.visible = true
			speaker_text.get_parent().add_child(button)
			var button_handler := func():
				selection = key
				show_message_finished.emit()
				queue_free()
			button.pressed.connect(button_handler)
	else:
		next_button.show()
		

## Handles canceling the typing effect based on key codes
func _should_end_typing(keycode:int) -> bool:
	return keycode == KEY_ESCAPE or keycode == KEY_SPACE
	

## Prevent user movement when the dialogue UI is shown
func _disable_player_movement() -> void:	
	var actions = InputMap.get_actions()
	for action in actions:
		Input.action_release(action)

extends CanvasLayer

signal show_message_finished(selected_index: int)

const TEXT_SPEED = 0.03

var _current_tween : Tween
var _current_choices : Dictionary

@onready var speaker = $MarginContainer/PanelContainer/MarginContainer/V/Speaker
@onready var speaker_text = $MarginContainer/PanelContainer/MarginContainer/V/SpeakerText
@onready var response_template = $MarginContainer/PanelContainer/MarginContainer/V/ResponseTemplate
@onready var next_button = $MarginContainer/PanelContainer/MarginContainer/V/NextButton

# Called when the node enters the scene tree for the first time.
func _ready():
	response_template.visible = false
	next_button.visible = false
	visible = false

	var button_handler := func():
		show_message_finished.emit(-1)
		queue_free()
	next_button.pressed.connect(button_handler)


func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey and event.is_pressed() and \
		(event.keycode == KEY_ESCAPE or event.keycode == KEY_SPACE):
		if _current_tween.is_running():
			_current_tween.stop()
			speaker_text.visible_characters = speaker_text.text.length()
			_on_tween_finished(_current_choices)

	# ShowMessage is only allowed to process input when visible
	get_viewport().set_input_as_handled()


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

func _on_tween_finished(choices: Dictionary) -> void:
	if choices.size() > 0:
		for key in choices.keys():
			var button = response_template.duplicate()
			button.text = choices[key]
			button.visible = true
			speaker_text.get_parent().add_child(button)

			var button_handler := func():
				show_message_finished.emit(key)
				queue_free()
			button.pressed.connect(button_handler)
	else:
		next_button.show()

extends CanvasLayer

signal show_message_finished(selected_index: int)

const TEXT_SPEED = 0.03

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
	# ShowMessage is only allowed to process input when visible
	get_viewport().set_input_as_handled()


func show_message(speaker_name: String, message: String, choices: Dictionary) -> void:
	speaker.text = speaker_name
	speaker_text.text = message
	await get_tree().process_frame

	var duration = speaker_text.text.length() * TEXT_SPEED
	var tween = get_tree().create_tween()
	tween.tween_property(speaker_text, "visible_characters", speaker_text.text.length(), duration)
	tween.finished.connect(Callable(_on_tween_finished).bind(choices))

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

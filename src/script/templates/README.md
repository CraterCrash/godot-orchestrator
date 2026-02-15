# Orchestration Script Templates

All Orchestration script templates must be in a subdirectory and follow these rules:

1. The subdirectory should be the same as the Godot Class the template extends. 
   For example, the empty template for `Object` is in a subdirectory called `Object`.
2. All script templates require two unique files, an `ini` and `torch` files, which are both named the same.

## Metadata files

The `ini` file is the metadata file that is primarily used to supply details about the script for the Godot UI.
For example, the current implementation simply stores the description of the template in these, e.g.

```ini
[meta]
description=The description of the script is placed here.
```

When the user selects the template in the **Create Script Dialog** window, this is the description that is shown in the bottom portion of the dialog window.
The goal of this description is to explain to the user in more detail what the template actually does.

## Script files

The basename of the script file will be used in the **Create Script Dialog** window as the template name.
For example, if selecting `CharacterBody2D` as the base type in the dialog, the user will be shown several templates:

* CharacterBody2D: Basic Movement
* Node: Default
* Object: Empty

As can be seen by looking in each of the class directories, the name presented here matches the basename of the script files.

### Script source requirements

A `torch` script file source contents must adhere to the following rules:

* The `[orchestration]` tag must not include a `uid` value. 
* the `guid` or `function_id` properties in `[obj]` sections should not specify a UUID value, but instead should be replaced with the `_GUIDn_` macros.
* The `base_type` property in `[resource]` should always specify `_BASE_` as the value, allowing it to be dynamically assigned based on the user specified details.
* The `class_name` property in `[resource]` and `script_class` in the `[orchestration]` tags should be dynamic based on `_CLASS_` macro, so that each instance of the generated scripts are unique.

By following these rules, when the substitutions occur, discussed below, all new script files generated from the templates are sufficiently unique.

## Script text substitutions

In all `.torch` script files, several text strings will be automatically replaced with information provided by the user when creating the script in the **Create Script Dialog** window.
The following chart explains the text string macros that are replaced and a description on how the values as sourced.

| Macro                | Description                                                                                                                                                                                                      | 
|----------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `_BASE_`             | Replaced by the _inherits_ value.                                                                                                                                                                                |
| `_CLASS_`            | Replaced by the script filename basename, converted to pascal case, e.g. `HelloWorld`.                                                                                                                           |
| `_CLASS_SNAKE_CASE_` | Replaced by the script filename basename, but converted to snake case, e.g. `hello_world`.                                                                                                                       |
| `_GUIDn_`            | Replaced by dynamically generated GUID values, where `n` represents a number starting at `1`. The same GUID can be repeated in the script by specifying the `_GUIDn_` macro with the same number multiple times. |


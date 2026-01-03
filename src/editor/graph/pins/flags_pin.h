// // This file is part of the Godot Orchestrator project.
// //
// // Copyright (c) 2023-present Crater Crash Studios LLC and its contributors.
// //
// // Licensed under the Apache License, Version 2.0 (the "License");
// // you may not use this file except in compliance with the License.
// // You may obtain a copy of the License at
// //
// //		http://www.apache.org/licenses/LICENSE-2.0
// //
// // Unless required by applicable law or agreed to in writing, software
// // distributed under the License is distributed on an "AS IS" BASIS,
// // WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// // See the License for the specific language governing permissions and
// // limitations under the License.
// //
// #ifndef ORCHESTRATOR_EDITOR_GRAPH_PIN_FLAGS_H
// #define ORCHESTRATOR_EDITOR_GRAPH_PIN_FLAGS_H
//
// #include "editor/graph/pins/base.h"
//
// #include <godot_cpp/classes/check_box.hpp>
// #include <godot_cpp/classes/popup_panel.hpp>
//
// /// An implementation of <code>OrchestratorEditorGraphPinBase</code> that shows a button with the selected
// /// flag details, and upon clicking, the user is shown a series of checkboxes in a panel to adjust the
// /// flag value.
// ///
// class OrchestratorEditorGraphPinFlags : public OrchestratorEditorGraphPinBase
// {
//     GDCLASS(OrchestratorEditorGraphPinFlags, OrchestratorEditorGraphPinBase);
//
//     struct Entry
//     {
//         String friendly_name;
//         String real_name;
//         int64_t value;
//     };
//
//     Button* _button = nullptr;
//     PopupPanel* _panel = nullptr;
//     Vector<CheckBox*> _checkboxes;
//
// protected:
//     static void _bind_methods() { }
//
//     //~ Begin OrchestratorEditorGraphPinBase Interface
//     void _update_control_value() override;
//     Variant _read_control_value() override;
//     //~ End OrchestratorEditorGraphPinBase Interface
//
// public:
//     void add_item(const String& p_real_name, const String& p_friendly_name, int64_t p_value);
//
//     OrchestratorEditorGraphPinFlags();
// };
//
// #endif // ORCHESTRATOR_EDITOR_GRAPH_PIN_FLAGS_H
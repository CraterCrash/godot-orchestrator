// This file is part of the Godot Orchestrator project.
//
// Copyright (c) 2023-present Vahera Studios LLC and its contributors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//		http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "editor/plugins/inspector_plugin_variable.h"

#include "editor/plugins/orchestrator_editor_plugin.h"
#include "editor/search/variable_classification_dialog.h"
#include "script/variable.h"


bool OrchestratorEditorInspectorPluginVariable::_can_handle(Object* p_object) const
{
    return p_object->get_class() == OScriptVariable::get_class_static();
}

bool OrchestratorEditorInspectorPluginVariable::_parse_property(Object* p_object, Variant::Type p_type, const String& p_name,
    PropertyHint p_hint, const String& p_hint_string, BitField<PropertyUsageFlags> p_usage, bool p_wide)
{
    Ref<OScriptVariable> variable = Object::cast_to<OScriptVariable>(p_object);
    if (variable.is_null())
        return false;

    if (p_name.match("classification"))
    {
        OrchestratorEditorPropertyVariableClassification* editor = memnew(OrchestratorEditorPropertyVariableClassification);
        _classification = editor;
        add_property_editor(p_name, editor, true);
        return true;
    }

    return false;
}

void OrchestratorEditorInspectorPluginVariable::edit_classification(Object* p_object)
{
    Ref<OScriptVariable> variable = Object::cast_to<OScriptVariable>(p_object);
    if (variable.is_null())
        return;

    // This is done to clear and reset the editor interface
    OrchestratorPlugin::get_singleton()->get_editor_interface()->edit_node(nullptr);
    OrchestratorPlugin::get_singleton()->get_editor_interface()->edit_resource(variable);

    _classification->edit();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OrchestratorEditorPropertyVariableClassification::_notification(int p_what)
{
    if (p_what == NOTIFICATION_READY)
    {
        // Register button that triggers opening the search dialog
        _property = memnew(Button);
        _property->set_clip_text(true);
        _property->set_text(_selected_name);
        add_child(_property);
        add_focusable(_property);

        _dialog = memnew(OrchestratorVariableTypeSearchDialog);
        _dialog->set_base_type(_base_type);
        add_child(_dialog);

        _dialog->connect("selected", callable_mp(this, &OrchestratorEditorPropertyVariableClassification::_on_search_selected));
        _property->connect("pressed", callable_mp(this, &OrchestratorEditorPropertyVariableClassification::_on_property_selected));
    }
}

void OrchestratorEditorPropertyVariableClassification::setup(const String& p_base_type, const String& p_selected_type)
{
    _base_type = p_base_type;
    _dialog->set_base_type(p_base_type);

    _selected_name = p_selected_type;

    if (p_selected_type.contains(":") && p_selected_type.match("type:Nil"))
        _property->set_text("Any");
    else
        _property->set_text(p_selected_type);
}

void OrchestratorEditorPropertyVariableClassification::edit()
{
    _dialog->popup_create(true, true, get_edited_object()->get(get_edited_property()), get_edited_property());
}

void OrchestratorEditorPropertyVariableClassification::_update_property()
{
    String value = get_edited_object()->get(get_edited_property());
    _selected_name = value;

    if (value.contains(":"))
    {
        if (value.match("type:Nil"))
            _property->set_text("Any");
        else
            _property->set_text(value.substr(value.find(":") + 1));
    }
    else
        _property->set_text(value);
}

void OrchestratorEditorPropertyVariableClassification::_on_search_selected()
{
    _selected_name = _dialog->get_selected_type();
    emit_changed(get_edited_property(), _selected_name);
    update_property();
}

void OrchestratorEditorPropertyVariableClassification::_on_property_selected()
{
    _dialog->popup_create(true, true, get_edited_object()->get(get_edited_property()), get_edited_property());
}
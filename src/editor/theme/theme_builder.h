// This file is part of the Godot Orchestrator project.
//
// Copyright (c) 2023-present Crater Crash Studios LLC and its contributors.
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
#pragma once

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/theme.hpp>

using namespace godot;

/// Class that is responsible for building the Orchestrator plugin's theme.
class OrchestratorEditorThemeBuilder : public Object {
    GDCLASS(OrchestratorEditorThemeBuilder, Object);

    struct ThemeParams {
        String theme;
        float border_radius;
        float border_width;
        Color border_color;
        Color selected_border_color;
        Color background_color;
    };

    Ref<Theme> _theme;
    bool _rebuilding = false;

    ThemeParams _read_theme_params() const;

    void _build_graph_styles(const Ref<Theme>& p_theme, const ThemeParams& p_params);

    void _rebuild_theme();

protected:
    static void _bind_methods();

    //~ Begin Wrapped Interface
    void _notification(int p_what);
    //~ End Wrapped Interface

public:
    Ref<Theme> get_theme() const { return _theme; }

    void queue_rebuild();
};

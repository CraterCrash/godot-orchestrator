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

namespace OSUtils {
    /// Whether this platform prefers the Meta/Command key over Control for shortcuts (macOS/iOS web builds).
    /// Mirrors the engine's <code>OS::prefer_meta_over_ctrl()</code>, which is not exposed.
    /// Uses runtime feature detection so a single binary behaves  correctly across platforms (notably web).
    /// @return true if Meta/Command is preferred over Control
    bool prefer_meta_over_ctrl();
}
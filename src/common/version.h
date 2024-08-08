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
#ifndef ORCHESTRATOR_VERSION_H
#define ORCHESTRATOR_VERSION_H

#include "version.gen.h"

#include <godot_cpp/core/version.hpp>

// Orchestrator versions are of the form <major>.<minor> for the initial release,
// and then <major>.<minor>.<maintenance> for subsequent maintenance releases where the
// <maintenance> is not equal-to 0.

// Define the main "branch" version.
// Maintenance versions in this branch be forward-compatible.
// Example: "2.1"
#define VERSION_BRANCH _MKSTR(VERSION_MAJOR) "." _MKSTR(VERSION_MINOR)
#if VERSION_MAINTENANCE
// Example: "2.1.3"
#define VERSION_NUMBER VERSION_BRANCH "." _MKSTR(VERSION_MAINTENANCE)
#else // maintenance is 0
// Example: "2.1" instead of "2.1.0"
#define VERSION_NUMBER VERSION_BRANCH
#endif

// Version number encoded as hexadecimal int with one byte for each number,
// for easy comparison from code.
// Example: 2.1.4 will be 0x020104
#define VERSION_HEX 0x10000 * VERSION_MAJOR + 0x100 * VERSION_MINOR + VERSION_MAINTENANCE

// Describes the full configuration of the Orchestrator version, including the version number,
// the status (beta, stable, etc) and potential module specific features.
#define VERSION_FULL_CONFIG VERSION_NUMBER "." VERSION_STATUS VERSION_MODULE_CONFIG

// Similar to VERSION_FULL_CONFIG, but also includes the (potentially custom) VERSION_BUILD
// description (e.g. official, custom_build, etc).
// Example: "2.1.4.stable.win32.official"
#ifdef _DEBUG
#define VERSION_FULL_BUILD VERSION_FULL_CONFIG "." VERSION_BUILD " (Debug)"
#else
#define VERSION_FULL_BUILD VERSION_FULL_CONFIG "." VERSION_BUILD
#endif

// Same as above, but prepend with Orchestrator's name and cosmetic "v" for version.
// Example: "Orchestrator v2.1.4.stable.win32.official"
#define VERSION_FULL_NAME VERSION_NAME " v" VERSION_FULL_BUILD

// Git commit hash, generated at build time in "version_hash.gen.cpp"
// extern const char *const VERSION_HASH;

// Version number encoded as a hexadecimal int with one byte for each nuber,
// for easy comparison from code.
// Exmaple: 2.1.4 will be 0x020104
#define GODOT_VERSION 0x10000 * GODOT_VERSION_MAJOR + 0x100 * GODOT_VERSION_MINOR + GODOT_VERSION_PATCH

#endif  // ORCHESTRATOR_VERSION_H

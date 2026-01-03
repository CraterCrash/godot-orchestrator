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
#include "core/godot/string/string.h"

using namespace godot;

StringPtr& StringPtr::operator=(const String& p_value) {
    if (!_data) {
        _data = memnew(String);
    }
    *_data = p_value;
    return *this;
}

StringPtr& StringPtr::operator=(const char* p_value) {
    if (!_data) {
        _data = memnew(String);
    }
    *_data = p_value;
    return *this;
}

const String& StringPtr::get() const {
    static String empty;
    return _data ? *_data : empty;
}

StringPtr::~StringPtr() {
    if (_data) {
        memdelete(_data);
        _data = nullptr;
    }
}
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
#include "logger.h"

Logger* Logger::_instance = nullptr;

Logger::Logger(const Ref<FileAccess>& p_file)
{
    _instance = this;
    _file = p_file;
}

Logger::~Logger()
{
    _instance = nullptr;
    _file->close();
}

Logger::Level Logger::get_level_from_name(const String& p_name)
{
    const String name = p_name.to_upper();
    if (name.match("FATAL"))
        return Level::FATAL;
    else if (name.match("ERROR"))
        return Level::ERROR;
    else if (name.match("WARN"))
        return Level::WARN;
    else if (name.match("INFO"))
        return Level::INFO;
    else if (name.match("DEBUG"))
        return Level::DEBUG;
    else if (name.match("TRACE"))
        return Level::TRACE;
    return Level::OFF;
}

Logger* LoggerFactory::create(const String& p_file_name)
{
    Ref<FileAccess> file = FileAccess::open(p_file_name, FileAccess::WRITE);
    if (!file.is_valid())
    {
        CRASH_NOW_MSG("Cannot create logger because file path " + p_file_name + " is not accessible.");
    }
    return new Logger(file);
}
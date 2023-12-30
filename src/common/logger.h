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
#ifndef ORCHESTRATOR_LOGGER_H
#define ORCHESTRATOR_LOGGER_H

#include <chrono>
#include <sstream>

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

/// Logger framework
class Logger
{
    friend class LoggerFactory;

public:
    /// Logger levels
    enum Level
    {
        OFF,    //! No logging
        FATAL,  //! Very severe errors that typically lead to application failures
        ERROR,  //! Severe errors that may allow the application to continue to run
        WARN,   //! Potentially harmful conditions
        INFO,   //! Information messages
        DEBUG,  //! Fine-grained information messages that are useful for debugging
        TRACE,  //! Finger-grained information messages beyond debug
    };

private:
    static Logger* _instance;  //! Singleton instance
    Level _level{ INFO };      //! Current log level
    Ref<FileAccess> _file;     //! Current log file

    Logger(const Ref<FileAccess>& p_file);

protected:
    /// Get the log level name
    /// @param p_level the level
    /// @return the log level
    String _get_level(Level p_level)
    {
        switch (p_level)
        {
            case FATAL:
                return "FATAL";
            case ERROR:
                return "ERROR";
            case WARN:
                return "WARN";
            case INFO:
                return "INFO";
            case DEBUG:
                return "DEBUG";
            case TRACE:
                return "TRACE";
            default:
                return "UNKNOWN";
        }
    }

    template <typename... Args>
    void _log(Level p_level, Args&&... p_args)
    {
        if (_level >= p_level && _file.is_valid())
        {
            std::array<Variant, sizeof...(Args)> args{ Variant(p_args)... };

            PackedStringArray strings;
            for (size_t i = 0; i < args.size(); i++)
                strings.push_back(UtilityFunctions::str(args[i]));

            using std::chrono::seconds, std::chrono::days;
            auto time = std::chrono::current_zone()->to_local(
                std::chrono::floor<seconds>(std::chrono::system_clock::now()));
            auto today = std::chrono::floor<days>(time);
            std::string datetime = std::format("{0:%Y-%m-%d} {1:%H:%M:%S}", std::chrono::year_month_day{ today },
                                               std::chrono::hh_mm_ss{ time - today });

            const String timestamp(datetime.c_str());

            String message;
            message += timestamp;
            message += " [" + _get_level(p_level) + "]: ";

            for (const String& part : strings)
                message += part;

            _file->store_line(message);
            _file->flush();
        }
    }

public:
    ~Logger();

    // Simple helpers
    template <typename... Args>
    static void fatal(Args&&... p_args)
    {
        _instance->_log(FATAL, std::forward<Args>(p_args)...);
    }

    template <typename... Args>
    static void error(Args&&... p_args)
    {
        _instance->_log(ERROR, std::forward<Args>(p_args)...);
    }

    template <typename... Args>
    static void warn(Args&&... p_args)
    {
        _instance->_log(WARN, std::forward<Args>(p_args)...);
    }

    template <typename... Args>
    static void info(Args&&... p_args)
    {
        _instance->_log(INFO, std::forward<Args>(p_args)...);
    }

    template <typename... Args>
    static void debug(Args&&... p_args)
    {
        _instance->_log(DEBUG, std::forward<Args>(p_args)...);
    }

    template <typename... Args>
    static void trace(Args&&... p_args)
    {
        _instance->_log(TRACE, std::forward<Args>(p_args)...);
    }

    /// Get the log level used
    /// @return the log level
    static Level get_level() { return _instance->_level; }

    /// Set the log level
    /// @param p_level the log level
    static void set_level(Level p_level) { _instance->_level = p_level; }

    /// Get the log level by text-based name
    /// @param p_name the log level name
    /// @return the Logger::Level enum value
    static Level get_level_from_name(const String& p_name);
};

class LoggerFactory
{
public:
    static Logger* create(const String& p_file_name);
};

#endif  // ORCHESTRATOR_LOGGER_H

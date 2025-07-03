/*
 * @file logger.cpp
 * @brief Modern C++20 logger implementation using spdlog
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include <logger.h>

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <filesystem>
#include <vector>

namespace base {

std::shared_ptr<spdlog::logger> Logger::s_logger{nullptr};

void Logger::init() {
    LoggerConfig config{
        .enable_console = true,
        .enable_file = false
    };
    init(config);
}

void Logger::init(const LoggerConfig& config) {
    // Drop existing logger if it exists
    if (s_logger) {
        spdlog::drop(s_logger->name());
        s_logger.reset();
    }

    std::vector<spdlog::sink_ptr> sinks;

    // Add console sink if enabled
    if (config.enable_console) {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern(config.pattern);

        // Set color mode based on configuration
        if (config.enable_colors) {
            console_sink->set_color_mode(spdlog::color_mode::always);
        } else {
            console_sink->set_color_mode(spdlog::color_mode::never);
        }

        sinks.push_back(console_sink);
    }

    // Add file sink if enabled
    if (config.enable_file && !config.log_file.empty()) {
        // Create directory if it doesn't exist
        if (auto parent = config.log_file.parent_path(); !parent.empty()) {
            std::filesystem::create_directories(parent);
        }

        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            config.log_file.string(), config.max_file_size, config.max_files);
        file_sink->set_pattern(config.pattern);
        sinks.push_back(file_sink);
    }

    // Create logger with sinks
    s_logger = std::make_shared<spdlog::logger>(config.app_name, sinks.begin(), sinks.end());
    s_logger->set_level(to_spdlog_level(config.level));
    s_logger->flush_on(spdlog::level::err);

    // Register with spdlog for global access
    spdlog::register_logger(s_logger);
}

void Logger::init(std::string_view app_name, std::string_view filename) {
    LoggerConfig config{
        .app_name = std::string{app_name},
        .log_file = std::filesystem::path{filename},
        .enable_console = false,
        .enable_file = true
    };
    init(config);
}

LogLevel Logger::get_level() noexcept {
    if (!s_logger) {
        return LogLevel::Off;
    }
    return from_spdlog_level(s_logger->level());
}

void Logger::set_level(LogLevel level) noexcept {
    if (s_logger) {
        s_logger->set_level(to_spdlog_level(level));
    }
}

void Logger::setLogLevel(std::string_view loglevel) {
    if (s_logger) {
        auto level = spdlog::level::from_str(std::string{loglevel});
        s_logger->set_level(level);
    }
}

void Logger::flush() noexcept {
    if (s_logger) {
        s_logger->flush();
    }
}

void Logger::shutdown() noexcept {
    if (s_logger) {
        s_logger->flush();
        spdlog::drop(s_logger->name());
        s_logger.reset();
    }
}

spdlog::level::level_enum Logger::to_spdlog_level(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::Trace:    return spdlog::level::trace;
        case LogLevel::Debug:    return spdlog::level::debug;
        case LogLevel::Info:     return spdlog::level::info;
        case LogLevel::Warn:     return spdlog::level::warn;
        case LogLevel::Error:    return spdlog::level::err;
        case LogLevel::Critical: return spdlog::level::critical;
        case LogLevel::Off:      return spdlog::level::off;
        default:                 return spdlog::level::info;
    }
}

LogLevel Logger::from_spdlog_level(spdlog::level::level_enum level) noexcept {
    switch (level) {
        case spdlog::level::trace:    return LogLevel::Trace;
        case spdlog::level::debug:    return LogLevel::Debug;
        case spdlog::level::info:     return LogLevel::Info;
        case spdlog::level::warn:     return LogLevel::Warn;
        case spdlog::level::err:      return LogLevel::Error;
        case spdlog::level::critical: return LogLevel::Critical;
        case spdlog::level::off:      return LogLevel::Off;
        default:                      return LogLevel::Info;
    }
}

} // namespace base

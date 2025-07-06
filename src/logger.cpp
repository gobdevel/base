/*
 * @file logger.cpp
 * @brief Modern C++20 logger implementation using spdlog
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include <logger.h>
#include <config.h>

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <filesystem>
#include <vector>
#include <algorithm>

namespace base {

std::shared_ptr<spdlog::logger> Logger::s_logger{nullptr};

// Component filtering state
bool Logger::s_component_logging_enabled = true;
std::vector<std::string> Logger::s_enabled_components{};
std::vector<std::string> Logger::s_disabled_components{};

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

    // Store component logging configuration
    s_component_logging_enabled = config.enable_component_logging;
    s_enabled_components = config.enabled_components;
    s_disabled_components = config.disabled_components;

    std::vector<spdlog::sink_ptr> sinks;

    // Add console sink if enabled
    if (config.enable_console) {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

        // Use component pattern if component logging is enabled, otherwise use regular pattern
        const std::string& pattern = config.enable_component_logging ? config.component_pattern : config.pattern;
        console_sink->set_pattern(pattern);

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

        // Use component pattern if component logging is enabled, otherwise use regular pattern
        const std::string& pattern = config.enable_component_logging ? config.component_pattern : config.pattern;
        file_sink->set_pattern(pattern);
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

bool Logger::is_component_enabled(std::string_view component) noexcept {
    if (!s_component_logging_enabled) {
        return true;  // Component logging disabled, all components enabled
    }

    const std::string comp_str{component};

    // Check if component is explicitly disabled
    for (const auto& disabled : s_disabled_components) {
        if (disabled == comp_str) {
            return false;
        }
    }

    // If we have a whitelist of enabled components, check if this component is in it
    if (!s_enabled_components.empty()) {
        for (const auto& enabled : s_enabled_components) {
            if (enabled == comp_str) {
                return true;
            }
        }
        return false;  // Component not in whitelist
    }

    // No whitelist and not explicitly disabled, so it's enabled
    return true;
}

void Logger::enable_components(const std::vector<std::string>& components) {
    s_enabled_components = components;

    // Remove any newly enabled components from the disabled list
    for (const auto& comp : components) {
        auto it = std::find(s_disabled_components.begin(), s_disabled_components.end(), comp);
        if (it != s_disabled_components.end()) {
            s_disabled_components.erase(it);
        }
    }
}

void Logger::disable_components(const std::vector<std::string>& components) {
    for (const auto& comp : components) {
        // Add to disabled list if not already there
        if (std::find(s_disabled_components.begin(), s_disabled_components.end(), comp) == s_disabled_components.end()) {
            s_disabled_components.push_back(comp);
        }

        // Remove from enabled list if present
        auto it = std::find(s_enabled_components.begin(), s_enabled_components.end(), comp);
        if (it != s_enabled_components.end()) {
            s_enabled_components.erase(it);
        }
    }
}

void Logger::clear_component_filters() {
    s_enabled_components.clear();
    s_disabled_components.clear();
}

void Logger::configure_component_filters(const LoggingConfig& config) {
    if (!config.enable_component_logging) {
        // Component logging disabled, clear all filters
        clear_component_filters();
        return;
    }

    // Clear existing filters first
    s_enabled_components.clear();
    s_disabled_components.clear();

    // Apply new filters from config
    if (!config.enabled_components.empty()) {
        s_enabled_components = config.enabled_components;
    }

    if (!config.disabled_components.empty()) {
        s_disabled_components = config.disabled_components;
    }
}

std::vector<std::string> Logger::get_enabled_components() {
    return s_enabled_components;
}

std::vector<std::string> Logger::get_disabled_components() {
    return s_disabled_components;
}

bool Logger::reload_config(std::string_view app_name) {
    auto& config_mgr = ConfigManager::instance();

    // First try to reload the config file
    if (!config_mgr.reload_config()) {
        return false;
    }

    // Get the updated logging configuration
    auto logging_config = config_mgr.get_logging_config(app_name);

    // Apply the new log level
    set_level(logging_config.level);

    // Apply the new component filter settings
    configure_component_filters(logging_config);

    return true;
}

} // namespace base

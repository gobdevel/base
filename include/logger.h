/*
 * @file logger.h
 * @brief Modern C++20 thread-safe logging wrapper for spdlog
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <string_view>
#include <source_location>
#include <memory>
#include <filesystem>

namespace crux {

/**
 * @brief Log level enumeration for type-safe level specification.
 */
enum class LogLevel : int {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warn = 3,
    Error = 4,
    Critical = 5,
    Off = 6
};

/**
 * @brief Modern C++20 thread-safe logging utility with console and file output options.
 *
 * Features:
 * - C++20 source location support
 * - Type-safe log levels
 * - Structured logging support
 * - Automatic log rotation
 * - Multiple output sinks
 *
 * Usage:
 * @code
 * Logger::init();  // Console output
 * Logger::init(LoggerConfig{.app_name = "MyApp", .log_file = "app.log"});
 * Logger::info("User {} logged in", username);
 * Logger::error("Failed with code: {}", error_code);
 * @endcode
 */
/**
 * @brief Configuration structure for logger initialization.
 */
struct LoggerConfig {
    std::string app_name = "crux";
    std::filesystem::path log_file{};
    std::size_t max_file_size = 5 * 1024 * 1024;  // 5MB
    std::size_t max_files = 3;
    LogLevel level = LogLevel::Info;
    bool enable_console = true;
    bool enable_file = false;
    bool enable_colors = true;  // Enable colored console output
    std::string pattern = "[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v";  // %^%l%$ for colored level
};

class Logger {
public:
    /**
     * @brief Initialize console logger with colored output.
     */
    static void init();

    /**
     * @brief Initialize logger with comprehensive configuration.
     * @param config Logger configuration
     */
    static void init(const LoggerConfig& config);

    /**
     * @brief Initialize rotating file logger (legacy interface).
     * @param app_name Logger identifier
     * @param filename Log file path
     * @deprecated Use init(LoggerConfig) instead
     */
    [[deprecated("Use init(LoggerConfig) instead")]]
    static void init(std::string_view app_name, std::string_view filename);

    /**
     * @brief Check if logger is initialized.
     * @return true if logger is ready for use
     */
    static bool is_initialized() noexcept { return s_logger != nullptr; }

    /**
     * @brief Get current log level.
     * @return Current log level
     */
    static LogLevel get_level() noexcept;

    /**
     * @brief Log trace message with C++20 format and source location.
     * @param fmt Format string
     * @param args Format arguments
     */
    template <typename... Args>
    static void trace(std::string_view fmt, Args&&... args) {
        if (s_logger && s_logger->should_log(spdlog::level::trace)) {
            log_with_location(spdlog::level::trace, fmt, std::source_location::current(), std::forward<Args>(args)...);
        }
    }

    /**
     * @brief Log debug message with C++20 format and source location.
     * @param fmt Format string
     * @param args Format arguments
     */
    template <typename... Args>
    static void debug(std::string_view fmt, Args&&... args) {
        if (s_logger && s_logger->should_log(spdlog::level::debug)) {
            log_with_location(spdlog::level::debug, fmt, std::source_location::current(), std::forward<Args>(args)...);
        }
    }

    /**
     * @brief Log info message with C++20 format and source location.
     * @param fmt Format string
     * @param args Format arguments
     */
    template <typename... Args>
    static void info(std::string_view fmt, Args&&... args) {
        if (s_logger && s_logger->should_log(spdlog::level::info)) {
            log_with_location(spdlog::level::info, fmt, std::source_location::current(), std::forward<Args>(args)...);
        }
    }

    /**
     * @brief Log warning message with C++20 format and source location.
     * @param fmt Format string
     * @param args Format arguments
     */
    template <typename... Args>
    static void warn(std::string_view fmt, Args&&... args) {
        if (s_logger && s_logger->should_log(spdlog::level::warn)) {
            log_with_location(spdlog::level::warn, fmt, std::source_location::current(), std::forward<Args>(args)...);
        }
    }

    /**
     * @brief Log error message with C++20 format and source location.
     * @param fmt Format string
     * @param args Format arguments
     */
    template <typename... Args>
    static void error(std::string_view fmt, Args&&... args) {
        if (s_logger && s_logger->should_log(spdlog::level::err)) {
            log_with_location(spdlog::level::err, fmt, std::source_location::current(), std::forward<Args>(args)...);
        }
    }

    /**
     * @brief Log critical message with C++20 format and source location.
     * @param fmt Format string
     * @param args Format arguments
     */
    template <typename... Args>
    static void critical(std::string_view fmt, Args&&... args) {
        if (s_logger && s_logger->should_log(spdlog::level::critical)) {
            log_with_location(spdlog::level::critical, fmt, std::source_location::current(), std::forward<Args>(args)...);
        }
    }

    /**
     * @brief Set minimum log level using type-safe enum.
     * @param level Log level
     */
    static void set_level(LogLevel level) noexcept;

    /**
     * @brief Set minimum log level using string (legacy interface).
     * @param loglevel Log level string
     * @deprecated Use set_level(LogLevel) instead
     */
    [[deprecated("Use set_level(LogLevel) instead")]]
    static void setLogLevel(std::string_view loglevel);

    /**
     * @brief Flush all pending log messages.
     */
    static void flush() noexcept;

    /**
     * @brief Shutdown logger and cleanup resources.
     */
    static void shutdown() noexcept;

private:
    static std::shared_ptr<spdlog::logger> s_logger;

    /**
     * @brief Internal logging function with source location support.
     */
    template <typename... Args>
    static void log_with_location(spdlog::level::level_enum level,
                                 std::string_view fmt,
                                 const std::source_location& loc,
                                 Args&&... args) {
        spdlog::source_loc source_loc{loc.file_name(), static_cast<int>(loc.line()), loc.function_name()};

        if constexpr (sizeof...(args) > 0) {
            // Use spdlog's runtime format to avoid consteval issues
            s_logger->log(source_loc, level, fmt::runtime(fmt), std::forward<Args>(args)...);
        } else {
            s_logger->log(source_loc, level, fmt::runtime(fmt));
        }
    }

    /**
     * @brief Convert LogLevel enum to spdlog level.
     */
    static spdlog::level::level_enum to_spdlog_level(LogLevel level) noexcept;

    /**
     * @brief Convert spdlog level to LogLevel enum.
     */
    static LogLevel from_spdlog_level(spdlog::level::level_enum level) noexcept;
};

} // namespace crux

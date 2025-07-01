/*
 * @file logger.h
 * @brief Thread-safe logging utility wrapper for spdlog library
 * @author Gobind Prasad <gobdeveloper@gmail.com>
 * @date 2025
 * @version 1.0
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <spdlog/common.h>
#include <spdlog/spdlog.h>

namespace crux {

/**
 * @brief A thread-safe logging utility wrapper around spdlog library.
 *
 * The Logger class provides a static interface for logging functionality
 * with support for different log levels (info, warn, error) and configurable
 * output destinations (console or rotating file).
 *
 * @note This class is designed as a singleton pattern with static methods.
 * @note All logging operations are thread-safe when using spdlog backend.
 *
 * Example usage:
 * @code
 * // Initialize console logger
 * crux::Logger::init();
 *
 * // Or initialize file logger
 * crux::Logger::init("MyApp", "logs/app.log");
 *
 * // Log messages
 * crux::Logger::info("Application started");
 * crux::Logger::warn("Low memory warning: {}MB remaining", 128);
 * crux::Logger::error("Failed to connect to database: {}", error_msg);
 * @endcode
 */
class Logger {
public:
    /**
     * @brief Initialize the logger with console output.
     *
     * Creates a colored console logger that outputs to stdout.
     * This is suitable for development and debugging purposes.
     *
     * @note If a logger is already initialized, this will replace it.
     */
    static void init();

    /**
     * @brief Initialize the logger with rotating file output.
     *
     * Creates a rotating file logger that automatically manages log file
     * rotation based on size limits. When the maximum file size is reached,
     * the current log file is archived and a new one is created.
     *
     * @param appName The name identifier for the logger instance
     * @param filename The path to the log file (directories will be created if needed)
     *
     * @note Maximum file size is set to 5MB with up to 3 archived files
     * @note If a logger is already initialized, this will replace it
     */
    static void init(const std::string appName, const std::string filename);

    /**
     * @brief Log an informational message.
     *
     * Logs a message at INFO level with optional formatted arguments.
     * Use this for general information about application flow and state.
     *
     * @tparam Args Variadic template parameter types for formatting arguments
     * @param msg The message string, may contain format specifiers (e.g., "{}", "{0}")
     * @param args Optional arguments to be formatted into the message string
     *
     * @note Uses fmt library formatting syntax
     * @note No-op if logger is not initialized
     *
     * Example:
     * @code
     * Logger::info("User {} logged in from IP {}", username, ip_address);
     * Logger::info("Processing {} files", file_count);
     * @endcode
     */
    template <typename... Args>
    static void info(const std::string msg, Args &&...args) {
        if (s_logger) {
            s_logger->info(msg, std::forward<Args>(args)...);
        }
    }

    /**
     * @brief Log a warning message.
     *
     * Logs a message at WARN level with optional formatted arguments.
     * Use this for potentially harmful situations that don't prevent
     * application execution but should be noted.
     *
     * @tparam Args Variadic template parameter types for formatting arguments
     * @param msg The message string, may contain format specifiers (e.g., "{}", "{0}")
     * @param args Optional arguments to be formatted into the message string
     *
     * @note Uses fmt library formatting syntax
     * @note No-op if logger is not initialized
     *
     * Example:
     * @code
     * Logger::warn("Memory usage is high: {}%", memory_percentage);
     * Logger::warn("Deprecated function called: {}", function_name);
     * @endcode
     */
    template <typename... Args>
    static void warn(const std::string msg, Args &&...args) {
        if (s_logger) {
            s_logger->warn(msg, std::forward<Args>(args)...);
        }
    }

    /**
     * @brief Log an error message.
     *
     * Logs a message at ERROR level with optional formatted arguments.
     * Use this for error conditions that impact functionality but
     * allow the application to continue running.
     *
     * @tparam Args Variadic template parameter types for formatting arguments
     * @param msg The message string, may contain format specifiers (e.g., "{}", "{0}")
     * @param args Optional arguments to be formatted into the message string
     *
     * @note Uses fmt library formatting syntax
     * @note No-op if logger is not initialized
     *
     * Example:
     * @code
     * Logger::error("Failed to open file: {}", filename);
     * Logger::error("Database connection failed with code: {}", error_code);
     * @endcode
     */
    template <typename... Args>
    static void error(const std::string msg, Args &&...args) {
        if (s_logger) {
            s_logger->error(msg, std::forward<Args>(args)...);
        }
    }

    /**
     * @brief Set the minimum log level for filtering messages.
     *
     * Configure the logger to only output messages at or above the specified level.
     * Messages below this level will be filtered out and not logged.
     *
     * @param loglevel String representation of the log level.
     *                 Valid values: "trace", "debug", "info", "warn", "error", "critical", "off"
     *
     * @note Case-insensitive string matching
     * @note Invalid level strings will result in undefined behavior
     * @note No-op if logger is not initialized
     *
     * Example:
     * @code
     * Logger::setLogLevel("warn");  // Only warn, error, and critical messages will be logged
     * Logger::setLogLevel("debug"); // All messages except trace will be logged
     * @endcode
     */
    static void setLogLevel(const std::string &loglevel) {
        auto level = spdlog::level::from_str(loglevel);
        s_logger->set_level(level);
    }

private:
    /**
     * @brief Shared pointer to the underlying spdlog logger instance.
     *
     * This static member holds the singleton logger instance used by all
     * logging operations. It's initialized by calling one of the init() methods
     * and is shared across all threads in the application.
     *
     * @note Thread-safe access is provided by the spdlog library
     * @note Nullptr when not initialized
     */
    static std::shared_ptr<spdlog::logger> s_logger;
};
}  // namespace crux

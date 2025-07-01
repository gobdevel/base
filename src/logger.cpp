/*
 * @file logger.cpp
 * @brief Implementation of thread-safe logging utility wrapper for spdlog library
 * @author Gobind Prasad <gobdeveloper@gmail.com>
 * @date 2025
 * @version 1.0
 *
 * This file contains the implementation of the Logger class methods for
 * initializing console and rotating file loggers using the spdlog library.
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * SPDX-License-Identifier: MIT
 */

#include <logger.h>

// spdlog includes for different sink types
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace crux {

// Static member definition - shared across all instances and threads
std::shared_ptr<spdlog::logger> Logger::s_logger{nullptr};

/**
 * @brief Initialize console logger with colored output.
 *
 * Creates a multi-threaded console logger that outputs colored log messages
 * to stdout. This is the simplest initialization method, suitable for
 * development, debugging, and applications that don't require persistent logging.
 *
 * The logger is created with the name "console" and uses spdlog's
 * stdout_color_mt sink which provides:
 * - Thread-safe operations
 * - Colored output based on log level
 * - Direct output to standard output stream
 *
 * @note Replaces any existing logger instance
 * @note Thread-safe operation guaranteed by spdlog
 *
 * @see spdlog::stdout_color_mt() for underlying implementation details
 */
void Logger::init() {
    s_logger = spdlog::stdout_color_mt("console");
}

/**
 * @brief Initialize rotating file logger with specified configuration.
 *
 * Creates a multi-threaded rotating file logger that automatically manages
 * log file rotation based on file size limits. This is suitable for production
 * applications that require persistent logging with automatic log management.
 *
 * Configuration details:
 * - Maximum file size: 5MB (5,242,880 bytes)
 * - Maximum number of archived files: 3
 * - Rotation behavior: When max size is reached, current file is renamed
 *   with a numeric suffix and a new log file is created
 *
 * File naming convention:
 * - Active log: {filename}
 * - Archived logs: {filename}.1, {filename}.2, {filename}.3
 *
 * @param appName The application name used as logger identifier
 * @param filename The path to the log file (parent directories must exist)
 *
 * @note If a logger already exists, it will be properly cleaned up before
 *       creating the new one
 * @note Thread-safe operation guaranteed by spdlog
 * @note Parent directories of the log file must exist, or an exception will be thrown
 *
 * @throws spdlog::spdlog_ex if file creation fails or directory doesn't exist
 *
 * @see spdlog::rotating_logger_mt() for underlying implementation details
 */
void Logger::init(const std::string appName, const std::string filename) {
    // Configuration constants for file rotation
    constexpr auto max_size  = 1048576 * 5;  // 5MB in bytes
    constexpr auto max_files = 3;            // Keep 3 archived log files

    // Clean up existing logger if present to prevent resource leaks
    if (s_logger) {
        s_logger.reset();
    }

    // Create the rotating file logger with specified parameters
    s_logger =
        spdlog::rotating_logger_mt(appName, filename, max_size, max_files);
}
}  // namespace crux

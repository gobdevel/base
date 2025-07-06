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
#include <vector>

namespace base {

// Forward declarations
struct LoggingConfig;

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
 * @brief Convenience macro for creating component loggers with cleaner syntax.
 *
 * @code
 * COMPONENT_LOGGER(database);  // Creates: auto database = base::Logger::get_component_logger("database");
 * database.info("Query executed successfully");
 * @endcode
 */
#define COMPONENT_LOGGER(name) auto name = base::Logger::get_component_logger(#name)

/**
 * @brief Convenience macro for creating component loggers with custom names.
 *
 * @code
 * COMPONENT_LOGGER_NAMED(db, "Database");  // Creates: auto db = base::Logger::get_component_logger("Database");
 * db.info("Connection established");
 * @endcode
 */
#define COMPONENT_LOGGER_NAMED(var, name) auto var = base::Logger::get_component_logger(name)

/**
 * @brief Modern C++20 thread-safe logging utility with console and file output options.
 *
 * The Logger class provides a comprehensive logging solution built on top of spdlog,
 * featuring component-level filtering, type-safe log levels, and modern C++20 features.
 *
 * Key Features:
 * - C++20 source location support for automatic file/line information
 * - Type-safe log levels with LogLevel enum
 * - Component-level logging and filtering for modular applications
 * - Structured logging support with fmt library
 * - Automatic log rotation and multiple output sinks
 * - Thread-safe operation
 * - Zero-overhead when logging is disabled
 *
 * Basic Usage:
 * @code
 * // Initialize with console output only
 * Logger::init();
 * Logger::info("Application started");
 *
 * // Initialize with file output
 * LoggerConfig config{
 *     .app_name = "MyApp",
 *     .log_file = "logs/app.log",
 *     .level = LogLevel::Debug
 * };
 * Logger::init(config);
 *
 * // Basic logging
 * Logger::info("User {} logged in at {}", username, timestamp);
 * Logger::error("Database connection failed: {}", error_msg);
 * @endcode
 *
 * Component-Level Logging:
 * @code
 * // Component-tagged messages
 * Logger::info(Logger::Component("Network"), "Connected to server {}", server_addr);
 * Logger::debug(Logger::Component("Database"), "Query executed in {}ms", duration);
 *
 * // Filter by components
 * Logger::enable_components({"Network", "Authentication"});
 * Logger::disable_components({"Debug", "Verbose"});
 * @endcode
 */
/**
 * @brief Configuration structure for logger initialization.
 *
 * Provides comprehensive configuration options for both basic logging
 * and advanced component-level filtering.
 *
 * @code
 * LoggerConfig config{
 *     .app_name = "MyApp",
 *     .log_file = "logs/app.log",
 *     .level = LogLevel::Debug,
 *     .enable_component_logging = true,
 *     .enabled_components = {"Network", "Database"}
 * };
 * Logger::init(config);
 * @endcode
 */
struct LoggerConfig {
    // Basic logging configuration
    std::string app_name = "base";                                  ///< Application identifier in logs
    std::filesystem::path log_file{};                               ///< Log file path (empty = no file logging)
    std::size_t max_file_size = 5 * 1024 * 1024;                   ///< Maximum file size before rotation (5MB)
    std::size_t max_files = 3;                                      ///< Number of rotated files to keep
    LogLevel level = LogLevel::Info;                                ///< Minimum log level to output
    bool enable_console = true;                                     ///< Enable console output
    bool enable_file = false;                                       ///< Enable file output
    bool enable_colors = true;                                      ///< Enable colored console output
    std::string pattern = "[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v"; ///< Log message pattern (%^%l%$ = colored level)

    // Component-level logging configuration
    bool enable_component_logging = true;                           ///< Enable component-based logging and filtering
    std::vector<std::string> enabled_components{};                  ///< Only log these components (empty = all enabled)
    std::vector<std::string> disabled_components{};                 ///< Exclude these components from logging
    std::string component_pattern = "[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v"; ///< Pattern for component logging
};

class ComponentLogger; // Forward declarations

/**
 * @brief Component-specific logger that automatically prepends component name to all logs.
 *
 * This class provides a convenient way to create component-specific loggers that automatically
 * include the component name in all log messages without requiring it to be passed each time.
 *
 * Usage:
 * @code
 * auto db_logger = Logger::get_component_logger("Database");
 * auto net_logger = Logger::get_component_logger("Network");
 *
 * db_logger.info("Connection established");     // Logs: [Database] Connection established
 * net_logger.error("Request failed: {}", err);  // Logs: [Network] Request failed: timeout
 * @endcode
 */
class ComponentLogger {
public:
    explicit ComponentLogger(std::string_view component_name)
        : component_name_(component_name) {}

    // TRACE LEVEL
    template <typename... Args>
    void trace(std::string_view fmt, Args&&... args) const;

    // DEBUG LEVEL
    template <typename... Args>
    void debug(std::string_view fmt, Args&&... args) const;

    // INFO LEVEL
    template <typename... Args>
    void info(std::string_view fmt, Args&&... args) const;

    // WARNING LEVEL
    template <typename... Args>
    void warn(std::string_view fmt, Args&&... args) const;

    // ERROR LEVEL
    template <typename... Args>
    void error(std::string_view fmt, Args&&... args) const;

    // CRITICAL LEVEL
    template <typename... Args>
    void critical(std::string_view fmt, Args&&... args) const;

    /**
     * @brief Get the component name associated with this logger.
     * @return Component name
     */
    std::string_view get_component_name() const noexcept { return component_name_; }

private:
    std::string component_name_;
};

class Logger {
private:
    // Static members must be declared first
    static std::shared_ptr<spdlog::logger> s_logger;
    static bool s_component_logging_enabled;
    static std::vector<std::string> s_enabled_components;
    static std::vector<std::string> s_disabled_components;

public:
    /**
     * @brief Component wrapper for explicit component logging.
     *
     * Use this wrapper to tag log messages with a specific component:
     * @code
     * Logger::info(Logger::Component("Network"), "Connection established to {}", server);
     * @endcode
     */
    struct Component {
        std::string_view name;
        explicit Component(std::string_view n) : name(n) {}
    };

    // ======================================
    // INITIALIZATION AND LIFECYCLE
    // ======================================

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
    static bool is_initialized() noexcept { return get_internal_logger() != nullptr; }

    /**
     * @brief Flush all pending log messages.
     */
    static void flush() noexcept;

    /**
     * @brief Shutdown logger and cleanup resources.
     */
    static void shutdown() noexcept;

    /**
     * @brief Reload logger configuration from config manager.
     * @param app_name Application name to get logging config from
     * @return true if logger was reconfigured successfully
     */
    static bool reload_config(std::string_view app_name = "default");

    // ======================================
    // LOG LEVEL MANAGEMENT
    // ======================================

    /**
     * @brief Get current log level.
     * @return Current log level
     */
    static LogLevel get_level() noexcept;

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

    // ======================================
    // COMPONENT FILTERING
    // ======================================

    /**
     * @brief Check if a component is enabled for logging.
     * @param component Component name
     * @return true if component logging is enabled
     */
    static bool is_component_enabled(std::string_view component) noexcept;

    /**
     * @brief Enable logging for specific components.
     * @param components List of component names to enable
     */
    static void enable_components(const std::vector<std::string>& components);

    /**
     * @brief Disable logging for specific components.
     * @param components List of component names to disable
     */
    static void disable_components(const std::vector<std::string>& components);

    /**
     * @brief Clear all component filters (enable all).
     */
    static void clear_component_filters();

    /**
     * @brief Get list of currently enabled components.
     * @return Vector of enabled component names (empty means all enabled)
     */
    static std::vector<std::string> get_enabled_components();

    /**
     * @brief Get list of currently disabled components.
     * @return Vector of disabled component names
     */
    static std::vector<std::string> get_disabled_components();

    /**
     * @brief Configure component filters from LoggingConfig.
     * @param config Logging configuration containing component filter settings
     */
    static void configure_component_filters(const LoggingConfig& config);

    // ======================================
    // LOGGING METHODS
    // ======================================

    // ======================================
    // LOGGING METHODS
    // ======================================

    // TRACE LEVEL
    /**
     * @brief Log trace message with C++20 format and source location.
     * @param fmt Format string
     * @param args Format arguments
     */
    template <typename... Args>
    static void trace(std::string_view fmt, Args&&... args) {
        if (s_logger && s_logger->should_log(spdlog::level::trace)) {
            log_with_location(spdlog::level::trace, "", fmt, std::source_location::current(), std::forward<Args>(args)...);
        }
    }

    /**
     * @brief Log trace message with component and C++20 format.
     * @param comp Component wrapper
     * @param fmt Format string
     * @param args Format arguments
     */
    template <typename... Args>
    static void trace(Component comp, std::string_view fmt, Args&&... args) {
        if (s_logger && s_logger->should_log(spdlog::level::trace) && is_component_enabled(comp.name)) {
            log_with_location(spdlog::level::trace, comp.name, fmt, std::source_location::current(), std::forward<Args>(args)...);
        }
    }

    // DEBUG LEVEL
    /**
     * @brief Log debug message with C++20 format and source location.
     * @param fmt Format string
     * @param args Format arguments
     */
    template <typename... Args>
    static void debug(std::string_view fmt, Args&&... args) {
        if (s_logger && s_logger->should_log(spdlog::level::debug)) {
            log_with_location(spdlog::level::debug, "", fmt, std::source_location::current(), std::forward<Args>(args)...);
        }
    }

    /**
     * @brief Log debug message with component and C++20 format.
     * @param comp Component wrapper
     * @param fmt Format string
     * @param args Format arguments
     */
    template <typename... Args>
    static void debug(Component comp, std::string_view fmt, Args&&... args) {
        if (s_logger && s_logger->should_log(spdlog::level::debug) && is_component_enabled(comp.name)) {
            log_with_location(spdlog::level::debug, comp.name, fmt, std::source_location::current(), std::forward<Args>(args)...);
        }
    }

    // INFO LEVEL

    // INFO LEVEL
    /**
     * @brief Log info message with C++20 format and source location.
     * @param fmt Format string
     * @param args Format arguments
     */
    template <typename... Args>
    static void info(std::string_view fmt, Args&&... args) {
        if (s_logger && s_logger->should_log(spdlog::level::info)) {
            log_with_location(spdlog::level::info, "", fmt, std::source_location::current(), std::forward<Args>(args)...);
        }
    }

    /**
     * @brief Log info message with component and C++20 format.
     * @param comp Component wrapper
     * @param fmt Format string
     * @param args Format arguments
     */
    template <typename... Args>
    static void info(Component comp, std::string_view fmt, Args&&... args) {
        if (s_logger && s_logger->should_log(spdlog::level::info) && is_component_enabled(comp.name)) {
            log_with_location(spdlog::level::info, comp.name, fmt, std::source_location::current(), std::forward<Args>(args)...);
        }
    }

    // WARNING LEVEL
    /**
     * @brief Log warning message with C++20 format and source location.
     * @param fmt Format string
     * @param args Format arguments
     */
    template <typename... Args>
    static void warn(std::string_view fmt, Args&&... args) {
        if (s_logger && s_logger->should_log(spdlog::level::warn)) {
            log_with_location(spdlog::level::warn, "", fmt, std::source_location::current(), std::forward<Args>(args)...);
        }
    }

    /**
     * @brief Log warning message with component and C++20 format.
     * @param comp Component wrapper
     * @param fmt Format string
     * @param args Format arguments
     */
    template <typename... Args>
    static void warn(Component comp, std::string_view fmt, Args&&... args) {
        if (s_logger && s_logger->should_log(spdlog::level::warn) && is_component_enabled(comp.name)) {
            log_with_location(spdlog::level::warn, comp.name, fmt, std::source_location::current(), std::forward<Args>(args)...);
        }
    }

    // ERROR LEVEL
    /**
     * @brief Log error message with C++20 format and source location.
     * @param fmt Format string
     * @param args Format arguments
     */
    template <typename... Args>
    static void error(std::string_view fmt, Args&&... args) {
        if (s_logger && s_logger->should_log(spdlog::level::err)) {
            log_with_location(spdlog::level::err, "", fmt, std::source_location::current(), std::forward<Args>(args)...);
        }
    }

    /**
     * @brief Log error message with component and C++20 format.
     * @param comp Component wrapper
     * @param fmt Format string
     * @param args Format arguments
     */
    template <typename... Args>
    static void error(Component comp, std::string_view fmt, Args&&... args) {
        if (s_logger && s_logger->should_log(spdlog::level::err) && is_component_enabled(comp.name)) {
            log_with_location(spdlog::level::err, comp.name, fmt, std::source_location::current(), std::forward<Args>(args)...);
        }
    }

    // CRITICAL LEVEL
    /**
     * @brief Log critical message with C++20 format and source location.
     * @param fmt Format string
     * @param args Format arguments
     */
    template <typename... Args>
    static void critical(std::string_view fmt, Args&&... args) {
        if (s_logger && s_logger->should_log(spdlog::level::critical)) {
            log_with_location(spdlog::level::critical, "", fmt, std::source_location::current(), std::forward<Args>(args)...);
        }
    }

    /**
     * @brief Log critical message with component and C++20 format.
     * @param comp Component wrapper
     * @param fmt Format string
     * @param args Format arguments
     */
    template <typename... Args>
    static void critical(Component comp, std::string_view fmt, Args&&... args) {
        if (s_logger && s_logger->should_log(spdlog::level::critical) && is_component_enabled(comp.name)) {
            log_with_location(spdlog::level::critical, comp.name, fmt, std::source_location::current(), std::forward<Args>(args)...);
        }
    }

    // ======================================
    // CONVENIENCE METHODS
    // ======================================

    /**
     * @brief Create a Component wrapper for cleaner syntax.
     * @param name Component name
     * @return Component wrapper
     *
     * @code
     * auto net = Logger::component("Network");
     * Logger::info(net, "Connected to {}", server);
     * @endcode
     */
    static Component component(std::string_view name) { return Component(name); }

    /**
     * @brief Check if logger is ready for use (convenience alias).
     * @return true if logger is initialized
     */
    static bool ready() noexcept { return is_initialized(); }

    /**
     * @brief Create a component-specific logger that automatically prepends component name.
     * @param component_name Name of the component
     * @return ComponentLogger instance for the specified component
     *
     * @code
     * auto db_logger = Logger::get_component_logger("Database");
     * auto net_logger = Logger::get_component_logger("Network");
     *
     * db_logger.info("Connected to server");        // Logs: [Database] Connected to server
     * net_logger.error("Connection failed: {}", e); // Logs: [Network] Connection failed: timeout
     * @endcode
     */
    static ComponentLogger get_component_logger(std::string_view component_name) {
        return ComponentLogger(component_name);
    }

    // ======================================
    // CONVENIENCE METHODS
    // ======================================

    // ======================================
    // LEGACY METHODS (DEPRECATED)
    // ======================================

private:
    // Allow ComponentLogger to access internal methods
    friend class ComponentLogger;

    // ======================================
    // INTERNAL HELPER METHODS
    // ======================================

    /**
     * @brief Internal logging function with source location and component support.
     */
    template <typename... Args>
    static void log_with_location(spdlog::level::level_enum level,
                                 std::string_view component,
                                 std::string_view fmt,
                                 const std::source_location& loc,
                                 Args&&... args) {
        if (!s_logger) return;

        spdlog::source_loc source_loc{loc.file_name(), static_cast<int>(loc.line()), loc.function_name()};

        // Format the original message first
        std::string formatted_message;
        if constexpr (sizeof...(args) > 0) {
            formatted_message = fmt::format(fmt::runtime(std::string{fmt}), std::forward<Args>(args)...);
        } else {
            formatted_message = std::string{fmt};
        }

        // If component logging is enabled and we have a component, prepend it to the message
        if (!component.empty() && s_component_logging_enabled) {
            formatted_message = fmt::format("[{}] {}", component, formatted_message);
        }

        // Use spdlog's log method with {} format to avoid re-interpretation as format string
        s_logger->log(source_loc, level, "{}", formatted_message);
    }

    /**
     * @brief Convert LogLevel enum to spdlog level.
     */
    static spdlog::level::level_enum to_spdlog_level(LogLevel level) noexcept;

    /**
     * @brief Convert spdlog level to LogLevel enum.
     */
    static LogLevel from_spdlog_level(spdlog::level::level_enum level) noexcept;

    // ======================================
    // INTERNAL ACCESSORS FOR COMPONENTLOGGER
    // ======================================

    /**
     * @brief Get the internal spdlog logger (for ComponentLogger access).
     * @return Shared pointer to spdlog logger
     */
    static std::shared_ptr<spdlog::logger> get_internal_logger() noexcept { return s_logger; }

    /**
     * @brief Internal logging function accessible to ComponentLogger.
     */
    template <typename... Args>
    static void internal_log_with_location(spdlog::level::level_enum level,
                                          std::string_view component,
                                          std::string_view fmt,
                                          const std::source_location& loc,
                                          Args&&... args) {
        log_with_location(level, component, fmt, loc, std::forward<Args>(args)...);
    }

private:
};

// ======================================
// COMPONENTLOGGER TEMPLATE DEFINITIONS
// ======================================

template <typename... Args>
void ComponentLogger::trace(std::string_view fmt, Args&&... args) const {
    auto logger = Logger::get_internal_logger();
    if (logger && logger->should_log(spdlog::level::trace) && Logger::is_component_enabled(component_name_)) {
        Logger::internal_log_with_location(spdlog::level::trace, component_name_, fmt, std::source_location::current(), std::forward<Args>(args)...);
    }
}

template <typename... Args>
void ComponentLogger::debug(std::string_view fmt, Args&&... args) const {
    auto logger = Logger::get_internal_logger();
    if (logger && logger->should_log(spdlog::level::debug) && Logger::is_component_enabled(component_name_)) {
        Logger::internal_log_with_location(spdlog::level::debug, component_name_, fmt, std::source_location::current(), std::forward<Args>(args)...);
    }
}

template <typename... Args>
void ComponentLogger::info(std::string_view fmt, Args&&... args) const {
    auto logger = Logger::get_internal_logger();
    if (logger && logger->should_log(spdlog::level::info) && Logger::is_component_enabled(component_name_)) {
        Logger::internal_log_with_location(spdlog::level::info, component_name_, fmt, std::source_location::current(), std::forward<Args>(args)...);
    }
}

template <typename... Args>
void ComponentLogger::warn(std::string_view fmt, Args&&... args) const {
    auto logger = Logger::get_internal_logger();
    if (logger && logger->should_log(spdlog::level::warn) && Logger::is_component_enabled(component_name_)) {
        Logger::internal_log_with_location(spdlog::level::warn, component_name_, fmt, std::source_location::current(), std::forward<Args>(args)...);
    }
}

template <typename... Args>
void ComponentLogger::error(std::string_view fmt, Args&&... args) const {
    auto logger = Logger::get_internal_logger();
    if (logger && logger->should_log(spdlog::level::err) && Logger::is_component_enabled(component_name_)) {
        Logger::internal_log_with_location(spdlog::level::err, component_name_, fmt, std::source_location::current(), std::forward<Args>(args)...);
    }
}

template <typename... Args>
void ComponentLogger::critical(std::string_view fmt, Args&&... args) const {
    auto logger = Logger::get_internal_logger();
    if (logger && logger->should_log(spdlog::level::critical) && Logger::is_component_enabled(component_name_)) {
        Logger::internal_log_with_location(spdlog::level::critical, component_name_, fmt, std::source_location::current(), std::forward<Args>(args)...);
    }
}

} // namespace base

// ======================================
// CONVENIENCE MACROS (OPTIONAL)
// ======================================

/**
 * @brief Convenience macro for creating component loggers with cleaner syntax.
 *
 * @code
 * LOGGER_COMPONENT(network);  // Creates: auto network = base::Logger::component("network");
 * base::Logger::info(network, "Connected to server");
 * @endcode
 */
#define LOGGER_COMPONENT(name) auto name = base::Logger::component(#name)

/**
 * @brief Convenience macro for logging with implicit component creation.
 *
 * @code
 * LOGGER_INFO("network", "Connected to server {}", addr);
 * // Equivalent to: base::Logger::info(base::Logger::component("network"), "Connected to server {}", addr);
 * @endcode
 */
#define LOGGER_TRACE(comp, ...) base::Logger::trace(base::Logger::component(comp), __VA_ARGS__)
#define LOGGER_DEBUG(comp, ...) base::Logger::debug(base::Logger::component(comp), __VA_ARGS__)
#define LOGGER_INFO(comp, ...)  base::Logger::info(base::Logger::component(comp), __VA_ARGS__)
#define LOGGER_WARN(comp, ...)  base::Logger::warn(base::Logger::component(comp), __VA_ARGS__)
#define LOGGER_ERROR(comp, ...) base::Logger::error(base::Logger::component(comp), __VA_ARGS__)
#define LOGGER_CRITICAL(comp, ...) base::Logger::critical(base::Logger::component(comp), __VA_ARGS__)

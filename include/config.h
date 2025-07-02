/*
 * @file config.h
 * @brief TOML-based configuration parser and handler with per-app support
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "singleton.h"
#include "logger.h"
#include <toml++/toml.hpp>
#include <string>
#include <string_view>
#include <filesystem>
#include <optional>
#include <memory>
#include <unordered_map>
#include <vector>
#include <shared_mutex>
#include <source_location>

namespace crux {

/**
 * @brief Configuration section for logging settings
 */
struct LoggingConfig {
    LogLevel level = LogLevel::Info;         ///< Default log level
    std::string pattern = "[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v"; ///< Log format pattern
    std::string file_path;                   ///< Optional log file path (empty = console only)
    bool enable_console = true;              ///< Enable console logging
    bool enable_file = false;                ///< Enable file logging
    size_t max_file_size = 10 * 1024 * 1024; ///< Max file size in bytes (10MB)
    size_t max_files = 5;                    ///< Max number of rotating files
    bool flush_immediately = false;          ///< Flush after each log message
};

/**
 * @brief Configuration section for application settings
 */
struct AppConfig {
    std::string name = "crux_app";           ///< Application name
    std::string version = "1.0.0";          ///< Application version
    std::string description;                 ///< Application description
    bool debug_mode = false;                 ///< Debug mode flag
    int worker_threads = 1;                  ///< Number of worker threads
    std::string working_directory;           ///< Working directory path
};

/**
 * @brief Configuration section for network settings
 */
struct NetworkConfig {
    std::string host = "localhost";          ///< Server host
    int port = 8080;                        ///< Server port
    int timeout_seconds = 30;               ///< Connection timeout
    int max_connections = 100;              ///< Maximum concurrent connections
    bool enable_ssl = false;                ///< Enable SSL/TLS
    std::string ssl_cert_path;              ///< SSL certificate path
    std::string ssl_key_path;               ///< SSL private key path
};

/**
 * @brief Main configuration container holding all app configurations
 */
class ConfigManager : public SingletonBase<ConfigManager> {
    friend class SingletonBase<ConfigManager>;

public:
    /**
     * @brief Load configuration from TOML file
     * @param config_path Path to the TOML configuration file
     * @param app_name Application name to load config for
     * @return true if configuration was loaded successfully
     */
    bool load_config(const std::filesystem::path& config_path,
                    std::string_view app_name = "default",
                    const std::source_location& location = std::source_location::current());

    /**
     * @brief Load configuration from TOML string content
     * @param toml_content TOML configuration content as string
     * @param app_name Application name to load config for
     * @return true if configuration was loaded successfully
     */
    bool load_from_string(std::string_view toml_content,
                         std::string_view app_name = "default",
                         const std::source_location& location = std::source_location::current());

    /**
     * @brief Get logging configuration for an application
     * @param app_name Application name
     * @return Logging configuration, or default if not found
     */
    LoggingConfig get_logging_config(std::string_view app_name = "default") const;

    /**
     * @brief Get application configuration for an application
     * @param app_name Application name
     * @return Application configuration, or default if not found
     */
    AppConfig get_app_config(std::string_view app_name = "default") const;

    /**
     * @brief Get network configuration for an application
     * @param app_name Application name
     * @return Network configuration, or default if not found
     */
    NetworkConfig get_network_config(std::string_view app_name = "default") const;

    /**
     * @brief Get a custom configuration value
     * @tparam T The type to convert the value to
     * @param key Configuration key in dot notation (e.g., "app.database.host")
     * @param app_name Application name
     * @return Optional value if found and convertible to T
     */
    template<typename T>
    std::optional<T> get_value(std::string_view key, std::string_view app_name = "default") const;

    /**
     * @brief Get a custom configuration value with default fallback
     * @tparam T The type to convert the value to
     * @param key Configuration key in dot notation
     * @param default_value Default value to return if key not found
     * @param app_name Application name
     * @return Configuration value or default
     */
    template<typename T>
    T get_value_or(std::string_view key, const T& default_value, std::string_view app_name = "default") const;

    /**
     * @brief Check if configuration exists for an application
     * @param app_name Application name
     * @return true if configuration exists
     */
    bool has_app_config(std::string_view app_name) const;

    /**
     * @brief Get list of all configured application names
     * @return Vector of application names
     */
    std::vector<std::string> get_app_names() const;

    /**
     * @brief Apply logging configuration to the logger
     * @param app_name Application name to get logging config from
     * @param logger_name Name of the logger to configure (defaults to app_name)
     * @return true if logger was configured successfully
     */
    bool configure_logger(std::string_view app_name = "default",
                         std::string_view logger_name = "") const;

    /**
     * @brief Create default configuration file template
     * @param config_path Path where to create the template
     * @param app_name Application name to use in template
     * @return true if template was created successfully
     */
    static bool create_config_template(const std::filesystem::path& config_path,
                                     std::string_view app_name = "default");

    /**
     * @brief Reload configuration from the last loaded file
     * @return true if configuration was reloaded successfully
     */
    bool reload_config();

    /**
     * @brief Clear all loaded configurations
     */
    void clear();

private:
    ConfigManager() = default;
    ~ConfigManager() = default;

    // Helper methods
    LoggingConfig parse_logging_config(const toml::table& app_table) const;
    AppConfig parse_app_config(const toml::table& app_table) const;
    NetworkConfig parse_network_config(const toml::table& app_table) const;

    LogLevel parse_log_level(std::string_view level_str) const;
    std::string log_level_to_string(LogLevel level) const;

    // Internal storage
    std::unordered_map<std::string, toml::table> app_configs_;
    std::filesystem::path last_config_path_;
    mutable std::shared_mutex config_mutex_;
};

// Template implementations
template<typename T>
std::optional<T> ConfigManager::get_value(std::string_view key, std::string_view app_name) const {
    std::shared_lock lock(config_mutex_);

    auto app_it = app_configs_.find(std::string(app_name));
    if (app_it == app_configs_.end()) {
        return std::nullopt;
    }

    try {
        // Parse dot-notation key path
        std::vector<std::string> path_parts;
        std::string current_part;
        for (char c : key) {
            if (c == '.') {
                if (!current_part.empty()) {
                    path_parts.push_back(std::move(current_part));
                    current_part.clear();
                }
            } else {
                current_part += c;
            }
        }
        if (!current_part.empty()) {
            path_parts.push_back(std::move(current_part));
        }

        // Navigate through the TOML structure
        const toml::table* current_table = &app_it->second;
        for (size_t i = 0; i < path_parts.size() - 1; ++i) {
            auto node = current_table->get(path_parts[i]);
            if (!node || !node->is_table()) {
                return std::nullopt;
            }
            current_table = node->as_table();
        }

        // Get the final value
        if (!path_parts.empty()) {
            auto node = current_table->get(path_parts.back());
            if (node) {
                if constexpr (std::is_same_v<T, std::string>) {
                    if (auto val = node->value<std::string>()) {
                        return *val;
                    }
                } else if constexpr (std::is_same_v<T, int64_t>) {
                    if (auto val = node->value<int64_t>()) {
                        return *val;
                    }
                } else if constexpr (std::is_same_v<T, int>) {
                    if (auto val = node->value<int64_t>()) {
                        return static_cast<int>(*val);
                    }
                } else if constexpr (std::is_same_v<T, double>) {
                    if (auto val = node->value<double>()) {
                        return *val;
                    }
                } else if constexpr (std::is_same_v<T, bool>) {
                    if (auto val = node->value<bool>()) {
                        return *val;
                    }
                }
            }
        }
    } catch (const std::exception&) {
        // Log error but don't throw
    }

    return std::nullopt;
}

template<typename T>
T ConfigManager::get_value_or(std::string_view key, const T& default_value, std::string_view app_name) const {
    auto value = get_value<T>(key, app_name);
    return value.value_or(default_value);
}

} // namespace crux

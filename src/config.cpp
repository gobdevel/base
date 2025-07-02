/*
 * @file config.cpp
 * @brief TOML-based configuration parser and handler implementation
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"
#include <fstream>
#include <iostream>
#include <shared_mutex>

namespace crux {

bool ConfigManager::load_config(const std::filesystem::path& config_path,
                                std::string_view app_name,
                                const std::source_location& location) {
    try {
        std::unique_lock lock(config_mutex_);

        if (!std::filesystem::exists(config_path)) {
            Logger::error("Configuration file not found: {}", config_path.string());
            return false;
        }

        auto config = toml::parse_file(config_path.string());
        last_config_path_ = config_path;

        // Clear existing configs for this app
        std::string app_key(app_name);
        app_configs_.erase(app_key);

        // Check if we have app-specific configuration
        if (auto app_table = config.get(app_key)) {
            if (app_table->is_table()) {
                app_configs_[app_key] = *app_table->as_table();
                Logger::info("Loaded configuration for app '{}' from {}", app_name, config_path.string());
            } else {
                Logger::error("App configuration '{}' is not a table", app_name);
                return false;
            }
        } else {
            // If no app-specific config, use the root as the app config
            app_configs_[app_key] = config;
            Logger::info("Loaded root configuration as app '{}' from {}", app_name, config_path.string());
        }

        return true;
    } catch (const toml::parse_error& e) {
        Logger::error("TOML parse error in {}: {}", config_path.string(), e.description());
        return false;
    } catch (const std::exception& e) {
        Logger::error("Error loading config from {}: {}", config_path.string(), e.what());
        return false;
    }
}

bool ConfigManager::load_from_string(std::string_view toml_content,
                                    std::string_view app_name,
                                    const std::source_location& location) {
    try {
        std::unique_lock lock(config_mutex_);

        auto config = toml::parse(toml_content);
        last_config_path_.clear(); // No file path for string content

        // Clear existing configs for this app
        std::string app_key(app_name);
        app_configs_.erase(app_key);

        // Check if we have app-specific configuration
        if (auto app_table = config.get(app_key)) {
            if (app_table->is_table()) {
                app_configs_[app_key] = *app_table->as_table();
                Logger::info("Loaded configuration for app '{}' from string", app_name);
            } else {
                Logger::error("App configuration '{}' is not a table", app_name);
                return false;
            }
        } else {
            // If no app-specific config, use the root as the app config
            app_configs_[app_key] = config;
            Logger::info("Loaded root configuration as app '{}' from string", app_name);
        }

        return true;
    } catch (const toml::parse_error& e) {
        Logger::error("TOML parse error: {}", e.description());
        return false;
    } catch (const std::exception& e) {
        Logger::error("Error loading config from string: {}", e.what());
        return false;
    }
}

LoggingConfig ConfigManager::get_logging_config(std::string_view app_name) const {
    std::shared_lock lock(config_mutex_);

    auto app_it = app_configs_.find(std::string(app_name));
    if (app_it != app_configs_.end()) {
        return parse_logging_config(app_it->second);
    }

    return LoggingConfig{}; // Return default config
}

AppConfig ConfigManager::get_app_config(std::string_view app_name) const {
    std::shared_lock lock(config_mutex_);

    auto app_it = app_configs_.find(std::string(app_name));
    if (app_it != app_configs_.end()) {
        return parse_app_config(app_it->second);
    }

    return AppConfig{}; // Return default config
}

NetworkConfig ConfigManager::get_network_config(std::string_view app_name) const {
    std::shared_lock lock(config_mutex_);

    auto app_it = app_configs_.find(std::string(app_name));
    if (app_it != app_configs_.end()) {
        return parse_network_config(app_it->second);
    }

    return NetworkConfig{}; // Return default config
}

bool ConfigManager::has_app_config(std::string_view app_name) const {
    std::shared_lock lock(config_mutex_);
    return app_configs_.find(std::string(app_name)) != app_configs_.end();
}

std::vector<std::string> ConfigManager::get_app_names() const {
    std::shared_lock lock(config_mutex_);

    std::vector<std::string> names;
    names.reserve(app_configs_.size());

    for (const auto& [name, _] : app_configs_) {
        names.push_back(name);
    }

    return names;
}

bool ConfigManager::configure_logger(std::string_view app_name,
                                    std::string_view logger_name) const {
    auto logging_config = get_logging_config(app_name);

    std::string actual_logger_name = logger_name.empty() ? std::string(app_name) : std::string(logger_name);

    LoggerConfig logger_config;
    logger_config.app_name = actual_logger_name;
    logger_config.level = logging_config.level;
    logger_config.pattern = logging_config.pattern;
    logger_config.enable_console = logging_config.enable_console;
    logger_config.enable_file = logging_config.enable_file;
    logger_config.log_file = logging_config.file_path;
    logger_config.max_file_size = logging_config.max_file_size;
    logger_config.max_files = logging_config.max_files;

    Logger::init(logger_config);
    return true;
}

bool ConfigManager::create_config_template(const std::filesystem::path& config_path,
                                          std::string_view app_name) {
    try {
        std::ofstream file(config_path);
        if (!file.is_open()) {
            return false;
        }

        file << "# Configuration template for " << app_name << "\n"
             << "# Generated by crux::ConfigManager\n\n"
             << "[" << app_name << "]\n\n"
             << "# Application settings\n"
             << "[" << app_name << ".app]\n"
             << "name = \"" << app_name << "\"\n"
             << "version = \"1.0.0\"\n"
             << "description = \"My application\"\n"
             << "debug_mode = false\n"
             << "worker_threads = 4\n"
             << "working_directory = \"/tmp\"\n\n"
             << "# Logging configuration\n"
             << "[" << app_name << ".logging]\n"
             << "level = \"info\"  # debug, info, warn, error, critical\n"
             << "pattern = \"[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v\"\n"
             << "enable_console = true\n"
             << "enable_file = false\n"
             << "file_path = \"logs/" << app_name << ".log\"\n"
             << "max_file_size = 10485760  # 10MB\n"
             << "max_files = 5\n"
             << "flush_immediately = false\n\n"
             << "# Network configuration\n"
             << "[" << app_name << ".network]\n"
             << "host = \"localhost\"\n"
             << "port = 8080\n"
             << "timeout_seconds = 30\n"
             << "max_connections = 100\n"
             << "enable_ssl = false\n"
             << "ssl_cert_path = \"\"\n"
             << "ssl_key_path = \"\"\n\n"
             << "# Custom configuration sections\n"
             << "[" << app_name << ".database]\n"
             << "host = \"localhost\"\n"
             << "port = 5432\n"
             << "name = \"mydb\"\n"
             << "user = \"dbuser\"\n"
             << "password = \"dbpass\"\n"
             << "max_connections = 10\n\n"
             << "[" << app_name << ".cache]\n"
             << "redis_host = \"localhost\"\n"
             << "redis_port = 6379\n"
             << "ttl_seconds = 3600\n";

        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool ConfigManager::reload_config() {
    if (last_config_path_.empty()) {
        Logger::warn("No config file path stored, cannot reload");
        return false;
    }

    // Get all current app names before clearing
    auto app_names = get_app_names();

    bool all_success = true;
    for (const auto& app_name : app_names) {
        if (!load_config(last_config_path_, app_name)) {
            all_success = false;
        }
    }

    return all_success;
}

void ConfigManager::clear() {
    std::unique_lock lock(config_mutex_);
    app_configs_.clear();
    last_config_path_.clear();
}

// Private helper methods
LoggingConfig ConfigManager::parse_logging_config(const toml::table& app_table) const {
    LoggingConfig config;

    if (auto logging_table = app_table.get("logging")) {
        if (logging_table->is_table()) {
            const auto& logging = *logging_table->as_table();

            if (auto level = logging.get("level")) {
                if (auto level_str = level->value<std::string>()) {
                    config.level = parse_log_level(*level_str);
                }
            }

            if (auto pattern = logging.get("pattern")) {
                if (auto pattern_str = pattern->value<std::string>()) {
                    config.pattern = *pattern_str;
                }
            }

            if (auto file_path = logging.get("file_path")) {
                if (auto path_str = file_path->value<std::string>()) {
                    config.file_path = *path_str;
                }
            }

            if (auto enable_console = logging.get("enable_console")) {
                if (auto enable = enable_console->value<bool>()) {
                    config.enable_console = *enable;
                }
            }

            if (auto enable_file = logging.get("enable_file")) {
                if (auto enable = enable_file->value<bool>()) {
                    config.enable_file = *enable;
                }
            }

            if (auto max_size = logging.get("max_file_size")) {
                if (auto size = max_size->value<int64_t>()) {
                    config.max_file_size = static_cast<size_t>(*size);
                }
            }

            if (auto max_files = logging.get("max_files")) {
                if (auto files = max_files->value<int64_t>()) {
                    config.max_files = static_cast<size_t>(*files);
                }
            }

            if (auto flush = logging.get("flush_immediately")) {
                if (auto flush_val = flush->value<bool>()) {
                    config.flush_immediately = *flush_val;
                }
            }
        }
    }

    return config;
}

AppConfig ConfigManager::parse_app_config(const toml::table& app_table) const {
    AppConfig config;

    if (auto app_config_table = app_table.get("app")) {
        if (app_config_table->is_table()) {
            const auto& app = *app_config_table->as_table();

            if (auto name = app.get("name")) {
                if (auto name_str = name->value<std::string>()) {
                    config.name = *name_str;
                }
            }

            if (auto version = app.get("version")) {
                if (auto version_str = version->value<std::string>()) {
                    config.version = *version_str;
                }
            }

            if (auto description = app.get("description")) {
                if (auto desc_str = description->value<std::string>()) {
                    config.description = *desc_str;
                }
            }

            if (auto debug = app.get("debug_mode")) {
                if (auto debug_val = debug->value<bool>()) {
                    config.debug_mode = *debug_val;
                }
            }

            if (auto threads = app.get("worker_threads")) {
                if (auto thread_count = threads->value<int64_t>()) {
                    config.worker_threads = static_cast<int>(*thread_count);
                }
            }

            if (auto work_dir = app.get("working_directory")) {
                if (auto dir_str = work_dir->value<std::string>()) {
                    config.working_directory = *dir_str;
                }
            }
        }
    }

    return config;
}

NetworkConfig ConfigManager::parse_network_config(const toml::table& app_table) const {
    NetworkConfig config;

    if (auto network_table = app_table.get("network")) {
        if (network_table->is_table()) {
            const auto& network = *network_table->as_table();

            if (auto host = network.get("host")) {
                if (auto host_str = host->value<std::string>()) {
                    config.host = *host_str;
                }
            }

            if (auto port = network.get("port")) {
                if (auto port_val = port->value<int64_t>()) {
                    config.port = static_cast<int>(*port_val);
                }
            }

            if (auto timeout = network.get("timeout_seconds")) {
                if (auto timeout_val = timeout->value<int64_t>()) {
                    config.timeout_seconds = static_cast<int>(*timeout_val);
                }
            }

            if (auto max_conn = network.get("max_connections")) {
                if (auto max_val = max_conn->value<int64_t>()) {
                    config.max_connections = static_cast<int>(*max_val);
                }
            }

            if (auto ssl = network.get("enable_ssl")) {
                if (auto ssl_val = ssl->value<bool>()) {
                    config.enable_ssl = *ssl_val;
                }
            }

            if (auto cert = network.get("ssl_cert_path")) {
                if (auto cert_str = cert->value<std::string>()) {
                    config.ssl_cert_path = *cert_str;
                }
            }

            if (auto key = network.get("ssl_key_path")) {
                if (auto key_str = key->value<std::string>()) {
                    config.ssl_key_path = *key_str;
                }
            }
        }
    }

    return config;
}

LogLevel ConfigManager::parse_log_level(std::string_view level_str) const {
    if (level_str == "debug") return LogLevel::Debug;
    if (level_str == "info") return LogLevel::Info;
    if (level_str == "warn" || level_str == "warning") return LogLevel::Warn;
    if (level_str == "error") return LogLevel::Error;
    if (level_str == "critical") return LogLevel::Critical;

    Logger::warn("Unknown log level '{}', using Info", level_str);
    return LogLevel::Info;
}

std::string ConfigManager::log_level_to_string(LogLevel level) const {
    switch (level) {
        case LogLevel::Debug: return "debug";
        case LogLevel::Info: return "info";
        case LogLevel::Warn: return "warn";
        case LogLevel::Error: return "error";
        case LogLevel::Critical: return "critical";
        default: return "info";
    }
}

} // namespace crux

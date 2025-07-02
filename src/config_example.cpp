/*
 * @file config_example.cpp
 * @brief Example application demonstrating the TOML configuration system
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"
#include "logger.h"
#include <iostream>
#include <filesystem>

using namespace crux;

void demonstrate_config_loading() {
    std::cout << "\n=== Configuration Loading Demo ===\n";

    auto& config = ConfigManager::instance();

    // Create a sample configuration template
    const std::string config_path = "demo_config.toml";
    if (!std::filesystem::exists(config_path)) {
        std::cout << "Creating configuration template: " << config_path << "\n";
        ConfigManager::create_config_template(config_path, "demo_app");
    }

    // Load configuration
    std::cout << "Loading configuration from: " << config_path << "\n";
    if (!config.load_config(config_path, "demo_app")) {
        std::cerr << "Failed to load configuration!\n";
        return;
    }

    std::cout << "✓ Configuration loaded successfully\n";
}

void demonstrate_app_config() {
    std::cout << "\n=== Application Configuration ===\n";

    auto& config = ConfigManager::instance();
    auto app_config = config.get_app_config("demo_app");

    std::cout << "App Name: " << app_config.name << "\n";
    std::cout << "Version: " << app_config.version << "\n";
    std::cout << "Description: " << app_config.description << "\n";
    std::cout << "Debug Mode: " << (app_config.debug_mode ? "enabled" : "disabled") << "\n";
    std::cout << "Worker Threads: " << app_config.worker_threads << "\n";
    std::cout << "Working Directory: " << app_config.working_directory << "\n";
}

void demonstrate_logging_config() {
    std::cout << "\n=== Logging Configuration ===\n";

    auto& config = ConfigManager::instance();
    auto logging_config = config.get_logging_config("demo_app");

    std::cout << "Log Level: ";
    switch (logging_config.level) {
        case LogLevel::Debug: std::cout << "Debug"; break;
        case LogLevel::Info: std::cout << "Info"; break;
        case LogLevel::Warn: std::cout << "Warning"; break;
        case LogLevel::Error: std::cout << "Error"; break;
        case LogLevel::Critical: std::cout << "Critical"; break;
        default: std::cout << "Unknown"; break;
    }
    std::cout << "\n";

    std::cout << "Pattern: " << logging_config.pattern << "\n";
    std::cout << "Console Enabled: " << (logging_config.enable_console ? "yes" : "no") << "\n";
    std::cout << "File Enabled: " << (logging_config.enable_file ? "yes" : "no") << "\n";
    std::cout << "File Path: " << logging_config.file_path << "\n";
    std::cout << "Max File Size: " << logging_config.max_file_size << " bytes\n";
    std::cout << "Max Files: " << logging_config.max_files << "\n";
    std::cout << "Flush Immediately: " << (logging_config.flush_immediately ? "yes" : "no") << "\n";

    // Configure the logger with these settings
    std::cout << "\nConfiguring logger with loaded settings...\n";
    if (config.configure_logger("demo_app")) {
        std::cout << "✓ Logger configured successfully\n";
    } else {
        std::cout << "✗ Failed to configure logger\n";
    }
}

void demonstrate_network_config() {
    std::cout << "\n=== Network Configuration ===\n";

    auto& config = ConfigManager::instance();
    auto network_config = config.get_network_config("demo_app");

    std::cout << "Host: " << network_config.host << "\n";
    std::cout << "Port: " << network_config.port << "\n";
    std::cout << "Timeout: " << network_config.timeout_seconds << " seconds\n";
    std::cout << "Max Connections: " << network_config.max_connections << "\n";
    std::cout << "SSL Enabled: " << (network_config.enable_ssl ? "yes" : "no") << "\n";
    std::cout << "SSL Cert Path: " << network_config.ssl_cert_path << "\n";
    std::cout << "SSL Key Path: " << network_config.ssl_key_path << "\n";
}

void demonstrate_custom_values() {
    std::cout << "\n=== Custom Configuration Values ===\n";

    auto& config = ConfigManager::instance();

    // Try to get database configuration
    auto db_host = config.get_value_or<std::string>("database.host", "localhost", "demo_app");
    auto db_port = config.get_value_or<int>("database.port", 5432, "demo_app");
    auto db_name = config.get_value_or<std::string>("database.name", "default_db", "demo_app");
    auto db_max_conn = config.get_value_or<int>("database.max_connections", 10, "demo_app");

    std::cout << "Database Host: " << db_host << "\n";
    std::cout << "Database Port: " << db_port << "\n";
    std::cout << "Database Name: " << db_name << "\n";
    std::cout << "Database Max Connections: " << db_max_conn << "\n";

    // Try to get cache configuration
    auto cache_host = config.get_value_or<std::string>("cache.redis_host", "localhost", "demo_app");
    auto cache_port = config.get_value_or<int>("cache.redis_port", 6379, "demo_app");
    auto cache_ttl = config.get_value_or<int>("cache.ttl_seconds", 3600, "demo_app");

    std::cout << "\nCache Host: " << cache_host << "\n";
    std::cout << "Cache Port: " << cache_port << "\n";
    std::cout << "Cache TTL: " << cache_ttl << " seconds\n";
}

void demonstrate_logging_with_config() {
    std::cout << "\n=== Logging with Configuration ===\n";

    // The logger should now be configured with settings from the config file
    Logger::info("This is an info message using configured logger");
    Logger::warn("This is a warning message with value: {}", 42);
    Logger::error("This is an error message");

    if (Logger::get_level() == LogLevel::Debug) {
        Logger::debug("Debug logging is enabled - this message should appear");
    }

    std::cout << "✓ Log messages sent (check console and/or log file based on configuration)\n";
}

void demonstrate_multi_app_config() {
    std::cout << "\n=== Multi-Application Configuration ===\n";

    auto& config = ConfigManager::instance();

    // Load configuration for another app using string content
    std::string other_app_config = R"(
[other_service]

[other_service.app]
name = "other_service"
version = "2.1.0"
description = "Another service with different configuration"
debug_mode = false
worker_threads = 2

[other_service.logging]
level = "warn"
enable_console = true
enable_file = false
pattern = "[%H:%M:%S] [%l] %v"

[other_service.network]
host = "0.0.0.0"
port = 9000
timeout_seconds = 15
)";

    if (config.load_from_string(other_app_config, "other_service")) {
        std::cout << "✓ Loaded configuration for 'other_service'\n";

        auto other_app = config.get_app_config("other_service");
        std::cout << "Other Service Name: " << other_app.name << "\n";
        std::cout << "Other Service Version: " << other_app.version << "\n";
        std::cout << "Other Service Worker Threads: " << other_app.worker_threads << "\n";

        auto other_logging = config.get_logging_config("other_service");
        std::cout << "Other Service Log Level: ";
        switch (other_logging.level) {
            case LogLevel::Warn: std::cout << "Warning"; break;
            default: std::cout << "Other"; break;
        }
        std::cout << "\n";

        // Show all loaded app names
        auto app_names = config.get_app_names();
        std::cout << "\nAll configured applications: ";
        for (size_t i = 0; i < app_names.size(); ++i) {
            std::cout << app_names[i];
            if (i < app_names.size() - 1) std::cout << ", ";
        }
        std::cout << "\n";
    }
}

int main() {
    std::cout << "TOML Configuration System Demo\n";
    std::cout << "==============================\n";

    try {
        demonstrate_config_loading();
        demonstrate_app_config();
        demonstrate_logging_config();
        demonstrate_network_config();
        demonstrate_custom_values();
        demonstrate_logging_with_config();
        demonstrate_multi_app_config();

        std::cout << "\n=== Demo Complete ===\n";
        std::cout << "✓ All configuration features demonstrated successfully!\n";
        std::cout << "\nConfiguration files created:\n";
        std::cout << "- demo_config.toml (template configuration)\n";
        std::cout << "\nYou can modify the configuration file and run the demo again to see changes.\n";

    } catch (const std::exception& e) {
        std::cerr << "Error during demo: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

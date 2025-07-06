/*
 * @file component_logging_example.cpp
 * @brief Example demonstrating runtime component-level logging and filtering via config
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include <application.h>
#include <logger.h>
#include <config.h>
#include <chrono>
#include <thread>
#include <fmt/ranges.h>
#include <filesystem>
#include <unistd.h>

using namespace base;

/**
 * @brief Example application demonstrating runtime component-level logging features.
 */
class ComponentLoggingApp : public Application {
public:
    ComponentLoggingApp() : Application(create_config()) {}

protected:
    bool on_initialize() override {
        // Load configuration from file
        auto& config_mgr = ConfigManager::instance();
        std::filesystem::path config_path = "examples/component_demo.toml";

        if (!config_mgr.load_config(config_path, "ComponentLoggingDemo")) {
            Logger::warn("Failed to load config from {}, using defaults", config_path.string());
        } else {
            Logger::info("Loaded configuration from {}", config_path.string());
        }

        // Configure logger from loaded config
        auto logging_config = config_mgr.get_logging_config("ComponentLoggingDemo");
        Logger::init(LoggerConfig{
            .app_name = "ComponentDemo",
            .level = logging_config.level,
            .enable_console = logging_config.enable_console,
            .enable_file = logging_config.enable_file,
            .enable_colors = true,
            .pattern = logging_config.pattern,
            .enable_component_logging = logging_config.enable_component_logging,
            .enabled_components = logging_config.enabled_components,
            .disabled_components = logging_config.disabled_components,
            .component_pattern = logging_config.component_pattern
        });

        Logger::info("Application starting up...");
        Logger::info("Configuration-based component filtering is active");
        Logger::info("You can modify examples/component_demo.toml and send SIGHUP to reload filters");

        // Show current filter state
        show_current_filters();

        return true;
    }

    bool on_start() override {
        Logger::info("Application running - demonstrating runtime component logging");
        Logger::info("Try editing examples/component_demo.toml and run: kill -HUP {}", getpid());

        // Start continuous logging from different components
        schedule_continuous_logging();

        return true;
    }

    bool on_config_reload() override {
        Logger::info("=== Configuration Reloaded ===");
        show_current_filters();
        return true;
    }

    void on_cleanup() override {
        Logger::info("Shutting down component logging demo");
    }

private:
    static ApplicationConfig create_config() {
        return ApplicationConfig{
            .name = "ComponentLoggingDemo",
            .version = "1.0.0",
            .description = "Demonstrates runtime component-level logging and filtering"
        };
    }

    void show_current_filters() {
        auto enabled = Logger::get_enabled_components();
        auto disabled = Logger::get_disabled_components();

        std::string enabled_str = enabled.empty() ? "ALL" : fmt::format("[{}]", fmt::join(enabled.begin(), enabled.end(), ", "));
        std::string disabled_str = disabled.empty() ? "NONE" : fmt::format("[{}]", fmt::join(disabled.begin(), disabled.end(), ", "));

        Logger::info("Current enabled components: {}", enabled_str);
        Logger::info("Current disabled components: {}", disabled_str);
    }

    void schedule_continuous_logging() {
        // Schedule recurring tasks to generate logs from different components
        schedule_recurring_task([this]() {
            simulate_database_operations();
        }, std::chrono::seconds(3));

        schedule_recurring_task([this]() {
            simulate_network_operations();
        }, std::chrono::seconds(4));

        schedule_recurring_task([this]() {
            simulate_auth_operations();
        }, std::chrono::seconds(5));

        schedule_recurring_task([this]() {
            demonstrate_automatic_component_logging();
        }, std::chrono::seconds(6));

        // Schedule shutdown after demonstration
        schedule_recurring_task([this]() {
            Logger::info("Component logging demonstration complete - shutting down application");
            shutdown();
        }, std::chrono::seconds(30));
    }

    void simulate_database_operations() {
        static int counter = 0;
        ++counter;

        Logger::debug(Logger::Component("database"), "Database operation #{}: Connecting to server", counter);
        Logger::info(Logger::Component("database"), "Database operation #{}: Query executed successfully", counter);
        if (counter % 3 == 0) {
            Logger::warn(Logger::Component("database"), "Database operation #{}: Slow query detected", counter);
        }
    }

    void simulate_network_operations() {
        static int counter = 0;
        ++counter;

        Logger::trace(Logger::Component("network"), "Network operation #{}: Opening connection", counter);
        Logger::info(Logger::Component("network"), "Network operation #{}: HTTP request completed", counter);
        if (counter % 4 == 0) {
            Logger::error(Logger::Component("network"), "Network operation #{}: Connection timeout", counter);
        }
    }

    void simulate_auth_operations() {
        static int counter = 0;
        ++counter;

        Logger::debug(Logger::Component("auth"), "Auth operation #{}: Validating credentials", counter);
        Logger::info(Logger::Component("auth"), "Auth operation #{}: User authenticated", counter);
        if (counter % 5 == 0) {
            Logger::critical(Logger::Component("auth"), "Auth operation #{}: Security alert!", counter);
        }
    }

    void demonstrate_automatic_component_logging() {
        static int counter = 0;
        ++counter;

        // Create component loggers that automatically prepend their names
        auto cache = Logger::get_component_logger("cache");
        auto storage = Logger::get_component_logger("storage");

        cache.info("Cache operation #{}: Processing request", counter);
        cache.debug("Cache operation #{}: Hit rate: {}%", counter, 85 + (counter % 10));

        storage.info("Storage operation #{}: File operation", counter);
        if (counter % 3 == 0) {
            storage.warn("Storage operation #{}: Disk space low", counter);
        }
    }
};

BASE_APPLICATION_MAIN(ComponentLoggingApp)

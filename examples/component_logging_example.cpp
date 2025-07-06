/*
 * @file component_logging_example.cpp
 * @brief Comprehensive example demonstrating all component logging features
 *
 * Features demonstrated:
 * - Basic component logging with Logger::Component
 * - Automatic component logging with ComponentLogger
 * - Runtime configuration-based filtering
 * - SIGHUP signal handling for config reload
 * - Convenience macros for component loggers
 * - Component filtering (enable/disable)
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
 * @brief Comprehensive component logging demonstration application
 */
class ComponentLoggingApp : public Application {
public:
    ComponentLoggingApp() : Application(create_config()) {}

protected:
    bool on_initialize() override {
        Logger::info("=== Component Logging Comprehensive Demo ===");

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

        Logger::info("Configuration-based component filtering is active");
        Logger::info("You can modify examples/component_demo.toml and send SIGHUP to reload filters");
        Logger::info("Process ID: {} - Use: kill -HUP {}", getpid(), getpid());

        // Show current filter state
        show_current_filters();

        // Demonstrate all component logging features
        demonstrate_basic_component_logging();
        demonstrate_automatic_component_logging();
        demonstrate_convenience_macros();
        demonstrate_programmatic_filtering();

        return true;
    }

    bool on_start() override {
        Logger::info("Application running - demonstrating continuous component logging");

        // Start continuous logging from different components
        schedule_continuous_logging();

        return true;
    }

    bool on_config_reload() override {
        Logger::info("=== Configuration Reloaded ===");
        show_current_filters();
        return true;
    }

private:
    static ApplicationConfig create_config() {
        return ApplicationConfig{
            .name = "ComponentLoggingDemo",
            .version = "1.0.0",
            .description = "Comprehensive component logging demonstration"
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

    void demonstrate_basic_component_logging() {
        Logger::info("=== 1. Basic Component Logging (Explicit Tagging) ===");

        Logger::trace(Logger::Component("database"), "Connecting to database server");
        Logger::debug(Logger::Component("database"), "Connection pool initialized with 10 connections");
        Logger::info(Logger::Component("database"), "Database connection established");
        Logger::warn(Logger::Component("database"), "Query took longer than expected: 250ms");
        Logger::error(Logger::Component("database"), "Failed to execute query: syntax error");
        Logger::critical(Logger::Component("database"), "Database server is unreachable!");

        Logger::debug(Logger::Component("network"), "Opening TCP socket");
        Logger::info(Logger::Component("network"), "HTTP server started on port 8080");
        Logger::warn(Logger::Component("network"), "Connection timeout, retrying...");
        Logger::error(Logger::Component("network"), "Failed to bind to port 8080");

        Logger::info(Logger::Component("auth"), "Loading user permissions from database");
        Logger::warn(Logger::Component("auth"), "User session expires in 5 minutes");
        Logger::error(Logger::Component("auth"), "Invalid credentials for user 'admin'");
        Logger::critical(Logger::Component("auth"), "Multiple failed login attempts detected!");
    }

    void demonstrate_automatic_component_logging() {
        Logger::info("=== 2. Automatic Component Logging (ComponentLogger) ===");

        // Create component loggers that automatically prepend their names
        auto database = Logger::get_component_logger("Database");
        auto network = Logger::get_component_logger("Network");
        auto auth = Logger::get_component_logger("Authentication");
        auto cache = Logger::get_component_logger("Cache");

        // Each logger automatically includes its component name
        database.trace("Preparing query statement");
        database.debug("Executing query: SELECT * FROM users WHERE active = 1");
        database.info("Query executed successfully in 15ms");
        database.warn("Connection pool usage: 85%");
        database.error("Deadlock detected, retrying transaction");

        network.debug("Incoming HTTP request: GET /api/users");
        network.info("Response sent: 200 OK (24 users)");
        network.warn("Rate limit approaching for client 192.168.1.100");
        network.error("Failed to connect to external API: timeout");

        auth.info("User 'alice' logged in from 10.0.0.15");
        auth.warn("Password expires in 7 days for user 'bob'");
        auth.error("Account locked after 3 failed attempts: user 'charlie'");
        auth.critical("Potential brute force attack detected from 192.168.1.200");

        cache.debug("Cache key 'user:123' not found, fetching from database");
        cache.info("Cache hit ratio: 92.5% (excellent)");
        cache.warn("Cache memory usage: 78% of 1GB limit");
        cache.error("Failed to serialize object for caching");
    }

    void demonstrate_convenience_macros() {
        Logger::info("=== 3. Convenience Macros ===");

        // Using convenience macros for even cleaner syntax
        COMPONENT_LOGGER(storage);
        COMPONENT_LOGGER_NAMED(fs, "FileSystem");
        COMPONENT_LOGGER_NAMED(metrics, "Metrics");

        storage.debug("Initializing storage subsystem");
        storage.info("Storage backend: PostgreSQL 14.2");
        storage.warn("Disk space low: 15% remaining on /data");
        storage.error("Failed to create backup: insufficient space");

        fs.debug("Scanning directory: /var/log/app");
        fs.info("Log rotation completed: archived 5 files");
        fs.warn("File descriptor limit approaching: 85% used");
        fs.error("Permission denied: cannot write to /etc/app.conf");

        metrics.debug("Collecting performance metrics");
        metrics.info("Average response time: 45ms");
        metrics.warn("CPU usage spike: 95% for 30 seconds");
        metrics.error("Metrics collection failed: time series DB unavailable");
    }

    void demonstrate_programmatic_filtering() {
        Logger::info("=== 4. Programmatic Component Filtering ===");

        // Show initial state
        Logger::info("Initial filter state:");
        show_current_filters();

        // Test disabling specific components
        Logger::info("--- Disabling 'database' and 'cache' components ---");
        Logger::disable_components({"database", "cache"});

        Logger::info(Logger::Component("database"), "This message should NOT appear");
        Logger::info(Logger::Component("network"), "This network message should appear");
        Logger::info(Logger::Component("cache"), "This cache message should NOT appear");
        Logger::info(Logger::Component("auth"), "This auth message should appear");

        // Test enabling only specific components
        Logger::info("--- Enabling only 'auth' and 'storage' components ---");
        Logger::enable_components({"auth", "storage"});

        Logger::info(Logger::Component("database"), "Database: should NOT appear");
        Logger::info(Logger::Component("network"), "Network: should NOT appear");
        Logger::info(Logger::Component("auth"), "Auth: should appear");
        Logger::info(Logger::Component("storage"), "Storage: should appear");

        // Clear all filters
        Logger::info("--- Clearing all filters (enable all) ---");
        Logger::clear_component_filters();

        Logger::info(Logger::Component("database"), "Database: should appear again");
        Logger::info(Logger::Component("network"), "Network: should appear again");
        Logger::info(Logger::Component("auth"), "Auth: still appears");
        Logger::info(Logger::Component("storage"), "Storage: still appears");

        Logger::info("Programmatic filtering demonstration complete");
        show_current_filters();
    }

    void schedule_continuous_logging() {
        // Schedule recurring tasks to generate logs from different components
        schedule_recurring_task([this]() {
            simulate_database_operations();
        }, std::chrono::seconds(4));

        schedule_recurring_task([this]() {
            simulate_network_operations();
        }, std::chrono::seconds(5));

        schedule_recurring_task([this]() {
            simulate_auth_operations();
        }, std::chrono::seconds(6));

        schedule_recurring_task([this]() {
            simulate_cache_operations();
        }, std::chrono::seconds(7));

        // Schedule shutdown after demonstration
        schedule_recurring_task([this]() {
            Logger::info("Component logging demonstration complete - shutting down");
            Logger::info("Try modifying examples/component_demo.toml and sending SIGHUP!");
            shutdown();
        }, std::chrono::seconds(30));
    }

    void simulate_database_operations() {
        static int counter = 0;
        ++counter;

        auto db = Logger::get_component_logger("Database");
        db.debug("Operation #{}: Connection pool check", counter);
        db.info("Operation #{}: Query executed successfully", counter);
        if (counter % 3 == 0) {
            db.warn("Operation #{}: Slow query detected (>100ms)", counter);
        }
        if (counter % 7 == 0) {
            db.error("Operation #{}: Connection timeout, retrying...", counter);
        }
    }

    void simulate_network_operations() {
        static int counter = 0;
        ++counter;

        Logger::trace(Logger::Component("network"), "Request #{}: Parsing HTTP headers", counter);
        Logger::info(Logger::Component("network"), "Request #{}: GET /api/data - 200 OK", counter);
        if (counter % 4 == 0) {
            Logger::warn(Logger::Component("network"), "Request #{}: High latency: 250ms", counter);
        }
        if (counter % 8 == 0) {
            Logger::error(Logger::Component("network"), "Request #{}: Gateway timeout", counter);
        }
    }

    void simulate_auth_operations() {
        static int counter = 0;
        ++counter;

        COMPONENT_LOGGER(auth);
        auth.debug("Session #{}: Validating JWT token", counter);
        auth.info("Session #{}: User authenticated successfully", counter);
        if (counter % 5 == 0) {
            auth.warn("Session #{}: Token expires in 2 minutes", counter);
        }
        if (counter % 9 == 0) {
            auth.critical("Session #{}: Suspicious activity detected!", counter);
        }
    }

    void simulate_cache_operations() {
        static int counter = 0;
        ++counter;

        COMPONENT_LOGGER_NAMED(cache, "Cache");
        cache.debug("Cache operation #{}: Key lookup", counter);
        cache.info("Cache operation #{}: Hit rate: {}%", counter, 85 + (counter % 10));
        if (counter % 6 == 0) {
            cache.warn("Cache operation #{}: Memory usage high: 89%", counter);
        }
    }
};

BASE_APPLICATION_MAIN(ComponentLoggingApp)

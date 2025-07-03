/*
 * @file logger_config_demo.cpp
 * @brief Demonstration of logger and configuration system integration
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "logger.h"
#include "config.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace base;

void demonstrate_basic_logging() {
    std::cout << "\n=== Basic Logging Demo ===" << std::endl;

    // Basic console logger with colors
    Logger::init();

    Logger::info("Basic logger initialized");
    Logger::debug("Debug message (may not show with default Info level)");
    Logger::warn("Warning message in yellow");
    Logger::error("Error message in red");
    Logger::critical("Critical message in bright red");

    // Change log level to see debug messages
    Logger::set_level(LogLevel::Debug);
    Logger::debug("Debug level enabled - now you can see this message");
}

void demonstrate_advanced_logging() {
    std::cout << "\n=== Advanced Logging Demo ===" << std::endl;

    // Advanced logger configuration
    LoggerConfig config{
        .app_name = "AdvancedDemo",
        .log_file = "logs/advanced_demo.log",
        .max_file_size = 1024 * 1024,  // 1MB
        .max_files = 3,
        .level = LogLevel::Trace,
        .enable_console = true,
        .enable_file = true,
        .enable_colors = true,
        .pattern = "[%H:%M:%S.%e] [%n] [%^%8l%$] [%s:%#] %v"
    };

    Logger::init(config);

    Logger::trace("Trace level message with source location");
    Logger::debug("Debug message with file and line info");
    Logger::info("Info message goes to both console and file");
    Logger::warn("Warning message with timestamp");
    Logger::error("Error message with full context");

    // Structured logging examples
    std::string username = "john_doe";
    int user_id = 12345;
    double cpu_usage = 67.5;

    Logger::info("User login: name={}, id={}, timestamp={}",
                 username, user_id, std::time(nullptr));
    Logger::warn("High CPU usage detected: {:.1f}%", cpu_usage);
    Logger::debug("Processing request for user {} (ID: {})", username, user_id);
}

void demonstrate_config_integration() {
    std::cout << "\n=== Configuration Integration Demo ===" << std::endl;

    // Create a TOML configuration
    std::string toml_config = R"(
[logger_demo]

[logger_demo.app]
name = "Logger Demo App"
version = "1.2.0"
debug_mode = true

[logger_demo.logging]
level = "debug"
pattern = "[%Y-%m-%d %H:%M:%S] [%n] [%^%l%$] %v"
enable_console = true
enable_file = true
file_path = "logs/config_demo.log"
max_file_size = 2097152
max_files = 2
flush_immediately = true
enable_colors = true

[logger_demo.network]
host = "localhost"
port = 8080
timeout_seconds = 30

[logger_demo.database]
host = "db.example.com"
port = 5432
name = "demo_db"
user = "demo_user"
max_connections = 10
)";

    // Load configuration and configure logger
    auto& config_manager = ConfigManager::instance();
    if (config_manager.load_from_string(toml_config, "logger_demo")) {
        std::cout << "✓ TOML configuration loaded successfully" << std::endl;

        // Get application config
        auto app_config = config_manager.get_app_config("logger_demo");
        std::cout << "App: " << app_config.name << " v" << app_config.version << std::endl;

        // Configure logger from TOML
        if (config_manager.configure_logger("logger_demo")) {
            std::cout << "✓ Logger configured from TOML settings" << std::endl;
        }

        // Now use the configured logger
        Logger::info("Logger now configured from TOML file");
        Logger::debug("Debug logging enabled via configuration");
        Logger::warn("File logging enabled - check logs/config_demo.log");

        // Log some application data
        auto network_config = config_manager.get_network_config("logger_demo");
        Logger::info("Server will listen on {}:{}", network_config.host, network_config.port);

        auto db_host = config_manager.get_value<std::string>("database.host", "logger_demo");
        auto db_port = config_manager.get_value<int>("database.port", "logger_demo");
        if (db_host && db_port) {
            Logger::info("Database connection: {}:{}", *db_host, *db_port);
        }

    } else {
        std::cout << "✗ Failed to load TOML configuration" << std::endl;
    }
}

void demonstrate_multi_threaded_logging() {
    std::cout << "\n=== Multi-threaded Logging Demo ===" << std::endl;

    // Configure logger for multi-threaded demo
    LoggerConfig config{
        .app_name = "MultiThreadDemo",
        .level = LogLevel::Debug,
        .enable_console = true,
        .enable_colors = true,
        .pattern = "[%H:%M:%S.%e] [%n] [%^%l%$] [thread:%t] %v"
    };
    Logger::init(config);

    Logger::info("Starting multi-threaded logging demo");

    // Launch multiple threads that log messages
    std::vector<std::thread> threads;

    for (int i = 0; i < 3; ++i) {
        threads.emplace_back([i]() {
            Logger::info("Thread {} started", i);

            for (int j = 0; j < 5; ++j) {
                Logger::debug("Thread {} processing item {}", i, j);
                std::this_thread::sleep_for(std::chrono::milliseconds(50));

                if (j == 2) {
                    Logger::warn("Thread {} reached checkpoint", i);
                }
            }

            Logger::info("Thread {} completed", i);
        });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    Logger::info("All threads completed successfully");
}

void demonstrate_error_scenarios() {
    std::cout << "\n=== Error Handling Demo ===" << std::endl;

    Logger::init();

    // Simulate various error scenarios
    try {
        Logger::info("Simulating application errors...");

        // Network error simulation
        Logger::error("Failed to connect to server: Connection refused (errno: 111)");

        // File system error
        Logger::error("Cannot write to file '/protected/file.txt': Permission denied");

        // Validation error
        std::string invalid_email = "not-an-email";
        Logger::warn("Invalid email format provided: '{}'", invalid_email);

        // Performance warning
        auto start = std::chrono::high_resolution_clock::now();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        Logger::warn("Slow operation detected: {}ms (threshold: 50ms)", duration.count());

        // Critical system error
        Logger::critical("System memory usage exceeded 95% - initiating emergency procedures");

    } catch (const std::exception& e) {
        Logger::critical("Unhandled exception: {}", e.what());
    }
}

void demonstrate_performance_logging() {
    std::cout << "\n=== Performance Logging Demo ===" << std::endl;

    Logger::init();

    // Measure and log performance metrics
    Logger::info("Starting performance measurement");

    auto start = std::chrono::high_resolution_clock::now();

    // Simulate some work
    for (int i = 0; i < 1000; ++i) {
        if (i % 100 == 0) {
            auto current = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current - start);
            Logger::debug("Progress: {}% ({}ms elapsed)", (i * 100) / 1000, elapsed.count());
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    Logger::info("Performance test completed in {}ms", total_duration.count());

    // Log memory usage (simulated)
    Logger::info("Memory usage: RSS={}MB, VSZ={}MB, Heap={}MB", 45, 128, 32);

    // Log throughput metrics
    double ops_per_second = 1000.0 / (total_duration.count() / 1000.0);
    Logger::info("Throughput: {:.2f} operations/second", ops_per_second);
}

int main() {
    std::cout << "Logger and Configuration System Integration Demo" << std::endl;
    std::cout << "================================================" << std::endl;

    try {
        demonstrate_basic_logging();
        demonstrate_advanced_logging();
        demonstrate_config_integration();
        demonstrate_multi_threaded_logging();
        demonstrate_error_scenarios();
        demonstrate_performance_logging();

        std::cout << "\n=== Demo Complete ===" << std::endl;
        std::cout << "✓ All logging features demonstrated successfully!" << std::endl;
        std::cout << "\nCheck the following files for log output:" << std::endl;
        std::cout << "- logs/advanced_demo.log" << std::endl;
        std::cout << "- logs/config_demo.log" << std::endl;
        std::cout << "\nFor more information, see:" << std::endl;
        std::cout << "- docs/LOGGER_README.md for detailed logger documentation" << std::endl;
        std::cout << "- docs/CONFIG_README.md for configuration system documentation" << std::endl;

    } catch (const std::exception& e) {
        Logger::critical("Demo failed with exception: {}", e.what());
        return 1;
    }

    // Clean shutdown
    Logger::flush();
    Logger::shutdown();

    return 0;
}

/*
 * @file simple_component_test.cpp
 * @brief Simple test for component-level logging
 */

#include <logger.h>
#include <iostream>

using namespace base;

int main() {
    // Initialize logger with component logging enabled
    LoggerConfig config{
        .app_name = "TestApp",
        .level = LogLevel::Trace,
        .enable_console = true,
        .enable_file = false,
        .enable_colors = true,
        .enable_component_logging = true
    };
    Logger::init(config);

    std::cout << "Testing component logging..." << std::endl;

    // Test basic logging
    Logger::info("This is a basic log message");

    // Test component logging
    Logger::info("database", "Database connection established");
    Logger::warn("network", "Network timeout occurred");
    Logger::error("auth", "Authentication failed for user {}", "test_user");

    // Test component filtering
    std::cout << "\nTesting component filtering..." << std::endl;
    Logger::disable_components({"database"});

    Logger::info("This should appear");
    Logger::info("database", "This should NOT appear");
    Logger::info("network", "This should appear");

    Logger::enable_components({"auth"});
    std::cout << "\nOnly auth component enabled..." << std::endl;

    Logger::info("database", "This should NOT appear");
    Logger::info("network", "This should NOT appear");
    Logger::info("auth", "This should appear");

    Logger::clear_component_filters();
    std::cout << "\nAll components enabled..." << std::endl;

    Logger::info("database", "This should appear");
    Logger::info("network", "This should appear");
    Logger::info("auth", "This should appear");

    Logger::shutdown();
    return 0;
}

/*
 * @file debug_component_test.cpp
 * @brief Debug test to isolate the message formatting issue
 */

#include <logger.h>
#include <iostream>
#include <fmt/format.h>

using namespace base;

int main() {
    // Initialize logger with component logging enabled
    LoggerConfig config{
        .app_name = "DebugTest",
        .level = LogLevel::Trace,
        .enable_console = true,
        .enable_file = false,
        .enable_colors = true,
        .enable_component_logging = true
    };
    Logger::init(config);

    std::cout << "=== Testing Component Wrapper API ===" << std::endl;

    // Test basic formatting without component first
    std::cout << "Testing basic message without component..." << std::endl;
    Logger::info("Simple message without component");

    // Test with Component wrapper
    std::cout << "Testing with Component wrapper..." << std::endl;
    Logger::info(Logger::Component("database"), "Database connection established");
    Logger::info(Logger::Component("network"), "Network timeout after {}ms", 5000);
    Logger::info(Logger::Component("auth"), "User {} logged in successfully", "admin");

    // Test component filtering
    std::cout << "\nTesting component filtering..." << std::endl;
    Logger::disable_components({"database"});

    Logger::info(Logger::Component("database"), "This should NOT appear");
    Logger::info(Logger::Component("network"), "This should appear");

    Logger::enable_components({"auth"});
    std::cout << "\nOnly auth component enabled..." << std::endl;

    Logger::info(Logger::Component("database"), "This should NOT appear");
    Logger::info(Logger::Component("network"), "This should NOT appear");
    Logger::info(Logger::Component("auth"), "This should appear");

    Logger::clear_component_filters();
    std::cout << "\nAll components enabled..." << std::endl;

    Logger::info(Logger::Component("database"), "This should appear");
    Logger::info(Logger::Component("network"), "This should appear");
    Logger::info(Logger::Component("auth"), "This should appear");

    Logger::shutdown();
    return 0;
}

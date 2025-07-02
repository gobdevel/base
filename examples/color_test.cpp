/*
 * @file color_test.cpp
 * @brief Test program to de    // Test 3: Logger with colors disabled (for comparison)
    std::cout << "\nTest 3: Logger with colors disabled (for comparison)" << std::endl;
    crux::Logger::shutdown();  // Clean up previous logger
    crux::LoggerConfig config_no_colors{
        .app_name = "no_color_test",
        .enable_console = true,
        .enable_colors = false,
        .pattern = "[%H:%M:%S] [%n] [%l] %v"  // Regular pattern without color markers
    };
    crux::Logger::init(config_no_colors); colored logging
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "logger.h"
#include <iostream>

int main() {
    std::cout << "=== Console Color Test ===" << std::endl;
    std::cout << "Testing ANSI color codes directly first..." << std::endl;

    // Test ANSI colors directly
    std::cout << "\033[31mRed text\033[0m" << std::endl;
    std::cout << "\033[32mGreen text\033[0m" << std::endl;
    std::cout << "\033[33mYellow text\033[0m" << std::endl;
    std::cout << "\033[34mBlue text\033[0m" << std::endl;
    std::cout << "\033[35mMagenta text\033[0m" << std::endl;
    std::cout << "\033[36mCyan text\033[0m" << std::endl;

    std::cout << "\nIf you see colors above, your terminal supports colors." << std::endl;
    std::cout << "If not, your terminal might not support ANSI colors." << std::endl;

    std::cout << "\n=== Logger Color Test ===" << std::endl;

    // Test 1: Default logger with colors enabled
    std::cout << "\nTest 1: Default logger (colors should be enabled)" << std::endl;
    crux::Logger::init();

    crux::Logger::trace("This is a TRACE message (usually not shown by default)");
    crux::Logger::debug("This is a DEBUG message (usually not shown by default)");
    crux::Logger::info("This is an INFO message - should be GREEN");
    crux::Logger::warn("This is a WARNING message - should be YELLOW");
    crux::Logger::error("This is an ERROR message - should be RED");
    crux::Logger::critical("This is a CRITICAL message - should be BRIGHT RED");

    // Test 2: Logger with colors explicitly enabled
    std::cout << "\nTest 2: Logger with colors explicitly enabled" << std::endl;
    crux::Logger::shutdown();  // Clean up previous logger
    crux::LoggerConfig config_with_colors{
        .app_name = "color_test",
        .enable_console = true,
        .enable_colors = true,
        .pattern = "[%H:%M:%S] [%n] [%^%l%$] %v"  // %^%l%$ ensures colored level
    };
    crux::Logger::init(config_with_colors);

    crux::Logger::info("INFO with explicit color config");
    crux::Logger::warn("WARNING with explicit color config");
    crux::Logger::error("ERROR with explicit color config");
    crux::Logger::critical("CRITICAL with explicit color config");

    // Test 3: Logger with colors disabled
    std::cout << "\nTest 3: Logger with colors disabled (for comparison)" << std::endl;
    crux::LoggerConfig config_no_colors{
        .app_name = "no_color_test",
        .enable_console = true,
        .enable_colors = false,
        .pattern = "[%H:%M:%S] [%n] [%l] %v"  // Regular pattern without color markers
    };
    crux::Logger::init(config_no_colors);

    crux::Logger::info("INFO without colors");
    crux::Logger::warn("WARNING without colors");
    crux::Logger::error("ERROR without colors");
    crux::Logger::critical("CRITICAL without colors");

    // Test 4: Enable all log levels and test colors
    std::cout << "\nTest 4: All log levels with colors enabled" << std::endl;
    crux::Logger::shutdown();  // Clean up previous logger
    crux::Logger::init(config_with_colors);
    crux::Logger::set_level(crux::LogLevel::Trace);  // Show all levels

    crux::Logger::trace("TRACE level message");
    crux::Logger::debug("DEBUG level message");
    crux::Logger::info("INFO level message");
    crux::Logger::warn("WARNING level message");
    crux::Logger::error("ERROR level message");
    crux::Logger::critical("CRITICAL level message");

    std::cout << "\n=== Color Test Complete ===" << std::endl;
    std::cout << "Expected colors:" << std::endl;
    std::cout << "- TRACE: gray/white" << std::endl;
    std::cout << "- DEBUG: cyan" << std::endl;
    std::cout << "- INFO: green" << std::endl;
    std::cout << "- WARNING: yellow" << std::endl;
    std::cout << "- ERROR: red" << std::endl;
    std::cout << "- CRITICAL: bright red/magenta" << std::endl;
    std::cout << "\nIf log levels are not colored but ANSI test showed colors," << std::endl;
    std::cout << "there might be an issue with spdlog's color detection." << std::endl;

    return 0;
}
